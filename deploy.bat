@echo off
echo Creating deploy directory...
if not exist "build\deploy\Debug" mkdir "build\deploy\Debug"

echo Copying executable...
copy "build\Debug\SeedSourceControl.exe" "build\deploy\Debug\"

echo Running windeployqt...
"D:\Qt6.9.1\6.5.3\msvc2019_64\bin\windeployqt.exe" --no-translations --no-system-d3d-compiler --no-opengl-sw --no-quick-import "build\deploy\Debug\SeedSourceControl.exe"

echo Deployment complete!
