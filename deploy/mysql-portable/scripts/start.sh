#!/usr/bin/env bash
#
# start.sh —— 启动便携式 MySQL（守护方式）
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"
RUN_DIR="$BASE_DIR/run"

log() { echo -e "\033[1;32m[start]\033[0m $*"; }
err() { echo -e "\033[1;31m[start:error]\033[0m $*" >&2; }

# 已在运行则直接返回
if [ -f "$RUN_DIR/mysqld.pid" ] && kill -0 "$(cat "$RUN_DIR/mysqld.pid")" 2>/dev/null; then
  log "MySQL 已在运行 (pid=$(cat "$RUN_DIR/mysqld.pid"))"
  exit 0
fi

if [ ! -x "$BASE_DIR/bin/mysqld" ]; then
  err "未找到 mysqld，请先运行 install.sh"
  exit 1
fi

log "启动 mysqld ..."
nohup "$BASE_DIR/bin/mysqld" --defaults-file="$BASE_DIR/config/my.cnf" \
  --basedir="$BASE_DIR" --datadir="$BASE_DIR/data" \
  >>"$RUN_DIR/start.log" 2>&1 &

# 等待就绪（最多 30s）
for _ in $(seq 1 30); do
  if "$BASE_DIR/bin/mysqladmin" --socket="$RUN_DIR/mysqld.sock" ping >/dev/null 2>&1; then
    log "✅ MySQL 启动成功 (pid=$(cat "$RUN_DIR/mysqld.pid"))"
    exit 0
  fi
  sleep 1
done

err "MySQL 启动超时，请查看 $RUN_DIR/error.log"
exit 1
