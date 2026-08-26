#pragma once

#include <string>
#include <vector>

#include "model/business/vote_record.h"
#include "model/transaction.h"

namespace hubsql {

// 投票交易解析器（type=13 VOTE）
// data.txInfo: {"voteHash": "0xOHI" 或 "0x<提案hash>", "voteTxType": 11,
//               "voteType": 0/1, "voteNumber": 票数}
class VoteParser {
public:
    std::string GetTxType() const { return "13"; }

    std::vector<VoteRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
