#!/usr/bin/env bash
# Clear hubsql runtime data without dropping any tables or schema objects.
#
# Usage:
#   ./scripts/clear_data.sh --yes
#
# Optional database overrides (useful when testing against an external DB):
#   HUBSQL_DB_HOST=... HUBSQL_DB_PORT=3306 HUBSQL_DB_NAME=hubsql \
#   HUBSQL_DB_USER=... HUBSQL_DB_PASSWORD=... ./scripts/clear_data.sh --yes

set -euo pipefail

if [[ "${1:-}" != "--yes" ]]; then
  echo "Refusing to clear data without --yes."
  echo "Usage: $0 --yes"
  exit 2
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
UTXO_DIR="$PROJECT_DIR/data/utxo"

# Do not race the synchronizer: stop hubsql first, then run this script.
if pgrep -f '/build/src/hubsql( |$)' >/dev/null; then
  echo "hubsql is running. Stop it before clearing data to avoid an immediate resync."
  exit 1
fi

MYSQL_BIN="$PROJECT_DIR/deploy/mysql-portable/bin/mysql"
if [[ ! -x "$MYSQL_BIN" ]]; then
  MYSQL_BIN="${MYSQL_BIN_OVERRIDE:-mysql}"
fi

DB_HOST="${HUBSQL_DB_HOST:-127.0.0.1}"
DB_PORT="${HUBSQL_DB_PORT:-3306}"
DB_NAME="${HUBSQL_DB_NAME:-hubsql}"
DB_USER="${HUBSQL_DB_USER:-hubsql}"
DB_PASSWORD="${HUBSQL_DB_PASSWORD:-hubsql123456}"

if [[ "$UTXO_DIR" != "$PROJECT_DIR/data/utxo" ]]; then
  echo "Unexpected UTXO directory: $UTXO_DIR" >&2
  exit 1
fi

echo "Clearing data in ${DB_HOST}:${DB_PORT}/${DB_NAME} (tables are preserved)..."
MYSQL_PWD="$DB_PASSWORD" "$MYSQL_BIN" \
  --protocol=TCP --host="$DB_HOST" --port="$DB_PORT" --user="$DB_USER" \
  "$DB_NAME" <<'SQL'
SET FOREIGN_KEY_CHECKS = 0;
TRUNCATE TABLE blocks;
TRUNCATE TABLE transactions;
TRUNCATE TABLE staking_records;
TRUNCATE TABLE investment_records;
TRUNCATE TABLE proposals;
TRUNCATE TABLE votes;
TRUNCATE TABLE lock_records;
TRUNCATE TABLE tx_records;
TRUNCATE TABLE contract_records;
TRUNCATE TABLE claim_records;
TRUNCATE TABLE fund_records;
TRUNCATE TABLE ohi_balances;
TRUNCATE TABLE proposal_asset_balances;
TRUNCATE TABLE erc20_contracts;
TRUNCATE TABLE erc20_balances;
TRUNCATE TABLE erc20_transfer_events;
TRUNCATE TABLE account_erc20_contracts;
TRUNCATE TABLE sync_status;
INSERT INTO sync_status (id, last_synced_height, status) VALUES (1, 0, 'running');
SET FOREIGN_KEY_CHECKS = 1;
SQL

if [[ -d "$UTXO_DIR" ]]; then
  find "$UTXO_DIR" -mindepth 1 -delete
fi

echo "Done. Tables were kept; business data, balances, sync progress, and UTXO cache were cleared."
