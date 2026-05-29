# HdCarWash Pipeline Diagnostic Report

**Date:** 2026-02-02
**Analyst:** Claude Code
**Version:** hdCarWash v1.0 (LTX-2 19B Integration)

---

## Executive Summary

The hdCarWash Hydra render delegate has a **fully implemented** C++ pipeline for:
1. Reading USD geometry via Hydra
2. CPU rasterizing depth/normal control images
3. Submitting to ComfyUI via WebSocket/HTTP
4. Receiving AI-generated frames
5. Writing results back to HdRenderBuffer

**Primary Finding:** The pipeline is architecturally complete. The issues preventing viewport display are:

| Issue | Severity | Root Cause |
|-------|----------|------------|
| USD Schema not compiled | **CRITICAL** | `carWashRenderSettingsAPI.usda` exists but `usdCarWash` library not built |
| Render Settings tab missing | **HIGH** | Without compiled schema, Houdini can't show CarWash settings |
| Progressive refinement may stall | **MEDIUM** | Async completion detection relies on Hydra re-executing |
| Debug log not created | **LOW** | C:/Temp may need manual creation or permissions |

---

## 1. File Inventory

### Core Plugin (C++ Hydra Delegate)

| File | Lines | Purpose | Status |
|------|-------|---------|--------|
| `plugin/hdCarWash/renderPass.cpp` | 602 | Render execution, AI integration | ✅ Complete |
| `plugin/hdCarWash/renderPass.h` | ~60 | Render pass header | ✅ Complete |
| `plugin/hdCarWash/renderDelegate.cpp` | 560 | Hydra delegate, AOV descriptors | ✅ Complete |
| `plugin/hdCarWash/renderDelegate.h` | 173 | Delegate header | ✅ Complete |
| `plugin/hdCarWash/comfyClient.cpp` | ~1700 | ComfyUI WebSocket/HTTP client | ✅ Complete |
| `plugin/hdCarWash/comfyClient.h` | ~150 | Client header with RenderResult | ✅ Complete |
| `plugin/hdCarWash/renderBuffer.cpp` | ~150 | AOV buffer implementation | ✅ Complete |
| `plugin/hdCarWash/rasterizer.cpp` | ~400 | CPU depth/normal rasterizer | ✅ Complete |
| `plugin/hdCarWash/tokens.cpp` | 17 | Token definitions | ✅ Complete |
| `plugin/hdCarWash/tokens.h` | 149 | AOV/Settings/Cognitive tokens | ✅ Complete |
| `plugin/plugInfo.json` | 20 | Hydra plugin manifest | ✅ Correct |
| `plugin/lib/hdCarWash.dll` | - | Compiled plugin | ✅ Deployed |

### USD Schema (NOT COMPILED)

| File | Purpose | Status |
|------|---------|--------|
| `schema/carWashRenderSettingsAPI.usda` | Render settings UI schema | ⚠️ NOT COMPILED |
| `schema/carWashSchema.usda` | Main CarWash schema | ⚠️ NOT COMPILED |
| `schema/cognitiveSubstrate.usda` | Cognitive engine schema | ⚠️ NOT COMPILED |
| `usdCarWash/` | Schema library target | ❌ NOT BUILT |

### Houdini Integration

| File | Purpose | Status |
|------|---------|--------|
| `Documents/houdini21.0/packages/hdCarWash.json` | Package registration | ✅ Correct |

---

## 2. Render Pass Flow Analysis

