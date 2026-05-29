// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// renderDelegate.cpp — Main render delegate implementation

#include "renderDelegate.h"
#include "renderPass.h"
#include "renderBuffer.h"
#include "mesh.h"
#include "camera.h"
#include "light.h"
#include "debugCodes.h"

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/getenv.h"

#include <fstream>  // Debug file logging

PXR_NAMESPACE_OPEN_SCOPE

// ==============================================================================
// Static Data
// ==============================================================================

// Supported Rprim types (geometry)
const TfTokenVector HdCarWashRenderDelegate::_supportedRprimTypes = {
    HdPrimTypeTokens->mesh,
    // TODO Phase 1: Add curves, points, volume
};

// Supported Sprim types (scene objects)
const TfTokenVector HdCarWashRenderDelegate::_supportedSprimTypes = {
    HdPrimTypeTokens->camera,
    HdPrimTypeTokens->extComputation,
    // Light types
    HdPrimTypeTokens->simpleLight,
    HdPrimTypeTokens->sphereLight,
    HdPrimTypeTokens->rectLight,
    HdPrimTypeTokens->distantLight,
    HdPrimTypeTokens->domeLight,
    HdPrimTypeTokens->diskLight,
    HdPrimTypeTokens->cylinderLight,
    // TODO: Add materials
};

// Supported Bprim types (buffers)
const TfTokenVector HdCarWashRenderDelegate::_supportedBprimTypes = {
    HdPrimTypeTokens->renderBuffer,
};

// ==============================================================================
// Construction / Destruction
// ==============================================================================

HdCarWashRenderDelegate::HdCarWashRenderDelegate()
    : HdRenderDelegate()
    , _resourceRegistry(std::make_shared<HdResourceRegistry>())
    , _activeBackend(HdCarWashSettingsTokens->backendLTX2)  // Default: LTX-2
    , _determinismMode(HdCarWashSettingsTokens->deterministicBalanced)
{
    _Initialize();
}

HdCarWashRenderDelegate::HdCarWashRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
    : HdRenderDelegate(settingsMap)
    , _resourceRegistry(std::make_shared<HdResourceRegistry>())
    , _activeBackend(HdCarWashSettingsTokens->backendLTX2)
    , _determinismMode(HdCarWashSettingsTokens->deterministicBalanced)
{
    _Initialize();
    _ApplySettings(settingsMap);
}

HdCarWashRenderDelegate::~HdCarWashRenderDelegate()
{
    // Cleanup is handled by smart pointers
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashRenderDelegate destroyed\n");
}

// ==============================================================================
// Initialization
// ==============================================================================

void
HdCarWashRenderDelegate::_Initialize()
{
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashRenderDelegate initializing...\n");

    // DEBUG: Log initialization to file
    std::ofstream debugLog("C:/Temp/hdcarwash_debug.txt", std::ios::app);
    debugLog << "=== HdCarWash Delegate Initializing ===" << std::endl;
    debugLog.close();

    // Check environment for backend override
    std::string backendEnv = TfGetenv("CARWASH_BACKEND", "");
    if (!backendEnv.empty()) {
        if (backendEnv == "ltx2") {
            _activeBackend = HdCarWashSettingsTokens->backendLTX2;
        } else if (backendEnv == "flux") {
            _activeBackend = HdCarWashSettingsTokens->backendFlux;
        } else if (backendEnv == "cosmos") {
            _activeBackend = HdCarWashSettingsTokens->backendCosmos;
        }
        TF_DEBUG_MSG(HD_CARWASH, "Backend override from env: %s\n",
                     _activeBackend.GetText());
    }

    // TODO Phase 2: Initialize ComfyUI client
    // TODO Phase 4: Initialize cognitive engine

    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashRenderDelegate initialized. "
                 "Backend: %s, Determinism: %s\n",
                 _activeBackend.GetText(),
                 _determinismMode.GetText());
}

void
HdCarWashRenderDelegate::_ApplySettings(
    HdRenderSettingsMap const& settingsMap)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _settingsMap = settingsMap;

    // Apply backend setting
    auto backendIt = settingsMap.find(HdCarWashSettingsTokens->backend);
    if (backendIt != settingsMap.end()) {
        _activeBackend = backendIt->second.Get<TfToken>();
    }

    // Apply determinism setting
    auto detIt = settingsMap.find(HdCarWashSettingsTokens->deterministicMode);
    if (detIt != settingsMap.end()) {
        _determinismMode = detIt->second.Get<TfToken>();
    }
}

