// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// renderBuffer.h — AOV buffer storage

#ifndef HD_CARWASH_RENDER_BUFFER_H
#define HD_CARWASH_RENDER_BUFFER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderBuffer.h"

#include "api.h"

#include <vector>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdCarWashRenderBuffer
///
/// CPU-side render buffer for AOV storage.
/// Stores color, depth, normal, and CarWash semantic AOVs.
///
/// Thread-safe for concurrent read/write access.
///
class HDCARWASH_API HdCarWashRenderBuffer final : public HdRenderBuffer
{
public:
    HdCarWashRenderBuffer(SdfPath const& id);
    ~HdCarWashRenderBuffer() override;

    // Non-copyable
    HdCarWashRenderBuffer(const HdCarWashRenderBuffer&) = delete;
    HdCarWashRenderBuffer& operator=(const HdCarWashRenderBuffer&) = delete;

    /// Allocate buffer storage.
    bool Allocate(
        GfVec3i const& dimensions,
        HdFormat format,
        bool multiSampled) override;

    /// Get buffer dimensions.
    unsigned int GetWidth() const override { return _width; }
    unsigned int GetHeight() const override { return _height; }
    unsigned int GetDepth() const override { return _depth; }

    /// Get buffer format.
    HdFormat GetFormat() const override { return _format; }

    /// Is this a multi-sampled buffer?
    bool IsMultiSampled() const override { return _multiSampled; }

    /// Map buffer for CPU access.
    void* Map() override;

    /// Unmap buffer.
    void Unmap() override;

    /// Is the buffer currently mapped?
    bool IsMapped() const override;

    /// Mark as converged (rendering complete).
    void SetConverged(bool converged);

    /// Is rendering converged?
    bool IsConverged() const override;

    /// Resolve multi-sampled buffer (no-op for CPU buffers).
    void Resolve() override;

protected:
    /// Deallocate buffer storage.
    void _Deallocate() override;

private:
    unsigned int _width;
    unsigned int _height;
    unsigned int _depth;
    HdFormat _format;
    bool _multiSampled;

    std::vector<uint8_t> _buffer;
    bool _mapped;
    bool _converged;

    mutable std::mutex _mutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_CARWASH_RENDER_BUFFER_H
