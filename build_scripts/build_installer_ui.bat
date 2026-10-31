@echo off
:: =============================================================================
:: Build script for Nirqoba Installer (modern UI .exe)
:: Uses MSVC + Windows SDK to compile installer_main.cpp
:: =============================================================================
setlocal EnableDelayedExpansion

echo.
echo  ╔══════════════════════════════════════╗
echo  ║   Nirqoba Installer Build Script     ║
echo  ╚══════════════════════════════════════╝
echo.

set "SRC_DIR=%~dp0..\src"
set "OUT_DIR=%~dp0..\dist"
set "ASSETS_DIR=%~dp0..\assets"

:: ---------------------------------------------------------------------------
:: Locate MSVC
:: ---------------------------------------------------------------------------
set "MSVC_VER=14.51.36231"
set "VS_ROOT=C:\Program Files\Microsoft Visual Studio\18\BuildTools"
set "CL=%VS_ROOT%\VC\Tools\MSVC\%MSVC_VER%\bin\Hostx64\x64\cl.exe"
set "LINK=%VS_ROOT%\VC\Tools\MSVC\%MSVC_VER%\bin\Hostx64\x64\link.exe"
set "RC_EXE=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe"

:: MSVC include/lib paths
set "MSVC_INC=%VS_ROOT%\VC\Tools\MSVC\%MSVC_VER%\include"
set "MSVC_LIB=%VS_ROOT%\VC\Tools\MSVC\%MSVC_VER%\lib\x64"

:: Windows SDK paths
set "SDK_VER=10.0.26100.0"
set "SDK_INC=C:\Program Files (x86)\Windows Kits\10\Include\%SDK_VER%"
set "SDK_LIB=C:\Program Files (x86)\Windows Kits\10\Lib\%SDK_VER%"

if not exist "%CL%" (
    echo  [ERROR] MSVC cl.exe not found at:
    echo          %CL%
    echo  Please update MSVC_VER in this script.
    pause
    exit /b 1
)

if not exist "%RC_EXE%" (
    echo  [ERROR] rc.exe not found at:
    echo          %RC_EXE%
    pause
    exit /b 1
)

echo  [OK] MSVC found: %CL%
echo  [OK] RC found:   %RC_EXE%

:: ---------------------------------------------------------------------------
:: Create output directory
:: ---------------------------------------------------------------------------
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

:: Copy assets to dist so the exe can find them at runtime
if not exist "%OUT_DIR%\assets" mkdir "%OUT_DIR%\assets"
copy /Y "%ASSETS_DIR%\logo_256.png" "%OUT_DIR%\assets\" >nul 2>&1
copy /Y "%ASSETS_DIR%\logo_64.png"  "%OUT_DIR%\assets\" >nul 2>&1
echo  [OK] Assets copied to dist\assets\

:: ---------------------------------------------------------------------------
:: Compile resource file
:: ---------------------------------------------------------------------------
echo.
echo  [1/3] Compiling resource file...

set "RC_INCLUDES=/I "%SDK_INC%\um" /I "%SDK_INC%\shared""
"%RC_EXE%" %RC_INCLUDES% /nologo /fo "%OUT_DIR%\installer.res" "%SRC_DIR%\installer.rc"
if %errorlevel% neq 0 (
    echo  [ERROR] Resource compilation failed.
    pause
    exit /b %errorlevel%
)
echo  [OK] Resources compiled.

:: ---------------------------------------------------------------------------
:: Compile main C++ source
:: ---------------------------------------------------------------------------
echo.
echo  [2/3] Compiling installer_main.cpp...

set "INCLUDES=/I "%MSVC_INC%" /I "%SDK_INC%\ucrt" /I "%SDK_INC%\um" /I "%SDK_INC%\shared" /I "%SDK_INC%\winrt""
set "CFLAGS=/nologo /W3 /WX- /O2 /Oi /GL /DNDEBUG /DUNICODE /D_UNICODE /EHsc /MD /std:c++17"

"%CL%" %CFLAGS% %INCLUDES% ^
    /Fo"%OUT_DIR%\installer.obj" ^
    /c "%SRC_DIR%\installer_main.cpp"

if %errorlevel% neq 0 (
    echo  [ERROR] C++ compilation failed.
    pause
    exit /b %errorlevel%
)
echo  [OK] C++ compiled.

:: ---------------------------------------------------------------------------
:: Link the executable
:: ---------------------------------------------------------------------------
echo.
echo  [3/3] Linking NirqobaSetup.exe...

set "LIBS=user32.lib gdi32.lib kernel32.lib shell32.lib shlwapi.lib"
set "LIBS=%LIBS% d2d1.lib dwrite.lib windowscodecs.lib dwmapi.lib"
set "LIBS=%LIBS% ole32.lib comctl32.lib comdlg32.lib uxtheme.lib"

set "LIB_PATHS=/LIBPATH:"%MSVC_LIB%" /LIBPATH:"%SDK_LIB%\um\x64" /LIBPATH:"%SDK_LIB%\ucrt\x64""

set "LFLAGS=/nologo /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS /MACHINE:X64"
set "LFLAGS=%LFLAGS% /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'""

"%LINK%" %LFLAGS% %LIB_PATHS% %LIBS% ^
    /OUT:"%OUT_DIR%\NirqobaSetup.exe" ^
    "%OUT_DIR%\installer.obj" ^
    "%OUT_DIR%\installer.res"

if %errorlevel% neq 0 (
    echo  [ERROR] Linking failed.
    pause
    exit /b %errorlevel%
)

:: ---------------------------------------------------------------------------
:: Done
:: ---------------------------------------------------------------------------
echo.
echo  ╔══════════════════════════════════════╗
echo  ║   Build successful!                  ║
echo  ╚══════════════════════════════════════╝
echo.
echo  Output: %OUT_DIR%\NirqobaSetup.exe

for %%F in ("%OUT_DIR%\NirqobaSetup.exe") do (
    set /a SIZE=%%~zF / 1024
    echo  Size:   !SIZE! KB
)

echo.
echo  Run: %OUT_DIR%\NirqobaSetup.exe
echo.
pause
