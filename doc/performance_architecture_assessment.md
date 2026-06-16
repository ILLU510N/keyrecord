# KeyRecord 架构性能评估报告

评估日期：2026-06-16

## 判断

当前项目存在两个较大的性能结构风险，但还没有到必须替换 SQLite 的程度。

1. 写入链路风险最大：`LowLevelKeyboardProc` 回调内直接执行 SQLite 单条同步写入，且每次按键都重新拼 SQL、执行 `sqlite3_exec`。随着输入频率升高或磁盘抖动，这条链路可能阻塞键盘 Hook 回调，带来输入延迟、Hook 超时甚至按键记录丢失。
2. 查询链路的主要瓶颈不是“JS 慢”，而是 Node 端 API 直接对原始明细表做全表聚合，当前表没有索引，也没有汇总表。数据达到百万级以后，`GROUP BY`、`COUNT(DISTINCT)`、全量排行查询会随行数线性变慢，并阻塞 `better-sqlite3` 所在的 Node 事件循环。
3. 单机、单用户、本地可视化场景下，SQLite 可以继续支撑增长。前提是补齐 WAL、必要索引、写入队列/批量提交、统计汇总表和数据归档策略。短期不建议迁移到 PostgreSQL/MySQL；迁移数据库不能解决当前 Hook 同步写入和全表聚合这两个根因。

## 当前架构与数据流

```text
键盘事件
  -> Windows WH_KEYBOARD_LL Hook
  -> main.cpp::KeyboardProc
  -> main.cpp::recordKey
  -> keyrecord.db / keys 明细表
  -> visualize/server.js 只读打开 SQLite
  -> /api/info / /api/daily-stats / /api/heatmap / /api/keys
  -> 前端日历图和键盘热力图
```

相关代码位置：

- `main.cpp:103-128`：初始化 `keys` 表，每次按键使用 `sprintf_s` 拼接 `INSERT`，再调用 `sqlite3_exec`。
- `main.cpp:131-138`：键盘 Hook 回调内同步调用 `recordKey`。
- `visualize/server.js:24-88`：核心 API 全部直接查询 `keys` 原始表，包含 `GROUP BY`、`ORDER BY count DESC`、`COUNT(DISTINCT vk_code)`。

## 当前数据库状态

本次检查的实际库状态：

| 项目 | 当前值 |
| --- | --- |
| 表 | `keys(id, timestamp, date, hour, vk_code, key_name)` |
| 行数 | 1379 |
| 日期范围 | 2026-06-12 ~ 2026-06-16 |
| 有数据的日期数 | 2 |
| 不同按键数 | 49 |
| 索引 | 无业务索引 |
| `journal_mode` | `delete` |
| `synchronous` | `2` |
| 页大小 / 页数 | 4096 / 14 |

当前数据量很小，因此界面不会明显慢。但执行计划已经暴露增长后的退化路径。

## 查询执行计划

当前没有索引，关键查询都是全表扫描：

```text
/api/daily-stats
SCAN keys
USE TEMP B-TREE FOR GROUP BY

/api/heatmap?start=...&end=...
SCAN keys
USE TEMP B-TREE FOR GROUP BY
USE TEMP B-TREE FOR ORDER BY

/api/keys
SCAN keys
USE TEMP B-TREE FOR GROUP BY
USE TEMP B-TREE FOR ORDER BY
```

这意味着数据越多，查询越慢。SQLite 官方查询规划文档也明确说明，无索引查询会读取并检查整张表，表达到百万级时应避免全表扫描。

## 本地基准验证

使用当前依赖 `better-sqlite3`，在内存库中模拟当前表结构和 API 查询。该测试不代表真实磁盘耗时，但能反映查询形态和索引收益。

300,000 行：

| 查询 | 无索引 | 加索引后 |
| --- | ---: | ---: |
| `/api/daily-stats` | 137 ms | 22 ms |
| `/api/heatmap` 日期范围 | 54 ms | 29 ms |
| `/api/keys` 全量排行 | 164 ms | 25 ms |
| `/api/info` 聚合信息 | 51 ms | 52 ms |

1,000,000 行：

| 查询 | 无索引 | 加索引后 |
| --- | ---: | ---: |
| `/api/daily-stats` | 533 ms | 78 ms |
| `/api/heatmap` 日期范围 | 150 ms | 60 ms |
| `/api/keys` 全量排行 | 960 ms | 126 ms |
| `/api/info` 聚合信息 | 176 ms | 141 ms |

解读：

