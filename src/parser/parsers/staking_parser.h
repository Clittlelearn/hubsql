#pragma once

#include <string>
#include <vector>

#include "model/business/stake_record.h"
#include "model/transaction.h"

namespace hubsql {

// 质押交易解析器（type=2 STAKE）
// data.txInfo: {"stakeAmount": ..., "commissionRate": 0.07, "stakeType": "Net"}
class StakingParser {
public:
    std::string GetTxType() const { return "2"; }

    std::vector<StakeRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
