#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "parser/ibusiness_module.h"
#include "parser/parsers/claim_parser.h"
#include "storage/claim_repo.h"

namespace hubsql {

// 申领业务模块：解析 + 存储 一体，注册进 BusinessRegistry（处理 BONUS=99）
class ClaimModule final : public IBusinessModule {
public:
    explicit ClaimModule(ClaimRepo& repo) : repo_(repo) {}

    std::string Name() const override { return "claim"; }

    bool Handles(const Transaction& tx) const override { return tx.type == 99; }

    int Process(sql::Connection& conn, const Transaction& tx,
                uint64_t block_height) override;

    nlohmann::json List(const nlohmann::json& filter, int page, int size) override;
    nlohmann::json Counts() override;

private:
    ClaimRepo& repo_;
    ClaimParser claim_parser_;
};

}  // namespace hubsql
