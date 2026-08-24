#include "storage/block_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/sqlstring.h>

#include <stdexcept>

namespace hubsql {

BlockRepo::BlockRepo(DbPool& pool) : pool_(pool) {}

void BlockRepo::Insert(sql::Connection& conn, const Block& block) {
    std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
        "INSERT IGNORE INTO blocks (height, hash, timestamp, tx_count, parse_status) "
        "VALUES (?, ?, 0, ?, 1)"));
    pstmt->setUInt64(1, block.blocks.height);
    pstmt->setString(2, block.blocks.hash);
    pstmt->setUInt(3, static_cast<unsigned int>(block.txs.size()));
    pstmt->executeUpdate();
}

std::optional<Block> BlockRepo::FindByHeight(uint64_t height) {
    return pool_.WithConnection([&](sql::Connection& conn) -> std::optional<Block> {
        std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
            "SELECT height, hash, tx_count FROM blocks WHERE height = ?"));
        pstmt->setUInt64(1, height);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) {
            Block b;
            b.blocks.height = res->getUInt64("height");
            b.blocks.hash   = std::string(res->getString("hash").c_str());
            return b;
        }
        return std::nullopt;
    });
}

std::vector<Block> BlockRepo::List(uint64_t start, uint64_t end, int page, int size) {
    return pool_.WithConnection([&](sql::Connection& conn) -> std::vector<Block> {
        std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
            "SELECT height, hash, tx_count FROM blocks "
            "WHERE height BETWEEN ? AND ? "
            "ORDER BY height DESC LIMIT ? OFFSET ?"));
        pstmt->setUInt64(1, start);
        pstmt->setUInt64(2, end);
        pstmt->setInt(3, size);
        pstmt->setInt(4, (page - 1) * size);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        std::vector<Block> blocks;
        while (res->next()) {
            Block b;
            b.blocks.height = res->getUInt64("height");
            b.blocks.hash   = std::string(res->getString("hash").c_str());
            blocks.push_back(std::move(b));
        }
        return blocks;
    });
}

int64_t BlockRepo::Count() {
    return pool_.WithConnection([](sql::Connection& conn) -> int64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT COUNT(*) FROM blocks"));
        return res->next() ? res->getInt64(1) : 0;
    });
}

}  // namespace hubsql
