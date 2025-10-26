@echo off
REM HyperRecall One-Click Run Script for Windows (Batch)

echo.
echo 🚀 HyperRecall One-Click Launcher
echo ==================================
echo.

cd /d "%~dp0"

REM Check if build directory exists
if not exist "build" (
    echo 📦 First run detected - configuring build...
    
    REM Check for cmake
    where cmake >nul 2>&1
    if errorlevel 1 (
        echo ❌ Error: cmake not found. Please install cmake first.
        echo    Download from: https://cmake.org/download/
        pause
        exit /b 1
    )
    
    REM Configure
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    if errorlevel 1 (
        echo ❌ Error: CMake configuration failed
        pause
        exit /b 1
    )
    echo.
)

REM Build the project
echo 🔨 Building HyperRecall...
cmake --build build --config Release
if errorlevel 1 (
    echo ❌ Error: Build failed
    pause
    exit /b 1
)
echo.

REM Check if executable exists
if exist "build\bin\hyperrecall.exe" (
    set EXE_PATH=build\bin\hyperrecall.exe
) else if exist "build\bin\Release\hyperrecall.exe" (
    set EXE_PATH=build\bin\Release\hyperrecall.exe
) else (
    echo ❌ Error: Build failed - executable not found
    pause
    exit /b 1
)

echo ✅ Build successful!
echo 🎯 Launching HyperRecall...
echo.

REM Run the application
cd build\bin 2>nul || cd build\bin\Release
hyperrecall.exe
