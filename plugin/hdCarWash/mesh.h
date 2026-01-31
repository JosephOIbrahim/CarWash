// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// mesh.h — Mesh geometry representation

#ifndef HD_CARWASH_MESH_H
#define HD_CARWASH_MESH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/vt/array.h"

#include "api.h"

#include <mutex>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

class HdCarWashRenderDelegate;

/// \class HdCarWashMesh
///
/// Mesh geometry representation for the CarWash render delegate.
/// Syncs geometry data from the scene delegate and stores it
/// for CPU rasterization to generate semantic AOVs.
///
class HDCARWASH_API HdCarWashMesh final : public HdMesh
{
public:
    HdCarWashMesh(SdfPath const& id);
    ~HdCarWashMesh() override;

    /// Sync geometry from scene delegate
    void Sync(
        HdSceneDelegate* sceneDelegate,
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits,
        TfToken const& reprToken) override;

    /// Get initial dirty bits mask for a new mesh
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Finalize mesh (cleanup)
    void Finalize(HdRenderParam* renderParam) override;

    // =========================================================================
    // Geometry Accessors (for rasterizer)
    // =========================================================================

    /// Get mesh points in local space
    VtVec3fArray const& GetPoints() const { return _points; }

    /// Get face vertex counts (polygon topology)
    VtIntArray const& GetFaceVertexCounts() const { return _faceVertexCounts; }

    /// Get face vertex indices
    VtIntArray const& GetFaceVertexIndices() const { return _faceVertexIndices; }

    /// Get computed vertex normals
    VtVec3fArray const& GetNormals() const { return _normals; }

    /// Get UV coordinates (if available)
    VtVec2fArray const& GetUVs() const { return _uvs; }

    /// Get object-to-world transform
    GfMatrix4d const& GetTransform() const { return _transform; }

    /// Get unique object ID for AOV
    int GetObjectId() const { return _objectId; }

    /// Check if mesh is visible
    bool IsVisible() const { return _visible; }

    /// Check if mesh is double-sided
    bool IsDoubleSided() const { return _doubleSided; }

protected:
    /// Process dirty bits and determine what needs updating
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    /// Initialize representation
    void _InitRepr(TfToken const& reprToken, HdDirtyBits* dirtyBits) override;

private:
    // Sync helpers
    void _SyncTopology(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits);
    void _SyncPoints(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits);
    void _SyncNormals(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits);
    void _SyncPrimvars(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits);
    void _SyncTransform(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits);
    void _SyncVisibility(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits);

    // Compute smooth normals if not provided
    void _ComputeNormals();

    // Geometry data
    VtVec3fArray _points;
    VtIntArray _faceVertexCounts;
    VtIntArray _faceVertexIndices;
    VtVec3fArray _normals;
    VtVec2fArray _uvs;

    // Transform
    GfMatrix4d _transform;

    // Mesh properties
    int _objectId;
    bool _visible;
    bool _doubleSided;
    HdInterpolation _normalsInterpolation;

    // Thread safety
    mutable std::mutex _mutex;

    // Static counter for unique object IDs
    static std::atomic<int> _nextObjectId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_CARWASH_MESH_H
