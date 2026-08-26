-- ============================================================
-- v005: 锁定/交易记录/合约交易 业务表
-- 锁定=LOCK(9)、解锁定=UNLOCK(10)；合约=DEPLOY(7)/CALL(8)（含跃入 FlowInTx/跃出 FlowOutTx）
-- 交易记录仅保留最新 20 个高度（滚动清理）
-- ============================================================

-- 锁定业务表（解锁定仅标记 is_unlocked，不删除记录）
CREATE TABLE IF NOT EXISTS lock_records (
    id              BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash         VARCHAR(128) NOT NULL COMMENT '锁定交易hash',
    block_height    BIGINT UNSIGNED NOT NULL COMMENT '锁定所在区块高度',
    address         VARCHAR(128) NOT NULL DEFAULT '' COMMENT '锁定地址',
    asset_type      VARCHAR(128) NOT NULL DEFAULT '' COMMENT '锁定资产类型(hash或OHI)',
    lock_amount     DECIMAL(36, 18) NOT NULL DEFAULT 0 COMMENT '锁定金额',
    lock_time       BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '锁定时间(微秒)',
    lock_type       VARCHAR(32) NOT NULL DEFAULT '' COMMENT '锁定类型(如 LockNet)',
    is_unlocked     TINYINT NOT NULL DEFAULT 0 COMMENT '是否已解锁定(0=否 1=是，不删除记录)',
    unlock_tx_hash  VARCHAR(128) DEFAULT NULL COMMENT '解锁定交易hash',
    unlock_time     BIGINT UNSIGNED DEFAULT NULL COMMENT '解锁定时间(微秒)',
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_address (address),
    KEY idx_asset_type (asset_type),
    KEY idx_is_unlocked (is_unlocked)
) ENGINE=InnoDB COMMENT='锁定记录(解锁定以标记形式存在)';

-- 交易记录表（滚动：仅保留最新 20 个高度）
CREATE TABLE IF NOT EXISTS tx_records (
    id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash      VARCHAR(128) NOT NULL COMMENT '交易hash',
    block_height BIGINT UNSIGNED NOT NULL COMMENT '交易所在区块高度',
    tx_type      VARCHAR(32) NOT NULL DEFAULT '' COMMENT '交易类型',
    utxo_json    JSON DEFAULT NULL COMMENT '交易 utxo (json)',
    created_at   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_block_height (block_height),
    KEY idx_tx_type (tx_type)
) ENGINE=InnoDB COMMENT='交易记录(仅保留最新20个高度)';

-- 合约交易表（DEPLOY=7 / CALL=8，含跃入 FlowInTx / 跃出 FlowOutTx）
CREATE TABLE IF NOT EXISTS contract_records (
    id              BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash         VARCHAR(128) NOT NULL COMMENT '交易hash',
    block_height    BIGINT UNSIGNED NOT NULL COMMENT '交易所在区块高度',
    address         VARCHAR(128) NOT NULL DEFAULT '' COMMENT '账户地址(sender)',
    sender          VARCHAR(128) NOT NULL DEFAULT '' COMMENT '发送者',
    recipient       VARCHAR(128) NOT NULL DEFAULT '' COMMENT '合约地址(recipient)',
    tx_type         VARCHAR(16) NOT NULL DEFAULT '' COMMENT 'deploy/call',
    is_flow_in      TINYINT NOT NULL DEFAULT 0 COMMENT '是否合约跃入',
    is_flow_out     TINYINT NOT NULL DEFAULT 0 COMMENT '是否合约跃出',
    flow_in_amount  DECIMAL(36, 18) NOT NULL DEFAULT 0 COMMENT '跃入金额',
    flow_out_amount DECIMAL(36, 18) NOT NULL DEFAULT 0 COMMENT '跃出金额',
    asset_type      VARCHAR(128) NOT NULL DEFAULT '' COMMENT '跃入跃出资产类型(hash或OHI)',
    tx_info         JSON DEFAULT NULL COMMENT 'txInfo (json)',
    tx_time         BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '交易时间(微秒)',
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_tx_type (tx_type),
    KEY idx_recipient (recipient),
    KEY idx_is_flow_in (is_flow_in),
    KEY idx_is_flow_out (is_flow_out),
    KEY idx_block_height (block_height)
) ENGINE=InnoDB COMMENT='合约交易记录(含跃入跃出)';
