#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 合约交易记录（contract_records 表，DEPLOY=7 / CALL=8，含跃入 FlowInTx/跃出 FlowOutTx）
struct ContractRecord {
    std::string tx_hash;          // 交易 hash
    uint64_t block_height{0};     // 交易所在区块高度
    std::string address;          // 账户地址（sender）
    std::string sender;           // 发送者
    std::string recipient;        // 合约地址（recipient）
    std::string tx_type;          // "deploy" / "call"
    bool is_flow_in{false};       // 是否合约跃入
    bool is_flow_out{false};      // 是否合约跃出
    std::string flow_in_amount;   // 跃入金额
    std::string flow_out_amount;  // 跃出金额
    std::string asset_type;       // 跃入跃出资产类型（hash 或 OHI）
    std::string tx_info;          // txInfo（json 字符串）
    uint64_t time{0};             // 交易时间（微秒）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
