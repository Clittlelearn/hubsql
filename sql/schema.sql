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

-- 提案业务表（撤销提案仅标记，不删除记录）
CREATE TABLE IF NOT EXISTS proposals (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash VARCHAR(128) NOT NULL COMMENT '提案hash',
    asset VARCHAR(128) NOT NULL DEFAULT '' COMMENT '资产名(OHI 或提案hash)，用于投票/撤销关联',
    block_height BIGINT UNSIGNED NOT NULL COMMENT '提案所在区块高度',
    address VARCHAR(128) NOT NULL DEFAULT '' COMMENT '提案人地址',
    tx_info JSON DEFAULT NULL COMMENT '提案交易 txInfo',
    vote_count BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '提案投票数量',
    is_first TINYINT NOT NULL DEFAULT 0 COMMENT '是否第一笔提案(OHI提案)',
    is_revoked TINYINT NOT NULL DEFAULT 0 COMMENT '是否已撤销(0=否 1=是，不删除记录)',
    revoke_tx_hash VARCHAR(128) DEFAULT NULL COMMENT '撤销提案交易hash',
    revoke_time BIGINT UNSIGNED DEFAULT NULL COMMENT '撤销时间(微秒)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash), UNIQUE KEY uk_asset (asset),
    KEY idx_address (address), KEY idx_is_revoked (is_revoked), KEY idx_vote_count (vote_count)
) ENGINE=InnoDB COMMENT='提案记录(撤销以标记形式存在)';

-- 投票业务表
CREATE TABLE IF NOT EXISTS votes (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash VARCHAR(128) NOT NULL COMMENT '投票交易hash',
    block_height BIGINT UNSIGNED NOT NULL COMMENT '投票所在区块高度',
    address VARCHAR(128) NOT NULL DEFAULT '' COMMENT '投票人地址',
    proposal_hash VARCHAR(128) NOT NULL DEFAULT '' COMMENT '被投票的提案hash(第一笔提案为OHI)',
    proposal_type BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '被投票交易类型(voteTxType)',
    vote_type BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '投票类型(0=反对 1=赞成)',
    vote_number BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '票数',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash), KEY idx_proposal_hash (proposal_hash),
    KEY idx_address (address), KEY idx_block_height (block_height)
) ENGINE=InnoDB COMMENT='投票记录';

-- 锁定业务表（解锁定仅标记，不删除记录）
CREATE TABLE IF NOT EXISTS lock_records (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash VARCHAR(128) NOT NULL COMMENT '锁定交易hash',
    block_height BIGINT UNSIGNED NOT NULL COMMENT '锁定所在区块高度',
    address VARCHAR(128) NOT NULL DEFAULT '' COMMENT '锁定地址',
    asset_type VARCHAR(128) NOT NULL DEFAULT '' COMMENT '锁定资产类型(hash或OHI)',
    lock_amount DECIMAL(36,18) NOT NULL DEFAULT 0 COMMENT '锁定金额',
    lock_time BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '锁定时间(微秒)',
    lock_type VARCHAR(32) NOT NULL DEFAULT '' COMMENT '锁定类型(如 LockNet)',
    is_unlocked TINYINT NOT NULL DEFAULT 0 COMMENT '是否已解锁定(0=否 1=是，不删除记录)',
    unlock_tx_hash VARCHAR(128) DEFAULT NULL COMMENT '解锁定交易hash',
    unlock_time BIGINT UNSIGNED DEFAULT NULL COMMENT '解锁定时间(微秒)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash), KEY idx_address (address),
    KEY idx_asset_type (asset_type), KEY idx_is_unlocked (is_unlocked)
) ENGINE=InnoDB COMMENT='锁定记录(解锁定以标记形式存在)';

