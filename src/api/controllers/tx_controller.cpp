#include "api/controllers/tx_controller.h"

#include "api/response.h"

namespace hubsql {

TxController::TxController(TxRepo& repo) : repo_(repo) {}

nlohmann::json TxController::List(const std::string& type, const std::string& address,
                                  int page, int size) {
    try {
        auto result = repo_.Query(type, address, page, size);
        nlohmann::json list = nlohmann::json::array();
        for (const auto& tx : result.items) {
            list.push_back(tx.ToJson());
        }
        return Ok({{"list", list}, {"total", result.total}, {"page", page}, {"size", size}});
    } catch (const std::exception& e) {
        return Err(500, e.what());
    }
}

nlohmann::json TxController::Detail(const std::string& tx_hash) {
    try {
        auto tx = repo_.FindByHash(tx_hash);
        if (!tx) return Err(404, "transaction not found");
        return Ok(tx->ToJson());
    } catch (const std::exception& e) {
        return Err(500, e.what());
    }
}

}  // namespace hubsql
