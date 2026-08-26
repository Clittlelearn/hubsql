#pragma once

#include <string>
#include <vector>

#include "model/business/investment_record.h"
#include "model/transaction.h"

namespace hubsql {

// 投资交易解析器（type=4 DELEGATE）
// data.txInfo: {"bonusAddr": "...", "delegateAmount": ..., "delegateType": "Normal"}
class InvestParser {
public:
    std::string GetTxType() const { return "4"; }

    std::vector<InvestmentRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
