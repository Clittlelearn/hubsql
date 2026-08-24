#include "fetcher/block_fetcher.h"

#include <stdexcept>

#include <nlohmann/json.hpp>

#include "common/logger.h"

namespace hubsql {

BlockFetcher::BlockFetcher(const ChainConfig& cfg, std::shared_ptr<HttpClient> http)
    : cfg_(cfg), http_(std::move(http)) {}

uint64_t BlockFetcher::GetLatestHeight() {
    auto body = http_->Get(cfg_.base_url + "/GetBlockHeight");
    auto j = nlohmann::json::parse(body);
    const auto& res = j.value("result", nlohmann::json::object());
    auto h = res.value("height", "0");
    return std::stoull(h);
}

std::vector<Block> BlockFetcher::FetchBlocks(uint64_t begin, uint64_t end) {
    nlohmann::json req = {
        {"id", "1"},
        {"jsonrpc", "2.0"},
        {"method", "GetBlockByHeight"},
        {"params", {{"begin", begin}, {"end", end}}},
    };
    auto body = http_->Post(cfg_.base_url + "/GetBlockByHeight", req.dump());
    auto j = nlohmann::json::parse(body);

    const auto& res = j.value("result", nlohmann::json::object());
    std::vector<Block> blocks;
    for (const auto& bj : res.value("blocks", nlohmann::json::array())) {
        blocks.push_back(Block::FromJson(bj));
    }
    return blocks;
}

}  // namespace hubsql
