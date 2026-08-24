#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "model/block.h"
#include "utxo/utxo_store.h"

namespace hubsql {

// UTXO 解析处理器：复刻链 ca_algorithm::SaveBlock 的余额计数规则，与节点 GetBalance 完全一致
//   - vout: 每个真实地址输出 balance += value（含 BONUS/DEPLOY/CALL/FUND，虚拟地址除外）
//   - vin:  按 (prevout.hash, signer=owner[0]) 扣减该父交易下 signer 的全部未消费输出之和
//           （等价于链 setUtxoValueByUtxoHashes 的累加语义）
//   - 赎回跳过: UNSTAKE/UNDELEGATE/UNLOCK 且 prevout.hash==redeem && n==1（redeem 取 data.txInfo）
class UtxoProcessor {
public:
    explicit UtxoProcessor(UtxoStore& store);

    // 返回: 地址 -> 余额增量（可为负，仅统计真实地址）
    std::unordered_map<std::string, int64_t> ProcessBlock(const Block& block);

private:
    UtxoStore& store_;
};

}  // namespace hubsql
