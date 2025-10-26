# Testing Guide for HyperRecall

This document provides guidance on how to test HyperRecall and make it easier for automated testing (including AI agents).

## Table of Contents

- [Quick Testing](#quick-testing)
- [Automated Testing Setup](#automated-testing-setup)
- [Testing Environments](#testing-environments)
- [Manual Testing](#manual-testing)
- [CI/CD Testing](#cicd-testing)
- [Making Testing Easier](#making-testing-easier)

---

## Quick Testing

### Build and Run Tests
```bash
# Quick build verification
make build

# Check for compilation warnings
make build 2>&1 | grep -i "warning\|error"

# Verify binary was created
ls -lh build/bin/hyperrecall
```

### Syntax Validation
```bash
# Validate C files compile
gcc -fsyntax-only -std=c17 -Wall -Wextra -Wpedantic src/*.c -I/usr/local/include -I./src -I./external/raygui

# Check for common issues
clang-tidy src/*.c -- -std=c17 -I/usr/local/include -I./src -I./external/raygui
```

---

## Automated Testing Setup

### For AI Agents / Automated Testing

To make testing easier for automated systems (like GitHub Copilot), the following setup is recommended:

#### 1. Headless Testing Environment

Since HyperRecall is a GUI application, testing in headless environments requires special setup:

```bash
# Install Xvfb (X Virtual Framebuffer)
sudo apt-get install -y xvfb

# Run tests with virtual display
xvfb-run -a ./build/bin/hyperrecall --version
```

#### 2. Test Mode Flag

Add a `--test` flag to run without GUI:
```c
// In src/main.c or src/app.c
bool test_mode = false;
for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--test") == 0) {
        test_mode = true;
    }
}

if (test_mode) {
    // Run validation tests without GUI
    printf("Running validation tests...\n");
    // Test card type validation
    // Test database operations
    // Exit without launching GUI
    return 0;
}
```

#### 3. Validation Test Script

Create a simple validation script:

```bash
#!/bin/bash
# test-validation.sh

set -e

echo "Testing HyperRecall validation..."

# Test 1: Build succeeds
echo "Test 1: Build verification"
make clean && make build

# Test 2: Binary exists
echo "Test 2: Binary created"
test -f build/bin/hyperrecall

# Test 3: No compilation warnings
echo "Test 3: Clean compilation"
make clean && make build 2>&1 | tee build.log
if grep -qi "warning" build.log; then
    echo "FAIL: Compilation warnings found"
    exit 1
fi

# Test 4: Basic syntax checks
echo "Test 4: Syntax validation"
gcc -fsyntax-only -std=c17 -Wall -Wextra src/*.c -I/usr/local/include -I./src -I./external/raygui 2>&1

echo "All validation tests passed!"
```

---

## Testing Environments

### 1. Development Environment

**Purpose:** Local testing during development

**Setup:**
```bash
# Install development dependencies
make install-deps

# Build in debug mode
./dev.sh

# Run with debug symbols
gdb ./build/bin/hyperrecall
```

### 2. CI/CD Environment (GitHub Actions)

**Purpose:** Automated testing on every commit

**Current Setup:**
- Ubuntu (Debug and Release builds)
- Windows (Release build)
- Automated dependency installation
- Build verification

**Improvements Made:**
- ✅ Fixed raylib installation on Ubuntu
- ✅ Removed unnecessary vcpkg for Linux
- ✅ Added artifact uploads
- ✅ Separated Linux and Windows configuration steps

### 3. Headless Environment

**Purpose:** Testing without display (servers, CI)

**Setup:**
```bash
# Install Xvfb
sudo apt-get install xvfb

# Run with virtual display
xvfb-run -a ./build/bin/hyperrecall

# Alternative: Set offscreen rendering
export SDL_VIDEODRIVER=dummy
```

---

## Manual Testing

### Testing New Card Types

When adding new card types (like Explain, PracticalTask, etc.), test:

1. **Compilation:**
   ```bash
   make clean && make build
   ```

2. **Type Registration:**
   ```bash
   # Verify type appears in enum
   grep "HR_CARD_TYPE_" src/model.h
   
   # Verify type name in array
   grep "CARD_TYPE_NAMES" src/model.c -A 20
   ```

3. **Validation:**
   ```c
   // In a test file
   HrCardExtras extras;
   hr_card_extras_init(&extras, HR_CARD_TYPE_EXPLAIN);
   HrValidationError error;
   bool valid = hr_card_extras_validate(&extras, &error);
   // Should pass with proper initialization
   ```

4. **Switch Statement Coverage:**
   ```bash
   # Ensure all switch statements handle new type
   grep -n "switch.*type" src/model.c
   grep -n "case HR_CARD_TYPE_EXPLAIN" src/model.c
   ```

### Testing Database Operations

```bash
# Create test database
sqlite3 test.db < schema.sql

# Verify tables exist
sqlite3 test.db ".tables"

# Test card insertion
sqlite3 test.db "INSERT INTO cards (topic_id, prompt, response, type) VALUES (1, 'Test', 'Answer', 'ShortAnswer');"
```

---

## CI/CD Testing

### GitHub Actions Workflow

The updated workflow now:

1. **Installs Dependencies Correctly:**
   - Linux: Builds raylib from source (more reliable)
   - Windows: Uses vcpkg for dependencies

2. **Builds Successfully:**
   - Debug build on Linux
   - Release builds on Linux and Windows
   - All with proper error checking

3. **Uploads Artifacts:**
   - Binary artifacts for each platform
   - Helpful for manual verification

### Running Locally

Test the GitHub Actions workflow locally:

```bash
# Install act (GitHub Actions local runner)
curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash

# Run workflow locally
act -j build
```

---

## Making Testing Easier

### For Developers

1. **Add Unit Tests:**
   ```c
   // tests/test_card_validation.c
   #include "../src/model.h"
   #include <assert.h>
   
   void test_explain_card_validation() {
       HrCardExtras extras;
       hr_card_extras_init(&extras, HR_CARD_TYPE_EXPLAIN);
       extras.data.explain.topic = "Test Topic";
       
       HrValidationError error;
       assert(hr_card_extras_validate(&extras, &error));
   }
   ```

2. **Add Test Target to Makefile:**
   ```makefile
   test: build
   	@echo "Running tests..."
   	gcc -o test_validation tests/test_*.c src/model.c -I./src
   	./test_validation
   ```

### For AI Agents

1. **Provide Test Mode:**
   - Add `--version` flag to print version and exit
   - Add `--validate` flag to run validation tests
   - Add `--help` flag for usage information

2. **Return Meaningful Exit Codes:**
   ```c
   // Exit codes
   #define EXIT_SUCCESS 0
   #define EXIT_BUILD_ERROR 1
   #define EXIT_VALIDATION_ERROR 2
   #define EXIT_RUNTIME_ERROR 3
   ```

3. **JSON Output for Tests:**
   ```c
   if (test_mode && json_output) {
       printf("{\"status\":\"success\",\"tests_passed\":17,\"tests_failed\":0}\n");
   }
   ```

### For CI/CD

1. **Cache Dependencies:**
   ```yaml
   - name: Cache raylib
     uses: actions/cache@v3
     with:
       path: /usr/local
       key: raylib-5.0-${{ runner.os }}
   ```

2. **Parallel Testing:**
   ```yaml
   strategy:
     matrix:
       test: [validation, build, integration]
   ```

3. **Test Reports:**
   ```yaml
   - name: Generate test report
     run: |
       make test > test-report.txt
   - name: Upload test report
     uses: actions/upload-artifact@v4
     with:
       name: test-report
       path: test-report.txt
   ```

---

## Recommended Additions

### 1. Add Basic Validation Tests

Create `tests/test_validation.c`:
```c
#include "../src/model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    int passed = 0;
    int failed = 0;
    
    // Test 1: Explain card validation
    HrCardExtras explain;
    hr_card_extras_init(&explain, HR_CARD_TYPE_EXPLAIN);
    explain.data.explain.topic = "Test";
    
    HrValidationError error;
    if (hr_card_extras_validate(&explain, &error)) {
        passed++;
        printf("✓ Explain card validation\n");
    } else {
        failed++;
        printf("✗ Explain card validation: %s\n", error.message);
    }
    
    // Add more tests...
    
    printf("\nResults: %d passed, %d failed\n", passed, failed);
    return (failed > 0) ? 1 : 0;
}
```

### 2. Add --version Flag

In `src/main.c`:
```c
if (argc > 1 && strcmp(argv[1], "--version") == 0) {
    printf("HyperRecall v1.0.0\n");
    printf("Built with C17, raylib 5.0, SQLite3\n");
    return 0;
}
```

### 3. Add Smoke Test Script

Create `scripts/smoke-test.sh`:
```bash
#!/bin/bash
set -e

echo "Running smoke tests..."

# Build
make clean && make build

# Version check
./build/bin/hyperrecall --version

# Validate binary
file ./build/bin/hyperrecall

echo "Smoke tests passed!"
```

---

## Summary

### Current Testing Capabilities

✅ **Build Verification:** Compiles cleanly with zero warnings  
✅ **GitHub Actions:** Automated CI/CD on Linux and Windows  
✅ **Manual Testing:** Easy build and run with `./run.sh`  
✅ **Documentation:** Comprehensive guides for all workflows  

### Improvements Made

✅ Fixed GitHub Actions workflow  
✅ Proper dependency installation  
✅ Build artifact uploads  
✅ Clear testing documentation  

### Recommended Next Steps

1. Add `--version` and `--validate` flags to main.c
2. Create basic unit tests in `tests/` directory
3. Add test target to Makefile
4. Enable headless testing with Xvfb in CI
5. Add test coverage reporting

---

## Questions or Issues?

- **Build Issues:** See [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
- **CI/CD Issues:** Check [.github/workflows/build.yml](.github/workflows/build.yml)
- **Contributing:** See [CONTRIBUTING.md](CONTRIBUTING.md)

---

**Last Updated:** October 26, 2025  
**Version:** 1.0.0
