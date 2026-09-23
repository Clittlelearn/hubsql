#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "fetcher/http_client.h"

namespace hubsql {
std::string DecodeTokenText(const std::string& value);
unsigned DecodeTokenDecimals(const std::string& value);

class TokenMetadataReader {
public:
    explicit TokenMetadataReader(std::string rpc_url);
    nlohmann::json Read(const std::string& contract);
private:
    struct Entry {
        std::mutex mutex;
        std::chrono::steady_clock::time_point expires{};
        nlohmann::json data;
        std::string error;
    };
    std::string Call(const std::string& contract, const std::string& selector);
    std::string rpc_url_;
    HttpClient http_;
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Entry>> cache_;
};
}
