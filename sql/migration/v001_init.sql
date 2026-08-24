-- ============================================================
-- migration/v001_init.sql —— 初始版本迁移
-- 说明：首次建库时执行，包含基础表结构。
--       后续结构变更请新增 v002_xxx.sql，保持增量式管理。
-- ============================================================

CREATE TABLE IF NOT EXISTS blocks (
    height       BIGINT UNSIGNED PRIMARY KEY,
    hash         VARCHAR(128) NOT NULL,
    timestamp    BIGINT UNSIGNED NOT NULL,
    tx_count     INT UNSIGNED NOT NULL DEFAULT 0,
    parse_status TINYINT NOT NULL DEFAULT 0 COMMENT '0=待处理 1=成功 2=失败',
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_hash (hash)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS transactions (
    tx_hash      VARCHAR(128) PRIMARY KEY,
    block_height BIGINT UNSIGNED NOT NULL,
    type         VARCHAR(32) NOT NULL,
    from_addr    VARCHAR(128) NOT NULL,
    to_addr      VARCHAR(128) DEFAULT NULL,
    amount       DECIMAL(36, 18) DEFAULT 0,
    raw_json     JSON DEFAULT NULL,
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    KEY idx_block_height (block_height),
    KEY idx_type (type),
    KEY idx_from_addr (from_addr)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS staking_records (
    id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash      VARCHAR(128) NOT NULL,
    block_height BIGINT UNSIGNED NOT NULL,
    address      VARCHAR(128) NOT NULL,
    amount       DECIMAL(36, 18) NOT NULL DEFAULT 0,
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_address (address)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS unstaking_records (
    id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash      VARCHAR(128) NOT NULL,
    block_height BIGINT UNSIGNED NOT NULL,
    address      VARCHAR(128) NOT NULL,
    amount       DECIMAL(36, 18) NOT NULL DEFAULT 0,
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_address (address)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS investment_records (
    id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash      VARCHAR(128) NOT NULL,
    block_height BIGINT UNSIGNED NOT NULL,
    address      VARCHAR(128) NOT NULL,
    amount       DECIMAL(36, 18) NOT NULL DEFAULT 0,
    product_id   VARCHAR(64) DEFAULT NULL,
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_address (address)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS sync_status (
    id                 INT PRIMARY KEY AUTO_INCREMENT,
    last_synced_height BIGINT UNSIGNED NOT NULL DEFAULT 0,
    status             VARCHAR(16) NOT NULL DEFAULT 'running',
    updated_at         TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS account_balances (
    address    VARCHAR(128) PRIMARY KEY,
    balance    BIGINT NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    KEY idx_balance (balance)
) ENGINE=InnoDB;

INSERT IGNORE INTO sync_status (id, last_synced_height, status)
VALUES (1, 0, 'running');
