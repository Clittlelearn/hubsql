#pragma once

#include <string>
#include <vector>

#include "model/business/unlock_record.h"
#include "model/transaction.h"

namespace hubsql {

// 解锁定交易解析器（type=10 UNLOCK）
// data.txInfo: {"unLockUtxo": "0x..."} 指向被解锁的锁定交易
class UnlockParser {
public:
    std::string GetTxType() const { return "10"; }

    std::vector<UnlockRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
