@echo off
setlocal

set "INSTALLER_ROOT=%~dp0.."
for %%I in ("%INSTALLER_ROOT%") do set "INSTALLER_ROOT=%%~fI"
set "REPO_ROOT=%INSTALLER_ROOT%\.."
for %%I in ("%REPO_ROOT%") do set "REPO_ROOT=%%~fI"
set "SOURCE_DIR=%INSTALLER_ROOT%\dist\package"
set "OUTPUT_DIR=%INSTALLER_ROOT%\dist"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="/src" (
  if "%~2"=="" goto usage
  for %%I in ("%~2") do set "SOURCE_DIR=%%~fI"
  shift
  shift
  goto parse_args
)
if /I "%~1"=="/out" (
  if "%~2"=="" goto usage
  for %%I in ("%~2") do set "OUTPUT_DIR=%%~fI"
  shift
  shift
  goto parse_args
)
goto usage

:args_done
if not exist "%SOURCE_DIR%\bin\nirqoba.exe" (
  echo Error: staged compiler was not found at "%SOURCE_DIR%\bin\nirqoba.exe".
  echo Run build_msvc.bat first, or pass /src with a staged package directory.
  exit /b 2
)

set "ISCC="
for %%I in (ISCC.exe) do set "ISCC=%%~$PATH:I"
if not defined ISCC if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not defined ISCC if exist "%ProgramFiles%\Inno Setup 6\ISCC.exe" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not defined ISCC (
  echo Error: Inno Setup 6 compiler ISCC.exe was not found.
  echo Install Inno Setup 6 and add ISCC.exe to PATH.
  exit /b 3
)

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
if errorlevel 1 exit /b 1

"%ISCC%" "/DStageDir=%SOURCE_DIR%" "/O%OUTPUT_DIR%" "/Fnirqoba-setup" "%INSTALLER_ROOT%\installer_scripts\nirqoba_installer.iss"
if errorlevel 1 exit /b %errorlevel%

if not exist "%OUTPUT_DIR%\nirqoba-setup.exe" (
  echo Error: ISCC completed without producing nirqoba-setup.exe.
  exit /b 4
)
echo Installer created: "%OUTPUT_DIR%\nirqoba-setup.exe"
exit /b 0

:usage
echo Usage: %~nx0 [/src staged-package-directory] [/out output-directory]
exit /b 2
