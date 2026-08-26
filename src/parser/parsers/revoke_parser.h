#pragma once

#include <string>
#include <vector>

#include "model/business/revoke_record.h"
#include "model/transaction.h"

namespace hubsql {

// 撤销提案交易解析器（type=12 REVOKEPROPOSAL）
// data.txInfo: {"proposalHash": "0xOHI" 或 "0x<提案hash>", ...}
class RevokeParser {
public:
    std::string GetTxType() const { return "12"; }

    std::vector<RevokeRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
