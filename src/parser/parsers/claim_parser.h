#pragma once

#include <string>
#include <vector>

#include "model/business/claim_record.h"
#include "model/transaction.h"

namespace hubsql {

// 申领交易解析器（type=99 BONUS）
// data.txInfo: {"bonusAmount": ..., ...}; assetType 取自 utxo
class ClaimParser {
public:
    std::string GetTxType() const { return "99"; }

    std::vector<ClaimRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
