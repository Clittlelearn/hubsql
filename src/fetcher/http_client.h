#pragma once

#include <chrono>
#include <string>

#include "common/config.h"

namespace hubsql {

// 基于 libcurl 的 HTTP 客户端：GET/POST、超时、重试（指数退避）
class HttpClient {
public:
    explicit HttpClient(const HttpConfig& cfg);

    // 发起 GET，成功返回响应体；失败抛异常
    std::string Get(const std::string& url);

    // 发起 POST（JSON-RPC），成功返回响应体；失败抛异常
    std::string Post(const std::string& url, const std::string& body);

private:
    std::string Request(const std::string& url, const std::string& body);

    HttpConfig cfg_;
};

}  // namespace hubsql
