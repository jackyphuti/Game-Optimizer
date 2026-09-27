@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo     Building Game Optimizer (C++20/C++17)
echo ===================================================

where cmake >nul 2>nul
if %errorlevel% equ 0 (
    echo [*] Found CMake in PATH. Building with CMake...
    if not exist build mkdir build
    cmake -B build -S .
    if %errorlevel% neq 0 (
        echo [-] CMake configuration failed.
        exit /b 1
    )
    cmake --build build --config Release
    if %errorlevel% neq 0 (
        echo [-] CMake build failed.
        exit /b 1
    )
    echo [+] Build succeeded! Executable is at build\game-optimizer.exe
    exit /b 0
)

where wsl >nul 2>nul
if %errorlevel% equ 0 (
    echo [*] CMake not in Windows PATH, utilizing WSL toolchain...
    wsl bash -c "cd /mnt/c/Users/jacky/Documents/GitHub/Game-Optimizer && cmake -B build-win -S . -GNinja -DCMAKE_TOOLCHAIN_FILE=mingw64-toolchain.cmake && cmake --build build-win"
    if %errorlevel% neq 0 (
        echo [-] WSL build failed.
        exit /b 1
    )
    if not exist build mkdir build
    copy /Y build-win\game-optimizer.exe build\game-optimizer.exe >nul
    echo [+] Build succeeded: build\game-optimizer.exe
    exit /b 0
)

echo [-] Error: Neither CMake nor WSL toolchain was found.
exit /b 1
