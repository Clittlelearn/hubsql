#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 投资记录（investment_records 表，type=4 DELEGATE）
struct InvestmentRecord {
    std::string tx_hash;         // 投资交易 hash
    uint64_t block_height{0};    // 投资所在区块高度
    std::string address;         // 投资地址
    std::string amount;          // 投资金额（delegateAmount，字符串保精度）
    uint64_t time{0};            // 投资时间（微秒）
    std::string bonus_addr;      // bonusAddr（被投资地址）
    std::string invest_type;     // 投资类型（delegateType，如 "Normal"）

    // 解投资状态（查询展示；解投资仅标记不删除）
    bool is_deinvested{false};   // 是否已解投资
    std::string deinvest_tx_hash; // 解投资交易 hash
    uint64_t deinvest_time{0};   // 解投资时间（微秒）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
