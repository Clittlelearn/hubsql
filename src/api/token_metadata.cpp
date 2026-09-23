#include "api/token_metadata.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace hubsql {
namespace {
std::string Hex(const std::string& value) {
    if (value.size() < 66 || value.size() > 65538 || value.size() % 2 ||
        value.rfind("0x", 0) != 0 ||
        !std::all_of(value.begin() + 2, value.end(), [](unsigned char c) { return std::isxdigit(c); }))
        throw std::runtime_error("Invalid token ABI response");
    return value.substr(2);
}
std::size_t SizeWord(const std::string& hex, std::size_t pos) {
    if (pos > hex.size() || hex.size() - pos < 64 || hex.substr(pos, 56) != std::string(56, '0'))
        throw std::runtime_error("Token ABI offset out of range");
    return std::stoul(hex.substr(pos + 56, 8), nullptr, 16);
}
}

std::string DecodeTokenText(const std::string& value) {
    const auto hex = Hex(value);
    std::size_t start = 0, length = 32;
    if (hex.size() != 64) {
        if (SizeWord(hex, 0) != 32) throw std::runtime_error("Invalid token string offset");
        length = SizeWord(hex, 64);
        start = 128;
        if (length > 1024 || start + length * 2 > hex.size())
            throw std::runtime_error("Invalid token string length");
    }
    std::string text;
    for (std::size_t i = 0; i < length; ++i) {
        char c = static_cast<char>(std::stoul(hex.substr(start + i * 2, 2), nullptr, 16));
        if (c == 0 && hex.size() == 64) break;
        if (static_cast<unsigned char>(c) < 32 || c == 127)
            throw std::runtime_error("Invalid token text");
        text += c;
    }
    if (text.empty()) throw std::runtime_error("Empty token text");
    // Reject malformed UTF-8 before this data reaches an HTTP JSON response.
    (void)nlohmann::json(text).dump();
    return text;
}

unsigned DecodeTokenDecimals(const std::string& value) {
    const auto hex = Hex(value);
    if (hex.size() != 64) throw std::runtime_error("Invalid token decimals");
    const auto decimals = SizeWord(hex, 0);
    if (decimals > 255) throw std::runtime_error("Invalid token decimals");
    return static_cast<unsigned>(decimals);
}

TokenMetadataReader::TokenMetadataReader(std::string rpc_url)
    : rpc_url_(std::move(rpc_url)), http_(HttpConfig{1500, 0, 0}) {}

std::string TokenMetadataReader::Call(const std::string& contract, const std::string& selector) {
    const nlohmann::json request = {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "eth_call"},
        {"params", nlohmann::json::array({{{"to", contract}, {"data", selector}}, "latest"})}};
    const auto response = nlohmann::json::parse(http_.Post(rpc_url_, request.dump()));
    if (response.contains("error") || !response.contains("result") || !response["result"].is_string())
        throw std::runtime_error("Token metadata unavailable from node");
    return response["result"].get<std::string>();
}

nlohmann::json TokenMetadataReader::Read(const std::string& contract) {
    std::shared_ptr<Entry> entry;
    {
        std::lock_guard lock(mutex_);
        if (!cache_.contains(contract)) {
            if (cache_.size() >= 512) cache_.erase(cache_.begin());
            cache_[contract] = std::make_shared<Entry>();
        }
        entry = cache_.at(contract);
    }
    std::lock_guard lock(entry->mutex);
    if (entry->expires <= std::chrono::steady_clock::now()) {
        try {
            entry->data = {{"name", DecodeTokenText(Call(contract, "0x06fdde03"))},
                {"symbol", DecodeTokenText(Call(contract, "0x95d89b41"))},
                {"decimals", DecodeTokenDecimals(Call(contract, "0x313ce567"))}};
            entry->error.clear();
            entry->expires = std::chrono::steady_clock::now() + std::chrono::minutes(5);
        } catch (const std::exception&) {
            entry->error = "Token metadata unavailable; retry later";
            entry->expires = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        }
    }
    if (!entry->error.empty()) throw std::runtime_error(entry->error);
    return entry->data;
}
}
