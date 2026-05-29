// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// rasterizer.cpp — CPU software rasterizer implementation

#include "rasterizer.h"
#include "mesh.h"
#include "debugCodes.h"

#include <algorithm>
#include <cmath>
#include <cstring>   // std::memcpy for raw-bits float hashing
#include <sstream>
#include <iomanip>

PXR_NAMESPACE_OPEN_SCOPE

// ============================================================================
// Framebuffer
// ============================================================================

void
HdCarWashFramebuffer::Resize(unsigned int w, unsigned int h)
{
    if (width == w && height == h) {
        return;
    }

    width = w;
    height = h;
    size_t numPixels = static_cast<size_t>(w) * h;

    color.resize(numPixels);
    depth.resize(numPixels);
    normal.resize(numPixels);
    objectId.resize(numPixels);
    primId.resize(numPixels);
    motionVector.resize(numPixels);
}

void
HdCarWashFramebuffer::Clear()
{
    std::fill(color.begin(), color.end(), GfVec4f(0.0f));
    std::fill(depth.begin(), depth.end(), 1.0f);
    std::fill(normal.begin(), normal.end(), GfVec3f(0.0f));
    std::fill(objectId.begin(), objectId.end(), -1);
    std::fill(primId.begin(), primId.end(), -1);
    std::fill(motionVector.begin(), motionVector.end(), GfVec2f(0.0f));
}

// ============================================================================
// Frame Hash (Determinism Verification)
// ============================================================================

namespace {
    // FNV-1a hash constants (64-bit)
    constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;

    inline uint64_t fnv1a_hash(uint64_t hash, const void* data, size_t size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < size; ++i) {
            hash ^= bytes[i];
            hash *= FNV_PRIME;
        }
        return hash;
    }

    inline uint64_t fnv1a_float(uint64_t hash, float value) {
        // Hash the RAW IEEE-754 bits — NOT a quantized value. Quantizing
        // (value*10000) before hashing collapses exactly the 5th-decimal
        // accumulation/reassociation drift this determinism check exists to
        // detect, making nondeterministic frames hash identical. Canonicalize
        // NaN and -0.0 so only meaningful differences register.
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        if ((bits & 0x7F800000u) == 0x7F800000u && (bits & 0x007FFFFFu) != 0u) {
            bits = 0x7FC00000u;  // canonical quiet NaN
        } else if (bits == 0x80000000u) {
            bits = 0x00000000u;  // -0.0 -> +0.0
        }
        return fnv1a_hash(hash, &bits, sizeof(bits));
    }
}

std::string
HdCarWashFrameHash::ToString() const
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    oss << "FrameHash{combined=0x" << std::setw(16) << combined
        << ", color=0x" << std::setw(16) << colorHash
        << ", depth=0x" << std::setw(16) << depthHash
        << ", normal=0x" << std::setw(16) << normalHash
        << ", id=0x" << std::setw(16) << idHash
        << std::dec << ", sampled=" << pixelsSampled
        << ", nonEmpty=" << nonEmptyPixels << "}";
    return oss.str();
}

HdCarWashFrameHash
HdCarWashFramebuffer::ComputeHash(unsigned int sampleRate) const
{
    HdCarWashFrameHash result;

    if (width == 0 || height == 0 || color.empty()) {
        return result;
    }

    uint64_t colorH = FNV_OFFSET_BASIS;
    uint64_t depthH = FNV_OFFSET_BASIS;
    uint64_t normalH = FNV_OFFSET_BASIS;
    uint64_t idH = FNV_OFFSET_BASIS;

    size_t numPixels = static_cast<size_t>(width) * height;
    uint32_t sampled = 0;
    uint32_t nonEmpty = 0;

    // Strategic sampling: sample every Nth pixel in a grid pattern
    // Also always sample corners and center for edge case coverage
    auto samplePixel = [&](size_t idx) {
        if (idx >= numPixels) return;

        sampled++;

        // Color (RGBA)
        const GfVec4f& c = color[idx];
        colorH = fnv1a_float(colorH, c[0]);
        colorH = fnv1a_float(colorH, c[1]);
        colorH = fnv1a_float(colorH, c[2]);
        colorH = fnv1a_float(colorH, c[3]);

        // Depth
        depthH = fnv1a_float(depthH, depth[idx]);

        // Normal
        const GfVec3f& n = normal[idx];
        normalH = fnv1a_float(normalH, n[0]);
        normalH = fnv1a_float(normalH, n[1]);
        normalH = fnv1a_float(normalH, n[2]);

        // Object and prim IDs
        idH = fnv1a_hash(idH, &objectId[idx], sizeof(int32_t));
        idH = fnv1a_hash(idH, &primId[idx], sizeof(int32_t));

        // Track non-empty pixels (has geometry)
        if (objectId[idx] >= 0) {
            nonEmpty++;
        }
    };

    // Sample corners
    samplePixel(0);                                          // Top-left
    samplePixel(width - 1);                                  // Top-right
    samplePixel(numPixels - width);                          // Bottom-left
    samplePixel(numPixels - 1);                              // Bottom-right
    samplePixel((height / 2) * width + (width / 2));         // Center

    // Grid sampling
    for (size_t y = 0; y < height; y += sampleRate) {
        for (size_t x = 0; x < width; x += sampleRate) {
            samplePixel(y * width + x);
        }
    }

    result.colorHash = colorH;
    result.depthHash = depthH;
    result.normalHash = normalH;
    result.idHash = idH;
    result.combined = colorH ^ depthH ^ normalH ^ idH;
    result.pixelsSampled = sampled;
    result.nonEmptyPixels = nonEmpty;

    return result;
}

