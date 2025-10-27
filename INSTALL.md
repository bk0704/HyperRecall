# Installation Guide

This guide provides comprehensive installation instructions for HyperRecall on different platforms.

## Recommended: Download Prebuilt Release

The easiest way to install HyperRecall is to download a prebuilt release:

**[Download Latest Release](https://github.com/bk0704/HyperRecall/releases/latest)**

- **Windows**: Download `HyperRecall-Setup-*.exe` and run the installer
- **Linux**: Download `HyperRecall-Linux-x64.tar.gz`, extract with `tar -xzf`, and run `./hyperrecall`

---

## Building from Source

If you prefer to build from source or need to customize the build, continue below.

## Quick Install

The fastest way to get started is to use the one-click scripts:

### Linux / macOS
```bash
./run.sh
```

### Windows
```powershell
.\run.ps1
```

These scripts will automatically install dependencies, configure, build, and run HyperRecall.

---

## Detailed Installation

### Table of Contents

- [Linux Installation](#linux-installation)
  - [Ubuntu/Debian](#ubuntudebian)
  - [Fedora/RHEL](#fedorarhel)
  - [Arch Linux](#arch-linux)
- [Windows Installation](#windows-installation)
  - [Using vcpkg](#using-vcpkg)
  - [Using MSYS2](#using-msys2)
- [macOS Installation](#macos-installation)
- [Building from Source](#building-from-source)
- [Troubleshooting](#troubleshooting)

---

## Linux Installation

### Ubuntu/Debian

#### Automated Installation
```bash
# Install dependencies
make install-deps

# Build and run
./run.sh
```

#### Manual Installation

1. **Install build tools**:
   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake ninja-build pkg-config
   ```

2. **Install libraries**:
   ```bash
   # SQLite3
   sudo apt install -y libsqlite3-dev
   
   # X11 dependencies
   sudo apt install -y libx11-dev libxrandr-dev libxinerama-dev \
                       libxcursor-dev libxi-dev libgl1-mesa-dev
   ```

3. **Install raylib**:
   ```bash
   # If available in your distribution
   sudo apt install -y libraylib-dev
   
   # Or build from source (recommended for latest version)
   git clone --depth 1 --branch 5.0 https://github.com/raysan5/raylib.git /tmp/raylib
   cd /tmp/raylib
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=/usr/local
   cmake --build build
   sudo cmake --install build
   ```

4. **Build HyperRecall**:
   ```bash
   cd HyperRecall
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ./build/bin/hyperrecall
   ```

#### Desktop Integration (Optional)
```bash
./install-launcher.sh
```

This creates a desktop entry for easy launching from your application menu.

---

### Fedora/RHEL

1. **Install dependencies**:
   ```bash
   sudo dnf install -y cmake ninja-build gcc sqlite-devel \
                       libX11-devel libXrandr-devel libXinerama-devel \
                       libXcursor-devel libXi-devel mesa-libGL-devel
   ```

2. **Install raylib** (build from source as shown above)

3. **Build HyperRecall** (same as Ubuntu)

---

### Arch Linux

1. **Install dependencies**:
   ```bash
   sudo pacman -S cmake ninja gcc sqlite libx11 libxrandr \
                  libxinerama libxcursor libxi mesa
   ```

2. **Install raylib**:
   ```bash
   sudo pacman -S raylib
   # Or build from source if you need version 5.0 specifically
   ```

3. **Build HyperRecall** (same as Ubuntu)

---

## Windows Installation

### Using vcpkg (Recommended)

1. **Install Visual Studio Build Tools or Visual Studio**:
   - Download from https://visualstudio.microsoft.com/downloads/
   - Install C++ build tools

2. **Install vcpkg**:
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git
   .\vcpkg\bootstrap-vcpkg.bat
   ```

3. **Install dependencies**:
   ```powershell
   .\vcpkg\vcpkg install raylib sqlite3 --triplet x64-windows
   ```

4. **Build HyperRecall**:
   ```powershell
   # Automated
   .\run.ps1
   
   # Or manually
   cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release `
         -DCMAKE_TOOLCHAIN_FILE=.\vcpkg\scripts\buildsystems\vcpkg.cmake
   cmake --build build
   .\build\bin\hyperrecall.exe
   ```

---

### Using MSYS2

1. **Install MSYS2**:
   - Download from https://www.msys2.org/
   - Run the installer

2. **Open MINGW64 terminal** and install dependencies:
   ```bash
   pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
             mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib \
             mingw-w64-x86_64-sqlite3
   ```

3. **Build HyperRecall**:
   ```bash
   cd HyperRecall
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ./build/bin/hyperrecall.exe
   ```

---

## macOS Installation

### Using Homebrew

1. **Install Homebrew** (if not already installed):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. **Install dependencies**:
   ```bash
   brew install cmake ninja raylib sqlite3
   ```

3. **Build HyperRecall**:
   ```bash
   cd HyperRecall
   ./run.sh
   ```

---

## Building from Source

### Prerequisites

Ensure you have:
- **CMake 3.21+**
- **C17-compatible compiler** (GCC 7+, Clang 5+, MSVC 2019+)
- **Ninja or Make**
- **raylib 5.x**
- **SQLite3 3.x**

### Build Steps

1. **Configure**:
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   ```

2. **Build**:
   ```bash
   cmake --build build
   ```

3. **Run**:
   ```bash
   ./build/bin/hyperrecall
   ```

### Build Options

- **Developer Tools**: `-DHYPERRECALL_ENABLE_DEVTOOLS=ON` (default: ON)
- **System raygui**: `-DHYPERRECALL_USE_SYSTEM_RAYGUI=ON` (default: OFF)
- **Build Type**: `-DCMAKE_BUILD_TYPE=Debug|Release` (default: Release)
- **UI Backend**: `-DHYPERRECALL_UI_BACKEND=RAYLIB|QT6` (default: RAYLIB)
- **raylib Path**: `-DRAYLIB_ROOT=/path/to/raylib`
- **SQLite Path**: `-DSQLITE3_ROOT=/path/to/sqlite3`

Example with options:
```bash
cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DHYPERRECALL_ENABLE_DEVTOOLS=ON \
      -DRAYLIB_ROOT=/usr/local
```

### Building with Qt6 UI Backend

HyperRecall supports two UI backends: the default Raylib backend and an alternative Qt6 backend. The Qt6 backend provides a more native desktop experience using Qt Widgets.

#### Prerequisites for Qt6 Backend

In addition to the base requirements, you need:
- **Qt6 6.x** (Widgets, Gui, Core components)
- **C++17-compatible compiler**

#### Installing Qt6 Dependencies

**Ubuntu/Debian**:
```bash
sudo apt install -y cmake ninja-build build-essential \
                    qt6-base-dev libsqlite3-dev
```

**openSUSE**:
```bash
sudo zypper install -y cmake ninja gcc gcc-c++ pkg-config \
                       qt6-base-devel sqlite3-devel
```

**Fedora**:
```bash
sudo dnf install -y cmake ninja-build gcc-c++ \
                    qt6-qtbase-devel sqlite-devel
```

**Arch Linux**:
```bash
sudo pacman -S cmake ninja gcc qt6-base sqlite
```

**macOS (via Homebrew)**:
```bash
brew install cmake ninja qt@6 sqlite3
```

**Windows (via vcpkg)**:
```powershell
vcpkg install qtbase:x64-windows sqlite3:x64-windows
```

#### Building with Qt6

**Linux/macOS**:
```bash
cmake -S . -B build-qt -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DHYPERRECALL_UI_BACKEND=QT6
cmake --build build-qt
./build-qt/bin/hyperrecall
```

**Windows (with vcpkg)**:
```powershell
cmake -S . -B build-qt -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
      -DHYPERRECALL_UI_BACKEND=QT6 `
      -DCMAKE_BUILD_TYPE=Release
cmake --build build-qt --config Release
.\build-qt\bin\Release\hyperrecall.exe
```

#### Qt6 Backend Features

The Qt6 backend provides:
- Native desktop window with standard menus (File, Help)
- Better integration with desktop environments
- More familiar UI toolkit for Qt developers
- Placeholder UI with core functionality initialized

**Note**: The Qt6 backend is currently in early development. Full feature parity with the Raylib backend is planned for future releases.

---

## Troubleshooting

### raylib Not Found

**Solution**: Install raylib or specify its location:
```bash
cmake -S . -B build -DRAYLIB_ROOT=/path/to/raylib
```

Or build from source:
```bash
git clone --depth 1 --branch 5.0 https://github.com/raysan5/raylib.git
cd raylib
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

### SQLite3 Not Found

**Solution**: Install SQLite development files:
```bash
# Ubuntu/Debian
sudo apt install libsqlite3-dev

# Fedora/RHEL
sudo dnf install sqlite-devel

# Arch Linux
sudo pacman -S sqlite

# Windows (vcpkg)
.\vcpkg\vcpkg install sqlite3
```

### X11 Libraries Not Found (Linux)

**Solution**: Install X11 development packages:
```bash
sudo apt install libx11-dev libxrandr-dev libxinerama-dev \
                 libxcursor-dev libxi-dev libgl1-mesa-dev
```

### CMake Version Too Old

**Solution**: Install a newer CMake version:
```bash
# Download from https://cmake.org/download/
# Or use pip
pip install --upgrade cmake
```

### Build Fails with Warnings as Errors

**Solution**: The project uses `-Werror` to catch issues. Fix the warnings or temporarily disable:
```bash
# Edit CMakeLists.txt and remove -Werror from compile options
# Or use a supported compiler version
```

### Display Error on Linux

If you see `GLFW: The DISPLAY environment variable is missing`:
- This is normal in headless/SSH environments
- Run on a system with a display server
- Or use Xvfb for headless testing:
  ```bash
  xvfb-run ./build/bin/hyperrecall
  ```

---

## Verifying Installation

Run the setup checker:
```bash
./check-setup.sh
```

This will verify:
- Build tools are installed
- Required libraries are available
- Source files are present
- Build status

---

## Uninstallation

HyperRecall stores data in:
- **Linux**: `~/.local/share/HyperRecall/`
- **Windows**: `%APPDATA%\HyperRecall\`
- **macOS**: `~/Library/Application Support/HyperRecall/`

To uninstall:
1. Delete the build directory: `rm -rf build/`
2. Remove data directory (optional, to keep your study data)
3. Remove desktop entry (Linux): `rm ~/.local/share/applications/hyperrecall.desktop`

---

## Getting Help

- **Documentation**: See [README.md](README.md), [QUICKSTART.md](QUICKSTART.md), [USAGE.md](USAGE.md)
- **Troubleshooting**: See [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
- **Issues**: Open an issue on GitHub
- **Community**: Join discussions on GitHub

---

## Next Steps

After installation:
1. **Quick Start**: See [QUICKSTART.md](QUICKSTART.md)
2. **Usage Guide**: See [USAGE.md](USAGE.md)
3. **Create Cards**: Start building your study deck!

---

**Happy Studying!**
