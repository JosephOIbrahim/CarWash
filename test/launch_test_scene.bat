@echo off
setlocal enabledelayedexpansion
REM Launch Houdini and build CarWash test scene
REM =============================================
REM Auto-detects the newest Houdini install; override with HOUDINI_VERSION.

echo.
echo CarWash Test Scene Launcher
echo ===========================
echo.

set "HOUDINI_BASE=C:\Program Files\Side Effects Software"
if not defined HOUDINI_VERSION (
    set "LATEST="
    for /f "delims=" %%D in ('dir /b /ad /o:n "%HOUDINI_BASE%\Houdini *" 2^>NUL ^| findstr /c:"."') do set "LATEST=%%D"
    if not defined LATEST (
        echo ERROR: No Houdini install found under %HOUDINI_BASE%
        pause
        exit /b 1
    )
    set "HOUDINI_VERSION=!LATEST:Houdini =!"
)
set "HOUDINI_PATH=%HOUDINI_BASE%\Houdini %HOUDINI_VERSION%"

REM Check if Houdini exists
if not exist "%HOUDINI_PATH%\bin\houdini.exe" (
    echo ERROR: Houdini not found at %HOUDINI_PATH%
    pause
    exit /b 1
)

echo Launching Houdini with test scene builder...
echo.
echo In Houdini Python Shell, run:
echo   import runpy
echo   runpy.run_path(r"%~dp0build_carwash_scene.py")
echo.

start "" "%HOUDINI_PATH%\bin\houdini.exe"
