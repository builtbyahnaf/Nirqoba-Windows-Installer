@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "INSTALLER_ROOT=%%~fI"
set "SRC_DIR=%INSTALLER_ROOT%\src"
set "OUT_DIR=%INSTALLER_ROOT%\dist"
set "ASSETS_DIR=%INSTALLER_ROOT%\assets"
set "PKG_DIR=%OUT_DIR%\package"

set "VS_ROOT=C:\Program Files\Microsoft Visual Studio\18\BuildTools"
set "MSVC_VER=14.51.36231"
set "CL_EXE=%VS_ROOT%\VC\Tools\MSVC\%MSVC_VER%\bin\Hostx64\x64\cl.exe"
set "LINK_EXE=%VS_ROOT%\VC\Tools\MSVC\%MSVC_VER%\bin\Hostx64\x64\link.exe"
set "MSVC_INC=%VS_ROOT%\VC\Tools\MSVC\%MSVC_VER%\include"
set "MSVC_LIB=%VS_ROOT%\VC\Tools\MSVC\%MSVC_VER%\lib\x64"
set "SDK_VER=10.0.26100.0"
set "SDK_ROOT=C:\Program Files (x86)\Windows Kits\10"
set "RC_EXE=%SDK_ROOT%\bin\%SDK_VER%\x64\rc.exe"
set "SDK_INC=%SDK_ROOT%\Include\%SDK_VER%"
set "SDK_LIB=%SDK_ROOT%\Lib\%SDK_VER%"
set "PATH=%SDK_ROOT%\bin\%SDK_VER%\x64;%PATH%"

if not exist "%CL_EXE%" (
    echo [ERROR] MSVC compiler not found: "%CL_EXE%"
    exit /b 1
)
if not exist "%LINK_EXE%" (
    echo [ERROR] MSVC linker not found: "%LINK_EXE%"
    exit /b 1
)
if not exist "%RC_EXE%" (
    echo [ERROR] Windows resource compiler not found: "%RC_EXE%"
    exit /b 1
)
if not exist "%ASSETS_DIR%\Nirqoba logo.png" (
    echo [ERROR] Required logo not found: "%ASSETS_DIR%\Nirqoba logo.png"
    exit /b 1
)

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
if not exist "%OUT_DIR%\assets" mkdir "%OUT_DIR%\assets"
copy /Y "%ASSETS_DIR%\Nirqoba logo.png" "%OUT_DIR%\assets\Nirqoba logo.png" >nul

echo [1/5] Bundling latest compiler from assets...
if not exist "%PKG_DIR%\bin" mkdir "%PKG_DIR%\bin"
if exist "%ASSETS_DIR%\nirqoba.exe" (
    copy /Y "%ASSETS_DIR%\nirqoba.exe" "%PKG_DIR%\bin\nirqoba.exe" >nul
    echo       Updated %PKG_DIR%\bin\nirqoba.exe from assets
) else (
    echo [ERROR] %ASSETS_DIR%\nirqoba.exe not found!
    exit /b 1
)

echo [2/5] Compiling and linking uninstaller (uninstall.exe)...
"%RC_EXE%" /I "%SRC_DIR%" /I "%ASSETS_DIR%" /I "%SDK_INC%\um" /I "%SDK_INC%\shared" /nologo /fo "%OUT_DIR%\uninstall.res" "%SRC_DIR%\uninstall.rc"
if errorlevel 1 (
    echo [ERROR] Uninstaller resource compilation failed.
    exit /b 1
)

"%CL_EXE%" /nologo /W3 /O2 /Oi /DNDEBUG /DUNICODE /D_UNICODE /utf-8 /EHsc /MD /std:c++17 /I"%MSVC_INC%" /I"%SDK_INC%\ucrt" /I"%SDK_INC%\um" /I"%SDK_INC%\shared" /I"%SDK_INC%\winrt" /Fo"%OUT_DIR%\uninstall.obj" /c "%SRC_DIR%\uninstall_main.cpp"
if errorlevel 1 (
    echo [ERROR] Uninstaller compilation failed.
    exit /b 1
)

"%LINK_EXE%" /nologo /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS /MACHINE:X64 /LIBPATH:"%MSVC_LIB%" /LIBPATH:"%SDK_LIB%\um\x64" /LIBPATH:"%SDK_LIB%\ucrt\x64" user32.lib gdi32.lib kernel32.lib shell32.lib shlwapi.lib advapi32.lib d2d1.lib dwrite.lib windowscodecs.lib dwmapi.lib ole32.lib uxtheme.lib /OUT:"%PKG_DIR%\uninstall.exe" "%OUT_DIR%\uninstall.obj" "%OUT_DIR%\uninstall.res"
if errorlevel 1 (
    echo [ERROR] Uninstaller linking failed.
    exit /b 1
)

echo [3/5] Packing payload package into binary bundle...
python "%INSTALLER_ROOT%\build_scripts\pack_payload.py" "%PKG_DIR%" "%OUT_DIR%\payload.bin"
if errorlevel 1 (
    echo [ERROR] Payload packing failed.
    exit /b 1
)

echo [4/5] Compiling installer resources...
"%RC_EXE%" /I "%SRC_DIR%" /I "%OUT_DIR%" /I "%SDK_INC%\um" /I "%SDK_INC%\shared" /nologo /fo "%OUT_DIR%\installer.res" "%SRC_DIR%\installer.rc"
if errorlevel 1 (
    echo [ERROR] Resource compilation failed.
    exit /b 1
)

echo [5/5] Compiling and linking NirqobaSetup.exe...
"%CL_EXE%" /nologo /W3 /WX- /O2 /Oi /GL /DNDEBUG /DUNICODE /D_UNICODE /utf-8 /EHsc /MD /std:c++17 /I"%MSVC_INC%" /I"%SDK_INC%\ucrt" /I"%SDK_INC%\um" /I"%SDK_INC%\shared" /I"%SDK_INC%\winrt" /Fo"%OUT_DIR%\installer.obj" /c "%SRC_DIR%\installer_main.cpp"
if errorlevel 1 (
    echo [ERROR] Installer UI compilation failed.
    exit /b 1
)

"%LINK_EXE%" /nologo /LTCG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS /MACHINE:X64 /LIBPATH:"%MSVC_LIB%" /LIBPATH:"%SDK_LIB%\um\x64" /LIBPATH:"%SDK_LIB%\ucrt\x64" user32.lib gdi32.lib kernel32.lib shell32.lib shlwapi.lib advapi32.lib d2d1.lib dwrite.lib windowscodecs.lib dwmapi.lib ole32.lib comctl32.lib comdlg32.lib uxtheme.lib /OUT:"%OUT_DIR%\NirqobaSetup.exe" "%OUT_DIR%\installer.obj" "%OUT_DIR%\installer.res"
if errorlevel 1 (
    echo [ERROR] Installer linking failed.
    exit /b 1
)

echo.
echo ============================================================
echo   Build Successful!
echo   Installer: %OUT_DIR%\NirqobaSetup.exe
for %%F in ("%OUT_DIR%\NirqobaSetup.exe") do echo   Size:      %%~zF bytes
echo ============================================================
exit /b 0