HdCarWashFrameHash
HdCarWashFramebuffer::ComputeAuthoritativeHash() const
{
    // The authoritative determinism hash: every pixel of every AOV, hashed
    // exactly once in index order via raw IEEE-754 bits (no subsampling, no
    // corner double-counting). This is the ONLY hash whose equality may back a
    // "VERIFIED deterministic" claim; ComputeHash(N>1) is a fast preview only.
    HdCarWashFrameHash result;

    if (width == 0 || height == 0 || color.empty()) {
        return result;
    }

    uint64_t colorH = FNV_OFFSET_BASIS;
    uint64_t depthH = FNV_OFFSET_BASIS;
    uint64_t normalH = FNV_OFFSET_BASIS;
    uint64_t idH = FNV_OFFSET_BASIS;

    const size_t numPixels = static_cast<size_t>(width) * height;
    uint32_t nonEmpty = 0;

    for (size_t idx = 0; idx < numPixels; ++idx) {
        const GfVec4f& c = color[idx];
        colorH = fnv1a_float(colorH, c[0]);
        colorH = fnv1a_float(colorH, c[1]);
        colorH = fnv1a_float(colorH, c[2]);
        colorH = fnv1a_float(colorH, c[3]);

        depthH = fnv1a_float(depthH, depth[idx]);

        const GfVec3f& n = normal[idx];
        normalH = fnv1a_float(normalH, n[0]);
        normalH = fnv1a_float(normalH, n[1]);
        normalH = fnv1a_float(normalH, n[2]);

        idH = fnv1a_hash(idH, &objectId[idx], sizeof(int32_t));
        idH = fnv1a_hash(idH, &primId[idx], sizeof(int32_t));

        if (objectId[idx] >= 0) {
            nonEmpty++;
        }
    }

    result.colorHash = colorH;
    result.depthHash = depthH;
    result.normalHash = normalH;
    result.idHash = idH;
    result.combined = colorH ^ depthH ^ normalH ^ idH;
    result.pixelsSampled = static_cast<uint32_t>(numPixels);
    result.nonEmptyPixels = nonEmpty;

    return result;
}

// ============================================================================
// Rasterizer
// ============================================================================

HdCarWashRasterizer::HdCarWashRasterizer()
    : _framebuffer(nullptr)
    , _viewProjMatrix(1.0)
    , _viewMatrix(1.0)
    , _normalMatrix(1.0)
    , _cameraPos(0.0f, 0.0f, 5.0f)
{
}

HdCarWashRasterizer::~HdCarWashRasterizer()
{
}

void
HdCarWashRasterizer::SetFramebuffer(HdCarWashFramebuffer* framebuffer)
{
    _framebuffer = framebuffer;
}

void
HdCarWashRasterizer::SetViewProjectionMatrix(GfMatrix4d const& viewProj)
{
    _viewProjMatrix = viewProj;
}

void
HdCarWashRasterizer::SetViewMatrix(GfMatrix4d const& view)
{
    _viewMatrix = view;
    // Normal matrix is inverse transpose of model-view
    // For now we'll compute per-mesh
}