// ==============================================================================
// AOV Support
// ==============================================================================

HdAovDescriptor
HdCarWashRenderDelegate::GetDefaultAovDescriptor(TfToken const& name) const
{
    // Standard AOVs
    if (name == HdAovTokens->color) {
        return HdAovDescriptor(HdFormatFloat32Vec4, false,
                               VtValue(GfVec4f(0.0f)));
    }
    if (name == HdAovTokens->depth) {
        return HdAovDescriptor(HdFormatFloat32, false,
                               VtValue(1.0f));
    }
    if (name == HdAovTokens->normal) {
        return HdAovDescriptor(HdFormatFloat32Vec3, false,
                               VtValue(GfVec3f(0.0f)));
    }
    if (name == HdAovTokens->primId ||
        name == HdAovTokens->instanceId ||
        name == HdAovTokens->elementId) {
        return HdAovDescriptor(HdFormatInt32, false,
                               VtValue(-1));
    }

    // CarWash semantic AOVs
    if (name == HdCarWashAovTokens->carwashObjectId) {
        return HdAovDescriptor(HdFormatInt32, false,
                               VtValue(-1));
    }
    if (name == HdCarWashAovTokens->carwashSemanticId) {
        return HdAovDescriptor(HdFormatInt32, false,
                               VtValue(-1));
    }
    if (name == HdCarWashAovTokens->carwashMotionVector) {
        return HdAovDescriptor(HdFormatFloat32Vec2, false,
                               VtValue(GfVec2f(0.0f)));
    }
    if (name == HdCarWashAovTokens->carwashEdges) {
        return HdAovDescriptor(HdFormatFloat32, false,
                               VtValue(0.0f));
    }
    if (name == HdCarWashAovTokens->carwashStabilityMask) {
        return HdAovDescriptor(HdFormatFloat32, false,
                               VtValue(1.0f));
    }

    return HdAovDescriptor();
}

// ==============================================================================
// Render Settings
// ==============================================================================

HdRenderSettingDescriptorList
HdCarWashRenderDelegate::GetRenderSettingDescriptors() const
{
    HdRenderSettingDescriptorList settings;

    // Backend selection (Cosmos-ready)
    settings.push_back({
        "AI Backend",
        HdCarWashSettingsTokens->backend,
        VtValue(HdCarWashSettingsTokens->backendLTX2)
    });

    // Determinism mode
    settings.push_back({
        "Determinism Mode",
        HdCarWashSettingsTokens->deterministicMode,
        VtValue(HdCarWashSettingsTokens->deterministicBalanced)
    });

    // ComfyUI server
    settings.push_back({
        "ComfyUI Server URL",
        HdCarWashSettingsTokens->comfyuiServerUrl,
        VtValue(std::string("http://localhost:8188"))
    });

    // Inference settings
    settings.push_back({
        "Inference Steps",
        HdCarWashSettingsTokens->inferenceSteps,
        VtValue(20)
    });

    settings.push_back({
        "Guidance Scale",
        HdCarWashSettingsTokens->guidanceScale,
        VtValue(7.5f)
    });

    // Cognitive substrate
    settings.push_back({
        "Enable Substrate",
        HdCarWashSettingsTokens->substrateEnabled,
        VtValue(true)
    });

    settings.push_back({
        "History Frames",
        HdCarWashSettingsTokens->substrateHistoryFrames,
        VtValue(10)
    });

    // Seed for determinism
    settings.push_back({
        "Random Seed",
        HdCarWashSettingsTokens->seed,
        VtValue(42)
    });

    // Prompt settings
    settings.push_back({
        "Prompt",
        HdCarWashSettingsTokens->prompt,
        VtValue(std::string("photorealistic 3D render, cinematic lighting, sharp details"))
    });

    settings.push_back({
        "Negative Prompt",
        HdCarWashSettingsTokens->negativePrompt,
        VtValue(std::string("blurry, low quality, distorted"))
    });

    // ControlNet settings
    settings.push_back({
        "Depth ControlNet Strength",
        HdCarWashSettingsTokens->depthControlNetStrength,
        VtValue(0.8f)
    });

    settings.push_back({
        "Normal ControlNet Strength",
        HdCarWashSettingsTokens->normalControlNetStrength,
        VtValue(0.6f)
    });

    settings.push_back({
        "Enable Depth ControlNet",
        HdCarWashSettingsTokens->enableDepthControl,
        VtValue(true)
    });

    settings.push_back({
        "Enable Normal ControlNet",
        HdCarWashSettingsTokens->enableNormalControl,
        VtValue(true)
    });

    return settings;
}

