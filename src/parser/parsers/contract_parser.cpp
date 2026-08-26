#include "parser/parsers/contract_parser.h"

#include <nlohmann/json.hpp>

#include "utxo/utxo_common.h"

namespace hubsql {

std::vector<ContractRecord> ContractParser::Parse(const Transaction& tx) {
    if (tx.type != 7 && tx.type != 8) return {};

    const std::string addr =
        tx.utxos.empty() || tx.utxos[0].owner.empty() ? "" : tx.utxos[0].owner[0];
    if (addr.empty()) return {};

    // 解析 txInfo（sender/recipient/callType）
    nlohmann::json ti;
    try {
        const auto j = nlohmann::json::parse(tx.data);
        ti = j.value("txInfo", nlohmann::json::object());
    } catch (...) {
        return {};
    }

    const bool is_deploy = (tx.type == 7);
    const bool is_flow_in =
        !is_deploy && ti.value("callType", "") == "FlowInTx";
    const bool is_flow_out =
        !is_deploy && ti.value("callType", "") == "FlowOutTx";

    // 跃入金额 = 跃入交易中真实地址 vout 之和；跃出金额 = VirtualCallFlowOutBurnGas vout 值
    int64_t flow_in_amount = 0, flow_out_amount = 0;
    for (const auto& u : tx.utxos) {
        for (const auto& vo : u.vout) {
            if (vo.addr == "VirtualCallFlowOutBurnGas") {
                flow_out_amount += std::stoll(vo.value);
            } else if (!IsVirtualAddr(vo.addr)) {
                if (is_flow_in) flow_in_amount += std::stoll(vo.value);
            }
        }
    }

    ContractRecord rec;
    rec.tx_hash         = tx.hash;
    rec.address         = addr;
    rec.sender          = ti.value("sender", addr);
    rec.recipient       = ti.value("recipient", "");
    rec.tx_type         = is_deploy ? "deploy" : "call";
    rec.is_flow_in      = is_flow_in;
    rec.is_flow_out     = is_flow_out;
    rec.flow_in_amount  = std::to_string(flow_in_amount);
    rec.flow_out_amount = std::to_string(flow_out_amount);
    rec.asset_type      = tx.utxos[0].assetType;  // 跃入跃出资产类型（OHI 或 hash）
    rec.tx_info         = ti.dump();               // txInfo (json)
    rec.time            = tx.time;
    return {rec};
}

}  // namespace hubsql
