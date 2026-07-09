# Headless render verification (husk)

Verifies that the CarWash Hydra delegate **loads, registers, and runs** in a given
Houdini build without opening the GUI. Build + launch must be the **same** Houdini
version — set `$HFS` to whichever install you built against (21.0.729 today, 22.x
when it ships).

## 1. Confirm the delegate registers

```powershell
# Point HFS at the install you built against:
$env:HFS = "C:\Program Files\Side Effects Software\Houdini 21.0.729"
$env:PXR_PLUGINPATH_NAME = "C:\Users\User\CARWASH\plugin"   # manifest dir (plugInfo.json + lib/hdCarWash.dll)
& "$env:HFS\bin\husk.exe" --list-renderers
```

Expect a line: `HdCarWashRendererPlugin (CarWash)`. If present, the DLL loaded into
that Houdini's USD runtime and the delegate is available (ABI-compatible).

## 2. Render the test stage (Phase 1 — CPU rasterizer)

```powershell
$env:TF_DEBUG = "HD_CARWASH"   # optional: write C:/Temp/hdcarwash_debug.txt
& "$env:HFS\bin\husk.exe" -R CarWash -c /World/Camera -r 256 256 `
    -o build\carwash_render.png -f 1 --make-output-path test\headless_cube.usda
```

> **Note:** `_enableAI` defaults **true**, so `_Execute` will try to submit a workflow
> to ComfyUI (`127.0.0.1:8188`). With no ComfyUI server up, the AI step blocks/falls
> back. To test **only** the Phase-1 CPU rasterizer, render with the `carwash:enableAI`
> render setting set to `false` (or start ComfyUI first for the full path).

## What the fixes guarantee here
- AOV writes are bounded by the buffer's real `HdFormat` (no heap overflow).
- The frame hash is full-buffer, raw-IEEE-754-bits (drift-sensitive); set
  `carwash:deterministicMode` on for reproducible AI seeds.
