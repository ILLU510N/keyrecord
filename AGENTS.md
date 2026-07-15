# Repository Guidelines

## Project Structure & Module Organization

This is a cross-platform C++20/CMake project for local keyboard-event capture and visualization. It supports Windows x64, Linux x64, and macOS arm64. Core source files live in `src/`. Capture-side code includes `tray_app.*`, `key_event_writer.*`, `config_path.*`, `app_config.*`, and `platform/` (native capture, tray/menu-bar integration, and key-code mapping). Visualization/server code includes `readonly_database.*`, `api_queries.*`, `key_classification.*`, `keyboard_layout.*`, `embedded_resources.*`, `http_router.*`, `visualization_service.*`, `server_bootstrap.*`, `server_startup.*`, `server_response_adapter.*`, `server.*`, and `server_main.cpp`. Unit-style executable tests live in `src/tests/`.

Static web assets are kept in `visualize/public/` and embedded during CMake generation. CMake helper scripts are in `cmake/`; platform packaging files are in `packaging/`; cross-platform CI and release automation are in `.github/workflows/`. Documentation belongs in `doc/`. `archive/node-server/visualize/` contains the retired Node.js visualization server; avoid changing it unless maintaining archived behavior.

## Build, Test, and Development Commands

- `pwsh.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File .\build.ps1` configures CMake, builds the Release `keyrecord_release_package`, produces `build/keyrecord-windows-x64.zip`, then builds `keyrecord_debug_tests` and runs Debug tests.
- `pwsh.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -SkipTests` performs a faster build without test execution.
- `cmake -B build` configures manually using the default vcpkg toolchain path.
- `cmake --build build --config Release --target keyrecord_release_package` builds the release package manually.
- `cmake --build build --config Debug --target keyrecord_debug_tests` builds the full Debug test set manually.
- `ctest --test-dir build -C Debug --output-on-failure` runs the configured test suite.
- `.\build\Release\keyrecord_server.exe` starts the visualization server at `http://127.0.0.1:3000/`.
- `cmake --preset linux-x64` / `macos-arm64`, followed by the corresponding build and test presets, builds and tests Unix targets. Packages are `keyrecord-linux-x64.tar.gz` and `keyrecord-darwin-arm64.tar.gz`.

Dependencies are CMake 3.20+, a C++20 compiler, SQLite3, and Boost.Asio/Beast headers for `keyrecord_server`. Windows uses MSVC and vcpkg; Linux and macOS use system package managers in CI.

## Coding Style & Naming Conventions

Use C++20 with UTF-8 source files. Follow the existing 4-space indentation, brace placement, and namespace style. Types use `PascalCase`; functions and local variables use `camelCase`; constants and CMake variables use descriptive uppercase names where already established. Keep public headers small and pair new modules as `name.h` / `name.cpp` in `src/`.

## 代码文案约束

- C++ 代码中的字符串字面量统一使用英文，包括测试代码中的断言文本、错误提示和期望字符串。
- 不修改现有中文注释；涉及文案调整时，仅替换字符串内容，不顺带翻译注释。

## Testing Guidelines

Add tests for core logic, boundary handling, SQLite behavior, routing, and startup paths. Tests are standalone C++ executables registered with CTest from `CMakeLists.txt`; name files as `src/tests/<module>_tests.cpp` and targets as `<module>_tests`. Prefer deterministic temporary databases and explicit expected output strings.

## Commit & Pull Request Guidelines

Recent history uses short, direct commit messages such as `front` and `Add CMake workflow for single platform builds`. Prefer concise imperative messages that name the changed area, for example `Add visualization startup tests`.

Pull requests should summarize behavior changes, list build/test commands run, mention dependency or packaging impacts, and include screenshots only for visible `visualize/public/` UI changes. Link related issues when applicable.

## Security & Configuration Tips

This application records keyboard input. Keep data paths, permissions, and logging changes narrow and explicit. Do not commit local databases, generated build directories, or packaged binaries unless the release process requires them.
