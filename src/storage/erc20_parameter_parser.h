#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "model/transaction.h"

namespace hubsql {

// 由交易参数直接确定的一次 ERC20 余额变化。零地址表示铸造或销毁。
struct Erc20ParameterTransfer {
    std::string contract;
    std::string from;
    std::string to;
    std::string amount;
};

// 支持 ERC20 部署初始发行、transfer、transferFrom、FlowIn 和 FlowOut。
// 返回空表示该交易不是能够仅凭参数确定的 ERC20 余额变化。
std::optional<Erc20ParameterTransfer> ParseErc20ParameterTransfer(
    const Transaction& tx, const nlohmann::json& tx_info);

}  // namespace hubsql
