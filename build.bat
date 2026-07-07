@echo off
rem Record Management System — Windows build script (needs g++ in PATH)
g++ -std=c++17 -Wall -Wextra -O2 -static src\main.cpp src\Book.cpp src\RecordManager.cpp src\ConsoleUI.cpp -o rms.exe
if %errorlevel% equ 0 (
    echo Build OK: run rms.exe
) else (
    echo Build failed.
)