// ==============================================================================
// Resource Registry
// ==============================================================================

HdResourceRegistrySharedPtr
HdCarWashRenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

// ==============================================================================
// Prim Factory Methods
// ==============================================================================

HdRenderPassSharedPtr
HdCarWashRenderDelegate::CreateRenderPass(
    HdRenderIndex* index,
    HdRprimCollection const& collection)
{
    return std::make_shared<HdCarWashRenderPass>(index, collection, this);
}

HdInstancer*
HdCarWashRenderDelegate::CreateInstancer(
    HdSceneDelegate* delegate,
    SdfPath const& id)
{
    // TODO Phase 1: Implement HdCarWashInstancer
    TF_CODING_ERROR("CreateInstancer not yet implemented");
    return nullptr;
}

void
HdCarWashRenderDelegate::DestroyInstancer(HdInstancer* instancer)
{
    delete instancer;
}

HdRprim*
HdCarWashRenderDelegate::CreateRprim(
    TfToken const& typeId,
    SdfPath const& rprimId)
{
    // DEBUG: Log all CreateRprim calls to file
    std::ofstream debugLog("C:/Temp/hdcarwash_debug.txt", std::ios::app);
    debugLog << "[CreateRprim] type: " << typeId.GetText()
             << " path: " << rprimId.GetText() << std::endl;
    debugLog.close();

    if (typeId == HdPrimTypeTokens->mesh) {
        TF_DEBUG_MSG(HD_CARWASH, "CreateRprim: mesh %s\n", rprimId.GetText());
        return new HdCarWashMesh(rprimId);
    }

    TF_CODING_ERROR("Unknown rprim type: %s", typeId.GetText());
    return nullptr;
}

void
HdCarWashRenderDelegate::DestroyRprim(HdRprim* rprim)
{
    delete rprim;
}

HdSprim*
HdCarWashRenderDelegate::CreateSprim(
    TfToken const& typeId,
    SdfPath const& sprimId)
{
    if (typeId == HdPrimTypeTokens->camera) {
        TF_DEBUG_MSG(HD_CARWASH, "CreateSprim: camera %s\n", sprimId.GetText());
        return new HdCarWashCamera(sprimId);
    }
    if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(sprimId);
    }

    // Light types
    if (typeId == HdPrimTypeTokens->simpleLight ||
        typeId == HdPrimTypeTokens->sphereLight ||
        typeId == HdPrimTypeTokens->rectLight ||
        typeId == HdPrimTypeTokens->distantLight ||
        typeId == HdPrimTypeTokens->domeLight ||
        typeId == HdPrimTypeTokens->diskLight ||
        typeId == HdPrimTypeTokens->cylinderLight) {
        TF_DEBUG_MSG(HD_CARWASH, "CreateSprim: light %s (type: %s)\n",
                     sprimId.GetText(), typeId.GetText());
        return new HdCarWashLight(sprimId, typeId);
    }

    TF_CODING_ERROR("Unknown sprim type: %s", typeId.GetText());
    return nullptr;
}

HdSprim*
HdCarWashRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdCarWashCamera(SdfPath::EmptyPath());
    }
    if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(SdfPath::EmptyPath());
    }

    // Fallback lights
    if (typeId == HdPrimTypeTokens->simpleLight ||
        typeId == HdPrimTypeTokens->sphereLight ||
        typeId == HdPrimTypeTokens->rectLight ||
        typeId == HdPrimTypeTokens->distantLight ||
        typeId == HdPrimTypeTokens->domeLight ||
        typeId == HdPrimTypeTokens->diskLight ||
        typeId == HdPrimTypeTokens->cylinderLight) {
        return new HdCarWashLight(SdfPath::EmptyPath(), typeId);
    }

    TF_CODING_ERROR("Unknown fallback sprim type: %s", typeId.GetText());
    return nullptr;
}

