#include <gtest/gtest.h>

#include "model/transaction.h"
#include "parser/parsers/claim_parser.h"
#include "parser/parsers/contract_parser.h"
#include "parser/parsers/fund_parser.h"
#include "parser/parsers/deinvest_parser.h"
#include "parser/parsers/invest_parser.h"
#include "parser/parsers/lock_parser.h"
#include "parser/parsers/proposal_parser.h"
#include "parser/parsers/revoke_parser.h"
#include "parser/parsers/staking_parser.h"
#include "parser/parsers/unlock_parser.h"
#include "parser/parsers/unstaking_parser.h"
#include "parser/parsers/vote_parser.h"

using namespace hubsql;

namespace {

// 构造一个含单个真实输出的交易
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

// 构造投资交易（type=4 DELEGATE，含 data.txInfo）
Transaction MakeInvestTx(const std::string& addr,
                         const std::string& tx_hash = "0xinvest1") {
    Transaction tx;
    tx.hash = tx_hash;
    tx.type = 4;
    tx.time = 1787278829843648ULL;
    Utxo u;
    u.owner = {addr};
    tx.utxos = {u};
    tx.data = "{\"txInfo\":{\"bonusAddr\":\"" + addr +
              "\",\"delegateAmount\":50000000000000,\"delegateType\":\"Normal\"}}";
    return tx;
}

// 构造解投资交易（type=5 UNDELEGATE，含 data.txInfo.undelegatingUtxo）
Transaction MakeDeinvestTx(const std::string& ref_invest_hash) {
    Transaction tx;
    tx.hash = "0xdeinvest1";
    tx.type = 5;
    tx.time = 1787278971000000ULL;
    Utxo u;
    u.owner = {"0x2E08bA82dcF52966dF03Aa0fd830aFAb70d8911d"};
    tx.utxos = {u};
    tx.data = "{\"txInfo\":{\"undelegatingUtxo\":\"" + ref_invest_hash + "\"}}";
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
    auto addr = "0x26e733E5481c079Abf859DDA613931298C9f0251";
    auto tx   = MakeInvestTx(addr);

    InvestParser p;
    EXPECT_EQ(p.GetTxType(), "4");  // DELEGATE

    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0xinvest1");
    EXPECT_EQ(recs[0].address, addr);
    EXPECT_EQ(recs[0].amount, "50000000000000");
    EXPECT_EQ(recs[0].time, 1787278829843648ULL);
    EXPECT_EQ(recs[0].bonus_addr, addr);
    EXPECT_EQ(recs[0].invest_type, "Normal");
    EXPECT_FALSE(recs[0].is_deinvested);
}

TEST(ParserTest, InvestParserMissingData) {
    // 无 data.txInfo 的交易不被识别为投资
    auto tx = MakeTx(4, "addr_invest");
    InvestParser p;
    EXPECT_TRUE(p.Parse(tx).empty());
}

TEST(ParserTest, DeinvestParser) {
    auto tx = MakeDeinvestTx("0xinvest1");
    DeinvestParser p;
    EXPECT_EQ(p.GetTxType(), "5");  // UNDELEGATE
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0xdeinvest1");
    EXPECT_EQ(recs[0].invest_tx_hash, "0xinvest1");
    EXPECT_EQ(recs[0].time, 1787278971000000ULL);
}

TEST(ParserTest, DeinvestParserMissingRef) {
    // 解投资交易缺少 undelegatingUtxo 则不产出记录
    auto tx = MakeTx(5, "addr_deinvest");
    DeinvestParser p;
    EXPECT_TRUE(p.Parse(tx).empty());
}

TEST(ParserTest, TypeMismatchReturnsEmpty) {
    auto tx = MakeTx(99, "addr_bonus");
    InvestParser p;
    EXPECT_TRUE(p.Parse(tx).empty());
}

// ---- 提案 / 撤销 / 投票 ----

TEST(ParserTest, ProposalParser) {
    auto addr = "0x755Ccf704E17570b64E247f0794314e4C8E542CA";
    Transaction tx;
    tx.hash = "0xproposal1";
    tx.type = 11;
    tx.time = 1787277289582392ULL;
    Utxo u;
    u.owner = {addr};
    tx.utxos = {u};
    tx.data = R"({"txInfo":{"name":"T0hJ","minVoteNum":1,"title":"MA=="}})";

    ProposalParser p;
    EXPECT_EQ(p.GetTxType(), "11");
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0xproposal1");
    EXPECT_EQ(recs[0].address, addr);
    EXPECT_FALSE(recs[0].tx_info.empty());
    // tx_info 应为 JSON 字符串，含 name/minVoteNum
    auto ti = nlohmann::json::parse(recs[0].tx_info);
    EXPECT_EQ(ti["minVoteNum"], 1);
}

TEST(ParserTest, RevokeParser) {
    Transaction tx;
    tx.hash = "0xrevoke1";
    tx.type = 12;
    tx.time = 1787280245333273ULL;
    Utxo u;
    u.owner = {"0x755Ccf704E17570b64E247f0794314e4C8E542CA"};
    tx.utxos = {u};
    tx.data = "{\"txInfo\":{\"proposalHash\":\"0xOHI\"}}";

    RevokeParser p;
    EXPECT_EQ(p.GetTxType(), "12");
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0xrevoke1");
    EXPECT_EQ(recs[0].proposal_hash, "0xOHI");
    EXPECT_EQ(nlohmann::json::parse(recs[0].tx_info)["proposalHash"], "0xOHI");
    EXPECT_EQ(recs[0].time, 1787280245333273ULL);
}

TEST(ParserTest, VoteParser) {
    auto addr = "0x755Ccf704E17570b64E247f0794314e4C8E542CA";
    Transaction tx;
    tx.hash = "0xvote1";
    tx.type = 13;
    tx.time = 1787277289582392ULL;
    Utxo u;
    u.owner = {addr};
    tx.utxos = {u};
    tx.data = "{\"txInfo\":{\"voteHash\":\"0xOHI\",\"voteNumber\":1,\"voteTxType\":11,\"voteType\":1}}";

    VoteParser p;
    EXPECT_EQ(p.GetTxType(), "13");
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0xvote1");
    EXPECT_EQ(recs[0].address, addr);
    EXPECT_EQ(recs[0].proposal_hash, "0xOHI");   // 第一笔提案 -> OHI
    EXPECT_EQ(recs[0].proposal_type, 11);         // 被投票交易类型=提案
    EXPECT_EQ(recs[0].vote_type, 1);              // 1=赞成
    EXPECT_EQ(recs[0].vote_number, 1);
}

TEST(ParserTest, VoteParserMissingHash) {
    // 投票缺少 voteHash 则不产出记录
    auto tx = MakeTx(13, "addr_vote");
    VoteParser p;
    EXPECT_TRUE(p.Parse(tx).empty());
}

// ---- 锁定 / 解锁定 / 合约 ----

TEST(ParserTest, LockParser) {
    auto addr = "0x2E08bA82dcF52966dF03Aa0fd830aFAb70d8911d";
    Transaction tx;
    tx.hash = "0xlock1";
    tx.type = 9;
    tx.time = 1787279543777324ULL;
    Utxo u;
    u.owner = {addr};
    u.assetType = "OHI";
    tx.utxos = {u};
    tx.data = R"({"txInfo":{"lockAmount":10000000000,"lockType":"LockNet"}})";

    LockParser p;
    EXPECT_EQ(p.GetTxType(), "9");
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0xlock1");
    EXPECT_EQ(recs[0].address, addr);
    EXPECT_EQ(recs[0].asset_type, "OHI");   // 锁定资产类型
    EXPECT_EQ(recs[0].amount, "10000000000");
    EXPECT_EQ(recs[0].lock_type, "LockNet");
    EXPECT_FALSE(recs[0].is_unlocked);
}

TEST(ParserTest, UnlockParser) {
    Transaction tx;
    tx.hash = "0xunlock1";
    tx.type = 10;
    tx.time = 1787279543779999ULL;
    Utxo u;
    u.owner = {"0x2E08bA82dcF52966dF03Aa0fd830aFAb70d8911d"};
    tx.utxos = {u};
    tx.data = "{\"txInfo\":{\"unLockUtxo\":\"0xlock1\"}}";

    UnlockParser p;
    EXPECT_EQ(p.GetTxType(), "10");
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0xunlock1");
    EXPECT_EQ(recs[0].lock_tx_hash, "0xlock1");
    EXPECT_EQ(recs[0].time, 1787279543779999ULL);
}

TEST(ParserTest, ContractParserDeploy) {
    auto addr = "0x755Ccf704E17570b64E247f0794314e4C8E542CA";
    Transaction tx;
    tx.hash = "0xdeploy1";
    tx.type = 7;
    tx.time = 12345;
    Utxo u;
    u.owner = {addr};
    u.assetType = "OHI";
    tx.utxos = {u};
    tx.data = std::string("{\"txInfo\":{\"sender\":\"") + addr +
              "\",\"recipient\":\"0xR1\",\"baseFee\":\"10\"}}";

    ContractParser p;
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_type, "deploy");
    EXPECT_EQ(recs[0].recipient, "0xR1");
    EXPECT_FALSE(recs[0].is_flow_in);
    EXPECT_FALSE(recs[0].is_flow_out);
    EXPECT_EQ(recs[0].asset_type, "OHI");
}

