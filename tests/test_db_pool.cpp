#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <thread>

#include "storage/db_pool.h"

namespace hubsql {
class DbPoolTestPeer {
public:
    static void Factory(DbPool& pool,
                        std::function<std::unique_ptr<sql::Connection>()> factory) {
        pool.connection_factory_ = std::move(factory);
    }
    static int Total(DbPool& pool) {
        std::lock_guard<std::mutex> lock(pool.mutex_);
        return pool.total_;
    }
};
}  // namespace hubsql

namespace {
using namespace hubsql;
using namespace std::chrono_literals;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Throw;

class MockConnection : public sql::Connection {
public:
    MOCK_METHOD(void, clearWarnings, (), (override));
    MOCK_METHOD(sql::Statement*, createStatement, (), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(void, commit, (), (override));
    MOCK_METHOD(bool, getAutoCommit, (), (override));
    MOCK_METHOD(sql::SQLString, getCatalog, (), (override));
    MOCK_METHOD(sql::Driver*, getDriver, (), (override));
    MOCK_METHOD(sql::SQLString, getSchema, (), (override));
    MOCK_METHOD(sql::SQLString, getClientInfo, (), (override));
    MOCK_METHOD(void, getClientOption, (const sql::SQLString&, void*), (override));
    MOCK_METHOD(sql::SQLString, getClientOption, (const sql::SQLString&), (override));
    MOCK_METHOD(sql::DatabaseMetaData*, getMetaData, (), (override));
    MOCK_METHOD(sql::enum_transaction_isolation, getTransactionIsolation, (), (override));
    MOCK_METHOD(const sql::SQLWarning*, getWarnings, (), (override));
    MOCK_METHOD(bool, isClosed, (), (override));
    MOCK_METHOD(bool, isReadOnly, (), (override));
    MOCK_METHOD(bool, isValid, (), (override));
    MOCK_METHOD(bool, reconnect, (), (override));
    MOCK_METHOD(sql::SQLString, nativeSQL, (const sql::SQLString&), (override));
    MOCK_METHOD(sql::PreparedStatement*, prepareStatement, (const sql::SQLString&), (override));
    MOCK_METHOD(sql::PreparedStatement*, prepareStatement, (const sql::SQLString&, int), (override));
    MOCK_METHOD(sql::PreparedStatement*, prepareStatement, (const sql::SQLString&, int*), (override));
    MOCK_METHOD(sql::PreparedStatement*, prepareStatement, (const sql::SQLString&, int, int), (override));
    MOCK_METHOD(sql::PreparedStatement*, prepareStatement, (const sql::SQLString&, int, int, int), (override));
    MOCK_METHOD(sql::PreparedStatement*, prepareStatement, (const sql::SQLString&, sql::SQLString*), (override));
    MOCK_METHOD(void, releaseSavepoint, (sql::Savepoint*), (override));
    MOCK_METHOD(void, rollback, (), (override));
    MOCK_METHOD(void, rollback, (sql::Savepoint*), (override));
    MOCK_METHOD(void, setAutoCommit, (bool), (override));
    MOCK_METHOD(void, setCatalog, (const sql::SQLString&), (override));
    MOCK_METHOD(void, setSchema, (const sql::SQLString&), (override));
    MOCK_METHOD(sql::Connection*, setClientOption, (const sql::SQLString&, const void*), (override));
    MOCK_METHOD(sql::Connection*, setClientOption, (const sql::SQLString&, const sql::SQLString&), (override));
    MOCK_METHOD(void, setHoldability, (int), (override));
    MOCK_METHOD(void, setReadOnly, (bool), (override));
    MOCK_METHOD(sql::Savepoint*, setSavepoint, (), (override));
    MOCK_METHOD(sql::Savepoint*, setSavepoint, (const sql::SQLString&), (override));
    MOCK_METHOD(void, setTransactionIsolation, (sql::enum_transaction_isolation), (override));
};

MysqlConfig TestConfig(int size = 1) {
    MysqlConfig cfg;
    cfg.pool_size = size;
    cfg.pool_acquire_timeout_ms = 100;
    return cfg;
}

class DbPoolTest : public ::testing::Test {
protected:
    DbPool pool{TestConfig()};
    int created{0};
    MockConnection* last{nullptr};
    std::unique_ptr<sql::Connection> NewConnection() {
        auto conn = std::make_unique<NiceMock<MockConnection>>();
        last = conn.get();
        ++created;
        ON_CALL(*last, isValid()).WillByDefault(Return(true));
        ON_CALL(*last, getAutoCommit()).WillByDefault(Return(true));
        EXPECT_CALL(*last, reconnect()).Times(0);
        return conn;
    }
    void SetUp() override {
        DbPoolTestPeer::Factory(pool, [this] { return NewConnection(); });
    }
};

TEST_F(DbPoolTest, ReusesHealthyConnections) {
    auto first = pool.Acquire();
    auto* address = first.get();
    first.reset();
    EXPECT_EQ(pool.Acquire().get(), address);
    EXPECT_EQ(created, 1);
}

TEST_F(DbPoolTest, FailedConnectDoesNotLeakSlots) {
    DbPoolTestPeer::Factory(pool, []() -> std::unique_ptr<sql::Connection> {
        throw sql::SQLException("Connection refused");
    });
    for (int i = 0; i < 3; ++i) {
        EXPECT_THROW(pool.Acquire(), sql::SQLException);
        EXPECT_EQ(DbPoolTestPeer::Total(pool), 0);
    }
    DbPoolTestPeer::Factory(pool, [this] { return NewConnection(); });
    EXPECT_NE(pool.Acquire(), nullptr);
}

TEST_F(DbPoolTest, InvalidIdleConnectionIsReplaced) {
    pool.Acquire().reset();
    ON_CALL(*last, isValid()).WillByDefault(Return(false));
    EXPECT_NE(pool.Acquire(), nullptr);
    EXPECT_EQ(created, 2);
    EXPECT_EQ(DbPoolTestPeer::Total(pool), 1);
}

TEST_F(DbPoolTest, IdlePingExceptionIsReplaced) {
    pool.Acquire().reset();
    ON_CALL(*last, isValid()).WillByDefault(Throw(sql::SQLException("Lost connection")));
    EXPECT_NE(pool.Acquire(), nullptr);
    EXPECT_EQ(created, 2);
}

TEST_F(DbPoolTest, BrokenReturnedConnectionIsDiscarded) {
    auto conn = pool.Acquire();
    ON_CALL(*last, isValid()).WillByDefault(Return(false));
    conn.reset();
    EXPECT_EQ(DbPoolTestPeer::Total(pool), 0);
    EXPECT_NE(pool.Acquire(), nullptr);
    EXPECT_EQ(created, 2);
}

TEST_F(DbPoolTest, ClosedConnectionIsDiscarded) {
    auto conn = pool.Acquire();
    ON_CALL(*last, isClosed()).WillByDefault(Return(true));
    conn.reset();
    EXPECT_EQ(DbPoolTestPeer::Total(pool), 0);
}

TEST_F(DbPoolTest, ResetFailureIsDiscardedWithoutThrowing) {
    auto conn = pool.Acquire();
    ON_CALL(*last, getAutoCommit()).WillByDefault(Return(false));
    EXPECT_CALL(*last, rollback()).WillOnce(Throw(sql::SQLException("Lost connection")));
    EXPECT_NO_THROW(conn.reset());
    EXPECT_EQ(DbPoolTestPeer::Total(pool), 0);
    EXPECT_NE(pool.Acquire(), nullptr);
}

TEST_F(DbPoolTest, AutoCommitResetFailureIsDiscarded) {
    auto conn = pool.Acquire();
    ON_CALL(*last, getAutoCommit()).WillByDefault(Return(false));
    EXPECT_CALL(*last, rollback()).Times(1);
    EXPECT_CALL(*last, setAutoCommit(true)).WillOnce(Throw(sql::SQLException("Lost connection")));
    conn.reset();
    EXPECT_EQ(DbPoolTestPeer::Total(pool), 0);
}

TEST_F(DbPoolTest, RollsBackBeforeRestoringAutoCommit) {
    auto conn = pool.Acquire();
    ON_CALL(*last, getAutoCommit()).WillByDefault(Return(false));
    ::testing::InSequence sequence;
    EXPECT_CALL(*last, rollback()).Times(1);
    EXPECT_CALL(*last, setAutoCommit(true)).Times(1);
    conn.reset();
    EXPECT_EQ(DbPoolTestPeer::Total(pool), 1);
}

TEST_F(DbPoolTest, SaturatedPoolTimesOutThenRecovers) {
    auto held = pool.Acquire();
    const auto start = std::chrono::steady_clock::now();
    EXPECT_THROW(pool.Acquire(), std::runtime_error);
    EXPECT_GE(std::chrono::steady_clock::now() - start, 80ms);
    EXPECT_LT(std::chrono::steady_clock::now() - start, 2s);
    held.reset();
    EXPECT_NE(pool.Acquire(), nullptr);
}

TEST_F(DbPoolTest, ReturnWakesWaitingBorrower) {
    auto held = pool.Acquire();
    auto waiting = std::async(std::launch::async, [this] { return pool.Acquire(); });
    EXPECT_EQ(waiting.wait_for(20ms), std::future_status::timeout);
    held.reset();
    EXPECT_NE(waiting.get(), nullptr);
}

TEST_F(DbPoolTest, DoesNotReplayFailedCallback) {
    int calls = 0;
    EXPECT_THROW(pool.WithConnection([&](sql::Connection&) {
        ++calls;
        throw sql::SQLException("Commit result unknown");
    }), sql::SQLException);
    EXPECT_EQ(calls, 1);
}

TEST(DbPoolConcurrencyTest, SlowConnectDoesNotBlockReturn) {
    DbPool pool(TestConfig(2));
    auto make = []() -> std::unique_ptr<sql::Connection> {
        auto conn = std::make_unique<NiceMock<MockConnection>>();
        ON_CALL(*conn, isValid()).WillByDefault(Return(true));
        ON_CALL(*conn, getAutoCommit()).WillByDefault(Return(true));
        return conn;
    };
    DbPoolTestPeer::Factory(pool, make);
    auto held = pool.Acquire();
    std::promise<void> entered, resume;
    auto gate = resume.get_future();
    DbPoolTestPeer::Factory(pool, [&] {
        entered.set_value();
        gate.wait();
        return make();
    });
    auto connecting = std::async(std::launch::async, [&] { return pool.Acquire(); });
    entered.get_future().wait();
    auto returning = std::async(std::launch::async, [&] { held.reset(); });
    EXPECT_EQ(returning.wait_for(100ms), std::future_status::ready);
    resume.set_value();
    returning.get();
    EXPECT_NE(connecting.get(), nullptr);
}

TEST(DbPoolConfigTest, RejectsInvalidLimits) {
    auto cfg = TestConfig(0);
    EXPECT_THROW(DbPool{cfg}, std::invalid_argument);
    cfg = TestConfig();
    cfg.pool_acquire_timeout_ms = 0;
    EXPECT_THROW(DbPool{cfg}, std::invalid_argument);
    cfg = TestConfig();
    cfg.port = 65536;
    EXPECT_THROW(DbPool{cfg}, std::invalid_argument);
    cfg = TestConfig();
    cfg.read_timeout_seconds = -1;
    EXPECT_THROW(DbPool{cfg}, std::invalid_argument);
}

TEST(DbPoolConcurrencyTest, FailedConnectWakesWaitingBorrower) {
    auto cfg = TestConfig();
    cfg.pool_acquire_timeout_ms = 1000;
    DbPool pool(cfg);
    std::promise<void> entered, fail;
    auto gate = fail.get_future();
    std::atomic<int> attempts{0};
    DbPoolTestPeer::Factory(pool, [&]() -> std::unique_ptr<sql::Connection> {
        if (++attempts == 1) {
            entered.set_value();
            gate.wait();
            throw sql::SQLException("Connection refused");
        }
        auto conn = std::make_unique<NiceMock<MockConnection>>();
        ON_CALL(*conn, isValid()).WillByDefault(Return(true));
        ON_CALL(*conn, getAutoCommit()).WillByDefault(Return(true));
        return conn;
    });
    auto first = std::async(std::launch::async, [&] { return pool.Acquire(); });
    entered.get_future().wait();
    auto second = std::async(std::launch::async, [&] { return pool.Acquire(); });
    EXPECT_EQ(second.wait_for(20ms), std::future_status::timeout);
    fail.set_value();
    EXPECT_THROW(first.get(), sql::SQLException);
    EXPECT_EQ(second.wait_for(500ms), std::future_status::ready);
    EXPECT_NE(second.get(), nullptr);
    EXPECT_EQ(DbPoolTestPeer::Total(pool), 1);
}

int ConnectionId(sql::Connection& conn) {
    std::unique_ptr<sql::Statement> statement(conn.createStatement());
    std::unique_ptr<sql::ResultSet> rows(statement->executeQuery("SELECT CONNECTION_ID()"));
    if (!rows->next()) throw std::runtime_error("Missing connection id");
    return rows->getInt(1);
}

// Opt-in: opens dedicated sessions and kills only a session created by this test.
// No persistent data changes and no MySQL/server or HubSQL restart.
TEST(DbPoolIntegrationTest, ReplacesKilledIdleSession) {
    const char* path = std::getenv("HUBSQL_DB_POOL_TEST_CONFIG");
    if (!path) GTEST_SKIP() << "Set HUBSQL_DB_POOL_TEST_CONFIG to enable live session test";
    auto cfg = AppConfig::LoadFromFile(path).mysql;
    cfg.pool_size = 1;
    DbPool pool(cfg), killer(cfg);
    auto conn = pool.Acquire();
    const int original = ConnectionId(*conn);
    conn.reset();
    auto control = killer.Acquire();
    std::unique_ptr<sql::Statement> statement(control->createStatement());
    statement->execute("KILL CONNECTION " + std::to_string(original));
    auto recovered = pool.Acquire();
    EXPECT_NE(ConnectionId(*recovered), original);
    EXPECT_EQ(DbPoolTestPeer::Total(pool), 1);
}
}  // namespace
