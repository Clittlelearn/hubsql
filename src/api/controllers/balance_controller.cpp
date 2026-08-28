#include "api/controllers/balance_controller.h"

#include "api/response.h"
#include <algorithm>
#include <cctype>

namespace hubsql {

BalanceController::BalanceController(BalanceRepo& repo) : repo_(repo) {}

nlohmann::json BalanceController::List(const std::string& asset_type,
                                       int page, int size) {
    try {
        auto all = repo_.ListAll(asset_type);
        nlohmann::json list = nlohmann::json::array();
        int start = (page - 1) * size;
        for (int i = start; i < start + size && i < static_cast<int>(all.size()); ++i) {
            list.push_back({{"address", all[i].address},
                            {"asset_type", all[i].asset_type},
                            {"balance", all[i].balance}});
        }
        return Ok({{"list", list},
                   {"total", static_cast<int64_t>(all.size())},
                   {"page", page},
                   {"size", size}});
    } catch (const std::exception& e) {
        return Err(500, e.what());
    }
}

nlohmann::json BalanceController::AssociateErc20(
    const std::string& address, const std::string& contract_address) {
    auto valid = [](const std::string& value) {
        return value.size() == 42 && value.rfind("0x", 0) == 0 &&
               std::all_of(value.begin() + 2, value.end(),
                           [](unsigned char c) { return std::isxdigit(c); });
    };
    if (!valid(address) || !valid(contract_address))
        return Err(400, "address and contract_address must be 0x-prefixed 20-byte addresses");
    try {
        if (!repo_.AssociateErc20(address, contract_address))
            return Err(404, "ERC20 contract not found on chain");
        return Ok({{"address", address}, {"contract_address", contract_address}});
    } catch (const std::exception& e) { return Err(500, e.what()); }
}

nlohmann::json BalanceController::ListErc20(const std::string& address) {
    try {
        nlohmann::json balances = nlohmann::json::array();
        for (const auto& item : repo_.ListErc20(address))
            balances.push_back({{"contract_address", item.contract_address},
                                {"balance", item.balance}});
        return Ok({{"address", address}, {"balances", balances}});
    } catch (const std::exception& e) { return Err(500, e.what()); }
}

nlohmann::json BalanceController::RemoveErc20(
    const std::string& address, const std::string& contract_address) {
    try {
        return Ok({{"removed", repo_.RemoveErc20(address, contract_address)},
                   {"address", address}, {"contract_address", contract_address}});
    } catch (const std::exception& e) { return Err(500, e.what()); }
}

nlohmann::json BalanceController::AssetCatalog(const std::string& address) {
    try {
        nlohmann::json list = nlohmann::json::array();
        for (const auto& item : repo_.ListAssetCatalog(address)) {
            list.push_back({{"kind", item.kind}, {"asset_id", item.asset_id},
                            {"name", item.name}, {"symbol", item.symbol},
                            {"contract_address", item.contract_address},
                            {"asset_type", item.asset_type}, {"decimals", item.decimals},
                            {"is_added", item.is_added}});
        }
        return Ok({{"list", list}, {"total", list.size()}});
    } catch (const std::exception& e) { return Err(500, e.what()); }
}

nlohmann::json BalanceController::Get(const std::string& address,
                                      const std::string& asset_type) {
    try {
        if (!asset_type.empty()) {
            auto bal = repo_.GetBalance(address, asset_type);
            if (!bal) return Err(404, "address not found");
            return Ok({{"address", address},
                       {"asset_type", asset_type},
                       {"balance", *bal}});
        }
        // 未指定资产：返回该地址全部资产余额
        auto all = repo_.ListAll();
        nlohmann::json assets = nlohmann::json::array();
        bool found = false;
        for (const auto& item : all) {
            if (item.address == address) {
                assets.push_back({{"asset_type", item.asset_type},
                                  {"balance", item.balance}});
                found = true;
            }
        }
        if (!found) return Err(404, "address not found");
        return Ok({{"address", address}, {"balances", assets}});
    } catch (const std::exception& e) {
        return Err(500, e.what());
    }
}

}  // namespace hubsql
