-- ============================================================
-- v006: 申领业务表
-- 申领=BONUS(type99)，txInfo.bonusAmount 为申领金额，asset_type 为资产类型
-- ============================================================

CREATE TABLE IF NOT EXISTS claim_records (
    id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash      VARCHAR(128) NOT NULL COMMENT '申领交易hash',
    block_height BIGINT UNSIGNED NOT NULL COMMENT '申领所在区块高度',
    address      VARCHAR(128) NOT NULL DEFAULT '' COMMENT '申领地址',
    asset_type   VARCHAR(128) NOT NULL DEFAULT '' COMMENT '申领资产类型(hash或OHI)',
    claim_amount DECIMAL(36, 18) NOT NULL DEFAULT 0 COMMENT '申领金额',
    claim_time   BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '申领时间(微秒)',
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_address (address),
    KEY idx_asset_type (asset_type)
) ENGINE=InnoDB COMMENT='申领记录';
