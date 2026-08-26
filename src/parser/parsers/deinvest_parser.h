#pragma once

#include <string>
#include <vector>

#include "model/business/deinvest_record.h"
#include "model/transaction.h"

namespace hubsql {

// 解投资交易解析器（type=5 UNDELEGATE）
// data.txInfo: {"bonusAddr": "...", "undelegatingUtxo": "0x..."} 指向被解的投资交易
class DeinvestParser {
public:
    std::string GetTxType() const { return "5"; }

    std::vector<DeinvestRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
