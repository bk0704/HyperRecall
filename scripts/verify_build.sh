#!/usr/bin/env bash
# Build verification script for HyperRecall

set -e

echo "=== HyperRecall Build Verification ==="
echo

# Check for required tools
echo "Checking build tools..."
command -v cmake >/dev/null 2>&1 || { echo "❌ cmake not found"; exit 1; }
command -v ninja >/dev/null 2>&1 || command -v make >/dev/null 2>&1 || { echo "❌ ninja or make not found"; exit 1; }
command -v gcc >/dev/null 2>&1 || command -v clang >/dev/null 2>&1 || { echo "❌ C compiler not found"; exit 1; }
echo "✓ Build tools OK"
echo

# Check for dependencies
echo "Checking dependencies..."
pkg-config --exists sqlite3 2>/dev/null || { echo "❌ SQLite3 not found"; exit 1; }
echo "✓ SQLite3 OK"

# raylib might not be in pkg-config, check for library
if [ -f /usr/local/lib/libraylib.a ] || [ -f /usr/lib/libraylib.so ] || [ -f /usr/lib/x86_64-linux-gnu/libraylib.so ]; then
    echo "✓ raylib found"
else
    echo "⚠️  raylib may not be installed (continuing anyway)"
fi
echo

# Clean build
echo "Cleaning previous build..."
rm -rf build
echo

# Configure
echo "Configuring build..."
if command -v ninja >/dev/null 2>&1; then
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
else
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
fi
echo

# Build
echo "Building..."
cmake --build build
echo

# Check outputs
echo "Verifying build outputs..."
[ -f build/bin/hyperrecall ] || { echo "❌ Executable not found"; exit 1; }
echo "✓ Executable built: build/bin/hyperrecall"

[ -d build/bin/assets ] || { echo "❌ Assets not copied"; exit 1; }
echo "✓ Assets copied to build/bin/assets"

[ -f build/bin/assets/themes.json ] || { echo "❌ themes.json not found"; exit 1; }
echo "✓ Theme configuration present"
echo

# Source file count check
src_files=$(find src -name "*.c" | wc -l)
echo "✓ Source files: $src_files"
header_files=$(find src -name "*.h" | wc -l)
echo "✓ Header files: $header_files"
echo

echo "=== Build Verification Complete ==="
echo
echo "To run HyperRecall: ./build/bin/hyperrecall"
echo "Note: Requires X11 display for GUI"
