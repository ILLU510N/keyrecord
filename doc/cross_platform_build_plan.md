# KeyRecord 跨平台编译改造方案

编制日期：2026-07-02
目标：将编译目标从「仅 MSVC / win-x64」扩展为至少支持 **win-x64、linux-x64、darwin-arm64** 三个目标三元组，并保持数据库结构与可视化前端契约不变。

---

## 0. 结论先行

- **可视化服务端几乎已经跨平台**：`server.cpp`、`server_main.cpp`、`visualization_service.*`、`api_queries.*`、`readonly_database.*`、`http_router.*`、`app_config.*`、`config_path.*` 全部只依赖标准库 + SQLite + Boost.Asio/Beast，`int main()` 入口。唯一的平台耦合是 CMake 里显式链接的 `ws2_32` 和硬编码的 vcpkg 路径。
- **`key_classification.cpp` / `keyboard_layout.cpp` 已经可移植**：它们用 `int vkCode` 和数字字面量，不含 `windows.h`。**改造时必须保持这一点。**
- **真正的平台相关代码集中且量小**：
  1. 采集端入口 `main.cpp`（`WinMain`）；
  2. 键盘 Hook + 托盘 `tray_app.cpp/.h`（`SetWindowsHookEx`/`Shell_NotifyIcon`/Win32 消息循环）；
  3. 三个共享文件把 Windows 类型/CRT 泄漏进来：`key_names.*`、`key_event_writer.*`（`DWORD`、`VK_*`、`localtime_s`、`sprintf_s`、`OutputDebugStringA`）。
- **最关键的设计约束**：数据库 `vk_code` 字段、`keyboard_layout.cpp` 的坐标表、前端 `visualize/public/js/keyboard.js`、`key_classification.cpp` 的分区/左右手逻辑，**全部以 Windows 虚拟键码（VK code）的具体数值为契约**。因此 Linux/macOS 采集后端不能直接写入各自的原生键码，**必须把原生键码翻译成 Windows VK 码**再入库，否则热力图、键盘分区、左右手统计会全部错位。这是整个改造的核心。

改造工作量按大小排序：构建系统 > macOS 采集后端 > Linux 采集后端 > 共享代码去耦合（最小、最安全，应最先做）。

---

## 1. 现状平台耦合点清单

### 1.1 构建 / 打包 / CI（全部 Windows 硬编码）

| 位置 | 耦合内容 | 处理方式 |
| --- | --- | --- |
| `CMakeLists.txt:3-5` | 默认 `CMAKE_TOOLCHAIN_FILE = C:/vcpkg/...` | 改为读 `VCPKG_ROOT` 环境变量，且仅在未显式传入时兜底；用 manifest 模式 |
| `CMakeLists.txt:12-14` | `CMAKE_PREFIX_PATH` 硬编码 `C:/vcpkg/installed/x64-windows` | 删除，交给 vcpkg toolchain 自动注入 |
| `CMakeLists.txt:18-22` | Boost 头 `HINTS C:/vcpkg/.../include` | 改用 `find_package(Boost)` 或按平台 HINTS |
| `CMakeLists.txt:24` | `add_definitions(-DUNICODE -D_UNICODE)` | 包进 `if(WIN32)` |
| `CMakeLists.txt:32-46` | `.ico` + `keyrecord.rc.in` + `configure_file` | 包进 `if(WIN32)`；资源仅 Windows 需要 |
| `CMakeLists.txt:48-54` | `copy_sqlite_dll`（把 DLL 拷到 exe 旁） | 包进 `if(WIN32)`；Linux/macOS 用 rpath 或静态库 |
| `CMakeLists.txt:56-60` | `enable_msvc_utf8`（`/utf-8`） | 已用 `if(MSVC)` 守卫，保留即可 |
| `CMakeLists.txt:145` | `keyrecord_server` 链接 `ws2_32` | 改为 `if(WIN32)` 时才追加 |
| `CMakeLists.txt:151-156` | `add_executable(keyrecord WIN32 ...)` + 链接 `shell32` | `WIN32` 子系统标志与 `shell32` 仅 Windows；按平台切换入口与源文件 |
| `CMakeLists.txt:163` | 发布包名写死 `keyrecord-windows-x64.zip` | 改为 `keyrecord-<os>-<arch>.<ext>` |
| `build.ps1` | 纯 PowerShell，仅 Windows | 保留；另加 `CMakePresets.json` 作为三平台统一入口 |
| `cmake/PackageRelease.cmake:45-49` | 用 PowerShell `Compress-Archive` 打包 | 改用 `cmake -E tar`（跨平台），Linux/macOS 出 `.tar.gz` |
| `.github/workflows/cmake-single-platform.yml` | 仅 `windows-2022` + `x64-windows` triplet | 改成 `{windows, ubuntu, macos-14}` 矩阵 |

