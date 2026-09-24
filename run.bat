@echo off
setlocal
echo Launching Smart Parking System...
if exist "build\Release\SmartParking.exe" (
    cd build\Release
    SmartParking.exe %*
    cd ..\..
) else if exist "build\SmartParking.exe" (
    cd build
    SmartParking.exe %*
    cd ..
) else (
    echo Error: SmartParking.exe not found. Please run build.bat first.
    exit /b 1
)
