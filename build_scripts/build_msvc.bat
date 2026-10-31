@echo off
:: Build Nirqoba compiler using MSVC toolchain
:: Usage: build_msvc.bat [Release|RelWithDebInfo|Debug]

set "CONFIGURATION=%1"
if "%CONFIGURATION%"=="" set "CONFIGURATION=RelWithDebInfo"

echo Building Nirqoba compiler with configuration: %CONFIGURATION%
cd /d "%~dp0.."

mkdir build\msvc-%CONFIGURATION% 2>nul
cd build\msvc-%CONFIGURATION%

cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=%CONFIGURATION% -DCMAKE_CXX_COMPILER="cl.exe" 2>&1 | findstr "--"
if %errorlevel% neq 0 (
    echo CMake configuration failed
    pause
    exit /b %errorlevel%
)

cmake --build . --config %CONFIGURATION% 2>&1 | findstr "error\|FAILED"
if %errorlevel% neq 0 (
    echo Build failed
    pause
    exit /b %errorlevel%
)

echo.
echo Build successful!
echo.
echo Output: .\bin\nirqoba.exe
pause