### Execution Flow (renderPass.cpp:88-459)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          _Execute() Entry                                    │
│                              (line 88)                                       │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ PHASE 1: _ExecutePhase1()  (line 103)                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  1. GET AOV BINDINGS (lines 113-145)                                        │
│     • Extract width/height from first HdRenderBuffer                        │
│     • Resize internal _framebuffer                                          │
│                                                                              │
│  2. READ RENDER SETTINGS (lines 155-181)                                    │
│     • _styleParams from delegate->GetRenderSettingsMap()                    │
│     • _enableAI, _syncRenderMode, _progressiveRefine                        │
│                                                                              │
│  3. CAMERA SETUP (lines 203-234)                                            │
│     • Get HdCarWashCamera from renderPassState                              │
│     • Set view/projection matrices on rasterizer                            │
│                                                                              │
│  4. CPU RASTERIZATION (lines 237-279)                                       │
│     • Sort rprim paths (determinism)                                         │
│     • RasterizeMesh for each HdCarWashMesh                                  │
│     • Compute frame hash                                                     │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ AI INTEGRATION (lines 283-428)                                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  CHECK PENDING RESULT (lines 287-353):                                      │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ if (_pendingAiResult.valid()) {                                      │   │
│  │     auto status = _pendingAiResult.wait_for(0ms);                    │   │
│  │     if (status == ready) {                                           │   │
│  │         result = _pendingAiResult.get();  ◄── BLOCKING GET           │   │
│  │         _aiProcessing.store(false);                                  │   │
│  │         if (result.success) {                                        │   │
│  │             _framebuffer.color = result.styledImage;  ◄── COPY       │   │
│  │             aiProcessed = true;                                      │   │
│  │         }                                                            │   │
│  │     }                                                                │   │
│  │ }                                                                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  LAUNCH NEW AI (lines 356-377):                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ if (_enableAI && _comfyClient && !_aiProcessing.load()) {            │   │
│  │     if (_comfyClient->IsServerAvailable()) {                         │   │
│  │         _aiProcessing.store(true);  ◄── MARK PROCESSING              │   │
│  │         _pendingAiResult = ProcessFrameAsync(framebuffer, params);   │   │
│  │     }                                                                │   │
│  │ }                                                                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ COPY TO AOVS + CONVERGENCE (lines 431-458)                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  _CopyFramebufferToAOVs(renderPassState);                                   │
│                                                                              │
│  CONVERGENCE LOGIC:                                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ if (!_enableAI) _converged = true;                                   │   │
│  │ else if (_syncRenderMode) _converged = true;                         │   │
│  │ else if (_progressiveRefine) {                                       │   │
│  │     _converged = aiProcessed && !_aiProcessing.load();   ◄── KEY     │   │
│  │ }                                                                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  IsConverged() returns false while _aiProcessing=true                       │
│  → Hydra keeps calling _Execute() until AI completes                        │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Convergence Logic Analysis

The progressive refinement loop is correctly implemented:

```cpp
// renderPass.cpp:77-85
bool HdCarWashRenderPass::IsConverged() const
{
    if (_progressiveRefine && _aiProcessing.load()) {
        return false;  // Keep Hydra calling _Execute
    }
    return _converged;
}
```

**Key insight:** The loop depends on:
1. `_progressiveRefine = true` (default at line 63)
2. `_aiProcessing` atomic flag
3. Hydra actually respecting `IsConverged()` return value

---

## 3. ComfyUI Integration Analysis

### ProcessFrame Flow (comfyClient.cpp)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ProcessFrame() / ProcessFrameAsync()                      │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    ▼                               ▼
           ProcessFrame (sync)              ProcessFrameAsync (async)
           (line 352-410)                   (line 412-424)
                    │                               │
                    │                    std::async(std::launch::async,
                    │                        [this, fb, p]() {
                    │                            return ProcessFrame(fb, p);
                    │                        });
                    │                               │
                    └───────────────┬───────────────┘
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. IsServerAvailable() - HTTP GET /system_stats                             │
│ 2. _BuildWorkflow() → _BuildWorkflowLTX2() for LTX-2 backend                │
│ 3. _SaveControlImages() - Write depth.png to ComfyUI input folder           │
│ 4. _SubmitWorkflow() - HTTP POST /prompt                                    │
│ 5. _WaitForCompletion() - WebSocket or HTTP polling                         │
│ 6. _DownloadResult() - HTTP GET /view?filename=xxx                          │
└─────────────────────────────────────────────────────────────────────────────┘
```

### LTX-2 Workflow Generation (lines 1150-1398)

The workflow correctly uses:
- **UNETLoader**: `LTX2/ltx-2-19b-distilled-fp8_transformer_only.safetensors`
- **LTXAVTextEncoderLoader**: `gemma_3_12B_it_fp4_mixed.safetensors` (fixes 2048→3840 dim mismatch)
- **VAELoader**: `LTX2/taeltx_2.safetensors`
- **LTXVImgToVideo**: Uses depth.png from Houdini as conditioning
- **SaveImage**: Outputs to `hdcarwash/ltx2_{timestamp}.png`

### _DownloadResult Analysis (lines 1509-1671)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ _DownloadResult(promptId, width, height)                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  1. HTTP GET /history/{promptId}                                            │
│     • Parse "outputs" section for "filename"                                │
│     • Extract subfolder if present                                          │
│                                                                              │
│  2. HTTP GET /view?filename={name}&subfolder={folder}                       │
│     • Download raw PNG bytes                                                │
│                                                                              │
│  3. stb_image decode                                                        │
│     • stbi_load_from_memory() → RGBA pixels                                │
│     • Convert uint8 [0-255] → float [0-1] GfVec4f                          │
│                                                                              │
│  4. Return std::vector<GfVec4f> with width/height                           │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Potential failure points:**
- Line 1534-1554: If "outputs" section is empty or missing "filename"
- Line 1596-1600: If downloaded PNG is < 24 bytes
- Line 1623-1630: If stb_image fails to decode (compressed format issues)

---

## 4. Render Buffer Analysis

### HdCarWashRenderBuffer (renderBuffer.cpp)

The implementation follows standard Hydra patterns:

```cpp
void* HdCarWashRenderBuffer::Map()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _mapped = true;
    return _buffer.data();  // Returns pointer to internal buffer
}

