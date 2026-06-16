# Visualize 静态资源目录

当前 `visualize/` 目录只保留会被 C++ 可视化服务内嵌的静态前端资源。

## 当前目录职责

- `public/`
  - 首页、样式、前端脚本和本地化的第三方资源。
- `scripts/`
  - 与前端资源相关的辅助脚本。

## 运行方式

请使用仓库根目录的 C++ 可视化服务，而不是旧的 Node.js 服务端：

```powershell
pwsh.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
.\build\Release\keyrecord_server.exe
```

默认访问地址为 `http://127.0.0.1:3000/`，默认读取 `%USERPROFILE%\.config\keyrecord\keyrecord.db`。

## 归档位置

旧的 Node.js 可视化服务端及其历史文档已移到：

```text
archive/node-server/visualize/
```
