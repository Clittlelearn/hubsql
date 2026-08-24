#!/usr/bin/env bash
#
# install.sh —— 便携式 MySQL 首次安装脚本
# 功能：安装系统依赖 -> 下载官方免安装二进制 -> 解压 -> 生成 my.cnf -> 初始化数据目录
#
set -euo pipefail

# ---------- 常量 ----------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"          # mysql-portable/
DATA_DIR="$BASE_DIR/data"
RUN_DIR="$BASE_DIR/run"
CONFIG_DIR="$BASE_DIR/config"

MYSQL_VER="${MYSQL_VER:-8.0.42}"
ARCH="${ARCH:-x86_64}"
TARBALL="mysql-${MYSQL_VER}-linux-glibc2.28-${ARCH}.tar.xz"
DOWNLOAD_URL="https://dev.mysql.com/get/Downloads/MySQL-8.0/${TARBALL}"

log() { echo -e "\033[1;32m[install]\033[0m $*"; }
err()  { echo -e "\033[1;31m[install:error]\033[0m $*" >&2; }

# ---------- 1. 检查并安装系统依赖 ----------
install_deps() {
  log "检查系统依赖 (libaio / libnuma / libncurses) ..."
  if command -v apt-get >/dev/null 2>&1; then
    # 1) libaio（Ubuntu 24.04 包名为 libaio1t64）
    if ! dpkg -s libaio1t64 >/dev/null 2>&1 && ! dpkg -s libaio1 >/dev/null 2>&1; then
      log "安装 libaio1t64 (Ubuntu 24.04 包名) ..."
      sudo apt-get update -y
      sudo apt-get install -y libaio1t64 || sudo apt-get install -y libaio1
    fi
    # 1b) Ubuntu 24.04 的 libaio1t64 只提供 libaio.so.1t64，需补 libaio.so.1 符号链接
    if ! ldconfig -p | grep -q "libaio.so.1 "; then
      if [ -f /usr/lib/x86_64-linux-gnu/libaio.so.1t64 ]; then
        log "检测到缺少 libaio.so.1，创建符号链接 (需 sudo) ..."
        sudo ln -sf /usr/lib/x86_64-linux-gnu/libaio.so.1t64 /usr/lib/x86_64-linux-gnu/libaio.so.1
      else
        err "缺少 libaio.so.1 且未找到 libaio.so.1t64，请手动安装 libaio 后重试"
        exit 1
      fi
    fi
    # 2) libnuma
    if ! dpkg -s libnuma1 >/dev/null 2>&1; then
      log "安装 libnuma1 ..."
      sudo apt-get install -y libnuma1
    fi
    # 3) libncurses（mysql 客户端需要 libncurses.so.6）
    if ! dpkg -s libncurses6 >/dev/null 2>&1; then
      log "安装 libncurses6 (mysql 客户端依赖) ..."
      sudo apt-get install -y libncurses6
    fi
  else
    log "未检测到 apt-get，请确认已安装 libaio、libnuma、libncurses"
  fi
}

# ---------- 2. 创建目录 ----------
prepare_dirs() {
  mkdir -p "$DATA_DIR" "$RUN_DIR" "$CONFIG_DIR" "$BASE_DIR/sql"
  log "目录就绪: $BASE_DIR"
}

# ---------- 3. 下载并解压 ----------
download_and_extract() {
  if [ -x "$BASE_DIR/bin/mysqld" ]; then
    log "已存在 mysqld 二进制，跳过下载"
    return
  fi
  log "下载 $TARBALL ..."
  # -C - 支持断点续传，网络中断后可重跑脚本继续下载
  curl -fSL -C - --retry 5 --retry-delay 3 -o "$BASE_DIR/$TARBALL" "$DOWNLOAD_URL" || {
    err "下载失败（可重新运行本脚本继续下载）"
    exit 1
  }
  log "解压中 ..."
  tar -xf "$BASE_DIR/$TARBALL" -C "$BASE_DIR" --strip-components=1
  rm -f "$BASE_DIR/$TARBALL"
  log "解压完成"
}

# ---------- 4. 生成 my.cnf（自动写入绝对路径，便于搬迁） ----------
generate_config() {
  cat > "$CONFIG_DIR/my.cnf" <<EOF
[mysqld]
basedir                 = $BASE_DIR
datadir                 = $DATA_DIR
socket                  = $RUN_DIR/mysqld.sock
pid-file                = $RUN_DIR/mysqld.pid
log-error               = $RUN_DIR/error.log
port                    = 3306
bind-address            = 127.0.0.1
character-set-server    = utf8mb4
collation-server        = utf8mb4_unicode_ci
max_connections         = 200
innodb_buffer_pool_size = 512M

[client]
socket                  = $RUN_DIR/mysqld.sock
default-character-set   = utf8mb4
EOF
  log "已生成 $CONFIG_DIR/my.cnf"
}

# ---------- 5. 初始化数据目录（root 免密，便于脚本建库） ----------
init_data() {
  if [ -d "$DATA_DIR/mysql" ]; then
    log "数据目录已初始化，跳过"
    return
  fi
  log "初始化数据目录 (--initialize-insecure) ..."
  "$BASE_DIR/bin/mysqld" --defaults-file="$CONFIG_DIR/my.cnf" \
    --initialize-insecure \
    --basedir="$BASE_DIR" --datadir="$DATA_DIR" \
    --user="$(id -un)" || {
      err "初始化失败，请查看 $RUN_DIR/error.log"
      exit 1
    }
  log "数据目录初始化完成"
}

# ---------- 入口 ----------
main() {
  install_deps
  prepare_dirs
  download_and_extract
  generate_config
  init_data
  log "=============================================="
  log "✅ 安装完成！接下来依次执行："
  echo "   1) 启动:   $SCRIPT_DIR/start.sh"
  echo "   2) 建库:   $SCRIPT_DIR/init_db.sh"
  echo "   3) 状态:   $SCRIPT_DIR/status.sh"
  log "=============================================="
}

main "$@"