void
HdCarWashRenderDelegate::DestroySprim(HdSprim* sprim)
{
    delete sprim;
}

HdBprim*
HdCarWashRenderDelegate::CreateBprim(
    TfToken const& typeId,
    SdfPath const& bprimId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdCarWashRenderBuffer(bprimId);
    }

    TF_CODING_ERROR("Unknown bprim type: %s", typeId.GetText());
    return nullptr;
}

HdBprim*
HdCarWashRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdCarWashRenderBuffer(SdfPath::EmptyPath());
    }

    TF_CODING_ERROR("Unknown fallback bprim type: %s", typeId.GetText());
    return nullptr;
}

void
HdCarWashRenderDelegate::DestroyBprim(HdBprim* bprim)
{
    delete bprim;
}

// ==============================================================================
// Commit Resources
// ==============================================================================

void
HdCarWashRenderDelegate::CommitResources(HdChangeTracker* tracker)
{
    // Called before each render pass
    // TODO Phase 4: Sync cognitive engine state
}

// ==============================================================================
// Supported Types
// ==============================================================================

TfTokenVector const&
HdCarWashRenderDelegate::GetSupportedRprimTypes() const
{
    // DEBUG: Log when Hydra queries supported rprim types
    static bool logged = false;
    if (!logged) {
        std::ofstream debugLog("C:/Temp/hdcarwash_debug.txt", std::ios::app);
        debugLog << "[GetSupportedRprimTypes] Returning: ";
        for (const auto& t : _supportedRprimTypes) {
            debugLog << t.GetText() << " ";
        }
        debugLog << std::endl;
        debugLog.close();
        logged = true;
    }
    return _supportedRprimTypes;
}

TfTokenVector const&
HdCarWashRenderDelegate::GetSupportedSprimTypes() const
{
    return _supportedSprimTypes;
}

TfTokenVector const&
HdCarWashRenderDelegate::GetSupportedBprimTypes() const
{
    return _supportedBprimTypes;
}

// ==============================================================================
// CarWash-Specific Methods
// ==============================================================================

HdRenderSettingsMap const&
HdCarWashRenderDelegate::GetRenderSettingsMap() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _settingsMap;
}

TfToken
HdCarWashRenderDelegate::GetActiveBackend() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _activeBackend;
}

bool
HdCarWashRenderDelegate::HasCapability(TfToken const& capability) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    // LTX-2 capabilities
    if (_activeBackend == HdCarWashSettingsTokens->backendLTX2) {
        if (capability == HdCarWashBackendTokens->capTemporalCoherence) return true;
        if (capability == HdCarWashBackendTokens->capAudioSync) return true;
        if (capability == HdCarWashBackendTokens->capStyleTransfer) return true;
        if (capability == HdCarWashBackendTokens->capBatchInvariance) return true;
    }

    // Flux capabilities
    if (_activeBackend == HdCarWashSettingsTokens->backendFlux) {
        if (capability == HdCarWashBackendTokens->capStyleTransfer) return true;
        // Flux doesn't have native temporal coherence
    }

    // Cosmos capabilities (future)
    if (_activeBackend == HdCarWashSettingsTokens->backendCosmos) {
        if (capability == HdCarWashBackendTokens->capTemporalCoherence) return true;
        if (capability == HdCarWashBackendTokens->capObjectPermanence) return true;
        if (capability == HdCarWashBackendTokens->capStyleTransfer) return true;
        if (capability == HdCarWashBackendTokens->capBatchInvariance) return true;
    }

    return false;
}

TfToken
HdCarWashRenderDelegate::GetDeterminismMode() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _determinismMode;
}

void
HdCarWashRenderDelegate::SetDeterminismMode(TfToken const& mode)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _determinismMode = mode;
    TF_DEBUG_MSG(HD_CARWASH, "Determinism mode set to: %s\n", mode.GetText());
}

PXR_NAMESPACE_CLOSE_SCOPE
