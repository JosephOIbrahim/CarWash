@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo    CarWash ComfyUI Setup
echo    Installing ComfyUI + Video Extensions
echo ============================================================
echo.

:: Configuration
set INSTALL_DIR=C:\ComfyUI
set PYTHON_VERSION=3.11

:: Check for Git
where git >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Git not found. Please install Git first.
    echo Download from: https://git-scm.com/download/win
    pause
    exit /b 1
)

:: Check for Python
where python >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python not found. Please install Python 3.10+ first.
    pause
    exit /b 1
)

echo [1/8] Cloning ComfyUI...
if exist "%INSTALL_DIR%" (
    echo ComfyUI directory exists, updating...
    cd /d "%INSTALL_DIR%"
    git pull
) else (
    git clone https://github.com/comfyanonymous/ComfyUI.git "%INSTALL_DIR%"
    cd /d "%INSTALL_DIR%"
)

echo.
echo [2/8] Installing ComfyUI dependencies...
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121
pip install -r requirements.txt

echo.
echo [3/8] Installing AnimateDiff-Evolved (video generation)...
cd custom_nodes
if exist "ComfyUI-AnimateDiff-Evolved" (
    cd ComfyUI-AnimateDiff-Evolved
    git pull
    cd ..
) else (
    git clone https://github.com/Kosinkadink/ComfyUI-AnimateDiff-Evolved.git
)
pip install -r ComfyUI-AnimateDiff-Evolved\requirements.txt 2>nul

echo.
echo [4/8] Installing VideoHelperSuite (video I/O)...
if exist "ComfyUI-VideoHelperSuite" (
    cd ComfyUI-VideoHelperSuite
    git pull
    cd ..
) else (
    git clone https://github.com/Kosinkadink/ComfyUI-VideoHelperSuite.git
)
pip install -r ComfyUI-VideoHelperSuite\requirements.txt 2>nul

echo.
echo [5/8] Installing Advanced-ControlNet (temporal control)...
if exist "ComfyUI-Advanced-ControlNet" (
    cd ComfyUI-Advanced-ControlNet
    git pull
    cd ..
) else (
    git clone https://github.com/Fannovel16/ComfyUI-Advanced-ControlNet.git
)
pip install -r ComfyUI-Advanced-ControlNet\requirements.txt 2>nul

echo.
echo [6/8] Installing IP-Adapter Plus (style memory)...
if exist "ComfyUI_IPAdapter_plus" (
    cd ComfyUI_IPAdapter_plus
    git pull
    cd ..
) else (
    git clone https://github.com/cubiq/ComfyUI_IPAdapter_plus.git
)

echo.
echo [7/8] Installing ControlNet Aux (preprocessors)...
if exist "comfyui_controlnet_aux" (
    cd comfyui_controlnet_aux
    git pull
    cd ..
) else (
    git clone https://github.com/Fannovel16/comfyui_controlnet_aux.git
)
pip install -r comfyui_controlnet_aux\requirements.txt 2>nul

cd ..

echo.
echo [8/8] Creating model directories...
if not exist "models\animatediff_models" mkdir models\animatediff_models
if not exist "models\controlnet" mkdir models\controlnet
if not exist "models\ipadapter" mkdir models\ipadapter
if not exist "models\clip_vision" mkdir models\clip_vision

echo.
echo ============================================================
echo    Setup Complete!
echo ============================================================
echo.
echo ComfyUI installed at: %INSTALL_DIR%
echo.
echo NEXT STEPS - Download these models:
echo.
echo 1. AnimateDiff Motion Model:
echo    https://huggingface.co/guoyww/animatediff/resolve/main/mm_sd_v15_v2.ckpt
echo    Save to: %INSTALL_DIR%\models\animatediff_models\
echo.
echo 2. ControlNet Depth:
echo    https://huggingface.co/lllyasviel/ControlNet-v1-1/resolve/main/control_v11f1p_sd15_depth.pth
echo    Save to: %INSTALL_DIR%\models\controlnet\
echo.
echo 3. ControlNet Normal:
echo    https://huggingface.co/lllyasviel/ControlNet-v1-1/resolve/main/control_v11p_sd15_normalbae.pth
echo    Save to: %INSTALL_DIR%\models\controlnet\
echo.
echo 4. IP-Adapter:
echo    https://huggingface.co/h94/IP-Adapter/resolve/main/models/ip-adapter-plus_sd15.bin
echo    Save to: %INSTALL_DIR%\models\ipadapter\
echo.
echo 5. CLIP Vision (for IP-Adapter):
echo    https://huggingface.co/h94/IP-Adapter/resolve/main/models/image_encoder/model.safetensors
echo    Save to: %INSTALL_DIR%\models\clip_vision\ (rename to CLIP-ViT-H-14-laion2B-s32B-b79K.safetensors)
echo.
echo 6. Any SD 1.5 Checkpoint (e.g., Realistic Vision):
echo    https://civitai.com/models/4201/realistic-vision-v51
echo    Save to: %INSTALL_DIR%\models\checkpoints\
echo.
echo To start ComfyUI:
echo    cd %INSTALL_DIR%
echo    python main.py --listen
echo.
echo ============================================================
pause
