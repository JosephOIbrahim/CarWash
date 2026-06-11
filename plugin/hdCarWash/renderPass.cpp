// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// renderPass.cpp — Frame rendering orchestration

#include "renderPass.h"
#include "renderDelegate.h"
#include "renderBuffer.h"
#include "mesh.h"
#include "camera.h"
#include "debugCodes.h"

#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/types.h"   // HdFormat, HdDataSizeOfFormat, HdGetComponent*
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/half.h"       // GfHalf for Float16 conversion

#include <algorithm>  // std::sort for deterministic mesh ordering, std::min/max
#include <cstdint>    // uint8_t, int32_t, uint64_t
#include <cmath>      // std::lround
#include <cstddef>    // size_t
#include <functional> // std::hash for the style-param cache key (#2)
#include <fstream>    // Debug file logging

PXR_NAMESPACE_OPEN_SCOPE

// Helper to safely get typed values from settings map
namespace {
    template<typename T>
    T GetSetting(const HdRenderSettingsMap& settings, const TfToken& key, const T& defaultValue) {
        auto it = settings.find(key);
        if (it != settings.end() && it->second.IsHolding<T>()) {
            return it->second.UncheckedGet<T>();
        }
        return defaultValue;
    }

    // Stable hash of the style params that change a generation's output. The
    // base seed is included, but NOT the time-jitter applied at submission, so
    // an unchanged scene reads as cached rather than regenerated forever. (#2)
    size_t HashStyleParams(const HdCarWashStyleParams& p) {
        size_t h = 1469598103934665603ull;  // FNV-1a offset basis
        auto mix = [&h](size_t v) {
            h ^= v + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        };
        mix(std::hash<std::string>{}(p.prompt));
        mix(std::hash<std::string>{}(p.negativePrompt));
        mix(std::hash<int>{}(p.inferenceSteps));
        mix(std::hash<float>{}(p.guidanceScale));
        mix(std::hash<float>{}(p.controlNetStrength));
        mix(std::hash<float>{}(p.normalControlNetStrength));
        mix(std::hash<int>{}(p.seed));
        mix(std::hash<bool>{}(p.useDepthControl));
        mix(std::hash<bool>{}(p.useNormalControl));
        mix(std::hash<bool>{}(p.useEdgeControl));
        mix(std::hash<bool>{}(p.deterministic));
        return h;
    }
}

HdCarWashRenderPass::HdCarWashRenderPass(
    HdRenderIndex* index,
    HdRprimCollection const& collection,
    HdCarWashRenderDelegate* delegate)
    : HdRenderPass(index, collection)
    , _delegate(delegate)
    , _rasterizer(std::make_unique<HdCarWashRasterizer>())
    , _comfyClient(std::make_unique<HdCarWashComfyClient>("http://127.0.0.1:8188"))  // WS URL derived from server (#5)
    , _styleParams()
    , _enableAI(true)  // Enable AI by default, will gracefully fallback if unavailable
    , _frameNumber(0)
    , _converged(false)
{
    // Set default style parameters
    _styleParams.prompt = "photorealistic 3D render, cinematic lighting, sharp details";
    _styleParams.negativePrompt = "blurry, low quality, distorted";
    _styleParams.inferenceSteps = 20;
    _styleParams.guidanceScale = 7.5f;
    _styleParams.seed = 42;  // Deterministic seed for reproducibility
    _styleParams.useDepthControl = true;
    _styleParams.useNormalControl = false;  // Disabled - requires canny ControlNet model
    _styleParams.controlNetStrength = 0.8f;

    // Render mode settings
    _syncRenderMode = false;     // Default: async for viewport interactivity
    _progressiveRefine = true;   // Default: keep refining until AI completes

    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashRenderPass created (AI=%s, sync=%s, progressive=%s)\n",
                 _enableAI ? "enabled" : "disabled",
                 _syncRenderMode ? "yes" : "no",
                 _progressiveRefine ? "yes" : "no");
}

HdCarWashRenderPass::~HdCarWashRenderPass()
{
    // Cancel any in-flight AI job BEFORE the _pendingAiResult future is
    // destroyed. Otherwise the future's destructor blocks the UI thread until
    // the job finishes (up to the full completion timeout) whenever the artist
    // stops the render or switches renderers. CancelPending() also POSTs
    // /interrupt to stop the server-side job. (#5)
    if (_comfyClient) {
        _comfyClient->CancelPending();
    }
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashRenderPass destroyed\n");
}

void
HdCarWashRenderPass::_ReportAiError(const std::string& message)
{
    if (message.empty() || message == _lastWarnedError) {
        return;
    }
    _lastWarnedError = message;
    TF_WARN("hdCarWash: %s", message.c_str());
}

