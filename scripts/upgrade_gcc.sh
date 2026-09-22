#!/usr/bin/env bash
# 将 gcc/g++ 升级到 13 (Ubuntu 22.04 jammy)
# 同时移除误混入的 noble (24.04) 软件源
# 用法: sudo bash scripts/upgrade_gcc.sh
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
  echo "错误: 请以 root 运行: sudo bash $0"
  exit 1
fi

echo "==> [1/5] 备份 sources.list"
if [ ! -f /etc/apt/sources.list.bak ]; then
  cp /etc/apt/sources.list /etc/apt/sources.list.bak
else
  cp /etc/apt/sources.list /etc/apt/sources.list.bak."$(date +%s)"
fi
echo "    已备份到 /etc/apt/sources.list.bak*"

echo "==> [2/5] 移除混入的 noble (24.04) 软件源"
if grep -q 'noble' /etc/apt/sources.list; then
  sed -i '/noble/d' /etc/apt/sources.list
  echo "    noble 源已移除"
else
  echo "    未发现 noble 源，跳过"
fi

echo "==> [3/5] 添加 ubuntu-toolchain-r/test PPA 并更新索引"
apt-get update -y
apt-get install -y software-properties-common
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update -y

echo "==> [4/5] 安装 gcc-13 / g++-13"
apt-get install -y gcc-13 g++-13

echo "==> [5/5] 将默认 gcc/g++ 切换到 13 (可逆，可通过 alternatives 切换回 11)"
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 110
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 130
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 110
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 130
update-alternatives --set gcc /usr/bin/gcc-13
update-alternatives --set g++ /usr/bin/g++-13

echo ""
echo "==> 验证结果"
gcc --version | head -1
g++ --version | head -1
echo ""
echo "完成! 如需切回 gcc-11: sudo update-alternatives --set gcc /usr/bin/gcc-11"
