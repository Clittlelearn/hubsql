#include "api/controllers/business_controller.h"

#include "api/response.h"

namespace hubsql {

BusinessController::BusinessController(BusinessRegistry& registry,
                                       BlockRepo& blocks, TxRepo& txs)
    : registry_(registry), blocks_(blocks), txs_(txs) {}

nlohmann::json BusinessController::List(const std::string& module,
                                        const nlohmann::json& filter,
                                        int page, int size) {
    auto* m = registry_.Find(module);
    if (!m) return Err(404, "unknown business module: " + module);
    try {
        return Ok(m->List(filter, page, size));
    } catch (const std::exception& e) {
        return Err(500, e.what());
    }
}

nlohmann::json BusinessController::Stats() {
    try {
        nlohmann::json counts;
        for (auto* m : registry_.All()) {
            auto c = m->Counts();
            for (auto it = c.begin(); it != c.end(); ++it) {
                counts[it.key()] = it.value();
            }
        }
        return Ok({
            {"total_blocks", blocks_.Count()},
            {"total_txs", txs_.Count()},
            {"business_counts", counts},
        });
    } catch (const std::exception& e) {
        return Err(500, e.what());
    }
}

}  // namespace hubsql
