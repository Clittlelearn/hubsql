#!/usr/bin/env bash
#
# stop.sh —— 优雅停止便携式 MySQL
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"
RUN_DIR="$BASE_DIR/run"

log() { echo -e "\033[1;32m[stop]\033[0m $*"; }

if [ ! -f "$RUN_DIR/mysqld.pid" ] || ! kill -0 "$(cat "$RUN_DIR/mysqld.pid")" 2>/dev/null; then
  log "MySQL 未在运行"
  exit 0
fi

log "优雅停止 MySQL ..."
"$BASE_DIR/bin/mysqladmin" --socket="$RUN_DIR/mysqld.sock" -uroot shutdown || true
sleep 2
log "✅ MySQL 已停止"
