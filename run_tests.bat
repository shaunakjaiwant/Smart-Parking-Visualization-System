@echo off
setlocal
echo Running Smart Parking Automated Tests...
if exist "build\Release\SmartParkingTests.exe" (
    cd build\Release
    SmartParkingTests.exe %*
    cd ..\..
) else if exist "build\SmartParkingTests.exe" (
    cd build
    SmartParkingTests.exe %*
    cd ..
) else (
    echo Error: SmartParkingTests.exe not found. Please run build.bat first.
    exit /b 1
)
