#pragma once

#include <string>
#include <vector>

#include "model/business/lock_record.h"
#include "model/transaction.h"

namespace hubsql {

// 锁定交易解析器（type=9 LOCK）
// data.txInfo: {"lockAmount": ..., "lockType": "LockNet"}; assetType 取自 utxo
class LockParser {
public:
    std::string GetTxType() const { return "9"; }

    std::vector<LockRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
