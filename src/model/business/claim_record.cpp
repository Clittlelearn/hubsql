#include "model/business/claim_record.h"

namespace hubsql {

nlohmann::json ClaimRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]      = tx_hash;
    j["block_height"] = block_height;
    j["address"]      = address;
    j["asset_type"]   = asset_type;
    j["claim_amount"] = amount;
    j["claim_time"]   = time;
    return j;
}

}  // namespace hubsql
