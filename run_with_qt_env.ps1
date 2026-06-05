# PowerShell script to run the Qt application with proper environment
$ErrorActionPreference = "Continue"

# Set Qt path
$qtPath = "D:\Qt6.9.1\6.5.3\msvc2019_64\bin"
$appPath = "F:\工作文档\2026\大功率可调恒流源\QT上位机\build\Debug\SeedSourceControl.exe"

# Add Qt to PATH
$env:PATH = "$qtPath;$env:PATH"

Write-Host "Starting SeedSourceControl application..." -ForegroundColor Green
Write-Host "Application path: $appPath" -ForegroundColor Cyan

# Start the application
Start-Process -FilePath $appPath -WorkingDirectory (Split-Path -Parent $appPath)
Write-Host "Application started successfully!" -ForegroundColor Green
