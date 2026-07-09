@echo off
setlocal enabledelayedexpansion
title HdCarWash Test Environment
color 1F

echo.
echo     ╔═══════════════════════════════════════════╗
echo     ║                                           ║
echo     ║        ≋≋≋ HdCarWash v0.1 ≋≋≋            ║
echo     ║         AI Render Delegate                ║
echo     ║        Phase 1: CPU Rasterizer            ║
echo     ║                                           ║
echo     ╚═══════════════════════════════════════════╝
echo.

set "HOUDINI_BASE=C:\Program Files\Side Effects Software"
if not defined HOUDINI_VERSION (
    set "LATEST="
    for /f "delims=" %%D in ('dir /b /ad /o:n "%HOUDINI_BASE%\Houdini *" 2^>NUL ^| findstr /c:"."') do set "LATEST=%%D"
    if not defined LATEST (
        echo     [ERROR] No Houdini install found under %HOUDINI_BASE%
        pause
        exit /b 1
    )
    set "HOUDINI_VERSION=!LATEST:Houdini =!"
)
set "HOUDINI_PATH=%HOUDINI_BASE%\Houdini %HOUDINI_VERSION%"
for /f "tokens=1,2 delims=." %%a in ("%HOUDINI_VERSION%") do set "HOUDINI_MAJORMINOR=%%a.%%b"

echo     Checking installation...
echo.

if not exist "%HOUDINI_PATH%\bin\houdini.exe" (
    echo     [ERROR] Houdini not found at:
    echo             %HOUDINI_PATH%
    echo.
    pause
    exit /b 1
)

echo     [OK] Houdini %HOUDINI_VERSION%

set CARWASH_DLL=%USERPROFILE%\houdini%HOUDINI_MAJORMINOR%\dso\usd\hdCarWash\lib\hdCarWash.dll
if exist "%CARWASH_DLL%" (
    echo     [OK] HdCarWash plugin installed
) else (
    echo     [!!] Plugin DLL not found - may need install
)

echo.
echo     ╭─────────────────────────────────────────────╮
echo     │  TEST INSTRUCTIONS                          │
echo     ├─────────────────────────────────────────────┤
echo     │  1. Houdini will launch                     │
echo     │  2. Windows menu → Python Shell             │
echo     │  3. Paste this command:                     │
echo     │                                             │
echo     │  exec(open(r"%~dp0test_carwash_render.py").read())
echo     │                                             │
echo     │  4. Watch for "CarWash (AI NPR)" renderer   │
echo     │  5. Check if blue sphere appears            │
echo     ╰─────────────────────────────────────────────╯
echo.
echo     Press any key to launch Houdini...
pause >nul

echo.
echo     Launching Houdini...
echo.

start "" "%HOUDINI_PATH%\bin\houdini.exe"

echo     ≋≋≋ Houdini launched ≋≋≋
echo.
echo     Run the test script in Python Shell.
echo     Report results to continue development.
echo.
pause
