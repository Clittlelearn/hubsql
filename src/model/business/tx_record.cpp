#include "model/business/tx_record.h"

namespace hubsql {

nlohmann::json TxRecord::ToJson() const {
    nlohmann::json j;
    j["tx_hash"]      = tx_hash;
    j["block_height"] = block_height;
    j["tx_type"]      = tx_type;
    if (!utxo_json.empty()) {
        // 保留数据库中的原始 JSON 文本，供客户端按需展开查看。
        j["utxo_raw"] = utxo_json;
        try {
            j["utxo"] = nlohmann::json::parse(utxo_json);
        } catch (...) {
            j["utxo"] = utxo_json;
        }
    }
    return j;
}

}  // namespace hubsql
