# HyperRecall Access Methods - Summary

This document provides a visual overview of all the ways to build and run HyperRecall.

## 🎯 Quick Reference

| Method | Command | Best For |
|--------|---------|----------|
| **One-Click Run** | `./run.sh` (Linux/Mac)<br>`run.bat` (Windows) | First-time users, daily use |
| **Make** | `make run` | Linux/Mac developers |
| **Desktop Launcher** | Click in app menu | Desktop environment users |
| **Development** | `./dev.sh` | Active development |
| **Shell Aliases** | `source aliases.sh` then `hr` | Power users |
| **Manual** | `cmake --build build && ./build/bin/hyperrecall` | Advanced users |

## 📊 Access Method Decision Tree

```
Are you on Windows?
├─ YES → Use run.bat or run.ps1
└─ NO → Are you developing?
    ├─ YES → Use dev.sh or 'source aliases.sh'
    └─ NO → Just want to run?
        ├─ YES → Use ./run.sh or make run
        └─ NO → Want desktop integration?
            ├─ YES → Run ./install-launcher.sh
            └─ NO → Use manual cmake commands
```

## 🚀 Complete Workflow Examples

### New User Journey
```bash
# 1. Clone repository
git clone https://github.com/bk0704/HyperRecall.git
cd HyperRecall

# 2. Check setup (optional but recommended)
./check-setup.sh

# 3. Install dependencies if needed (Linux)
make install-deps

# 4. Run!
./run.sh
```

### Developer Journey
```bash
# 1. Clone and setup
git clone https://github.com/bk0704/HyperRecall.git
cd HyperRecall

# 2. Setup aliases for convenience
source aliases.sh

# 3. Initial development build
hr-dev

# 4. Make changes to code...

# 5. Quick rebuild and run
hr-build
hr-run

# 6. Or just
hr
```

### Daily User Journey
```bash
# Option 1: Command line
cd HyperRecall
./run.sh

# Option 2: Desktop launcher (after running install-launcher.sh)
# Just click HyperRecall in your application menu!
```

## 📂 File Overview

| File | Purpose | When to Use |
|------|---------|-------------|
| `run.sh` | Production build & run (Linux/Mac) | Daily use |
| `run.bat` | Production build & run (Windows) | Daily use |
| `run.ps1` | Production build & run (PowerShell) | Daily use |
| `dev.sh` | Debug build with dev tools | Development |
| `Makefile` | Make targets for common tasks | Development |
| `aliases.sh` | Shell shortcuts | Power users |
| `check-setup.sh` | Verify environment | First setup |
| `install-launcher.sh` | Desktop integration (Linux) | Desktop users |
| `HyperRecall.desktop` | Desktop entry template | Used by installer |

## 🎨 Feature Comparison

| Feature | run.sh | Makefile | dev.sh | Manual |
|---------|--------|----------|--------|--------|
| One command | ✅ | ✅ | ✅ | ❌ |
| Auto-configure | ✅ | ✅ | ✅ | ❌ |
| Debug symbols | ❌ | ❌ | ✅ | Choose |
| Dev tools | ❌ | ❌ | ✅ | Choose |
| Optimizations | ✅ | ✅ | ❌ | Choose |
| Platform | Unix | Unix | Unix | All |

## 🔧 Script Details

### run.sh / run.bat / run.ps1
- Checks for dependencies
- Configures build on first run
- Builds in Release mode
- Launches application
- **Use for**: Daily running, production use

### dev.sh
- Configures with Debug mode
- Enables developer tools
- No optimizations (faster compile)
- Provides detailed error info
- **Use for**: Active development

### Makefile
- Provides convenient targets
- Wraps common CMake commands
- Includes install-deps target
- **Use for**: Development workflow

### check-setup.sh
- Validates build tools
- Checks dependencies
- Reports build status
- Gives helpful suggestions
- **Use for**: Troubleshooting, first setup

### install-launcher.sh
- Creates desktop entry file
- Sets up app menu integration
- Requires Linux with XDG
- **Use for**: Desktop environment integration

## 💡 Tips & Tricks

### Fastest Rebuild
```bash
# After making code changes
make build  # or hr-build if using aliases
```

### Clean Rebuild
```bash
make clean
make run
```

### Check if Everything Works
```bash
./check-setup.sh
```

### Install for Desktop Use
```bash
./install-launcher.sh
# Now available in app menu!
```

### Development Workflow
```bash
# Setup once per session
source aliases.sh

# Then use short commands
hr-dev      # Build with debug
hr          # Run
hr-clean    # Clean
hr-rebuild  # Clean + build
```

### Update to Latest Version
```bash
git pull
./run.sh    # Automatically rebuilds
```

## 🎓 Learning Path

1. **First Time**: `./check-setup.sh` → `make install-deps` → `./run.sh`
2. **Daily Use**: `./run.sh` or click desktop launcher
3. **Development**: `source aliases.sh` → `hr-dev` → make changes → `hr-build` → `hr-run`
4. **Power User**: Learn all aliases, use Makefile targets, customize CMake options

## 📞 Getting Help

If something doesn't work:

1. Run `./check-setup.sh` to diagnose
2. Check `QUICKSTART.md` for setup instructions
3. See `USAGE.md` for detailed command documentation
4. Check `README.md` for comprehensive information
5. Open an issue on GitHub

---

**Remember**: The goal is to make it as easy as possible to use HyperRecall. Choose the method that works best for you!
