#!/usr/bin/env bash
# HyperRecall Setup Status Checker
# Verifies that all dependencies and files are in place

set -e

echo "🔍 HyperRecall Setup Status Check"
echo "=================================="
echo

STATUS_OK=0

# Check build tools
echo "Checking build tools..."
if command -v cmake >/dev/null 2>&1; then
    echo "  ✅ cmake: $(cmake --version | head -1)"
else
    echo "  ❌ cmake: Not found"
    STATUS_OK=1
fi

if command -v ninja >/dev/null 2>&1; then
    echo "  ✅ ninja: $(ninja --version)"
elif command -v make >/dev/null 2>&1; then
    echo "  ✅ make: $(make --version | head -1)"
else
    echo "  ❌ build tool: Neither ninja nor make found"
    STATUS_OK=1
fi

if command -v gcc >/dev/null 2>&1; then
    echo "  ✅ gcc: $(gcc --version | head -1)"
elif command -v clang >/dev/null 2>&1; then
    echo "  ✅ clang: $(clang --version | head -1)"
else
    echo "  ❌ compiler: Neither gcc nor clang found"
    STATUS_OK=1
fi

echo

# Check libraries
echo "Checking libraries..."
if pkg-config --exists sqlite3 2>/dev/null; then
    echo "  ✅ SQLite3: $(pkg-config --modversion sqlite3)"
else
    echo "  ⚠️  SQLite3: Not found via pkg-config (may still work if installed)"
fi

if pkg-config --exists raylib 2>/dev/null; then
    echo "  ✅ raylib: $(pkg-config --modversion raylib)"
elif [ -f /usr/local/lib/libraylib.a ] || [ -f /usr/lib/libraylib.so ] || [ -f /usr/lib/x86_64-linux-gnu/libraylib.so ]; then
    echo "  ✅ raylib: Found in system libraries"
else
    echo "  ⚠️  raylib: Not found (required for running HyperRecall)"
    echo "      Install with: sudo apt install libraylib-dev"
    echo "      Or see README.md for build instructions"
    STATUS_OK=1
fi

echo

# Check source files
echo "Checking source files..."
if [ -d "src" ] && [ -f "CMakeLists.txt" ]; then
    SRC_COUNT=$(find src -name "*.c" | wc -l)
    HDR_COUNT=$(find src -name "*.h" | wc -l)
    echo "  ✅ Source files: $SRC_COUNT C files, $HDR_COUNT headers"
else
    echo "  ❌ Source directory or CMakeLists.txt not found"
    STATUS_OK=1
fi

if [ -d "external/raygui" ]; then
    echo "  ✅ raygui: Bundled dependency found"
else
    echo "  ❌ raygui: Bundled dependency not found"
    STATUS_OK=1
fi

echo

# Check scripts
echo "Checking convenience scripts..."
[ -f "run.sh" ] && echo "  ✅ run.sh" || echo "  ❌ run.sh"
[ -f "run.bat" ] && echo "  ✅ run.bat" || echo "  ❌ run.bat"
[ -f "run.ps1" ] && echo "  ✅ run.ps1" || echo "  ❌ run.ps1"
[ -f "Makefile" ] && echo "  ✅ Makefile" || echo "  ❌ Makefile"
[ -f "dev.sh" ] && echo "  ✅ dev.sh" || echo "  ❌ dev.sh"
[ -f "install-launcher.sh" ] && echo "  ✅ install-launcher.sh" || echo "  ❌ install-launcher.sh"

echo

# Check build status
echo "Checking build status..."
if [ -d "build" ]; then
    echo "  ✅ Build directory exists"
    if [ -f "build/bin/hyperrecall" ]; then
        BINARY_SIZE=$(ls -lh build/bin/hyperrecall | awk '{print $5}')
        echo "  ✅ Executable built: $BINARY_SIZE"
        echo ""
        echo "Ready to run! Use: ./run.sh or ./build/bin/hyperrecall"
    else
        echo "  ⚠️  Executable not yet built"
        echo ""
        echo "Run './run.sh' or 'make build' to build"
    fi
else
    echo "  ℹ️  Not yet configured"
    echo ""
    echo "Run './run.sh' or 'make configure' to get started"
fi

echo
echo "=================================="
if [ $STATUS_OK -eq 0 ]; then
    echo "✅ All checks passed! Ready to build HyperRecall."
    echo ""
    echo "Quick start: ./run.sh"
else
    echo "⚠️  Some dependencies are missing."
    echo ""
    echo "To install dependencies:"
    echo "  • Linux: make install-deps"
    echo "  • Or see QUICKSTART.md for instructions"
fi
