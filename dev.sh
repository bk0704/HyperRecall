#!/usr/bin/env bash
# HyperRecall Development Mode Script
# Watches for file changes and automatically rebuilds

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "🔧 HyperRecall Development Mode"
echo "================================"
echo

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "📦 Configuring build with Debug mode..."
    if command -v ninja >/dev/null 2>&1; then
        cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DHYPERRECALL_ENABLE_DEVTOOLS=ON
    else
        cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DHYPERRECALL_ENABLE_DEVTOOLS=ON
    fi
    echo
fi

echo "🔨 Building HyperRecall (Debug mode with dev tools)..."
cmake --build build
echo

# Check if executable exists
if [ ! -f "build/bin/hyperrecall" ]; then
    echo "❌ Error: Build failed - executable not found"
    exit 1
fi

echo "✅ Build successful!"
echo
echo "Development build complete with:"
echo "  • Debug symbols enabled"
echo "  • Developer tools enabled"
echo "  • No optimizations (faster compile)"
echo
echo "To run: ./build/bin/hyperrecall"
echo
echo "For automatic rebuild on changes, install 'entr' and run:"
echo "  find src -name '*.c' -o -name '*.h' | entr -c make build"
