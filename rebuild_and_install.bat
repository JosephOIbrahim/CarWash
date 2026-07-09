@echo off
REM HdCarWash Rebuild and Install Script
REM =====================================
REM Rebuilds the plugin and installs to the active Houdini install.
REM Auto-detects the newest Houdini under "C:\Program Files\Side Effects
REM Software"; override with:  set HOUDINI_VERSION=22.0.500  before running.

setlocal enabledelayedexpansion

title HdCarWash Build
color 1F

echo.
echo     =============================================
echo          HdCarWash Rebuild and Install
echo     =============================================
echo.

set "HOUDINI_BASE=C:\Program Files\Side Effects Software"

REM Resolve Houdini version (override or newest install)
if not defined HOUDINI_VERSION (
    set "LATEST="
    for /f "delims=" %%D in ('dir /b /ad /o:n "%HOUDINI_BASE%\Houdini *" 2^>NUL ^| findstr /c:"."') do set "LATEST=%%D"
    if not defined LATEST (
        echo [ERROR] No Houdini install found under %HOUDINI_BASE%
        pause
        exit /b 1
    )
    set "HOUDINI_VERSION=!LATEST:Houdini =!"
)
set "HOUDINI_PATH=%HOUDINI_BASE%\Houdini %HOUDINI_VERSION%"

REM Derive houdini<major>.<minor> user-pref directory
for /f "tokens=1,2 delims=." %%a in ("%HOUDINI_VERSION%") do set "HOUDINI_MAJORMINOR=%%a.%%b"
set "INSTALL_DIR=%USERPROFILE%\houdini%HOUDINI_MAJORMINOR%\dso\usd\hdCarWash"

echo     [INFO] Houdini version: %HOUDINI_VERSION%
echo     [INFO] Install target:  %INSTALL_DIR%
echo.

REM Check if Houdini is running
tasklist /FI "IMAGENAME eq houdini.exe" 2>NUL | find /I /N "houdini.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo [WARNING] Houdini is running - DLL may be locked!
    echo           Close Houdini before rebuilding.
    echo.
    pause
    exit /b 1
)

echo [INFO] Houdini not running - safe to build
echo.

REM Clear debug log
if exist "C:\Temp\hdcarwash_debug.txt" (
    del "C:\Temp\hdcarwash_debug.txt"
    echo [INFO] Cleared debug log
)

REM Navigate to build directory
cd /d "%BUILD_DIR%"
if errorlevel 1 (
    echo [ERROR] Build directory not found: %BUILD_DIR%
    echo         Run CMake configure first
    pause
    exit /b 1
)

echo [INFO] Building in: %BUILD_DIR%
echo.

REM Build with Visual Studio
cmake --build . --config Release --parallel
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Build completed!
echo.

REM Install
echo [INFO] Installing to: %INSTALL_DIR%

if not exist "%INSTALL_DIR%\lib" mkdir "%INSTALL_DIR%\lib"

copy /Y "%BUILD_DIR%\plugin\hdCarWash\Release\hdCarWash.dll" "%INSTALL_DIR%\lib\" >NUL
if errorlevel 1 (
    echo [ERROR] Failed to copy DLL
    pause
    exit /b 1
)

echo [SUCCESS] Plugin installed!
echo.
echo     =============================================
echo          Ready to test in Houdini
echo     =============================================
echo.
echo     1. Launch Houdini
echo     2. Select "CarWash (AI NPR)" renderer
echo     3. Check: C:\Temp\hdcarwash_debug.txt
echo.
pause
