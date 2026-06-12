# KeyRecord - 键盘敲击记录工具（后台守护进程版）

记录 Windows 10/11 键盘敲击，统计每日和每小时热力值，并记录每个按键的详细信息。

## 功能特性

- **静默后台运行**：无控制台窗口，自动隐藏到系统托盘
- **系统托盘图标**：右键托盘图标可退出程序
- **基础统计**：记录每次按键的时间戳、日期和小时
- **按键识别**：记录每个按键的虚拟键码（vk_code）和键名（key_name）
- **支持的按键类型**：
  - 字母和数字：A-Z, 0-9
  - 功能键：F1-F12
  - 控制键：Enter, Space, Backspace, Tab, Esc
  - 方向键：Up, Down, Left, Right
  - 修饰键：Shift, Ctrl, Alt（包括左右区分）
  - 数字键盘：Numpad0-9, +, -, *, /, .
  - 其他特殊键：Home, End, PageUp, PageDown, Insert, Delete, PrintScreen 等
  - 标点符号：; = , - . / ` [ \ ] '

## 依赖

- CMake 3.20+
- SQLite3
- Visual Studio (MSVC)

Windows 安装 SQLite：
```powershell
# 使用 vcpkg
vcpkg install sqlite3:x64-windows
```

## 编译

```powershell
cmake -B build
cmake --build build --config Release
```

或使用 MSBuild：
```powershell
MSBuild.exe build\keyrecord.vcxproj /p:Configuration=Release /p:Platform=x64
```

## 运行

### 方式一：直接运行（静默后台）

```powershell
.\build\Release\keyrecord.exe
```

程序会自动隐藏，在系统托盘显示图标。

### 方式二：使用启动脚本（推荐）

```powershell
.\start.vbs
```

双击 `start.vbs` 即可完全静默启动，不会有任何窗口闪烁。

### 停止程序

- 方法1：右键点击系统托盘的程序图标，选择"退出"
- 方法2：使用任务管理器结束 `keyrecord.exe` 进程

## 开机自启动

### 方法一：通过启动文件夹

1. 按 `Win + R`，输入 `shell:startup` 打开启动文件夹
2. 创建 `start.vbs` 的快捷方式到该文件夹
3. 重启电脑后自动运行

### 方法二：通过任务计划程序

```powershell
$action = New-ScheduledTaskAction -Execute "D:\code\keyrecord\build\Release\keyrecord.exe"
$trigger = New-ScheduledTaskTrigger -AtLogon
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries
Register-ScheduledTask -TaskName "KeyRecord" -Action $action -Trigger $trigger -Settings $settings -Description "键盘记录守护进程"
```

卸载自启动：
```powershell
Unregister-ScheduledTask -TaskName "KeyRecord" -Confirm:$false
```

## 数据库结构

```sql
CREATE TABLE keys (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER,      -- Unix 时间戳
    date TEXT,              -- 日期（YYYY-MM-DD）
    hour INTEGER,           -- 小时（0-23）
    vk_code INTEGER,        -- 虚拟键码
    key_name TEXT           -- 键名（如 "A", "Enter", "F1"）
);
```

## 查询统计

使用 `query.sql` 中的 SQL 查询：

```powershell
sqlite3 keyrecord.db < query.sql
```

或交互式查询：
```powershell
sqlite3 keyrecord.db
sqlite> SELECT date, COUNT(*) FROM keys GROUP BY date;
```

### 查询示例

- **每日敲击总数**
- **每小时热力分布**
- **最常按的键（Top 20）**
- **特定键的使用频率**
- **最近的按键记录**
- **字母键使用统计**
- **功能键使用统计**

详细查询语句参见 `query.sql` 文件。

## 技术特性

- **WIN32 GUI应用**：编译为 Windows GUI 应用，不会显示控制台窗口
- **消息循环窗口**：使用 `HWND_MESSAGE` 创建隐藏窗口，不占用任务栏
- **系统托盘集成**：使用 Shell_NotifyIcon API 集成到系统托盘
- **全局键盘钩子**：使用 `WH_KEYBOARD_LL` 低级键盘钩子捕获所有按键
- **优雅退出**：正确释放钩子、关闭数据库、删除托盘图标

## 注意事项

1. 需要管理员权限才能正常捕获所有应用的按键
2. 杀毒软件可能会将键盘记录程序标记为可疑，请添加白名单
3. 数据库文件位于程序运行目录，建议定期备份
4. 长期运行会产生大量数据，建议定期清理旧数据
