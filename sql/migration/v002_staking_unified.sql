-- ============================================================
-- v002: 质押表统一重构
-- 删除旧的质押表/解质押表，创建统一的质押表（解质押仅标记，不删除记录）
-- ============================================================

DROP TABLE IF EXISTS staking_records;
DROP TABLE IF EXISTS unstaking_records;

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
