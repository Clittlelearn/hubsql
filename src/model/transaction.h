#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>
#include "utils/reflect_struct.h"

namespace hubsql {

// 与链节点 GetBlockByHeight 返回格式对齐

// 输入引用：节点 prevout 为 [{hash, n}]，n 为父交易内引用（赎回交易 redeem@n==1 跳过扣减）
struct Prevout {
    std::string hash;
    uint32_t n{0};
    REFLECT(hash, n)
};

struct Vin {
    std::vector<Prevout> prevout;
    REFLECT(prevout)
};

struct Vout {
    std::string value;  // 节点为数字，FromJson 统一转字符串保持精度
    std::string addr;
    REFLECT(value, addr)
};

struct Utxo {
    std::vector<std::string> owner;
    Vin vin;
    std::vector<Vout> vout;
    std::string assetType;  // 资产类型（OHI 或合约/提案hash）
    REFLECT(owner, vin, vout, assetType)
};

struct Transaction {
    std::string hash;
    std::string identity;
    uint64_t time;
    uint64_t type;
    std::vector<Utxo> utxos;  // 节点 utxos 可能是单个对象或数组，FromJson 统一为数组
    std::string data;        // 节点 data 为 JSON 字符串，内含 txInfo（redeem 哈希等）

    REFLECT(hash, identity, time, type, utxos, data)

    nlohmann::json ToJson() const;
};

}  // namespace hubsql
