#include "model/business/deinvest_record.h"

namespace hubsql {

nlohmann::json DeinvestRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]        = tx_hash;
    j["block_height"]   = block_height;
    j["address"]        = address;
    j["invest_tx_hash"] = invest_tx_hash;
    j["deinvest_time"]  = time;
    return j;
}

}  // namespace hubsql
