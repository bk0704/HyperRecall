#!/bin/bash
# Quick test script to verify dual backend build configuration
# This doesn't require dependencies to be installed, just tests CMake configuration

set -e

echo "=================================="
echo "HyperRecall Dual Backend Test"
echo "=================================="
echo ""

# Test 1: Default Raylib backend
echo "✓ Test 1: Default (Raylib) backend configuration..."
rm -rf /tmp/hr-test-default
cmake -S . -B /tmp/hr-test-default -DCMAKE_BUILD_TYPE=Release 2>&1 | \
    grep -q "Building with Raylib" && echo "  → Raylib backend selected" || \
    (echo "  ✗ Failed"; exit 1)

# Test 2: Explicit Raylib backend  
echo "✓ Test 2: Explicit Raylib backend configuration..."
rm -rf /tmp/hr-test-raylib
cmake -S . -B /tmp/hr-test-raylib -DHYPERRECALL_UI_BACKEND=RAYLIB -DCMAKE_BUILD_TYPE=Release 2>&1 | \
    grep -q "Building with Raylib" && echo "  → Raylib backend selected" || \
    (echo "  ✗ Failed"; exit 1)

# Test 3: Qt6 backend
echo "✓ Test 3: Qt6 backend configuration..."
rm -rf /tmp/hr-test-qt6
cmake -S . -B /tmp/hr-test-qt6 -DHYPERRECALL_UI_BACKEND=QT6 -DCMAKE_BUILD_TYPE=Release 2>&1 | \
    grep -q "Building with Qt6" && echo "  → Qt6 backend selected" || \
    (echo "  ✗ Failed"; exit 1)

# Test 4: Invalid backend value
echo "✓ Test 4: Invalid backend value rejection..."
rm -rf /tmp/hr-test-invalid
if cmake -S . -B /tmp/hr-test-invalid -DHYPERRECALL_UI_BACKEND=INVALID 2>&1 | \
    grep -q "Invalid HYPERRECALL_UI_BACKEND"; then
    echo "  → Invalid backend correctly rejected"
else
    echo "  ✗ Failed"
    exit 1
fi

echo ""
echo "=================================="
echo "All configuration tests passed! ✓"
echo "=================================="
echo ""
echo "To build with Raylib (default):"
echo "  cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build build"
echo ""
echo "To build with Qt6:"
echo "  cmake -B build-qt -DHYPERRECALL_UI_BACKEND=QT6 -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build build-qt"
echo ""
echo "See INSTALL.md for dependency installation instructions."
