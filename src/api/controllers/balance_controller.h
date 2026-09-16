#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "storage/balance_repo.h"

namespace hubsql {

// 账户余额查询控制器（按地址 + 资产类型）
class BalanceController {
public:
    explicit BalanceController(BalanceRepo& repo);

    // 全部余额（分页，按余额降序）；asset_type 为空 = 全部资产
    nlohmann::json List(const std::string& asset_type, int page, int size);

    // 单地址余额；asset_type 为空 = 返回该地址全部资产，否则返回指定资产
    nlohmann::json Get(const std::string& address, const std::string& asset_type);

    nlohmann::json AssociateErc20(const std::string& address,
                                  const std::string& contract_address);
    nlohmann::json RemoveErc20(const std::string& address,
                               const std::string& contract_address);
    nlohmann::json ListErc20(const std::string& address);
    nlohmann::json ListIndexedErc20(const std::string& address);
    nlohmann::json AssetCatalog(const std::string& address);

private:
    BalanceRepo& repo_;
};

}  // namespace hubsql
