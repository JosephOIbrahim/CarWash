# HdCarWash - Project Index

Houdini Hydra render delegate for AI-powered video generation via ComfyUI/LTX-2.

**Repository:** https://github.com/JosephOIbrahim/hdCarWash (private)

---

## Architecture Overview

```
Houdini (USD/Hydra) → hdCarWash Delegate → ComfyUI (WebSocket) → LTX2/Gemma → Video Output
```

---

## Naming Convention

This repo uses three related names consistently:

- **`HdCarWash`** — the project / repository / class-name prefix (C++ classes are `HdCarWash*`).
- **`hdCarWash`** — the plugin library / source directory (`plugin/hdCarWash/`, built `hdCarWash.dll`).
- **`CarWash`** — the user-facing renderer label that appears in Houdini's render-settings UI.

## Directory Structure

Actual top-level layout of the repository as it exists today:

```
CARWASH/                       # Repository root
├── plugin/                    # Hydra render delegate (C++)
│   ├── hdCarWash/             # Plugin source
│   │   ├── api.h              # Export macros
│   │   ├── camera.*           # USD camera → matrices
│   │   ├── comfyClient.*      # ComfyUI WebSocket client / workflow builder
│   │   ├── debugCodes.*       # TF_DEBUG codes
│   │   ├── mesh.*             # USD mesh → triangles
│   │   ├── rasterizer.*       # CPU depth/normal rasterizer + frame hashing
│   │   ├── renderBuffer.*     # AOV buffer management
│   │   ├── renderDelegate.*   # Hydra delegate implementation
│   │   ├── renderPass.*       # Render pass logic
│   │   ├── rendererPlugin.*   # Plugin registration
│   │   ├── tokens.*           # USD/TfToken definitions
│   │   └── CMakeLists.txt     # Plugin build configuration
│   └── plugInfo.json          # Hydra plugin manifest
├── automation/                # Python automation scripts (synapse_*, carwash_automation)
├── branding/                  # Visual identity assets
├── docs/                      # Project documentation
├── test/                      # Manual render test (batch launcher + script)
├── tests/                     # C++ test scaffold (tests/cpp/CMakeLists.txt)
├── CMakeLists.txt             # Top-level build configuration
├── rebuild_and_install.bat    # Windows rebuild + install helper
├── README.md
├── INDEX.md
└── DETERMINISM_CROSS_REFERENCE.md
```

> There is currently **no** `workflows/`, `python/`, `schema/`, `comfyui/`, `houdini/`, `scripts/`, or `plugin/lib/` directory, and no `download_models.ps1`. Items below marked **(planned — not yet in repo)** are design intent only.

---

## Core Components

### Plugin (C++ Hydra Delegate)

All files below exist in the repository.

| File | Purpose |
|------|---------|
| `plugin/hdCarWash/api.h` | Export macros |
| `plugin/hdCarWash/camera.cpp` / `.h` | USD camera → matrices |
| `plugin/hdCarWash/comfyClient.cpp` / `.h` | ComfyUI WebSocket client, workflow builder |
| `plugin/hdCarWash/debugCodes.cpp` / `.h` | TF_DEBUG codes |
| `plugin/hdCarWash/mesh.cpp` / `.h` | USD mesh → triangles |
| `plugin/hdCarWash/rasterizer.cpp` / `.h` | CPU depth/normal rasterizer + frame hashing |
| `plugin/hdCarWash/renderBuffer.cpp` / `.h` | AOV buffer management |
| `plugin/hdCarWash/renderDelegate.cpp` / `.h` | Main Hydra delegate implementation |
| `plugin/hdCarWash/renderPass.cpp` / `.h` | Render pass execution logic |
| `plugin/hdCarWash/rendererPlugin.cpp` / `.h` | Plugin registration |
| `plugin/hdCarWash/tokens.cpp` / `.h` | USD/TfToken definitions |
| `plugin/hdCarWash/CMakeLists.txt` | Plugin build configuration |
| `plugin/plugInfo.json` | Hydra plugin manifest |

### Automation Scripts

All files below exist in `automation/`.

| File | Purpose |
|------|---------|
| `automation/carwash_automation.py` | CarWash automation entry point |
| `automation/synapse_discover.py` | Synapse discovery script |
| `automation/synapse_execute.py` | Synapse execution script |
| `automation/synapse_reload.py` | Synapse reload script |
| `automation/synapse_run.py` | Synapse run script |
| `automation/synapse_test.py` | Synapse test script |

