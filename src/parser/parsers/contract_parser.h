#pragma once

#include <string>
#include <vector>

#include "model/business/contract_record.h"
#include "model/transaction.h"

namespace hubsql {

// 合约交易解析器（type=7 DEPLOY / type=8 CALL，含跃入 FlowInTx / 跃出 FlowOutTx）
// data.txInfo: {"sender","recipient","callType":"FlowInTx/FlowOutTx",...}
class ContractParser {
public:
    std::string GetTxType() const { return "7"; }  // 7=deploy, 8=call

    std::vector<ContractRecord> Parse(const Transaction& tx);
};

}  // namespace hubsql