bool
HdCarWashRenderPass::IsConverged() const
{
    // If progressive refinement is enabled and AI is still processing,
    // report NOT converged so Hydra keeps calling _Execute
    if (_progressiveRefine && _aiProcessing.load()) {
        return false;
    }
    return _converged;
}

void
HdCarWashRenderPass::_Execute(
    HdRenderPassStateSharedPtr const& renderPassState,
    TfTokenVector const& renderTags)
{
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashRenderPass::_Execute frame %d\n",
                 _frameNumber);

    // Use Phase 1 (CPU rasterization) instead of Phase 0 (solid color)
    _ExecutePhase1(renderPassState);

    _frameNumber++;
    // _converged is set by _ExecutePhase1's convergence logic (lines 440-453)
    // Do NOT override here — it kills progressive refinement
}

void
HdCarWashRenderPass::_ExecutePhase1(
    HdRenderPassStateSharedPtr const& renderPassState)
{
    TF_DEBUG_MSG(HD_CARWASH, "Executing Phase 1: CPU rasterization\n");

    // DEBUG: Write to file so we can see what's happening
    std::ofstream debugLog("C:/Temp/hdcarwash_debug.txt", std::ios::app);
    debugLog << "=== Phase 1 Execute Frame " << _frameNumber << " ===" << std::endl;

    // Get AOV bindings to determine framebuffer size
    HdRenderPassAovBindingVector const& aovBindings =
        renderPassState->GetAovBindings();

    if (aovBindings.empty()) {
        TF_DEBUG_MSG(HD_CARWASH, "No AOV bindings, nothing to render\n");
        debugLog << "ERROR: No AOV bindings" << std::endl;
        debugLog.close();
        return;
    }
    debugLog << "AOV bindings count: " << aovBindings.size() << std::endl;
    for (const auto& b : aovBindings) {
        debugLog << "  AOV: " << b.aovName.GetText() << std::endl;
    }

    // Determine framebuffer dimensions from first valid AOV
    unsigned int width = 0;
    unsigned int height = 0;
    for (const auto& binding : aovBindings) {
        if (binding.renderBuffer) {
            HdCarWashRenderBuffer* buffer =
                static_cast<HdCarWashRenderBuffer*>(binding.renderBuffer);
            width = buffer->GetWidth();
            height = buffer->GetHeight();
            break;
        }
    }

    if (width == 0 || height == 0) {
        TF_WARN("Invalid framebuffer dimensions");
        debugLog << "ERROR: Invalid dimensions " << width << "x" << height << std::endl;
        debugLog.close();
        return;
    }

    debugLog << "Framebuffer: " << width << "x" << height << std::endl;
    TF_DEBUG_MSG(HD_CARWASH, "Framebuffer: %d x %d\n", width, height);

    // Resize internal framebuffer
    _framebuffer.Resize(width, height);
    _rasterizer->SetFramebuffer(&_framebuffer);

    // Read style parameters from render settings
    HdRenderSettingsMap const& settings = _delegate->GetRenderSettingsMap();
    _styleParams.prompt = GetSetting<std::string>(settings,
        HdCarWashSettingsTokens->prompt, _styleParams.prompt);
    _styleParams.negativePrompt = GetSetting<std::string>(settings,
        HdCarWashSettingsTokens->negativePrompt, _styleParams.negativePrompt);
    _styleParams.inferenceSteps = GetSetting<int>(settings,
        HdCarWashSettingsTokens->inferenceSteps, _styleParams.inferenceSteps);
    _styleParams.guidanceScale = GetSetting<float>(settings,
        HdCarWashSettingsTokens->guidanceScale, _styleParams.guidanceScale);
    _styleParams.seed = GetSetting<int>(settings,
        HdCarWashSettingsTokens->seed, _styleParams.seed);
    _styleParams.controlNetStrength = GetSetting<float>(settings,
        HdCarWashSettingsTokens->depthControlNetStrength, _styleParams.controlNetStrength);
    _styleParams.normalControlNetStrength = GetSetting<float>(settings,
        HdCarWashSettingsTokens->normalControlNetStrength, _styleParams.normalControlNetStrength);
    _styleParams.useDepthControl = GetSetting<bool>(settings,
        HdCarWashSettingsTokens->enableDepthControl, _styleParams.useDepthControl);
    _styleParams.useNormalControl = GetSetting<bool>(settings,
        HdCarWashSettingsTokens->enableNormalControl, _styleParams.useNormalControl);
    // #3b: when deterministicMode is on, ProcessFrame uses the fixed seed (no
    // time-jitter), so the same scene+seed reproduces the same AI render.
    _styleParams.deterministic = GetSetting<bool>(settings,
        HdCarWashSettingsTokens->deterministicMode, _styleParams.deterministic);

    // Render mode settings
    _enableAI = GetSetting<bool>(settings,
        HdCarWashSettingsTokens->enableAI, _enableAI);
    _syncRenderMode = GetSetting<bool>(settings,
        HdCarWashSettingsTokens->syncRenderMode, _syncRenderMode);
    _progressiveRefine = GetSetting<bool>(settings,
        HdCarWashSettingsTokens->progressiveRefine, _progressiveRefine);

    // (#5) Completion timeout + ComfyUI server URL from settings. Defaults
    // preserve current behavior; SetServerUrl also re-derives the WebSocket
    // progress URL so both endpoints track the same host:port.
    _comfyClient->SetCompletionTimeout(GetSetting<float>(settings,
        HdCarWashSettingsTokens->comfyuiTimeoutSeconds, _comfyClient->GetCompletionTimeout()));
    std::string serverUrl = GetSetting<std::string>(settings,
        HdCarWashSettingsTokens->comfyuiServerUrl, _comfyClient->GetServerUrl());
    if (serverUrl != _comfyClient->GetServerUrl()) {
        _comfyClient->SetServerUrl(serverUrl);
    }

    debugLog << "Render mode: enableAI=" << _enableAI
             << ", sync=" << _syncRenderMode
             << ", progressive=" << _progressiveRefine << std::endl;
    debugLog << "Style params: prompt='" << _styleParams.prompt.substr(0, 50) << "...'" << std::endl;
    debugLog << "  depthStrength=" << _styleParams.controlNetStrength
             << ", useDepth=" << _styleParams.useDepthControl
             << ", useNormal=" << _styleParams.useNormalControl << std::endl;

    // Clear buffers - but preserve color if AI processing is active
    // This prevents flickering between AI frames
    bool preserveColorBuffer = _enableAI && _aiProcessing.load();
    if (!preserveColorBuffer) {
        GfVec4f clearColor(0.1f, 0.1f, 0.15f, 1.0f);  // Dark blue-grey background
        _rasterizer->Clear(clearColor, 1.0f);
    } else {
        // Only clear depth/normal, preserve color from previous AI result
        _rasterizer->ClearDepthOnly(1.0f);
    }

    // Get camera and set up matrices
    HdCarWashCamera* camera = _GetCamera(renderPassState);
    debugLog << "Camera: " << (camera ? "found" : "using default") << std::endl;
    if (camera) {
        float aspectRatio = static_cast<float>(width) / height;
        GfMatrix4d viewMatrix = camera->GetViewMatrix();
        GfMatrix4d projMatrix = camera->ComputeProjectionMatrix(aspectRatio);
        GfMatrix4d viewProj = viewMatrix * projMatrix;

        _rasterizer->SetViewMatrix(viewMatrix);
        _rasterizer->SetViewProjectionMatrix(viewProj);

        // Extract camera position from view matrix (inverse of camera transform)
        GfMatrix4d invView = viewMatrix.GetInverse();
        GfVec3f cameraPos(static_cast<float>(invView[3][0]),
                          static_cast<float>(invView[3][1]),
                          static_cast<float>(invView[3][2]));
        _rasterizer->SetCameraPosition(cameraPos);

        TF_DEBUG_MSG(HD_CARWASH, "Camera set up, aspect ratio: %.2f, pos: (%.2f, %.2f, %.2f)\n",
                     aspectRatio, cameraPos[0], cameraPos[1], cameraPos[2]);
    } else {
        // Default camera looking at origin
        GfMatrix4d viewMatrix(1.0);
        viewMatrix.SetTranslate(GfVec3d(0, 0, -5));
        GfMatrix4d projMatrix(1.0);

        _rasterizer->SetViewMatrix(viewMatrix);
        _rasterizer->SetViewProjectionMatrix(viewMatrix * projMatrix);
        _rasterizer->SetCameraPosition(GfVec3f(0.0f, 0.0f, 5.0f));

        TF_DEBUG_MSG(HD_CARWASH, "Using default camera\n");
    }

    // Iterate through all mesh prims and rasterize
    HdRenderIndex* renderIndex = GetRenderIndex();
    HdRprimCollection const& collection = GetRprimCollection();

    // Get all rprims in the collection
    SdfPathVector rprimPaths = renderIndex->GetRprimIds();

    // DETERMINISM: Sort paths for canonical ordering (same as RAG System)
    std::sort(rprimPaths.begin(), rprimPaths.end());

    debugLog << "Rprim paths count: " << rprimPaths.size() << std::endl;
    for (const auto& path : rprimPaths) {
        debugLog << "  Rprim path: " << path.GetText() << std::endl;
    }

    int meshCount = 0;
    for (const auto& path : rprimPaths) {
        const HdRprim* rprim = renderIndex->GetRprim(path);
        if (!rprim) {
            debugLog << "  NULL rprim for: " << path.GetText() << std::endl;
            continue;
        }

        // Check if it's our mesh type using dynamic_cast
        const HdCarWashMesh* mesh = dynamic_cast<const HdCarWashMesh*>(rprim);
        if (mesh) {
            debugLog << "  Found HdCarWashMesh: " << path.GetText() << std::endl;
            _rasterizer->RasterizeMesh(mesh);
            meshCount++;
        } else {
            debugLog << "  NOT HdCarWashMesh: " << path.GetText() << std::endl;
        }
    }

    debugLog << "Meshes rasterized: " << meshCount << std::endl;
    TF_DEBUG_MSG(HD_CARWASH, "Rasterized %d meshes\n", meshCount);

    // Compute determinism hash (before AI processing)
    HdCarWashFrameHash frameHash = _framebuffer.ComputeAuthoritativeHash();  // Full-buffer, drift-sensitive — the authoritative VERIFIED signal
    debugLog << "FrameHash (pre-AI): 0x" << std::hex << frameHash.combined << std::dec
             << " (sampled=" << frameHash.pixelsSampled
             << ", nonEmpty=" << frameHash.nonEmptyPixels << ")" << std::endl;
    TF_DEBUG_MSG(HD_CARWASH, "Frame hash (pre-AI): 0x%016llx\n",
                 static_cast<unsigned long long>(frameHash.combined));

    // Identity of the conditioning the AI actually consumes. Since #4 feeds the
    // shaded color (beauty) buffer to the model as the first-frame image, the
    // hash MUST include colorHash — otherwise a lighting or material edit (which
    // changes shading but not depth/normal/id) would never re-trigger a new
    // generation. At hash time the color buffer always holds the deterministic
    // CPU render (the AI result is written later in this frame and re-rasterized
    // before the next hash), so including it is stable for a static scene and
    // does not loop — this relies on the rasterizer being frame-to-frame
    // deterministic, which is a verified core property of the project. Paired
    // with the style-param hash so prompt/seed/strength edits also count as new
    // work. (#2 + #4)
    uint64_t conditioningHash = frameHash.depthHash;
    conditioningHash = (conditioningHash * 1099511628211ull) ^ frameHash.normalHash;
    conditioningHash = (conditioningHash * 1099511628211ull) ^ frameHash.idHash;
    conditioningHash = (conditioningHash * 1099511628211ull) ^ frameHash.colorHash;
    const size_t paramsHash = HashStyleParams(_styleParams);
    const bool conditioningChanged =
        !_hasSubmittedOnce ||
        conditioningHash != _lastConditioningHash ||
        paramsHash != _lastParamsHash;
    debugLog << "Conditioning: hash=0x" << std::hex << conditioningHash << std::dec
             << " changed=" << (conditioningChanged ? "yes" : "no") << std::endl;

    // =========================================================================
    // AI STYLIZATION (Non-Blocking ComfyUI Integration)
    // =========================================================================
    bool aiProcessed = false;

    // Check if previous async AI result is ready
    if (_pendingAiResult.valid()) {
        auto status = _pendingAiResult.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            debugLog << "Previous AI result ready, retrieving..." << std::endl;
            HdCarWashRenderResult result = _pendingAiResult.get();
            _aiProcessing.store(false);

            if (result.success) {
                debugLog << "AI processing successful!" << std::endl;
                _lastWarnedError.clear();  // a later identical error will warn again (#5)
                debugLog << "  Result size: " << result.styledImage.size() << " pixels" << std::endl;
                TF_DEBUG_MSG(HD_CARWASH, "AI stylization complete, %zu pixels\n",
                             result.styledImage.size());

                // Overwrite color buffer with AI result (thread-safe swap)
                std::lock_guard<std::mutex> lock(_resultMutex);
                size_t fbSize = _framebuffer.color.size();
                size_t aiSize = result.styledImage.size();

                if (aiSize == fbSize) {
                    _framebuffer.color = std::move(result.styledImage);
                    aiProcessed = true;
                    debugLog << "AI result: exact size match" << std::endl;
                } else {
                    // Handle SD/SDXL 8-pixel rounding: allow up to 8 pixel difference in each dimension
                    int widthDiff = static_cast<int>(width) - static_cast<int>(result.width);
                    int heightDiff = static_cast<int>(height) - static_cast<int>(result.height);
                    bool widthOK = (widthDiff >= 0 && widthDiff <= 8);
                    bool heightOK = (heightDiff >= 0 && heightDiff <= 8);

                    debugLog << "AI result: " << result.width << "x" << result.height
                             << " vs framebuffer " << width << "x" << height
                             << " (diff: " << widthDiff << "x" << heightDiff << ")" << std::endl;

                    if (widthOK && heightOK && result.width > 0 && result.height > 0) {
                        // Copy AI result into framebuffer, centered or top-left aligned
                        size_t copyWidth = std::min(static_cast<size_t>(result.width), static_cast<size_t>(width));
                        size_t copyHeight = std::min(static_cast<size_t>(result.height), static_cast<size_t>(height));

                        for (size_t y = 0; y < copyHeight; y++) {
                            for (size_t x = 0; x < copyWidth; x++) {
                                size_t srcIdx = y * result.width + x;
                                size_t dstIdx = y * width + x;
                                if (srcIdx < aiSize && dstIdx < fbSize) {
                                    _framebuffer.color[dstIdx] = result.styledImage[srcIdx];
                                }
                            }
                        }
                        aiProcessed = true;
                        debugLog << "AI result: copied " << copyWidth << "x" << copyHeight << " pixels" << std::endl;
                    } else {
                        debugLog << "WARNING: AI result size mismatch too large, keeping CPU render" << std::endl;
                        debugLog << "  (widthOK=" << widthOK << ", heightOK=" << heightOK << ")" << std::endl;
                    }
                }

                if (aiProcessed) {
                    HdCarWashFrameHash postAIHash = _framebuffer.ComputeAuthoritativeHash();
                    debugLog << "FrameHash (post-AI): 0x" << std::hex << postAIHash.combined
                             << std::dec << std::endl;
                }
            } else {
                debugLog << "AI processing failed: " << result.errorMessage << std::endl;
                _ReportAiError(result.errorMessage);  // surface to the Houdini console (#5)
            }
        } else {
            debugLog << "AI processing still in progress..." << std::endl;
        }
    }

    // Launch new async AI processing only when there is genuinely new work:
    // AI enabled, no job already in flight, and the conditioning or params
    // changed since our last submission. The unchanged case is exactly what
    // previously looped forever — every completed frame immediately resubmitted
    // an identical 25-frame video generation. (#2)
    if (_enableAI && _comfyClient && !_aiProcessing.load() && conditioningChanged) {
        debugLog << "Checking ComfyUI server availability..." << std::endl;

        if (_comfyClient->IsServerAvailable()) {
            debugLog << "ComfyUI server available, launching async processing..." << std::endl;
            TF_DEBUG_MSG(HD_CARWASH, "Launching async AI stylization\n");

            // Mark as processing BEFORE launching async, and record the identity
            // of what we're submitting so an unchanged next frame won't resubmit.
            _aiProcessing.store(true);
            _hasSubmittedOnce = true;
            _lastConditioningHash = conditioningHash;
            _lastParamsHash = paramsHash;

            // Launch async with COPY of framebuffer and params (safe capture)
            // The ProcessFrameAsync uses value capture internally
            _pendingAiResult = _comfyClient->ProcessFrameAsync(_framebuffer, _styleParams);

            debugLog << "Async AI processing launched" << std::endl;
        } else {
            debugLog << "ComfyUI server not available, using CPU rasterization only" << std::endl;
            TF_DEBUG_MSG(HD_CARWASH, "ComfyUI not available, fallback to CPU render\n");
        }
    } else if (!_enableAI) {
        debugLog << "AI disabled" << std::endl;
    } else if (!conditioningChanged && !_aiProcessing.load()) {
        debugLog << "Conditioning unchanged since last submission — "
                    "skipping regeneration (converged)" << std::endl;
    }

    debugLog << "AI processed this frame: " << (aiProcessed ? "yes" : "no")
             << ", async pending: " << (_aiProcessing.load() ? "yes" : "no") << std::endl;

    // =========================================================================
    // SYNCHRONOUS MODE: Wait for AI to complete before returning
    // =========================================================================
    if (_syncRenderMode && _enableAI && _aiProcessing.load() && _pendingAiResult.valid()) {
        debugLog << "Sync mode: waiting for AI to complete..." << std::endl;
        TF_DEBUG_MSG(HD_CARWASH, "Sync mode: waiting for AI completion\n");

        // Wait for the async result (blocking)
        HdCarWashRenderResult syncResult = _pendingAiResult.get();
        _aiProcessing.store(false);

        if (syncResult.success) {
            debugLog << "Sync AI completed successfully!" << std::endl;
            _lastWarnedError.clear();  // a later identical error will warn again (#5)
            std::lock_guard<std::mutex> lock(_resultMutex);
            size_t fbSize = _framebuffer.color.size();
            size_t aiSize = syncResult.styledImage.size();

            if (aiSize == fbSize) {
                _framebuffer.color = std::move(syncResult.styledImage);
                aiProcessed = true;
            } else {
                // Handle SD/SDXL 8-pixel rounding
                int widthDiff = static_cast<int>(width) - static_cast<int>(syncResult.width);
                int heightDiff = static_cast<int>(height) - static_cast<int>(syncResult.height);
                bool widthOK = (widthDiff >= 0 && widthDiff <= 8);
                bool heightOK = (heightDiff >= 0 && heightDiff <= 8);

                if (widthOK && heightOK && syncResult.width > 0 && syncResult.height > 0) {
                    size_t copyWidth = std::min(static_cast<size_t>(syncResult.width), static_cast<size_t>(width));
                    size_t copyHeight = std::min(static_cast<size_t>(syncResult.height), static_cast<size_t>(height));
                    for (size_t y = 0; y < copyHeight; y++) {
                        for (size_t x = 0; x < copyWidth; x++) {
                            size_t srcIdx = y * syncResult.width + x;
                            size_t dstIdx = y * width + x;
                            if (srcIdx < aiSize && dstIdx < fbSize) {
                                _framebuffer.color[dstIdx] = syncResult.styledImage[srcIdx];
                            }
                        }
                    }
                    aiProcessed = true;
                    debugLog << "Sync AI: copied " << copyWidth << "x" << copyHeight << " pixels" << std::endl;
                }
            }
        } else {
            debugLog << "Sync AI failed: " << syncResult.errorMessage << std::endl;
            _ReportAiError(syncResult.errorMessage);  // surface to the Houdini console (#5)
        }
    }

    // Copy framebuffer to AOV buffers
    _CopyFramebufferToAOVs(renderPassState);

    // =========================================================================
    // CONVERGENCE LOGIC
    // =========================================================================
    // - If AI is disabled: always converged
    // - If sync mode: we waited above, so converged
    // - If progressive mode: only converge when AI has completed at least once
    // - Otherwise: converge immediately (original behavior)
    if (!_enableAI) {
        _converged = true;
        debugLog << "Converged: AI disabled" << std::endl;
    } else if (_syncRenderMode) {
        _converged = true;
        debugLog << "Converged: sync mode completed" << std::endl;
    } else if (_progressiveRefine) {
        // Converge once no AI job is in flight AND the current conditioning
        // matches what we last submitted (nothing new to generate). Gating on
        // the change flag rather than `aiProcessed` is what lets an idle scene
        // settle — `aiProcessed` is only true on the single frame a result
        // lands, so the old test could never hold after a relaunch. (#2)
        _converged = !_aiProcessing.load() && !conditioningChanged;
        debugLog << "Progressive mode: converged=" << _converged << std::endl;
    } else {
        _converged = true;
        debugLog << "Converged: default behavior" << std::endl;
    }

    debugLog << "Phase 1 complete, converged=" << _converged << std::endl;
    debugLog.close();
    TF_DEBUG_MSG(HD_CARWASH, "Phase 1 render complete (converged=%s)\n",
                 _converged ? "yes" : "no");
}

