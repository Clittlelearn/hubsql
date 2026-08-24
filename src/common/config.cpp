#include "common/config.h"

#include <fstream>
#include <stdexcept>

#include "common/json_utils.h"

namespace hubsql {

AppConfig AppConfig::FromJson(const nlohmann::json& j) {
    AppConfig cfg;

    const auto& chain = j.value("chain", nlohmann::json::object());
    cfg.chain.base_url             = chain.value("base_url", "");
    cfg.chain.start_height         = chain.value("start_height", 0ULL);
    cfg.chain.sync_interval_seconds= chain.value("sync_interval_seconds", 3);
    cfg.chain.batch_size           = chain.value("batch_size", 50);
    const auto& http = chain.value("http", nlohmann::json::object());
    cfg.chain.http.timeout_ms       = http.value("timeout_ms", 10000);
    cfg.chain.http.retry            = http.value("retry", 3);
    cfg.chain.http.max_retry_wait_ms= http.value("max_retry_wait_ms", 60000);

    const auto& mysql = j.value("mysql", nlohmann::json::object());
    cfg.mysql.host     = mysql.value("host", "127.0.0.1");
    cfg.mysql.port     = mysql.value("port", 3306);
    cfg.mysql.user     = mysql.value("user", "");
    cfg.mysql.password = mysql.value("password", "");
    cfg.mysql.database = mysql.value("database", "");
    cfg.mysql.pool_size= mysql.value("pool_size", 10);

    const auto& api = j.value("api", nlohmann::json::object());
    cfg.api.host    = api.value("host", "0.0.0.0");
    cfg.api.port    = api.value("port", 8080);
    cfg.api.threads = api.value("threads", 4);

    const auto& log = j.value("log", nlohmann::json::object());
    cfg.log.level = log.value("level", "info");
    cfg.log.file  = log.value("file", "./logs/hubsql.log");

    const auto& rocksdb = j.value("rocksdb", nlohmann::json::object());
    cfg.utxo_db_path = rocksdb.value("path", "./data/utxo");

    return cfg;
}

AppConfig AppConfig::LoadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("无法打开配置文件: " + path);
    }
    nlohmann::json j;
    try {
        in >> j;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("配置文件 JSON 解析失败: " + std::string(e.what()));
    }
    return FromJson(j);
}

}  // namespace hubsql
