// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// light.cpp — Light primitive implementation

#include "light.h"
#include "debugCodes.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/usd/usdLux/tokens.h"

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

HdCarWashLight::HdCarWashLight(SdfPath const& id, TfToken const& lightType)
    : HdLight(id)
    , _lightType(lightType)
    , _color(1.0f, 1.0f, 1.0f)
    , _intensity(1.0f)
    , _exposure(0.0f)
    , _transform(1.0)
    , _enabled(true)
    , _radius(0.5f)
    , _width(1.0f)
    , _height(1.0f)
    , _coneAngle(90.0f)
    , _coneSoftness(0.0f)
    , _textureFile("")
{
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashLight created: %s (type: %s)\n",
                 id.GetText(), lightType.GetText());
}

HdCarWashLight::~HdCarWashLight()
{
    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashLight destroyed: %s\n", GetId().GetText());
}

void
HdCarWashLight::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    TF_UNUSED(renderParam);

    SdfPath const& id = GetId();

    TF_DEBUG_MSG(HD_CARWASH, "HdCarWashLight::Sync %s dirtyBits=0x%x\n",
                 id.GetText(), *dirtyBits);

    // Transform
    if (*dirtyBits & DirtyTransform) {
        _transform = sceneDelegate->GetTransform(id);
        TF_DEBUG_MSG(HD_CARWASH, "  Light transform updated\n");
    }

    // Parameters
    if (*dirtyBits & DirtyParams) {
        // Color
        VtValue colorVal = sceneDelegate->GetLightParamValue(id, HdLightTokens->color);
        if (colorVal.IsHolding<GfVec3f>()) {
            _color = colorVal.UncheckedGet<GfVec3f>();
        }

        // Intensity
        VtValue intensityVal = sceneDelegate->GetLightParamValue(id, HdLightTokens->intensity);
        if (intensityVal.IsHolding<float>()) {
            _intensity = intensityVal.UncheckedGet<float>();
        }

        // Exposure
        VtValue exposureVal = sceneDelegate->GetLightParamValue(id, HdLightTokens->exposure);
        if (exposureVal.IsHolding<float>()) {
            _exposure = exposureVal.UncheckedGet<float>();
        }

        // Enabled - check visibility as a proxy for enabled state
        // (HdLightTokens->enable may not exist in all USD versions)
        _enabled = sceneDelegate->GetVisible(id);

        // Type-specific parameters
        if (_lightType == HdPrimTypeTokens->sphereLight) {
            VtValue radiusVal = sceneDelegate->GetLightParamValue(id, HdLightTokens->radius);
            if (radiusVal.IsHolding<float>()) {
                _radius = radiusVal.UncheckedGet<float>();
            }
        }
        else if (_lightType == HdPrimTypeTokens->rectLight) {
            VtValue widthVal = sceneDelegate->GetLightParamValue(id, HdLightTokens->width);
            if (widthVal.IsHolding<float>()) {
                _width = widthVal.UncheckedGet<float>();
            }
            VtValue heightVal = sceneDelegate->GetLightParamValue(id, HdLightTokens->height);
            if (heightVal.IsHolding<float>()) {
                _height = heightVal.UncheckedGet<float>();
            }
        }
        else if (_lightType == HdPrimTypeTokens->domeLight) {
            VtValue textureVal = sceneDelegate->GetLightParamValue(id, HdLightTokens->textureFile);
            if (textureVal.IsHolding<SdfAssetPath>()) {
                _textureFile = textureVal.UncheckedGet<SdfAssetPath>().GetResolvedPath();
            }
        }

        TF_DEBUG_MSG(HD_CARWASH, "  Light params: color=(%.2f,%.2f,%.2f) intensity=%.2f exposure=%.2f\n",
                     _color[0], _color[1], _color[2], _intensity, _exposure);
    }

    // Clear dirty bits
    *dirtyBits = HdChangeTracker::Clean;
}

HdDirtyBits
HdCarWashLight::GetInitialDirtyBitsMask() const
{
    return HdLight::AllDirty;
}

void
HdCarWashLight::Finalize(HdRenderParam* renderParam)
{
    TF_UNUSED(renderParam);
    // Nothing to clean up
}

GfVec3f
HdCarWashLight::GetRadiance() const
{
    // Radiance = color * intensity * 2^exposure
    float exposureMultiplier = std::pow(2.0f, _exposure);
    return _color * _intensity * exposureMultiplier;
}

GfVec3f
HdCarWashLight::GetDirection() const
{
    // Light direction is -Z axis in light's local space
    // Transform it to world space
    GfVec4d localDir(0.0, 0.0, -1.0, 0.0);
    GfVec4d worldDir = localDir * _transform;  // USD row-vector convention
    return GfVec3f(
        static_cast<float>(worldDir[0]),
        static_cast<float>(worldDir[1]),
        static_cast<float>(worldDir[2])
    ).GetNormalized();
}

GfVec3f
HdCarWashLight::GetPosition() const
{
    // Position is the translation component of the transform
    GfVec3d translation = _transform.ExtractTranslation();
    return GfVec3f(
        static_cast<float>(translation[0]),
        static_cast<float>(translation[1]),
        static_cast<float>(translation[2])
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
