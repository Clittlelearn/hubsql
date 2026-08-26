# hubsql REST API 对接文档

> 面向前端对接的接口说明与数据格式示例。
> 服务地址：`http://<host>:8080`（默认 8080 端口，见 `config/config.json` 的 `api` 配置）

---

## 1. 通用约定

### 1.1 响应结构（统一封装）

所有接口统一返回：

```json
{
  "code": 0,          // 0=成功；非 0 为错误码（如 400/404/500）
  "message": "ok",    // 错误时为人性化描述
  "data": { ... }     // 业务数据；错误时为 null
}
```

- 成功：`code=0`
- 参数/业务错误：`code=400`（如 `invalid table`）
- 资源不存在：`code=404`（如 `block not found`、`unknown business module: xxx`）
- 服务端异常：`code=500`

### 1.2 分页参数

列表接口统一支持以下查询参数：

| 参数 | 类型 | 默认 | 说明 |
| --- | --- | --- | --- |
| `page` | int | 1 | 页码（从 1 开始） |
| `size` | int | 20 | 每页条数 |

分页响应统一为：

```json
{ "list": [ ... ], "total": 8, "page": 1, "size": 20 }
```

### 1.3 数据格式说明

- **时间戳**：均为**微秒**（微秒级 Unix 时间戳，`1787278243920870` ≈ 2026-08）。前端展示时除以 `1_000_000` 转毫秒/秒。
- **金额**：均以**十进制字符串**返回，且带 **18 位小数**（`"10000000000000.000000000000000000"`）。前端需按字符串处理避免精度丢失；如展示为整数，可去掉小数点及小数部分（注意部分金额本身含小数）。
- **hash/地址**：均为 `0x` 开头的小写十六进制字符串（提案的资产名特殊值为 `"OHI"`）。
- **`is_xxx` 布尔字段**：true/false。

### 1.4 通用业务路由

除下面的语义化别名外，还提供通用路由（按业务模块名查找）：

```
GET /api/v1/business/{name}
```

`{name}` 取值：`staking` / `investment` / `proposal` / `vote` / `lock` / `txrecord` / `contract` / `claim`。

示例：`GET /api/v1/business/staking?page=1&size=20`

---

## 2. 接口总览

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/health` | 健康检查 |
| GET | `/api/v1/staking` | 质押记录（含解质押标记） |
| GET | `/api/v1/unstaking` | 已解质押视图（`is_unstaked=1`） |
| GET | `/api/v1/investments` | 投资记录（含解投资标记） |
| GET | `/api/v1/deinvestments` | 已解投资视图（`is_deinvested=1`） |
| GET | `/api/v1/proposals` | 提案记录 |
| GET | `/api/v1/revokedproposals` | 已撤销提案视图（`is_revoked=1`） |
| GET | `/api/v1/votes` | 投票记录 |
| GET | `/api/v1/locks` | 锁定记录（含解锁定标记） |
| GET | `/api/v1/unlockedlocks` | 已解锁定视图（`is_unlocked=1`） |
| GET | `/api/v1/txrecords` | 交易记录（仅保留最新 20 个高度） |
| GET | `/api/v1/contracts` | 合约交易（部署/调用，含跃入跃出） |
| GET | `/api/v1/claims` | 申领记录 |
| GET | `/api/v1/balances` | 账户余额列表（按余额降序） |
| GET | `/api/v1/balances/{address}` | 单地址余额 |
| GET | `/api/v1/stats/overview` | 全局统计 |
| GET | `/api/v1/business/{name}` | 通用业务路由 |

---

## 3. 基础接口

### 3.1 健康检查

```
GET /health
```

```json
{ "code": 0, "data": { "status": "healthy" }, "message": "ok" }
```

---

## 4. 质押 / 解质押

### 4.1 质押列表

```
GET /api/v1/staking?address=&is_unstaked=&page=&size=
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `address` | string | 按质押人地址过滤，可空 |
| `is_unstaked` | int | `-1` 全部（默认）、`0` 未解质押、`1` 已解质押 |

```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "tx_hash": "0xdb8739b509f6d1ff1eb609887dc72f3c8e004a94d320f7bd3d3bb6bf1fe7737c",
        "block_height": 30,
        "address": "0x458e53542299BeC018A6118c7544C44d3b8911Df",
        "stake_amount": "10000000000000.000000000000000000",
        "stake_time": 1787383628237711,
        "commission_rate": "0.080000",
        "stake_type": "Net",
        "is_unstaked": false,
        "unstake_tx_hash": "0xa5a099a90e62957bebaeab6882aecf2a345d3c4fbf702ca19ca2abac695b07b7",
        "unstake_time": 1787278665779694
      }
    ],
    "total": 8, "page": 1, "size": 20
  },
  "message": "ok"
}
```

