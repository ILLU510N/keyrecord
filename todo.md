# KeyRecord 可视化开发计划

## 目标
使用 heatmap.js + calendar-heatmap 组合方案，从 SQLite 数据库读取按键记录数据并进行可视化展示。

## 技术栈选择

### 前端可视化库
1. **heatmap.js** - 键盘热力图渲染
   - 使用 HTML5 Canvas
   - 支持自定义渐变色
   - 可配置背景图片（键盘布局）
   
2. **calendar-heatmap (D3.js)** - GitHub 风格日历图
   - SVG 渲染
   - 时间序列数据展示
   - 支持按日期着色

3. **D3.js** - 时间轴选择器
   - 实现日期范围选择
   - 联动更新热力图数据

### 后端数据服务
- **Node.js + Express** - 提供 RESTful API
- **better-sqlite3** - 读取 keyrecord.db 数据库
- **CORS** 支持 - 允许前端跨域请求

## 架构设计

```
┌─────────────────────────────────────────────┐
│           前端 HTML 页面                     │
│  ┌───────────────────────────────────────┐  │
│  │  时间轴选择器 (D3.js)                  │  │
│  │  [========|====|========]              │  │
│  └───────────────────────────────────────┘  │
│                    │                         │
│         选择日期范围 (AJAX)                  │
│                    ↓                         │
│  ┌───────────────────────────────────────┐  │
│  │  键盘热力图 (heatmap.js)              │  │
│  │  [键盘布局 + 热力叠加层]              │  │
│  └───────────────────────────────────────┘  │
│                                              │
│  ┌───────────────────────────────────────┐  │
│  │  每日统计日历图 (calendar-heatmap)    │  │
│  │  [■][■][□][■][■][□][□] ...          │  │
│  └───────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
                    ↑ HTTP API
┌─────────────────────────────────────────────┐
│      Node.js API 服务 (localhost:3000)      │
│  ┌───────────────────────────────────────┐  │
│  │  GET /api/daily-stats                 │  │
│  │  - 返回每日按键总数                    │  │
│  │                                        │  │
│  │  GET /api/heatmap?date=YYYY-MM-DD     │  │
│  │  - 返回指定日期的按键热力数据           │  │
│  │                                        │  │
│  │  GET /api/keys?start=...&end=...      │  │
│  │  - 返回时间范围内的按键统计             │  │
│  └───────────────────────────────────────┘  │
│                    ↓ SQL Query              │
│           better-sqlite3                    │
└─────────────────────────────────────────────┘
                    ↓
            keyrecord.db (SQLite)
```

## 开发步骤

### Phase 1: API 服务开发
**目标**: 创建 Node.js 后端服务，提供数据 API

#### 1.1 项目初始化
- [x] 创建 `visualize/` 目录
- [x] 初始化 npm 项目: `npm init -y`
- [ ] 安装依赖:（部分完成：express / better-sqlite3 / cors 已安装，nodemon 未安装）
  ```bash
  npm install express better-sqlite3 cors
  npm install --save-dev nodemon
  ```

#### 1.2 实现 API 端点
**文件**: `visualize/server.js`

- [x] **GET /api/daily-stats**
  - 查询: `SELECT date, COUNT(*) as count FROM keys GROUP BY date ORDER BY date`
  - 返回格式: `[{date: "2024-01-01", count: 1234}, ...]`
  - 用途: calendar-heatmap 日历图数据

- [x] **GET /api/heatmap?date=YYYY-MM-DD**
  - 查询: `SELECT vk_code, key_name, COUNT(*) as count FROM keys WHERE date = ? GROUP BY vk_code`
  - 返回格式: `[{vk_code: 65, key_name: "A", count: 123, x: 100, y: 200}, ...]`
  - 需要映射虚拟键码到键盘坐标 (x, y)
  - 用途: heatmap.js 键盘热力图数据

- [x] **GET /api/heatmap?start=YYYY-MM-DD&end=YYYY-MM-DD**
  - 查询: `SELECT vk_code, key_name, COUNT(*) as count FROM keys WHERE date BETWEEN ? AND ? GROUP BY vk_code`
  - 支持时间范围查询
  - 用途: 时间轴选择器联动更新

- [ ] **GET /api/hourly-stats?date=YYYY-MM-DD**
  - 查询: `SELECT hour, COUNT(*) as count FROM keys WHERE date = ? GROUP BY hour`
  - 返回格式: `[{hour: 0, count: 50}, {hour: 1, count: 30}, ...]`
  - 用途: 小时级别热力分布（可选功能）