void
HdCarWashRasterizer::Clear(GfVec4f const& clearColor, float clearDepth)
{
    if (!_framebuffer) return;

    std::fill(_framebuffer->color.begin(), _framebuffer->color.end(), clearColor);
    std::fill(_framebuffer->depth.begin(), _framebuffer->depth.end(), clearDepth);
    std::fill(_framebuffer->normal.begin(), _framebuffer->normal.end(), GfVec3f(0.0f));
    std::fill(_framebuffer->objectId.begin(), _framebuffer->objectId.end(), -1);
    std::fill(_framebuffer->primId.begin(), _framebuffer->primId.end(), -1);
}

void
HdCarWashRasterizer::ClearDepthOnly(float clearDepth)
{
    if (!_framebuffer) return;

    // Preserve color buffer (AI result), only clear depth and auxiliary buffers
    std::fill(_framebuffer->depth.begin(), _framebuffer->depth.end(), clearDepth);
    std::fill(_framebuffer->normal.begin(), _framebuffer->normal.end(), GfVec3f(0.0f));
    std::fill(_framebuffer->objectId.begin(), _framebuffer->objectId.end(), -1);
    std::fill(_framebuffer->primId.begin(), _framebuffer->primId.end(), -1);
}

void
HdCarWashRasterizer::SetLights(const std::vector<HdCarWashLightData>& lights)
{
    _lights = lights;
    TF_DEBUG_MSG(HD_CARWASH, "Rasterizer: %zu lights set\n", _lights.size());
}

void
HdCarWashRasterizer::ClearLights()
{
    _lights.clear();
}

void
HdCarWashRasterizer::SetCameraPosition(GfVec3f const& cameraPos)
{
    _cameraPos = cameraPos;
}

GfVec4f
HdCarWashRasterizer::ComputeShading(
    GfVec3f const& worldNormal,
    GfVec3f const& worldPos) const
{
    // Neutral gray base color (better for AI stylization)
    GfVec3f baseColor(0.7f, 0.7f, 0.7f);

    // Specular parameters (Blinn-Phong)
    float specularPower = 32.0f;
    float specularIntensity = 0.3f;
    GfVec3f specularColor(1.0f, 1.0f, 1.0f);

    // Ambient term
    float ambient = 0.15f;
    GfVec3f result = baseColor * ambient;
    float specularAccum = 0.0f;

    // View direction for specular
    GfVec3f viewDir = (_cameraPos - worldPos).GetNormalized();

    // If no lights, use default directional light
    if (_lights.empty()) {
        GfVec3f defaultLightDir = GfVec3f(0.5f, 0.5f, 0.7f).GetNormalized();
        float NdotL = std::max(0.0f, GfDot(worldNormal, defaultLightDir));
        result += baseColor * NdotL * 0.85f;

        // Blinn-Phong specular
        GfVec3f halfDir = (defaultLightDir + viewDir).GetNormalized();
        float NdotH = std::max(0.0f, GfDot(worldNormal, halfDir));
        specularAccum += std::pow(NdotH, specularPower);
    } else {
        // Accumulate contribution from all lights
        for (const auto& light : _lights) {
            if (!light.enabled) continue;

            GfVec3f lightDir;
            float attenuation = 1.0f;

            if (light.type == HdPrimTypeTokens->distantLight ||
                light.type == HdPrimTypeTokens->simpleLight) {
                // Directional light - use light direction directly
                lightDir = light.direction.GetNormalized();
            } else {
                // Point/sphere/rect light - compute direction from position
                lightDir = (light.position - worldPos).GetNormalized();

                // Distance attenuation (inverse square falloff)
                float distance = (light.position - worldPos).GetLength();
                if (distance > 0.001f) {
                    attenuation = 1.0f / (1.0f + distance * distance * 0.1f);
                }
            }

            // N dot L (diffuse)
            float NdotL = std::max(0.0f, GfDot(worldNormal, lightDir));

            // Accumulate diffuse light contribution
            GfVec3f lightContrib = GfVec3f(
                baseColor[0] * light.radiance[0],
                baseColor[1] * light.radiance[1],
                baseColor[2] * light.radiance[2]
            ) * NdotL * attenuation;

            result += lightContrib;

            // Blinn-Phong specular
            GfVec3f halfDir = (lightDir + viewDir).GetNormalized();
            float NdotH = std::max(0.0f, GfDot(worldNormal, halfDir));
            specularAccum += std::pow(NdotH, specularPower) * attenuation;
        }
    }

    // Add specular contribution
    result += specularColor * specularAccum * specularIntensity;

    // Clamp to [0, 1]
    result[0] = std::min(1.0f, result[0]);
    result[1] = std::min(1.0f, result[1]);
    result[2] = std::min(1.0f, result[2]);

    return GfVec4f(result[0], result[1], result[2], 1.0f);
}

