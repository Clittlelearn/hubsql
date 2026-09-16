#include <gtest/gtest.h>

#include "storage/erc20_parameter_parser.h"

namespace hubsql {
namespace {

constexpr const char* kContract = "0x93f5700d562abc93e9298c26eac423f3a7c21eb2";
constexpr const char* kSender = "0x755ccf704e17570b64e247f0794314e4c8e542ca";
constexpr const char* kReceiver = "0xc1b157d18f7db7168ad4af97850d45c1d693967e";

nlohmann::json Info(const std::string& input,
                    const std::string& call_type = "") {
    nlohmann::json info{{"input", input}, {"recipient", kContract},
                        {"sender", kSender}};
    if (!call_type.empty()) info["callType"] = call_type;
    return info;
}

TEST(Erc20ParameterParserTest, ParsesTransfer) {
    Transaction tx; tx.type = 8;
    auto parsed = ParseErc20ParameterTransfer(
        tx, Info("0xa9059cbb000000000000000000000000c1b157d18f7db7168ad4af97850d45c1d693967e"
                 "0000000000000000000000000000000000000000000000000000000005f5e100"));
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->from, kSender);
    EXPECT_EQ(parsed->to, kReceiver);
    EXPECT_EQ(parsed->amount, "100000000");
}

TEST(Erc20ParameterParserTest, ParsesTransferFrom) {
    Transaction tx; tx.type = 8;
    auto parsed = ParseErc20ParameterTransfer(
        tx, Info("0x23b872dd000000000000000000000000755ccf704e17570b64e247f0794314e4c8e542ca"
                 "000000000000000000000000c1b157d18f7db7168ad4af97850d45c1d693967e"
                 "000000000000000000000000000000000000000000000000000000000000002a"));
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->from, kSender);
    EXPECT_EQ(parsed->to, kReceiver);
    EXPECT_EQ(parsed->amount, "42");
}

TEST(Erc20ParameterParserTest, ParsesFlowInBurn) {
    Transaction tx; tx.type = 8;
    auto parsed = ParseErc20ParameterTransfer(
        tx, Info("0x388814b3" + std::string(50, '0') + "11c37937e08000",
                 "FlowInTx"));
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->from, kSender);
    EXPECT_EQ(parsed->to, "0x0000000000000000000000000000000000000000");
    EXPECT_EQ(parsed->amount, "5000000000000000");
}

TEST(Erc20ParameterParserTest, ParsesFlowOutMint) {
    Transaction tx; tx.type = 8;
    auto parsed = ParseErc20ParameterTransfer(
        tx, Info("0x8e0cb3c1000000000000000000000000755ccf704e17570b64e247f0794314e4c8e542ca"
                 "0000000000000000000000000000000000000000000000000000000005f5e100",
                 "FlowOutTx"));
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->from, "0x0000000000000000000000000000000000000000");
    EXPECT_EQ(parsed->to, kSender);
    EXPECT_EQ(parsed->amount, "100000000");
}

TEST(Erc20ParameterParserTest, RequiresFlowCallType) {
    Transaction tx; tx.type = 8;
    EXPECT_FALSE(ParseErc20ParameterTransfer(
        tx, Info("0x388814b300000000000000000000000000000000000000000000000000000001")));
}

TEST(Erc20ParameterParserTest, ParsesDeploymentInitialSupply) {
    Transaction tx; tx.type = 7;
    // 一小段伪字节码 + 与链上 ERC20 相同的 7 参数 ABI 构造数据。
    const std::string abi =
        "00000000000000000000000000000000000000000000000000000000000000e0"
        "0000000000000000000000000000000000000000000000000000000000000120"
        "0000000000000000000000000000000000000000000000000000000000000008"
        "000000000000000000000000000000000000000000000000016345785d8a0000"
        "0000000000000000000000000000000000000000000000000000000000000160"
        "000000000000000000000000755ccf704e17570b64e247f0794314e4c8e542ca"
        "000000000000000000000000c1b157d18f7db7168ad4af97850d45c1d693967e"
        "0000000000000000000000000000000000000000000000000000000000000003"
        "4f48490000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000003"
        "4f48490000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000003"
        "75726c0000000000000000000000000000000000000000000000000000000000";
    auto parsed = ParseErc20ParameterTransfer(tx, Info("0x60006000" + abi));
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->contract, kContract);
    EXPECT_EQ(parsed->from, "0x0000000000000000000000000000000000000000");
    EXPECT_EQ(parsed->to, kSender);
    EXPECT_EQ(parsed->amount, "100000000000000000");
}

}  // namespace
}  // namespace hubsql
