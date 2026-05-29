// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// light.h — Light primitive implementation

#ifndef HD_CARWASH_LIGHT_H
#define HD_CARWASH_LIGHT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdCarWashLight
///
/// Represents a light in the CarWash render delegate.
/// Supports sphere, rect, distant, dome, disk, and cylinder lights.
///
/// For CPU rasterization, lights are collected and used for
/// simple N·L shading. For AI stylization, light information
/// can be encoded into prompts or conditioning.
///
class HDCARWASH_API HdCarWashLight final : public HdLight
{
public:
    HdCarWashLight(SdfPath const& id, TfToken const& lightType);
    ~HdCarWashLight() override;

    /// Sync light parameters from scene delegate
    void Sync(
        HdSceneDelegate* sceneDelegate,
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits) override;

    /// Get initial dirty bits
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Finalize (cleanup)
    void Finalize(HdRenderParam* renderParam) override;

    // =========================================================================
    // Light Properties
    // =========================================================================

    /// Get light type (sphere, rect, distant, dome, etc.)
    const TfToken& GetLightType() const { return _lightType; }

    /// Get light color (RGB, linear)
    const GfVec3f& GetColor() const { return _color; }

    /// Get light intensity multiplier
    float GetIntensity() const { return _intensity; }

    /// Get light exposure (2^exposure multiplier)
    float GetExposure() const { return _exposure; }

    /// Get computed radiance (color * intensity * 2^exposure)
    GfVec3f GetRadiance() const;

    /// Get light transform (world space)
    const GfMatrix4d& GetTransform() const { return _transform; }

    /// Get light direction (for distant/directional lights)
    /// Returns normalized -Z axis of transform
    GfVec3f GetDirection() const;

    /// Get light position (for point/sphere/rect lights)
    GfVec3f GetPosition() const;

    /// Is this light enabled?
    bool IsEnabled() const { return _enabled; }

    // =========================================================================
    // Type-Specific Properties
    // =========================================================================

    /// Sphere light radius
    float GetRadius() const { return _radius; }

    /// Rect light dimensions
    float GetWidth() const { return _width; }
    float GetHeight() const { return _height; }

    /// Cone angle (for spot lights, in degrees)
    float GetConeAngle() const { return _coneAngle; }
    float GetConeSoftness() const { return _coneSoftness; }

    /// Dome light texture path
    const std::string& GetTextureFile() const { return _textureFile; }

private:
    TfToken _lightType;

    // Common properties
    GfVec3f _color;
    float _intensity;
    float _exposure;
    GfMatrix4d _transform;
    bool _enabled;

    // Type-specific properties
    float _radius;          // Sphere light
    float _width;           // Rect light
    float _height;          // Rect light
    float _coneAngle;       // Spot/cone angle
    float _coneSoftness;    // Spot falloff
    std::string _textureFile;  // Dome light
};

/// \struct HdCarWashLightData
///
/// Simplified light data for passing to rasterizer.
/// Avoids pulling full HdCarWashLight objects into render loop.
///
struct HDCARWASH_API HdCarWashLightData
{
    TfToken type;
    GfVec3f position;
    GfVec3f direction;
    GfVec3f radiance;  // Pre-computed color * intensity * 2^exposure
    float radius;
    bool enabled;

    HdCarWashLightData()
        : type()
        , position(0.0f)
        , direction(0.0f, 0.0f, -1.0f)
        , radiance(1.0f)
        , radius(0.0f)
        , enabled(true)
    {}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_CARWASH_LIGHT_H
