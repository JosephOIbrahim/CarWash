# HdCarWash — Claude Desktop Index

**Project**: Cognitively-Aware AI Render Delegate for Houdini 21 (Solaris/LOPs)
**Version**: 1.0.0 (Development)
**Author**: Joseph O. Ibrahim
**License**: Proprietary
**Last Updated**: 2026-02-02

---

## Quick Reference

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ARCHITECTURE                                                                 │
│   USD Hydra Render Delegate → CPU Rasterizer → ComfyUI AI Backend           │
│   AOVs: color, depth, normal, objectId, primId, motionVector                │
├─────────────────────────────────────────────────────────────────────────────┤
│ DETERMINISM ([He2025] Compliant)                                            │
│   Layer 1: Content Identity    - HdCarWashFrameHash (FNV-1a)                │
│   Layer 2: Canonical Ordering  - std::sort(rprimPaths) by SdfPath           │
│   Layer 3: Fixed Boundaries    - Sequential face iteration (no parallel)    │
│   Layer 4: Output Verification - ComputeHash() with strategic sampling      │
├─────────────────────────────────────────────────────────────────────────────┤
│ KEY FILES                                                                    │
│   renderPass.cpp   - Frame orchestration, AI integration                    │
│   rasterizer.cpp   - CPU software rasterizer, determinism hash              │
│   comfyClient.cpp  - ComfyUI HTTP/WS client, ControlNet support             │
│   renderDelegate.cpp - Hydra plugin registration                            │
├─────────────────────────────────────────────────────────────────────────────┤
│ COMFYUI INTEGRATION                                                          │
│   Server: http://localhost:8188                                             │
│   Workflow: txt2img + ControlNet (depth/normal conditioning)                │
│   Models: SD 1.5 + control_v11f1p_sd15_depth + control_v11p_sd15_normalbae  │
│   Mode: Async (progressive) or Sync (blocking)                              │
├─────────────────────────────────────────────────────────────────────────────┤
│ BUILD & DEPLOY                                                               │
│   Build: cmake --build build --config Release                               │
│   Deploy: deploy.bat (one-click) or deploy_hdcarwash.py                     │
│   Target: C:\Users\User\houdini21.0\dso\usd\hdCarWash\                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Project Structure

```
HdCarWash/
├── plugin/hdCarWash/           # Main plugin source
│   ├── api.h                   # DLL export macros
│   ├── rendererPlugin.cpp/h    # HdRendererPlugin registration
│   ├── renderDelegate.cpp/h    # HdRenderDelegate implementation
│   ├── renderPass.cpp/h        # HdRenderPass (frame orchestration)
│   ├── renderBuffer.cpp/h      # HdRenderBuffer (AOV storage)
│   ├── rasterizer.cpp/h        # CPU software rasterizer
│   ├── comfyClient.cpp/h       # ComfyUI HTTP/WS client
│   ├── mesh.cpp/h              # HdMesh handling
│   ├── camera.cpp/h            # HdCamera handling
│   ├── light.cpp/h             # HdSprim light handling
│   ├── tokens.cpp/h            # TfToken definitions
│   ├── debugCodes.cpp/h        # TF_DEBUG codes
│   └── third_party/
│       └── stb_image.h         # PNG encoding/decoding
├── schema/                     # USD schema definitions
│   ├── carWashSchema.usda
│   ├── cognitiveSubstrate.usda
│   ├── carWashRenderSettingsAPI.usda
│   └── generatedSchema.usda
├── workflows/                  # ComfyUI workflow templates
│   └── *.json
├── build/                      # CMake build output
├── deploy_hdcarwash.py         # Automated deployment script
├── deploy.bat                  # One-click deploy wrapper
├── rebuild_and_install.bat     # Legacy manual deploy
├── CMakeLists.txt              # Build configuration
├── DETERMINISM_CROSS_REFERENCE.md  # Determinism documentation
└── CLAUDE_DESKTOP_INDEX.md     # This file
```

---

## Core Components

### 1. HdCarWashRenderPass (renderPass.cpp)

**Purpose**: Frame rendering orchestration, AI integration pipeline

