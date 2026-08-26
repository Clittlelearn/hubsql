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

-- 账户余额表（UTXO 未花费真实输出的汇总，供前端查询）
CREATE TABLE IF NOT EXISTS account_balances (
    address    VARCHAR(128) PRIMARY KEY,
    balance    BIGINT NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    KEY idx_balance (balance)
) ENGINE=InnoDB COMMENT='账户余额';

-- 初始化同步进度（仅当表为空时插入）
INSERT IGNORE INTO sync_status (id, last_synced_height, status)
VALUES (1, 0, 'running');
