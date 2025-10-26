#!/usr/bin/env bash
# HyperRecall One-Click Run Script
# This script builds and runs HyperRecall in a single command

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "🚀 HyperRecall One-Click Launcher"
echo "=================================="
echo

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "📦 First run detected - configuring build..."
    
    # Check for dependencies
    if ! command -v cmake >/dev/null 2>&1; then
        echo "❌ Error: cmake not found. Please install cmake first."
        echo "   Run: sudo apt install cmake ninja-build"
        exit 1
    fi
    
    # Configure with Ninja if available, otherwise use Make
    if command -v ninja >/dev/null 2>&1; then
        cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    else
        cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    fi
    echo
fi

# Build the project
echo "🔨 Building HyperRecall..."
cmake --build build
echo

# Check if executable exists
if [ ! -f "build/bin/hyperrecall" ]; then
    echo "❌ Error: Build failed - executable not found"
    exit 1
fi

echo "✅ Build successful!"
echo "🎯 Launching HyperRecall..."
echo

# Run the application
cd build/bin
./hyperrecall
