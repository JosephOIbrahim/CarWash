# CarWash

**AI-Powered Video Generation for Houdini via USD Hydra**

CarWash is a Houdini Hydra render delegate that bridges SideFX Houdini with cutting-edge AI video generation models. Render your 3D scenes directly to AI-generated video using LTX-2, AnimateDiff, and other diffusion models through ComfyUI.

---

## Features

- **Native Hydra Integration** — Appears as a standard renderer in Houdini's viewport and render settings
- **LTX-2 19B Support** — State-of-the-art video generation with Gemma 3 12B text encoding
- **Depth-Conditioned Generation** — Uses Houdini scene depth as control signal for consistent output
- **Real-time Preview** — CPU rasterizer provides instant depth/normal previews
- **Deterministic Pipeline** — Reproducible results with fixed seeds and batch-invariant processing
- **WebSocket Architecture** — Asynchronous communication with ComfyUI for non-blocking renders

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

```bash
cd hdCarWash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Program Files/Side Effects Software/Houdini 21.0.607"
cmake --build . --config Release
```

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

Copy `build/plugin/hdCarWash/Release/hdCarWash.dll` to `plugin/lib/hdCarWash.dll`

### 5. Download Models

Manually place the LTX-2 and text-encoder models in ComfyUI's `models/` directories
(`models/checkpoints`, `models/text_encoders`, `models/vae`). See the ComfyUI-LTXVideo
documentation for the current canonical filenames.

---

## Usage

### Basic Workflow

1. **Start ComfyUI** on port 8188 (default)

2. **Launch Houdini** and create a LOP network

3. **Build your scene** with geometry, cameras, and lights

4. **Add Render Settings** node and select **"CarWash"** as the renderer

5. **Configure prompts** in the render settings:
   - Positive: `"photorealistic 3D render, cinematic lighting, sharp details"`
   - Negative: `"blurry, low quality, distorted"`

6. **Render** — Output appears in ComfyUI's output folder

### Render Settings

| Parameter | Default | Description |
|-----------|---------|-------------|
| `positive_prompt` | (scene description) | What to generate |
| `negative_prompt` | `"blurry, low quality"` | What to avoid |
| `inference_steps` | 20 | Denoising steps |
| `guidance_scale` | 7.5 | Prompt adherence |
| `seed` | random | For reproducibility |
| `video_length` | 25 | Frames to generate |
| `frame_rate` | 25.0 | Output FPS |

---

## Architecture

```
plugin/
├── hdCarWash/
│   ├── comfyClient.cpp      # WebSocket client & workflow builder
│   ├── renderDelegate.cpp   # Hydra delegate implementation
│   ├── renderPass.cpp       # Render execution logic
│   ├── rasterizer.cpp       # CPU depth/normal rasterizer
│   ├── camera.cpp           # USD camera handling
│   ├── mesh.cpp             # USD mesh processing
│   └── ...
├── lib/
│   └── hdCarWash.dll        # Compiled plugin
└── plugInfo.json            # Hydra registration
```

### Key Components

| Component | Purpose |
|-----------|---------|
| **ComfyClient** | Manages WebSocket connection to ComfyUI, builds LTX-2 workflows |
| **RenderDelegate** | Implements HdRenderDelegate interface for Hydra |
| **RenderPass** | Executes render, coordinates rasterizer and ComfyUI submission |
| **Rasterizer** | CPU-based depth and normal map generation |

---

## Model Configuration

hdCarWash uses **LTX-2 19B Distilled** with **Gemma 3 12B** for text encoding:

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

> **Note:** LTX-2 uses Gemma 3 12B for text encoding (not T5/T5-XXL).
>
> **Accuracy note (under revision):** the node names above (`UNETLoader` +
> `transformer_only`, `LTXAVTextEncoderLoader`, `taeltx_2` as the final VAE) do **not** match
> the current canonical ComfyUI-LTXVideo graph, which uses `CheckpointLoaderSimple`,
> `LTXVGemmaCLIPModelLoader`, `LTXVImgToVideoInplace`/`ConditionOnly`, and tiled VAE decode
> with `CreateVideo`/`SaveVideo`. The text-encoder *family* (Gemma 3 12B) is correct; the
> loader node names and the exact embedding dimension are being corrected as the workflow is
> modernized (see `state/tasks/t011/findings.md` and `state/plan.md`, task t022).

---

## Workflows

> **Status:** The ComfyUI workflow graph is currently constructed in C++
> (`comfyClient.cpp`). Migrating it to versioned, on-disk template `.json` files under a
> `workflows/` directory is planned (see `state/plan.md`, task t022). No `workflows/`
> directory ships yet.

---

## Troubleshooting

### "Shape mismatch: 128x2048 vs 3840x4096"

You're using T5 XXL instead of Gemma 3 12B. Ensure `LTXAVTextEncoderLoader` is used with `gemma_3_12B_it_fp4_mixed.safetensors`.

### Plugin not appearing in Houdini

1. Verify `hdCarWash.json` package file exists in `Documents/houdini21.0/packages/`
2. Check `PXR_PLUGINPATH_NAME` points to folder containing `plugInfo.json`
3. Ensure `plugin/lib/hdCarWash.dll` exists

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

A Houdini-side render smoke test lives at `test/test_carwash_render.py` (run from a Houdini
21+ Python shell). A C++ unit-test target is scaffolded under `tests/cpp/` (CTest) and is
being populated (see `state/plan.md`, tasks t040/t041). The previously documented
`pytest` suites did not exist and have been removed from this section.

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
