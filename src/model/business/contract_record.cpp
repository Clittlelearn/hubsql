#include "model/business/contract_record.h"

namespace hubsql {

nlohmann::json ContractRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]         = tx_hash;
    j["block_height"]    = block_height;
    j["address"]         = address;
    j["sender"]          = sender;
    j["recipient"]       = recipient;
    j["tx_type"]         = tx_type;
    j["is_flow_in"]      = is_flow_in;
    j["is_flow_out"]     = is_flow_out;
    j["flow_in_amount"]  = flow_in_amount;
    j["flow_out_amount"] = flow_out_amount;
    j["asset_type"]      = asset_type;
    j["tx_time"]         = time;
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
