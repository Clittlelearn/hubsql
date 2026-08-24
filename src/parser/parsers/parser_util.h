#pragma once

#include <string>

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

}  // namespace hubsql
