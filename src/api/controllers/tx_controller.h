#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "storage/tx_repo.h"

namespace hubsql {

// 交易查询控制器
class TxController {
public:
    explicit TxController(TxRepo& repo);

    nlohmann::json List(const std::string& type, const std::string& address,
                        int page, int size);
    nlohmann::json Detail(const std::string& tx_hash);

private:
    TxRepo& repo_;
};

}  // namespace hubsql
