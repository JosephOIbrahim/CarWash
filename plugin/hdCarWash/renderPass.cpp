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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/gf/vec4f.h"

#include <algorithm>  // std::sort for deterministic mesh ordering
#include <fstream>    // Debug file logging

PXR_NAMESPACE_OPEN_SCOPE

HdCarWashRenderPass::HdCarWashRenderPass(
    HdRenderIndex* index,
    HdRprimCollection const& collection,
    HdCarWashRenderDelegate* delegate)
    : HdRenderPass(index, collection)
    , _delegate(delegate)
    , _rasterizer(std::make_unique<HdCarWashRasterizer>())
    , _frameNumber(0)
    , _converged(false)
{
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashRenderPass created\n");
}

HdCarWashRenderPass::~HdCarWashRenderPass()
{
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashRenderPass destroyed\n");
}

bool
HdCarWashRenderPass::IsConverged() const
{
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
    _converged = true;  // Single-pass render for now
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

    // Clear buffers
    GfVec4f clearColor(0.1f, 0.1f, 0.15f, 1.0f);  // Dark blue-grey background
    _rasterizer->Clear(clearColor, 1.0f);

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

        TF_DEBUG_MSG(HD_CARWASH, "Camera set up, aspect ratio: %.2f\n", aspectRatio);
    } else {
        // Default camera looking at origin
        GfMatrix4d viewMatrix(1.0);
        viewMatrix.SetTranslate(GfVec3d(0, 0, -5));
        GfMatrix4d projMatrix(1.0);

        _rasterizer->SetViewMatrix(viewMatrix);
        _rasterizer->SetViewProjectionMatrix(viewMatrix * projMatrix);

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

    // Compute determinism hash
    HdCarWashFrameHash frameHash = _framebuffer.ComputeHash(16);  // Sample every 16th pixel
    debugLog << "FrameHash: 0x" << std::hex << frameHash.combined << std::dec
             << " (sampled=" << frameHash.pixelsSampled
             << ", nonEmpty=" << frameHash.nonEmptyPixels << ")" << std::endl;
    TF_DEBUG_MSG(HD_CARWASH, "Frame hash: 0x%016llx\n",
                 static_cast<unsigned long long>(frameHash.combined));

    // Copy framebuffer to AOV buffers
    _CopyFramebufferToAOVs(renderPassState);

    debugLog << "Phase 1 complete" << std::endl;
    debugLog.close();
    TF_DEBUG_MSG(HD_CARWASH, "Phase 1 render complete\n");
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

        // Copy appropriate data based on AOV type
        if (binding.aovName == HdAovTokens->color) {
            GfVec4f* pixels = static_cast<GfVec4f*>(data);
            std::copy(_framebuffer.color.begin(),
                      _framebuffer.color.end(),
                      pixels);
        }
        else if (binding.aovName == HdAovTokens->depth) {
            float* pixels = static_cast<float*>(data);
            std::copy(_framebuffer.depth.begin(),
                      _framebuffer.depth.end(),
                      pixels);
        }
        else if (binding.aovName == HdAovTokens->normal) {
            GfVec3f* pixels = static_cast<GfVec3f*>(data);
            std::copy(_framebuffer.normal.begin(),
                      _framebuffer.normal.end(),
                      pixels);
        }
        else if (binding.aovName == HdAovTokens->primId ||
                 binding.aovName == HdCarWashAovTokens->carwashObjectId) {
            int32_t* pixels = static_cast<int32_t*>(data);
            std::copy(_framebuffer.objectId.begin(),
                      _framebuffer.objectId.end(),
                      pixels);
        }
        else if (binding.aovName == HdCarWashAovTokens->carwashSemanticId) {
            int32_t* pixels = static_cast<int32_t*>(data);
            std::copy(_framebuffer.primId.begin(),
                      _framebuffer.primId.end(),
                      pixels);
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
