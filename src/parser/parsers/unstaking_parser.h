#pragma once

#include <string>
#include <vector>

#include "model/business/unstake_record.h"
#include "model/transaction.h"

namespace hubsql {

// 解质押交易解析器（type=3 UNSTAKE）
// data.txInfo: {"unstakeUtxo": "0x..."} 指向被解质押的质押交易
class UnstakingParser {
public:
    std::string GetTxType() const { return "3"; }

    std::vector<UnstakeRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
