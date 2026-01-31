// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// rendererPlugin.cpp — USD/Hydra plugin entry point implementation

#include "rendererPlugin.h"
#include "renderDelegate.h"

#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the plugin with USD/Hydra
// This macro creates the factory function that USD calls to discover plugins
TF_REGISTRY_FUNCTION(TfType)
{
    HdRendererPluginRegistry::Define<HdCarWashRendererPlugin>();
}

HdRenderDelegate*
HdCarWashRendererPlugin::CreateRenderDelegate()
{
    return new HdCarWashRenderDelegate();
}

HdRenderDelegate*
HdCarWashRendererPlugin::CreateRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
{
    return new HdCarWashRenderDelegate(settingsMap);
}

void
HdCarWashRendererPlugin::DeleteRenderDelegate(HdRenderDelegate* renderDelegate)
{
    delete renderDelegate;
}

bool
HdCarWashRendererPlugin::IsSupported(bool gpuEnabled) const
{
    // CarWash can run in CPU-only mode for AOV generation,
    // but AI inference requires GPU
    if (!gpuEnabled) {
        TF_WARN("HdCarWash: GPU disabled, AI features will be unavailable");
    }

    // For Phase 0, always return true to test basic integration
    // Later phases will check for CUDA, ComfyUI availability, etc.
    return true;
}

bool
HdCarWashRendererPlugin::_CheckBackendAvailability() const
{
    // TODO Phase 2: Check ComfyUI server availability
    // TODO Phase 4: Check LTX-2/Cosmos backend availability

    // For Phase 0, assume backend is available
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
