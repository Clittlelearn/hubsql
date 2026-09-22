// 端到端余额验证工具（直连链节点）
// 用法: hubsql_verify_balance <config.json>
// 用真实生产流水线(BlockFetcher -> BlockParser -> RocksDB -> MySQL)处理链上全部区块，
// 输出 OHI/提案资产余额表的联合视图。
#include <iostream>

#include "common/config.h"
#include "common/logger.h"
#include "fetcher/block_fetcher.h"
#include "fetcher/http_client.h"
#include "parser/block_parser.h"
#include "parser/business_registry.h"
#include "parser/parsers/claim_module.h"
#include "parser/parsers/contract_module.h"
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
#include "storage/db_pool.h"
#include "storage/investment_repo.h"
#include "storage/lock_repo.h"
#include "storage/proposal_repo.h"
#include "storage/staking_repo.h"
#include "storage/tx_record_repo.h"
#include "storage/vote_repo.h"
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
        hubsql::BlockRepo block_repo(db_pool);
        hubsql::StakingRepo staking_repo(db_pool);
        hubsql::InvestmentRepo investment_repo(db_pool);
        hubsql::ProposalRepo proposal_repo(db_pool);
        hubsql::VoteRepo vote_repo(db_pool);
        hubsql::LockRepo lock_repo(db_pool);
        hubsql::TxRecordRepo tx_record_repo(db_pool);
        hubsql::ContractRepo contract_repo(db_pool);
        hubsql::ClaimRepo claim_repo(db_pool);
        hubsql::UtxoStore store(cfg.utxo_db_path);

        hubsql::BusinessRegistry registry;
        registry.Register(std::make_shared<hubsql::StakingModule>(staking_repo));
        registry.Register(std::make_shared<hubsql::InvestmentModule>(investment_repo));
        registry.Register(std::make_shared<hubsql::ProposalModule>(proposal_repo));
        registry.Register(std::make_shared<hubsql::VoteModule>(vote_repo, proposal_repo));
        registry.Register(std::make_shared<hubsql::LockModule>(lock_repo));
        registry.Register(std::make_shared<hubsql::TxRecordModule>(tx_record_repo, 20));
        registry.Register(std::make_shared<hubsql::ContractModule>(contract_repo));
        registry.Register(std::make_shared<hubsql::ClaimModule>(claim_repo));
        hubsql::BlockParser parser(store, balance_repo, block_repo, registry,
                                   db_pool);

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
        std::cout << "=== native/proposal balances (" << all.size() << " 条) ===\n";
        for (const auto& item : all) {
            std::cout << item.address << "  [" << item.asset_type << "]  "
                      << item.balance << "\n";
        }
        std::cout << "总余额: " << balance_repo.TotalBalance()
                  << " (OHI: " << balance_repo.TotalBalance("OHI") << ")\n";
    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
