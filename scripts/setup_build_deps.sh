#!/usr/bin/env bash
#
# setup_build_deps.sh —— 安装 C++ 构建依赖（需 sudo）
# 仅安装系统包；json/spdlog/asio/crow/gtest 由 CMake FetchContent 自动拉取。
#
set -euo pipefail

echo "==> 安装构建工具与依赖库 ..."
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  git \
  libcurl4-openssl-dev \
  libmysqlcppconn-dev \
  libasio-dev

echo "==> 完成。接下来执行："
echo "   cmake -S . -B build"
echo "   cmake --build build -j"
