# KeyRecord

English | [中文](README.md)

KeyRecord is a local, cross-platform keyboard-event recorder and visualization tool. It captures keyboard events on Windows 10/11, Linux, and macOS, stores them in SQLite, and serves a local web UI for usage statistics and keyboard heatmaps.

> This project records keyboard input. Run it only on devices and with data that you are authorized to use, and protect the database file appropriately.

## Features

- Native keyboard capture on Windows, Linux, and macOS
- Batched SQLite writes of timestamps, dates, hours, virtual-key codes, and key names
- Local read-only visualization service with daily statistics, key rankings, and keyboard heatmaps
- Static web assets embedded in `keyrecord_server`; no Node.js runtime is required
- Optional `config.ini` for the database location and server listen address and port
- Release packages and GitHub Actions coverage for Windows x64, Linux x64, and macOS arm64

## Platform support

| Platform | Capture mechanism | Resident mode |
| --- | --- | --- |
| Windows 10/11 | `WH_KEYBOARD_LL` | System tray |
| Linux | evdev (`/dev/input/event*`) | Headless / systemd user service |
| macOS | CGEventTap | Menu bar |

All capture backends normalize events to Windows virtual-key codes, keeping the SQLite data, statistics API, and keyboard layout consistent across platforms.

## Dependencies

- CMake 3.20+
- A C++20 compiler
- SQLite3
- Boost.Asio and Boost.Beast headers (required to build `keyrecord_server`)

```powershell
# Windows (vcpkg classic mode)
vcpkg install sqlite3:x64-windows boost-asio[core]:x64-windows boost-beast:x64-windows
```

```bash
# Linux (Debian/Ubuntu)
sudo apt-get install -y ninja-build libsqlite3-dev libboost-dev

# macOS
brew install ninja sqlite boost
```

## Build and test

On Windows, use the all-in-one script. It builds a Release package, then builds and runs Debug tests:

```powershell
pwsh.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Build the release package without tests:

```powershell
pwsh.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -SkipTests
```

If the Boost headers are outside the default search paths, pass their include root:

```powershell
pwsh.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -BoostIncludeDir C:\path\to\boost
```

You can also use CMake presets. This is a Linux example; replace `linux-x64` with `macos-arm64` on macOS:

```bash
cmake --preset linux-x64
cmake --build --preset linux-x64-release
cmake --build --preset linux-x64-tests
ctest --preset linux-x64
```

Windows presets:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64-release
cmake --build --preset windows-x64-tests
ctest --preset windows-x64
```

Release packages are created in `build/`: `keyrecord-windows-x64.zip`, `keyrecord-linux-x64.tar.gz`, and `keyrecord-darwin-arm64.tar.gz`.

## Run

Start the capture process:

```powershell
# Windows
.\build\Release\keyrecord.exe
```

```bash
# Linux / macOS
./build/Release/keyrecord
```

On Windows, the process hides in the system tray. On Linux, press `Ctrl+C` to stop a foreground process. On macOS, quit from the menu-bar keyboard icon. The first macOS run requires Input Monitoring permission in **System Settings > Privacy & Security > Input Monitoring**, then a restart of the process.

Start the visualization service:

```powershell
.\build\Release\keyrecord_server.exe
```

```bash
./build/Release/keyrecord_server
```

The default listen address is `http://0.0.0.0:3000/`. Its web assets are embedded in the executable, so changes under `visualize/public/` require a rebuild.

The Linux capture process needs read access to `/dev/input/event*`. A common setup is to add the running user to the `input` group and sign in again:

```bash
sudo usermod -aG input "$USER"
```

The repository includes `packaging/keyrecord.service` for a systemd user service:

```bash
install -Dm755 build/Release/keyrecord ~/.local/bin/keyrecord
install -Dm644 packaging/keyrecord.service ~/.config/systemd/user/keyrecord.service
systemctl --user daemon-reload
systemctl --user enable --now keyrecord.service
```

## Configuration

Both executables read `~/.config/keyrecord/config.ini` (`~` maps to `%USERPROFILE%` on Windows). Missing configuration falls back to built-in defaults. Precedence is: **built-in defaults < configuration file < command-line arguments**.

```ini
[server]
# Default: 0.0.0.0. Use 127.0.0.1 for local-only access.
address = 127.0.0.1
# Default: 3000. Valid range: 1..65535.
port = 3000

[storage]
# Full database path, shared by capture and server processes.
db_path = D:/data/keyrecord/keyrecord.db
# Or specify only a directory; the filename is keyrecord.db (db_path wins).
# db_dir = D:/data/keyrecord
```

See [resources/config.example.ini](resources/config.example.ini) for the full example. The default database path is `~/.config/keyrecord/keyrecord.db`.

## Project layout

```text
src/
  platform/                  # Platform capture, tray, and key mapping
  key_event_writer.*         # SQLite queue and batched persistence
  app_config.* config_path.* # Configuration parsing and path rules
  api_queries.*              # Visualization API queries
  keyboard_layout.*          # Keyboard heatmap coordinates
  key_classification.*       # Key classification
  readonly_database.*        # Read-only SQLite connection for visualization
  embedded_resources.*       # Embedded static-resource index
  http_router.*              # Static-resource and API routing
  visualization_service.*    # Visualization service runtime
  server_*.* server.*        # Server bootstrap, response adapter, and Boost.Beast layer
  tests/                     # C++ executable tests
visualize/public/            # Frontend assets embedded into the server
cmake/                       # CMake resource-generation and packaging scripts
packaging/                   # Platform packaging files
.github/workflows/           # Cross-platform build, test, and release workflow
archive/node-server/         # Retired Node.js server
```

## Database

```sql
CREATE TABLE keys (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER,
    date TEXT,
    hour INTEGER,
    vk_code INTEGER,
    key_name TEXT
);
```

Use [query.sql](query.sql) to query the data:

```powershell
sqlite3 "$env:USERPROFILE\.config\keyrecord\keyrecord.db" < query.sql
```

## Security and privacy

- Restrict access to `keyrecord.db` and back up or clean up historical data as needed.
- Windows does not require elevation, but it cannot capture input from processes running at a higher integrity level.
- Security software may flag a global keyboard capture process. Review it according to your organization's security policy before use.