### Tests

| File | Purpose |
|------|---------|
| `test/launch_carwash_test.bat` | Windows launcher for the manual render test |
| `test/test_carwash_render.py` | Render test script |
| `tests/cpp/CMakeLists.txt` | C++ test build scaffold |

### Documentation

| File | Purpose |
|------|---------|
| `docs/AGENT_OFFLOAD_PRD_TEMPLATE.md` | Agent-offload PRD template |
| `branding/CARWASH_IDENTITY.md` | Visual identity / color palette |
| `DETERMINISM_CROSS_REFERENCE.md` | Determinism design intent (forward-looking) |

### Planned Artifacts (not yet in repo)

The following were referenced by earlier drafts of this index but **do not currently exist**. They are listed here only as design intent.

| Planned artifact | Intended purpose |
|------------------|------------------|
| `workflows/*.json` | ComfyUI workflow definitions (LTX-2, SDXL depth, AnimateDiff) |
| `schema/*.usda` | USD render-settings / cognitive-substrate schemas |
| `python/`, `houdini/` utilities | Video bridge, color, production, Synapse server helpers |
| `comfyui/` custom nodes | ComfyUI-side integration nodes |
| `download_models.ps1` | Model download helper |
| `plugin/lib/hdCarWash.dll` | Built plugin output (produced by the build, not committed) |
| pytest suites (`test_determinism.py`, `test_comfyui_nodes.py`, `test_schema.py`) | Automated test coverage |

---

## Model Configuration (LTX-2) — planned backend target

> The LTX-2 / ComfyUI integration is **planned** (Phase 2). The delegate currently
> registers backend settings (default `ltx2`) but does not yet submit workflows.

```
UNET:         LTX2/ltx-2-19b-distilled-fp8_transformer_only.safetensors
Text Encoder: LTXAVTextEncoderLoader + gemma_3_12B_it_fp4_mixed.safetensors
VAE:          LTX2/taeltx_2.safetensors
```

**Key insight:** LTX-2 19B requires 3840-dim embeddings. T5 XXL outputs 2048-dim (incompatible).
Gemma 3 12B via `LTXAVTextEncoderLoader` provides correct 3840-dim embeddings.

---

## Installation

### Houdini Package

Create `Documents/houdini21.0/packages/hdCarWash.json`:

```json
{
    "env": [
        {
            "HOUDINI_PATH": {
                "value": "C:/path/to/HdCarWash/plugin",
                "method": "prepend"
            }
        },
        {
            "PXR_PLUGINPATH_NAME": {
                "value": "C:/path/to/HdCarWash/plugin",
                "method": "prepend"
            }
        }
    ],
    "path": "C:/path/to/HdCarWash/plugin"
}
```

### DLL Locations

After building, the resulting `hdCarWash.dll` may need to be deployed to one or more
of the following (none of these paths are committed to the repo):
- a `lib/` folder next to `plugInfo.json` on `PXR_PLUGINPATH_NAME`
- `Documents/houdini21.0/dso/hdCarWash.dll` (user DSO)
- the USD plugin path expected by your Houdini install

`rebuild_and_install.bat` at the repo root automates the rebuild + install step.

---

## Build

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Run these from the repository root (the directory containing the top-level
`CMakeLists.txt`). See `README.md` for the build invocation that pins
`CMAKE_PREFIX_PATH` to your Houdini install.

---

## Usage

1. Start ComfyUI (port 8188)
2. Launch Houdini 21.0
3. Create LOP network with geometry
4. Add Render Settings node, select "CarWash" renderer
5. Render - output appears in ComfyUI output folder

---

## Commit History

| Hash | Description |
|------|-------------|
| `f2f8c1f` | Fix LTX2 text encoder: use LTXAVTextEncoderLoader with Gemma 3 12B |
| `b03021f` | Phase 1 complete: CPU rasterizer with determinism verification |

---

## Related Files (External, machine-specific)

These live outside the repository on the development workstation and are **not** part of the repo:

| Path | Purpose |
|------|---------|
| `C:\ComfyUI\user\default\workflows\carwash_ltx2_gemma.json` | Bookmarked working workflow |
| `C:\Temp\ltx2_av_loader.json` | Test workflow (API format) |

---

*Generated: 2026-02-02*