- `unstake_tx_hash` / `unstake_time` 仅在 `is_unstaked=true` 时返回。
- `stake_time` 为微秒。

### 4.2 已解质押视图

```
GET /api/v1/unstaking?address=&page=&size=
```

等价于 `/api/v1/staking?is_unstaked=1`，返回格式同 4.1（`is_unstaked` 恒为 `true`）。

---

## 5. 投资 / 解投资

### 5.1 投资列表

```
GET /api/v1/investments?address=&is_deinvested=&page=&size=
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `address` | string | 按投资地址过滤，可空 |
| `is_deinvested` | int | `-1` 全部（默认）、`0` 未解投资、`1` 已解投资 |

```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "tx_hash": "0x3657796961df228ef8d8224fd6101e4f0ca3799adaa3ee8c6d01268403403b5d",
        "block_height": 32,
        "address": "0x2E08bA82dcF52966dF03Aa0fd830aFAb70d8911d",
        "invest_amount": "50000000000000.000000000000000000",
        "invest_time": 1787383719545956,
        "bonus_addr": "0x2E08bA82dcF52966dF03Aa0fd830aFAb70d8911d",
        "invest_type": "Normal",
        "is_deinvested": false,
        "deinvest_tx_hash": "0x3bc6dbcbb199283a1b683d92f3d0bfe2f9ddc9d4b5674d7c88ab5ac590441f2a",
        "deinvest_time": 1787279297719862
      }
    ],
    "total": 9, "page": 1, "size": 20
  },
  "message": "ok"
}
```

- `bonus_addr`：被投资节点地址（bonusAddr）。
- `deinvest_tx_hash` / `deinvest_time` 仅在 `is_deinvested=true` 时返回。

### 5.2 已解投资视图

```
GET /api/v1/deinvestments?address=&page=&size=
```

等价于 `/api/v1/investments?is_deinvested=1`。

---

## 6. 提案 / 投票 / 撤销

### 6.1 提案列表

```
GET /api/v1/proposals?address=&is_revoked=&page=&size=
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `address` | string | 按提案人过滤，可空 |
| `is_revoked` | int | `-1` 全部（默认）、`0` 未撤销、`1` 已撤销 |

```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "tx_hash": "0x0dd3a3dc63e81a6bd4884bed2355408377aa8a7aff018c86b9a7c80dc6100896",
        "asset": "0x0dd3a3dc63e81a6bd4884bed2355408377aa8a7aff018c86b9a7c80dc6100896",
        "block_height": 29,
        "address": "0x755Ccf704E17570b64E247f0794314e4C8E542CA",
        "vote_count": 0,
        "is_first": false,
        "is_revoked": false,
        "revoke_tx_hash": "0x68061497bca27fecf96b3fd66b9d896d578ae49b8b022f3ad528d17406c29dec",
        "revoke_time": 1787280245337672,
        "tx_info": {
          "name": "dGVzdDAx",
          "title": "MQ==",
          "minVoteNum": 1,
          "genesisToken": 0,
          "beginTime": 1787364092155442,
          "endTime": 1787364452155442,
          "tokenContractAddr": "0x77c835D837666B7A23131A46bda35E6325E36e1B",
          "tokenDecimals": 8,
          "exchangeRate": "1",
          "version": 1
        }
      }
    ],
    "total": 2, "page": 1, "size": 20
  },
  "message": "ok"
}
```

- `asset`：提案资产名。**第一笔提案为 `"OHI"`，之后为提案自身 hash**（= `tx_hash`）。
- `is_first`：是否第一笔提案（OHI 提案）。
- `tx_info`：提案交易的完整 txInfo（JSON 对象，含 name/title/minVoteNum 等）。
- `revoke_tx_hash` / `revoke_time` 仅在 `is_revoked=true` 时返回。

### 6.2 已撤销提案视图

```
GET /api/v1/revokedproposals?address=&page=&size=
```

等价于 `/api/v1/proposals?is_revoked=1`。

### 6.3 投票列表

```
GET /api/v1/votes?address=&proposal_hash=&page=&size=
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `address` | string | 按投票人过滤，可空 |
| `proposal_hash` | string | 按被投票提案过滤（第一笔为 `OHI`），可空 |

```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "tx_hash": "0x1376c4b3a0b1b8826a5c2eb6cc7d5bfdb4e9cfb69de9308b1dc8f3639a03cd10",
        "block_height": 9,
        "address": "0x755Ccf704E17570b64E247f0794314e4C8E542CA",
        "proposal_hash": "0xOHI",
        "proposal_type": 11,
        "vote_type": 1,
        "vote_number": 1
      }
    ],
    "total": 1, "page": 1, "size": 20
  },
  "message": "ok"
}
```

- `proposal_hash`：被投票的提案（第一笔提案为 `"OHI"`，其余为提案 hash）。
- `proposal_type`：被投票交易类型（提案为 `11`）。
- `vote_type`：投票类型，`0`=反对、`1`=赞成。
- `vote_number`：票数。

---

## 7. 锁定 / 解锁定

### 7.1 锁定列表

```
GET /api/v1/locks?address=&is_unlocked=&page=&size=
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `address` | string | 按锁定地址过滤，可空 |
| `is_unlocked` | int | `-1` 全部（默认）、`0` 未解锁定、`1` 已解锁定 |

