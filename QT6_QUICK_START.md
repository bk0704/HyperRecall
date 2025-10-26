# Qt6 UI Backend - Quick Start Guide

## Overview

This guide will help you build and run the HyperRecall Qt6 UI prototype. The prototype includes three functional screens with placeholder data to demonstrate the UI structure.

## What's Included

The Qt6 prototype features:

- **Study Screen**: Session controls and SRS card review interface
- **Analytics Screen**: Dashboard with statistics and activity tracking
- **Library Screen**: Topic tree and card management interface
- **Navigation**: View menu with F1/F2/F3 shortcuts to switch screens
- **UI Elements**: Toast notifications, modal dialogs, status bar

## Prerequisites

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y cmake ninja-build build-essential qt6-base-dev libsqlite3-dev
```

### Linux (Fedora/RHEL)

```bash
sudo dnf install -y cmake ninja-build gcc gcc-c++ qt6-qtbase-devel sqlite-devel
```

### Linux (openSUSE)

```bash
sudo zypper install -y cmake ninja gcc gcc-c++ pkg-config qt6-base-devel sqlite3-devel
```

### Linux (Arch)

```bash
sudo pacman -S cmake ninja gcc qt6-base sqlite
```

### macOS (via Homebrew)

```bash
brew install cmake ninja qt@6 sqlite3
```

### Windows (via vcpkg)

```powershell
# Install vcpkg if not already installed
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install qtbase:x64-windows sqlite3:x64-windows
```

## Building the Qt6 Backend

### Linux/macOS

```bash
# Navigate to the repository
cd /path/to/HyperRecall

# Configure with Qt6 backend
cmake -S . -B build-qt -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DHYPERRECALL_UI_BACKEND=QT6

# Build
cmake --build build-qt

# Run
./build-qt/bin/hyperrecall
```

### Windows (Visual Studio)

```powershell
# Navigate to the repository
cd C:\path\to\HyperRecall

# Configure with Qt6 backend
cmake -S . -B build-qt -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
      -DHYPERRECALL_UI_BACKEND=QT6 `
      -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-qt --config Release

# Run
.\build-qt\bin\Release\hyperrecall.exe
```

### Windows (Ninja with MSVC)

```powershell
# Open "x64 Native Tools Command Prompt for VS 2022"

# Navigate to the repository
cd C:\path\to\HyperRecall

# Configure with Qt6 backend
cmake -S . -B build-qt -G Ninja `
      -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
      -DHYPERRECALL_UI_BACKEND=QT6 `
      -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-qt

# Run
.\build-qt\bin\hyperrecall.exe
```

## Using the Prototype

### Navigation

- **F1** or **View → Study**: Switch to Study screen
- **F2** or **View → Analytics**: Switch to Analytics screen
- **F3** or **View → Library**: Switch to Library screen
- **Ctrl+Q** or **File → Exit**: Quit application

### Study Screen

1. Click "Start Mastery Session" or "Start Cram Session" to begin
2. View the card content in the text area
3. Rate your response using one of four buttons:
   - **Again** (red): Didn't remember
   - **Hard** (orange): Remembered with difficulty
   - **Good** (green): Remembered correctly
   - **Easy** (bright green): Remembered easily

### Analytics Screen

View study statistics:
- **Total Reviews**: Number of cards reviewed
- **Average Ease**: Average difficulty rating
- **Current Streak**: Consecutive study days
- **Recent Activity**: Table of recent study sessions

### Library Screen

Browse and manage content:
- **Left panel**: Topic hierarchy tree
- **Right panel**: Cards for selected topic
- **Buttons**: Add/edit/delete topics and cards

## Troubleshooting

### Qt6 not found

**Error**: `Could not find a package configuration file provided by "Qt6"`

**Solution**: Ensure Qt6 is installed and CMake can find it:

```bash
# Linux: Check Qt6 installation
dpkg -l | grep qt6   # Debian/Ubuntu
rpm -qa | grep qt6   # Fedora/RHEL

# Set Qt6_DIR if needed
export Qt6_DIR=/path/to/qt6/lib/cmake/Qt6
cmake -S . -B build-qt -DHYPERRECALL_UI_BACKEND=QT6
```

