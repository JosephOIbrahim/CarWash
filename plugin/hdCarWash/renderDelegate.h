// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// renderDelegate.h — Main render delegate implementation

#ifndef HD_CARWASH_RENDER_DELEGATE_H
#define HD_CARWASH_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/base/vt/dictionary.h"

#include "api.h"
#include "tokens.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// Forward declarations
class HdCarWashRenderPass;

/// \class HdCarWashRenderDelegate
///
/// The main render delegate for CarWash. This class is responsible for:
///
/// 1. Creating and managing render infrastructure (passes, buffers, prims)
/// 2. Coordinating the cognitive engine and AI backend
/// 3. Managing render settings and AOV configuration
///
/// Architecture Note (Cosmos-Ready):
/// The delegate uses a backend abstraction layer that can swap between
/// LTX-2, Flux, or future Cosmos WFM without changing the core interface.
/// The cognitive substrate remains constant regardless of backend.
///
class HDCARWASH_API HdCarWashRenderDelegate final : public HdRenderDelegate
{
public:
    /// Default constructor
    HdCarWashRenderDelegate();

    /// Constructor with initial settings
    explicit HdCarWashRenderDelegate(HdRenderSettingsMap const& settingsMap);

    /// Destructor
    ~HdCarWashRenderDelegate() override;

    // Non-copyable
    HdCarWashRenderDelegate(const HdCarWashRenderDelegate&) = delete;
    HdCarWashRenderDelegate& operator=(const HdCarWashRenderDelegate&) = delete;

    // =========================================================================
    // HdRenderDelegate Interface
    // =========================================================================

    /// Returns a list of AOV descriptors supported by this delegate.
    /// CarWash supports standard AOVs plus semantic buffers.
    HdAovDescriptor GetDefaultAovDescriptor(TfToken const& name) const override;

    /// Returns the list of supported render settings and their defaults.
    HdRenderSettingDescriptorList GetRenderSettingDescriptors() const override;

    /// Applies a render setting edit pushed from the scene (e.g. Solaris).
    /// Chains to the base — which owns the authoritative _settingsMap and bumps
    /// _settingsVersion for change detection — then mirrors the backend and
    /// determinism values we also cache as typed members. Without this override,
    /// edits landed only in the base map while the render pass read a frozen
    /// derived copy, so live parameter changes never reached the renderer. (#1)
    void SetRenderSetting(TfToken const& key, VtValue const& value) override;

    /// Returns a shared resource registry.
    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    /// Creates a render pass for the given collection.
    HdRenderPassSharedPtr CreateRenderPass(
        HdRenderIndex* index,
        HdRprimCollection const& collection) override;

    /// Creates an instancer (for instanced geometry).
    HdInstancer* CreateInstancer(
        HdSceneDelegate* delegate,
        SdfPath const& id) override;

    void DestroyInstancer(HdInstancer* instancer) override;

    /// Creates a render prim (mesh, curve, etc.).
    HdRprim* CreateRprim(
        TfToken const& typeId,
        SdfPath const& rprimId) override;

    void DestroyRprim(HdRprim* rprim) override;

    /// Creates a scene prim (light, camera, etc.).
    HdSprim* CreateSprim(
        TfToken const& typeId,
        SdfPath const& sprimId) override;

    HdSprim* CreateFallbackSprim(TfToken const& typeId) override;

    void DestroySprim(HdSprim* sprim) override;

    /// Creates a buffer prim (render buffer for AOVs).
    HdBprim* CreateBprim(
        TfToken const& typeId,
        SdfPath const& bprimId) override;

    HdBprim* CreateFallbackBprim(TfToken const& typeId) override;

    void DestroyBprim(HdBprim* bprim) override;

    /// Called before each render pass.
    void CommitResources(HdChangeTracker* tracker) override;

    /// Render statistics surfaced to Solaris/Husk: generation progress
    /// (percentDone/fractionDone) and the last AI error, both pushed by the
    /// render pass. (#6)
    VtDictionary GetRenderStats() const override;

    /// Returns supported prim types.
    TfTokenVector const& GetSupportedRprimTypes() const override;
    TfTokenVector const& GetSupportedSprimTypes() const override;
    TfTokenVector const& GetSupportedBprimTypes() const override;

    // =========================================================================
    // CarWash-Specific Interface
    // =========================================================================

    /// Returns the current AI backend type (LTX-2, Flux, Cosmos)
    TfToken GetActiveBackend() const;

    /// Check if a capability is available from the current backend
    bool HasCapability(TfToken const& capability) const;

    /// Get the current determinism mode
    TfToken GetDeterminismMode() const;

    /// Set the current determinism mode
    void SetDeterminismMode(TfToken const& mode);

    /// Get the current render settings map (for render pass)
    HdRenderSettingsMap const& GetRenderSettingsMap() const;

    /// Push generation progress (0..1) for GetRenderStats. Called by the render
    /// pass each frame. (#6)
    void SetProgress(float fraction);

    /// Push the last AI error string for GetRenderStats (empty clears it).
    /// Called by the render pass. (#5/#6)
    void SetLastError(const std::string& message);

private:
    /// Initialize the delegate with default settings
    void _Initialize();

    /// Apply settings from a settings map
    void _ApplySettings(HdRenderSettingsMap const& settingsMap);

    // =========================================================================
    // Member Data
    // =========================================================================

    /// Resource registry (shared by all render passes)
    HdResourceRegistrySharedPtr _resourceRegistry;

    // Render settings live in the base HdRenderDelegate::_settingsMap (a
    // protected member), mutated through SetRenderSetting() and read back via
    // GetRenderSettingsMap(). We deliberately do NOT declare our own
    // _settingsMap here: doing so shadowed the base member, so base-class
    // setting edits never reached the copy the render pass read. (#1)

    /// Active backend type
    TfToken _activeBackend;

    /// Determinism mode
    TfToken _determinismMode;

    /// Mutex for thread safety
    mutable std::mutex _mutex;

    /// Generation progress (0..1) and last AI error, surfaced via
    /// GetRenderStats. _progressFraction is atomic (read on the render thread,
    /// written from the pass); _lastError is guarded by _mutex. (#5/#6)
    std::atomic<float> _progressFraction{0.0f};
    std::string _lastError;

    // TODO Phase 4: Add cognitive engine member
    // std::unique_ptr<CognitiveEngine> _cognitiveEngine;

    // TODO Phase 2: Add ComfyUI client member
    // std::unique_ptr<ComfyUIClient> _comfyClient;

    /// Supported prim type lists (cached for performance)
    static const TfTokenVector _supportedRprimTypes;
    static const TfTokenVector _supportedSprimTypes;
    static const TfTokenVector _supportedBprimTypes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_CARWASH_RENDER_DELEGATE_H
