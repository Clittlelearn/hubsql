#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 投资记录（investment_records 表；链暂无 invest 类型，保留扩展用）
struct InvestmentRecord {
    std::string tx_hash;         // 投资交易 hash
    uint64_t block_height{0};    // 投资所在区块高度
    std::string address;         // 投资地址
    std::string amount;          // 投资金额（字符串保精度）
    std::string product_id;      // 投资产品 ID

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
