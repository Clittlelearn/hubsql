-- ============================================================
-- v003: 投资表统一重构
-- 删除旧投资表，创建统一的投资表（投资=DELEGATE(type4)、解投资=UNDELEGATE(type5)，
-- 解投资仅标记不删除记录）
-- ============================================================

DROP TABLE IF EXISTS investment_records;

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