### 1.2 采集端源码（真正平台相关，需要抽象层）

| 文件 | 耦合内容 |
| --- | --- |
| `main.cpp` | `WinMain` / `HINSTANCE` 入口 |
| `tray_app.cpp/.h` | `SetWindowsHookEx(WH_KEYBOARD_LL)`、`KBDLLHOOKSTRUCT`、Win32 隐藏窗口 + `GetMessage` 消息循环、`Shell_NotifyIcon` 托盘、`MessageBoxW`、`LoadImageW`、`HINSTANCE/HWND/HHOOK/HICON` |

### 1.3 共享代码里的「小泄漏」（去耦合后三平台通用）

| 文件 | 泄漏点 | 替换 |
| --- | --- | --- |
| `key_names.h:6` / `key_event_writer.h:6` | `#include <windows.h>` 仅为拿 `DWORD` | 换成 `KeyCode = std::uint32_t` 别名 |
| `key_names.cpp` | 使用 60+ 个 `VK_*` 宏 | 引入可移植 `virtual_keys.h` 常量头 |
| `key_event_writer.cpp:37` | `OutputDebugStringA` | 平台日志封装（Windows→OutputDebugString，其余→stderr） |
| `key_event_writer.cpp:246` | `localtime_s`（MSVC 签名） | 封装 `localtime_r`/`localtime_s` |
| `key_event_writer.cpp:249` | `sprintf_s`（MSVC 专有） | 换成可移植 `snprintf` |
| `tests/keyrecord_db_tests.cpp:176` | `VK_OEM_7` | 改用 `virtual_keys.h` |

> 注：`tests/*_tests.cpp` 已经用 `#ifdef _WIN32`（`_putenv_s` vs `setenv/unsetenv`）处理环境变量，说明测试层已具备跨平台意识，成本低。

---

## 2. 目标架构：采集端平台抽象层

保持「下游一切以 VK 码为契约」不变，把平台差异收敛到一个薄接口后面。建议新增 `src/platform/` 目录：

```
src/platform/
  key_code.h              // using KeyCode = std::uint32_t;  取代共享头里的 DWORD
  virtual_keys.h          // 可移植 VK_* 常量（镜像 Windows 数值，成为唯一事实源）
  platform_util.h/.cpp    // localtime 封装、snprintf、日志 sink
  capture_backend.h       // 采集后端接口（见下）
  capture_backend_windows.cpp   // SetWindowsHookEx 实现
  capture_backend_linux.cpp     // evdev 实现 + Linux keycode -> VK 映射表
  capture_backend_macos.mm      // CGEventTap 实现 + macOS keycode -> VK 映射表
  tray.h                  // 托盘接口（可选特性）
  tray_windows.cpp        // Shell_NotifyIcon
  tray_macos.mm           // NSStatusBar
  tray_stub.cpp           // Linux/无托盘环境的空实现（headless）
```

### 2.1 采集后端接口（关键）

Windows 的 `WH_KEYBOARD_LL` 依赖 Win32 消息循环，所以现有 `tray_app.cpp` 把「Hook + 消息循环 + 托盘」耦合在一起。跨平台后每个系统的捕获机制自带事件循环（macOS 是 `CFRunLoop`，evdev 是 `read()`/`epoll` 循环），因此抽象接口应让**后端拥有自己的循环**，只回调入库：

```cpp
// capture_backend.h
#pragma once
#include "key_code.h"
#include <chrono>
#include <functional>

namespace keyrecord {

// 回调携带「已归一化为 Windows VK 码」的键码 + 事件时间。
using KeyEventCallback =
    std::function<void(KeyCode vk, std::chrono::system_clock::time_point when)>;

// 启动全局按键捕获，阻塞运行直到 requestStop() 被调用。
// 各平台内部维护自己的事件循环，命中按下事件时调用 callback。
int runCaptureLoop(const KeyEventCallback& callback);

// 从信号处理器 / 托盘退出项 / 其它线程请求停止捕获循环。
void requestStopCapture();

} // namespace keyrecord
```

