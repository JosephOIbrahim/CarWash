@echo off
REM Launch Houdini and build CarWash test scene
REM =============================================

echo.
echo CarWash Test Scene Launcher
echo ===========================
echo.

set HOUDINI_PATH=C:\Program Files\Side Effects Software\Houdini 21.0.729

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
echo   runpy.run_path(r"C:\Users\User\Downloads\HDCARWAASH\HdCarWash\test\build_carwash_scene.py")
echo.

start "" "%HOUDINI_PATH%\bin\houdini.exe"
