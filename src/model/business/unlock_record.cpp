#include "model/business/unlock_record.h"

namespace hubsql {

nlohmann::json UnlockRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]       = tx_hash;
    j["block_height"]  = block_height;
    j["address"]       = address;
    j["lock_tx_hash"]  = lock_tx_hash;
    j["unlock_time"]   = time;
    return j;
}

}  // namespace hubsql
