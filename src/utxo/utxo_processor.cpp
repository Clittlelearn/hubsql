#include "utxo/utxo_processor.h"

#include <algorithm>
#include <set>
#include <utility>

#include <nlohmann/json.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include "utxo/utxo_common.h"

namespace hubsql {

namespace {

// 全零 hash 的 prevOut 引用 = 创世/coinbase 引用，无实际 UTXO 可删（兼容 0x 前缀）
bool IsGenesisRef(const std::string& h) {
    if (h.empty()) return true;
    std::string_view v = h;
    if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) {
        v.remove_prefix(2);
    }
    return std::all_of(v.begin(), v.end(), [](char c) { return c == '0'; });
}

// 赎回类交易类型：UNSTAKE=3 / UNDELEGATE=5 / UNLOCK=10（见链 ca/global.h TxType）
bool IsRedeemType(uint64_t ty) {
    return ty == 3 || ty == 5 || ty == 10;
}

// 从交易 data（JSON 字符串，含 txInfo）中取赎回目标 hash
std::string GetRedeemHash(const Transaction& tx) {
    if (tx.data.empty()) return "";
    try {
        const auto j  = nlohmann::json::parse(tx.data);
        const auto ti = j.value("txInfo", nlohmann::json::object());
        for (const char* k : {"unLockUtxo", "unstakeUtxo", "undelegatingUtxo"}) {
            if (ti.contains(k)) return ti.at(k).get<std::string>();
        }
    } catch (...) {
        // 解析失败则视为无赎回目标
    }
    return "";
}

}  // namespace

UtxoProcessor::UtxoProcessor(UtxoStore& store) : store_(store) {}

std::unordered_map<BalanceKey, boost::multiprecision::cpp_int, BalanceKeyHash>
UtxoProcessor::ProcessBlock(const Block& block) {
    std::unordered_map<BalanceKey, boost::multiprecision::cpp_int, BalanceKeyHash>
        deltas;

    for (const auto& tx : block.txs) {
        const uint64_t ty = tx.type;
        const std::string redeem = IsRedeemType(ty) ? GetRedeemHash(tx) : "";

        // ---- 1) 消费输入（vin）：按 (prevout.hash, signer, assetType) 扣减父交易下
        //      signer 的同资产未消费输出之和（资产类型 = 消费 utxo 的 assetType，对应链 currency）----
        for (const auto& u : tx.utxos) {
            const std::string& signer = u.owner.empty() ? "" : u.owner[0];
            if (signer.empty()) continue;
            const std::string asset_type = u.assetType;  // 链上 currency = utxo.assettype()

            // 同一 utxo 内 (hash, n) 去重（对应链 per-utxo vin_hash_values）
            std::set<std::pair<std::string, uint32_t>> seen;
            for (const auto& p : u.vin.prevout) {
                const std::string& h = p.hash;
                if (h.empty() || IsGenesisRef(h)) continue;

                // 赎回跳过：UNSTAKE/UNDELEGATE/UNLOCK 且 prevout.hash==redeem && n==1
                if (!redeem.empty() && h == redeem && p.n == 1) continue;
                if (!seen.insert({h, p.n}).second) continue;

                auto unspent = store_.GetUnspentByTx(h);
                // signer 在该父交易下、同资产类型的未消费输出之和
                boost::multiprecision::cpp_int sum = 0;
                std::vector<const UtxoOut*> to_spend;
                for (const auto& o : unspent) {
                    if (o.addr == signer && o.asset_type == asset_type) {
                        sum += boost::multiprecision::cpp_int(o.value);
                        to_spend.push_back(&o);
                    }
                }
                if (sum == 0) continue;  // 已消费或无非真实输出可扣（减 0 无影响）

                deltas[{signer, asset_type}] -= sum;
                // 删除已消费输出（对应链 removeUtxoHashesByAddr + removeUtxoValueByUtxoHashes）
                for (const auto* o : to_spend) {
                    store_.Delete(h, o->utxo_i, o->vout_j);
                }
            }
        }

        // ---- 2) 创建输出（vout）：真实地址记余额（资产类型=创建 utxo 的 assetType）+ 写 RocksDB ----
        //      注：创世/coinbase 交易的 utxo 可能没有 assetType 字段（如 value=0 的创世输出），
        //      资产类型未知无法归属余额，且无真实资产，故跳过。
        for (size_t ui = 0; ui < tx.utxos.size(); ++ui) {
            const auto& u = tx.utxos[ui];
            const std::string asset_type = u.assetType;
            if (asset_type.empty()) continue;  // 无资产类型 -> 不归属余额、不写 RocksDB
            for (size_t vj = 0; vj < u.vout.size(); ++vj) {
                const auto& vo = u.vout[vj];
                if (IsVirtualAddr(vo.addr)) continue;
                const std::string& raw_value = vo.value;
                store_.Put(tx.hash, static_cast<uint32_t>(ui),
                           static_cast<uint32_t>(vj), vo.addr, raw_value,
                           asset_type);
                deltas[{vo.addr, asset_type}] +=
                    boost::multiprecision::cpp_int(raw_value);
            }
        }
    }
    return deltas;
}

}  // namespace hubsql
