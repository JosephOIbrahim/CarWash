// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// mesh.cpp — Mesh geometry implementation

#include "mesh.h"
#include "debugCodes.h"

#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/base/gf/vec3f.h"

#include <fstream>  // Debug file logging

PXR_NAMESPACE_OPEN_SCOPE

// Static object ID counter
std::atomic<int> HdCarWashMesh::_nextObjectId{1};

HdCarWashMesh::HdCarWashMesh(SdfPath const& id)
    : HdMesh(id)
    , _transform(1.0)  // Identity
    , _objectId(_nextObjectId++)
    , _visible(true)
    , _doubleSided(false)
    , _normalsInterpolation(HdInterpolationVertex)
{
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashMesh created: %s (objectId=%d)\n",
                 id.GetText(), _objectId);

    // DEBUG: Log mesh creation to file
    std::ofstream debugLog("C:/Temp/hdcarwash_debug.txt", std::ios::app);
    debugLog << "[MeshCreated] path: " << id.GetText()
             << " objectId: " << _objectId << std::endl;
    debugLog.close();
}

HdCarWashMesh::~HdCarWashMesh()
{
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashMesh destroyed: %s\n", GetId().GetText());
}

HdDirtyBits
HdCarWashMesh::GetInitialDirtyBitsMask() const
{
    // Request all geometry data on first sync
    return HdChangeTracker::Clean
        | HdChangeTracker::DirtyPoints
        | HdChangeTracker::DirtyTopology
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyNormals
        | HdChangeTracker::DirtyDoubleSided;
}

void
HdCarWashMesh::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits,
    TfToken const& reprToken)
{
    TF_UNUSED(renderParam);
    TF_UNUSED(reprToken);

    std::lock_guard<std::mutex> lock(_mutex);

    SdfPath const& id = GetId();

    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashMesh::Sync %s dirtyBits=0x%x\n",
                 id.GetText(), *dirtyBits);

    // DEBUG: Log sync to file
    std::ofstream debugLog("C:/Temp/hdcarwash_debug.txt", std::ios::app);
    debugLog << "[MeshSync] path: " << id.GetText()
             << " dirtyBits: 0x" << std::hex << *dirtyBits << std::dec << std::endl;
    debugLog.close();

    // Sync topology first (needed for other syncs)
    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
        _SyncTopology(sceneDelegate, dirtyBits);
    }

    // Sync points
    if (HdChangeTracker::IsPrimvarDirty(*dirtyBits, id, HdTokens->points)) {
        _SyncPoints(sceneDelegate, dirtyBits);
    }

    // Sync transform
    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id)) {
        _SyncTransform(sceneDelegate, dirtyBits);
    }

    // Sync visibility
    if (HdChangeTracker::IsVisibilityDirty(*dirtyBits, id)) {
        _SyncVisibility(sceneDelegate, dirtyBits);
    }

    // Sync normals
    if (*dirtyBits & HdChangeTracker::DirtyNormals) {
        _SyncNormals(sceneDelegate, dirtyBits);
    }

    // Sync other primvars (UVs, etc.)
    if (*dirtyBits & HdChangeTracker::DirtyPrimvar) {
        _SyncPrimvars(sceneDelegate, dirtyBits);
    }

    // Sync double-sided
    if (HdChangeTracker::IsDoubleSidedDirty(*dirtyBits, id)) {
        _doubleSided = sceneDelegate->GetDoubleSided(id);
    }

    // Clear dirty bits
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

void
HdCarWashMesh::_SyncTopology(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits)
{
    TF_UNUSED(dirtyBits);
    SdfPath const& id = GetId();
    HdMeshTopology topology = sceneDelegate->GetMeshTopology(id);

    _faceVertexCounts = topology.GetFaceVertexCounts();
    _faceVertexIndices = topology.GetFaceVertexIndices();

    TF_DEBUG_MSG(HD_CARWASH, "  Topology: %zu faces, %zu indices\n",
                 _faceVertexCounts.size(), _faceVertexIndices.size());
}