```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "tx_hash": "0x917f69f93311d5bba961058c811451d91f3f5ba6969faeda0635eab4b476958b",
        "block_height": 23,
        "address": "0x2E08bA82dcF52966dF03Aa0fd830aFAb70d8911d",
        "asset_type": "OHI",
        "lock_amount": "10000000000.000000000000000000",
        "lock_time": 1787279543777324,
        "lock_type": "LockNet",
        "is_unlocked": true,
        "unlock_tx_hash": "0xb8ba3af5c95fae0cf9b4e82d2d49cc52daab30cbf44c982e8cb0a102f5bb2aff",
        "unlock_time": 1787279643983441
      }
    ],
    "total": 2, "page": 1, "size": 20
  },
  "message": "ok"
}
```

- `asset_type`：锁定资产类型（`OHI` 或 hash）。
- `unlock_tx_hash` / `unlock_time` 仅在 `is_unlocked=true` 时返回。

### 7.2 已解锁定视图

```
GET /api/v1/unlockedlocks?address=&page=&size=
```

等价于 `/api/v1/locks?is_unlocked=1`。

---

## 8. 交易记录（滚动）

```
GET /api/v1/txrecords?type=&page=&size=
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `type` | string | 按交易类型过滤（如 `2`=质押、`9`=锁定、`99`=申领），可空 |

> 说明：本表**只保留最新 20 个高度**的交易（滚动清理），`total` 为当前窗口内条数。

```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "tx_hash": "0x1b9fc5f49755a2c59f4bafb082097bf4b806d1d35b5ecbdeccca84d6fa4eccdc",
        "block_height": 35,
        "tx_type": "99",
        "utxo": [
          {
            "assetType": "OHI",
            "owner": ["0x03bEE3dB52E736B6422DaC4668CC784d05cb2e63"],
            "vin": {
              "prevout": [
                { "hash": "0xb0e151035b239321483f44c00205dc25de9347fc008237bd87fe27426bdf81b0", "n": 0 }
              ]
            },
            "vout": [
              { "addr": "0x03bEE3dB52E736B6422DaC4668CC784d05cb2e63", "value": "60583140788" },
              { "addr": "0x03bEE3dB52E736B6422DaC4668CC784d05cb2e63", "value": "40004559934150" },
              { "addr": "VirtualBurnGas", "value": "26000" }
            ]
          }
        ]
      }
    ],
    "total": 28, "page": 1, "size": 20
  },
  "message": "ok"
}
```

- `utxo`：交易的全部 UTXO 数组（JSON 对象），含 `assetType` / `owner` / `vin.prevout`（hash、n）/ `vout`（addr、value）。

---

## 9. 合约交易（含跃入/跃出）

```
GET /api/v1/contracts?address=&tx_type=&is_flow_in=&is_flow_out=&page=&size=
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `address` | string | 按账户地址过滤，可空 |
| `tx_type` | string | `deploy`（部署）/ `call`（调用），可空 |
| `is_flow_in` | int | `-1` 全部（默认）、`1` 仅跃入、`0` 排除跃入 |
| `is_flow_out` | int | `-1` 全部（默认）、`1` 仅跃出、`0` 排除跃出 |

**跃入/跃出示例**（`is_flow_in=1`）：

```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "tx_hash": "0x6b73d1138f2e6baca7cfda40e3623f44071119b827ddd5d60d126ff2dce3b0ad",
        "block_height": 11,
        "address": "0x755Ccf704E17570b64E247f0794314e4C8E542CA",
        "sender": "0x755Ccf704E17570b64E247f0794314e4C8E542CA",
        "recipient": "0x45d786f7159368ceE858402794CF6B4c9daF38E4",
        "tx_type": "call",
        "is_flow_in": true,
        "is_flow_out": false,
        "flow_in_amount": "5000000000000000.000000000000000000",
        "flow_out_amount": "0.000000000000000000",
        "asset_type": "OHI",
        "tx_time": 1787277971816462,
        "tx_info": {
          "baseFee": "1",
          "callType": "FlowInTx",
          "contractDeployer": "0x755Ccf704E17570b64E247f0794314e4C8E542CA",
          "recipient": "0x45d786f7159368ceE858402794CF6B4c9daF38E4",
          "sender": "0x755Ccf704E17570b64E247f0794314e4C8E542CA",
          "transfer": 0,
          "version": 0,
          "virtualMachine": 0
        }
      }
    ],
    "total": 1, "page": 1, "size": 20
  },
  "message": "ok"
}
```

