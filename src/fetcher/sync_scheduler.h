#pragma once

#include <atomic>
#include <chrono>

#include "fetcher/block_fetcher.h"
#include "parser/block_parser.h"
#include "storage/sync_repo.h"

namespace hubsql {

// 同步调度器：按高度区间批量拉取区块，解析入库，维护同步进度（断点续拉）
class SyncScheduler {
public:
    SyncScheduler(BlockFetcher& fetcher,
                  BlockParser& parser,
                  SyncRepo& sync_repo,
                  int sync_interval_seconds,
                  int batch_size);

    // 阻塞式运行；在独立线程调用
    void Run();

    // 停止调度
    void Stop() { running_ = false; }

private:
    void ProcessBlock(const Block& block);

    BlockFetcher& fetcher_;
    BlockParser& parser_;
    SyncRepo& sync_repo_;
    std::chrono::seconds interval_;
    int batch_size_{50};
    std::atomic<bool> running_{true};
};

}  // namespace hubsql
