# hubsql_core：除 main 外的所有模块
file(GLOB_RECURSE HUBSQL_CORE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(REMOVE_ITEM HUBSQL_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(hubsql_core STATIC ${HUBSQL_CORE_SOURCES})
target_include_directories(hubsql_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hubsql_core PUBLIC
  nlohmann_json::nlohmann_json
  spdlog::spdlog
  Crow::Crow
  CURL::libcurl
  mysqlcppconn
  rocksdb
)

# 可执行程序：区块链数据采集 + REST API 服务
add_executable(hubsql main.cpp)
target_link_libraries(hubsql PRIVATE hubsql_core)
