#pragma once

#include "model/block.h"
#include "parser/business_registry.h"
#include "storage/balance_repo.h"
#include "storage/block_repo.h"
#include "storage/db_pool.h"
#include "utxo/utxo_processor.h"
#include "utxo/utxo_store.h"

namespace hubsql {

// 区块解析器：执行 UTXO 规则（RocksDB 读写 + 余额增量）并落库余额表；
// 业务解析/存储通过 BusinessRegistry 注册的模块统一分发（注册制，扩展业务不改本类）。
class BlockParser {
public:
    BlockParser(UtxoStore& utxo_store, BalanceRepo& balance_repo,
                BlockRepo& block_repo,
                BusinessRegistry& registry, DbPool& pool);

    // 解析并存储整个区块
    void ParseAndStore(const Block& block);

private:
    UtxoProcessor processor_;
    BalanceRepo& balance_repo_;
    BlockRepo& block_repo_;
    BusinessRegistry& registry_;
    DbPool& pool_;
};

}  // namespace hubsql