void
HdCarWashRasterizer::RasterizeMesh(HdCarWashMesh const* mesh)
{
    if (!mesh || !_framebuffer || !mesh->IsVisible()) {
        return;
    }

    VtVec3fArray const& points = mesh->GetPoints();
    VtIntArray const& faceVertexCounts = mesh->GetFaceVertexCounts();
    VtIntArray const& faceVertexIndices = mesh->GetFaceVertexIndices();
    VtVec3fArray const& normals = mesh->GetNormals();
    GfMatrix4d const& modelMatrix = mesh->GetTransform();
    int objectId = mesh->GetObjectId();

    if (points.empty() || faceVertexIndices.empty()) {
        return;
    }

    TF_DEBUG_MSG(HD_CARWASH, "Rasterizing mesh objectId=%d, %zu verts, %zu faces\n",
                 objectId, points.size(), faceVertexCounts.size());

    // Compute MVP matrix for this mesh
    GfMatrix4d mvp = modelMatrix * _viewProjMatrix;

    // Normal matrix (inverse transpose of model matrix upper 3x3)
    GfMatrix4d normalMat = modelMatrix.GetInverse().GetTranspose();

    // Collect triangles
    std::vector<ScreenTriangle> triangles;
    triangles.reserve(faceVertexIndices.size() / 3);

    size_t indexOffset = 0;
    int primId = 0;

    for (size_t faceIdx = 0; faceIdx < faceVertexCounts.size(); ++faceIdx) {
        int vertCount = faceVertexCounts[faceIdx];

        // Triangulate n-gons using fan triangulation
        for (int v = 1; v < vertCount - 1; ++v) {
            int i0 = faceVertexIndices[indexOffset];
            int i1 = faceVertexIndices[indexOffset + v];
            int i2 = faceVertexIndices[indexOffset + v + 1];

            // Get positions
            GfVec3f p0 = points[i0];
            GfVec3f p1 = points[i1];
            GfVec3f p2 = points[i2];

            // Get normals (use computed if available, otherwise face normal)
            GfVec3f n0, n1, n2;
            if (!normals.empty() && normals.size() == points.size()) {
                n0 = normals[i0];
                n1 = normals[i1];
                n2 = normals[i2];
            } else {
                // Compute face normal
                GfVec3f edge1 = p1 - p0;
                GfVec3f edge2 = p2 - p0;
                GfVec3f faceNormal = GfCross(edge1, edge2).GetNormalized();
                n0 = n1 = n2 = faceNormal;
            }

            // Transform normals to world space (USD row-vector convention: vec * matrix)
            GfVec4d n0w = GfVec4d(n0[0], n0[1], n0[2], 0.0) * normalMat;
            GfVec4d n1w = GfVec4d(n1[0], n1[1], n1[2], 0.0) * normalMat;
            GfVec4d n2w = GfVec4d(n2[0], n2[1], n2[2], 0.0) * normalMat;
            n0 = GfVec3f(static_cast<float>(n0w[0]), static_cast<float>(n0w[1]), static_cast<float>(n0w[2])).GetNormalized();
            n1 = GfVec3f(static_cast<float>(n1w[0]), static_cast<float>(n1w[1]), static_cast<float>(n1w[2])).GetNormalized();
            n2 = GfVec3f(static_cast<float>(n2w[0]), static_cast<float>(n2w[1]), static_cast<float>(n2w[2])).GetNormalized();

            // Transform and clip
            TransformTriangle(p0, p1, p2, n0, n1, n2, modelMatrix, mvp,
                              objectId, primId, triangles);
        }

        indexOffset += vertCount;
        primId++;
    }

    TF_DEBUG_MSG(HD_CARWASH, "  Generated %zu screen triangles\n", triangles.size());

    // Rasterize all triangles
    for (auto const& tri : triangles) {
        RasterizeTriangle(tri);
    }
}

