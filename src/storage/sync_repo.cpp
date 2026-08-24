#include "storage/sync_repo.h"

#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

namespace hubsql {

SyncRepo::SyncRepo(DbPool& pool) : pool_(pool) {}

uint64_t SyncRepo::GetLastSyncedHeight() {
    return pool_.WithConnection([](sql::Connection& conn) -> uint64_t {
        std::unique_ptr<sql::Statement> stmt(conn.createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
            "SELECT last_synced_height FROM sync_status WHERE id = 1"));
        return res->next() ? res->getUInt64(1) : 0;
    });
}

void SyncRepo::UpdateProgress(uint64_t height) {
    pool_.WithConnection([&](sql::Connection& conn) {
        std::unique_ptr<sql::PreparedStatement> pstmt(conn.prepareStatement(
            "INSERT INTO sync_status (id, last_synced_height, status) "
            "VALUES (1, ?, 'running') "
            "ON DUPLICATE KEY UPDATE last_synced_height = ?"));
        pstmt->setUInt64(1, height);
        pstmt->setUInt64(2, height);
        pstmt->executeUpdate();
    });
}

}  // namespace hubsql
