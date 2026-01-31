// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// camera.cpp — Camera implementation

#include "camera.h"
#include "debugCodes.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/base/gf/frustum.h"

PXR_NAMESPACE_OPEN_SCOPE

HdCarWashCamera::HdCarWashCamera(SdfPath const& id)
    : HdCamera(id)
    , _viewMatrix(1.0)
    , _projectionMatrix(1.0)
    , _clippingRange(0.1f, 10000.0f)
    , _horizontalAperture(36.0f)  // 35mm full frame default
    , _verticalAperture(24.0f)
    , _focalLength(50.0f)
    , _projection(HdCamera::Perspective)
{
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashCamera created: %s\n", id.GetText());
}

HdCarWashCamera::~HdCarWashCamera()
{
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashCamera destroyed: %s\n", GetId().GetText());
}

void
HdCarWashCamera::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    TF_UNUSED(renderParam);

    SdfPath const& id = GetId();

    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashCamera::Sync %s dirtyBits=0x%x\n",
                 id.GetText(), *dirtyBits);

    if (*dirtyBits & HdCamera::DirtyTransform) {
        // Get camera transform (world-to-camera)
        // The scene delegate gives us the camera's world transform,
        // we need to invert it for the view matrix
        GfMatrix4d worldTransform = sceneDelegate->GetTransform(id);
        _viewMatrix = worldTransform.GetInverse();

        TF_DEBUG_MSG(HD_CARWASH, "  View matrix updated\n");
    }

    if (*dirtyBits & HdCamera::DirtyParams) {
        // Get camera parameters
        VtValue clippingRangeValue = sceneDelegate->GetCameraParamValue(
            id, HdCameraTokens->clippingRange);
        if (clippingRangeValue.IsHolding<GfRange1f>()) {
            _clippingRange = clippingRangeValue.UncheckedGet<GfRange1f>();
        }

        VtValue horizontalApertureValue = sceneDelegate->GetCameraParamValue(
            id, HdCameraTokens->horizontalAperture);
        if (horizontalApertureValue.IsHolding<float>()) {
            _horizontalAperture = horizontalApertureValue.UncheckedGet<float>();
        }

        VtValue verticalApertureValue = sceneDelegate->GetCameraParamValue(
            id, HdCameraTokens->verticalAperture);
        if (verticalApertureValue.IsHolding<float>()) {
            _verticalAperture = verticalApertureValue.UncheckedGet<float>();
        }

        VtValue focalLengthValue = sceneDelegate->GetCameraParamValue(
            id, HdCameraTokens->focalLength);
        if (focalLengthValue.IsHolding<float>()) {
            _focalLength = focalLengthValue.UncheckedGet<float>();
        }

        VtValue projectionValue = sceneDelegate->GetCameraParamValue(
            id, HdCameraTokens->projection);
        if (projectionValue.IsHolding<HdCamera::Projection>()) {
            _projection = projectionValue.UncheckedGet<HdCamera::Projection>();
        }

        TF_DEBUG_MSG(HD_CARWASH, "  Aperture: %.1f x %.1f mm, focal: %.1f mm\n",
                     _horizontalAperture, _verticalAperture, _focalLength);
        TF_DEBUG_MSG(HD_CARWASH, "  Clipping: %.2f - %.2f\n",
                     _clippingRange.GetMin(), _clippingRange.GetMax());
    }

    // Clear dirty bits
    *dirtyBits = HdChangeTracker::Clean;
}

bool
HdCarWashCamera::IsOrthographic() const
{
    return _projection == HdCamera::Orthographic;
}

GfMatrix4d
HdCarWashCamera::ComputeProjectionMatrix(float aspectRatio) const
{
    GfFrustum frustum;

    if (IsOrthographic()) {
        // Orthographic projection using frustum
        float halfWidth = _horizontalAperture * 0.5f;
        float halfHeight = halfWidth / aspectRatio;

        frustum.SetOrthographic(
            -halfWidth, halfWidth,
            -halfHeight, halfHeight,
            _clippingRange.GetMin(),
            _clippingRange.GetMax());
    } else {
        // Perspective projection
        // FOV from focal length and aperture
        // tan(fov/2) = (aperture/2) / focalLength
        double fovRadians = 2.0 * atan(_horizontalAperture * 0.5 / _focalLength);
        double fovDegrees = GfRadiansToDegrees(fovRadians);

        frustum.SetPerspective(
            fovDegrees,
            aspectRatio,
            _clippingRange.GetMin(),
            _clippingRange.GetMax());
    }

    return frustum.ComputeProjectionMatrix();
}

void
HdCarWashCamera::Finalize(HdRenderParam* renderParam)
{
    TF_UNUSED(renderParam);
    // Cleanup if needed
}

PXR_NAMESPACE_CLOSE_SCOPE
