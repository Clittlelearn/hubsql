#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

namespace hubsql {

// 交易记录（tx_records 表，仅保留最新 20 个高度）
struct TxRecord {
    std::string tx_hash;         // 交易 hash
    uint64_t block_height{0};    // 交易所在区块高度
    std::string tx_type;         // 交易类型（数字转字符串）
    std::string utxo_json;       // 交易 utxo（json 字符串）

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
