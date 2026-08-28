#include "parser/parsers/fund_parser.h"

#include <algorithm>
#include <cctype>

#include <boost/multiprecision/cpp_int.hpp>
#include <nlohmann/json.hpp>

namespace hubsql {
namespace {

std::string HexToDecimal(const std::string& hex) {
    boost::multiprecision::cpp_int value = 0;
    for (char c : hex) {
        value <<= 4;
        if (c >= '0' && c <= '9') value += c - '0';
        else if (c >= 'a' && c <= 'f') value += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') value += c - 'A' + 10;
        else return {};
    }
    return value.convert_to<std::string>();
}

}  // namespace

std::vector<FundRecord> FundParser::Parse(const Transaction& tx) {
    if (tx.type != 8) return {};

    nlohmann::json ti;
    try {
        ti = nlohmann::json::parse(tx.data).value(
            "txInfo", nlohmann::json::object());
    } catch (...) {
        return {};
    }

    std::string input = ti.value("input", "");
    if (input.rfind("0x", 0) == 0) input.erase(0, 2);
    std::transform(input.begin(), input.end(), input.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // transfer(address,uint256): 4-byte selector + two 32-byte ABI words.
    if (input.size() < 8 + 64 + 64 || input.substr(0, 8) != "a9059cbb") {
        return {};
    }

    const std::string address_word = input.substr(8, 64);
    const std::string amount_word  = input.substr(72, 64);
    const std::string amount       = HexToDecimal(amount_word);
    if (amount.empty()) return {};

    FundRecord rec;
    rec.tx_hash          = tx.hash;
    rec.sender           = ti.value("sender", tx.identity);
    rec.recipient        = "0x" + address_word.substr(24);
    rec.contract_address = ti.value("recipient", "");
    rec.amount           = amount;
    rec.time             = tx.time;
    return {rec};
}

}  // namespace hubsql