TEST(ParserTest, ContractParserFlowIn) {
    auto addr = "0x755Ccf704E17570b64E247f0794314e4C8E542CA";
    Transaction tx;
    tx.hash = "0xflowin1";
    tx.type = 8;
    tx.time = 12346;
    Utxo u;
    u.owner = {addr};
    u.assetType = "OHI";
    u.vout = {{Vout{"5000000000000000", addr}}, {Vout{"0", "VirtualBurnGas"}}};
    tx.utxos = {u};
    tx.data = "{\"txInfo\":{\"callType\":\"FlowInTx\"}}";

    ContractParser p;
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_type, "call");
    EXPECT_TRUE(recs[0].is_flow_in);
    EXPECT_FALSE(recs[0].is_flow_out);
    EXPECT_EQ(recs[0].flow_in_amount, "5000000000000000");
    // 节点 UTXO 的值即为 8 位原始金额。
}

TEST(ParserTest, ContractParserFlowOut) {
    auto addr = "0x755Ccf704E17570b64E247f0794314e4C8E542CA";
    Transaction tx;
    tx.hash = "0xflowout1";
    tx.type = 8;
    tx.time = 12347;
    Utxo u;
    u.owner = {addr};
    u.assetType = "OHI";
    u.vout = {{Vout{"100000000", "VirtualCallFlowOutBurnGas"}},
              {Vout{"9900000000", addr}}};
    tx.utxos = {u};
    tx.data = "{\"txInfo\":{\"callType\":\"FlowOutTx\"}}";

    ContractParser p;
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_TRUE(recs[0].is_flow_out);
    EXPECT_FALSE(recs[0].is_flow_in);
    EXPECT_EQ(recs[0].flow_out_amount, "100000000");
    // 节点 UTXO 的值即为 8 位原始金额。
}

