@echo off
echo ====================================
echo 种子源模块控制器 - 部署与运行脚本
echo ====================================
echo.

call "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d "%~dp0build\Debug"

echo.
echo [1/2] 部署Qt库文件...
"D:\Qt6.9.1\6.5.3\msvc2019_64\bin\windeployqt.exe" SeedSourceControl.exe --no-translations --no-system-d3d-compiler --no-opengl-sw

echo.
echo [2/2] 启动应用程序...
SeedSourceControl.exe

echo.
echo 程序已退出。
pause
