#!/usr/bin/env bash
set -euo pipefail

cat <<'MSG'
HyperRecall Environment Setup (Linux)
====================================
This helper script provides guidance for installing build prerequisites.
MSG

if [[ "${EUID}" -ne 0 ]]; then
    echo "TIP: Run with sudo when executing package manager commands." >&2
fi

if command -v apt >/dev/null 2>&1; then
    echo "Detected apt-based distribution. Installing dependencies..."
    sudo apt update
    sudo apt install -y build-essential cmake ninja-build pkg-config libraylib-dev libsqlite3-dev
    echo "raygui.h is bundled with the repository; use -DHYPERRECALL_USE_SYSTEM_RAYGUI=ON to pick up a system header."
elif command -v dnf >/dev/null 2>&1; then
    echo "Detected dnf-based distribution. Installing dependencies..."
    sudo dnf install -y @development-tools cmake ninja-build pkgconf-pkg-config raylib-devel sqlite-devel
    echo "Use -DHYPERRECALL_USE_SYSTEM_RAYGUI=ON when a packaged raygui header is available; otherwise the bundled header is used."
elif command -v pacman >/dev/null 2>&1; then
    echo "Detected pacman-based distribution. Installing dependencies..."
    sudo pacman -Sy --needed base-devel cmake ninja raylib sqlite
    echo "The project ships raygui.h; set -DHYPERRECALL_USE_SYSTEM_RAYGUI=ON to use an installed copy instead."
else
    echo "Unsupported package manager. Install raylib 5.x and sqlite3 manually, then decide whether to use the bundled raygui.h or provide a system header via -DHYPERRECALL_USE_SYSTEM_RAYGUI=ON." >&2
fi

cat <<'NEXT'
Next steps:
1. Configure the project with cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
   (add -DHYPERRECALL_USE_SYSTEM_RAYGUI=ON if you prefer a system-provided raygui header).
2. Build with cmake --build build
3. Launch with cmake --build build --target run
NEXT
