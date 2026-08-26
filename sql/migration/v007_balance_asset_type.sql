-- ============================================================
-- v007: 账户余额表支持按资产类型区分
-- 链上余额按 (地址, 资产类型) 分别存储（ADDRESS_TO_BALANCE + addr + assetType）
-- 资产类型：OHI（原生）或合约/提案 hash
-- ============================================================

DROP TABLE IF EXISTS account_balances;

CREATE TABLE IF NOT EXISTS ohi_balances (
    address VARCHAR(128) PRIMARY KEY, balance BIGINT NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    KEY idx_ohi_balance (balance)
) ENGINE=InnoDB COMMENT='OHI 原生资产余额';
CREATE TABLE IF NOT EXISTS proposal_asset_balances (
    address VARCHAR(128) NOT NULL, asset_type VARCHAR(128) NOT NULL,
    balance BIGINT NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (address, asset_type), KEY idx_proposal_asset (asset_type, balance)
) ENGINE=InnoDB COMMENT='其他提案跃入资产余额';
CREATE TABLE IF NOT EXISTS erc20_contracts (
    contract_address VARCHAR(42) PRIMARY KEY, deploy_tx_hash VARCHAR(128) NOT NULL DEFAULT '',
    deployer_address VARCHAR(128) NOT NULL DEFAULT '', created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB COMMENT='链上 ERC20 合约';
CREATE TABLE IF NOT EXISTS erc20_balances (
    contract_address VARCHAR(42) NOT NULL, account_address VARCHAR(42) NOT NULL,
    balance VARCHAR(78) NOT NULL DEFAULT '0',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (contract_address, account_address), KEY idx_erc20_account (account_address)
) ENGINE=InnoDB COMMENT='ERC20 合约内余额';
CREATE TABLE IF NOT EXISTS erc20_transfer_events (
    tx_hash VARCHAR(128) NOT NULL, log_index INT UNSIGNED NOT NULL,
    contract_address VARCHAR(42) NOT NULL, PRIMARY KEY (tx_hash, log_index)
) ENGINE=InnoDB COMMENT='ERC20 Transfer 去重记录';
CREATE TABLE IF NOT EXISTS account_erc20_contracts (
    account_address VARCHAR(42) NOT NULL, contract_address VARCHAR(42) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (account_address, contract_address), KEY idx_account_contract (contract_address)
) ENGINE=InnoDB COMMENT='用户账号与 ERC20 合约关联';

-- 新余额表需要从链历史重建；其他业务表使用 INSERT IGNORE，可安全重放。
UPDATE sync_status SET last_synced_height = 0, status = 'running' WHERE id = 1;
