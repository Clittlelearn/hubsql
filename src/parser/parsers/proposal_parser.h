#pragma once

#include <string>
#include <vector>

#include "model/business/proposal_record.h"
#include "model/transaction.h"

namespace hubsql {

// 提案交易解析器（type=11 PROPOSAL）
// data.txInfo: 完整提案信息（name/title/minVoteNum/...），整体存入 tx_info 字段
class ProposalParser {
public:
    std::string GetTxType() const { return "11"; }

    std::vector<ProposalRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