#### 1.3 键盘坐标映射
**文件**: `visualize/keyboard-layout.js`

- [x] 定义标准 QWERTY 键盘布局坐标
  ```javascript
  const keyboardLayout = {
    // 虚拟键码 -> {x, y, width, height} 坐标
    65: { x: 150, y: 200, width: 50, height: 50 }, // A
    66: { x: 350, y: 250, width: 50, height: 50 }, // B
    // ... 映射所有支持的按键
  };
  ```
- [ ] 支持的按键类型:（部分完成：字母、数字、基础控制键、修饰键和部分标点已映射；功能键、方向键、数字键盘等未映射）
  - 字母键 A-Z (VK 65-90)
  - 数字键 0-9 (VK 48-57)
  - 功能键 F1-F12 (VK 112-123)
  - 控制键 Enter, Space, Backspace, Tab, Esc
  - 方向键
  - 修饰键 Shift, Ctrl, Alt
  - 数字键盘 Numpad
  - 标点符号

#### 1.4 测试 API
- [x] 启动服务: `node server.js`
- [x] 测试端点:
  - `curl http://localhost:3000/api/daily-stats`
  - `curl http://localhost:3000/api/heatmap?date=2024-01-01`
- [x] 验证 CORS 配置

---

### Phase 2: 前端页面开发
**目标**: 创建可视化 HTML 页面

#### 2.1 静态资源准备
**目录结构**:
```
visualize/
├── public/
│   ├── index.html          # 主页面
│   ├── css/
│   │   └── style.css       # 样式
│   ├── js/
│   │   ├── main.js         # 主逻辑
│   │   ├── keyboard.js     # 键盘热力图
│   │   ├── calendar.js     # 日历图
│   │   └── timeline.js     # 时间轴选择器
│   └── images/
│       └── keyboard.png    # 键盘布局背景图
├── server.js               # API 服务
├── keyboard-layout.js      # 键盘坐标映射
└── package.json
```

#### 2.2 引入依赖库
**文件**: `public/index.html`

- [ ] 引入 CDN 资源:（部分完成：已引入 D3.js 和 heatmap.js，未引入 calendar-heatmap）
  ```html
  <!-- D3.js -->
  <script src="https://d3js.org/d3.v7.min.js"></script>
  
  <!-- heatmap.js -->
  <script src="https://cdn.jsdelivr.net/npm/heatmap.js@2.0.5/build/heatmap.min.js"></script>
  
  <!-- calendar-heatmap -->
  <script src="https://cdn.jsdelivr.net/npm/cal-heatmap@4.2.4/cal-heatmap.min.js"></script>
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/cal-heatmap@4.2.4/cal-heatmap.css">
  ```

#### 2.3 页面布局设计
**文件**: `public/index.html`

```html
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>KeyRecord 可视化</title>
  <link rel="stylesheet" href="css/style.css">
</head>
<body>
  <div class="container">
    <header>
      <h1>KeyRecord - 按键统计可视化</h1>
    </header>
    
    <!-- 时间轴选择器 -->
    <section id="timeline-section">
      <h2>时间范围选择</h2>
      <div id="timeline"></div>
      <div class="date-range">
        <input type="date" id="start-date">
        <span> 至 </span>
        <input type="date" id="end-date">
        <button id="apply-range">应用</button>
      </div>
    </section>
    
    <!-- 键盘热力图 -->
    <section id="heatmap-section">
      <h2>键盘热力图</h2>
      <div class="stats-bar">
        <span>总按键数: <strong id="total-keys">0</strong></span>
        <span>时间范围: <strong id="date-range">-</strong></span>
      </div>
      <div id="keyboard-heatmap"></div>
      <div class="legend">
        <span>低频</span>
        <div class="gradient"></div>
        <span>高频</span>
      </div>
    </section>
    
    <!-- GitHub 风格日历图 -->
    <section id="calendar-section">
      <h2>每日按键统计</h2>
      <div id="calendar-heatmap"></div>
      <div class="calendar-legend">
        <span>少</span>
        <span class="box" style="background: #ebedf0"></span>
        <span class="box" style="background: #9be9a8"></span>
        <span class="box" style="background: #40c463"></span>
        <span class="box" style="background: #30a14e"></span>
        <span class="box" style="background: #216e39"></span>
        <span>多</span>
      </div>
    </section>
    
    <!-- Top 按键排行 -->
    <section id="top-keys-section">
      <h2>Top 20 常用按键</h2>
      <table id="top-keys-table">
        <thead>
          <tr>
            <th>排名</th>
            <th>按键</th>
            <th>次数</th>
            <th>占比</th>
          </tr>
        </thead>
        <tbody></tbody>
      </table>
    </section>
  </div>
  
  <script src="js/keyboard.js"></script>
  <script src="js/calendar.js"></script>
  <script src="js/timeline.js"></script>
  <script src="js/main.js"></script>
</body>
</html>
```