下游 `enqueueKeyEvent` / `startWriter` / `stopWriter`（`key_event_writer.*`）完全不变，只把参数类型从 `DWORD` 改为 `KeyCode`。

---

## 3. 分平台采集实现要点

### 3.1 Windows（保持现有行为）

把 `tray_app.cpp` 拆成：
- `capture_backend_windows.cpp`：`SetWindowsHookEx(WH_KEYBOARD_LL)` + 隐藏 message-only 窗口 + `GetMessage` 循环，命中 `WM_KEYDOWN` 时回调；`requestStopCapture` → `PostQuitMessage`。
- `tray_windows.cpp`：`Shell_NotifyIcon` 托盘与右键退出菜单。
- 入口 `main.cpp` 用 `#ifdef _WIN32` 提供 `WinMain`（薄封装），或统一 `int main()` 配合 CMake `WIN32_EXECUTABLE` 属性 + `/ENTRY`。回调直接 `enqueueKeyEvent`，与现状等价。

VK 码天然就是 Windows 值，无需翻译表。

### 3.2 Linux-x64

**捕获机制**（按推荐度）：
- **evdev（推荐）**：直接 `read()` `/dev/input/event*` 的 `input_event` 结构。全局有效，X11 与 Wayland 都工作。需要进程对设备节点有读权限（把用户加入 `input` 组，或用 udev 规则 / 以服务身份运行）。
- X11 XRecord / XInput2：仅 X11，Wayland 下失效，不推荐作为唯一方案。

**键码翻译（必须）**：Linux `linux/input-event-codes.h` 的 `KEY_*`（如 `KEY_A=30`、`KEY_ESC=1`、`KEY_LEFTCTRL=29`）→ Windows VK 数值。集中在一张 `static constexpr` 映射表里，是本后端最核心的交付物。示例：

```cpp
// KEY_A(30)->'A'(0x41), KEY_1(2)->'1'(0x31), KEY_ESC(1)->VK_ESCAPE(0x1B) ...
```

**入口 / 退出**：`int main()`；注册 `SIGINT`/`SIGTERM` → `requestStopCapture()` 后 `stopWriter()` 干净收尾。

**托盘**：Linux 托盘依赖桌面环境（libappindicator/GTK 或 DBus StatusNotifierItem），为避免引入重依赖，**默认走 headless（`tray_stub.cpp`）**，以 systemd user service 常驻；托盘作为可选编译特性（`-DKEYRECORD_ENABLE_TRAY=ON` 再引 GTK）。

**依赖**：`pthread`（已隐含）；若走 libevdev 则 `find_library(evdev)`，也可零依赖直接读原始 `input_event`。

### 3.3 macOS darwin-arm64

**捕获机制**：`CGEventTap`（Quartz Event Services），监听 `kCGEventKeyDown`，运行在 `CFRunLoop` 上。

**授权（关键，非代码问题但必须处理）**：需要用户在「系统设置 → 隐私与安全性 → 辅助功能 / 输入监控」中授权本程序（TCC）。首启检测 `AXIsProcessTrusted()`，未授权时引导用户开启。README 必须显著说明。

**键码翻译（必须）**：macOS 虚拟键码（`<Carbon/HIToolbox/Events.h>` 的 `kVK_ANSI_A=0x00`、`kVK_Escape=0x35`、`kVK_ANSI_1=0x12` …）→ Windows VK 数值。同样是一张集中映射表。

**入口 / 退出**：`int main()` 起 `CFRunLoop`；`requestStopCapture` → `CFRunLoopStop`。

**托盘**：`NSStatusBar`（`tray_macos.mm`，Objective-C++）。同样可做成可选。

**构建**：`.mm` 文件需 `enable_language(OBJCXX)`；链接框架 `ApplicationServices`（CGEventTap）、`Carbon`（键码常量）、`AppKit`（NSStatusBar）。可选打包为 `.app` bundle。

---

## 4. 共享代码去 Windows 化（Phase 0，最小且安全）

