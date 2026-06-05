@echo off
call "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d "%~dp0build"
"D:\Qt6.9.1\Tools\CMake_64\bin\cmake.exe" --build . --config Debug
echo.
echo Build complete!
