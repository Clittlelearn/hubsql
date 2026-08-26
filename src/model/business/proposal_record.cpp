#include "model/business/proposal_record.h"

namespace hubsql {

nlohmann::json ProposalRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]        = tx_hash;
    j["asset"]          = asset;
    j["block_height"]   = block_height;
    j["address"]        = address;
    j["vote_count"]     = vote_count;
    j["is_first"]       = is_first;
    j["is_revoked"]     = is_revoked;
    if (!revoke_tx_hash.empty()) j["revoke_tx_hash"] = revoke_tx_hash;
    if (revoke_time > 0) j["revoke_time"] = revoke_time;
    if (!tx_info.empty()) {
        try {
            j["tx_info"] = nlohmann::json::parse(tx_info);
        } catch (...) {
            j["tx_info"] = tx_info;
        }
    }
    return j;
}

}  // namespace hubsql