1. **新增 `src/platform/key_code.h`**：`namespace keyrecord { using KeyCode = std::uint32_t; }`。把 `key_names.h`、`key_event_writer.h` 里的 `#include <windows.h>` 删除、`DWORD` 改为 `keyrecord::KeyCode`（`key_event_writer.cpp` 的 `KeyEvent::vkCode`、各函数签名同步）。
2. **新增 `src/platform/virtual_keys.h`**：以 `constexpr KeyCode VK_BACK = 0x08;` 等镜像现用到的全部 `VK_*` 数值（这些值本就是 Windows 固定常量）。`key_names.cpp`、`keyrecord_db_tests.cpp` 改 include 此头。**好处**：VK 数值成为三平台后端翻译表共同引用的唯一事实源。
3. **`localtime_s` → 封装**：`platform_util` 里
   ```cpp
   inline void localTime(std::tm& out, std::time_t t) {
   #ifdef _WIN32
       localtime_s(&out, &t);
   #else
       localtime_r(&t, &out);
   #endif
   }
   ```
4. **`sprintf_s` → `snprintf`**：`key_event_writer.cpp:249` 的日期格式化改成可移植 `std::snprintf(date, sizeof date, "%04d-%02d-%02d", ...)`。
5. **`OutputDebugStringA` → 日志 sink**：Windows 用 `OutputDebugStringA`，其余 `#else` 走 `fputs(stderr)`。

**验收**：完成 Phase 0 后 Windows 端 `build.ps1` 编译 + 全部 CTest 仍通过，行为零变化。此阶段独立可交付、风险最低，应最先合入。

---

## 5. 构建系统改造

### 5.1 CMakeLists.txt 分平台守卫

- 顶部 toolchain 兜底改为：
  ```cmake
  if(NOT CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
      set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
          CACHE STRING "Vcpkg toolchain file")
  endif()
  ```
- 所有 Windows 专属块包进 `if(WIN32)`：`UNICODE` 定义、`.rc`/图标/`configure_file`、`copy_sqlite_dll`、`ws2_32`、`shell32`、`WIN32` 可执行标志。
- 采集端源文件按平台选择：
  ```cmake
  if(WIN32)
      set(KEYRECORD_CAPTURE_PLATFORM_SRC platform/capture_backend_windows.cpp platform/tray_windows.cpp)
  elseif(APPLE)
      enable_language(OBJCXX)
      set(KEYRECORD_CAPTURE_PLATFORM_SRC platform/capture_backend_macos.mm platform/tray_macos.mm)
  elseif(UNIX)
      set(KEYRECORD_CAPTURE_PLATFORM_SRC platform/capture_backend_linux.cpp platform/tray_stub.cpp)
  endif()
  ```
- macOS 追加框架链接；Linux 追加 `pthread`（及可选 evdev）。
- SQLite 依赖：`unofficial-sqlite3`（vcpkg）三平台均可用；或改用 CMake 内置 `find_package(SQLite3)`。二选一，建议沿用 vcpkg 以保持一致。

### 5.2 vcpkg 清单模式（`vcpkg.json`）

根目录加清单，triplet 随平台自动选择（`x64-windows` / `x64-linux` / `arm64-osx`）：
```json
{
  "name": "keyrecord",
  "version-string": "0.1.0",
  "dependencies": ["sqlite3", "boost-asio", "boost-beast"]
}
```

### 5.3 统一入口：`CMakePresets.json`

项目已要求 CMake ≥ 3.20，直接用 presets 作为三平台统一配置/构建入口（IDE 友好，替代大部分脚本分支）：`windows-x64`、`linux-x64`、`macos-arm64` 三个 configurePreset + 对应 buildPreset。`build.ps1` 保留给 Windows 习惯用户；另可加薄 `build.sh` 包裹 presets。

### 5.4 跨平台打包（`PackageRelease.cmake`）

- 用 `cmake -E tar` 替换 PowerShell `Compress-Archive`：Windows 出 `.zip`（`cmake -E tar cf ... --format=zip`），Linux/macOS 出 `.tar.gz`。
- 包名参数化：`keyrecord-${os}-${arch}.<ext>`。
- Windows 才拷 SQLite DLL；Linux/macOS 处理 rpath 或静态链接 SQLite。

### 5.5 CI 矩阵（`.github/workflows`）

