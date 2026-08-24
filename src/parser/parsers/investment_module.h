#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "parser/ibusiness_module.h"
#include "parser/parsers/invest_parser.h"
#include "storage/investment_repo.h"

namespace hubsql {

// 投资业务模块：注册进 BusinessRegistry。
// 注意：链上暂无 invest 交易类型（type=3 实为 UNSTAKE），故 Handles 恒为 false，
// 仅注册用于 REST API（/investments）与统计；待链新增投资类型后启用 Handles/Process。
class InvestmentModule final : public IBusinessModule {
public:
    explicit InvestmentModule(InvestmentRepo& repo) : repo_(repo) {}

    std::string Name() const override { return "investment"; }

    bool Handles(const Transaction&) const override { return false; }

    int Process(sql::Connection&, const Transaction&, uint64_t) override { return 0; }

    nlohmann::json List(const nlohmann::json& filter, int page, int size) override;
    nlohmann::json Counts() override;

private:
    InvestmentRepo& repo_;
    InvestParser invest_parser_;
};

}  // namespace hubsql