#### 2.4 样式设计
**文件**: `public/css/style.css`

- [x] 响应式布局
- [ ] 深色/浅色主题切换（可选）
- [ ] 键盘热力图容器尺寸: 1200px × 400px
- [x] 日历图容器自适应宽度
- [ ] 渐变色图例
- [x] GitHub 风格色板

---

### Phase 3: 核心功能实现

#### 3.1 键盘热力图模块
**文件**: `public/js/keyboard.js`

- [ ] 初始化 heatmap.js 实例:
  ```javascript
  const heatmapInstance = h337.create({
    container: document.getElementById('keyboard-heatmap'),
    radius: 30,
    maxOpacity: 0.8,
    minOpacity: 0.3,
    blur: 0.9,
    gradient: {
      0.0: 'blue',
      0.5: 'green',
      0.7: 'yellow',
      1.0: 'red'
    }
  });
  ```

- [ ] 设置键盘背景图:
  ```javascript
  document.getElementById('keyboard-heatmap').style.backgroundImage = 
    'url(images/keyboard.png)';
  ```

- [ ] 实现数据加载和渲染:
  ```javascript
  async function loadKeyboardHeatmap(startDate, endDate) {
    const response = await fetch(
      `http://localhost:3000/api/heatmap?start=${startDate}&end=${endDate}`
    );
    const data = await response.json();
    
    // 转换为 heatmap.js 格式
    const heatmapData = {
      max: Math.max(...data.map(d => d.count)),
      data: data.map(d => ({
        x: d.x,
        y: d.y,
        value: d.count
      }))
    };
    
    heatmapInstance.setData(heatmapData);
  }
  ```

- [ ] 添加鼠标悬停提示（显示按键名和次数）
- [ ] 实现缩放和平移功能（可选）

#### 3.2 日历图模块
**文件**: `public/js/calendar.js`

- [x] 使用 D3.js 手动实现或使用 calendar-heatmap 库
- [ ] 方案 A: 使用 calendar-heatmap 库（推荐）:
  ```javascript
  const cal = new CalHeatmap();
  cal.init({
    itemSelector: '#calendar-heatmap',
    domain: 'month',
    subDomain: 'day',
    data: 'http://localhost:3000/api/daily-stats',
    dataType: 'json',
    start: new Date(2024, 0, 1),
    cellSize: 12,
    cellPadding: 2,
    domainGutter: 10,
    range: 12,
    legend: [100, 500, 1000, 2000],
    legendColors: ['#ebedf0', '#9be9a8', '#40c463', '#30a14e', '#216e39']
  });
  ```

- [x] 方案 B: 使用 D3.js 手动实现（更灵活）:
  - 创建 SVG 画布
  - 按日期绘制矩形格子
  - 根据按键数量映射颜色
  - 添加日期标签和提示

- [ ] 实现点击日期联动更新键盘热力图
- [x] 添加日期悬停提示

#### 3.3 时间轴选择器模块
**文件**: `public/js/timeline.js`

- [ ] 使用 D3.js 创建日期范围滑块:
  ```javascript
  const timeScale = d3.scaleTime()
    .domain([new Date(2024, 0, 1), new Date()])
    .range([0, width]);
  
  const brush = d3.brushX()
    .extent([[0, 0], [width, height]])
    .on('end', brushed);
  
  function brushed(event) {
    if (event.selection) {
      const [startDate, endDate] = event.selection.map(timeScale.invert);
      updateVisualization(startDate, endDate);
    }
  }
  ```

- [ ] 或使用原生 HTML5 date input
- [ ] 添加快捷选择按钮:
  - 今天
  - 最近 7 天
  - 最近 30 天
  - 本月
  - 全部

- [ ] 联动更新键盘热力图和统计数据

#### 3.4 主控制器
**文件**: `public/js/main.js`

- [ ] 页面加载时初始化所有模块
- [ ] 默认显示最近 7 天的数据
- [ ] 实现模块间数据联动:
  ```javascript
  async function updateVisualization(startDate, endDate) {
    // 更新日期范围显示
    document.getElementById('date-range').textContent = 
      `${startDate} ~ ${endDate}`;
    
    // 并行加载数据
    await Promise.all([
      loadKeyboardHeatmap(startDate, endDate),
      loadTopKeys(startDate, endDate),
      updateStats(startDate, endDate)
    ]);
  }
  ```

- [ ] 实现 Top 20 按键排行表:
  ```javascript
  async function loadTopKeys(startDate, endDate) {
    const response = await fetch(
      `http://localhost:3000/api/keys?start=${startDate}&end=${endDate}`
    );
    const data = await response.json();
    const total = data.reduce((sum, d) => sum + d.count, 0);
    
    const tbody = document.querySelector('#top-keys-table tbody');
    tbody.innerHTML = data.slice(0, 20).map((d, i) => `
      <tr>
        <td>${i + 1}</td>
        <td><strong>${d.key_name}</strong></td>
        <td>${d.count.toLocaleString()}</td>
        <td>${(d.count / total * 100).toFixed(2)}%</td>
      </tr>
    `).join('');
  }
  ```

- [ ] 添加加载动画
- [x] 错误处理和提示

---

### Phase 4: 键盘布局图准备

#### 4.1 方案 A: 使用现成的键盘布局图片
- [ ] 在网上找一张标准 QWERTY 键盘俯视图
- [ ] 推荐尺寸: 1200px × 400px
- [ ] 透明背景或纯色背景
- [ ] 保存为 `public/images/keyboard.png`

#### 4.2 方案 B: 使用 CSS/SVG 绘制键盘
- [ ] 创建 `keyboard-layout.svg` 文件
- [ ] 使用矩形绘制每个按键
- [ ] 标注按键标签
- [ ] 优点: 完全可控，矢量图形
- [ ] 缺点: 开发工作量大

#### 4.3 方案 C: 使用 HTML/CSS 构建键盘
- [ ] 使用 `<div>` 元素布局键盘
- [ ] 每个按键一个 div，使用 absolute 定位
- [ ] heatmap.js 叠加在键盘 div 上层
- [ ] 优点: 灵活，可交互
- [ ] 缺点: 布局复杂

**推荐**: 先使用方案 A 快速实现，后续可升级到方案 B 或 C

---

### Phase 5: 优化和增强功能

#### 5.1 性能优化
- [ ] API 响应缓存（Redis 或内存缓存）
- [ ] 数据库查询优化（添加索引）:
  ```sql
  CREATE INDEX idx_date ON keys(date);
  CREATE INDEX idx_vk_code ON keys(vk_code);
  CREATE INDEX idx_date_vk ON keys(date, vk_code);
  ```
- [ ] 前端数据缓存（LocalStorage）
- [ ] 按需加载数据（虚拟滚动）

#### 5.2 用户体验增强
- [ ] 响应式设计（支持移动端）
- [ ] 主题切换（深色/浅色模式）
- [ ] 数据导出功能（CSV, JSON, PNG）
- [ ] 打印优化
- [ ] 加载进度条
- [x] 空数据状态提示

#### 5.3 高级功能（可选）
- [ ] 小时级热力图
- [ ] 按键时序动画回放
- [ ] 多键组合统计（Ctrl+C, Ctrl+V）
- [ ] 键盘分区统计（字母区、数字区、功能区）
- [ ] 左右手使用频率对比
- [ ] 输入速度曲线图
- [ ] 数据对比（不同日期/时间段）
- [ ] 实时监控模式（WebSocket 推送）

#### 5.4 部署和打包
- [ ] 使用 PM2 管理 Node.js 进程
- [ ] 配置开机自启动
- [ ] 打包为单个可执行文件（可选 electron）
- [ ] 提供安装脚本

---

## 开发顺序建议

### Sprint 1: MVP (最小可行产品)
**时间**: 1-2 天
1. 实现基础 API (daily-stats, heatmap)
2. 创建简单的 HTML 页面
3. 使用现成键盘图片 + heatmap.js 显示基础热力图
4. 实现 calendar-heatmap 日历图

### Sprint 2: 交互功能
**时间**: 1 天
1. 实现时间范围选择器
2. 添加 Top 20 按键排行
3. 模块间联动更新
4. 基础样式美化

### Sprint 3: 优化和增强
**时间**: 1-2 天
1. 性能优化（缓存、索引）
2. 用户体验优化（加载动画、错误处理）
3. 响应式设计
4. 数据导出功能

### Sprint 4: 高级功能（可选）
**时间**: 根据需求
1. 小时级热力图
2. 按键组合统计
3. 实时监控
4. Electron 打包

---

## 技术难点和解决方案

### 难点 1: 虚拟键码到键盘坐标的映射
**解决方案**:
1. 手动测量键盘图片中每个按键的位置
2. 使用图像编辑软件（如 Photoshop）标注坐标
3. 创建映射表存储在 `keyboard-layout.js` 中
4. 对于不同键盘布局（QWERTY, AZERTY），提供多套坐标映射

### 难点 2: heatmap.js 热力点精确对齐
**解决方案**:
1. 设置合适的 radius 参数（建议 20-30px）
2. 调整 blur 参数实现热力扩散效果
3. 使用相对坐标而非绝对坐标
4. 考虑键盘按键大小差异（Space, Enter, Shift 等）

### 难点 3: 大数据量性能问题
**解决方案**:
1. 数据库添加索引
2. API 返回聚合后的数据而非原始记录
3. 前端使用分页或虚拟滚动
4. 使用 Web Worker 处理数据转换

### 难点 4: 跨域问题
**解决方案**:
1. API 服务器启用 CORS
2. 或将前端页面也由 Express 服务（推荐）
3. 使用代理服务器

---

## 测试计划

### 单元测试
- [ ] API 端点响应格式
- [ ] 键盘坐标映射正确性
- [ ] 数据聚合逻辑

### 集成测试
- [ ] 前后端数据流
- [ ] 时间范围选择联动
- [ ] 日历图点击联动

### 浏览器兼容性测试
- [ ] Chrome
- [ ] Firefox
- [ ] Edge
- [ ] Safari (可选)

### 性能测试
- [ ] 大数据量加载（10万+ 记录）
- [ ] 热力图渲染性能
- [ ] 内存泄漏检测

---

## 里程碑

- [x] **M1**: API 服务可用，能返回正确数据
- [ ] **M2**: 基础键盘热力图显示
- [x] **M3**: 日历图显示每日统计
- [ ] **M4**: 时间轴选择器联动工作
- [ ] **M5**: 完整功能可用，样式美化
- [ ] **M6**: 性能优化完成
- [ ] **M7**: 部署上线

---

## 参考资源

### 文档
- [heatmap.js 官方文档](https://www.patrick-wied.at/static/heatmapjs/)
- [calendar-heatmap GitHub](https://github.com/DKirwan/calendar-heatmap)
- [D3.js 官方文档](https://d3js.org/)
- [better-sqlite3 文档](https://github.com/WiseLibs/better-sqlite3)

### 示例项目
- [InputScope](https://github.com/suurjaak/InputScope) - 参考完整实现
- [Keyboard-Heatmap](https://github.com/pa7/Keyboard-Heatmap) - 参考键盘布局
- [GitHub 贡献图](https://github.com/) - 参考日历图设计

### 工具
- [Keyboard Layout Editor](http://www.keyboard-layout-editor.com/) - 键盘布局设计
- [ColorBrewer](https://colorbrewer2.org/) - 热力图配色方案

---

## 风险和缓解措施

| 风险 | 影响 | 概率 | 缓解措施 |
|-----|------|------|---------|
| 键盘坐标映射工作量大 | 高 | 高 | 提供工具辅助测量，或使用 CSS 布局代替图片 |
| heatmap.js 性能问题 | 中 | 中 | 限制数据点数量，使用 Canvas 优化 |
| 数据库查询慢 | 中 | 低 | 添加索引，使用缓存 |
| 浏览器兼容性问题 | 低 | 中 | 使用 Babel 转译，Polyfill 支持 |
| 依赖库停止维护 | 低 | 低 | 准备备选方案，考虑自己实现核心功能 |

---

## 下一步行动

1. ✅ 确认开发计划
2. ⏭️ 开始 Phase 1: API 服务开发
3. ⏭️ 准备键盘布局图片
4. ⏭️ 搭建前端页面框架

**预计总开发时间**: 3-5 天（MVP），1-2 周（完整版）
