# 单元测试
add_executable(hubsql_tests
  test_config.cpp
  test_token_metadata.cpp
  test_erc20_parameter_parser.cpp
  test_parser.cpp
)
target_link_libraries(hubsql_tests PRIVATE hubsql_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(hubsql_tests)

# 端到端余额验证工具（离线处理区块 JSON 文件）
add_executable(hubsql_verify_balance verify_balance.cpp)
target_link_libraries(hubsql_verify_balance PRIVATE hubsql_core)
