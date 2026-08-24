#include "api/controllers/balance_controller.h"

#include "api/response.h"

namespace hubsql {

BalanceController::BalanceController(BalanceRepo& repo) : repo_(repo) {}

nlohmann::json BalanceController::List(int page, int size) {
    try {
        auto all = repo_.ListAll();
        nlohmann::json list = nlohmann::json::array();
        int start = (page - 1) * size;
        for (int i = start; i < start + size && i < static_cast<int>(all.size()); ++i) {
            list.push_back({{"address", all[i].first}, {"balance", all[i].second}});
        }
        return Ok({{"list", list},
                   {"total", static_cast<int64_t>(all.size())},
                   {"page", page},
                   {"size", size}});
    } catch (const std::exception& e) {
        return Err(500, e.what());
    }
}

nlohmann::json BalanceController::Get(const std::string& address) {
    try {
        auto bal = repo_.GetBalance(address);
        if (!bal) return Err(404, "address not found");
        return Ok({{"address", address}, {"balance", *bal}});
    } catch (const std::exception& e) {
        return Err(500, e.what());
    }
}

}  // namespace hubsql
