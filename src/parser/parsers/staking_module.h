#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "parser/ibusiness_module.h"
#include "parser/parsers/staking_parser.h"
#include "parser/parsers/unstaking_parser.h"
#include "storage/staking_repo.h"

namespace hubsql {

// 质押业务模块：解析 + 存储 一体，注册进 BusinessRegistry（处理 STAKE=2 / UNSTAKE=3）
class StakingModule final : public IBusinessModule {
public:
    explicit StakingModule(StakingRepo& repo) : repo_(repo) {}

    std::string Name() const override { return "staking"; }

    bool Handles(const Transaction& tx) const override {
        return tx.type == 2 || tx.type == 3;
    }

    int Process(sql::Connection& conn, const Transaction& tx,
                uint64_t block_height) override;

    nlohmann::json List(const nlohmann::json& filter, int page, int size) override;
    nlohmann::json Counts() override;

private:
    StakingRepo& repo_;
    StakingParser staking_parser_;
    UnstakingParser unstaking_parser_;
};

}  // namespace hubsql
