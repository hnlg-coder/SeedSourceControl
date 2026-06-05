@echo off
call "D:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d "%~dp0build\Debug"
set PATH=D:\Qt6.9.1\6.5.3\msvc2019_64\bin;%PATH%
"D:\Qt6.9.1\6.5.3\msvc2019_64\bin\windeployqt.exe" SeedSourceControl.exe
if %ERRORLEVEL% EQU 0 (
    echo Deployment complete, starting application...
    SeedSourceControl.exe
)
