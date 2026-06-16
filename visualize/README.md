# KeyRecord 可视化系统

## 功能特点

- ✅ **每日按键统计日历图**: GitHub 风格的热力日历，展示每日按键总数
- ✅ **数据统计面板**: 显示总按键数、数据范围等信息
- ✅ **响应式设计**: 适配不同屏幕尺寸
- 🚧 **键盘热力图**: 需要有完整的按键数据（vk_code 和 key_name）
- 🚧 **Top 按键排行**: 需要有完整的按键数据

## 快速开始

### 1. 安装依赖

```bash
cd visualize
npm install
```

### 2. 启动服务

```bash
npm start
```

服务器将在 `http://localhost:3000` 启动

### 3. 访问网页

在浏览器中打开: http://localhost:3000

## API 端点

- `GET /api/info` - 获取数据库信息（总按键数、日期范围、唯一按键数）
- `GET /api/daily-stats` - 获取每日按键统计
- `GET /api/heatmap?date=YYYY-MM-DD` - 获取指定日期的热力图数据
- `GET /api/heatmap?start=YYYY-MM-DD&end=YYYY-MM-DD` - 获取时间范围的热力图数据
- `GET /api/keys?start=...&end=...&limit=N` - 获取按键排行

## 数据库结构

确保数据库表结构包含以下字段：

```sql
CREATE TABLE keys(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  timestamp INTEGER,
  date TEXT,
  hour INTEGER,
  vk_code INTEGER,
  key_name TEXT
);
```

**注意**: 如果你的数据库是旧版本创建的，可能缺少 `vk_code` 和 `key_name` 字段。请运行：

```bash
sqlite3 keyrecord.db "ALTER TABLE keys ADD COLUMN vk_code INTEGER; ALTER TABLE keys ADD COLUMN key_name TEXT;"
```

## 技术栈

### 后端
- Node.js + Express
- better-sqlite3
- CORS

### 前端
- D3.js - 日历图渲染
- heatmap.js - 键盘热力图（待完整实现）
- 原生 JavaScript

## 项目结构

```
visualize/
├── server.js              # API 服务器
├── keyboard-layout.js     # 键盘坐标映射
├── package.json
├── public/
│   ├── index.html         # 主页面
│   ├── css/
│   │   └── style.css      # 样式文件
│   └── js/
│       ├── main.js        # 主控制器
│       └── calendar.js    # 日历图模块
└── README.md
```

## 开发说明

### 添加更多功能

1. **键盘热力图**: 需要完整的按键记录数据（vk_code 和 key_name）
2. **时间范围选择**: 可以扩展日历图支持点击选择日期范围
3. **实时更新**: 可以添加 WebSocket 支持实时数据推送

### 修改端口

修改 `server.js` 中的 `PORT` 常量：

```javascript
const PORT = 3000; // 改为你想要的端口
```

## 常见问题

### Q: 为什么日历图没有数据？
A: 确保你的 C++ 程序正在运行并记录按键数据到数据库。

### Q: 如何停止服务器？
A: 在终端按 `Ctrl+C`

### Q: 数据库在哪里？
A: 数据库文件 `keyrecord.db` 位于项目根目录（visualize 的上一级目录）

## 许可证

MIT
