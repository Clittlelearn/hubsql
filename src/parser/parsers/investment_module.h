#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "parser/ibusiness_module.h"
#include "parser/parsers/deinvest_parser.h"
#include "parser/parsers/invest_parser.h"
#include "storage/investment_repo.h"

namespace hubsql {

// 投资业务模块：解析 + 存储 一体，注册进 BusinessRegistry（处理 DELEGATE=4 / UNDELEGATE=5）
class InvestmentModule final : public IBusinessModule {
public:
    explicit InvestmentModule(InvestmentRepo& repo) : repo_(repo) {}

    std::string Name() const override { return "investment"; }

    bool Handles(const Transaction& tx) const override {
        return tx.type == 4 || tx.type == 5;
    }

    int Process(sql::Connection& conn, const Transaction& tx,
                uint64_t block_height) override;

    nlohmann::json List(const nlohmann::json& filter, int page, int size) override;
    nlohmann::json Counts() override;

private:
    InvestmentRepo& repo_;
    InvestParser invest_parser_;
    DeinvestParser deinvest_parser_;
};

}  // namespace hubsql
