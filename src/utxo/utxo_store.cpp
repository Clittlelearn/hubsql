#include "utxo/utxo_store.h"

#include <rocksdb/db.h>
#include <rocksdb/options.h>

#include <filesystem>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "common/logger.h"

namespace hubsql {

UtxoStore::UtxoStore(const std::string& db_path) {
    std::filesystem::create_directories(db_path);

    rocksdb::Options opts;
    opts.create_if_missing = true;
    opts.max_open_files = -1;  // 全部缓存，简单部署

    rocksdb::DB* db = nullptr;
    rocksdb::Status st = rocksdb::DB::Open(opts, db_path, &db);
    if (!st.ok()) {
        throw std::runtime_error("RocksDB 打开失败: " + st.ToString());
    }
    db_.reset(db);
    LOG_INFO("RocksDB UTXO 存储已打开: {} (未消费输出 {} 条)", db_path, Count());
}

UtxoStore::~UtxoStore() = default;

std::string UtxoStore::MakeKey(const std::string& h, uint32_t ui, uint32_t vj) {
    return h + ":" + std::to_string(ui) + ":" + std::to_string(vj);
}

void UtxoStore::Put(const std::string& tx_hash, uint32_t utxo_i, uint32_t vout_j,
                    const std::string& addr, const std::string& value,
                    const std::string& asset_type) {
    nlohmann::json v{
        {"addr", addr}, {"value", value}, {"assetType", asset_type}};
    rocksdb::Status st =
        db_->Put(rocksdb::WriteOptions(), MakeKey(tx_hash, utxo_i, vout_j), v.dump());
    if (!st.ok()) {
        throw std::runtime_error("RocksDB Put 失败: " + st.ToString());
    }
}

bool UtxoStore::Delete(const std::string& tx_hash, uint32_t utxo_i, uint32_t vout_j) {
    auto key = MakeKey(tx_hash, utxo_i, vout_j);
    std::string val;
    rocksdb::Status st = db_->Get(rocksdb::ReadOptions(), key, &val);
    if (st.IsNotFound()) return false;
    if (!st.ok()) {
        throw std::runtime_error("RocksDB Get 失败: " + st.ToString());
    }
    rocksdb::Status d = db_->Delete(rocksdb::WriteOptions(), key);
    if (!d.ok()) {
        throw std::runtime_error("RocksDB Delete 失败: " + d.ToString());
    }
    return true;
}

std::vector<UtxoOut> UtxoStore::GetUnspentByTx(const std::string& tx_hash) const {
    std::vector<UtxoOut> out;
    const std::string prefix = tx_hash + ":";

    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        const std::string key = it->key().ToString();
        const std::string rest = key.substr(prefix.size());
        auto colon = rest.find(':');
        if (colon == std::string::npos) continue;

        UtxoOut o;
        o.utxo_i = static_cast<uint32_t>(std::stoul(rest.substr(0, colon)));
        o.vout_j = static_cast<uint32_t>(std::stoul(rest.substr(colon + 1)));
        try {
            auto j = nlohmann::json::parse(it->value().ToString());
            o.addr = j.value("addr", "");
            o.value = j.value("value", "0");
            o.asset_type = j.value("assetType", "");
        } catch (...) {
            continue;
        }
        out.push_back(std::move(o));
    }
    delete it;
    return out;
}

uint64_t UtxoStore::Count() const {
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    uint64_t n = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next()) ++n;
    delete it;
    return n;
}

}  // namespace hubsql
