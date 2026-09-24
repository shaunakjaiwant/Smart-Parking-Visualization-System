@echo off
setlocal
echo ===================================================
echo  Smart Parking System - Build Script (MSVC x64)
echo ===================================================

set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set CMAKE="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if not exist %VCVARS% (
    echo Error: Visual Studio 2019 Build Tools vcvars64.bat not found at %VCVARS%
    exit /b 1
)

call %VCVARS%

if not exist build (
    mkdir build
)

cd build
%CMAKE% -G "Visual Studio 16 2019" -A x64 ..
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed.
    exit /b %ERRORLEVEL%
)

%CMAKE% --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo Build failed.
    exit /b %ERRORLEVEL%
)

cd ..
echo ===================================================
echo  Build successful! Executable is in build/Release/
echo ===================================================
