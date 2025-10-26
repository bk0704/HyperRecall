# HyperRecall Usage Guide

This guide covers all the ways to build, run, and develop HyperRecall.

## 🚀 Quick Run (Recommended)

The easiest way to get started:

### Linux / macOS
```bash
./run.sh
```

### Windows
```cmd
run.bat
```

These scripts automatically:
1. Check for required tools
2. Configure the build (first time only)
3. Build the application
4. Launch HyperRecall

## 📦 Using Make (Linux/macOS)

HyperRecall includes a Makefile for convenient development:

```bash
# Build and run in one command
make run

# Just build
make build

# Configure the build system
make configure

# Clean build artifacts
make clean

# Install dependencies (Ubuntu/Debian/Fedora/Arch)
make install-deps

# Show all available commands
make help
```

## 🔧 Development Mode

For development with debug symbols and dev tools:

```bash
./dev.sh
```

This builds with:
- Debug symbols enabled
- Developer tools enabled
- No optimizations (faster compile)
- Detailed error messages

## ⚡ Shell Aliases

For frequent developers, source the aliases:

```bash
source aliases.sh
```

Then use short commands:
```bash
hr              # Run HyperRecall
hr-build        # Build only
hr-dev          # Development build
hr-clean        # Clean build
hr-run          # Run without rebuilding
hr-rebuild      # Clean and rebuild
hr-logs         # View application logs
hr-data         # Open data directory
hr-config       # View config directory
```

## 🖥️ Desktop Launcher (Linux)

Install a launcher for your application menu:

```bash
./install-launcher.sh
```

After installation, launch HyperRecall from:
- Application menu → Education → HyperRecall
- GNOME Activities / KDE Krunner: Search "HyperRecall"
- Desktop file at: `~/.local/share/applications/hyperrecall.desktop`

To uninstall:
```bash
rm ~/.local/share/applications/hyperrecall.desktop
```

## 🛠️ Manual Build (Advanced)

If you prefer full control:

### Configure (First Time)
```bash
# With Ninja (recommended)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# With Make
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Debug build with dev tools
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DHYPERRECALL_ENABLE_DEVTOOLS=ON
```

### Build
```bash
cmake --build build

# Or with parallel jobs
cmake --build build -j $(nproc)
```

### Run
```bash
# Linux/macOS
./build/bin/hyperrecall

# Windows
.\build\bin\hyperrecall.exe
```

## 🔍 Check Setup

Verify your environment is ready:

```bash
./check-setup.sh
```

This checks:
- Build tools (cmake, ninja/make, gcc/clang)
- Libraries (raylib, sqlite3)
- Source files
- Scripts
- Build status

## 📋 Build Options

Customize your build with CMake options:

```bash
# Enable/disable developer tools
cmake -S . -B build -DHYPERRECALL_ENABLE_DEVTOOLS=ON

# Use system raygui instead of bundled
cmake -S . -B build -DHYPERRECALL_USE_SYSTEM_RAYGUI=ON

# Specify raylib location
cmake -S . -B build -DRAYLIB_ROOT=/path/to/raylib

# Specify SQLite3 location
cmake -S . -B build -DSQLITE3_ROOT=/path/to/sqlite3
```

## 🌐 Cross-Platform Notes

### Linux
- Default data directory: `~/.local/share/HyperRecall/`
- Desktop launcher: `~/.local/share/applications/hyperrecall.desktop`
- Requires X11 or Wayland for GUI

### Windows
- Default data directory: `%APPDATA%\HyperRecall\`
- Use `run.bat` for Command Prompt
- Use `run.ps1` for PowerShell
- May need to enable script execution: `Set-ExecutionPolicy RemoteSigned -Scope CurrentUser`

### macOS
- Default data directory: `~/Library/Application Support/HyperRecall/`
- Use `./run.sh` to build and run
- May need to install Xcode Command Line Tools

## 🚨 Troubleshooting

### Build fails with "cmake not found"
```bash
# Ubuntu/Debian
sudo apt install cmake

# macOS
brew install cmake

# Windows
# Download from https://cmake.org/download/
```

### Build fails with "raylib not found"
```bash
# Ubuntu/Debian
sudo apt install libraylib-dev

# Or build from source (see README.md)
```

### "Permission denied" on scripts
```bash
chmod +x run.sh dev.sh install-launcher.sh check-setup.sh
```

### Clean build not working
```bash
# Remove build directory completely
rm -rf build

# Reconfigure and build
./run.sh
```

### Display errors on Linux
Ensure you have X11 or Wayland:
```bash
echo $DISPLAY  # Should show :0 or similar
```

## 📊 Build Performance

For faster builds:

1. **Use Ninja** instead of Make:
   ```bash
   cmake -S . -B build -G Ninja
   ```

2. **Parallel builds**:
   ```bash
   cmake --build build -j $(nproc)
   ```

3. **ccache** for incremental builds:
   ```bash
   sudo apt install ccache
   export CC="ccache gcc"
   export CXX="ccache g++"
   ```

## 🔄 Updating HyperRecall

To update to the latest version:

```bash
# Pull latest changes
git pull

# Rebuild (old build directory is reused)
./run.sh

# Or for a clean rebuild
make clean
make run
```

## 🎯 Recommended Workflows

### First-time user
```bash
./check-setup.sh          # Verify setup
make install-deps         # Install dependencies (if needed)
./run.sh                  # Build and run
```

### Daily use
```bash
./run.sh                  # Just run it!
```

### Developer
```bash
source aliases.sh         # Set up aliases (once per session)
hr-dev                    # Development build
hr                        # Run
# Make changes to code
hr-build                  # Rebuild
hr-run                    # Test
```

### Contributor
```bash
./dev.sh                  # Development build with tools
# Make changes
make build                # Rebuild
clang-format -i src/*.c   # Format code
git add .
git commit -m "Description"
```

## 📝 Environment Variables

Customize behavior with environment variables:

```bash
# Specify build type
export CMAKE_BUILD_TYPE=Debug

# Specify generator
export CMAKE_GENERATOR=Ninja

# Custom install prefix
export CMAKE_INSTALL_PREFIX=$HOME/.local

# Verbose build output
export VERBOSE=1
cmake --build build
```

## 🎓 Learn More

- **Quick Start**: See `QUICKSTART.md` for the fastest path to running HyperRecall
- **Full Documentation**: See `README.md` for comprehensive information
- **Development**: See `ROADMAP.md` for development status and plans
- **Testing**: See `ACCEPTANCE_TESTS.md` for testing details

---

**Need help?** Open an issue on GitHub or check existing documentation.
