# hubsql — 区块链数据采集与解析服务

通过 HTTP/HTTPS 从自定义/私有链节点拉取数据块，解析区块内的多种交易（质押、解质押、投资…），落库到 **MySQL**，并通过 **REST API** 提供给前端展示。

- 语言：C++20 / CMake ≥ 3.20
- 数据库：MySQL 8.0+（便携式脚本部署，见 `deploy/mysql-portable/`）
- HTTP 客户端：libcurl ｜ HTTP 服务：crow ｜ JSON：nlohmann/json ｜ 日志：spdlog ｜ 测试：GoogleTest

## 目录结构

```
hubsql/
├── CMakeLists.txt
├── config/config.json          # 服务配置
├── docs/                       # 架构设计方案 / MySQL 部署文档
├── sql/                        # 建表脚本与迁移
├── deploy/mysql-portable/      # 便携式 MySQL（脚本部署）
├── src/
│   ├── main.cpp                # 入口：组装采集 + API
│   ├── common/                 # 日志、配置、线程池、JSON 工具
│   ├── model/                  # 区块/交易/业务数据模型
│   ├── fetcher/                # HTTP 客户端、区块拉取、同步调度
│   ├── parser/                 # 解析器基类、注册表、区块解析、业务解析器
│   ├── storage/                # MySQL 连接池 + Repository
│   └── api/                    # REST API 服务与控制器
├── tests/                      # 单元测试
└── scripts/setup_build_deps.sh # 安装构建依赖
```

## 快速开始

### 1. 安装构建依赖（首次，需 sudo）

```bash
./scripts/setup_build_deps.sh
```

> 安装：build-essential、cmake、libcurl4-openssl-dev、libmysqlcppconn-dev。
> nlohmann/json、spdlog、asio、crow、googletest 由 CMake 自动拉取。

### 2. 部署并启动 MySQL（可选，若已部署可跳过）

```bash
cd deploy/mysql-portable
./scripts/install.sh && ./scripts/start.sh && ./scripts/init_db.sh
```

### 3. 配置

编辑 `config/config.json`，设置链节点地址与 MySQL 连接信息。

### 4. 构建

```bash
cmake -S . -B build
cmake --build build -j
```

### 5. 运行

```bash
./build/src/hubsql config/config.json
```

### 6. 运行单元测试

```bash
ctest --test-dir build --output-on-failure
```

## 链节点接口约定（自定义链）

采集服务假定链节点提供以下 HTTP 接口（可按实际链调整 `fetcher/block_fetcher.cpp`）：

| 接口 | 说明 |
| --- | --- |
| `GET {base_url}/api/v1/chain/height` | 返回 `{"height": 1000}` 最新高度 |
| `GET {base_url}/api/v1/block/{height}` | 返回区块 JSON |

区块 JSON 结构：

```json
{
  "height": 100,
  "hash": "0x...",
  "timestamp": 1755750000,
  "txs": [
    { "hash": "0x...", "type": "staking", "from": "addr1", "to": "",
      "amount": "100.5", "validator": "v1", "lock_days": 90 },
    { "hash": "0x...", "type": "invest", "from": "addr2", "to": "",
      "amount": "50", "product_id": "P-100" }
  ]
}
```

`type` 字段决定交易如何被解析；当前内置业务模块：`staking`、`investment`。

## 新增业务类型（注册制扩展点）

每个业务 = 一个模块（记录结构 + 解析器 + 存储），封装为 `IBusinessModule` 后
在 `src/main.cpp` 中 `registry.Register(...)` 注册一次，主流水线与 REST API 自动接入。

步骤（参考 `staking_module.*` / `staking_repo.*` / `stake_record.h`）：

1. 定义记录结构：`src/model/business/xxx_record.h`
2. 定义解析逻辑：`src/parser/parsers/xxx_parser.*`
3. 定义存储：`src/storage/xxx_repo.*`
4. 封装模块 `src/parser/parsers/xxx_module.*`（实现 `Name/Handles/Process/List/Counts`），
   在 `sql/schema.sql` 新增业务表，并在 `src/main.cpp` 注册。

REST API 自动获得通用路由 `/api/v1/business/{name}`，并可加别名路由。

## REST API

详细接口说明与数据格式示例见 **[docs/API文档.md](docs/API文档.md)**（含各业务字段、分页、时间戳/金额格式约定）。

服务默认端口 `8080`：
| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/health` | 健康检查 |
| GET | `/api/v1/blocks?start=&end=&page=&size=` | 区块列表 |
| GET | `/api/v1/blocks/{height}` | 区块详情 |
| GET | `/api/v1/txs?type=&address=&page=&size=` | 交易列表 |
| GET | `/api/v1/txs/{tx_hash}` | 交易详情 |
| GET | `/api/v1/staking?address=&is_unstaked=&page=&size=` | 质押记录（金额/时间/佣金率/类型/解质押标记） |
| GET | `/api/v1/unstaking?address=&page=&size=` | 已解质押记录（`is_unstaked=1` 视图） |
| GET | `/api/v1/investments?address=&is_deinvested=&page=&size=` | 投资记录（金额/时间/bonusAddr/类型/解投资标记） |
| GET | `/api/v1/deinvestments?address=&page=&size=` | 已解投资记录（`is_deinvested=1` 视图） |
| GET | `/api/v1/proposals?address=&is_revoked=&page=&size=` | 提案记录（txInfo/投票数/是否第一笔/撤销标记） |
| GET | `/api/v1/revokedproposals?address=&page=&size=` | 已撤销提案视图（`is_revoked=1`） |
| GET | `/api/v1/votes?address=&proposal_hash=&page=&size=` | 投票记录（被投票提案/类型/投票类型/票数） |
| GET | `/api/v1/locks?address=&is_unlocked=&page=&size=` | 锁定记录（资产类型/金额/类型/解锁定标记） |
| GET | `/api/v1/unlockedlocks?address=&page=&size=` | 已解锁定视图（`is_unlocked=1`） |
| GET | `/api/v1/txrecords?type=&page=&size=` | 交易记录（仅保留最新 20 个高度） |
| GET | `/api/v1/contracts?tx_type=&is_flow_in=&is_flow_out=&page=&size=` | 合约交易（部署/调用，含跃入跃出） |
| GET | `/api/v1/claims?address=&asset_type=&page=&size=` | 申领记录（BONUS，金额/资产类型） |
| GET | `/api/v1/stats/overview` | 全局统计 |

统一响应结构：`{"code": 0, "message": "ok", "data": {...}}`
