#pragma once

#include <memory>
#include <string>

#include "common/config.h"
#include "fetcher/http_client.h"
#include "model/block.h"

namespace hubsql {

// 与链节点 HTTP 接口交互，拉取区块原始数据
//   GET  {base_url}/GetBlockHeight       -> {"result":{"height":"35"}}
//   POST {base_url}/GetBlockByHeight     -> {"result":{"blocks":[...]}}
class BlockFetcher {
public:
    BlockFetcher(const ChainConfig& cfg, std::shared_ptr<HttpClient> http);

    // 当前链最新高度
    uint64_t GetLatestHeight();

    // 批量拉取 [begin, end] 高度区间内的所有区块（含同高度分叉块）
    std::vector<Block> FetchBlocks(uint64_t begin, uint64_t end);

private:
    ChainConfig cfg_;
    std::shared_ptr<HttpClient> http_;
};

}  // namespace hubsql
