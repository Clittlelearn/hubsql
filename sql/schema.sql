-- ============================================================
-- hubsql 数据库表结构 (schema.sql)
-- 区块链数据解析与展示系统
-- 说明：本文件为最新全量表结构，可重复执行（幂等）
-- ============================================================

-- 区块主表
CREATE TABLE IF NOT EXISTS blocks (
    height       BIGINT UNSIGNED PRIMARY KEY,
    hash         VARCHAR(128) NOT NULL,
    timestamp    BIGINT UNSIGNED NOT NULL,
    tx_count     INT UNSIGNED NOT NULL DEFAULT 0,
    parse_status TINYINT NOT NULL DEFAULT 0 COMMENT '0=待处理 1=成功 2=失败',
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_hash (hash)
) ENGINE=InnoDB COMMENT='区块主表';

-- 交易明细表
CREATE TABLE IF NOT EXISTS transactions (
    tx_hash      VARCHAR(128) PRIMARY KEY,
    block_height BIGINT UNSIGNED NOT NULL,
    type         VARCHAR(32) NOT NULL COMMENT 'staking/unstaking/invest/...',
    from_addr    VARCHAR(128) NOT NULL,
    to_addr      VARCHAR(128) DEFAULT NULL,
    amount       DECIMAL(36, 18) DEFAULT 0,
    raw_json     JSON DEFAULT NULL COMMENT '原始数据留档',
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    KEY idx_block_height (block_height),
    KEY idx_type (type),
    KEY idx_from_addr (from_addr)
) ENGINE=InnoDB COMMENT='交易明细表';

-- 质押业务表（质押与解质押统一：解质押仅标记，不删除记录）
CREATE TABLE IF NOT EXISTS staking_records (
    id               BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash          VARCHAR(128) NOT NULL COMMENT '质押交易hash',
    block_height     BIGINT UNSIGNED NOT NULL COMMENT '质押所在区块高度',
    address          VARCHAR(128) NOT NULL COMMENT '质押人地址',
    stake_amount     DECIMAL(36, 18) NOT NULL DEFAULT 0 COMMENT '质押金额',
    stake_time       BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '质押时间(微秒)',
    commission_rate  DECIMAL(10, 6) NOT NULL DEFAULT 0 COMMENT '佣金率(commissionRate)',
    stake_type       VARCHAR(32) NOT NULL DEFAULT '' COMMENT '质押类型(如 Net)',
    is_unstaked      TINYINT NOT NULL DEFAULT 0 COMMENT '是否已解质押(0=否 1=是，不删除记录)',
    unstake_tx_hash  VARCHAR(128) DEFAULT NULL COMMENT '解质押交易hash',
    unstake_time     BIGINT UNSIGNED DEFAULT NULL COMMENT '解质押时间(微秒)',
    created_at       TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_address (address),
    KEY idx_block_height (block_height),
    KEY idx_is_unstaked (is_unstaked)
) ENGINE=InnoDB COMMENT='质押记录(解质押以标记形式存在)';

-- 投资业务表（投资=DELEGATE(type4)、解投资=UNDELEGATE(type5)，解投资仅标记不删除）
CREATE TABLE IF NOT EXISTS investment_records (
    id                BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash           VARCHAR(128) NOT NULL COMMENT '投资交易hash',
    block_height      BIGINT UNSIGNED NOT NULL COMMENT '投资所在区块高度',
    address           VARCHAR(128) NOT NULL COMMENT '投资地址',
    invest_amount     DECIMAL(36, 18) NOT NULL DEFAULT 0 COMMENT '投资金额',
    invest_time       BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '投资时间(微秒)',
    bonus_addr        VARCHAR(128) NOT NULL DEFAULT '' COMMENT 'bonusAddr(被投资地址)',
    invest_type       VARCHAR(32) NOT NULL DEFAULT '' COMMENT '投资类型(如 Normal)',
    is_deinvested     TINYINT NOT NULL DEFAULT 0 COMMENT '是否已解投资(0=否 1=是，不删除记录)',
    deinvest_tx_hash  VARCHAR(128) DEFAULT NULL COMMENT '解投资交易hash',
    deinvest_time     BIGINT UNSIGNED DEFAULT NULL COMMENT '解投资时间(微秒)',
    created_at        TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_address (address),
    KEY idx_bonus_addr (bonus_addr),
    KEY idx_block_height (block_height),
    KEY idx_is_deinvested (is_deinvested)
) ENGINE=InnoDB COMMENT='投资记录(解投资以标记形式存在)';

-- 同步进度表
CREATE TABLE IF NOT EXISTS sync_status (
    id                 INT PRIMARY KEY AUTO_INCREMENT,
    last_synced_height BIGINT UNSIGNED NOT NULL DEFAULT 0,
    status             VARCHAR(16) NOT NULL DEFAULT 'running',
    updated_at         TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB COMMENT='同步进度';

-- 三类余额分表：原生 OHI、其他提案资产、EVM ERC20
CREATE TABLE IF NOT EXISTS ohi_balances (
    address    VARCHAR(128) PRIMARY KEY,
    balance    BIGINT NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    KEY idx_ohi_balance (balance)
) ENGINE=InnoDB COMMENT='OHI 原生资产余额';

CREATE TABLE IF NOT EXISTS proposal_asset_balances (
    address VARCHAR(128) NOT NULL, asset_type VARCHAR(128) NOT NULL COMMENT '提案 hash',
    balance BIGINT NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (address, asset_type), KEY idx_proposal_asset (asset_type, balance)
) ENGINE=InnoDB COMMENT='其他提案跃入资产余额';

CREATE TABLE IF NOT EXISTS erc20_contracts (
    contract_address VARCHAR(42) PRIMARY KEY, deploy_tx_hash VARCHAR(128) NOT NULL DEFAULT '',
    deployer_address VARCHAR(128) NOT NULL DEFAULT '', created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB COMMENT='链上 ERC20 合约';

-- uint256 最大 78 位，MySQL DECIMAL 最大 65 位，因此以十进制字符串无损存储
CREATE TABLE IF NOT EXISTS erc20_balances (
    contract_address VARCHAR(42) NOT NULL, account_address VARCHAR(42) NOT NULL,
    balance VARCHAR(78) NOT NULL DEFAULT '0',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (contract_address, account_address), KEY idx_erc20_account (account_address)
) ENGINE=InnoDB COMMENT='ERC20 合约内余额';

CREATE TABLE IF NOT EXISTS erc20_transfer_events (
    tx_hash VARCHAR(128) NOT NULL, log_index INT UNSIGNED NOT NULL,
    contract_address VARCHAR(42) NOT NULL,
    PRIMARY KEY (tx_hash, log_index)
) ENGINE=InnoDB COMMENT='ERC20 Transfer 去重记录';

CREATE TABLE IF NOT EXISTS account_erc20_contracts (
    account_address VARCHAR(42) NOT NULL, contract_address VARCHAR(42) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (account_address, contract_address), KEY idx_account_contract (contract_address)
) ENGINE=InnoDB COMMENT='用户账号与 ERC20 合约关联';

-- 初始化同步进度（仅当表为空时插入）
INSERT IGNORE INTO sync_status (id, last_synced_height, status)
VALUES (1, 0, 'running');
