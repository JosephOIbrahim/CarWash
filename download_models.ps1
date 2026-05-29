# CarWash ComfyUI Model Downloader
# Run as: powershell -ExecutionPolicy Bypass -File download_models.ps1

$ErrorActionPreference = "Continue"
$ComfyDir = "C:\ComfyUI"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "   CarWash Model Downloader" -ForegroundColor Cyan
Write-Host "   Downloading required AI models..." -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Create directories
$dirs = @(
    "$ComfyDir\models\animatediff_models",
    "$ComfyDir\models\controlnet",
    "$ComfyDir\models\ipadapter",
    "$ComfyDir\models\clip_vision",
    "$ComfyDir\models\checkpoints"
)

foreach ($dir in $dirs) {
    if (!(Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host "Created: $dir" -ForegroundColor Green
    }
}

# Download function with progress
function Download-Model {
    param (
        [string]$Url,
        [string]$Output,
        [string]$Name
    )

    if (Test-Path $Output) {
        Write-Host "[SKIP] $Name already exists" -ForegroundColor Yellow
        return
    }

    Write-Host "[DOWNLOADING] $Name..." -ForegroundColor Cyan
    Write-Host "  URL: $Url" -ForegroundColor Gray
    Write-Host "  Destination: $Output" -ForegroundColor Gray

    try {
        $ProgressPreference = 'SilentlyContinue'
        Invoke-WebRequest -Uri $Url -OutFile $Output -UseBasicParsing
        $size = (Get-Item $Output).Length / 1MB
        Write-Host "[OK] Downloaded $Name ({0:N0} MB)" -f $size -ForegroundColor Green
    }
    catch {
        Write-Host "[ERROR] Failed to download $Name : $_" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "[1/6] AnimateDiff Motion Model (mm_sd_v15_v2.ckpt)..." -ForegroundColor White
Download-Model `
    -Url "https://huggingface.co/guoyww/animatediff/resolve/main/mm_sd_v15_v2.ckpt" `
    -Output "$ComfyDir\models\animatediff_models\mm_sd_v15_v2.ckpt" `
    -Name "AnimateDiff v2"

Write-Host ""
Write-Host "[2/6] ControlNet Depth..." -ForegroundColor White
Download-Model `
    -Url "https://huggingface.co/lllyasviel/ControlNet-v1-1/resolve/main/control_v11f1p_sd15_depth.pth" `
    -Output "$ComfyDir\models\controlnet\control_v11f1p_sd15_depth.pth" `
    -Name "ControlNet Depth"

Write-Host ""
Write-Host "[3/6] ControlNet Normal..." -ForegroundColor White
Download-Model `
    -Url "https://huggingface.co/lllyasviel/ControlNet-v1-1/resolve/main/control_v11p_sd15_normalbae.pth" `
    -Output "$ComfyDir\models\controlnet\control_v11p_sd15_normalbae.pth" `
    -Name "ControlNet Normal"

Write-Host ""
Write-Host "[4/6] IP-Adapter Plus..." -ForegroundColor White
Download-Model `
    -Url "https://huggingface.co/h94/IP-Adapter/resolve/main/models/ip-adapter-plus_sd15.safetensors" `
    -Output "$ComfyDir\models\ipadapter\ip-adapter-plus_sd15.safetensors" `
    -Name "IP-Adapter Plus"

Write-Host ""
Write-Host "[5/6] CLIP Vision Encoder..." -ForegroundColor White
Download-Model `
    -Url "https://huggingface.co/h94/IP-Adapter/resolve/main/models/image_encoder/model.safetensors" `
    -Output "$ComfyDir\models\clip_vision\CLIP-ViT-H-14-laion2B-s32B-b79K.safetensors" `
    -Name "CLIP Vision"

Write-Host ""
Write-Host "[6/6] Stable Diffusion 1.5 Base..." -ForegroundColor White
# Using a reliable mirror for SD 1.5
Download-Model `
    -Url "https://huggingface.co/runwayml/stable-diffusion-v1-5/resolve/main/v1-5-pruned-emaonly.safetensors" `
    -Output "$ComfyDir\models\checkpoints\v1-5-pruned-emaonly.safetensors" `
    -Name "SD 1.5"

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "   Download Complete!" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Models installed in: $ComfyDir\models\" -ForegroundColor White
Write-Host ""
Write-Host "To start ComfyUI with CarWash support:" -ForegroundColor Yellow
Write-Host "  cd $ComfyDir" -ForegroundColor White
Write-Host "  python main.py --listen" -ForegroundColor White
Write-Host ""
Write-Host "Then in Houdini, set CarWash Backend:" -ForegroundColor Yellow
Write-Host "  Host: localhost" -ForegroundColor White
Write-Host "  Port: 8188" -ForegroundColor White
Write-Host ""

Read-Host "Press Enter to exit"
