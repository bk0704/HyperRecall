# HyperRecall One-Click Run Script for Windows
# This script builds and runs HyperRecall in a single command

$ErrorActionPreference = "Stop"

Write-Host "🚀 HyperRecall One-Click Launcher" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan
Write-Host ""

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

# Check if build directory exists
if (-not (Test-Path "build")) {
    Write-Host "📦 First run detected - configuring build..." -ForegroundColor Yellow
    
    # Check for dependencies
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Write-Host "❌ Error: cmake not found. Please install cmake first." -ForegroundColor Red
        Write-Host "   Download from: https://cmake.org/download/" -ForegroundColor Yellow
        exit 1
    }
    
    # Configure with Ninja if available, otherwise use default
    if (Get-Command ninja -ErrorAction SilentlyContinue) {
        cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    } else {
        cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    }
    Write-Host ""
}

# Build the project
Write-Host "🔨 Building HyperRecall..." -ForegroundColor Yellow
cmake --build build --config Release
Write-Host ""

# Check if executable exists
$exePath = "build\bin\hyperrecall.exe"
if (-not (Test-Path $exePath)) {
    $exePath = "build\bin\Release\hyperrecall.exe"
    if (-not (Test-Path $exePath)) {
        Write-Host "❌ Error: Build failed - executable not found" -ForegroundColor Red
        exit 1
    }
}

Write-Host "✅ Build successful!" -ForegroundColor Green
Write-Host "🎯 Launching HyperRecall..." -ForegroundColor Cyan
Write-Host ""

# Run the application
Set-Location (Split-Path $exePath -Parent)
& ".\$(Split-Path $exePath -Leaf)"