```yaml
strategy:
  matrix:
    include:
      - { os: windows-2022, triplet: x64-windows,  name: windows-x64 }
      - { os: ubuntu-latest, triplet: x64-linux,   name: linux-x64 }
      - { os: macos-14,      triplet: arm64-osx,   name: darwin-arm64 }
```
每个 job：装 vcpkg（清单模式自动解析）、Linux 额外 `apt-get install`（若用 libevdev/GTK）、按 preset 配置构建、跑 CTest、上传各自命名的 artifact。

---

## 6. 分阶段落地路线

| 阶段 | 内容 | 产出 / 验收 | 状态 |
| --- | --- | --- | --- |
| **Phase 0** | 共享代码去耦合（§4）：`key_code.h`、`virtual_keys.h`、localtime/snprintf/日志封装 | Windows 构建 + 全部 CTest 不变通过；零行为变化，可独立合入 | ✅ 已完成并通过 Windows 构建验证（见 §9） |
| **Phase 1** | 构建系统跨平台化（§5）：presets、`if(WIN32)` 守卫、`cmake -E tar`、CI 三平台矩阵 | 三平台服务端、采集端和可移植测试可构建 | 🚧 Windows 已验证；Linux/macOS CI 待跑（见 §9） |
| **Phase 2** | Linux 采集后端：`capture_backend_linux.cpp`（evdev）+ keycode→VK 表 + 信号收尾 + headless/systemd | Linux 上 `keyrecord` 采集入库，前端展示正常 | 🚧 代码完成；Linux 编译与运行验证待跑（见 §9） |
| **Phase 3** | macOS 采集后端：`capture_backend_macos.mm`（CGEventTap）+ keycode→VK 表 + Input Monitoring + NSStatusBar | macOS(arm64) 上采集入库，前端展示正常 | 🚧 代码完成；macOS 编译、授权与运行验证待跑（见 §9） |
| **Phase 4** | 收尾：三平台 README（构建/运行/权限说明）、三平台 release artifact、CI 全量上传 | 三平台产物齐全 | 🚧 文档与打包逻辑完成；Linux/macOS artifact 待 CI 验证（见 §9） |

---

## 7. 风险与注意事项

- **权限与安全（keylogger 性质）**：三平台授权模型不同——Windows 无需特权；Linux 需 `input` 组权限或 root；macOS 需 TCC 辅助功能/输入监控授权。README 必须显著说明，且这是「记录键盘输入」的应用，安全边界要谨慎。
- **VK 码契约不可破坏**：Linux/macOS 后端若翻译表有误，热力图、键盘分区、左右手统计会静默错位。翻译表需配单元测试（原生码→VK 的关键条目断言）。
- **Wayland**：选 evdev 而非 X11 XRecord，保证 Wayland 可用。
- **macOS OBJCXX / arm64**：`.mm` 需 `enable_language(OBJCXX)`；确认 vcpkg 的 boost 在 `arm64-osx` 可用，否则改用 Homebrew boost（`find_package(Boost)`）。
- **时区 / 线程安全**：`localtime_r`/`localtime_s` 均线程安全；保持本地时区语义与现状一致。
- **托盘可选化**：Linux 默认 headless，避免强依赖 GTK，降低移植面。

关键 API 依据：
- Linux Kernel Input 文档：https://docs.kernel.org/input/event-codes.html（`EV_KEY` 的 0/1/2 分别表示释放、按下、自动重复）。
- Apple `CGEventTapCreate`：https://developer.apple.com/documentation/coregraphics/cgevent/tapcreate(tap:place:options:eventsofinterest:callback:userinfo:)
- Apple Input Monitoring 预检/请求：https://developer.apple.com/documentation/coregraphics/cgpreflightlisteneventaccess() 、https://developer.apple.com/documentation/coregraphics/cgrequestlisteneventaccess()

---

## 8. 改动文件速查

**新增**：`src/platform/{key_code.h, virtual_keys.h, platform_util.h/.cpp, capture_backend.h, capture_backend_windows.cpp, capture_backend_linux.cpp, capture_backend_macos.mm, linux_keymap.h, macos_keymap.h, tray.h, tray_windows.cpp, tray_macos.mm, tray_stub.cpp}`、`CMakePresets.json`、`packaging/keyrecord.service`（systemd）。

