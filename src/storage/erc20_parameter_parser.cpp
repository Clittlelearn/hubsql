#include "storage/erc20_parameter_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

namespace hubsql {
namespace {

constexpr const char* kZeroAddress =
    "0x0000000000000000000000000000000000000000";

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return value;
}

std::string NormalizeHex(std::string value) {
    value = Lower(std::move(value));
    if (value.rfind("0x", 0) == 0) value.erase(0, 2);
    if (!std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return std::isxdigit(c);
        })) {
        return {};
    }
    return value;
}

std::string HexToDec(const std::string& value) {
    boost::multiprecision::cpp_int number = 0;
    for (char c : value) {
        const int digit = c >= '0' && c <= '9' ? c - '0'
                        : c >= 'a' && c <= 'f' ? c - 'a' + 10
                        : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if (digit < 0) return {};
        number = number * 16 + digit;
    }
    return number.convert_to<std::string>();
}

bool WordToSize(const std::string& word, std::size_t& result) {
    if (word.size() != 64) return false;
    boost::multiprecision::cpp_int value = 0;
    for (char c : word) {
        const int digit = c >= '0' && c <= '9' ? c - '0'
                        : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        if (digit < 0) return false;
        value = value * 16 + digit;
    }
    if (value > std::numeric_limits<std::size_t>::max()) return false;
    result = value.convert_to<std::size_t>();
    return true;
}

bool IsAddressWord(const std::string& word) {
    return word.size() == 64 &&
           word.compare(0, 24, std::string(24, '0')) == 0;
}

std::string AddressFromWord(const std::string& word) {
    return IsAddressWord(word) ? "0x" + word.substr(24) : std::string();
}

std::optional<Erc20ParameterTransfer> ParseDeployment(
    const nlohmann::json& info, const std::string& input) {
    const std::string contract = Lower(info.value("recipient", ""));
    if (contract.empty() || input.size() < 7 * 64) return std::nullopt;

    // 当前链的 ERC20 构造参数为：
    // (string name,string symbol,uint8 decimals,uint256 supply,string url,
    //  address owner,address flowManager)。参数附加在部署字节码末尾。
    for (std::size_t start = 0; start + 7 * 64 <= input.size(); start += 2) {
        auto word = [&](std::size_t index) {
            return input.substr(start + index * 64, 64);
        };
        std::size_t name_offset = 0, symbol_offset = 0, url_offset = 0;
        std::size_t decimals = 0;
        if (!WordToSize(word(0), name_offset) || name_offset != 7 * 32 ||
            !WordToSize(word(1), symbol_offset) ||
            !WordToSize(word(2), decimals) || decimals > 255 ||
            !WordToSize(word(4), url_offset) ||
            !IsAddressWord(word(5)) || !IsAddressWord(word(6)) ||
            !(name_offset < symbol_offset && symbol_offset < url_offset) ||
            name_offset % 32 != 0 || symbol_offset % 32 != 0 ||
            url_offset % 32 != 0) {
            continue;
        }

        const std::size_t abi_hex_size = input.size() - start;
        auto dynamic_end = [&](std::size_t offset,
                               std::size_t& padded_end) -> bool {
            const std::size_t length_pos = offset * 2;
            if (length_pos + 64 > abi_hex_size) return false;
            std::size_t length = 0;
            if (!WordToSize(input.substr(start + length_pos, 64), length)) return false;
            if (length > (abi_hex_size - length_pos - 64) / 2) return false;
            const std::size_t padded = ((length + 31) / 32) * 64;
            padded_end = length_pos + 64 + padded;
            return padded_end <= abi_hex_size;
        };

        std::size_t name_end = 0, symbol_end = 0, url_end = 0;
        if (!dynamic_end(name_offset, name_end) ||
            !dynamic_end(symbol_offset, symbol_end) ||
            !dynamic_end(url_offset, url_end) ||
            name_end != symbol_offset * 2 ||
            symbol_end != url_offset * 2 || url_end != abi_hex_size) {
            continue;
        }

        const std::string amount = HexToDec(word(3));
        const std::string owner = AddressFromWord(word(5));
        if (amount.empty() || amount == "0" || owner.empty()) return std::nullopt;
        return Erc20ParameterTransfer{contract, kZeroAddress, owner, amount};
    }
    return std::nullopt;
}

}  // namespace

std::optional<Erc20ParameterTransfer> ParseErc20ParameterTransfer(
    const Transaction& tx, const nlohmann::json& info) {
    if (tx.type != 7 && tx.type != 8) return std::nullopt;

    const std::string input = NormalizeHex(info.value("input", ""));
    if (input.empty()) return std::nullopt;
    if (tx.type == 7) return ParseDeployment(info, input);

    const std::string contract = Lower(info.value("recipient", ""));
    const std::string sender = Lower(info.value("sender", tx.identity));
    if (contract.empty()) return std::nullopt;

    Erc20ParameterTransfer result{contract, "", "", ""};
    if (input.size() >= 8 + 64 * 2 && input.substr(0, 8) == "a9059cbb") {
        result.from = sender;
        result.to = AddressFromWord(input.substr(8, 64));
        result.amount = HexToDec(input.substr(8 + 64, 64));
    } else if (input.size() >= 8 + 64 * 3 &&
               input.substr(0, 8) == "23b872dd") {
        result.from = AddressFromWord(input.substr(8, 64));
        result.to = AddressFromWord(input.substr(8 + 64, 64));
        result.amount = HexToDec(input.substr(8 + 64 * 2, 64));
    } else if (input.size() >= 8 + 64 && input.substr(0, 8) == "388814b3" &&
               info.value("callType", "") == "FlowInTx") {
        result.from = sender;
        result.to = kZeroAddress;
        result.amount = HexToDec(input.substr(8, 64));
    } else if (input.size() >= 8 + 64 * 2 &&
               input.substr(0, 8) == "8e0cb3c1" &&
               info.value("callType", "") == "FlowOutTx") {
        result.from = kZeroAddress;
        result.to = AddressFromWord(input.substr(8, 64));
        result.amount = HexToDec(input.substr(8 + 64, 64));
    } else {
        return std::nullopt;
    }

    if (result.from.empty() || result.to.empty() || result.amount.empty() ||
        result.amount == "0") {
        return std::nullopt;
    }
    return result;
}

}  // namespace hubsql
