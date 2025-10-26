# HyperRecall Quick Start Guide

Get up and running with HyperRecall in seconds!

## 🚀 One-Click Launch

### Linux / macOS

**Just run:**
```bash
./run.sh
```

Or use Make:
```bash
make run
```

That's it! The script will:
- ✅ Check for dependencies
- ✅ Configure the build system (first run only)
- ✅ Build the application
- ✅ Launch HyperRecall

### Windows

**Double-click** `run.bat` 

Or in PowerShell:
```powershell
.\run.ps1
```

Or in Command Prompt:
```cmd
run.bat
```

## 📦 First Time Setup

### Linux (Ubuntu/Debian)

If you haven't installed dependencies yet:

```bash
# Option 1: Use the Makefile
make install-deps

# Option 2: Manual installation
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config \
    libsqlite3-dev libraylib-dev
```

### Windows

Install dependencies with vcpkg:

```powershell
# Clone vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install raylib sqlite3 --triplet x64-windows

# Go back to HyperRecall directory
cd ..
```

Then use `run.bat` or `run.ps1` to build and run.

## 🖥️ Desktop Launcher (Linux)

For even easier access, install a desktop launcher:

```bash
./install-launcher.sh
```

After installation, you can launch HyperRecall from:
- Your application menu (Education category)
- GNOME Activities / KDE Application Launcher
- Search for "HyperRecall"

## 🎯 Available Commands

### Quick Commands
- `./run.sh` or `make run` - Build and run (Linux/macOS)
- `run.bat` or `run.ps1` - Build and run (Windows)
- `make build` - Build only (Linux/macOS)
- `make clean` - Clean build artifacts (Linux/macOS)
- `make help` - Show all available commands (Linux/macOS)

### Manual Build (Advanced)
If you prefer to build manually:

```bash
# Configure (first time only)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run
./build/bin/hyperrecall          # Linux/macOS
.\build\bin\hyperrecall.exe      # Windows
```

## ⚡ Quick Tips

- **First Launch**: HyperRecall will create a configuration directory and database automatically
- **Location**: `~/.local/share/HyperRecall/` (Linux) or `%APPDATA%\HyperRecall\` (Windows)
- **Updates**: Just run `git pull` and then `./run.sh` again
- **Rebuilds**: The scripts automatically rebuild if source files change

## 🆘 Troubleshooting

### "cmake not found"
Install CMake:
- Ubuntu/Debian: `sudo apt install cmake`
- Windows: Download from [cmake.org](https://cmake.org/download/)

### "raylib not found"
- Ubuntu/Debian: `sudo apt install libraylib-dev`
- Or follow the full installation guide in `README.md`

### Build fails
Try a clean rebuild:
```bash
make clean
make run
```

### Can't run - "Display not found"
HyperRecall requires a graphical environment (X11 on Linux, GUI on Windows).

## 📖 Next Steps

Once HyperRecall is running:

1. **Create your first deck** - Click "New Card" or press `Ctrl+N`
2. **Start studying** - Click a study mode button (Mastery/Cram)
3. **Explore settings** - Press `Ctrl+P` for the command palette
4. **Check analytics** - View your progress and statistics

## 📚 Learn More

- Full documentation: `README.md`
- Development roadmap: `ROADMAP.md`
- Acceptance tests: `ACCEPTANCE_TESTS.md`

Happy studying! 🎓
