#include <gtest/gtest.h>

#include "model/transaction.h"
#include "parser/parsers/invest_parser.h"
#include "parser/parsers/staking_parser.h"
#include "parser/parsers/unstaking_parser.h"

using namespace hubsql;

namespace {

// 构造一个含单个真实输出的交易（legacy：投资解析器使用）
Transaction MakeTx(uint64_t type, const std::string& addr = "addr_owner") {
    Transaction tx;
    tx.hash     = "0x" + std::to_string(type);
    tx.identity = "0x" + std::to_string(type);
    tx.time     = 123;
    tx.type     = type;

    Utxo u;
    u.owner = {addr};
    u.vout  = {{"100.5", addr}};
    tx.utxos = {u};
    return tx;
}

// 构造质押交易（type=2，含 data.txInfo）
Transaction MakeStakeTx(const std::string& addr,
                        const std::string& tx_hash = "0xstake1") {
    Transaction tx;
    tx.hash = tx_hash;
    tx.type = 2;
    tx.time = 1787278243920870ULL;
    Utxo u;
    u.owner = {addr};
    tx.utxos = {u};
    tx.data = R"({"txInfo":{"stakeAmount":10000000000000,"commissionRate":0.07,"stakeType":"Net"}})";
    return tx;
}

// 构造解质押交易（type=3，含 data.txInfo.unstakeUtxo）
Transaction MakeUnstakeTx(const std::string& ref_stake_hash) {
    Transaction tx;
    tx.hash = "0xunstake1";
    tx.type = 3;
    tx.time = 1787278665779694ULL;
    Utxo u;
    u.owner = {"0x755Ccf704E17570b64E247f0794314e4C8E542CA"};
    tx.utxos = {u};
    tx.data = "{\"txInfo\":{\"unstakeUtxo\":\"" + ref_stake_hash + "\"}}";
    return tx;
}

}  // namespace

TEST(ParserTest, StakingParser) {
    auto tx = MakeStakeTx("0x755Ccf704E17570b64E247f0794314e4C8E542CA");

    StakingParser p;
    EXPECT_EQ(p.GetTxType(), "2");

    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0xstake1");
    EXPECT_EQ(recs[0].address, "0x755Ccf704E17570b64E247f0794314e4C8E542CA");
    EXPECT_EQ(recs[0].amount, "10000000000000");
    EXPECT_EQ(recs[0].time, 1787278243920870ULL);
    EXPECT_EQ(recs[0].commission_rate, "0.070000");
    EXPECT_EQ(recs[0].stake_type, "Net");
    EXPECT_FALSE(recs[0].is_unstaked);
}

TEST(ParserTest, StakingParserMissingData) {
    // 无 data.txInfo 的交易不被识别为质押
    auto tx = MakeTx(2, "addr_staking");
    StakingParser p;
    EXPECT_TRUE(p.Parse(tx).empty());
}

TEST(ParserTest, UnstakingParser) {
    auto tx = MakeUnstakeTx("0xstake1");
    UnstakingParser p;
    EXPECT_EQ(p.GetTxType(), "3");  // 真实链 UNSTAKE = type 3
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0xunstake1");
    EXPECT_EQ(recs[0].stake_tx_hash, "0xstake1");
    EXPECT_EQ(recs[0].time, 1787278665779694ULL);
}

TEST(ParserTest, UnstakingParserMissingRef) {
    // 解质押交易缺少 unstakeUtxo 则不产出记录
    auto tx = MakeTx(3, "addr_unstake");
    UnstakingParser p;
    EXPECT_TRUE(p.Parse(tx).empty());
}

TEST(ParserTest, InvestParser) {
    auto tx = MakeTx(3, "addr_invest");
    InvestParser p;
    EXPECT_EQ(p.GetTxType(), "3");
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0x3");
    EXPECT_EQ(recs[0].address, "addr_invest");
    EXPECT_EQ(recs[0].amount, "100.5");
}

TEST(ParserTest, TypeMismatchReturnsEmpty) {
    auto tx = MakeTx(99, "addr_bonus");
    InvestParser p;
    EXPECT_TRUE(p.Parse(tx).empty());
}
