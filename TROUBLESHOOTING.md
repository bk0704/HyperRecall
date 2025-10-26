# HyperRecall Troubleshooting Guide

This guide helps you resolve common issues when building or running HyperRecall.

## 🔍 Quick Diagnostics

**First step**: Run the setup checker to identify issues:

```bash
./check-setup.sh
```

This will tell you exactly what's missing or misconfigured.

---

## Common Build Issues

### "cmake not found"

**Symptoms**: Error when running `./run.sh` or `make run`

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install cmake

# Fedora
sudo dnf install cmake

# Arch
sudo pacman -S cmake

# macOS
brew install cmake

# Windows
# Download installer from https://cmake.org/download/
```

### "ninja not found" or "make not found"

**Symptoms**: CMake fails to find a build tool

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install ninja-build

# Or use Make instead
sudo apt install build-essential

# The scripts will automatically detect which is available
```

### "raylib not found"

**Symptoms**: Build fails with raylib errors

**Solution Option 1** - Install from package manager:
```bash
# Ubuntu/Debian (if available in repos)
sudo apt install libraylib-dev
```

**Solution Option 2** - Build from source:
```bash
git clone --depth 1 --branch 5.0 https://github.com/raysan5/raylib.git /tmp/raylib
cd /tmp/raylib
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
cd -
```

**Solution Option 3** - Specify custom location:
```bash
cmake -S . -B build -DRAYLIB_ROOT=/path/to/raylib
```

### "SQLite3 not found"

**Symptoms**: Build fails with sqlite3 errors

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install libsqlite3-dev

# Fedora
sudo dnf install sqlite-devel

# Arch
sudo pacman -S sqlite

# macOS
brew install sqlite3
```

### "raygui.h not found"

**Symptoms**: Build fails with raygui errors

**Solution**: raygui should be bundled in `external/raygui/`. If missing:
```bash
# Ensure you have the full repository
git clone --recurse-submodules https://github.com/bk0704/HyperRecall.git

# Or if already cloned
cd external/raygui
# Download raygui.h manually from https://github.com/raysan5/raygui
```

### "Permission denied" on scripts

**Symptoms**: Cannot execute `./run.sh` or other scripts

**Solution**:
```bash
chmod +x run.sh dev.sh check-setup.sh install-launcher.sh
```

### Build fails with "undefined reference"

**Symptoms**: Linker errors during build

**Solution**: Usually means missing dependencies
```bash
# Clean and reconfigure
rm -rf build
./run.sh
```

---

## Runtime Issues

### "Display not found" or "Failed to create window"

**Symptoms**: Application crashes on startup

**Solutions**:

**Linux** - Ensure X11 or Wayland is running:
```bash
echo $DISPLAY  # Should output something like :0
```

If empty, you're likely on a headless system or SSH session without display forwarding.

**SSH with X11 forwarding**:
```bash
ssh -X user@host
# Then run application
```

**Use a display**:
```bash
export DISPLAY=:0
./build/bin/hyperrecall
```

### "Cannot open database"

**Symptoms**: Error about database access

**Solution**:
```bash
# Check data directory exists and is writable
ls -la ~/.local/share/HyperRecall/

