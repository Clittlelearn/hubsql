#include "model/business/stake_record.h"

namespace hubsql {

nlohmann::json StakeRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]         = tx_hash;
    j["block_height"]    = block_height;
    j["address"]         = address;
    j["stake_amount"]    = amount;
    j["stake_time"]      = time;
    j["commission_rate"] = commission_rate;
    j["stake_type"]      = stake_type;
    j["is_unstaked"]     = is_unstaked;
    if (!unstake_tx_hash.empty()) j["unstake_tx_hash"] = unstake_tx_hash;
    if (unstake_time > 0) j["unstake_time"] = unstake_time;
    return j;
}

}  // namespace hubsql
