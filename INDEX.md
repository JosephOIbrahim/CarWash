# hdCarWash - Project Index

Houdini Hydra render delegate for AI-powered video generation via ComfyUI/LTX2.

**Repository:** https://github.com/JosephOIbrahim/hdCarWash (private)

---

## Architecture Overview

```
Houdini (USD/Hydra) → hdCarWash Delegate → ComfyUI (WebSocket) → LTX2/Gemma → Video Output
```

---

## Directory Structure

```
HdCarWash/
├── plugin/                    # Hydra render delegate (C++)
│   ├── hdCarWash/            # Main plugin source
│   │   ├── comfyClient.*     # ComfyUI WebSocket client
│   │   ├── renderDelegate.*  # Hydra delegate implementation
│   │   ├── renderPass.*      # Render pass logic
│   │   ├── rasterizer.*      # CPU rasterizer for depth/control images
│   │   ├── camera.*          # Camera handling
│   │   ├── mesh.*            # Mesh processing
│   │   ├── light.*           # Light handling
│   │   ├── renderBuffer.*    # AOV buffer management
│   │   └── tokens.*          # USD tokens
│   ├── lib/                  # Built DLL output
│   └── plugInfo.json         # Hydra plugin registration
├── workflows/                 # ComfyUI workflow definitions
├── automation/               # Build & test automation
├── python/                   # Python utilities
├── schema/                   # USD schema definitions
├── comfyui/                  # ComfyUI custom nodes
├── houdini/                  # Houdini-side Python scripts
├── tests/                    # Test suite
└── scripts/                  # Utility scripts
```

---

## Core Components

### Plugin (C++ Hydra Delegate)

| File | Purpose |
|------|---------|
| `plugin/hdCarWash/comfyClient.cpp` | ComfyUI WebSocket client, LTX2 workflow builder |
| `plugin/hdCarWash/comfyClient.h` | ComfyUI client header |
| `plugin/hdCarWash/renderDelegate.cpp` | Main Hydra delegate implementation |
| `plugin/hdCarWash/renderDelegate.h` | Delegate header with render settings |
| `plugin/hdCarWash/renderPass.cpp` | Render pass execution logic |
| `plugin/hdCarWash/renderPass.h` | Render pass header |
| `plugin/hdCarWash/rasterizer.cpp` | CPU depth/normal rasterizer |
| `plugin/hdCarWash/rasterizer.h` | Rasterizer header |
| `plugin/hdCarWash/camera.cpp` | USD camera → matrices |
| `plugin/hdCarWash/mesh.cpp` | USD mesh → triangles |
| `plugin/hdCarWash/light.cpp` | USD light handling |
| `plugin/hdCarWash/renderBuffer.cpp` | AOV buffer management |
| `plugin/hdCarWash/tokens.cpp` | USD token definitions |
| `plugin/hdCarWash/debugCodes.cpp` | TF_DEBUG codes |
| `plugin/hdCarWash/rendererPlugin.cpp` | Plugin registration |
| `plugin/hdCarWash/api.h` | Export macros |
| `plugin/hdCarWash/third_party/stb_image.h` | Image I/O |
| `plugin/plugInfo.json` | Hydra plugin manifest |
| `plugin/hdCarWash/CMakeLists.txt` | Build configuration |

### ComfyUI Workflows

| File | Purpose |
|------|---------|
| `workflows/carwash_ltx2_img2vid.json` | LTX2 image-to-video workflow |
| `workflows/hdcarwash_sdxl_depth.json` | SDXL depth-conditioned workflow |
| `workflows/carwash_animatediff_workflow.json` | AnimateDiff workflow |
| `workflows/carwash_video_workflow.json` | General video workflow |
| `workflows/test_workflow_api.json` | API format test workflow |

### USD Schemas

| File | Purpose |
|------|---------|
| `schema/carWashSchema.usda` | CarWash render settings schema |
| `schema/carWashRenderSettingsAPI.usda` | Render settings API schema |
| `schema/cognitiveSubstrate.usda` | Cognitive substrate integration |
| `schema/generatedSchema.usda` | Generated schema output |

### Python Utilities

| File | Purpose |
|------|---------|
| `python/carwash_video_bridge.py` | Video bridge utilities |
| `python/carwash_color.py` | Color processing |
| `python/carwash_production.py` | Production helpers |
| `python/carwash_test_suite.py` | Test suite runner |
| `houdini/synapse_server.py` | Synapse WebSocket server |

### Automation Scripts

| File | Purpose |
|------|---------|
| `automation/deploy_and_test.py` | Build, deploy, and test |
| `automation/diagnose_carwash.py` | Diagnostic utilities |
| `automation/check_comfyui_nodes.py` | Verify ComfyUI nodes |
| `automation/quick_test.py` | Quick integration test |
| `automation/quick_diagnostic.py` | Quick diagnostic |
| `automation/synapse_*.py` | Synapse automation scripts |
| `deploy_hdcarwash.py` | DLL deployment script |
| `deploy.bat` | Windows deploy batch |
| `setup_comfyui.bat` | ComfyUI setup |
| `download_models.ps1` | Model download script |
| `build_all_versions.py` | Multi-version build |

### Tests

| File | Purpose |
|------|---------|
| `tests/test_determinism.py` | Determinism verification |
| `tests/test_comfyui_nodes.py` | ComfyUI node tests |
| `tests/test_schema.py` | USD schema tests |
| `test/test_carwash_render.py` | Render integration test |

---

## Model Configuration (LTX2)

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

The DLL may need to be deployed to multiple locations:
- `plugin/lib/hdCarWash.dll` (package path)
- `Documents/houdini21.0/dso/hdCarWash.dll` (user DSO)
- `houdini21.0/dso/usd/hdCarWash/lib/hdCarWash.dll` (USD plugin path)

---

## Build

```bash
cd HdCarWash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

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

## Related Files (External)

| Path | Purpose |
|------|---------|
| `C:\ComfyUI\user\default\workflows\carwash_ltx2_gemma.json` | Bookmarked working workflow |
| `C:\Temp\ltx2_av_loader.json` | Test workflow (API format) |

---

*Generated: 2026-02-02*
