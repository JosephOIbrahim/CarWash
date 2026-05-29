@echo off
REM HdCarWash Rebuild and Install Script
REM =====================================
REM Rebuilds the plugin and installs to Houdini 21

title HdCarWash Build
color 1F

echo.
echo     =============================================
echo          HdCarWash Rebuild and Install
echo     =============================================
echo.

REM HFS = Houdini install root. Honor an existing HFS (set by Houdini's own
REM environment / hcmd shell); otherwise fall back to a default install.
REM Override by setting HFS before running this script, e.g.:
REM   set "HFS=C:\Program Files\Side Effects Software\Houdini 21.0.640"
if not defined HFS set "HFS=C:\Program Files\Side Effects Software\Houdini 21.0.607"
set "HOUDINI_PATH=%HFS%"
set "BUILD_DIR=%~dp0build"

REM Install PREFIX is the Houdini user dir. CMake's install() rules place the
REM plugin under <prefix>\dso\... so plugInfo.json + lib\hdCarWash.dll land
REM together as a discoverable pxr plugin. Override HOUDINI_USER_PREF_DIR to
REM target a different Houdini version dir.
if not defined HOUDINI_USER_PREF_DIR set "HOUDINI_USER_PREF_DIR=%USERPROFILE%\houdini21.0"
set "INSTALL_PREFIX=%HOUDINI_USER_PREF_DIR%\dso"

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

REM Install via CMake so plugInfo.json AND the dll deploy together using the
REM project's install() rules. This deploys:
REM   %INSTALL_PREFIX%\usd\hdCarWash\plugInfo.json   (the pxr manifest)
REM   %INSTALL_PREFIX%\usd\hdCarWash\lib\hdCarWash.dll (LibraryPath target)
echo [INFO] Installing to: %INSTALL_PREFIX%\usd\hdCarWash

cmake --install . --config Release --prefix "%INSTALL_PREFIX%"
if errorlevel 1 (
    echo [ERROR] Install failed!
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
