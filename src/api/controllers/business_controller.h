#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "parser/business_registry.h"
#include "storage/block_repo.h"
#include "storage/tx_repo.h"

namespace hubsql {

// 业务数据控制器：完全基于 BusinessRegistry 注册的模块做通用分发（注册制）。
// 新增业务注册到 registry 后，无需再改控制器即可提供 List/Stats。
class BusinessController {
public:
    BusinessController(BusinessRegistry& registry, BlockRepo& blocks, TxRepo& txs);

    // module: 业务名（staking/investment...）；filter: 业务过滤参数（如 is_unstaked）
    nlohmann::json List(const std::string& module, const nlohmann::json& filter,
                        int page, int size);

    // 聚合所有已注册业务模块的统计
    nlohmann::json Stats();

private:
    BusinessRegistry& registry_;
    BlockRepo& blocks_;
    TxRepo& txs_;
};

}  // namespace hubsql
