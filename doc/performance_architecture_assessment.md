# KeyRecord 架构性能待办

评估日期：2026-06-16  
更新说明：已删除当前代码中已经完成的写入队列、批量提交、prepared statement、WAL、busy timeout 和业务索引相关内容。

## 判断

当前项目仍可以继续使用 SQLite。剩余风险主要集中在长期数据增长后的统计查询成本、数据保留策略和 API 输入约束。

当前核心数据流：

`	ext
键盘事件
  -> Windows WH_KEYBOARD_LL Hook
  -> tray_app.cpp::KeyboardProc
  -> key_event_writer.cpp 写入队列和 writer 线程
  -> keyrecord.db / keys 明细表
  -> keyrecord_server C++ 只读可视化服务
  -> /api/info / /api/daily-stats / /api/heatmap / /api/keys
  -> 前端日历图、键盘热力图和 Top 20 排行
`

## P1：原始明细表仍承担所有统计职责

当前 API 仍从 keys 明细表聚合统计。数据长期增长后，即使已有索引，全量统计仍会变慢，尤其是：

- /api/daily-stats 按日期聚合。
- /api/heatmap 按日期范围聚合按键。
- /api/keys 全量或范围内按键排行。
- /api/info 中的 COUNT(DISTINCT vk_code)。

建议增加汇总表：

`sql
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
`

写入明细时同步 UPSERT 汇总计数。可视化接口优先查询汇总表，只有诊断或明细查询读取 keys。

## P1：缺少保留、归档和压缩策略

长期运行会持续扩大主库文件。建议定义明确策略：

- 默认保留最近 12 到 24 个月明细。
- 月度归档旧明细到 archive/keyrecord_YYYY_MM.db。
- 主库只保留可视化常用时间范围。
- 大量删除后按需执行 VACUUM，避免文件长期不回缩。

## P2：AUTOINCREMENT 可评估去除

当前 keys 表仍使用：

`sql
id INTEGER PRIMARY KEY AUTOINCREMENT
`

如果业务不要求 id 永不复用，可改为普通 INTEGER PRIMARY KEY，减少 sqlite_sequence 维护成本。该项优先级低于汇总表和归档策略。

## P2：API 输入约束仍需补齐

当前 /api/keys 的 limit 只处理非法值回退，缺少最大值限制；日期参数也缺少统一格式校验。

建议：

- limit 设置最大值，例如 100 或 500。
- 日期参数仅接受 YYYY-MM-DD。
- start > end 时返回 400。
- 对缺失或非法参数返回稳定 JSON 错误格式。

## 后续观察点

数据量接近千万行后，再评估是否需要：

- API 查询结果缓存。
- 查询 worker 线程。
- 按月拆分数据库。
- 更激进的数据归档策略。

满足以下任一条件时，再考虑 PostgreSQL/MySQL：

- 多个采集端集中写入同一个库。
- 多个用户频繁访问复杂统计页面。
- 需要远程访问、权限体系、审计或多租户。
- 数据规模接近 TB，且仍需要大量 ad-hoc 明细分析。
- 需要长查询与写入并发，且不能接受 SQLite 单 writer 模型。

## 参考资料

- [SQLite: Appropriate Uses For SQLite](https://www.sqlite.org/whentouse.html)
- [SQLite: Write-Ahead Logging](https://www.sqlite.org/wal.html)
- [SQLite: Query Planning](https://www.sqlite.org/queryplanner.html)
- [Microsoft: LowLevelKeyboardProc function](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc)
