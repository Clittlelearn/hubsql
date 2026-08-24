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

-- 质押业务表
CREATE TABLE IF NOT EXISTS staking_records (
    id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash      VARCHAR(128) NOT NULL,
    block_height BIGINT UNSIGNED NOT NULL,
    address      VARCHAR(128) NOT NULL,
    amount       DECIMAL(36, 18) NOT NULL DEFAULT 0,
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_address (address),
    KEY idx_block_height (block_height)
) ENGINE=InnoDB COMMENT='质押记录';

-- 解质押业务表
CREATE TABLE IF NOT EXISTS unstaking_records (
    id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash      VARCHAR(128) NOT NULL,
    block_height BIGINT UNSIGNED NOT NULL,
    address      VARCHAR(128) NOT NULL,
    amount       DECIMAL(36, 18) NOT NULL DEFAULT 0,
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_address (address),
    KEY idx_block_height (block_height)
) ENGINE=InnoDB COMMENT='解质押记录';

-- 投资业务表
CREATE TABLE IF NOT EXISTS investment_records (
    id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash      VARCHAR(128) NOT NULL,
    block_height BIGINT UNSIGNED NOT NULL,
    address      VARCHAR(128) NOT NULL,
    amount       DECIMAL(36, 18) NOT NULL DEFAULT 0,
    product_id   VARCHAR(64) DEFAULT NULL COMMENT '投资产品ID(业务专属字段)',
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_address (address),
    KEY idx_block_height (block_height)
) ENGINE=InnoDB COMMENT='投资记录';

-- 同步进度表
CREATE TABLE IF NOT EXISTS sync_status (
    id                 INT PRIMARY KEY AUTO_INCREMENT,
    last_synced_height BIGINT UNSIGNED NOT NULL DEFAULT 0,
    status             VARCHAR(16) NOT NULL DEFAULT 'running',
    updated_at         TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB COMMENT='同步进度';

-- 初始化同步进度（仅当表为空时插入）
INSERT IGNORE INTO sync_status (id, last_synced_height, status)
VALUES (1, 0, 'running');