bool
HdCarWashRasterizer::TransformTriangle(
    GfVec3f const& p0, GfVec3f const& p1, GfVec3f const& p2,
    GfVec3f const& n0, GfVec3f const& n1, GfVec3f const& n2,
    GfMatrix4d const& modelMatrix,
    GfMatrix4d const& mvp,
    int objectId, int primId,
    std::vector<ScreenTriangle>& outTriangles)
{
    // Compute world positions (for shading)
    GfVec4d world0d = GfVec4d(p0[0], p0[1], p0[2], 1.0) * modelMatrix;
    GfVec4d world1d = GfVec4d(p1[0], p1[1], p1[2], 1.0) * modelMatrix;
    GfVec4d world2d = GfVec4d(p2[0], p2[1], p2[2], 1.0) * modelMatrix;

    // Transform to clip space using post-multiplication (USD row-vector convention)
    // USD uses row-major matrices with point * matrix order
    GfVec4d clip0d = GfVec4d(p0[0], p0[1], p0[2], 1.0) * mvp;
    GfVec4d clip1d = GfVec4d(p1[0], p1[1], p1[2], 1.0) * mvp;
    GfVec4d clip2d = GfVec4d(p2[0], p2[1], p2[2], 1.0) * mvp;
    GfVec4f clip0(static_cast<float>(clip0d[0]), static_cast<float>(clip0d[1]),
                  static_cast<float>(clip0d[2]), static_cast<float>(clip0d[3]));
    GfVec4f clip1(static_cast<float>(clip1d[0]), static_cast<float>(clip1d[1]),
                  static_cast<float>(clip1d[2]), static_cast<float>(clip1d[3]));
    GfVec4f clip2(static_cast<float>(clip2d[0]), static_cast<float>(clip2d[1]),
                  static_cast<float>(clip2d[2]), static_cast<float>(clip2d[3]));

    // Simple near-plane culling (reject if all behind near plane)
    // In clip space, w component determines visibility
    if (clip0[3] <= 0 && clip1[3] <= 0 && clip2[3] <= 0) {
        return false;
    }

    // Perspective divide
    auto perspDiv = [](GfVec4f const& c) -> GfVec3f {
        if (std::abs(c[3]) < 1e-6f) {
            return GfVec3f(0.0f);
        }
        float invW = 1.0f / c[3];
        return GfVec3f(c[0] * invW, c[1] * invW, c[2] * invW);
    };

    GfVec3f ndc0 = perspDiv(clip0);
    GfVec3f ndc1 = perspDiv(clip1);
    GfVec3f ndc2 = perspDiv(clip2);

    // Frustum culling (reject if entirely outside [-1,1] in XY)
    auto outsideNDC = [](GfVec3f const& p) -> bool {
        return p[0] < -1.0f || p[0] > 1.0f ||
               p[1] < -1.0f || p[1] > 1.0f ||
               p[2] < -1.0f || p[2] > 1.0f;
    };

    if (outsideNDC(ndc0) && outsideNDC(ndc1) && outsideNDC(ndc2)) {
        // Simple rejection - could be more precise with clipping
        // For now, accept triangles that might partially overlap
    }

    // Convert NDC to screen coordinates
    float halfW = _framebuffer->width * 0.5f;
    float halfH = _framebuffer->height * 0.5f;

    auto ndcToScreen = [halfW, halfH](GfVec3f const& ndc) -> GfVec3f {
        // NDC X,Y in [-1,1] -> Screen [0, width], [0, height]
        // Z stays as linear depth in [0,1] range (0=near, 1=far after perspective divide)
        return GfVec3f(
            (ndc[0] + 1.0f) * halfW,
            (1.0f - ndc[1]) * halfH,  // Flip Y for screen coords
            (ndc[2] + 1.0f) * 0.5f    // Map depth from [-1,1] to [0,1]
        );
    };

    ScreenTriangle tri;
    tri.screenPos[0] = ndcToScreen(ndc0);
    tri.screenPos[1] = ndcToScreen(ndc1);
    tri.screenPos[2] = ndcToScreen(ndc2);
    tri.worldPos[0] = GfVec3f(static_cast<float>(world0d[0]),
                               static_cast<float>(world0d[1]),
                               static_cast<float>(world0d[2]));
    tri.worldPos[1] = GfVec3f(static_cast<float>(world1d[0]),
                               static_cast<float>(world1d[1]),
                               static_cast<float>(world1d[2]));
    tri.worldPos[2] = GfVec3f(static_cast<float>(world2d[0]),
                               static_cast<float>(world2d[1]),
                               static_cast<float>(world2d[2]));
    tri.worldNormal[0] = n0;
    tri.worldNormal[1] = n1;
    tri.worldNormal[2] = n2;
    tri.objectId = objectId;
    tri.primId = primId;

    outTriangles.push_back(tri);
    return true;
}

