# 便携式 MySQL 部署文档

> 版本：v1.0
> 日期：2026-08-21
> 数据库：MySQL 8.0.42（官方绿色版二进制）
> 适用环境：Linux x86_64（已实测 Ubuntu 24.04 / WSL2）

---

## 1. 方案概述

本项目采用 **便携式（绿色版）MySQL 部署**，不依赖 Docker 或系统包管理器：

- 使用官方免安装（Generic Linux）二进制压缩包，解压到项目目录。
- 通过脚本统一管理：下载、初始化、启停、建库建表。
- 整个部署**完全本地化、可迁移**——拷贝整个 `mysql-portable/` 目录即可完成数据迁移。

**目录结构**：

```
hubsql/deploy/mysql-portable/
├── bin/                        # MySQL 二进制（解压自官方 tarball）
├── lib/
├── share/
├── data/                       # 数据目录（mysqld --initialize 生成）
├── run/                        # 运行目录：socket、pid、日志
│   ├── mysqld.sock
│   ├── mysqld.pid
│   ├── error.log
│   └── start.log
├── config/
│   └── my.cnf                  # MySQL 配置（安装时自动生成）
├── sql/
│   └── schema.sql              # 项目建表脚本
└── scripts/
    ├── install.sh              # 首次安装：依赖检查 + 下载解压 + 初始化数据目录
    ├── start.sh                # 启动 mysqld
    ├── stop.sh                 # 优雅停止 mysqld
    ├── status.sh               # 查看运行状态
    └── init_db.sh              # 建库建用户 + 导入 schema
```

---

## 2. 环境要求

| 要求 | 说明 |
| --- | --- |
| 操作系统 | Linux x86_64（glibc ≥ 2.28） |
| 磁盘空间 | ≥ 2 GB（MySQL 8.0 二进制约 850 MB + 数据目录） |
| 内存 | ≥ 1 GB（建议 2 GB+） |
| 网络 | 可访问 MySQL 官方下载源 |
| 命令 | `curl`、`tar`、`sudo`（首次安装依赖时使用） |

---

## 3. 系统依赖

MySQL 8.0 官方二进制运行需要以下系统库：

| 依赖 | Ubuntu 24.04 包名 | 用途 |
| --- | --- | --- |
| libaio | `libaio1t64` | mysqld 运行时依赖 |
| libnuma | `libnuma1` | mysqld 内存分配 |
| libncurses | `libncurses6` | mysql 客户端依赖 |

> ⚠️ 以上依赖已在 `install.sh` 中自动检测并安装（需 sudo）。安装命令也会被自动执行。

---

## 4. 部署步骤

### 4.1 一键安装（下载 + 初始化）

```bash
cd /home/wbl/hubsql/deploy/mysql-portable
./scripts/install.sh
```

脚本自动完成：

1. **检查并安装系统依赖**（libaio / libnuma / libncurses）
2. **创建目录结构**（data / run / config / sql）
3. **下载官方二进制包**：`mysql-8.0.42-linux-glibc2.28-x86_64.tar.xz`
   - 支持断点续传（`curl -C -`），网络中断后重跑脚本即可继续
4. **解压**到 `mysql-portable/`（bin/lib/share）
5. **生成 `config/my.cnf`**（自动写入绝对路径，utf8mb4，innodb_buffer_pool_size=512M）
6. **初始化数据目录**：`mysqld --initialize-insecure`（root 免密，便于脚本建库）

> 说明：默认版本 `MYSQL_VER=8.0.42`，可通过环境变量覆盖：
> `MYSQL_VER=8.0.40 ./scripts/install.sh`

### 4.2 启动 MySQL

```bash
./scripts/start.sh
```

启动后等待就绪（最多 30 秒），成功提示 `✅ MySQL 启动成功 (pid=xxxx)`。

### 4.3 初始化数据库（建库 + 建用户 + 导入表结构）

```bash
./scripts/init_db.sh
```

脚本自动执行：

1. 创建数据库 `hubsql`（utf8mb4）
2. 创建用户 `hubsql`（密码默认 `hubsql123456`，可用环境变量覆盖）
3. 导入 `sql/schema.sql` 表结构（6 张表）

可通过环境变量覆盖默认值：

```bash
DB_NAME=mydb DB_USER=myuser DB_PASS=mypass ./scripts/init_db.sh
```

### 4.4 验证部署

