@echo off
setlocal

rem Build the pinned configuration and stage the developer package.
set "REPO_ROOT=%~dp0..\.."
for %%I in ("%REPO_ROOT%") do set "REPO_ROOT=%%~fI"
set "CONFIGURATION=%~1"
if not defined CONFIGURATION set "CONFIGURATION=Release"
if /I not "%CONFIGURATION%"=="Release" if /I not "%CONFIGURATION%"=="RelWithDebInfo" (
  echo Usage: %~nx0 [Release^|RelWithDebInfo]
  exit /b 2
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%REPO_ROOT%\scripts\build.ps1" -Configuration "%CONFIGURATION%"
if errorlevel 1 exit /b %errorlevel%

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0stage_package.ps1" -Configuration "%CONFIGURATION%"
if errorlevel 1 exit /b %errorlevel%

echo Package staged at "%~dp0..\dist\package".
exit /b 0

