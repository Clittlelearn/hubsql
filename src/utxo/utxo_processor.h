#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include <boost/multiprecision/cpp_int.hpp>

#include "model/block.h"
#include "utxo/utxo_store.h"

namespace hubsql {

// 余额键：地址 + 资产类型（链上余额按资产类型分别存储）
struct BalanceKey {
    std::string address;
    std::string asset_type;
    bool operator==(const BalanceKey&) const = default;
};

struct BalanceKeyHash {
    std::size_t operator()(const BalanceKey& k) const {
        std::size_t h1 = std::hash<std::string>{}(k.address);
        std::size_t h2 = std::hash<std::string>{}(k.asset_type);
        return h1 ^ (h2 << 1);
    }
};

// UTXO 解析处理器：复刻链 ca_algorithm::SaveBlock 的余额计数规则，与节点 GetBalance 完全一致
//   - vout: 每个真实地址输出 balance[addr][assetType] += value（含 BONUS/DEPLOY/CALL/FUND，虚拟地址除外）
//   - vin:  按 (prevout.hash, signer=owner[0], assetType) 扣减父交易下 signer 的同资产未消费输出之和
//           （等价于链 setUtxoValueByUtxoHashes 的累加语义）
//   - 赎回跳过: UNSTAKE/UNDELEGATE/UNLOCK 且 prevout.hash==redeem && n==1（redeem 取 data.txInfo）
class UtxoProcessor {
public:
    explicit UtxoProcessor(UtxoStore& store);

    // 返回: (地址, 资产类型) -> 余额增量（可为负，仅统计真实地址）
    std::unordered_map<BalanceKey, boost::multiprecision::cpp_int, BalanceKeyHash>
    ProcessBlock(const Block& block);

private:
    UtxoStore& store_;
};

}  // namespace hubsql