- 索引能明显降低日历、日期范围热力图、按键排行查询耗时。
- `/api/info` 里 `COUNT(DISTINCT vk_code)` 仍偏向全量聚合，索引帮助有限。数据继续增长后，应改为读汇总表或元信息表。
- `better-sqlite3` 是同步 API，单次慢查询会占住 Node 线程。单用户本地页面可以接受几十毫秒查询；达到数百毫秒到秒级后，刷新页面和并发 API 会互相阻塞。

## 单 SQLite 能否支撑增长

可以支撑当前产品形态，但需要补齐数据库工程化措施。

SQLite 适合本地桌面应用作为单文件数据存储。官方文档也把桌面程序、记录类工具和低到中等流量网站列为合适场景。当前项目是单机写入、单机读取、低并发可视化，符合 SQLite 的优势场景。

真正限制不是单文件数据库本身，而是：

- SQLite 同一时刻只有一个 writer。当前只有一个采集进程写入，这不是问题；若未来多进程、多设备或服务端集中写入，就会变成架构限制。
- 当前是 `DELETE` journal，读服务和写进程同时访问时更容易互相影响。WAL 模式允许读写更好地并发，SQLite 文档明确说明 WAL 下 reader 和 writer 可以同时运行，但仍只有一个 writer。
- 明细表持续增长后，全量聚合查询会越来越慢。即使 SQLite 能存下几千万行，也不代表每次页面加载都应该扫几千万行。

粗略数据量估算：

| 日按键量 | 年行数 | 无索引原始数据粗估 | 加 2~3 个业务索引后粗估 |
| ---: | ---: | ---: | ---: |
| 50,000 | 18,250,000 | 约 0.7~1.5 GB/年 | 约 2~5 GB/年 |
| 200,000 | 73,000,000 | 约 3~6 GB/年 | 约 8~20 GB/年 |

这些规模 SQLite 仍可处理，但必须用索引、汇总表、归档/清理策略控制查询成本和文件增长。

## JS 查询数据库是否有速度瓶颈

JS 层本身不是首要瓶颈，当前瓶颈来自同步查询加低效 SQL 形态。

`better-sqlite3` 主仓说明它提供同步 API，也建议为了性能设置 WAL；同时指出性能问题最常见原因是低效查询、索引不当或缺少 WAL，而不是驱动本身。当前项目正好命中“索引不当/缺少 WAL/全量聚合”。

具体风险：

1. `visualize/server.js` 每个请求都会 `db.prepare(...)`，SQL 本身不复杂，但可以缓存 prepared statements，减少重复编译成本。
2. 查询执行期间 Node 事件循环被同步占用。单用户本地访问问题不大，多 tab 或查询变慢后会表现为接口串行等待。
3. 前端启动时会加载 `/api/info`、全日期范围 `/api/heatmap`、`/api/daily-stats`。返回数据量不大，因为最终按键种类有限；真正消耗在服务端扫描和聚合原始明细。
4. 日期范围缺少上限和格式校验，未来若暴露给外部访问，容易触发超大范围查询。

## 主要性能缺陷

### P0：键盘 Hook 中同步写 SQLite

`KeyboardProc` 在 Hook 回调内调用 `recordKey`，`recordKey` 又同步执行 SQLite 写入。Windows 低级键盘 Hook 文档要求 Hook 处理应小于 `LowLevelHooksTimeout`，Windows 7 之后超时可能静默移除 Hook；文档也建议低级 Hook 把工作交给 worker thread 后尽快返回。

当前实现把磁盘 I/O、SQL 编译和事务提交放在 Hook 路径上，是最大的性能和可靠性风险。

建议：

- Hook 回调只采集 `vkCode` 和时间，写入内存队列后立即返回。
- 单独 writer 线程消费队列。
- 使用 `sqlite3_prepare_v2` 预编译 `INSERT`，通过 `sqlite3_bind_*` 绑定参数。
- 按时间或条数做短事务批量提交，例如 100 条或 1 秒 flush 一次。需要在性能和异常退出丢失窗口之间做明确取舍。
- 所有 SQLite 返回值必须检查并记录错误。

### P0：缺少 WAL 和 busy timeout

当前数据库是 `journal_mode=delete`。可视化服务只读打开数据库，采集端持续写入时，读写互相影响概率高于 WAL。

建议采集端初始化数据库时设置：

```sql
PRAGMA journal_mode=WAL;
PRAGMA busy_timeout=3000;
```

是否调整 `synchronous=NORMAL` 需要单独权衡数据安全：它能降低提交开销，但异常断电时风险高于 FULL。按当前优先级“安全性 = 正确性 > 性能”，建议先保留默认同步级别，只引入 WAL 和批量写。

### P0：缺少业务索引

建议至少增加：

