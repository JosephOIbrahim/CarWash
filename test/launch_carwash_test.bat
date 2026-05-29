@echo off
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

set HOUDINI_PATH=C:\Program Files\Side Effects Software\Houdini 21.0.729

echo     Checking installation...
echo.

if not exist "%HOUDINI_PATH%\bin\houdini.exe" (
    echo     [ERROR] Houdini not found at:
    echo             %HOUDINI_PATH%
    echo.
    pause
    exit /b 1
)

echo     [OK] Houdini 21.0.729

set CARWASH_DLL=%USERPROFILE%\houdini21.0\dso\usd\hdCarWash\lib\hdCarWash.dll
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
