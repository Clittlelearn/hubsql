#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace rocksdb {
class DB;
}

namespace hubsql {

// RocksDB 中的单个未花费输出
struct UtxoOut {
    uint32_t utxo_i{0};
    uint32_t vout_j{0};
    std::string addr;
    std::string value;
    std::string asset_type;  // 该输出所属资产类型（OHI 或 hash）
};

// RocksDB 封装的 UTXO KV 存储
//   key   = txhash:utxo_i:vout_j
//   value = {"addr": "...", "value": "...", "assetType": "..."}
// 创建输出 -> Put；消费输出 -> Delete（删除后即视为已花费）
class UtxoStore {
public:
    explicit UtxoStore(const std::string& db_path);
    ~UtxoStore();

    UtxoStore(const UtxoStore&) = delete;
    UtxoStore& operator=(const UtxoStore&) = delete;

    // 创建输出（写入）
    void Put(const std::string& tx_hash, uint32_t utxo_i, uint32_t vout_j,
             const std::string& addr, const std::string& value,
             const std::string& asset_type);

    // 消费输出（删除），返回是否删除成功
    bool Delete(const std::string& tx_hash, uint32_t utxo_i, uint32_t vout_j);

    // 查询某交易的全部未消费输出（按键序返回）
    std::vector<UtxoOut> GetUnspentByTx(const std::string& tx_hash) const;

    // 全部未消费输出数量（调试用）
    uint64_t Count() const;

private:
    static std::string MakeKey(const std::string& h, uint32_t ui, uint32_t vj);

    std::unique_ptr<rocksdb::DB> db_;
};

}  // namespace hubsql