### SQLite3 not found

**Error**: `SQLite3 development files not found`

**Solution**: Install SQLite development package:

```bash
# Ubuntu/Debian
sudo apt install libsqlite3-dev

# Fedora
sudo dnf install sqlite-devel
```

### Build fails on Windows

**Error**: Build errors or linking failures

**Solutions**:
1. Ensure you're using the correct Visual Studio version (2022 recommended)
2. Use the "x64 Native Tools Command Prompt" for command-line builds
3. Check that vcpkg installed packages for x64-windows triplet
4. Verify CMAKE_TOOLCHAIN_FILE points to vcpkg cmake file

### Application crashes on startup

**Solution**: The application needs a valid database. On first run:

```bash
# Linux/macOS: Check permissions
ls -la ~/.local/share/HyperRecall/

# Windows: Check directory
dir %APPDATA%\HyperRecall\

# If issues persist, remove old database
rm -rf ~/.local/share/HyperRecall/  # Linux/macOS
rmdir /s %APPDATA%\HyperRecall\    # Windows
```

## Prototype Limitations

This is a functional prototype with **placeholder data**:

- Study sessions don't actually update the database
- Analytics shows sample statistics
- Library displays hardcoded topics and cards
- Cards are not editable yet
- Theme colors not fully applied

The prototype demonstrates:
- ✅ UI structure and navigation
- ✅ Screen layouts and widgets
- ✅ Menu system and shortcuts
- ✅ Toast notifications
- ✅ Modal dialogs
- ✅ Frame-based updates

## Verifying the Build

To confirm the Qt6 backend is working:

1. **Window opens** with "HyperRecall - Study" title
2. **Menu bar** shows File, View, and Help menus
3. **View menu** has checkmarks next to screen names
4. **F1/F2/F3 keys** switch between screens
5. **Status bar** shows frame counter and FPS
6. **About dialog** mentions "Qt6 UI Backend - Functional Prototype"

## Comparing with Raylib Backend

Build the Raylib backend for comparison:

```bash
# Configure with Raylib backend (default)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Or explicitly specify
cmake -S . -B build -DHYPERRECALL_UI_BACKEND=RAYLIB -DCMAKE_BUILD_TYPE=Release

# Build and run
cmake --build build
./build/bin/hyperrecall
```

Both backends share the same C core (database, SRS, analytics, sessions) but have different UIs.

## Next Steps

To extend the prototype:

1. **Connect to SessionManager**: Implement actual session logic in study_screen.cpp
2. **Load Analytics Data**: Pull real stats from AnalyticsHandle in analytics_screen.cpp
3. **Database Integration**: Load topics/cards from DatabaseHandle in library_screen.cpp
4. **Card Editor**: Create dialogs for adding/editing cards
5. **Theme Integration**: Apply theme colors to Qt widgets
6. **Command Palette**: Implement keyboard-driven command interface

See `src/qt/README.md` for technical details and development guidelines.

## Getting Help

If you encounter issues:

1. Check this guide's Troubleshooting section
2. Review the main `INSTALL.md` for general build instructions
3. Check `test-dual-backend.sh` output for CMake configuration issues
4. Verify dependencies are installed correctly

## Summary

**Quick build (Linux):**
```bash
sudo apt install qt6-base-dev libsqlite3-dev cmake ninja-build
cmake -S . -B build-qt -G Ninja -DHYPERRECALL_UI_BACKEND=QT6 -DCMAKE_BUILD_TYPE=Release
cmake --build build-qt
./build-qt/bin/hyperrecall
```

**Quick build (Windows with vcpkg):**
```powershell
vcpkg install qtbase:x64-windows sqlite3:x64-windows
cmake -S . -B build-qt -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DHYPERRECALL_UI_BACKEND=QT6
cmake --build build-qt --config Release
.\build-qt\bin\Release\hyperrecall.exe
```

The Qt6 prototype is now ready to run! Press F1/F2/F3 to explore the three screens.
