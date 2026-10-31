@echo off
:: Build the nirqoba GUI application using MSVC
:: This creates a real .exe Windows application

set "PROJECT_DIR=%~dp0.."
set "BUILD_DIR=%PROJECT_DIR%\build\msvc-dev"
set "OUTPUT_DIR=%PROJECT_DIR%\windows_installer\dist"

echo.
echo ============================================
echo Building nirqoba GUI Application
echo ============================================
echo.

:: Check if MSVC environment is available
where cl.exe >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: MSVC compiler (cl.exe) not found in PATH.
    echo Please set up the MSVC environment or use the Developer Command Prompt.
    pause
    exit /b 1
)

echo Found MSVC compiler.

:: Create output directory
mkdir "%OUTPUT_DIR%" 2>nul

:: Compile the GUI application
echo Compiling nirqoba_gui.cpp...
cl.exe /nologo /W3 /O2 /DNDEBUG ^
    /EHsc /MD ^
    /Fe"%OUTPUT_DIR%\nirqoba_gui.exe" ^
    /link user32.lib gdi32.lib kernel32.lib ^
    "%PROJECT_DIR%\windows_installer\src\nirqoba_gui.cpp"

if %errorlevel% neq 0 (
    echo.
    echo Error: Compilation failed with error code %errorlevel%
    echo.
    pause
    exit /b %errorlevel%
)

echo.
echo Compilation successful!
echo.

:: Verify the .exe was created
if exist "%OUTPUT_DIR%\nirqoba_gui.exe" (
    echo Output: %OUTPUT_DIR%\nirqoba_gui.exe
    echo Size: %~zxnirqoba_gui.exe bytes
    echo.
    
    :: Check file version
    ver.exe "%OUTPUT_DIR%\nirqoba_gui.exe" 2>nul | find "File Version"
    
    :: List imports (basic check)
    dumpbin /IMPORTS "%OUTPUT_DIR%\nirqoba_gui.exe" | find "kernel32" >nul && echo "  - Imports kernel32.dll" || echo "  - No kernel32 import detected"
    dumpbin /IMPORTS "%OUTPUT_DIR%\nirqoba_gui.exe" | find "user32" >nul && echo "  - Imports user32.dll" || echo "  - No user32 import detected"
    
    echo.
    echo GUI application build complete!
} else (
    echo Error: Output executable not created
    pause
    exit /b 1
)

echo.
echo ============================================
echo Build complete.
echo ============================================
pause