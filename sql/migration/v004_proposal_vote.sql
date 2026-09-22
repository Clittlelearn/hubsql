-- ============================================================
-- v004: 提案/投票业务表
-- 提案=PROPOSAL(type11)、撤销提案=REVOKEPROPOSAL(type12)、投票=VOTE(type13)
-- 资产命名：第一笔提案的资产=OHI，之后提案的资产=提案hash（用于投票/撤销关联）
-- 撤销提案仅标记 is_revoked，不删除记录
-- ============================================================

CREATE TABLE IF NOT EXISTS proposals (
    id              BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash         VARCHAR(128) NOT NULL COMMENT '提案hash',
    asset           VARCHAR(128) NOT NULL DEFAULT '' COMMENT '资产名(OHI 或提案hash)，用于投票/撤销关联',
    block_height    BIGINT UNSIGNED NOT NULL COMMENT '提案所在区块高度',
    address         VARCHAR(128) NOT NULL DEFAULT '' COMMENT '提案人地址',
    tx_info         JSON DEFAULT NULL COMMENT '提案交易 txInfo',
    vote_count      BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '提案投票数量',
    is_first        TINYINT NOT NULL DEFAULT 0 COMMENT '是否第一笔提案(OHI提案)',
    is_revoked      TINYINT NOT NULL DEFAULT 0 COMMENT '是否已撤销(0=否 1=是，不删除记录)',
    revoke_tx_hash  VARCHAR(128) DEFAULT NULL COMMENT '撤销提案交易hash',
    revoke_tx_info  JSON DEFAULT NULL COMMENT '撤销提案 txInfo',
    revoke_time     BIGINT UNSIGNED DEFAULT NULL COMMENT '撤销时间(微秒)',
    native_flow_state VARCHAR(16) NOT NULL DEFAULT 'pending' COMMENT 'pending/active/missing/revoked/ineligible',
    revoke_state VARCHAR(16) NOT NULL DEFAULT 'none' COMMENT 'none/pending/revoked/missing',
    finalized_height BIGINT UNSIGNED DEFAULT NULL,
    finalized_time BIGINT UNSIGNED DEFAULT NULL COMMENT '裁决区块时间(微秒)',
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    UNIQUE KEY uk_asset (asset),
    KEY idx_address (address),
    KEY idx_is_revoked (is_revoked),
    KEY idx_vote_count (vote_count),
    KEY idx_native_flow_state (native_flow_state)
) ENGINE=InnoDB COMMENT='提案记录(撤销以标记形式存在)';

CREATE TABLE IF NOT EXISTS votes (
    id             BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    tx_hash        VARCHAR(128) NOT NULL COMMENT '投票交易hash',
    block_height   BIGINT UNSIGNED NOT NULL COMMENT '投票所在区块高度',
    address        VARCHAR(128) NOT NULL DEFAULT '' COMMENT '投票人地址',
    proposal_hash  VARCHAR(128) NOT NULL DEFAULT '' COMMENT '被投票的提案hash(第一笔提案为OHI)',
    proposal_type  BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '被投票交易类型(voteTxType)',
    vote_type      BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '投票类型(0=反对 1=赞成)',
    vote_number    BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '票数',
    created_at     TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY uk_tx_hash (tx_hash),
    KEY idx_proposal_hash (proposal_hash),
    KEY idx_address (address),
    KEY idx_block_height (block_height)
) ENGINE=InnoDB COMMENT='投票记录';
