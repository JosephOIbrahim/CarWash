# CarWash Model Installer — LTX-2.3 22B Distilled
# Run as: powershell -ExecutionPolicy Bypass -File download_models.ps1
#
# Models required for the LTX-2.3 22B distilled workflow:
#
#   UNET (transformer-only, fp8):
#     ltx-2.3-22b-distilled_transformer_only_fp8_input_scaled_v3.safetensors
#     → ComfyUI/models/diffusion_models/
#
#   Checkpoint (config + tokenizer for LTXAVTextEncoderLoader):
#     ltx-2.3-22b-distilled-fp8.safetensors
#     → ComfyUI/models/checkpoints/
#
#   Text encoder (Gemma 3 12B fp4 — runs on CPU):
#     gemma_3_12B_it_fp4_mixed.safetensors
#     → ComfyUI/models/text_encoders/
#
#   VAE (full 32x spatial, bf16):
#     LTX23_video_vae_bf16.safetensors
#     → ComfyUI/models/vae/
#
# Source: https://huggingface.co/Lightricks/LTX-Video
#         https://huggingface.co/Lightricks/LTX-Video-0.9.7-distilled

$ErrorActionPreference = "Continue"

# ── Configuration ────────────────────────────────────────────────────────────
# Adjust $ComfyDir to match your ComfyUI installation.
$ComfyDir = "G:\COMFY\ComfyUI"
$ModelBase = "G:\COMFYUI_Database\Models"   # extra_model_paths.yaml base_path

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "   CarWash Model Installer — LTX-2.3 22B Distilled" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "ComfyUI dir : $ComfyDir" -ForegroundColor Gray
Write-Host "Model base  : $ModelBase" -ForegroundColor Gray
Write-Host ""

# ── Directory setup ───────────────────────────────────────────────────────────
$dirs = @(
    "$ModelBase\diffusion_models",
    "$ModelBase\checkpoints",
    "$ModelBase\text_encoders",
    "$ModelBase\vae"
)

foreach ($dir in $dirs) {
    if (!(Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
        Write-Host "[CREATED] $dir" -ForegroundColor Green
    }
}

# ── Download helper ───────────────────────────────────────────────────────────
function Download-HF {
    param (
        [string]$Repo,     # e.g. "Lightricks/LTX-Video-0.9.7-distilled"
        [string]$File,     # path within the repo, e.g. "ltx-2.3-22b-distilled-fp8.safetensors"
        [string]$Output,   # absolute destination path
        [string]$Label
    )

    if (Test-Path $Output) {
        $mb = [math]::Round((Get-Item $Output).Length / 1MB)
        Write-Host "[SKIP] $Label already present ($mb MB)" -ForegroundColor Yellow
        return
    }

    # Prefer huggingface-cli if available (handles auth + resumable downloads)
    $hfcli = (Get-Command "huggingface-cli" -ErrorAction SilentlyContinue)
    if ($hfcli) {
        Write-Host "[DL] $Label (huggingface-cli)..." -ForegroundColor Cyan
        huggingface-cli download $Repo $File --local-dir (Split-Path $Output)
        return
    }

    # Fall back to direct URL
    $Url = "https://huggingface.co/$Repo/resolve/main/$File"
    Write-Host "[DL] $Label..." -ForegroundColor Cyan
    Write-Host "     $Url" -ForegroundColor Gray
    try {
        $ProgressPreference = 'SilentlyContinue'
        Invoke-WebRequest -Uri $Url -OutFile $Output -UseBasicParsing
        $mb = [math]::Round((Get-Item $Output).Length / 1MB)
        Write-Host "[OK] $Label — $mb MB" -ForegroundColor Green
    } catch {
        Write-Host "[ERR] $Label : $_" -ForegroundColor Red
        Write-Host "      Download manually from: https://huggingface.co/$Repo" -ForegroundColor Yellow
    }
}

# ── Models ────────────────────────────────────────────────────────────────────

Write-Host "[1/4] UNET — LTX-2.3 22B transformer-only fp8 (~22 GB)" -ForegroundColor White
Download-HF `
    -Repo  "Lightricks/LTX-Video-0.9.7-distilled" `
    -File  "ltx-2.3-22b-distilled_transformer_only_fp8_input_scaled_v3.safetensors" `
    -Output "$ModelBase\diffusion_models\ltx-2.3-22b-distilled_transformer_only_fp8_input_scaled_v3.safetensors" `
    -Label "LTX-2.3 22B UNET (fp8)"

Write-Host ""
Write-Host "[2/4] Checkpoint — LTX-2.3 22B distilled fp8 (config + tokenizer, ~22 GB)" -ForegroundColor White
Download-HF `
    -Repo  "Lightricks/LTX-Video-0.9.7-distilled" `
    -File  "ltx-2.3-22b-distilled-fp8.safetensors" `
    -Output "$ModelBase\checkpoints\ltx-2.3-22b-distilled-fp8.safetensors" `
    -Label "LTX-2.3 22B checkpoint"

Write-Host ""
Write-Host "[3/4] Text encoder — Gemma 3 12B fp4 mixed (~12 GB, runs on CPU)" -ForegroundColor White
# Note: gemma_3_12B_it_fp4_mixed.safetensors is a community fp4 quantization of
# google/gemma-3-12b-it. Check the LTX-Video HF page for the current recommended source.
Download-HF `
    -Repo  "Lightricks/LTX-Video-0.9.7-distilled" `
    -File  "gemma_3_12B_it_fp4_mixed.safetensors" `
    -Output "$ModelBase\text_encoders\gemma_3_12B_it_fp4_mixed.safetensors" `
    -Label "Gemma 3 12B fp4 (text encoder)"

Write-Host ""
Write-Host "[4/4] VAE — LTX-2.3 full video VAE bf16 (~1.4 GB)" -ForegroundColor White
Download-HF `
    -Repo  "Lightricks/LTX-Video-0.9.7-distilled" `
    -File  "LTX23_video_vae_bf16.safetensors" `
    -Output "$ModelBase\vae\LTX23_video_vae_bf16.safetensors" `
    -Label "LTX-2.3 video VAE (bf16, 32x spatial)"

# ── Summary ───────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "   Installation complete" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Model locations:" -ForegroundColor White
Write-Host "  $ModelBase\diffusion_models\ltx-2.3-22b-distilled_transformer_only_fp8_input_scaled_v3.safetensors" -ForegroundColor Gray
Write-Host "  $ModelBase\checkpoints\ltx-2.3-22b-distilled-fp8.safetensors" -ForegroundColor Gray
Write-Host "  $ModelBase\text_encoders\gemma_3_12B_it_fp4_mixed.safetensors" -ForegroundColor Gray
Write-Host "  $ModelBase\vae\LTX23_video_vae_bf16.safetensors" -ForegroundColor Gray
Write-Host ""
Write-Host "Ensure ComfyUI's extra_model_paths.yaml has:" -ForegroundColor Yellow
Write-Host "  ltx_video:" -ForegroundColor White
Write-Host "    base_path: $ModelBase\" -ForegroundColor White
Write-Host "    diffusion_models: diffusion_models/" -ForegroundColor White
Write-Host "    checkpoints: checkpoints/" -ForegroundColor White
Write-Host "    text_encoders: text_encoders/" -ForegroundColor White
Write-Host "    vae: vae/" -ForegroundColor White
Write-Host ""
Write-Host "Then start ComfyUI:" -ForegroundColor Yellow
Write-Host "  cd $ComfyDir" -ForegroundColor White
Write-Host "  python main.py --listen" -ForegroundColor White
Write-Host ""

Read-Host "Press Enter to exit"
