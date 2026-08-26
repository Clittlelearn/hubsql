#include "parser/parsers/tx_record_module.h"

#include <cppconn/connection.h>

#include "utils/reflect_struct.h"

namespace hubsql {

int TxRecordModule::Process(sql::Connection& conn, const Transaction& tx,
                            uint64_t block_height) {
    TxRecord rec;
    rec.tx_hash      = tx.hash;
    rec.block_height = block_height;
    rec.tx_type      = std::to_string(tx.type);
    if (!tx.utxos.empty()) {
        nlohmann::json utxo_json;
        reflect::Serialize(tx.utxos, utxo_json);
        rec.utxo_json = utxo_json.dump();
    } else {
        rec.utxo_json = "[]";
    }
    repo_.Insert(conn, rec);
    return 1;
}

void TxRecordModule::OnBlockEnd(sql::Connection& conn, uint64_t block_height) {
    // 仅保留最新 keep_heights 个高度（如当前高度 35，保留 16..35）
    if (block_height >= keep_heights_) {
        repo_.Prune(conn, block_height - keep_heights_ + 1);
    }
}

nlohmann::json TxRecordModule::List(const nlohmann::json& filter, int page,
                                    int size) {
    const std::string t = filter.value("type", "");
    auto result         = repo_.Query(t, page, size);

    nlohmann::json list = nlohmann::json::array();
    for (auto& rec : result.items) list.push_back(rec.ToJson());
    return {{"list", list}, {"total", result.total},
            {"page", page}, {"size", size}};
}

nlohmann::json TxRecordModule::Counts() {
    return {{"tx_records", repo_.Count()}};
}

}  // namespace hubsql
