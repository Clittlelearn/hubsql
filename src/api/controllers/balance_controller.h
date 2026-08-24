#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "storage/balance_repo.h"

namespace hubsql {

// 账户余额查询控制器
class BalanceController {
public:
    explicit BalanceController(BalanceRepo& repo);

    // 全部余额（分页，按余额降序）
    nlohmann::json List(int page, int size);

    // 单地址余额
    nlohmann::json Get(const std::string& address);

private:
    BalanceRepo& repo_;
};

}  // namespace hubsql
