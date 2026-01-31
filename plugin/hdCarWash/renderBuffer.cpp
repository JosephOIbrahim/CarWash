// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// renderBuffer.cpp — AOV buffer storage implementation

#include "renderBuffer.h"
#include "debugCodes.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/gf/vec3i.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Calculate bytes per pixel for a given format
size_t _GetBytesPerPixel(HdFormat format)
{
    switch (format) {
        case HdFormatUNorm8:     return 1;
        case HdFormatUNorm8Vec2: return 2;
        case HdFormatUNorm8Vec3: return 3;
        case HdFormatUNorm8Vec4: return 4;
        case HdFormatSNorm8:     return 1;
        case HdFormatSNorm8Vec2: return 2;
        case HdFormatSNorm8Vec3: return 3;
        case HdFormatSNorm8Vec4: return 4;
        case HdFormatFloat16:     return 2;
        case HdFormatFloat16Vec2: return 4;
        case HdFormatFloat16Vec3: return 6;
        case HdFormatFloat16Vec4: return 8;
        case HdFormatFloat32:     return 4;
        case HdFormatFloat32Vec2: return 8;
        case HdFormatFloat32Vec3: return 12;
        case HdFormatFloat32Vec4: return 16;
        case HdFormatInt32:       return 4;
        case HdFormatInt32Vec2:   return 8;
        case HdFormatInt32Vec3:   return 12;
        case HdFormatInt32Vec4:   return 16;
        default:
            TF_CODING_ERROR("Unknown format %d", format);
            return 0;
    }
}

} // anonymous namespace

HdCarWashRenderBuffer::HdCarWashRenderBuffer(SdfPath const& id)
    : HdRenderBuffer(id)
    , _width(0)
    , _height(0)
    , _depth(1)
    , _format(HdFormatInvalid)
    , _multiSampled(false)
    , _mapped(false)
    , _converged(false)
{
}

HdCarWashRenderBuffer::~HdCarWashRenderBuffer()
{
    _Deallocate();
}

bool
HdCarWashRenderBuffer::Allocate(
    GfVec3i const& dimensions,
    HdFormat format,
    bool multiSampled)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _Deallocate();

    if (dimensions[0] <= 0 || dimensions[1] <= 0 || dimensions[2] <= 0) {
        TF_CODING_ERROR("Invalid buffer dimensions: %d x %d x %d",
                        dimensions[0], dimensions[1], dimensions[2]);
        return false;
    }

    size_t bytesPerPixel = _GetBytesPerPixel(format);
    if (bytesPerPixel == 0) {
        return false;
    }

    _width = dimensions[0];
    _height = dimensions[1];
    _depth = dimensions[2];
    _format = format;
    _multiSampled = multiSampled;

    size_t bufferSize = _width * _height * _depth * bytesPerPixel;
    _buffer.resize(bufferSize, 0);

    TF_DEBUG_MSG(HD_CARWASH, "Allocated render buffer %s: %dx%dx%d, %zu bytes\n",
                 GetId().GetText(), _width, _height, _depth, bufferSize);

    return true;
}

void*
HdCarWashRenderBuffer::Map()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_buffer.empty()) {
        TF_CODING_ERROR("Cannot map unallocated buffer");
        return nullptr;
    }

    _mapped = true;
    return _buffer.data();
}

void
HdCarWashRenderBuffer::Unmap()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _mapped = false;
}

bool
HdCarWashRenderBuffer::IsMapped() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _mapped;
}

void
HdCarWashRenderBuffer::SetConverged(bool converged)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _converged = converged;
}

bool
HdCarWashRenderBuffer::IsConverged() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _converged;
}

void
HdCarWashRenderBuffer::Resolve()
{
    // No-op for CPU buffers (no multi-sampling to resolve)
}

void
HdCarWashRenderBuffer::_Deallocate()
{
    // Note: _mutex should already be held by caller
    _width = 0;
    _height = 0;
    _depth = 1;
    _format = HdFormatInvalid;
    _buffer.clear();
    _buffer.shrink_to_fit();
    _mapped = false;
    _converged = false;
}

PXR_NAMESPACE_CLOSE_SCOPE
