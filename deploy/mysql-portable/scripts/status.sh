#!/usr/bin/env bash
#
# status.sh —— 查看便携式 MySQL 运行状态
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"
RUN_DIR="$BASE_DIR/run"

if [ -f "$RUN_DIR/mysqld.pid" ] && kill -0 "$(cat "$RUN_DIR/mysqld.pid")" 2>/dev/null; then
  PID="$(cat "$RUN_DIR/mysqld.pid")"
  VER="$("$BASE_DIR/bin/mysql" -uroot --socket="$RUN_DIR/mysqld.sock" -N -e 'SELECT VERSION();' 2>/dev/null || echo '?')"
  echo "状态 : 运行中"
  echo "PID  : $PID"
  echo "版本 : $VER"
  echo "Socket: $RUN_DIR/mysqld.sock"
else
  echo "状态 : 未运行"
fi
