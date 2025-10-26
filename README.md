# HyperRecall

HyperRecall is a desktop spaced repetition study application built with C17, raylib 5.x, raygui, and SQLite3. This repository currently provides the scaffolding for the core systems, build configuration, and tooling required to implement the full study experience.

## Quick Start

1. Install the raylib, raygui, and SQLite3 development packages for your platform (see [Build Prerequisites](#build-prerequisites)).
2. Generate a build directory with CMake:
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   ```
3. Compile and launch HyperRecall in a single step:
   ```bash
   cmake --build build --target run
   ```
4. Tweak runtime behaviour in `~/.config/hyperrecall/settings.ini` (created on first launch) to toggle analytics capture or adjust autosave cadence.

## Project Goals

* Deliver a fast, focused study workflow using a modern native UI powered by raylib/raygui.
* Synchronize decks, media, and review history through a reliable SQLite-backed data model.
* Provide deep analytics and developer tooling to iterate on the learning experience.

## Directory Layout

```
/ (repository root)
├── assets/            # Runtime assets such as fonts, icons, and theme definitions
├── external/          # Third-party libraries bundled with the project (raygui placeholder)
├── scripts/           # Environment setup helpers for Linux and Windows
├── src/               # Application source code organized by subsystem
├── .github/workflows/ # Continuous integration configuration
├── CMakeLists.txt     # Primary CMake build script
├── LICENSE            # MIT license
└── README.md          # Project documentation
```

Each source module contains TODO notes describing its future responsibilities to guide development.

## Build Prerequisites

HyperRecall depends on the following libraries:

* **raylib 5.x** — graphics, input, and audio runtime.
* **raygui** — immediate-mode GUI toolkit (single-header integration supported).
* **SQLite3** — persistent storage for decks, sessions, and analytics.

### Environment Variables

The build can be guided with these optional environment variables:

* `RAYLIB_ROOT` — points to the installation prefix for raylib.
* `RAYGUI_ROOT` — points to the installation prefix containing `raygui.h` when using a system copy.
* `SQLITE3_ROOT` — points to a custom SQLite3 installation.
* `HYPERRECALL_RAYGUI_HEADER` — overrides the bundled raygui single-header path when `HYPERRECALL_USE_SYSTEM_RAYGUI=OFF`.

### CMake Options

* `HYPERRECALL_ENABLE_DEVTOOLS` (default: `ON`) — toggles developer-only instrumentation and overlays.
* `HYPERRECALL_USE_SYSTEM_RAYGUI` (default: `OFF`) — uses a system-provided `raygui.h` when enabled; otherwise the project expects a bundled header at `external/raygui/raygui.h`.

## Building HyperRecall

These instructions assume raylib, raygui, and SQLite3 development packages are installed. The commands mirror the steps in the [Quick Start](#quick-start) section and can be adapted for alternate generators or installation layouts.

### Linux (Ubuntu/Debian example)

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config libraylib-dev libsqlite3-dev
# The repository bundles external/raygui/raygui.h. Set -DHYPERRECALL_USE_SYSTEM_RAYGUI=ON to rely on
# a system-provided header when available.
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cmake --build build --target run
```

For Release builds with link-time optimization (LTO):

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

### Windows (PowerShell with vcpkg)

```powershell
# Optional: install Ninja if not already available
choco install ninja -y
# Install raylib 5.x and sqlite3 via vcpkg or your preferred package manager. Enable
# -DHYPERRECALL_USE_SYSTEM_RAYGUI=ON when raygui is provided by the toolchain; otherwise the bundled
# header at external/raygui/raygui.h will be used.
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug `
      -DCMAKE_TOOLCHAIN_FILE=$Env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake
cmake --build build
cmake --build build --target run
```

> **Note:** On MinGW environments, ensure `RAYLIB_ROOT` and `SQLITE3_ROOT` point to directories containing `include/` and `lib/` folders with the appropriate binaries (`raylib`, `opengl32`, `gdi32`, `winmm`, and `sqlite3`). Pass `-DHYPERRECALL_USE_SYSTEM_RAYGUI=ON` when raygui ships with your toolchain; otherwise the bundled header at `external/raygui/raygui.h` is used automatically. Toggle the developer overlay on any platform with `-DHYPERRECALL_ENABLE_DEVTOOLS=OFF` when preparing production builds.

### Developer Tooling

Run the provided scripts to install dependencies or review guidance:

* `scripts/setup_env.sh` — Linux helper for package installation tips.
* `scripts/setup_env.ps1` — Windows helper for Chocolatey/vcpkg guidance.

## Running HyperRecall

After a successful build the executable is located at `build/hyperrecall`. The default configuration is generated on first launch and stored alongside other workspace data inside the platform-specific configuration directory (for example `~/.config/hyperrecall/` on Linux). Key options include:

* `analytics.enabled` — disables event capture and dashboard aggregation when set to `false`.
* `workspace_autosave_minutes` — configures the cadence for timed backups and on-review autosave snapshots.

Autosave events create JSON snapshots for each reviewed card under the configured autosave directory and schedule periodic `db_create_backup` runs. Both successes and failures surface as toast notifications in the UI to keep the learner informed.

## Continuous Integration

The GitHub Actions workflow builds Debug and Release configurations on Ubuntu and Release on Windows using Ninja. To reproduce locally, run:

```bash
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

Ensure the required dependencies are installed before executing the commands above.

## Feature Checklist

* [x] Study session loop with toast notifications for review feedback and system events.
* [x] Analytics dashboards powered by aggregated review, streak, and retention metrics.
* [x] Configurable autosave pipeline combining per-review snapshots with timed database backups.
* [ ] Deck import/export workflows and advanced topic management tools.
* [ ] Cloud synchronization and collaborative study features.

## Roadmap

* [ ] [Implement the spaced repetition engine](https://github.com/HyperRecall/roadmap/issues/1)
* [ ] [Design the raygui-powered study UI](https://github.com/HyperRecall/roadmap/issues/2)
* [ ] [Add analytics dashboards and export workflows](https://github.com/HyperRecall/roadmap/issues/3)

## Screenshots

Binary assets are not tracked in this repository to keep the project friendly for
text-only contribution workflows. To capture your own analytics dashboard
preview, build and run HyperRecall, open the analytics panel, and take a
platform screenshot. We recommend storing any captures outside the repository
or in external issue/PR attachments.

