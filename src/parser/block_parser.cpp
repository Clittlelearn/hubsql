#include "parser/block_parser.h"

#include <cppconn/connection.h>

#include "common/logger.h"

namespace hubsql {

BlockParser::BlockParser(UtxoStore& utxo_store, BalanceRepo& balance_repo,
                         BusinessRegistry& registry, DbPool& pool)
    : processor_(utxo_store),
      balance_repo_(balance_repo),
      registry_(registry),
      pool_(pool) {}

void BlockParser::ParseAndStore(const Block& block) {
    // 1) UTXO 解析：创建写入 / 消费删除，得到余额增量
    auto deltas = processor_.ProcessBlock(block);

    // 2) 余额增量原子落库
    balance_repo_.ApplyDeltas(deltas);

    // 3) 业务解析/存储：整块一个事务，按注册表分发给各业务模块
    int biz = 0;
    pool_.WithConnection([&](sql::Connection& conn) {
        conn.setAutoCommit(false);
        try {
            for (const auto& tx : block.txs) {
                biz += registry_.ProcessTransaction(conn, tx, block.blocks.height);
            }
            conn.commit();
        } catch (...) {
            conn.rollback();
            throw;
        }
        // 恢复自动提交，避免污染连接池中的连接
        conn.setAutoCommit(true);
    });

    LOG_INFO("区块 {} 处理完成: UTXO 已更新, 余额变动 {} 个账户, 业务记录 {} 条",
             block.blocks.height, deltas.size(), biz);
}

}  // namespace hubsql
