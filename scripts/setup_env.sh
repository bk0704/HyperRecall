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
    echo "TODO: Install raygui by copying raygui.h into external/raygui/ or by packaging it system-wide."
elif command -v dnf >/dev/null 2>&1; then
    echo "Detected dnf-based distribution. Installing dependencies..."
    sudo dnf install -y @development-tools cmake ninja-build pkgconf-pkg-config raylib-devel sqlite-devel
    echo "TODO: Provide the raygui single-header file and configure RAYGUI_ROOT if packaged separately."
elif command -v pacman >/dev/null 2>&1; then
    echo "Detected pacman-based distribution. Installing dependencies..."
    sudo pacman -Sy --needed base-devel cmake ninja raylib sqlite
    echo "TODO: Place raygui.h under external/raygui/ or install from the AUR if available."
else
    echo "Unsupported package manager. TODO: Document manual dependency installation steps for your distribution." >&2
fi

cat <<'NEXT'
Next steps:
1. Ensure raygui.h is available (see TODO messages above).
2. Configure the project with cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
3. Build with cmake --build build
NEXT