TEST(ParserTest, ContractParserUnknownType) {
    auto tx = MakeTx(1, "addr");  // type 1 非合约
    ContractParser p;
    EXPECT_TRUE(p.Parse(tx).empty());
}

// ---- 申领 ----

TEST(ParserTest, ClaimParser) {
    auto addr = "0x03bEE3dB52E736B6422DaC4668CC784d05cb2e63";
    Transaction tx;
    tx.hash = "0xclaim1";
    tx.type = 99;
    tx.time = 1787384124650457ULL;
    Utxo u;
    u.owner = {addr};
    u.assetType = "OHI";
    tx.utxos = {u};
    tx.data = R"({"txInfo":{"bonusAddrList":3,"bonusAmount":65143162138}})";

    ClaimParser p;
    EXPECT_EQ(p.GetTxType(), "99");
    auto recs = p.Parse(tx);
    ASSERT_EQ(recs.size(), 1);
    EXPECT_EQ(recs[0].tx_hash, "0xclaim1");
    EXPECT_EQ(recs[0].address, addr);
    EXPECT_EQ(recs[0].asset_type, "OHI");   // 申领资产类型
    EXPECT_EQ(recs[0].amount, "65143162138");  // 申领金额
    EXPECT_EQ(recs[0].time, 1787384124650457ULL);
}

TEST(ParserTest, ClaimParserMissingData) {
    // 无 data.txInfo 的交易不被识别为申领
    auto tx = MakeTx(99, "addr_claim");
    ClaimParser p;
    EXPECT_TRUE(p.Parse(tx).empty());
}

// ---- FUND / ERC20 transfer ----

TEST(ParserTest, FundParserAtHeight70Shape) {
    Transaction tx;
    tx.hash = "0x198779ef24a367cfa8ff92665054088f77454b89f763f15f9dbfcee3fab85370";
    tx.identity = "0x458e53542299BeC018A6118c7544C44d3b8911Df";
    tx.type = 8;
    tx.time = 1787988242436320ULL;
    tx.data = R"({"txInfo":{"input":"0xa9059cbb00000000000000000000000026e733e5481c079abf859dda613931298c9f0251000000000000000000000000000000000000000000000000000000012a05f200","recipient":"0x77c835D837666B7A23131A46bda35E6325E36e1B","sender":"0x755Ccf704E17570b64E247f0794314e4C8E542CA"}})";

    FundParser parser;
    auto records = parser.Parse(tx);
    ASSERT_EQ(records.size(), 1);
    EXPECT_EQ(records[0].sender, "0x755Ccf704E17570b64E247f0794314e4C8E542CA");
    EXPECT_EQ(records[0].recipient, "0x26e733e5481c079abf859dda613931298c9f0251");
    EXPECT_EQ(records[0].contract_address, "0x77c835D837666B7A23131A46bda35E6325E36e1B");
    EXPECT_EQ(records[0].amount, "5000000000");
}

TEST(ParserTest, FundParserIgnoresOtherContractCalls) {
    Transaction tx;
    tx.type = 8;
    tx.data = R"({"txInfo":{"input":"0x12345678"}})";
    FundParser parser;
    EXPECT_TRUE(parser.Parse(tx).empty());
}
