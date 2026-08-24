#!/usr/bin/env bash
#
# init_db.sh —— 建库、建用户、导入表结构
#
# 可通过环境变量覆盖：
#   DB_NAME  DB_USER  DB_PASS
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(dirname "$SCRIPT_DIR")"
RUN_DIR="$BASE_DIR/run"

DB_NAME="${DB_NAME:-hubsql}"
DB_USER="${DB_USER:-hubsql}"
DB_PASS="${DB_PASS:-hubsql123456}"
MYSQL="$BASE_DIR/bin/mysql"

log() { echo -e "\033[1;32m[init_db]\033[0m $*"; }
err() { echo -e "\033[1;31m[init_db:error]\033[0m $*" >&2; }

# 确认 MySQL 已运行
if ! "$BASE_DIR/bin/mysqladmin" --socket="$RUN_DIR/mysqld.sock" ping >/dev/null 2>&1; then
  err "MySQL 未运行，请先执行 start.sh"
  exit 1
fi

# 建库、建用户
log "创建数据库 $DB_NAME 与用户 $DB_USER ..."
"$MYSQL" -uroot --socket="$RUN_DIR/mysqld.sock" <<SQL
CREATE DATABASE IF NOT EXISTS \`$DB_NAME\`
  DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER IF NOT EXISTS '$DB_USER'@'localhost' IDENTIFIED BY '$DB_PASS';
CREATE USER IF NOT EXISTS '$DB_USER'@'127.0.0.1' IDENTIFIED BY '$DB_PASS';
GRANT ALL PRIVILEGES ON \`$DB_NAME\`.* TO '$DB_USER'@'localhost';
GRANT ALL PRIVILEGES ON \`$DB_NAME\`.* TO '$DB_USER'@'127.0.0.1';
FLUSH PRIVILEGES;
SQL

# 导入表结构
SCHEMA_FILE="$BASE_DIR/sql/schema.sql"
if [ -f "$SCHEMA_FILE" ]; then
  log "导入表结构 $SCHEMA_FILE ..."
  "$MYSQL" -u"$DB_USER" -p"$DB_PASS" --socket="$RUN_DIR/mysqld.sock" "$DB_NAME" < "$SCHEMA_FILE"
  log "✅ 表结构导入完成"
else
  log "未找到 $SCHEMA_FILE，跳过导入（请稍后手动导入）"
fi

log "=============================================="
log "✅ 数据库初始化完成"
echo "   数据库: $DB_NAME"
echo "   用户  : $DB_USER / $DB_PASS"
echo "   连接  : 127.0.0.1:3306 (项目 config.json 使用)"
log "=============================================="
