#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "parser/ibusiness_module.h"
#include "parser/parsers/lock_parser.h"
#include "parser/parsers/unlock_parser.h"
#include "storage/lock_repo.h"

namespace hubsql {

// 锁定业务模块：解析 + 存储 一体，注册进 BusinessRegistry
// 处理 LOCK=9（插入锁定，含资产类型）、UNLOCK=10（仅标记 is_unlocked，不删除）
class LockModule final : public IBusinessModule {
public:
    explicit LockModule(LockRepo& repo) : repo_(repo) {}

    std::string Name() const override { return "lock"; }

    bool Handles(const Transaction& tx) const override {
        return tx.type == 9 || tx.type == 10;
    }

    int Process(sql::Connection& conn, const Transaction& tx,
                uint64_t block_height) override;

    nlohmann::json List(const nlohmann::json& filter, int page, int size) override;
    nlohmann::json Counts() override;

private:
    LockRepo& repo_;
    LockParser lock_parser_;
    UnlockParser unlock_parser_;
};

}  // namespace hubsql
