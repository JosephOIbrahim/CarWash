// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// renderPass.h — Frame rendering orchestration

#ifndef HD_CARWASH_RENDER_PASS_H
#define HD_CARWASH_RENDER_PASS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderPass.h"

#include "api.h"
#include "rasterizer.h"
#include "comfyClient.h"

#include <memory>
#include <future>
#include <atomic>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

class HdCarWashRenderDelegate;
class HdCarWashCamera;

/// \class HdCarWashRenderPass
///
/// Orchestrates the rendering of a single frame. This is where the
/// magic happens — the render pass:
///
/// 1. Reads cognitive substrate state
/// 2. Generates AOVs (depth, normal, edges, etc.)
/// 3. Assembles context payload
/// 4. Calls AI backend (LTX-2/Flux/Cosmos)
/// 5. Processes result and writes to render buffers
/// 6. Updates substrate with new state
///
/// Phase 0: Outputs solid color to verify integration
/// Phase 1: CPU rasterization for AOV generation
/// Phase 2+: Full pipeline with AI backend
///
class HDCARWASH_API HdCarWashRenderPass final : public HdRenderPass
{
public:
    HdCarWashRenderPass(
        HdRenderIndex* index,
        HdRprimCollection const& collection,
        HdCarWashRenderDelegate* delegate);

    ~HdCarWashRenderPass() override;

    // Non-copyable
    HdCarWashRenderPass(const HdCarWashRenderPass&) = delete;
    HdCarWashRenderPass& operator=(const HdCarWashRenderPass&) = delete;

protected:
    /// Called to check if the render pass needs updating.
    bool IsConverged() const override;

    /// Main render execution.
    void _Execute(
        HdRenderPassStateSharedPtr const& renderPassState,
        TfTokenVector const& renderTags) override;

private:
    /// Phase 0: Fill buffers with solid color (integration test)
    void _ExecutePhase0(HdRenderPassStateSharedPtr const& renderPassState);

    /// Phase 1: CPU rasterization for AOV generation
    void _ExecutePhase1(HdRenderPassStateSharedPtr const& renderPassState);

    /// Phase 2+: Full render pipeline with AI backend
    void _ExecuteFullPipeline(
        HdRenderPassStateSharedPtr const& renderPassState,
        TfTokenVector const& renderTags);

    /// Copy framebuffer data to AOV render buffers
    void _CopyFramebufferToAOVs(HdRenderPassStateSharedPtr const& renderPassState);

    /// Find the active camera
    HdCarWashCamera* _GetCamera(HdRenderPassStateSharedPtr const& renderPassState);

    /// Owner delegate
    HdCarWashRenderDelegate* _delegate;

    /// CPU rasterizer
    std::unique_ptr<HdCarWashRasterizer> _rasterizer;

    /// ComfyUI client for AI stylization
    std::unique_ptr<HdCarWashComfyClient> _comfyClient;

    /// AI stylization parameters
    HdCarWashStyleParams _styleParams;

    /// Enable AI stylization (false = CPU rasterization only)
    bool _enableAI;

    /// Synchronous render mode (wait for AI to complete before returning)
    /// Good for final renders, bad for viewport interactivity
    bool _syncRenderMode;

    /// Progressive refinement mode (keep refining until AI completes)
    bool _progressiveRefine;

    /// Internal framebuffer
    HdCarWashFramebuffer _framebuffer;

    /// Copy of framebuffer for async processing (value copy to avoid race conditions)
    HdCarWashFramebuffer _asyncFramebufferCopy;

    /// Pending async AI result
    std::future<HdCarWashRenderResult> _pendingAiResult;

    /// Flag indicating AI processing is in progress
    std::atomic<bool> _aiProcessing{false};

    /// Mutex for protecting result buffer swap
    std::mutex _resultMutex;

    /// Current frame number
    int _frameNumber;

    /// Is the current frame converged?
    bool _converged;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_CARWASH_RENDER_PASS_H
