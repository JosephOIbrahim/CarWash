# CarWash

**AI-Powered Video Generation for Houdini via USD Hydra**

CarWash is a Houdini Hydra render delegate that bridges SideFX Houdini with cutting-edge AI video generation models. The goal is to render 3D scenes directly to AI-generated video using LTX-2 (and, in future, other diffusion backends) through ComfyUI.

> **Project status:** the C++ Hydra delegate (scene traversal, CPU rasterizer producing depth/normal/ID AOVs, render-settings registration, and frame hashing) is implemented. The ComfyUI / LTX-2 submission path is **planned** (Phase 2) and not yet wired up — features and instructions tied to it are marked **(planned)** below.

> **Naming convention used in this repo:**
> - **`HdCarWash`** — project / repository / C++ class prefix
> - **`hdCarWash`** — plugin source directory (`plugin/hdCarWash/`) and built `hdCarWash.dll`
> - **`CarWash`** — the user-facing renderer label in Houdini's render-settings UI

---

## Features

- **Native Hydra Integration** — Appears as a standard renderer (`CarWash`) in Houdini's render settings
- **CPU Rasterizer** — Generates depth, normal, and object/prim-ID AOVs from scene geometry
- **Frame Hashing** — FNV-1a frame hashing of AOV buffers for determinism checks (see `rasterizer.cpp`)
- **LTX-2 19B Support** *(planned)* — Video generation with Gemma 3 12B text encoding
- **Depth-Conditioned Generation** *(planned)* — Use scene depth as a control signal
- **WebSocket Architecture** *(planned)* — Asynchronous communication with ComfyUI

---

## How It Works

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Houdini   │────▶│  hdCarWash  │────▶│   ComfyUI   │────▶│   Output    │
│  USD Scene  │     │   Delegate  │     │    LTX-2    │     │   Video     │
└─────────────┘     └─────────────┘     └─────────────┘     └─────────────┘
       │                   │                   │
       │              WebSocket           AI Inference
       │                   │                   │
       ▼                   ▼                   ▼
   Geometry          Depth Maps         25 Frame Video
   Cameras           Control Images     1312×992 px
   Lights            Prompts            25 FPS
```

1. **Scene Export** — hdCarWash receives USD prims from Houdini's Hydra viewport
2. **Rasterization** — CPU rasterizer generates depth and normal maps from scene geometry
3. **Workflow Build** — Constructs LTX-2 workflow JSON with scene-derived control images
4. **Submission** — Sends workflow to ComfyUI via WebSocket API
5. **Generation** — LTX-2 generates video conditioned on depth maps and text prompts
6. **Return** — Results displayed in Houdini viewport or saved to disk

---

## Requirements

- **Houdini** 20.5+ or 21.0+ (with USD/Hydra support)
- **ComfyUI** with LTX-2 nodes installed
- **Models:**
  - `ltx-2-19b-distilled-fp8_transformer_only.safetensors` (UNET)
  - `gemma_3_12B_it_fp4_mixed.safetensors` (Text Encoder)
  - `taeltx_2.safetensors` (VAE)
- **Windows** 10/11 (64-bit)
- **CUDA** capable GPU (RTX 3090+ recommended for 19B model)

---

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/JosephOIbrahim/hdCarWash.git
```

### 2. Build the Plugin