**Key Methods**:
- `_Execute()` - Main render entry point (calls `_ExecutePhase1`)
- `_ExecutePhase1()` - CPU rasterization + async AI stylization
- `_CopyFramebufferToAOVs()` - Copy internal buffers to Hydra AOVs
- `_GetCamera()` - Extract camera from render pass state
- `IsConverged()` - Progressive refinement control

**Render Modes**:
| Mode | `_syncRenderMode` | `_progressiveRefine` | Behavior |
|------|-------------------|---------------------|----------|
| Async Progressive | false | true | CPU render shown, AI replaces when ready |
| Async Immediate | false | false | CPU render, AI ignored after first frame |
| Sync Blocking | true | N/A | Wait for AI before returning |

**Style Parameters** (HdCarWashStyleParams):
- `prompt` - AI generation prompt
- `negativePrompt` - Things to avoid
- `inferenceSteps` - Diffusion steps (default: 20)
- `guidanceScale` - CFG scale (default: 7.5)
- `seed` - Fixed seed for determinism (default: 42)
- `controlNetStrength` - Depth ControlNet strength (default: 0.8)
- `normalControlNetStrength` - Normal ControlNet strength (default: 0.6)
- `useDepthControl` - Enable depth conditioning (default: true)
- `useNormalControl` - Enable normal conditioning (default: false)

**Determinism Implementation**:
```cpp
// Line 244: Canonical mesh ordering
std::sort(rprimPaths.begin(), rprimPaths.end());

// Line 274: Frame hash for verification
HdCarWashFrameHash frameHash = _framebuffer.ComputeHash(16);
```

---

### 2. HdCarWashRasterizer (rasterizer.cpp)

**Purpose**: CPU software rasterizer for AOV generation

**Key Methods**:
- `RasterizeMesh()` - Transform and rasterize a mesh
- `RasterizeTriangle()` - Per-pixel rasterization with z-buffer
- `ComputeShading()` - Blinn-Phong lighting model
- `TransformTriangle()` - MVP transform + clip space conversion
- `Clear()` / `ClearDepthOnly()` - Buffer clearing

**AOV Buffers** (HdCarWashFramebuffer):
| Buffer | Type | Description |
|--------|------|-------------|
| `color` | `GfVec4f` | RGBA color output |
| `depth` | `float` | Linear depth (camera Z) |
| `normal` | `GfVec3f` | World-space normals |
| `objectId` | `int32_t` | Per-mesh unique ID |
| `primId` | `int32_t` | Per-face ID |
| `motionVector` | `GfVec2f` | Screen-space motion (future) |

**Determinism Implementation**:
```cpp
// Frame hash (lines 92-166): FNV-1a with strategic sampling
HdCarWashFrameHash ComputeHash(unsigned int sampleRate = 16) const {
    // Samples corners + center + grid pattern
    // Quantizes floats to int32 (×10000) to avoid precision issues
    // Returns combined XOR hash of all AOVs
}

// Triangle iteration (lines 366-409): Fixed face index order
for (size_t faceIdx = 0; faceIdx < faceVertexCounts.size(); ++faceIdx) {
    // DETERMINISTIC: Sequential iteration, no parallel reordering
}
```

---

### 3. HdCarWashComfyClient (comfyClient.cpp)

**Purpose**: ComfyUI backend communication (HTTP POST + polling)

**Key Methods**:
- `ProcessFrame()` - Synchronous frame processing
- `ProcessFrameAsync()` - Async frame processing (std::future)
- `IsServerAvailable()` - Health check
- `_BuildWorkflow()` - Construct ComfyUI workflow JSON
- `_BuildWorkflowControlNet()` - ControlNet workflow with depth/normal
- `_SaveControlImages()` - Save depth/normal PNGs to ComfyUI input folder
- `_SubmitWorkflow()` - POST workflow to /prompt endpoint
- `_WaitForCompletion()` - Poll /history for completion
- `_DownloadResult()` - GET result image from /view endpoint

**Buffer Encoding**:
- `EncodeDepthBuffer()` - Depth → grayscale PNG (base64)
- `EncodeNormalBuffer()` - Normal → RGB PNG (base64)
- `EncodeColorBuffer()` - Color → RGBA PNG (base64)
- `DecodeColorImage()` - PNG (base64) → color buffer

