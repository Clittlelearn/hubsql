#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

struct HttpConfig {
    int timeout_ms{10000};
    int retry{3};
    int max_retry_wait_ms{60000};
};

struct ChainConfig {
    std::string base_url;
    uint64_t start_height{0};
    int sync_interval_seconds{3};
    int batch_size{50};
    HttpConfig http;
};

struct MysqlConfig {
    std::string host{"127.0.0.1"};
    int port{3306};
    std::string user;
    std::string password;
    std::string database;
    int pool_size{10};
};

struct ApiConfig {
    std::string host{"0.0.0.0"};
    int port{8080};
    int threads{4};
};

struct LogConfig {
    std::string level{"info"};
    std::string file{"./logs/hubsql.log"};
};

struct AppConfig {
    ChainConfig chain;
    MysqlConfig mysql;
    ApiConfig api;
    LogConfig log;
    std::string utxo_db_path{"./data/utxo"};  // RocksDB UTXO 存储路径

    // 从 JSON 对象解析配置
    static AppConfig FromJson(const nlohmann::json& j);

    // 从配置文件加载配置，解析失败抛异常
    static AppConfig LoadFromFile(const std::string& path);
};

}  // namespace hubsql
