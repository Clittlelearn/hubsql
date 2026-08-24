#pragma once

#include <string>
#include <vector>

#include "model/business/investment_record.h"
#include "model/transaction.h"

namespace hubsql {

// 投资交易解析器（链暂无 invest 类型，保留扩展用；type 标记与 UNSTAKE=3 相同，
// 因此不注册进主流水线，避免把解质押交易误判为投资）
class InvestParser {
public:
    std::string GetTxType() const { return "3"; }

    std::vector<InvestmentRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
