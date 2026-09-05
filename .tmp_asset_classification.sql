SELECT asset, JSON_UNQUOTE(JSON_EXTRACT(tx_info,'$.tokenContractAddr')) AS token_contract
FROM proposals;
SELECT block_height, is_flow_in, asset_type, recipient, tx_hash
FROM contract_records
WHERE is_flow_in = 1;
SELECT contract_address, account_address, balance
FROM erc20_balances;
