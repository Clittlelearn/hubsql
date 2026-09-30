#include <gtest/gtest.h>

#include "common/config.h"

using namespace hubsql;

TEST(ConfigTest, FromJsonParsesValues) {
    nlohmann::json j = R"({
        "chain": { "base_url": "http://node:8545", "start_height": 10 },
        "mysql": { "host": "127.0.0.1", "port": 3306, "user": "u",
                   "password": "p", "database": "d", "pool_size": 5 }
    })"_json;

    auto cfg = AppConfig::FromJson(j);
    EXPECT_EQ(cfg.chain.base_url, "http://node:8545");
    EXPECT_EQ(cfg.chain.start_height, 10u);
    EXPECT_EQ(cfg.chain.sync_interval_seconds, 3);  // 默认值
    EXPECT_EQ(cfg.mysql.host, "127.0.0.1");
    EXPECT_EQ(cfg.mysql.port, 3306);
    EXPECT_EQ(cfg.mysql.user, "u");
    EXPECT_EQ(cfg.mysql.pool_size, 5);
    EXPECT_EQ(cfg.api.port, 8080);  // 默认值
    EXPECT_EQ(cfg.log.level, "info");
}

TEST(ConfigTest, MissingSectionsUseDefaults) {
    nlohmann::json j = R"({})"_json;
    auto cfg = AppConfig::FromJson(j);
    EXPECT_EQ(cfg.mysql.host, "127.0.0.1");
    EXPECT_EQ(cfg.api.threads, 4);
    EXPECT_EQ(cfg.chain.http.timeout_ms, 10000);
    EXPECT_EQ(cfg.mysql.pool_acquire_timeout_ms, 5000);
    EXPECT_EQ(cfg.mysql.connect_timeout_seconds, 5);
    EXPECT_EQ(cfg.mysql.read_timeout_seconds, 30);
    EXPECT_EQ(cfg.mysql.write_timeout_seconds, 30);
}

TEST(ConfigTest, MysqlPoolTimeoutsAreConfigurable) {
    auto cfg = AppConfig::FromJson(R"({"mysql": {
        "pool_acquire_timeout_ms": 1500, "connect_timeout_seconds": 2,
        "read_timeout_seconds": 60, "write_timeout_seconds": 45
    }})"_json);
    EXPECT_EQ(cfg.mysql.pool_acquire_timeout_ms, 1500);
    EXPECT_EQ(cfg.mysql.connect_timeout_seconds, 2);
    EXPECT_EQ(cfg.mysql.read_timeout_seconds, 60);
    EXPECT_EQ(cfg.mysql.write_timeout_seconds, 45);
}

TEST(ConfigTest, LoadFromFileWorks) {
    // config/config.json 相对于运行目录；跳过文件不存在的情况
    auto cfg = AppConfig::LoadFromFile("../../config/config.json");
    EXPECT_FALSE(cfg.mysql.database.empty());
}