HdCarWashCamera*
HdCarWashRenderPass::_GetCamera(HdRenderPassStateSharedPtr const& renderPassState)
{
    // Get camera from render pass state
    HdCamera const* hdCamera = renderPassState->GetCamera();

    if (!hdCamera) {
        return nullptr;
    }

    // Try to cast to our camera type
    return const_cast<HdCarWashCamera*>(
        dynamic_cast<HdCarWashCamera const*>(hdCamera));
}

namespace {

// FINDING #4a fix — format-aware AOV writes.
//
// The internal framebuffer always stores float32 source data (GfVec4f color,
// float depth, GfVec3f normal, int32_t ids). The destination HdRenderBuffer,
// however, may have been allocated by the client (Solaris/Husk/usdview) in a
// *narrower* format: color is routinely UNorm8Vec4 (4 B/px) or Float16Vec4
// (8 B/px), not Float32Vec4 (16 B/px). Blindly std::copy-ing the float32
// source into the mapped buffer therefore overruns the heap allocation by up
// to 4x. Every write below is bounded by the buffer's real per-pixel byte
// size (HdDataSizeOfFormat(fmt)), and color is converted to the destination
// component format rather than memcpy'd.

inline void
_WriteFloatComponent(void* dst, HdFormat componentFormat, float value)
{
    switch (componentFormat) {
        case HdFormatFloat32:
            *static_cast<float*>(dst) = value;
            break;
        case HdFormatFloat16:
            *static_cast<GfHalf*>(dst) = GfHalf(value);
            break;
        case HdFormatUNorm8: {
            float c = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
            *static_cast<uint8_t*>(dst) =
                static_cast<uint8_t>(std::lround(c * 255.0f));
            break;
        }
        case HdFormatSNorm8: {
            float c = value < -1.0f ? -1.0f : (value > 1.0f ? 1.0f : value);
            *static_cast<int8_t*>(dst) =
                static_cast<int8_t>(std::lround(c * 127.0f));
            break;
        }
        default:
            break;  // unsupported component family; caller guards this
    }
}

bool
_WriteColorAov(void* data, HdFormat fmt,
               std::vector<GfVec4f> const& src, size_t numPixels)
{
    const HdFormat compFmt = HdGetComponentFormat(fmt);
    const size_t   compCount = HdGetComponentCount(fmt);

    if (compFmt != HdFormatFloat32 &&
        compFmt != HdFormatFloat16 &&
        compFmt != HdFormatUNorm8 &&
        compFmt != HdFormatSNorm8) {
        return false;  // integer / unsupported color format
    }
    const size_t compSize = HdDataSizeOfFormat(compFmt);
    if (compSize == 0 || compCount == 0 || compCount > 4) {
        return false;
    }

    uint8_t* out = static_cast<uint8_t*>(data);
    for (size_t i = 0; i < numPixels; ++i) {
        const GfVec4f& px = src[i];
        for (size_t c = 0; c < compCount; ++c) {
            _WriteFloatComponent(out + c * compSize, compFmt, px[static_cast<int>(c)]);
        }
        out += compCount * compSize;
    }
    return true;
}

} // anonymous namespace

