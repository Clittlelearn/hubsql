#include <atomic>
#include <chrono>
#include <iostream>
#include <signal.h>
#include <pthread.h>
#include <thread>

#include "api/api_server.h"
#include "common/config.h"
#include "common/logger.h"
#include "fetcher/block_fetcher.h"
#include "fetcher/http_client.h"
#include "fetcher/sync_scheduler.h"
#include "parser/block_parser.h"
#include "parser/business_registry.h"
#include "parser/parsers/claim_module.h"
#include "parser/parsers/contract_module.h"
#include "parser/parsers/fund_module.h"
#include "parser/parsers/investment_module.h"
#include "parser/parsers/lock_module.h"
#include "parser/parsers/proposal_module.h"
#include "parser/parsers/staking_module.h"
#include "parser/parsers/tx_record_module.h"
#include "parser/parsers/vote_module.h"
#include "storage/balance_repo.h"
#include "storage/block_repo.h"
#include "storage/claim_repo.h"
#include "storage/contract_repo.h"
#include "storage/fund_repo.h"
#include "storage/db_pool.h"
#include "storage/investment_repo.h"
#include "storage/lock_repo.h"
#include "storage/proposal_repo.h"
#include "storage/staking_repo.h"
#include "storage/sync_repo.h"
#include "storage/tx_record_repo.h"
#include "storage/tx_repo.h"
#include "storage/vote_repo.h"
#include "utxo/utxo_store.h"

namespace {

std::atomic<bool> g_running{true};

// 可靠的多线程信号处理（sigwait 模式）：
// 1) 安装空处理器，防止信号被 SIG_IGN 直接丢弃；
// 2) 在创建其他线程前屏蔽 SIGINT/SIGTERM，使所有线程继承阻塞掩码；
// 3) 由专用线程 sigwait 统一捕获，收到后置停止标志。
void SetupSignalHandling() {
    struct sigaction sa {};
    sa.sa_handler = [](int) {};
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);

    std::thread([set] {
        int sig = 0;
        if (sigwait(&set, &sig) == 0) {
            LOG_INFO("收到退出信号: {}", sig);
            g_running = false;
        }
    }).detach();
}

}  // namespace

// 用法: ./hubsql [config.json]
int main(int argc, char** argv) {
    try {
        const std::string cfg_path = argc > 1 ? argv[1] : "config/config.json";
        auto cfg = hubsql::AppConfig::LoadFromFile(cfg_path);
        hubsql::Logger::Init(cfg.log);

        // 必须在创建工作线程之前设置信号处理
        SetupSignalHandling();

        LOG_INFO("========== hubsql 启动 ==========");
        LOG_INFO("链节点: {}  同步间隔: {}s", cfg.chain.base_url,
                 cfg.chain.sync_interval_seconds);
        LOG_INFO("数据库: {}:{}/{}  连接池: {}",
                 cfg.mysql.host, cfg.mysql.port, cfg.mysql.database, cfg.mysql.pool_size);

        // ---- 存储层 ----
        hubsql::DbPool db_pool(cfg.mysql);
        hubsql::BlockRepo block_repo(db_pool);
        hubsql::TxRepo tx_repo(db_pool);
        hubsql::StakingRepo staking_repo(db_pool);
        hubsql::InvestmentRepo investment_repo(db_pool);
        hubsql::ProposalRepo proposal_repo(db_pool);
        hubsql::VoteRepo vote_repo(db_pool);
        hubsql::LockRepo lock_repo(db_pool);
        hubsql::TxRecordRepo tx_record_repo(db_pool);
        hubsql::ContractRepo contract_repo(db_pool);
        hubsql::FundRepo fund_repo(db_pool);
        hubsql::ClaimRepo claim_repo(db_pool);
        hubsql::SyncRepo sync_repo(db_pool);
        hubsql::BalanceRepo balance_repo(db_pool);

        // ---- UTXO KV 存储（RocksDB）----
        hubsql::UtxoStore utxo_store(cfg.utxo_db_path);

        // ---- 采集层 ----
        auto http_client = std::make_shared<hubsql::HttpClient>(cfg.chain.http);
        hubsql::BlockFetcher fetcher(cfg.chain, http_client);

        // ---- 业务注册表（注册制：新增业务=新增模块并 Register 一次）----
        hubsql::BusinessRegistry business_registry;
        business_registry.Register(
            std::make_shared<hubsql::StakingModule>(staking_repo));
        business_registry.Register(
            std::make_shared<hubsql::InvestmentModule>(investment_repo));
        business_registry.Register(
            std::make_shared<hubsql::ProposalModule>(proposal_repo));
        business_registry.Register(
            std::make_shared<hubsql::VoteModule>(vote_repo, proposal_repo));
        business_registry.Register(
            std::make_shared<hubsql::LockModule>(lock_repo));
        business_registry.Register(
            std::make_shared<hubsql::TxRecordModule>(tx_record_repo, 20));
        business_registry.Register(
            std::make_shared<hubsql::ContractModule>(contract_repo));
        business_registry.Register(
            std::make_shared<hubsql::ClaimModule>(claim_repo));
        business_registry.Register(
            std::make_shared<hubsql::FundModule>(fund_repo));

        // ---- 解析层：UTXO 规则 + 余额落库 + 注册表分发业务解析 ----
        hubsql::BlockParser block_parser(utxo_store, balance_repo, block_repo,
                                         business_registry, db_pool);

        // ---- 同步调度（后台线程）----
        hubsql::SyncScheduler scheduler(fetcher, block_parser, sync_repo,
                                        cfg.chain.sync_interval_seconds,
                                        cfg.chain.batch_size);
        std::jthread scheduler_thread([&scheduler] {
            try {
                scheduler.Run();
            } catch (const std::exception& e) {
                LOG_ERROR("调度线程异常: {}", e.what());
            }
        });

        // ---- REST API 服务（后台线程）----
        hubsql::ApiServer api(cfg.api, db_pool, block_repo, tx_repo,
                              business_registry, balance_repo, cfg.chain.base_url);
        std::jthread api_thread([&api] {
            try {
                api.Run();
            } catch (const std::exception& e) {
                LOG_ERROR("API 线程异常: {}", e.what());
                g_running = false;
            }
        });

        // ---- 等待退出信号 ----
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        LOG_INFO("收到退出信号，正在优雅停止 ...");
        scheduler.Stop();
        api.Stop();
        // jthread 析构时自动 join

    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << std::endl;
        return 1;
    }

    LOG_INFO("========== hubsql 已退出 ==========");
    return 0;
}
