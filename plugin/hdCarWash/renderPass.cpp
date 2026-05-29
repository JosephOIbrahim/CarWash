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
#include <cstdint>    // uint8_t, int32_t
#include <cmath>      // std::lround
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

    // Compute determinism hash. Use the authoritative full-buffer hash: this is
    // the value the "VERIFIED" determinism claim rests on, so it must cover every
    // pixel (the sampled ComputeHash(N>1) preview can miss 5th-decimal drift).
    HdCarWashFrameHash frameHash = _framebuffer.ComputeAuthoritativeHash();
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

// Clamp + write a single float component into a destination of the given
// component format. Only the float32/float16/unorm8 component families are
// produced by the conversions here.
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

// Convert/copy the float32 color framebuffer into the destination format,
// honoring its component format and component count. Returns false (and
// writes nothing) if the destination component family is not one we can
// produce, so the caller can warn-and-skip instead of corrupting memory.
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
            // Color may be UNorm8Vec4 / Float16Vec4 / Float32Vec4 etc.
            // Convert per-component into whatever the client allocated.
            if (!_WriteColorAov(data, fmt, _framebuffer.color, numPixels)) {
                TF_WARN("Unsupported color AOV format (%d) for '%s'; skipping",
                        static_cast<int>(fmt), binding.aovName.GetText());
            }
        }
        else if (binding.aovName == HdAovTokens->depth) {
            // Source is float32. Only copy if the destination is a single
            // float32 component of matching capacity.
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
            // Source is GfVec3f (float32 x3). Convert per-component for narrow
            // formats; require >=3 components.
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
            // Source is int32_t. Require a single-component int32 destination
            // of matching capacity.
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