void
HdCarWashRenderPass::_CopyFramebufferToAOVs(
    HdRenderPassStateSharedPtr const& renderPassState)
{
    HdRenderPassAovBindingVector const& aovBindings =
        renderPassState->GetAovBindings();

    for (const HdRenderPassAovBinding& binding : aovBindings) {
        if (!binding.renderBuffer) {
            continue;
        }

        HdCarWashRenderBuffer* buffer =
            static_cast<HdCarWashRenderBuffer*>(binding.renderBuffer);

        unsigned int width = buffer->GetWidth();
        unsigned int height = buffer->GetHeight();

        if (width != _framebuffer.width || height != _framebuffer.height) {
            TF_WARN("AOV buffer size mismatch");
            continue;
        }

        void* data = buffer->Map();
        if (!data) {
            TF_WARN("Failed to map render buffer for '%s'",
                    binding.aovName.GetText());
            continue;
        }

        size_t numPixels = static_cast<size_t>(width) * height;

        // Destination format + capacity. The buffer was allocated as exactly
        // HdDataSizeOfFormat(fmt) * width * height bytes, so that product is
        // the hard upper bound for every write below.
        const HdFormat fmt = buffer->GetFormat();
        const size_t dstBytesPerPixel = HdDataSizeOfFormat(fmt);
        const size_t dstCapacity = dstBytesPerPixel * numPixels;

        // Copy appropriate data based on AOV type. Each branch either converts
        // the float32 source into the destination format, or verifies the
        // source byte count matches the destination capacity before copying.
        if (binding.aovName == HdAovTokens->color) {
            if (!_WriteColorAov(data, fmt, _framebuffer.color, numPixels)) {
                TF_WARN("Unsupported color AOV format (%d) for '%s'; skipping",
                        static_cast<int>(fmt), binding.aovName.GetText());
            }
        }
        else if (binding.aovName == HdAovTokens->depth) {
            const size_t srcBytes = _framebuffer.depth.size() * sizeof(float);
            if (fmt == HdFormatFloat32 && srcBytes == dstCapacity) {
                std::copy(_framebuffer.depth.begin(),
                          _framebuffer.depth.end(),
                          static_cast<float*>(data));
            } else if (HdGetComponentFormat(fmt) == HdFormatFloat16 &&
                       HdGetComponentCount(fmt) == 1 &&
                       dstCapacity >= numPixels * sizeof(GfHalf)) {
                GfHalf* out = static_cast<GfHalf*>(data);
                for (size_t i = 0; i < numPixels; ++i) {
                    out[i] = GfHalf(_framebuffer.depth[i]);
                }
            } else {
                TF_WARN("Depth AOV format mismatch (fmt=%d, %zu src vs %zu dst "
                        "bytes) for '%s'; skipping",
                        static_cast<int>(fmt), srcBytes, dstCapacity,
                        binding.aovName.GetText());
            }
        }
        else if (binding.aovName == HdAovTokens->normal) {
            const size_t srcBytes = _framebuffer.normal.size() * sizeof(GfVec3f);
            const HdFormat compFmt = HdGetComponentFormat(fmt);
            const size_t compCount = HdGetComponentCount(fmt);
            if (fmt == HdFormatFloat32Vec3 && srcBytes == dstCapacity) {
                std::copy(_framebuffer.normal.begin(),
                          _framebuffer.normal.end(),
                          static_cast<GfVec3f*>(data));
            } else if (compCount >= 3 &&
                       (compFmt == HdFormatFloat32 ||
                        compFmt == HdFormatFloat16 ||
                        compFmt == HdFormatUNorm8 ||
                        compFmt == HdFormatSNorm8) &&
                       dstBytesPerPixel != 0) {
                const size_t compSize = HdDataSizeOfFormat(compFmt);
                uint8_t* out = static_cast<uint8_t*>(data);
                for (size_t i = 0; i < numPixels; ++i) {
                    const GfVec3f& n = _framebuffer.normal[i];
                    for (size_t c = 0; c < 3; ++c) {
                        _WriteFloatComponent(out + c * compSize, compFmt,
                                             n[static_cast<int>(c)]);
                    }
                    out += dstBytesPerPixel;  // skip any extra (e.g. alpha) channel
                }
            } else {
                TF_WARN("Normal AOV format mismatch (fmt=%d, %zu src vs %zu dst "
                        "bytes) for '%s'; skipping",
                        static_cast<int>(fmt), srcBytes, dstCapacity,
                        binding.aovName.GetText());
            }
        }
        else if (binding.aovName == HdAovTokens->primId ||
                 binding.aovName == HdCarWashAovTokens->carwashObjectId) {
            const size_t srcBytes =
                _framebuffer.objectId.size() * sizeof(int32_t);
            if (fmt == HdFormatInt32 && srcBytes == dstCapacity) {
                std::copy(_framebuffer.objectId.begin(),
                          _framebuffer.objectId.end(),
                          static_cast<int32_t*>(data));
            } else {
                TF_WARN("Id AOV format mismatch (fmt=%d, %zu src vs %zu dst "
                        "bytes) for '%s'; skipping",
                        static_cast<int>(fmt), srcBytes, dstCapacity,
                        binding.aovName.GetText());
            }
        }
        else if (binding.aovName == HdCarWashAovTokens->carwashSemanticId) {
            const size_t srcBytes =
                _framebuffer.primId.size() * sizeof(int32_t);
            if (fmt == HdFormatInt32 && srcBytes == dstCapacity) {
                std::copy(_framebuffer.primId.begin(),
                          _framebuffer.primId.end(),
                          static_cast<int32_t*>(data));
            } else {
                TF_WARN("Semantic id AOV format mismatch (fmt=%d, %zu src vs "
                        "%zu dst bytes) for '%s'; skipping",
                        static_cast<int>(fmt), srcBytes, dstCapacity,
                        binding.aovName.GetText());
            }
        }

        buffer->Unmap();
        buffer->SetConverged(true);

        TF_DEBUG_MSG(HD_CARWASH, "Copied AOV '%s'\n", binding.aovName.GetText());
    }
}

