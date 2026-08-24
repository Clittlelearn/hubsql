#include "fetcher/sync_scheduler.h"

#include <thread>

#include "common/logger.h"

namespace hubsql {

SyncScheduler::SyncScheduler(BlockFetcher& fetcher,
                             BlockParser& parser,
                             SyncRepo& sync_repo,
                             int sync_interval_seconds,
                             int batch_size)
    : fetcher_(fetcher),
      parser_(parser),
      sync_repo_(sync_repo),
      interval_(std::chrono::seconds(std::max(sync_interval_seconds, 1))),
      batch_size_(std::max(batch_size, 1)) {}

void SyncScheduler::ProcessBlock(const Block& block) {
    LOG_INFO("拉取并处理区块 height={} hash={}", block.blocks.height, block.blocks.hash);
    parser_.ParseAndStore(block);
}

void SyncScheduler::Run() {
    LOG_INFO("同步调度启动，轮询间隔 {}s，批大小 {}", interval_.count(), batch_size_);
    while (running_) {
        try {
            uint64_t latest = fetcher_.GetLatestHeight();
            uint64_t local  = sync_repo_.GetLastSyncedHeight();
            LOG_INFO("最新高度={}, 已同步高度={}", latest, local);

            if (local < latest) {
                uint64_t begin = local + 1;
                while (begin <= latest && running_) {
                    uint64_t end = std::min(begin + batch_size_ - 1, latest);
                    auto blocks = fetcher_.FetchBlocks(begin, end);  // 含同高度分叉块
                    for (auto& b : blocks) {
                        if (!running_) break;
                        ProcessBlock(b);
                    }
                    begin = end + 1;
                }
                sync_repo_.UpdateProgress(latest);
                LOG_INFO("已同步至高度 {}", latest);
            }
        } catch (const std::exception& e) {
            LOG_ERROR("同步出错: {}", e.what());
        }

        // 轮询间隔（支持提前停止）
        for (int i = 0; i < interval_.count() && running_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    LOG_INFO("同步调度已停止");
}

}  // namespace hubsql