Run from the repository root (the directory containing the top-level `CMakeLists.txt`).
Point `CMAKE_PREFIX_PATH` at your own Houdini install path:

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Program Files/Side Effects Software/Houdini 21.0.607"
cmake --build . --config Release
```

On Windows you can instead run `rebuild_and_install.bat` from the repo root, which
performs the rebuild and install in one step.

### 3. Install Houdini Package

Create `Documents/houdini21.0/packages/hdCarWash.json`:

```json
{
    "env": [
        {
            "HOUDINI_PATH": {
                "value": "C:/path/to/hdCarWash/plugin",
                "method": "prepend"
            }
        },
        {
            "PXR_PLUGINPATH_NAME": {
                "value": "C:/path/to/hdCarWash/plugin",
                "method": "prepend"
            }
        }
    ],
    "path": "C:/path/to/hdCarWash/plugin"
}
```

### 4. Deploy DLL

Copy the built `hdCarWash.dll` (under `build/plugin/hdCarWash/Release/`) next to the
`plugInfo.json` on your `PXR_PLUGINPATH_NAME` (commonly a `lib/` subfolder), or use
`rebuild_and_install.bat`, which copies it to the expected location for you.

### 5. Download Models *(planned — for the future ComfyUI/LTX-2 path)*

There is **no `download_models.ps1` script in this repository yet.** When the ComfyUI
integration lands, place the LTX-2 / Gemma / VAE models listed under **Requirements**
into ComfyUI's `models/` directories manually.

---

## Usage

### Basic Workflow

1. **Start ComfyUI** on port 8188 (default)

2. **Launch Houdini** and create a LOP network

3. **Build your scene** with geometry, cameras, and lights

4. **Add Render Settings** node and select **"CarWash"** as the renderer

5. **Configure render settings** (see the table below for the settings the delegate
   actually registers today)

6. **Render** — the CPU rasterizer produces depth/normal/ID AOVs. *(ComfyUI/LTX-2
   submission and video output are planned — not yet wired up.)*

### Render Settings

These are the render settings the delegate **actually registers** today, as defined in
`renderDelegate.cpp::GetRenderSettingDescriptors()`. Token names come from
`tokens.h` (`HDCARWASH_SETTINGS_TOKENS`).

| Token | UI Label | Default | Description |
|-------|----------|---------|-------------|
| `carwash:backend` | AI Backend | `ltx2` | Generation backend (`ltx2`, `flux`, `cosmos`) — `flux`/`cosmos` are planned |
| `carwash:deterministicMode` | Determinism Mode | `balanced` | `fast` / `balanced` / `maximum` |
| `carwash:comfyui:serverUrl` | ComfyUI Server URL | `http://localhost:8188` | ComfyUI endpoint *(used once Phase 2 lands)* |
| `carwash:inferenceSteps` | Inference Steps | `20` | Denoising steps |
| `carwash:guidanceScale` | Guidance Scale | `7.5` | Guidance / prompt adherence |
| `carwash:substrate:enabled` | Enable Substrate | `true` | Enable cognitive substrate |
| `carwash:substrate:historyFrames` | History Frames | `10` | Frames of substrate history |
| `carwash:seed` | Random Seed | `42` | Seed for reproducibility |

> Earlier drafts documented `positive_prompt`, `negative_prompt`, `video_length`, and
> `frame_rate`. The delegate does **not** register those tokens. Additional tokens such
> as `carwash:outputDirectory`, `carwash:outputFormat`, `carwash:comfyui:workflowPath`,
> `carwash:comfyui:timeoutSeconds`, and `carwash:substrate:autoAnnotate` are *defined* in
> `tokens.h` but not currently exposed as render-setting descriptors.

---

## Architecture

```
plugin/
├── hdCarWash/
│   ├── comfyClient.cpp      # WebSocket client & workflow builder
│   ├── renderDelegate.cpp   # Hydra delegate implementation
│   ├── renderPass.cpp       # Render execution logic
│   ├── rasterizer.cpp       # CPU depth/normal rasterizer + frame hashing
│   ├── camera.cpp           # USD camera handling
│   ├── mesh.cpp             # USD mesh processing
│   └── ...                  # api.h, debugCodes.*, renderBuffer.*, rendererPlugin.*, tokens.*
└── plugInfo.json            # Hydra registration
```

> The compiled `hdCarWash.dll` is a build artifact and is not committed; there is no
> `plugin/lib/` directory in the repo. See **Installation → Deploy DLL** for placement.

### Key Components

