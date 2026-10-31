@echo off
:: Build Nirqoba Windows Installer using Inno Setup
:: Requires: Inno Setup 6.x installed and in PATH
:: Usage: build_installer.bat [/src <directory>] [/out <output directory>]

set "SRCDIR=%~dp0.."
set "OUTDIR=%SRCDIR%\windows_installer\dist"
set "ISCC="

:: Parse arguments
for %%i in (%) do (
    if /I "%%~i"=="/src" set "SRCDIR=%%~xi"
    if /I "%%~i"=="/out" set "OUTDIR=%%~xi"
)

echo.
echo ============================================
echo Nirqoba Windows Installer Builder
echo ============================================
echo.

:: Check for Inno Setup
where iscc >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo Error: Inno Setup compiler (iscc) not found in PATH.
    echo Please install Inno Setup 6.x from: https://jrsoftware.org/isdownload.php
    echo.
    pause
    exit /b 1
)

echo Found Inno Setup compiler.
iscc --version

:: Change to installer scripts directory
pushd "%SRCDIR%\windows_installer\installer_scripts"

echo.
echo Building installer from: installer_scripts\
echo Output to: %OUTDIR\
echo.

:: Build the installer
iscc nirqoba_installer.iss

if %errorlevel% neq 0 (
    echo.
    echo Error: Inno Setup compilation failed with error code %errorlevel%
    echo.
    popd
    exit /b %errorlevel%
)

echo.
echo.
echo Installer build successful!
echo.

:: Check if output files exist
if exist "%OUTDIR\nirqoba_setup.exe" (
    echo Installer executable: %OUTDIR\nirqoba_setup.exe
    echo Size: %~zxnirqoba_setup.exe bytes
) else (
    echo Warning: Installer output not found in %OUTDIR\
)

:: List all output files
dir "%OUTDIR\*.exe" /-b 2>nul
dir "%OUTDIR\*.iss" /-b 2>nul

popd
echo.
echo ============================================
echo Build complete.
echo ============================================
pause