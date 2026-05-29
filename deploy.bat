@echo off
REM ============================================
REM HdCarWash Deployment Wrapper
REM ============================================
REM One-click deployment for Houdini 21
REM
REM Usage:
REM   deploy.bat              Full build and deploy
REM   deploy.bat --skip-build Deploy only (no build)
REM   deploy.bat --verbose    Verbose output
REM ============================================

title HdCarWash Deploy

echo.
echo     =============================================
echo          HdCarWash Automated Deployment
echo     =============================================
echo.

REM Run the Python deployment script with all passed arguments
python "%~dp0deploy_hdcarwash.py" %*

REM Check for errors
if errorlevel 1 (
    echo.
    echo [Press any key to exit...]
    pause >nul
    exit /b 1
)

echo.
echo [Press any key to exit...]
pause >nul
exit /b 0
