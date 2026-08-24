#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "model/transaction.h"
#include "utils/reflect_struct.h"

namespace hubsql {

// 区块头（节点区块元素中的 "blocks" 字段）
struct BlockHeader {
    uint64_t bytes;
    std::string hash;
    uint64_t height;
    std::string merkleRoot;
    std::string prevHash;
    uint64_t time;
    REFLECT(bytes, hash, height, merkleRoot, prevHash, time)
};

// 区块（节点返回: {blocks: {...}, txs: [...]}）
struct Block {
    BlockHeader blocks;
    std::vector<Transaction> txs;

    REFLECT(blocks, txs)

    static Block FromJson(const nlohmann::json& j);
    nlohmann::json ToJson() const;
};

}  // namespace hubsql
