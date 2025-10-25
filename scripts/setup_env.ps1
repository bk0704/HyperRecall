#!/usr/bin/env pwsh
Write-Host "HyperRecall Environment Setup (Windows)" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan

Write-Host "Checking for Chocolatey..."
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Warning "Chocolatey not found. TODO: Install Chocolatey from https://chocolatey.org/install to continue."
} else {
    Write-Host "Chocolatey detected. Installing recommended packages..."
    choco install -y ninja cmake git
    Write-Host "TODO: Install raylib and sqlite3 via your preferred package manager or vcpkg."
}

Write-Host "Configuring vcpkg (optional)..."
if (-not $Env:VCPKG_ROOT) {
    Write-Warning "VCPKG_ROOT is not set. TODO: Clone https://github.com/microsoft/vcpkg and set VCPKG_ROOT for dependency resolution."
} else {
    & "$Env:VCPKG_ROOT\vcpkg" integrate install
    Write-Host "You can install dependencies with: vcpkg install raylib sqlite3 --triplet x64-windows"
    Write-Host "TODO: Use vcpkg fetch to acquire raygui or supply raygui.h manually."
}

Write-Host "Next steps:" -ForegroundColor Green
Write-Host "1. Ensure raylib, raygui, and sqlite3 headers/libs are discoverable (see TODOs above)."
Write-Host "2. Configure the build: cmake -S . -B build -G 'Ninja' -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$Env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
Write-Host "3. Build and run: cmake --build build; cmake --build build --target run"