**修改**：`CMakeLists.txt`（平台守卫 + 源文件选择 + 打包名）、`cmake/PackageRelease.cmake`（`cmake -E tar`）、`.github/workflows/cmake-single-platform.yml`（矩阵）、`README.md`、`key_names.h/.cpp`、`key_event_writer.h/.cpp`、`main.cpp`、`tray_app.h/.cpp`、`src/tests/keyrecord_db_tests.cpp`。

**拆分**：`tray_app.cpp/.h` 仅保留跨平台生命周期协调；Windows Hook/托盘分别迁移到 `capture_backend_windows.cpp` 与 `tray_windows.cpp`。

**保持不变（重要）**：`key_classification.*`、`keyboard_layout.*`、`api_queries.*`、`readonly_database.*`、`http_router.*`、`visualization_service.*`、`server*.cpp`、`app_config.*`、`config_path.*`、`embedded_resources.*` 及前端 `visualize/public/**`。

---

## 9. 实施进度记录

### Phase 0 — 共享代码去 Windows 化（2026-07-02，✅ 代码完成）

**新增文件**
- `src/platform/key_code.h`：`using KeyCode = std::uint32_t;`（取代共享头里的 `DWORD`）。
- `src/platform/virtual_keys.h`：`keyrecord::vk::` 命名空间常量，逐一镜像原 `VK_*` 数值；用命名空间常量而非宏，避免与 `<windows.h>` 宏冲突。
- `src/platform/platform_util.{h,cpp}`：`localTime()` 封装 `localtime_s`/`localtime_r`；`debugLog()` 封装 `OutputDebugStringA`/`stderr`。

**修改文件**
- `src/key_names.h`：删除 `<windows.h>`，改含 `platform/key_code.h`；`getKeyName(DWORD)` → `getKeyName(keyrecord::KeyCode)`。
- `src/key_names.cpp`：改含 `platform/virtual_keys.h`；`std::map<DWORD,...>` → `std::map<keyrecord::KeyCode,...>`；全部 `VK_*` → `vk::*`。
- `src/key_event_writer.h`：删除 `<windows.h>`，改含 `platform/key_code.h`；`enqueueKeyEvent(DWORD,...)` → `KeyCode`。
- `src/key_event_writer.cpp`：`KeyEvent::vkCode`/`updateSummaries` 形参 `DWORD` → `KeyCode`；`logError` 改用 `debugLog`；`tm`+`localtime_s` → `std::tm`+`keyrecord::localTime`；`sprintf_s` → `std::snprintf`；新增 `<cstdio>`。
- `src/tests/keyrecord_db_tests.cpp`：`VK_OEM_7` → `keyrecord::vk::Oem7`。
- `CMakeLists.txt`：`platform/platform_util.cpp` 加入 `keyrecord_capture`。
- `src/tests/virtual_keys_tests.cpp`（新增，注册为 CTest 用例 `virtual_keys_tests`）：用 `static_assert` 锁定关键 `vk::*` 数值契约；仅依赖 `virtual_keys.h`，三平台均可编译，作为跨平台后端归一化的回归护栏。

**验证**
- 静态：共享代码与 `tray_app.*` 已无 `DWORD`/`VK_*`/`sprintf_s`/`localtime_s`/`OutputDebugString`/`windows.h` 泄漏；Win32 API 仅保留在 `main.cpp` 与 `src/platform/*_windows.cpp`。
- 动态（2026-07-14）：复用现有 Visual Studio 18 2026 构建目录执行 `build.ps1`，Release 包 `keyrecord-windows-x64.zip` 生成成功，Debug CTest 16/16 通过。

### Phase 1 — 构建系统跨平台化（2026-07-02，✅ 代码完成）

