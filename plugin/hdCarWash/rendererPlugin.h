// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// rendererPlugin.h — USD/Hydra plugin entry point

#ifndef HD_CARWASH_RENDERER_PLUGIN_H
#define HD_CARWASH_RENDERER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rendererPlugin.h"

#include "api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdCarWashRendererPlugin
///
/// Plugin entry point for the CarWash render delegate.
/// This is discovered by USD/Hydra via plugInfo.json and creates
/// the actual HdCarWashRenderDelegate when selected in Solaris.
///
/// The plugin architecture allows CarWash to appear in the Solaris
/// renderer dropdown alongside Karma, Storm, etc.
///
class HDCARWASH_API HdCarWashRendererPlugin final : public HdRendererPlugin
{
public:
    HdCarWashRendererPlugin() = default;
    ~HdCarWashRendererPlugin() override = default;

    // Non-copyable
    HdCarWashRendererPlugin(const HdCarWashRendererPlugin&) = delete;
    HdCarWashRendererPlugin& operator=(const HdCarWashRendererPlugin&) = delete;

    /// Returns true if this plugin is supported on the current platform.
    /// CarWash requires CUDA-capable GPU for AI inference.
    HdRenderDelegate* CreateRenderDelegate() override;

    /// Creates the render delegate with custom settings.
    HdRenderDelegate* CreateRenderDelegate(
        HdRenderSettingsMap const& settingsMap) override;

    /// Destroys a render delegate created by this plugin.
    void DeleteRenderDelegate(HdRenderDelegate* renderDelegate) override;

    /// Returns true if the plugin is supported on the running system.
    bool IsSupported(bool gpuEnabled = true) const override;

private:
    /// Check if CUDA/ComfyUI backend is available
    bool _CheckBackendAvailability() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_CARWASH_RENDERER_PLUGIN_H