**字段说明**：
- `tx_type`：`deploy`（合约部署，type=7）/ `call`（合约调用，type=8）。
- `is_flow_in` / `is_flow_out`：是否为**跃入**（`callType=FlowInTx`）/ **跃出**（`callType=FlowOutTx`）—— 跃入/跃出是特殊的合约调用。
- `flow_in_amount` / `flow_out_amount`：跃入/跃出金额（非跃入跃出时为 `0`）。
- `asset_type`：跃入/跃出资产类型（`OHI` 或 hash）。
- `recipient`：合约地址；`sender`：发送者；`tx_info`：完整 txInfo（JSON 对象）。

---

## 10. 申领

```
GET /api/v1/claims?address=&asset_type=&page=&size=
```

| 参数 | 类型 | 说明 |
| --- | --- | --- |
| `address` | string | 按申领地址过滤，可空 |
| `asset_type` | string | 按资产类型过滤（`OHI` 或 hash），可空 |

```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "tx_hash": "0x1b9fc5f49755a2c59f4bafb082097bf4b806d1d35b5ecbdeccca84d6fa4eccdc",
        "block_height": 35,
        "address": "0x03bEE3dB52E736B6422DaC4668CC784d05cb2e63",
        "asset_type": "OHI",
        "claim_amount": "65143162138.000000000000000000",
        "claim_time": 1787384124650457
      }
    ],
    "total": 1, "page": 1, "size": 20
  },
  "message": "ok"
}
```

---

## 11. 账户余额

### 11.1 余额列表（按余额降序）

```
GET /api/v1/balances?page=&size=
```

```json
{
  "code": 0,
  "data": {
    "list": [
      { "address": "0x755Ccf704E17570b64E247f0794314e4C8E542CA", "balance": 4149999900000000 },
      { "address": "0x2E08bA82dcF52966dF03Aa0fd830aFAb70d8911d", "balance": 139999999814600 },
      { "address": "0x03bEE3dB52E736B6422DaC4668CC784d05cb2e63", "balance": 40065143074938 }
    ],
    "total": 8, "page": 1, "size": 3
  },
  "message": "ok"
}
```

- `balance`：整数（BIGINT），**单位与链上一致**（OHI 最小单位，非 18 位小数；直接作为整数展示/计算即可，与节点 `GetBalance` 逐位一致）。

### 11.2 单地址余额

```
GET /api/v1/balances/{address}
```

```json
{
  "code": 0,
  "data": {
    "address": "0x755Ccf704E17570b64E247f0794314e4C8E542CA",
    "balance": 4149999900000000
  },
  "message": "ok"
}
```

---

## 12. 全局统计

```
GET /api/v1/stats/overview
```

```json
{
  "code": 0,
  "data": {
    "business_counts": {
      "staking_records": 8,
      "unstaked_count": 1,
      "investment_records": 9,
      "deinvested_count": 1,
      "proposals": 2,
      "revoked_proposals": 1,
      "votes": 1,
      "lock_records": 2,
      "unlocked_count": 2,
      "tx_records": 28,
      "contract_records": 13,
      "claim_records": 1
    },
    "total_blocks": 0,
    "total_txs": 0
  },
  "message": "ok"
}
```

- `business_counts`：各业务模块的数量统计（随已注册模块自动增减）。
- `total_blocks` / `total_txs`：预留（区块/交易明细主表尚未接入同步流水线，当前恒为 0）。

---

## 13. 错误示例

**未知业务模块：**

```
GET /api/v1/business/unknown
```

```json
{
  "code": 404,
  "data": null,
  "message": "unknown business module: unknown"
}
```

**区块不存在（预留接口）：**

```json
{ "code": 404, "data": null, "message": "block not found" }
```

---

## 14. 附：当前链数据速览（高度 35）

| 业务 | 数量 | 说明 |
| --- | --- | --- |
| 账户余额 | 8 | 与节点 `GetBalance` 逐位一致 |
| 质押 | 8（已解质押 1） | 佣金率 0.06~0.08 |
| 投资 | 9（已解投资 1） | bonusAddr=自身 |
| 提案 | 2（已撤销 1） | 第一笔资产=OHI |
| 投票 | 1 | 赞成 OHI 提案 |
| 锁定 | 2（均已解锁定） | LockNet |
| 交易记录 | 28 | 仅最新 20 高度（16~35） |
| 合约 | 13 | 7 部署 + 6 调用（含 1 跃入、1 跃出） |
| 申领 | 1 | h35 |
