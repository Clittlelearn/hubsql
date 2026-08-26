#include "model/business/tx_record.h"

namespace hubsql {

nlohmann::json TxRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]      = tx_hash;
    j["block_height"] = block_height;
    j["tx_type"]      = tx_type;
    if (!utxo_json.empty()) {
        try {
            j["utxo"] = nlohmann::json::parse(utxo_json);
        } catch (...) {
            j["utxo"] = utxo_json;
        }
    }
    return j;
}

}  // namespace hubsql