**修改 / 新增**
- `CMakeLists.txt`：
  - 工具链改为「显式 `CMAKE_TOOLCHAIN_FILE` > `VCPKG_ROOT` 环境变量 > 默认 Windows 路径」三级回退。
  - 新增目标平台命名（`KEYRECORD_OS_NAME`/`KEYRECORD_ARCH_NAME`/`KEYRECORD_PACKAGE_BASENAME`）。
  - SQLite 依赖：`unofficial-sqlite3`（vcpkg）优先，缺失时回退 `find_package(SQLite3)`；抽象为 `KEYRECORD_SQLITE_TARGET`。
  - 新增 `find_package(Threads)`（Linux `std::thread`/Asio 需要 pthread）。
  - Boost `find_path` 增加 Homebrew / `/usr` HINTS。
  - `UNICODE` 定义、`.ico`/`.rc`/`configure_file`、采集端 `keyrecord` 可执行目标、`shell32` 全部包进 `if(WIN32)`。
  - `copy_sqlite_dll` 函数体加 `if(WIN32 AND KEYRECORD_SQLITE_FROM_VCPKG)` 守卫（非 Windows 变空操作）。
  - `keyrecord_server` 的 `ws2_32` 改为仅 `if(WIN32)` 链接，并补链 `Threads::Threads`。
  - 发布包按平台命名与格式（Windows `.zip`/其它 `.tar.gz`），并以「采集端优先、否则服务端」为锚定目标推导运行目录与依赖。
  - UTF-8 目标列表对 Windows-only 的 `keyrecord` 目标做 `if(TARGET keyrecord)` 保护。
- `cmake/PackageRelease.cmake`：改用 `cmake -E tar`（`--format=zip` / `czf`）替换 PowerShell `Compress-Archive`；`OUTPUT_ZIP` → `OUTPUT_ARCHIVE` + `ARCHIVE_FORMAT`；`KEYRECORD_EXE_PATH` 变为可选，缺失时跳过。
- `CMakePresets.json`（新增）：三平台统一入口——`windows-x64`（VS 2022）、`linux-x64` / `macos-arm64`（Ninja Multi-Config），含 build/test presets。
- `.github/workflows/cmake-single-platform.yml`：改为 `{windows-2022, ubuntu-latest, macos-14}` 三平台矩阵；Windows 用 classic vcpkg + `build.ps1`，Linux 用 apt（`libsqlite3-dev`/`libboost-dev`/`ninja-build`），macOS 用 brew（`sqlite`/`boost`/`ninja`）；按平台校验并上传对应命名的归档。

**与原方案的偏差（重要）**
- 原 §5.2 建议根目录加 `vcpkg.json` 清单模式；实施时**暂不引入**。原因：一旦仓库根存在 `vcpkg.json` 且启用 vcpkg 工具链，vcpkg 会切换到 manifest 模式并在 build 目录重新编译 Boost，破坏现有本地/CI 的 classic-mode Windows 构建（也拖慢速度）。改为让 CMake 依赖发现**同时兼容** classic vcpkg（Windows）与系统包（Linux apt / macOS brew），CI 分平台安装依赖。待需要可复现依赖锁定时再引入 `vcpkg.json`（届时用 `VCPKG_MANIFEST_MODE` 或独立 preset 隔离）。
- Phase 1 初始提交仅构建 Linux/macOS 服务端；2026-07-14 接入 Phase 2/3 后，`keyrecord` 采集目标已扩展到三平台。

**验证**
- 静态：`CMakeLists.txt` 逐行走查 Windows 路径，确认工具链、SQLite 目标、`keyrecord-windows-x64.zip` 包名、`keyrecord`/服务端目标与打包依赖均与改造前一致。
- 动态（2026-07-14）：Windows Release 打包与 Debug CTest 16/16 通过；Linux/macOS CI 尚未触发，不据此宣称目标平台已验证。

### Phase 2 — Linux 采集后端（2026-07-14，🚧 代码完成，待目标机验证）

已完成的 build-neutral、可跨平台验证部分（不触碰 Windows 采集构建路径）：
- `src/platform/capture_backend.h`（新增）：采集端平台抽象接口 `runCaptureLoop(callback)` / `requestStopCapture()`；回调携带的键码约定为「已归一化的 Windows VK 数值」。各平台后端拥有各自事件循环。
- `src/platform/linux_keymap.h`（新增）：`constexpr KeyCode linuxEvdevToVk(int)`，把 evdev `KEY_*` 全量映射到 Windows VK（返回 0 表示未映射）。纯数值、零 Linux 头依赖，可在任意平台编译测试。这是 Linux 后端最核心、最易错的交付物。
- `src/tests/linux_keymap_tests.cpp`（新增，CTest 用例 `linux_keymap_tests`）：`static_assert` 锁定字母/数字/控制/修饰/符号/导航/小键盘及未映射的代表性条目。

