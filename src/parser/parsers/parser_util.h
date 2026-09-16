#pragma once

#include <string>

#include <boost/multiprecision/cpp_int.hpp>
#include <nlohmann/json.hpp>

#include "model/transaction.h"
#include "utxo/utxo_common.h"

namespace hubsql {

// 从交易 utxos 中提取第一个真实（非虚拟）输出的地址/金额
inline std::string FirstRealAddr(const Transaction& tx) {
    for (const auto& u : tx.utxos)
        for (const auto& vo : u.vout)
            if (!IsVirtualAddr(vo.addr)) return vo.addr;
    return "";
}

inline std::string FirstRealValue(const Transaction& tx) {
    for (const auto& u : tx.utxos)
        for (const auto& vo : u.vout)
            if (!IsVirtualAddr(vo.addr)) return vo.value;
    return "0";
}

// 节点 UTXO 的 value 已经是链上 8 位最小单位，无需再次换算。
inline const std::string& UtxoValueToRaw(const std::string& value) {
    return value;
}

// txInfo 可能把大金额编码为字符串（避免 JSON uint64 溢出），也可能是整数。
inline std::string JsonRawAmount(const nlohmann::json& value) {
    if (value.is_string()) return value.get<std::string>();
    if (value.is_number_unsigned() || value.is_number_integer()) return value.dump();
    return "0";
}

}  // namespace hubsql
