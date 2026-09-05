TRUNCATE TABLE ohi_balances;
TRUNCATE TABLE erc20_balances;
TRUNCATE TABLE erc20_transfer_events;
TRUNCATE TABLE proposal_asset_balances;
UPDATE sync_status SET last_synced_height = 0, status = 'running' WHERE id = 1;