**ControlNet Integration** (NEW):
```cpp
// _SaveControlImages(): Saves depth.png and normal.png to ComfyUI input folder
// _BuildWorkflowControlNet(): Creates workflow with:
//   - LoadImage nodes for depth/normal
//   - ControlNetLoader nodes for models
//   - ControlNetApplyAdvanced nodes for conditioning
//   - Standard KSampler → VAEDecode → SaveImage
```

**Models Required**:
- `v1-5-pruned-emaonly.safetensors` (SD 1.5 checkpoint)
- `control_v11f1p_sd15_depth.pth` (Depth ControlNet)
- `control_v11p_sd15_normalbae.pth` (Normal ControlNet)

---

### 4. HdCarWashRenderDelegate (renderDelegate.cpp)

**Purpose**: Hydra plugin registration and prim factory

**Key Methods**:
- `GetSupportedRprimTypeIds()` - Returns `{HdPrimTypeTokens->mesh}`
- `CreateRprim()` - Factory for HdCarWashMesh
- `GetRenderSettingsDescriptors()` - Expose render settings to UI
- `GetAovDescriptors()` - Declare supported AOVs
- `CommitResources()` - Finalize resource updates

**Render Settings**:
| Token | Type | Default | Description |
|-------|------|---------|-------------|
| `enableAI` | bool | true | Enable AI stylization |
| `prompt` | string | "photorealistic..." | AI prompt |
| `negativePrompt` | string | "blurry..." | Negative prompt |
| `inferenceSteps` | int | 20 | Diffusion steps |
| `guidanceScale` | float | 7.5 | CFG scale |
| `seed` | int | 42 | Random seed |
| `enableDepthControl` | bool | true | Use depth ControlNet |
| `enableNormalControl` | bool | false | Use normal ControlNet |
| `depthControlNetStrength` | float | 0.8 | Depth strength |
| `normalControlNetStrength` | float | 0.6 | Normal strength |
| `syncRenderMode` | bool | false | Wait for AI |
| `progressiveRefine` | bool | true | Keep refining |

---

## Determinism Framework

### [He2025] Compliance Matrix

| [He2025] Principle | HdCarWash Implementation | Status |
|-------------------|-------------------------|--------|
| **Batch Invariance** | Single-threaded CPU rasterizer | ✅ Implicit |
| **Fixed Reduction Order** | Sequential face iteration | ✅ Implemented |
| **Fixed Split Size** | No parallel batching | ✅ By design |
| **Same Input → Same Output** | Canonical mesh ordering + fixed seed | ✅ Verified |

### Four-Layer Determinism Stack

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ LAYER 4: OUTPUT VERIFICATION                                                │
│   HdCarWashFrameHash::ComputeHash() → 64-bit FNV-1a combined hash          │
│   Strategic sampling: corners + center + every 16th pixel                   │
├─────────────────────────────────────────────────────────────────────────────┤
│ LAYER 3: FIXED BOUNDARIES                                                   │
│   Triangle iteration: face index order (0, 1, 2, ...)                       │
│   No parallel reordering in rasterization loop                              │
├─────────────────────────────────────────────────────────────────────────────┤
│ LAYER 2: CANONICAL ORDERING                                                 │
│   std::sort(rprimPaths.begin(), rprimPaths.end())                          │
│   Lexicographic SdfPath ordering                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│ LAYER 1: CONTENT IDENTITY                                                   │
│   Scene state: geometry + transforms + camera + lights                      │
│   Frame hash: color + depth + normal + objectId + primId                    │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Hash Algorithm (FNV-1a)

```cpp
constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME = 1099511628211ULL;

// Float quantization to avoid precision issues
int32_t quantized = static_cast<int32_t>(value * 10000.0f);
hash ^= byte;
hash *= FNV_PRIME;
```

---

## Build & Deployment

### Prerequisites

- Houdini 21.0 (with USD/Hydra)
- CMake 3.20+
- Visual Studio 2022 (MSVC)
- ComfyUI (for AI backend)

### Build Commands

```bash
# Configure
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="C:/Program Files/Side Effects Software/Houdini 21.0"

# Build
cmake --build build --config Release

# Deploy (one-click)
deploy.bat
```

