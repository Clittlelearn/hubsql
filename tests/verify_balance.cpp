// 端到端余额验证工具（直连链节点）
// 用法: hubsql_verify_balance <config.json>
// 用真实生产流水线(BlockFetcher -> BlockParser -> RocksDB -> MySQL)处理链上全部区块，
// 输出 account_balances。
#include <iostream>

#include "common/config.h"
#include "common/logger.h"
#include "fetcher/block_fetcher.h"
#include "fetcher/http_client.h"
#include "parser/block_parser.h"
#include "parser/business_registry.h"
#include "parser/parsers/staking_module.h"
#include "storage/balance_repo.h"
#include "storage/db_pool.h"
#include "storage/staking_repo.h"
#include "utxo/utxo_store.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "用法: hubsql_verify_balance <config.json>\n";
        return 1;
    }
    try {
        auto cfg = hubsql::AppConfig::LoadFromFile(argv[1]);
        hubsql::Logger::Init(cfg.log);

        hubsql::DbPool db_pool(cfg.mysql);
        hubsql::BalanceRepo balance_repo(db_pool);
        hubsql::StakingRepo staking_repo(db_pool);
        hubsql::UtxoStore store(cfg.utxo_db_path);

        hubsql::BusinessRegistry registry;
        registry.Register(std::make_shared<hubsql::StakingModule>(staking_repo));
        hubsql::BlockParser parser(store, balance_repo, registry, db_pool);

        auto http = std::make_shared<hubsql::HttpClient>(cfg.chain.http);
        hubsql::BlockFetcher fetcher(cfg.chain, http);

        uint64_t latest = fetcher.GetLatestHeight();
        std::cout << "最新高度: " << latest << "\n";

        auto blocks = fetcher.FetchBlocks(0, latest);
        for (auto& b : blocks) {
            parser.ParseAndStore(b);
        }

        std::cout << "\n共处理 " << blocks.size() << " 个区块\n";
        std::cout << "RocksDB 未消费输出数: " << store.Count() << "\n";
        auto all = balance_repo.ListAll();
        std::cout << "=== account_balances (" << all.size() << " 个账户) ===\n";
        for (const auto& [addr, bal] : all) {
            std::cout << addr << "  " << bal << "\n";
        }
        std::cout << "总余额: " << balance_repo.TotalBalance() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
