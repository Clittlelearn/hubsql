# Asset Catalog Count Audit

## Question

The Manage Tokens directory contains 990 entries while the test network is only
around height 850. Determine whether HubSQL fabricated or duplicated tokens.

## Scope and Method

Read-only audit on 2026-09-22 against the local HubSQL database and node
`http://192.168.1.110:13134`. No transactions, database updates, data deletion,
service restarts or deployments were performed.

- Read `erc20_contracts` and `erc20_transfer_events` from MySQL.
- Read every height from 0 through 858 using `GetBlockByHeight`, in ranges of 20.
  Deduplicate block and transaction hashes rather than assuming one block per
  height. This is a read-only node API, not a transaction submission interface.
- Compare every recorded deployment transaction with the node transaction type
  and `txInfo.recipient`.
- Check that each catalog contract has at least one indexed transfer source
  transaction present in the node response.
- Query `eth_getCode` for every ERC20 address at `latest`. Group runtime bytecode
  by SHA-256. These groups are not contract identities.
- Read and ABI-decode `name()`, `symbol()`, `decimals()` and `totalSupply()` for
  every ERC20 address using ETH RPC and the installed ethers ABI decoder.
- Compare genesis and height-858 block hashes with HubSQL's configured RPC source
  (`http://222.128.23.254:13134`). Both checkpoints match node 110.

The generated per-contract evidence is in
`build/audits/asset-catalog-2026-09-22.json`. It contains addresses, source hashes,
deployment heights, bytecode sizes and fingerprints, but no credentials.

## Results

| Measurement | Count |
| --- | ---: |
| Node height at start and end of the block/code audit | 858 |
| Distinct heights, including genesis | 859 |
| Distinct block hashes returned by the node | 1,297 |
| Heights with multiple blocks | 329 |
| Maximum blocks at one height | 4 |
| Distinct transactions | 1,314 |
| Deployment transactions, type 7 | 989 |
| Contract calls, type 8 | 224 |
| Catalog ERC20 addresses | 979 |
| Catalog proposal entries, including OHI | 11 |
| Total catalog entries | 990 |
| ERC20 entries with a recorded deployment hash | 977 |
| Missing recorded deployment transactions on the node | 0 |
| Deployment type or recipient mismatches | 0 |
| ERC20 entries without any indexed source transaction on the node | 0 |
| ERC20 addresses with nonempty runtime code | 979 |
| Empty code responses or code RPC failures | 0 |

The other two ERC20 entries were discovered from contract calls/Transfer events;
their catalog `deploy_tx_hash` is empty. They are
`0x5fd0bd6c3e6d3d2d1b724f03f739703c13e43398` and
`0x5af1bf521e0576502897a9a7e8ee11fcd5157082` (HBR). An empty discovery-time
deployment hash does not mean that a contract was never deployed.

Runtime code groups contain 960, 15, 2, 1 and 1 contracts respectively. Distinct
addresses can share identical code and names; they remain separate ERC20 assets.

All 979 metadata and total-supply checks completed without errors. In particular,
962 addresses return name `ohi`, symbol `o`, and decimals `8`. Two return
`xiaomm`/`XXMM` with decimals `0`; the remaining 15 each have their own
name/symbol/decimals combination, including HBR, PPST, W03, W04 and W05.
This confirms a large set of same-named contracts, not duplicated rows for one
address. It does not establish who intended or authorized the old deployments.

## Concrete Example

Height 509 returns four different block hashes, each containing one deployment.
All four deployed addresses currently return nonempty runtime code and `name()`
returns `ohi`:

- `0x704cb28c62f8d8ff3f2d5168784c103e4af9e9e7`
- `0x49a6ac65113f09ae4dfa463014437b1cc16fe556`
- `0xcebb84797c0cd17a079b08f40aa979fae6aac800`
- `0xefab74dab0ff43222dbab519d4984fa553142930`

These are ERC20 contract names, not proof that they represent native OHI. Native
OHI is identified by its native asset type; ERC20 identity is the contract address.

## Conclusion and Limits

The count alone is not an indexer error. The 990-entry API response combines two
different kinds of records: 979 ERC20 contracts and 11 proposal/native asset
entries. It must not be described as 990 distinct ERC20 deployments. Proposal
entries may point to an ERC20 already present in the same directory.

The audited deployment hashes and addresses match the current node data; all
979 contracts have runtime code. There is no evidence here supporting deletion
of those records as phantom tokens. A lower block height is not an upper bound
on deployment count, particularly with multiple blocks per height in this node.

This audit does not prove historical ERC20 balance conservation, validate every
contract's semantics, or change the node's consensus/fork rules. Those are
separate questions. No parser or balance logic was changed on the basis of a
count comparison alone.
