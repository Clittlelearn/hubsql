#pragma once

#include <string>

namespace hubsql {

// 资产名归一化：投票(voteHash)与撤销(proposalHash)引用的 "0xOHI" → "OHI"（第一笔提案），
// "0x<提案hash>" → 原样返回（等于提案的 tx_hash，用于关联提案记录）。
inline std::string NormalizeProposalAsset(const std::string& ref) {
    if ((ref.rfind("0x", 0) == 0 || ref.rfind("0X", 0) == 0) && ref.size() > 2) {
        if (ref.compare(2, std::string::npos, "OHI") == 0) return "OHI";
    }
    return ref;
}

}  // namespace hubsql