### Deployment Structure

```
C:\Users\User\houdini21.0\dso\usd\hdCarWash\
├── lib\
│   └── hdCarWash.dll           # Plugin binary
└── resources\
    ├── plugInfo.json           # USD plugin registry
    └── schema\
        ├── carWashSchema.usda
        └── cognitiveSubstrate.usda
```

### Debug Output

```
C:\Temp\hdcarwash_debug.txt     # Render pass debug log
```

---

## ComfyUI Setup

### Required Models

Place in `C:\ComfyUI\models\`:

```
checkpoints/
└── v1-5-pruned-emaonly.safetensors

controlnet/
├── control_v11f1p_sd15_depth.pth
└── control_v11p_sd15_normalbae.pth

vae/
└── (uses checkpoint VAE)
```

### Control Image Directory

```
C:\ComfyUI\input\hdcarwash_{timestamp}\
├── depth.png    # Grayscale depth map
└── normal.png   # RGB normal map
```

### Workflow Structure

```json
{
  "1": {"class_type": "CheckpointLoaderSimple", ...},
  "2": {"class_type": "CLIPTextEncode", ...},      // Positive prompt
  "3": {"class_type": "CLIPTextEncode", ...},      // Negative prompt
  "4": {"class_type": "EmptyLatentImage", ...},
  "5": {"class_type": "ControlNetLoader", ...},    // Depth model
  "6": {"class_type": "LoadImage", ...},           // depth.png
  "7": {"class_type": "ControlNetApplyAdvanced", ...},
  "8": {"class_type": "KSampler", ...},
  "9": {"class_type": "VAEDecode", ...},
  "10": {"class_type": "SaveImage", ...}
}
```

---

## Troubleshooting

### Plugin Not Loading

1. Check `C:\Temp\hdcarwash_debug.txt` for errors
2. Verify `PXR_PLUGINPATH_NAME` includes hdCarWash resources
3. Ensure DLL and plugInfo.json are in correct locations
4. Restart Houdini after deployment

### ComfyUI Connection Failed

1. Verify ComfyUI running on `http://localhost:8188`
2. Check Windows Firewall settings
3. Fallback: Plugin works without ComfyUI (CPU render only)

### AI Result Size Mismatch

SD 1.5 rounds dimensions to multiples of 8. Plugin handles up to 8-pixel difference:
```cpp
bool widthOK = (widthDiff >= 0 && widthDiff <= 8);
bool heightOK = (heightDiff >= 0 && heightDiff <= 8);
```

### Flickering Viewport

Enable progressive refinement and check `ClearDepthOnly()` preserves AI result:
```cpp
if (preserveColorBuffer) {
    _rasterizer->ClearDepthOnly(1.0f);  // Preserve AI color
}
```

---

## API Reference

### Tokens (tokens.h)

```cpp
// AOV tokens
TF_DEFINE_PUBLIC_TOKENS(HdCarWashAovTokens,
    (carwashColor)
    (carwashDepth)
    (carwashNormal)
    (carwashObjectId)
    (carwashSemanticId)
);

// Settings tokens
TF_DEFINE_PUBLIC_TOKENS(HdCarWashSettingsTokens,
    (enableAI)
    (prompt)
    (negativePrompt)
    (inferenceSteps)
    (guidanceScale)
    (seed)
    (enableDepthControl)
    (enableNormalControl)
    (depthControlNetStrength)
    (normalControlNetStrength)
    (syncRenderMode)
    (progressiveRefine)
);
```

### Debug Codes (debugCodes.h)

```cpp
TF_DEBUG_CODES(
    HD_CARWASH       // General debug messages
);

// Enable with: TF_DEBUG=HD_CARWASH houdini
```

---

## References

- **[He2025]**: He, Horace and Thinking Machines Lab, "Defeating Nondeterminism in LLM Inference", Sep 2025.
- **USD Hydra**: https://graphics.pixar.com/usd/docs/api/hd_page_front.html
- **ComfyUI API**: https://github.com/comfyanonymous/ComfyUI
- **ControlNet**: https://github.com/lllyasviel/ControlNet

---

*This index is designed for Claude Desktop to quickly understand the HdCarWash codebase.*
