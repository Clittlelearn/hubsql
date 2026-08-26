#include "model/business/lock_record.h"

namespace hubsql {

nlohmann::json LockRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]         = tx_hash;
    j["block_height"]    = block_height;
    j["address"]         = address;
    j["asset_type"]      = asset_type;
    j["lock_amount"]     = amount;
    j["lock_time"]       = time;
    j["lock_type"]       = lock_type;
    j["is_unlocked"]     = is_unlocked;
    if (!unlock_tx_hash.empty()) j["unlock_tx_hash"] = unlock_tx_hash;
    if (unlock_time > 0) j["unlock_time"] = unlock_time;
    return j;
}

}  // namespace hubsql
