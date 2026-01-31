// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// camera.h — Camera representation for view/projection matrices

#ifndef HD_CARWASH_CAMERA_H
#define HD_CARWASH_CAMERA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range1f.h"

#include "api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdCarWashCamera
///
/// Camera representation for the CarWash render delegate.
/// Provides view and projection matrices for CPU rasterization.
///
class HDCARWASH_API HdCarWashCamera final : public HdCamera
{
public:
    HdCarWashCamera(SdfPath const& id);
    ~HdCarWashCamera() override;

    /// Sync camera data from scene delegate
    void Sync(
        HdSceneDelegate* sceneDelegate,
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits) override;

    /// Finalize camera
    void Finalize(HdRenderParam* renderParam) override;

    // =========================================================================
    // Camera Accessors (for rasterizer)
    // =========================================================================

    /// Get world-to-view (camera) matrix
    GfMatrix4d const& GetViewMatrix() const { return _viewMatrix; }

    /// Get view-to-clip (projection) matrix
    GfMatrix4d const& GetProjectionMatrix() const { return _projectionMatrix; }

    /// Get combined view-projection matrix
    GfMatrix4d GetViewProjectionMatrix() const {
        return _viewMatrix * _projectionMatrix;
    }

    /// Get clipping range (near, far)
    GfRange1f const& GetClippingRange() const { return _clippingRange; }

    /// Get horizontal aperture (for FOV calculation)
    float GetHorizontalAperture() const { return _horizontalAperture; }

    /// Get vertical aperture
    float GetVerticalAperture() const { return _verticalAperture; }

    /// Get focal length
    float GetFocalLength() const { return _focalLength; }

    /// Check if camera uses orthographic projection
    bool IsOrthographic() const;

    /// Compute projection matrix for given aspect ratio
    GfMatrix4d ComputeProjectionMatrix(float aspectRatio) const;

private:
    // Camera matrices
    GfMatrix4d _viewMatrix;
    GfMatrix4d _projectionMatrix;

    // Camera parameters
    GfRange1f _clippingRange;
    float _horizontalAperture;
    float _verticalAperture;
    float _focalLength;
    HdCamera::Projection _projection;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_CARWASH_CAMERA_H
