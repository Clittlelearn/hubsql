#include "model/business/vote_record.h"

namespace hubsql {

nlohmann::json VoteRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]        = tx_hash;
    j["block_height"]   = block_height;
    j["address"]        = address;
    j["proposal_hash"]  = proposal_hash;
    j["proposal_type"]  = proposal_type;
    j["vote_type"]      = vote_type;
    j["vote_number"]    = vote_number;
    return j;
}

}  // namespace hubsql
