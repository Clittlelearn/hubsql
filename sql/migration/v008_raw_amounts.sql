-- All monetary values are stored as 8-decimal raw integer strings.
-- Run this migration, then clear and resync the indexed data so legacy
-- 8-decimal balances and business records are not mixed with raw-18 values.

ALTER TABLE transactions MODIFY amount VARCHAR(78) NOT NULL DEFAULT '0';
ALTER TABLE staking_records MODIFY stake_amount VARCHAR(78) NOT NULL DEFAULT '0';
ALTER TABLE investment_records MODIFY invest_amount VARCHAR(78) NOT NULL DEFAULT '0';
ALTER TABLE lock_records MODIFY lock_amount VARCHAR(78) NOT NULL DEFAULT '0';
ALTER TABLE contract_records
    MODIFY flow_in_amount VARCHAR(78) NOT NULL DEFAULT '0',
    MODIFY flow_out_amount VARCHAR(78) NOT NULL DEFAULT '0';
ALTER TABLE claim_records MODIFY claim_amount VARCHAR(78) NOT NULL DEFAULT '0';

ALTER TABLE ohi_balances
    DROP INDEX idx_ohi_balance,
    MODIFY balance VARCHAR(78) NOT NULL DEFAULT '0';
ALTER TABLE proposal_asset_balances
    DROP INDEX idx_proposal_asset,
    MODIFY balance VARCHAR(78) NOT NULL DEFAULT '0',
    ADD KEY idx_proposal_asset (asset_type);