-- 交易记录表（滚动：仅保留最新 20 个高度）
CREATE TABLE IF NOT EXISTS tx_records (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash VARCHAR(128) NOT NULL COMMENT '交易hash',
    block_height BIGINT UNSIGNED NOT NULL COMMENT '交易所在区块高度',
    tx_type VARCHAR(32) NOT NULL DEFAULT '' COMMENT '交易类型',
    utxo_json JSON DEFAULT NULL COMMENT '交易 utxo (json)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash), KEY idx_block_height (block_height), KEY idx_tx_type (tx_type)
) ENGINE=InnoDB COMMENT='交易记录(仅保留最新20个高度)';

-- 合约交易表（DEPLOY=7 / CALL=8，含跃入/跃出）
CREATE TABLE IF NOT EXISTS contract_records (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash VARCHAR(128) NOT NULL COMMENT '交易hash',
    block_height BIGINT UNSIGNED NOT NULL COMMENT '交易所在区块高度',
    address VARCHAR(128) NOT NULL DEFAULT '' COMMENT '账户地址(sender)',
    sender VARCHAR(128) NOT NULL DEFAULT '' COMMENT '发送者',
    recipient VARCHAR(128) NOT NULL DEFAULT '' COMMENT '合约地址(recipient)',
    tx_type VARCHAR(16) NOT NULL DEFAULT '' COMMENT 'deploy/call',
    is_flow_in TINYINT NOT NULL DEFAULT 0 COMMENT '是否合约跃入',
    is_flow_out TINYINT NOT NULL DEFAULT 0 COMMENT '是否合约跃出',
    flow_in_amount DECIMAL(36,18) NOT NULL DEFAULT 0 COMMENT '跃入金额',
    flow_out_amount DECIMAL(36,18) NOT NULL DEFAULT 0 COMMENT '跃出金额',
    asset_type VARCHAR(128) NOT NULL DEFAULT '' COMMENT '跃入跃出资产类型(hash或OHI)',
    tx_info JSON DEFAULT NULL COMMENT 'txInfo (json)',
    tx_time BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '交易时间(微秒)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash), KEY idx_tx_type (tx_type), KEY idx_recipient (recipient),
    KEY idx_is_flow_in (is_flow_in), KEY idx_is_flow_out (is_flow_out), KEY idx_block_height (block_height)
) ENGINE=InnoDB COMMENT='合约交易记录(含跃入跃出)';

-- BONUS(type99) 申领业务表
CREATE TABLE IF NOT EXISTS claim_records (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash VARCHAR(128) NOT NULL COMMENT '申领交易hash',
    block_height BIGINT UNSIGNED NOT NULL COMMENT '申领所在区块高度',
    address VARCHAR(128) NOT NULL DEFAULT '' COMMENT '申领地址',
    asset_type VARCHAR(128) NOT NULL DEFAULT '' COMMENT '申领资产类型(hash或OHI)',
    claim_amount DECIMAL(36,18) NOT NULL DEFAULT 0 COMMENT '申领金额',
    claim_time BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '申领时间(微秒)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash), KEY idx_address (address), KEY idx_asset_type (asset_type)
) ENGINE=InnoDB COMMENT='申领记录';

-- FUND：ERC20 transfer(address,uint256) 资金发放记录（链上 type=8 合约调用）
CREATE TABLE IF NOT EXISTS fund_records (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash VARCHAR(128) NOT NULL COMMENT 'fund交易hash',
    block_height BIGINT UNSIGNED NOT NULL COMMENT '所在区块高度',
    sender VARCHAR(128) NOT NULL DEFAULT '' COMMENT '资金发送地址',
    recipient VARCHAR(128) NOT NULL DEFAULT '' COMMENT '资金接收地址',
    contract_address VARCHAR(128) NOT NULL DEFAULT '' COMMENT 'ERC20合约地址',
    fund_amount VARCHAR(78) NOT NULL DEFAULT '0' COMMENT 'ERC20最小单位金额(uint256，十进制字符串)',
    fund_time BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '交易时间(微秒)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_sender (sender), KEY idx_recipient (recipient),
    KEY idx_contract_address (contract_address), KEY idx_block_height (block_height)
) ENGINE=InnoDB COMMENT='ERC20资金发放记录';

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