2026-07-14 完成：
- `src/platform/capture_backend_linux.cpp`：枚举并筛选键盘 `/dev/input/event*`，使用 `poll` 读取 `EV_KEY`，处理按下与自动重复，归一化后携带 evdev 事件时间回调。
- `SIGINT`/`SIGTERM` 使用 lock-free `atomic_flag` 请求停止；设备读错误或断开时关闭对应 fd，全部设备断开则 fail-fast。
- Windows 原实现已拆为 `capture_backend_windows.cpp` + `tray_windows.cpp`，并通过 Windows 构建验证。
- Linux 使用 `tray_stub.cpp` headless 运行；新增 `packaging/keyrecord.service`，并随 Linux release 包分发。
- CMake 在 Linux 构建 `keyrecord` 采集端并与 `keyrecord_server` 一起打包。

**验证**
- 静态：`linux_keymap.h` 映射逐条对照 `linux/input-event-codes.h` 与 `virtual_keys.h`；`linux_keymap_tests` 覆盖关键条目。
- 动态：Windows 上 `virtual_keys_tests` / `linux_keymap_tests` 已通过；本机无 WSL/Linux 环境，evdev 后端的 Linux 编译、权限与真实输入验证仍待 CI/目标机完成。

### Phase 3 — macOS 采集后端（2026-07-14，🚧 代码完成，待目标机验证）

已完成的 build-neutral、可跨平台验证部分：
- `src/platform/macos_keymap.h`（新增）：`constexpr KeyCode macVirtualKeyToVk(int)`，把 macOS 虚拟键码（Carbon `kVK_*`）全量映射到 Windows VK；归一化约定 Command→Win、Option→Alt，主键区 Delete→退格、ForwardDelete→删除。纯数值、零 macOS 头依赖。
- `src/tests/macos_keymap_tests.cpp`（新增，CTest 用例 `macos_keymap_tests`）：`static_assert` 锁定字母/数字/控制/修饰/符号/导航/小键盘/功能键及未映射的代表性条目。

2026-07-14 完成：
- `src/platform/capture_backend_macos.mm`：使用 listen-only `CGEventTap` 监听 `kCGEventKeyDown`，在 AppKit 主事件循环中派发事件，并在 Tap 超时/用户输入禁用后重新启用。
- 使用权限范围更窄且与监听行为对应的 `CGPreflightListenEventAccess` / `CGRequestListenEventAccess` 请求“输入监控”，不额外申请辅助功能权限。
- `SIGINT`/`SIGTERM` 由 CFRunLoop timer 安全转换为主线程停止；状态栏退出通过 `requestStopCapture()` 停止事件循环。
- `tray_macos.mm` 使用 `NSStatusBar` 与系统键盘符号；CMake 启用 OBJCXX、ARC，并链接 `ApplicationServices`/`AppKit`。

**验证**
- 静态：`macos_keymap.h` 逐条对照 Carbon `Events.h` 与 `virtual_keys.h`；`macos_keymap_tests` 覆盖关键条目。
- 动态：Windows 上 `macos_keymap_tests` 已通过；当前无 macOS arm64 环境，Objective-C++ 编译、TCC 授权、状态栏菜单和真实输入仍待 CI/目标机验证。

### Phase 4 — 收尾（2026-07-14，🚧 目标平台验证待完成）

已完成：
- `README.md`：同步三平台能力、依赖、presets、运行/停止方式、Linux `input` 组权限与 systemd user service、macOS Input Monitoring 授权。
- 发布包脚本会在 Linux artifact 中加入 `keyrecord.service`；三平台包均以采集端为锚点，同时包含可用的 `keyrecord_server`。
- CI 已配置 Windows/Linux/macOS 构建、测试、归档校验与 artifact 上传。

### 待办：目标平台验证

1. Linux CI：确认 `keyrecord`、`keyrecord_server`、16 个测试与 `keyrecord-linux-x64.tar.gz`；在有 `input` 权限的 Linux 目标机确认真实按键写入与 `SIGTERM` 收尾。
2. macOS CI：确认 Objective-C++/ARC、系统框架链接、16 个测试与 `keyrecord-darwin-arm64.tar.gz`；在 arm64 目标机确认 Input Monitoring 授权、状态栏退出和真实按键写入。
3. 只有上述目标平台证据均通过后，才能把 Phase 1/2/3/4 标记为完成。
