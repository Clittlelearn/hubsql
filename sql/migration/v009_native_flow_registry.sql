-- Mirror HiveX Native Flow proposal finalization from parsed blocks.
-- Existing proposals remain pending and are finalized before the next parsed
-- block's transactions, using votes that were present in the parent state.

ALTER TABLE proposals
    ADD COLUMN revoke_tx_info JSON DEFAULT NULL AFTER revoke_tx_hash,
    ADD COLUMN native_flow_state VARCHAR(16) NOT NULL DEFAULT 'pending' AFTER revoke_time,
    ADD COLUMN revoke_state VARCHAR(16) NOT NULL DEFAULT 'none' AFTER native_flow_state,
    ADD COLUMN finalized_height BIGINT UNSIGNED DEFAULT NULL AFTER revoke_state,
    ADD COLUMN finalized_time BIGINT UNSIGNED DEFAULT NULL AFTER finalized_height,
    ADD KEY idx_native_flow_state (native_flow_state);

-- A successfully parsed FlowIn is conclusive historical proof that the node
-- registry had activated the asset. This keeps an upgraded index useful before
-- the next block arrives; proposals without that proof are finalized normally
-- at the next parsed block boundary.
UPDATE proposals p
SET p.native_flow_state = 'active',
    p.finalized_height = (
        SELECT MIN(cr.block_height) FROM contract_records cr
        WHERE cr.asset_type = p.asset AND cr.is_flow_in = 1
    ),
    p.finalized_time = (
        SELECT MIN(cr.tx_time) FROM contract_records cr
        WHERE cr.asset_type = p.asset AND cr.is_flow_in = 1
    )
WHERE p.is_revoked = 0 AND EXISTS (
    SELECT 1 FROM contract_records cr
    WHERE cr.asset_type = p.asset AND cr.is_flow_in = 1
);

-- Older hubsql versions marked a revoke immediately. A clean resync is the
-- authoritative migration path for databases containing revoke proposals.
