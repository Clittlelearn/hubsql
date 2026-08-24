#include "model/business/unstake_record.h"

namespace hubsql {

nlohmann::json UnstakeRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]        = tx_hash;
    j["block_height"]   = block_height;
    j["address"]        = address;
    j["stake_tx_hash"]  = stake_tx_hash;
    j["unstake_time"]   = time;
    return j;
}

}  // namespace hubsql
