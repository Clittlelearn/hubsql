#include "model/business/revoke_record.h"

namespace hubsql {

nlohmann::json RevokeRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]        = tx_hash;
    j["block_height"]   = block_height;
    j["address"]        = address;
    j["proposal_hash"]  = proposal_hash;
    j["revoke_time"]    = time;
    return j;
}

}  // namespace hubsql