| Component | Purpose |
|-----------|---------|
| **ComfyClient** | Manages WebSocket connection to ComfyUI, builds LTX-2 workflows |
| **RenderDelegate** | Implements HdRenderDelegate interface for Hydra |
| **RenderPass** | Executes render, coordinates rasterizer and ComfyUI submission |
| **Rasterizer** | CPU-based depth and normal map generation |

---

## Model Configuration *(planned — Phase 2 target)*

When the ComfyUI integration lands, HdCarWash will target **LTX-2 19B Distilled** with
**Gemma 3 12B** for text encoding:

```
┌────────────────────────────────────────────────────────────────┐
│ LTX-2 19B Configuration                                        │
├────────────────────────────────────────────────────────────────┤
│ UNET:          ltx-2-19b-distilled-fp8_transformer_only        │
│ Text Encoder:  LTXAVTextEncoderLoader + Gemma 3 12B (3840-dim) │
│ VAE:           taeltx_2                                        │
│ Resolution:    Up to 1312×992                                  │
│ Frame Count:   25 frames                                       │
│ Frame Rate:    25 FPS                                          │
└────────────────────────────────────────────────────────────────┘
```

> **Note:** LTX-2 19B requires 3840-dimensional text embeddings. The older T5 XXL encoder (2048-dim) is incompatible. Gemma 3 12B provides the correct embedding dimensions.

---

## Workflows *(planned — not yet in repo)*

There is **no `workflows/` directory in this repository yet.** The following ComfyUI
workflows are planned for the Phase 2 ComfyUI integration:

| Workflow (planned) | Description |
|--------------------|-------------|
| `carwash_ltx2_img2vid.json` | LTX-2 image-to-video with depth conditioning |
| `hdcarwash_sdxl_depth.json` | SDXL with ControlNet depth |
| `carwash_animatediff_workflow.json` | AnimateDiff video generation |

---

## Troubleshooting

### "Shape mismatch: 128x2048 vs 3840x4096"

You're using T5 XXL instead of Gemma 3 12B. Ensure `LTXAVTextEncoderLoader` is used with `gemma_3_12B_it_fp4_mixed.safetensors`.

### Plugin not appearing in Houdini

1. Verify `hdCarWash.json` package file exists in `Documents/houdini21.0/packages/`
2. Check `PXR_PLUGINPATH_NAME` points to the folder containing `plugInfo.json`
3. Ensure the built `hdCarWash.dll` is deployed next to that `plugInfo.json` (see Installation → Deploy DLL)

### ComfyUI connection failed

1. Verify ComfyUI is running on `localhost:8188`
2. Check firewall settings allow WebSocket connections
3. Look for connection errors in Houdini console

### Out of VRAM

LTX-2 19B requires significant VRAM. Try:
- Reduce resolution (768×512 minimum)
- Use fp8 quantized model
- Close other GPU applications

---

## Development

### Debug Logging

Enable debug output:
```cpp
TF_DEBUG_CODES(
    HD_CARWASH
);
```

Set environment variable:
```bash
set TF_DEBUG=HD_CARWASH
```

### Running Tests

A manual render test lives in `test/`:

```bash
# Windows: launches Houdini with the test scene
test\launch_carwash_test.bat
# or run the render test script directly
python test/test_carwash_render.py
```

> The pytest suites referenced by earlier drafts (`test_determinism.py`,
> `test_comfyui_nodes.py`, `test_schema.py`) are **planned** and not yet in the repo.
> A C++ test scaffold exists at `tests/cpp/CMakeLists.txt`.

---

## License

This project is proprietary software. All rights reserved.

---

## Acknowledgments

- **SideFX** — Houdini and USD/Hydra framework
- **Lightricks** — LTX-2 video generation model
- **Google** — Gemma 3 text encoder
- **ComfyUI** — Node-based diffusion interface

---

## Contact

For questions or support, please open an issue on GitHub.

---

*Built with Houdini 21.0 | ComfyUI | LTX-2 19B | Gemma 3 12B*
