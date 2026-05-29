// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// rasterizer.h — CPU software rasterizer for AOV generation

#ifndef HD_CARWASH_RASTERIZER_H
#define HD_CARWASH_RASTERIZER_H

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/vt/array.h"

#include "api.h"

#include <vector>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

class HdCarWashMesh;
class HdCarWashCamera;

/// \struct HdCarWashFrameHash
///
/// Determinism verification hash for a rendered frame.
/// Uses FNV-1a over the raw IEEE-754 bits of each AOV value. The authoritative
/// path (ComputeAuthoritativeHash / ComputeHash(1)) covers every pixel; a
/// faster sampled preview (ComputeHash with sampleRate > 1) is also available.
///
struct HDCARWASH_API HdCarWashFrameHash
{
    uint64_t colorHash = 0;
    uint64_t depthHash = 0;
    uint64_t normalHash = 0;
    uint64_t idHash = 0;
    uint64_t combined = 0;      // XOR of all hashes
    uint32_t pixelsSampled = 0;
    uint32_t nonEmptyPixels = 0;

    /// Format as hex string for logging
    std::string ToString() const;

    /// Compare for equality (determinism check). Compare every sub-hash, not just
    /// the XOR-folded `combined` — XOR folding loses information (e.g. two AOVs
    /// swapping hashes folds identically), so comparing all fields is strictly
    /// stronger and avoids false "VERIFIED" matches.
    bool operator==(HdCarWashFrameHash const& other) const {
        return colorHash == other.colorHash
            && depthHash == other.depthHash
            && normalHash == other.normalHash
            && idHash == other.idHash
            && combined == other.combined;
    }
    bool operator!=(HdCarWashFrameHash const& other) const {
        return !(*this == other);
    }
};

/// \struct HdCarWashFramebuffer
///
/// Holds all AOV buffers for a single frame.
///
struct HDCARWASH_API HdCarWashFramebuffer
{
    unsigned int width = 0;
    unsigned int height = 0;

    // AOV buffers
    std::vector<GfVec4f> color;      // RGBA color
    std::vector<float> depth;         // Linear depth (camera space Z)
    std::vector<GfVec3f> normal;      // World-space normals
    std::vector<int32_t> objectId;    // Per-object ID
    std::vector<int32_t> primId;      // Per-primitive (face) ID
    std::vector<GfVec2f> motionVector;// Screen-space motion (future)

    void Resize(unsigned int w, unsigned int h);
    void Clear();

    /// Authoritative determinism hash: hashes EVERY pixel of every AOV using
    /// the raw IEEE-754 bits (no quantization, no subsampling). This is the
    /// only hash whose equality may be used to claim a frame is "VERIFIED"
    /// deterministic. Equivalent to ComputeHash(1).
    HdCarWashFrameHash ComputeAuthoritativeHash() const;

    /// Compute determinism hash. With @p sampleRate == 1 this is the
    /// authoritative full-buffer hash (every pixel, hashed once, in order).
    /// With @p sampleRate > 1 it is a FAST, NON-AUTHORITATIVE preview that
    /// samples only every Nth pixel (plus corners/center); differences in
    /// unsampled pixels are invisible, so it must not back a "VERIFIED" claim.
    /// Float AOVs are hashed by their raw IEEE-754 bits (NaN and -0.0
    /// canonicalized) so 5th-decimal accumulation-order drift is detected.
    /// @param sampleRate Sample every Nth pixel (1 = all/authoritative).
    HdCarWashFrameHash ComputeHash(unsigned int sampleRate = 1) const;
};

/// \class HdCarWashRasterizer
///
/// Simple CPU software rasterizer for generating semantic AOVs.
/// Uses a basic scanline algorithm with z-buffering.
///
/// This is intentionally simple — the goal is correct AOVs for AI,
/// not production-quality rendering speed.
///
class HDCARWASH_API HdCarWashRasterizer
{
public:
    HdCarWashRasterizer();
    ~HdCarWashRasterizer();

    /// Set the output framebuffer
    void SetFramebuffer(HdCarWashFramebuffer* framebuffer);

    /// Set view-projection matrix
    void SetViewProjectionMatrix(GfMatrix4d const& viewProj);

    /// Set view matrix (for normal transformation)
    void SetViewMatrix(GfMatrix4d const& view);

    /// Rasterize a mesh into the framebuffer
    void RasterizeMesh(HdCarWashMesh const* mesh);

    /// Clear all buffers
    void Clear(GfVec4f const& clearColor, float clearDepth);

private:
    // Triangle vertex data after transformation
    struct Vertex {
        GfVec4f clipPos;      // Clip space position
        GfVec3f worldPos;     // World space position
        GfVec3f worldNormal;  // World space normal
        GfVec2f uv;           // Texture coordinates
    };

    // Screen-space triangle for rasterization
    struct ScreenTriangle {
        GfVec3f screenPos[3]; // Screen XY + linear depth Z
        GfVec3f worldNormal[3];
        int objectId;
        int primId;
    };

    // Transform and clip a triangle, return false if fully clipped
    bool TransformTriangle(
        GfVec3f const& p0, GfVec3f const& p1, GfVec3f const& p2,
        GfVec3f const& n0, GfVec3f const& n1, GfVec3f const& n2,
        GfMatrix4d const& modelMatrix,
        int objectId, int primId,
        std::vector<ScreenTriangle>& outTriangles);

    // Rasterize a single screen-space triangle
    void RasterizeTriangle(ScreenTriangle const& tri);

    // Compute barycentric coordinates
    static GfVec3f Barycentric(
        GfVec2f const& p,
        GfVec2f const& a, GfVec2f const& b, GfVec2f const& c);

    // Edge function for triangle rasterization
    static float EdgeFunction(
        GfVec2f const& a, GfVec2f const& b, GfVec2f const& c);

    HdCarWashFramebuffer* _framebuffer;
    GfMatrix4d _viewProjMatrix;
    GfMatrix4d _viewMatrix;
    GfMatrix4d _normalMatrix;  // Inverse transpose of view for normals
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_CARWASH_RASTERIZER_H
