# KeyRecord 架构性能待办

评估日期：2026-06-16  
更新说明：已删除当前代码中已经完成的写入队列、批量提交、prepared statement、WAL、busy timeout、业务索引、API 输入约束、汇总表和缓存相关待办。

## 判断

当前项目仍可以继续使用 SQLite。写入端已经通过批量事务、prepared statement、WAL、业务索引和日粒度汇总表降低常规读写成本；可视化服务和前端也已经增加短期缓存。剩余风险主要集中在长期明细数据增长后的归档执行、恢复验证和维护窗口管理。

当前核心数据流：

`	ext
键盘事件
  -> Windows WH_KEYBOARD_LL Hook
  -> tray_app.cpp::KeyboardProc
  -> key_event_writer.cpp 写入队列和 writer 线程
  -> keyrecord.db / keys 明细表
  -> daily_key_stats / daily_hour_stats 日粒度汇总表
  -> keyrecord_server C++ 只读可视化服务
  -> VisualizationService 2 秒 API 响应缓存
  -> /api/info / /api/daily-stats / /api/heatmap / /api/keys / 高级分析 API
  -> 前端 LocalStorage/内存缓存
  -> 日历图、键盘热力图、Top 20 排行和按需加载的高级分析
`

## 已完成：汇总表承接常规统计职责

写入端现在维护两个日粒度汇总表：

```sql
CREATE TABLE IF NOT EXISTS daily_key_stats (
    date TEXT NOT NULL,
    vk_code INTEGER NOT NULL,
    key_name TEXT NOT NULL,
    count INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (date, vk_code, key_name)
);

CREATE TABLE IF NOT EXISTS daily_hour_stats (
    date TEXT NOT NULL,
    hour INTEGER NOT NULL,
    count INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (date, hour)
);
```

写入端启动时会用 `INSERT OR IGNORE` 从既有 `keys` 明细回填缺失的汇总行，兼容旧数据库升级场景。后续写入明细时在同一个批量事务内同步 UPSERT 汇总计数。以下接口优先读取汇总表，旧数据库缺少汇总表时回退到 `keys`：

- `/api/info`
- `/api/daily-stats`
- `/api/hourly-stats`
- `/api/keys`
- `/api/heatmap`
- `/api/hourly-heatmap`
- `/api/region-stats`
- `/api/hand-stats`

以下接口仍读取 `keys` 明细，因为它们依赖事件顺序或分钟粒度：

- `/api/timeline`
- `/api/combos`
- `/api/speed`

## 已完成：服务端与前端缓存

`VisualizationService` 对成功的 `GET /api/*` 响应做 2 秒内存缓存，不缓存静态资源、错误响应、非 GET 请求和 CORS 预检。该缓存用于吸收同一范围的短时间重复请求，并避免实时监控 5 秒刷新时返回过旧数据。

前端新增 `ApiCache`，对统计 API 做内存缓存和 LocalStorage 缓存。实时监控刷新前会清理前端缓存；时序回放数据量较大，只使用短期内存缓存，不写入 LocalStorage。

高级分析模块改为按需加载：首次滚动到高级分析区域时才加载统计图表；范围变化后，仅当高级分析已经可见或已经加载过时才刷新。时序回放不随高级分析面板预加载，点击播放时再请求 `/api/timeline`。

## 明细数据保留、归档和压缩策略

默认策略：

- 主库 `keys` 明细默认保留最近 24 个月。
- 归档粒度为自然月，归档库路径为 `archive/keyrecord_YYYY_MM.db`。
- 归档库必须保留与主库一致的 `keys` 表结构，并复制对应月份的明细记录。
- 汇总表 `daily_key_stats` 和 `daily_hour_stats` 长期保留在主库，用于跨月、跨年统计。
- 删除主库旧明细前必须确认归档库记录数与待删除月份记录数一致。
- 大量删除后只允许在明确维护窗口中执行 `VACUUM`，避免正常记录期间长时间占用数据库。

当前实现状态：

- 本轮实现汇总表、缓存和按需加载。
- 自动归档、自动删除和自动 `VACUUM` 尚未实现，后续应以单独维护命令交付。

长期运行仍会持续扩大主库文件。达到保留阈值后，应先交付可审计的手动维护命令，再考虑后台自动化。

## P2：AUTOINCREMENT 可评估去除

当前 keys 表仍使用：

`sql
id INTEGER PRIMARY KEY AUTOINCREMENT
`

如果业务不要求 id 永不复用，可改为普通 INTEGER PRIMARY KEY，减少 sqlite_sequence 维护成本。该项优先级低于汇总表和归档策略。

## 已完成：API 输入约束

API 日期参数仅接受 `YYYY-MM-DD`，缺失必要日期、非法日期、范围不完整和 `start > end` 均返回稳定 JSON 错误。`/api/keys`、`/api/combos`、`/api/timeline` 和 `/api/speed` 的数值参数已经设置上限，避免异常大请求触发重查询。

## 后续观察点

数据量接近千万行后，再评估是否需要：

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