void
HdCarWashRasterizer::RasterizeTriangle(ScreenTriangle const& tri)
{
    if (!_framebuffer) return;

    int width = static_cast<int>(_framebuffer->width);
    int height = static_cast<int>(_framebuffer->height);

    // Get screen positions
    GfVec2f v0(tri.screenPos[0][0], tri.screenPos[0][1]);
    GfVec2f v1(tri.screenPos[1][0], tri.screenPos[1][1]);
    GfVec2f v2(tri.screenPos[2][0], tri.screenPos[2][1]);

    // Compute bounding box
    int minX = static_cast<int>(std::floor(std::min({v0[0], v1[0], v2[0]})));
    int maxX = static_cast<int>(std::ceil(std::max({v0[0], v1[0], v2[0]})));
    int minY = static_cast<int>(std::floor(std::min({v0[1], v1[1], v2[1]})));
    int maxY = static_cast<int>(std::ceil(std::max({v0[1], v1[1], v2[1]})));

    // Clamp to screen bounds
    minX = std::max(0, minX);
    maxX = std::min(width - 1, maxX);
    minY = std::max(0, minY);
    maxY = std::min(height - 1, maxY);

    // Skip degenerate triangles
    float area = EdgeFunction(v0, v1, v2);
    if (std::abs(area) < 1e-6f) {
        return;
    }

    float invArea = 1.0f / area;

    // Rasterize using edge functions
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            GfVec2f p(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);

            // Compute barycentric coordinates using edge functions
            float w0 = EdgeFunction(v1, v2, p);
            float w1 = EdgeFunction(v2, v0, p);
            float w2 = EdgeFunction(v0, v1, p);

            // Check if point is inside triangle
            bool inside = (area > 0) ?
                (w0 >= 0 && w1 >= 0 && w2 >= 0) :
                (w0 <= 0 && w1 <= 0 && w2 <= 0);

            if (!inside) {
                continue;
            }

            // Normalize barycentric coordinates
            w0 *= invArea;
            w1 *= invArea;
            w2 *= invArea;

            // Make sure they're positive
            w0 = std::abs(w0);
            w1 = std::abs(w1);
            w2 = std::abs(w2);

            // Interpolate depth
            float depth = w0 * tri.screenPos[0][2] +
                          w1 * tri.screenPos[1][2] +
                          w2 * tri.screenPos[2][2];

            // Depth test
            size_t pixelIdx = static_cast<size_t>(y) * width + x;
            if (depth >= _framebuffer->depth[pixelIdx]) {
                continue;
            }

            // Update depth buffer
            _framebuffer->depth[pixelIdx] = depth;

            // Interpolate normal
            GfVec3f normal = (w0 * tri.worldNormal[0] +
                              w1 * tri.worldNormal[1] +
                              w2 * tri.worldNormal[2]).GetNormalized();
            _framebuffer->normal[pixelIdx] = normal;

            // Interpolate world position
            GfVec3f worldPos = w0 * tri.worldPos[0] +
                               w1 * tri.worldPos[1] +
                               w2 * tri.worldPos[2];

            // Write IDs
            _framebuffer->objectId[pixelIdx] = tri.objectId;
            _framebuffer->primId[pixelIdx] = tri.primId;

            // Compute shading using scene lights (or fallback)
            GfVec4f shadedColor = ComputeShading(normal, worldPos);
            _framebuffer->color[pixelIdx] = shadedColor;
        }
    }
}

float
HdCarWashRasterizer::EdgeFunction(
    GfVec2f const& a, GfVec2f const& b, GfVec2f const& c)
{
    return (c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]);
}

GfVec3f
HdCarWashRasterizer::Barycentric(
    GfVec2f const& p,
    GfVec2f const& a, GfVec2f const& b, GfVec2f const& c)
{
    GfVec2f v0 = b - a;
    GfVec2f v1 = c - a;
    GfVec2f v2 = p - a;

    float d00 = GfDot(v0, v0);
    float d01 = GfDot(v0, v1);
    float d11 = GfDot(v1, v1);
    float d20 = GfDot(v2, v0);
    float d21 = GfDot(v2, v1);

    float denom = d00 * d11 - d01 * d01;
    if (std::abs(denom) < 1e-10f) {
        return GfVec3f(1.0f, 0.0f, 0.0f);
    }

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return GfVec3f(u, v, w);
}

PXR_NAMESPACE_CLOSE_SCOPE
