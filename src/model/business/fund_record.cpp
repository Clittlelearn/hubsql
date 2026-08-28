#include "model/business/fund_record.h"

namespace hubsql {

nlohmann::json FundRecord::ToJson() const {
    return {{"tx_hash", tx_hash},
            {"block_height", block_height},
            {"sender", sender},
            {"recipient", recipient},
            {"contract_address", contract_address},
            {"fund_amount", amount},
            {"fund_time", time}};
}

}  // namespace hubsql