void
HdCarWashMesh::_SyncPoints(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits)
{
    TF_UNUSED(dirtyBits);
    SdfPath const& id = GetId();
    VtValue pointsValue = sceneDelegate->Get(id, HdTokens->points);

    if (pointsValue.IsHolding<VtVec3fArray>()) {
        _points = pointsValue.UncheckedGet<VtVec3fArray>();
        TF_DEBUG_MSG(HD_CARWASH, "  Points: %zu vertices\n", _points.size());
    } else {
        TF_WARN("Mesh %s: points not VtVec3fArray", id.GetText());
    }
}

void
HdCarWashMesh::_SyncNormals(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits)
{
    TF_UNUSED(dirtyBits);
    SdfPath const& id = GetId();

    // Try to get authored normals
    HdPrimvarDescriptorVector primvars =
        sceneDelegate->GetPrimvarDescriptors(id, HdInterpolationVertex);

    bool foundNormals = false;
    for (const auto& pv : primvars) {
        if (pv.name == HdTokens->normals) {
            VtValue normalsValue = sceneDelegate->Get(id, HdTokens->normals);
            if (normalsValue.IsHolding<VtVec3fArray>()) {
                _normals = normalsValue.UncheckedGet<VtVec3fArray>();
                _normalsInterpolation = pv.interpolation;
                foundNormals = true;
                TF_DEBUG_MSG(HD_CARWASH, "  Normals: %zu (authored)\n",
                             _normals.size());
            }
            break;
        }
    }

    // Check face-varying normals too
    if (!foundNormals) {
        primvars = sceneDelegate->GetPrimvarDescriptors(id, HdInterpolationFaceVarying);
        for (const auto& pv : primvars) {
            if (pv.name == HdTokens->normals) {
                VtValue normalsValue = sceneDelegate->Get(id, HdTokens->normals);
                if (normalsValue.IsHolding<VtVec3fArray>()) {
                    _normals = normalsValue.UncheckedGet<VtVec3fArray>();
                    _normalsInterpolation = pv.interpolation;
                    foundNormals = true;
                    TF_DEBUG_MSG(HD_CARWASH, "  Normals: %zu (face-varying)\n",
                                 _normals.size());
                }
                break;
            }
        }
    }

    // Compute smooth normals if not provided
    if (!foundNormals && !_points.empty() && !_faceVertexIndices.empty()) {
        _ComputeNormals();
    }
}

void
HdCarWashMesh::_ComputeNormals()
{
    TF_DEBUG_MSG(HD_CARWASH, "  Computing smooth normals...\n");

    // Simple smooth normal computation:
    // 1. Compute face normals
    // 2. Average normals at each vertex from adjacent faces

    size_t numVerts = _points.size();
    _normals.resize(numVerts);

    // Initialize normals to zero
    for (size_t i = 0; i < numVerts; ++i) {
        _normals[i] = GfVec3f(0.0f);
    }

    // Iterate through faces and accumulate normals at vertices
    size_t indexOffset = 0;
    for (size_t faceIdx = 0; faceIdx < _faceVertexCounts.size(); ++faceIdx) {
        int vertCount = _faceVertexCounts[faceIdx];
        if (vertCount < 3) {
            indexOffset += vertCount;
            continue;
        }

        // Get first three vertices of the face for normal calculation
        int i0 = _faceVertexIndices[indexOffset];
        int i1 = _faceVertexIndices[indexOffset + 1];
        int i2 = _faceVertexIndices[indexOffset + 2];

        GfVec3f const& p0 = _points[i0];
        GfVec3f const& p1 = _points[i1];
        GfVec3f const& p2 = _points[i2];

        // Compute face normal using cross product
        GfVec3f edge1 = p1 - p0;
        GfVec3f edge2 = p2 - p0;
        GfVec3f faceNormal = GfCross(edge1, edge2);

        // Add face normal to all vertices of this face
        for (int v = 0; v < vertCount; ++v) {
            int vertIdx = _faceVertexIndices[indexOffset + v];
            _normals[vertIdx] += faceNormal;
        }

        indexOffset += vertCount;
    }

    // Normalize all vertex normals
    for (size_t i = 0; i < numVerts; ++i) {
        float len = _normals[i].GetLength();
        if (len > 1e-6f) {
            _normals[i] /= len;
        } else {
            _normals[i] = GfVec3f(0.0f, 0.0f, 1.0f);  // Default up
        }
    }

    _normalsInterpolation = HdInterpolationVertex;

    TF_DEBUG_MSG(HD_CARWASH, "  Normals: %zu (computed)\n", _normals.size());
}

