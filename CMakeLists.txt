cmake_minimum_required(VERSION 3.20)
project(hubsql VERSION 0.1.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(HUBSQL_BUILD_TESTS "Build unit tests" ON)

# ============ 依赖管理 (FetchContent) ============
include(FetchContent)

# 1) nlohmann/json —— header-only
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)

# 2) spdlog —— 日志
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.14.1
)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)

# 3) asio —— 使用系统包 libasio-dev（crow 依赖），由 apt 提供 /usr/include/asio.hpp

# 4) crow —— header-only REST 服务
FetchContent_Declare(crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG        v1.2.0
)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)

# 5) rocksdb —— 从 GitHub 拉取源码编译（UTXO KV 存储）
FetchContent_Declare(rocksdb
  GIT_REPOSITORY https://github.com/facebook/rocksdb.git
  GIT_TAG        v8.6.7
  GIT_SHALLOW    TRUE
)
# 精简编译：仅核心库，关闭测试/工具/压缩库/内存分配器（避免额外系统依赖）
set(WITH_TESTS           OFF CACHE BOOL "" FORCE)
set(WITH_TOOLS           OFF CACHE BOOL "" FORCE)
set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_GFLAGS          OFF CACHE BOOL "" FORCE)
set(WITH_JMALLOC         OFF CACHE BOOL "" FORCE)
set(WITH_ZLIB            OFF CACHE BOOL "" FORCE)
set(WITH_BZIP2           OFF CACHE BOOL "" FORCE)
set(WITH_LZ4             OFF CACHE BOOL "" FORCE)
set(WITH_ZSTD            OFF CACHE BOOL "" FORCE)
set(WITH_SNAPPY          OFF CACHE BOOL "" FORCE)
set(FAIL_ON_WARNINGS     OFF CACHE BOOL "" FORCE)
set(PORTABLE             1  CACHE STRING "" FORCE)   # 1=基线便携编译（0=本机native，其他=当作架构名）

# 6) googletest —— 单元测试
if(HUBSQL_BUILD_TESTS)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(nlohmann_json spdlog crow rocksdb)
if(HUBSQL_BUILD_TESTS)
  FetchContent_MakeAvailable(googletest)
endif()

# ============ 系统库 ============
find_package(CURL REQUIRED)

# mysql-connector-c++（系统包 libmysqlcppconn-dev）
find_path(MYSQLCPPCONN_INCLUDE_DIR mysql_driver.h
  PATH_SUFFIXES mysql-cppconn REQUIRED)
find_library(MYSQLCPPCONN_LIBRARY mysqlcppconn REQUIRED)
add_library(mysqlcppconn SHARED IMPORTED)
set_target_properties(mysqlcppconn PROPERTIES
  IMPORTED_LOCATION "${MYSQLCPPCONN_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${MYSQLCPPCONN_INCLUDE_DIR}")

# ============ 子目录 ============
add_subdirectory(src)
if(HUBSQL_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
