#include <gtest/gtest.h>
#include "api/token_metadata.h"

namespace hubsql {
TEST(TokenMetadata, DecodesDynamicAndBytes32Names) {
    EXPECT_EQ(DecodeTokenText("0x" + std::string(62, '0') + "20" +
        std::string(63, '0') + "3" + "484252" + std::string(58, '0')), "HBR");
    EXPECT_EQ(DecodeTokenText("0x484252" + std::string(58, '0')), "HBR");
    EXPECT_EQ(DecodeTokenDecimals("0x" + std::string(63, '0') + "8"), 8);
}
TEST(TokenMetadata, RejectsMalformedAbiInsteadOfDefaultingDecimals) {
    EXPECT_THROW(DecodeTokenText("0x"), std::exception);
    EXPECT_THROW(DecodeTokenText("0x" + std::string(64, 'f') + std::string(64, '0')), std::exception);
    EXPECT_THROW(DecodeTokenText("0x" + std::string(62, '0') + "20" + std::string(64, 'f')), std::exception);
    EXPECT_THROW(DecodeTokenDecimals("0x" + std::string(61, '0') + "100"), std::exception);
    EXPECT_THROW(DecodeTokenDecimals("0x" + std::string(63, '0') + "g"), std::exception);
}
}