# If missing, it should be created automatically
# If permissions issue:
chmod 700 ~/.local/share/HyperRecall/
chmod 644 ~/.local/share/HyperRecall/*.db
```

### Application crashes immediately

**Symptoms**: Starts then closes

**Solutions**:

1. **Run from terminal to see errors**:
   ```bash
   ./build/bin/hyperrecall
   ```

2. **Try debug build**:
   ```bash
   ./dev.sh
   ./build/bin/hyperrecall
   ```

3. **Check logs**:
   ```bash
   ls ~/.local/share/HyperRecall/logs/
   cat ~/.local/share/HyperRecall/logs/*.log
   ```

### Fonts or icons not showing

**Symptoms**: Missing text or icons in UI

**Solution**: Assets should be copied automatically. Verify:
```bash
ls -la build/bin/assets/
```

If missing:
```bash
# Rebuild to copy assets
cmake --build build
```

---

## Windows-Specific Issues

### "vcpkg not found"

**Symptoms**: Can't install dependencies on Windows

**Solution**:
```powershell
# Install vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install raylib sqlite3 --triplet x64-windows
```

### "Script execution disabled"

**Symptoms**: Cannot run PowerShell scripts

**Solution**:
```powershell
# Enable script execution (run PowerShell as Administrator)
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### "MSVC not found" or "Compiler not found"

**Symptoms**: Build fails to find compiler

**Solution**:
```powershell
# Install Visual Studio Build Tools
# Download from: https://visualstudio.microsoft.com/downloads/

# Or use MinGW
# Download from: https://www.mingw-w64.org/
```

---

## Development Issues

### Changes not reflected after rebuild

**Symptoms**: Code changes don't appear to take effect

**Solution**:
```bash
# Force clean rebuild
make clean
make build

# Or
rm -rf build
./run.sh
```

### Slow builds

**Solutions**:

1. **Use Ninja** instead of Make:
   ```bash
   cmake -S . -B build -G Ninja
   ```

2. **Parallel builds**:
   ```bash
   cmake --build build -j $(nproc)
   ```

3. **Use ccache**:
   ```bash
   sudo apt install ccache
   export CC="ccache gcc"
   export CXX="ccache g++"
   cmake -S . -B build
   ```

### CMake cache issues

**Symptoms**: Configuration changes not taking effect

**Solution**:
```bash
# Remove CMake cache
rm -rf build/CMakeCache.txt build/CMakeFiles

# Or full clean
rm -rf build
cmake -S . -B build
```

---

## Installation Issues

### "make install-deps" fails

**Symptoms**: Dependency installation script errors

**Solution**: Install manually based on your system:

**Ubuntu/Debian**:
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build pkg-config \
                 libsqlite3-dev libraylib-dev
```

**Fedora**:
```bash
sudo dnf install @development-tools cmake ninja-build pkgconf-pkg-config \
                 sqlite-devel raylib-devel
```

**Arch**:
```bash
sudo pacman -S base-devel cmake ninja sqlite raylib
```

### Desktop launcher not appearing

**Symptoms**: After running `install-launcher.sh`, icon not in menu

**Solution**:
```bash
# Verify file was created
ls -la ~/.local/share/applications/hyperrecall.desktop

# Update desktop database (some systems)
update-desktop-database ~/.local/share/applications/

# Restart your desktop environment or log out/in
```

---

## Performance Issues

### Low FPS or laggy

**Symptoms**: Application runs slowly

**Solutions**:

1. **Ensure Release build**:
   ```bash
   rm -rf build
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```

2. **Check system resources**:
   ```bash
   # While app is running
   top
   # or
   htop
   ```

3. **GPU drivers**: Ensure you have proper graphics drivers installed

---

## Getting More Help

### Enable Verbose Output

For more detailed error messages:

```bash
# Verbose CMake
cmake -S . -B build --log-level=DEBUG

# Verbose build
cmake --build build --verbose

# Or
VERBOSE=1 make build
```

### Check System Information

```bash
# System info
uname -a

# CMake version
cmake --version

# Compiler version
gcc --version

# Library versions
pkg-config --modversion raylib sqlite3
```

### Collect Debug Information

When reporting issues, include:

1. Output of `./check-setup.sh`
2. Output of build command
3. Any error messages
4. System information (OS, version, architecture)
5. CMake version
6. Compiler version

### Report a Bug

If you've tried everything and still have issues:

1. Check existing issues: https://github.com/bk0704/HyperRecall/issues
2. Open a new issue with:
   - Detailed description
   - Steps to reproduce
   - Output of `./check-setup.sh`
   - Full error messages
   - System information

---

## Quick Reference

| Issue | Quick Fix |
|-------|-----------|
| Can't find dependencies | `./check-setup.sh` then `make install-deps` |
| Build fails | `make clean && make build` |
| Scripts won't run | `chmod +x *.sh` |
| CMake issues | `rm -rf build && ./run.sh` |
| Runtime crash | Run from terminal to see errors |
| Missing assets | `cmake --build build` to recopy |
| Slow build | Use Ninja: `cmake -S . -B build -G Ninja` |

---

**Still stuck?** Check the other documentation files:
- `QUICKSTART.md` - Quick start guide
- `USAGE.md` - Detailed usage instructions
- `README.md` - Full documentation
- `ACCESS_METHODS.md` - All ways to run HyperRecall
