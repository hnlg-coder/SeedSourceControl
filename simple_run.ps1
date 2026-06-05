# Simple run script - no complex paths
$env:PATH = "D:\Qt6.9.1\6.5.3\msvc2019_64\bin;" + $env:PATH
cd build\Debug
Start-Process .\SeedSourceControl.exe