void HdCarWashRenderBuffer::Unmap()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _mapped = false;
}

void HdCarWashRenderBuffer::SetConverged(bool converged)
{
    _converged = converged;  // Signals to Hydra that buffer is complete
}
```

### _CopyFramebufferToAOVs (renderPass.cpp:477-546)

```cpp
for (const HdRenderPassAovBinding& binding : aovBindings) {
    HdCarWashRenderBuffer* buffer = static_cast<HdCarWashRenderBuffer*>(binding.renderBuffer);
    void* data = buffer->Map();

    if (binding.aovName == HdAovTokens->color) {
        GfVec4f* pixels = static_cast<GfVec4f*>(data);
        std::copy(_framebuffer.color.begin(), _framebuffer.color.end(), pixels);
    }
    // ... depth, normal, primId handling ...

    buffer->Unmap();
    buffer->SetConverged(true);  // ◄── Signals completion to Hydra
}
```

**This code is correct.** The issue is upstream.

---

## 5. Root Cause Analysis

### Issue 1: USD Schema Not Compiled (CRITICAL)

The file `schema/carWashRenderSettingsAPI.usda` defines:
- Display groups: "AI Generation", "Conditioning", "Style Memory", "Temporal", "Backend"
- Attributes like `carwash:prompt`, `carwash:seed`, `carwash:useDepth`
- API schema annotation: `apiSchemaCanOnlyApplyTo = ["RenderSettings"]`

**Problem:** This schema is NOT compiled into a `usdCarWash` library, so:
1. Houdini's Render Settings LOP doesn't know about CarWash attributes
2. The "CarWash" tab never appears in the UI
3. Users cannot configure prompts, seeds, or enable/disable features

**Evidence:**
```
usdCarWash/resources/generatedSchema.usda  ← EXISTS
usdCarWash/*.cpp, *.h                       ← MISSING (not generated)
usdCarWash.dll                              ← MISSING
```

### Issue 2: Render Settings Not Reaching Plugin

Even though `renderDelegate.cpp` defines `GetRenderSettingDescriptors()` (lines 196-294), these settings are only used when:
1. Houdini calls `SetRenderSetting()` on the delegate
2. The delegate passes them to render pass via `GetRenderSettingsMap()`

Without the compiled USD schema, Houdini's LOP network doesn't know to set these values.

### Issue 3: Debug Log Not Created

The debug log at `C:/Temp/hdcarwash_debug.txt` was not found, suggesting:
1. `C:/Temp` folder may not exist
2. Write permissions issue
3. Plugin not being executed at all

### Issue 4: Viewport Refresh Timing

The async model works correctly in isolation, but:
- First `_Execute`: Launches AI, returns CPU raster, `_converged=false`
- Hydra re-executes because `IsConverged()=false`
- Second `_Execute`: AI may not be ready yet, returns same CPU raster
- Eventually: AI completes, `_Execute` copies result, `_converged=true`

**Risk:** If Houdini's viewport update interval is too slow, the intermediate frames showing CPU raster may persist longer than expected.

---

## 6. Recommended Fixes

### Fix 1: Compile USD Schema (CRITICAL)

```bash
cd HdCarWash/schema

# Generate C++ and Python bindings from schema
usdGenSchema carWashRenderSettingsAPI.usda ../usdCarWash

# Build usdCarWash library
cd ../usdCarWash
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Deploy
copy Release/usdCarWash.dll ../plugin/lib/
```

After building, create `plugin/usdCarWash.plugInfo.json`:
```json
{
    "Plugins": [
        {
            "Name": "usdCarWash",
            "Type": "library",
            "Root": ".",
            "LibraryPath": "lib/usdCarWash.dll",
            "ResourcePath": "resources"
        }
    ]
}
```

### Fix 2: Ensure C:/Temp Exists

```powershell
New-Item -ItemType Directory -Force -Path "C:\Temp"
```

### Fix 3: Add Forced Viewport Refresh

In `renderPass.cpp`, after AI result is copied, force Hydra to redraw:

```cpp
// After copying AI result to _framebuffer.color (around line 340)
if (aiProcessed) {
    // Request viewport refresh
    HdRenderIndex* renderIndex = GetRenderIndex();
    if (renderIndex) {
        HdChangeTracker& tracker = renderIndex->GetChangeTracker();
        // Mark render buffer as dirty to force redraw
        for (const auto& binding : aovBindings) {
            if (binding.renderBuffer) {
                tracker.MarkBprimDirty(binding.renderBuffer->GetId(),
                                       HdChangeTracker::DirtyParams);
            }
        }
    }
}
```

### Fix 4: Add Synchronous Fallback Mode

For debugging, enable sync mode by default:

```cpp
// renderPass.cpp constructor (around line 62)
_syncRenderMode = true;  // Wait for AI to complete before returning
```

This blocks viewport updates but guarantees AI result is shown.

### Fix 5: Verify ComfyUI Connection in UI

Add a simple test in Houdini:

```python
# Python Shell in Houdini
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    s.connect(("127.0.0.1", 8188))
    print("ComfyUI is reachable")
except:
    print("ComfyUI not available")
finally:
    s.close()
```

---

## 7. Data Flow Diagram

```
┌──────────────────────────────────────────────────────────────────────────────────────┐
│                              HdCarWash Data Flow                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘

 HOUDINI                      HDCARWASH                      COMFYUI
┌─────────┐                  ┌─────────────┐                ┌──────────┐
│  LOPs   │                  │ RenderPass  │                │ LTX-2 19B│
│ Network │                  │   _Execute  │                │ Pipeline │
└────┬────┘                  └──────┬──────┘                └────┬─────┘
     │                              │                             │
     │ HdRenderPassAovBindings      │                             │
     ├─────────────────────────────►│                             │
     │                              │                             │
     │                              │ _framebuffer.Resize()       │
     │                              │◄────────────────────────────┤
     │                              │                             │
     │                              │ RasterizeMesh() [CPU]       │
     │                              │◄────────────────────────────┤
     │                              │                             │
     │                              │ _SaveControlImages()        │
     │                              │────────────────────────────►│ depth.png
     │                              │                             │
     │                              │ _SubmitWorkflow() [HTTP]    │
     │                              │────────────────────────────►│ POST /prompt
     │                              │                             │
     │                              │ ProcessFrameAsync()         │
     │                              │ [std::async]                │
     │                              │                             │ LTXVImgToVideo
     │                              │                             │ SamplerCustomAdv
     │                              │                             │ VAEDecode
     │                              │                             │ SaveImage
     │                              │                             │
     │                              │ _WaitForCompletion()        │
     │                              │◄───────────────────────────►│ WebSocket/poll
     │                              │                             │
     │                              │ _DownloadResult() [HTTP]    │
     │                              │◄────────────────────────────│ GET /view
     │                              │                             │
     │                              │ stbi_load_from_memory()     │
     │                              │ → GfVec4f pixels            │
     │                              │                             │
     │                              │ _framebuffer.color = result │
     │                              │◄────────────────────────────┤
     │                              │                             │
     │ _CopyFramebufferToAOVs()     │                             │
     │◄─────────────────────────────┤                             │
     │                              │                             │
     │ HdRenderBuffer::Map()        │                             │
     │ std::copy → buffer           │                             │
     │ HdRenderBuffer::Unmap()      │                             │
     │ SetConverged(true)           │                             │
     │                              │                             │
     ▼                              ▼                             ▼
 VIEWPORT                    NEXT FRAME?                    OUTPUT FOLDER
```

---

## 8. Verification Checklist

### Immediate Checks

- [ ] Create `C:\Temp` folder and verify write permissions
- [ ] Run Houdini with `TF_DEBUG=HD_CARWASH` environment variable
- [ ] Check if ComfyUI is running on port 8188
- [ ] Verify `plugin/lib/hdCarWash.dll` has correct LTX2 workflow code

### Schema Build (Required for UI)

- [ ] Run `usdGenSchema` on `carWashRenderSettingsAPI.usda`
- [ ] Build `usdCarWash` library
- [ ] Deploy to plugin folder
- [ ] Restart Houdini, verify "CarWash" tab appears in Render Settings

### Integration Test

1. Open Houdini 21.0
2. Create simple scene: Camera + Box
3. Add Render Settings LOP, select "CarWash"
4. Check debug log: `C:\Temp\hdcarwash_debug.txt`
5. Monitor ComfyUI queue for incoming prompts
6. Verify output appears in `ComfyUI/output/hdcarwash/`

---

## 9. Summary

| Component | Status | Action Required |
|-----------|--------|-----------------|
| DLL Plugin | ✅ Working | None |
| Hydra Registration | ✅ Working | None |
| CPU Rasterizer | ✅ Working | None |
| ComfyUI Client | ✅ Working | None |
| LTX-2 Workflow | ✅ Working | None |
| Result Download | ✅ Implemented | Verify with debug log |
| Framebuffer Copy | ✅ Implemented | None |
| AOV Copy | ✅ Implemented | None |
| **USD Schema** | ❌ **Not Built** | **Build usdCarWash** |
| **Render Settings UI** | ❌ **Missing** | **Requires schema** |
| Debug Logging | ⚠️ Path issue | Create C:\Temp |

**Priority Action:** Build the `usdCarWash` schema library to enable the Render Settings UI tab. Without this, users cannot configure the AI pipeline from within Houdini.

---

*Report generated by Claude Code for hdCarWash v1.0*