void
HdCarWashMesh::_SyncPrimvars(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits)
{
    TF_UNUSED(dirtyBits);
    SdfPath const& id = GetId();

    // Look for UV coordinates in various primvar names
    static const TfToken uvNames[] = {
        TfToken("st"),
        TfToken("uv"),
        TfToken("UVMap")
    };

    bool foundUVs = false;

    // Check vertex interpolation
    HdPrimvarDescriptorVector primvars =
        sceneDelegate->GetPrimvarDescriptors(id, HdInterpolationVertex);

    for (const auto& pv : primvars) {
        for (const auto& uvName : uvNames) {
            if (pv.name == uvName) {
                VtValue uvValue = sceneDelegate->Get(id, pv.name);
                if (uvValue.IsHolding<VtVec2fArray>()) {
                    _uvs = uvValue.UncheckedGet<VtVec2fArray>();
                    foundUVs = true;
                    TF_DEBUG_MSG(HD_CARWASH, "  UVs: %zu (vertex, '%s')\n",
                                 _uvs.size(), pv.name.GetText());
                    break;
                }
            }
        }
        if (foundUVs) break;
    }

    // Check face-varying interpolation if not found
    if (!foundUVs) {
        primvars = sceneDelegate->GetPrimvarDescriptors(id, HdInterpolationFaceVarying);
        for (const auto& pv : primvars) {
            for (const auto& uvName : uvNames) {
                if (pv.name == uvName) {
                    VtValue uvValue = sceneDelegate->Get(id, pv.name);
                    if (uvValue.IsHolding<VtVec2fArray>()) {
                        _uvs = uvValue.UncheckedGet<VtVec2fArray>();
                        foundUVs = true;
                        TF_DEBUG_MSG(HD_CARWASH, "  UVs: %zu (face-varying, '%s')\n",
                                     _uvs.size(), pv.name.GetText());
                        break;
                    }
                }
            }
            if (foundUVs) break;
        }
    }
}

void
HdCarWashMesh::_SyncTransform(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits)
{
    TF_UNUSED(dirtyBits);
    _transform = sceneDelegate->GetTransform(GetId());

    TF_DEBUG_MSG(HD_CARWASH, "  Transform updated\n");
}

void
HdCarWashMesh::_SyncVisibility(HdSceneDelegate* sceneDelegate, HdDirtyBits* dirtyBits)
{
    TF_UNUSED(dirtyBits);
    _visible = sceneDelegate->GetVisible(GetId());

    TF_DEBUG_MSG(HD_CARWASH, "  Visible: %s\n", _visible ? "true" : "false");
}

HdDirtyBits
HdCarWashMesh::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void
HdCarWashMesh::_InitRepr(TfToken const& reprToken, HdDirtyBits* dirtyBits)
{
    TF_UNUSED(reprToken);
    TF_UNUSED(dirtyBits);
    // No representation caching needed for CPU rasterization
}

void
HdCarWashMesh::Finalize(HdRenderParam* renderParam)
{
    TF_UNUSED(renderParam);
    // Cleanup if needed
}

PXR_NAMESPACE_CLOSE_SCOPE
