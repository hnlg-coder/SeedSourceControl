@echo off
call "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d "%~dp0"
"D:\Qt6.9.1\Tools\CMake_64\bin\cmake.exe" -G "Visual Studio 17 2022" -A x64 -B build -S . -DCMAKE_PREFIX_PATH="D:/Qt6.9.1/6.5.3/msvc2019_64"
if %ERRORLEVEL% EQU 0 (
    echo Configuration successful! Building...
    "D:\Qt6.9.1\Tools\CMake_64\bin\cmake.exe" --build build --config Debug
)