```sql
CREATE INDEX IF NOT EXISTS idx_keys_date
ON keys(date);

CREATE INDEX IF NOT EXISTS idx_keys_date_vk_name
ON keys(date, vk_code, key_name);

CREATE INDEX IF NOT EXISTS idx_keys_key_name_vk_code
ON keys(key_name, vk_code);
```

对应收益：

- `idx_keys_date`：支撑 `/api/daily-stats`、`MIN(date)`、`MAX(date)` 和按日期过滤。
- `idx_keys_date_vk_name`：支撑日期范围内的键盘热力图聚合。
- `idx_keys_key_name_vk_code`：支撑全量按键排行，降低全表排序/分组成本。

索引会增加写入成本和文件体积，但对当前读多、统计多的可视化场景是必要成本。新增索引前应在实际数据库上执行 `EXPLAIN QUERY PLAN` 验证是否命中。

### P1：原始明细表承担了所有统计职责

当前 API 每次都从 `keys` 明细表重新聚合。数据长期增长后，即使有索引，全量统计仍会变慢。

建议增加汇总表：

```sql
CREATE TABLE IF NOT EXISTS daily_key_stats (
    date TEXT NOT NULL,
    vk_code INTEGER NOT NULL,
    key_name TEXT NOT NULL,
    count INTEGER NOT NULL,
    PRIMARY KEY (date, vk_code, key_name)
);

CREATE TABLE IF NOT EXISTS daily_hour_stats (
    date TEXT NOT NULL,
    hour INTEGER NOT NULL,
    count INTEGER NOT NULL,
    PRIMARY KEY (date, hour)
);
```

写入明细时同步 `UPSERT` 汇总计数。可视化接口优先查询汇总表，只有最近记录或诊断查询读明细表。

这样页面加载成本从“扫描全部明细行”变成“扫描天数 × 按键数”的小表，数据增长几年后仍稳定。

### P1：缺少保留、归档和压缩策略

README 已提示“长期运行会产生大量数据，建议定期清理旧数据”，但代码和文档没有定义保留策略。

建议：

- 默认保留最近 12~24 个月明细。
- 月度归档旧明细到 `archive/keyrecord_YYYY_MM.db`。
- 主库只保留可视化常用时间范围。
- 大量删除后按需 `VACUUM`，避免文件长期不回缩。

### P2：`AUTOINCREMENT` 可去掉

`id INTEGER PRIMARY KEY AUTOINCREMENT` 会引入额外的 `sqlite_sequence` 维护成本。当前业务没有要求 id 永不复用，普通 `INTEGER PRIMARY KEY` 即可。这个不是主要瓶颈，优先级低于写入队列、WAL、索引和汇总表。

## 附带发现

这些问题不是本次性能评估的主线，但会影响数据正确性和排障能力。

1. `main.cpp` 使用字符串拼接 SQL，`key_name` 为单引号 `'` 时会生成非法 SQL。当前又没有检查 `sqlite3_exec` 返回值，可能静默丢记录。改用 prepared statement 和 bind 可以同时解决性能、转义和错误处理。
2. `sqlite3_open`、建表、插入都没有检查返回值。数据库路径不可写、文件损坏、SQL 失败时程序仍会继续运行。
3. Node API 的 `limit` 没有做范围限制，日期参数也没有格式校验。当前只在本地使用风险较低，但不应对外暴露。

## 推荐改造顺序

1. 先改写入链路：Hook 入队、writer 线程、prepared statement、错误日志。
2. 启用 WAL 和 `busy_timeout`，验证采集端与可视化端同时运行时无锁冲突。
3. 增加三类索引，并用 `EXPLAIN QUERY PLAN` 验证核心 API 不再全表扫描。
4. 引入日维度汇总表，API 改查汇总表。
5. 定义明细保留和归档策略。
6. 数据量接近千万行后，再评估是否需要 worker thread 查询、缓存 API 结果或拆分月库。

## 迁移到服务端数据库的触发条件

满足以下任一条件时，再考虑 PostgreSQL/MySQL：

- 多个采集端集中写入同一个库。
- 多个用户频繁访问复杂统计页面。
- 需要远程访问、权限体系、审计、多租户。
- 数据规模接近 TB，且仍需要大量 ad-hoc 明细分析。
- 需要长查询与写入并发，且不能接受 SQLite 单 writer 模型。

当前项目未达到这些条件。

## 参考资料

- [SQLite: Appropriate Uses For SQLite](https://www.sqlite.org/whentouse.html)
- [SQLite: Write-Ahead Logging](https://www.sqlite.org/wal.html)
- [SQLite: Query Planning](https://www.sqlite.org/queryplanner.html)
- [better-sqlite3 README](https://github.com/WiseLibs/better-sqlite3)
- [Microsoft: LowLevelKeyboardProc function](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc)