```bash
# 1) 查看运行状态
./scripts/status.sh

# 2) TCP 连接测试（项目将使用该方式连接）
./bin/mysql -uhubsql -phubsql123456 -h127.0.0.1 -P3306 hubsql -e "SELECT VERSION(); SHOW TABLES;"
```

---

## 5. 部署结果

| 项目 | 值 |
| --- | --- |
| MySQL 版本 | 8.0.42 |
| 数据库 | `hubsql`（utf8mb4 / utf8mb4_unicode_ci） |
| 用户 | `hubsql` / `hubsql123456` |
| 连接地址 | `127.0.0.1:3306` |
| 数据表 | blocks、transactions、staking_records、unstaking_records、investment_records、sync_status |

**项目配置文件对应关系**（`config/config.json`）：

```json
"mysql": {
  "host": "127.0.0.1",
  "port": 3306,
  "user": "hubsql",
  "password": "hubsql123456",
  "database": "hubsql",
  "pool_size": 10
}
```

---

## 6. 日常管理命令

| 操作 | 命令 |
| --- | --- |
| 查看状态 | `./scripts/status.sh` |
| 启动 | `./scripts/start.sh` |
| 停止 | `./scripts/stop.sh` |
| 重建库/用户/表 | `./scripts/init_db.sh`（幂等，可重复执行） |
| 查看错误日志 | `tail -f run/error.log` |
| 手动连接 | `./bin/mysql -uhubsql -p -h127.0.0.1 -P3306 hubsql` |

---

## 7. Ubuntu 24.04 兼容性问题与解决

部署过程中遇到并解决了 **3 个 Ubuntu 24.04 特有兼容性问题**，已全部写入 `install.sh` 自动处理。

### 7.1 libaio 缺失（`libaio.so.1`）

**现象**：

```
mysqld: error while loading shared libraries: libaio.so.1: cannot open shared object file
```

**原因**：Ubuntu 24.04 的 `libaio1t64` 包只提供 `libaio.so.1t64`（t64 命名），而 MySQL 二进制查找的是 `libaio.so.1`。

**解决**（创建符号链接）：

```bash
sudo apt-get install -y libaio1t64
sudo ln -sf /usr/lib/x86_64-linux-gnu/libaio.so.1t64 /usr/lib/x86_64-linux-gnu/libaio.so.1
```

### 7.2 libncurses 缺失（`libncurses.so.6`）

**现象**：

```
mysql: error while loading shared libraries: libncurses.so.6: cannot open shared object file
```

**原因**：`mysql` 客户端需要 `libncurses.so.6`，Ubuntu 24.04 默认只安装了宽字符版 `libncursesw6`。

**解决**：

```bash
sudo apt-get install -y libncurses6
```

### 7.3 下载中断

**现象**：`curl: (18) transfer closed with xxx bytes remaining to read`（大文件下载时网络中断）。

**解决**：`install.sh` 已使用 `curl -C -` 支持断点续传，网络中断后**重新运行脚本**即可从断点继续，无需重新下载。

---

## 8. 常见问题排查

| 问题 | 排查方法 |
| --- | --- |
| 启动超时 | 查看 `run/error.log`，多为端口占用或数据目录权限问题 |
| 端口 3306 被占用 | `sudo lsof -i:3306` 或修改 `my.cnf` 中的 `port` |
| 密码遗忘 | root 使用 socket 免密登录：`./bin/mysql -uroot --socket=run/mysqld.sock`，再 `ALTER USER` 重置 |
| 数据迁移 | 停止 MySQL 后整体拷贝 `mysql-portable/` 目录，新机器运行 `install.sh`（会自动跳过已存在部分）+ `start.sh` |
| my.cnf 路径错误 | 目录搬迁后重新运行 `install.sh` 重新生成 my.cnf（自动写入新绝对路径） |

---

## 9. 注意事项

1. **my.cnf 使用绝对路径**，由 `install.sh` 自动生成；搬迁目录后请重新生成。
2. **数据目录安全**：`data/` 是核心数据，建议定期备份或纳入版本控制排除。
3. **密码修改**：默认密码 `hubsql123456` 仅用于开发环境，生产环境请通过 `DB_PASS` 环境变量或 `ALTER USER` 修改。
4. **TCP vs Socket**：项目（C++ 服务）通过 TCP `127.0.0.1:3306` 连接；脚本管理使用本地 Socket 连接。
