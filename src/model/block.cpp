#include "model/block.h"

#include <stdexcept>

#include "utils/reflect_struct.h"

namespace hubsql {

namespace {

// 数字或字符串 -> 字符串（保持精度）
std::string ToValueStr(const nlohmann::json& v) {
    if (v.is_string()) return v.get<std::string>();
    return v.dump();
}

}  // namespace

Block Block::FromJson(const nlohmann::json& j) {
    Block block;

    // 区块头（节点包裹在 "blocks" 里）
    const auto& hdr = j.value("blocks", nlohmann::json::object());
    block.blocks.bytes      = hdr.value("bytes", 0ULL);
    block.blocks.hash       = hdr.value("hash", "");
    block.blocks.height     = hdr.value("height", 0ULL);
    block.blocks.merkleRoot = hdr.value("merkleRoot", "");
    block.blocks.prevHash   = hdr.value("prevHash", "");
    block.blocks.time       = hdr.value("time", 0ULL);

    // 交易
    for (const auto& tj : j.value("txs", nlohmann::json::array())) {
        Transaction tx;
        tx.hash     = tj.value("hash", "");
        tx.identity = tj.value("identity", "");
        tx.time     = tj.value("time", 0ULL);
        tx.type     = tj.value("type", 0ULL);

        // data 字段（JSON 字符串，内含 txInfo；也可能是 JSON 对象 -> 统一转字符串）
        if (tj.contains("data")) {
            const auto& dj = tj.at("data");
            if (dj.is_string()) {
                tx.data = dj.get<std::string>();
            } else {
                tx.data = dj.dump();
            }
        }

        // utxos 可能是单个对象（创世）或数组 -> 统一为数组
        const auto& uj = tj.at("utxos");
        std::vector<nlohmann::json> utxo_list;
        if (uj.is_array()) {
            utxo_list = uj.get<std::vector<nlohmann::json>>();
        } else {
            utxo_list = {uj};
        }

        for (const auto& u : utxo_list) {
            Utxo ut;

            for (const auto& o : u.value("owner", nlohmann::json::array())) {
                ut.owner.push_back(o.get<std::string>());
            }
            ut.assetType = u.value("assetType", "");
            // vin.prevout 为 [{hash, n}]（可能是单个对象或数组 -> 统一解析）
            const auto& vinj = u.value("vin", nlohmann::json::object());
            const auto& pj   = vinj.value("prevout", nlohmann::json::array());
            std::vector<nlohmann::json> po_list;
            if (pj.is_array()) {
                po_list = pj.get<std::vector<nlohmann::json>>();
            } else if (pj.is_object()) {
                po_list = {pj};
            }
            for (const auto& po : po_list) {
                Prevout p;
                p.hash = po.value("hash", "");
                p.n    = po.value("n", 0U);
                ut.vin.prevout.push_back(std::move(p));
            }
            // vout
            for (const auto& vo : u.value("vout", nlohmann::json::array())) {
                Vout v;
                v.addr   = vo.value("addr", "");
                v.value  = ToValueStr(vo.value("value", nlohmann::json()));
                ut.vout.push_back(std::move(v));
            }
            tx.utxos.push_back(std::move(ut));
        }
        block.txs.push_back(std::move(tx));
    }
    return block;
}

nlohmann::json Block::ToJson() const {
    nlohmann::json j;
    reflect::Serialize(*this, j);
    return j;
}

}  // namespace hubsql
