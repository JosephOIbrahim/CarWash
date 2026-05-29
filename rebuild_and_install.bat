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

set HOUDINI_PATH=C:\Program Files\Side Effects Software\Houdini 21.0.729
set BUILD_DIR=%~dp0build
set INSTALL_DIR=%USERPROFILE%\houdini21.0\dso\usd\hdCarWash

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
