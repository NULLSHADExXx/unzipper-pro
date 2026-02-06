@echo off
REM Unzipper Pro 2.0 - Windows Build Script

echo.
echo ==========================================
echo Unzipper Pro 2.0 - Build Script (Windows)
echo ==========================================
echo.

REM Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: CMake not found. Please install CMake from https://cmake.org
    pause
    exit /b 1
)

REM Create build directory
echo Creating build directory...
if not exist build mkdir build
cd build

REM Configure
echo Configuring CMake...
cmake -G "Visual Studio 16 2019" -A x64 ..
if %errorlevel% neq 0 (
    echo Error: CMake configuration failed
    pause
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Error: Build failed
    pause
    exit /b 1
)

echo.
echo ==========================================
echo Build successful!
echo ==========================================
echo.
echo Run the application:
echo   Release\UnzipperPro.exe
echo.
pause