void
HdCarWashRenderPass::_ExecutePhase0(
    HdRenderPassStateSharedPtr const& renderPassState)
{
    // Legacy Phase 0: solid color output
    // Kept for reference/fallback

    HdRenderPassAovBindingVector const& aovBindings =
        renderPassState->GetAovBindings();

    if (aovBindings.empty()) {
        TF_DEBUG_MSG(HD_CARWASH, "No AOV bindings, nothing to render\n");
        return;
    }

    for (const HdRenderPassAovBinding& binding : aovBindings) {
        if (!binding.renderBuffer) {
            continue;
        }

        HdCarWashRenderBuffer* buffer =
            static_cast<HdCarWashRenderBuffer*>(binding.renderBuffer);

        unsigned int width = buffer->GetWidth();
        unsigned int height = buffer->GetHeight();

        void* data = buffer->Map();
        if (!data) {
            continue;
        }

        if (binding.aovName == HdAovTokens->color) {
            GfVec4f carwashBlue(0.2f, 0.6f, 0.9f, 1.0f);
            GfVec4f* pixels = static_cast<GfVec4f*>(data);
            for (unsigned int i = 0; i < width * height; ++i) {
                pixels[i] = carwashBlue;
            }
        }

        buffer->Unmap();
        buffer->SetConverged(true);
    }
}

void
HdCarWashRenderPass::_ExecuteFullPipeline(
    HdRenderPassStateSharedPtr const& renderPassState,
    TfTokenVector const& renderTags)
{
    // TODO Phase 2+: Full render pipeline with AI backend
    TF_CODING_ERROR("Full pipeline not yet implemented");
}

PXR_NAMESPACE_CLOSE_SCOPE
