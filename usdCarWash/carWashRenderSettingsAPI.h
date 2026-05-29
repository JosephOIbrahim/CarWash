//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDCARWASH_GENERATED_CARWASHRENDERSETTINGSAPI_H
#define USDCARWASH_GENERATED_CARWASHRENDERSETTINGSAPI_H

/// \file usdCarWash/carWashRenderSettingsAPI.h

#include "pxr/pxr.h"
#include "./api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "./tokens.h"

#include "pxr/imaging/hd/renderDelegate.h"


#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// CARWASHRENDERSETTINGSAPI                                                   //
// -------------------------------------------------------------------------- //

/// \class UsdCarWashCarWashRenderSettingsAPI
///
/// CarWash AI renderer settings API schema.
/// 
/// This API schema provides settings for the CarWash Hydra render delegate,
/// which uses AI-based image generation with depth and normal conditioning.
/// 
/// The schema enables the CarWash tab to appear in Houdini's Render Settings LOP.
/// 
///
class UsdCarWashCarWashRenderSettingsAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdCarWashCarWashRenderSettingsAPI on UsdPrim \p prim .
    /// Equivalent to UsdCarWashCarWashRenderSettingsAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdCarWashCarWashRenderSettingsAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdCarWashCarWashRenderSettingsAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdCarWashCarWashRenderSettingsAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdCarWashCarWashRenderSettingsAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDCARWASH_API
    virtual ~UsdCarWashCarWashRenderSettingsAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDCARWASH_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdCarWashCarWashRenderSettingsAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdCarWashCarWashRenderSettingsAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDCARWASH_API
    static UsdCarWashCarWashRenderSettingsAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Returns true if this <b>single-apply</b> API schema can be applied to 
    /// the given \p prim. If this schema can not be a applied to the prim, 
    /// this returns false and, if provided, populates \p whyNot with the 
    /// reason it can not be applied.
    /// 
    /// Note that if CanApply returns false, that does not necessarily imply
    /// that calling Apply will fail. Callers are expected to call CanApply
    /// before calling Apply if they want to ensure that it is valid to 
    /// apply a schema.
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDCARWASH_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "CarWashRenderSettingsAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdCarWashCarWashRenderSettingsAPI object is returned upon success. 
    /// An invalid (or empty) UsdCarWashCarWashRenderSettingsAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDCARWASH_API
    static UsdCarWashCarWashRenderSettingsAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDCARWASH_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDCARWASH_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDCARWASH_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // CARWASHPROMPT 
    // --------------------------------------------------------------------- //
    /// The text prompt that guides AI image generation.
    /// This prompt is combined with scene context for consistent results.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `string carwash:prompt = "photorealistic 3D render, high quality, detailed"` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    USDCARWASH_API
    UsdAttribute GetCarwashPromptAttr() const;

    /// See GetCarwashPromptAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashPromptAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHNEGATIVEPROMPT 
    // --------------------------------------------------------------------- //
    /// Text describing what to avoid in the generated image.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `string carwash:negativePrompt = "blurry, low quality, artifacts"` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    USDCARWASH_API
    UsdAttribute GetCarwashNegativePromptAttr() const;

    /// See GetCarwashNegativePromptAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashNegativePromptAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHSEED 
    // --------------------------------------------------------------------- //
    /// Random seed for deterministic generation.
    /// Same seed + same inputs = same output (batch-invariant).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int carwash:seed = 42` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    USDCARWASH_API
    UsdAttribute GetCarwashSeedAttr() const;

    /// See GetCarwashSeedAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashSeedAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHSTEPS 
    // --------------------------------------------------------------------- //
    /// Number of diffusion steps. More steps = higher quality, longer time.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int carwash:steps = 20` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    USDCARWASH_API
    UsdAttribute GetCarwashStepsAttr() const;

    /// See GetCarwashStepsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashStepsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHCFG 
    // --------------------------------------------------------------------- //
    /// Classifier-free guidance scale. Higher = more prompt adherence.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float carwash:cfg = 7` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDCARWASH_API
    UsdAttribute GetCarwashCfgAttr() const;

    /// See GetCarwashCfgAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashCfgAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHDENOISE 
    // --------------------------------------------------------------------- //
    /// Denoising strength (0-1). Lower preserves more of the input.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float carwash:denoise = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDCARWASH_API
    UsdAttribute GetCarwashDenoiseAttr() const;

    /// See GetCarwashDenoiseAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashDenoiseAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHUSEDEPTH 
    // --------------------------------------------------------------------- //
    /// Enable depth-conditioned generation from rendered depth pass.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool carwash:useDepth = 1` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDCARWASH_API
    UsdAttribute GetCarwashUseDepthAttr() const;

    /// See GetCarwashUseDepthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashUseDepthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHDEPTHSTRENGTH 
    // --------------------------------------------------------------------- //
    /// How strongly depth conditioning influences the output (0-1).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float carwash:depthStrength = 0.8` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDCARWASH_API
    UsdAttribute GetCarwashDepthStrengthAttr() const;

    /// See GetCarwashDepthStrengthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashDepthStrengthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHUSENORMAL 
    // --------------------------------------------------------------------- //
    /// Enable normal-conditioned generation from rendered normal pass.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool carwash:useNormal = 1` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDCARWASH_API
    UsdAttribute GetCarwashUseNormalAttr() const;

    /// See GetCarwashUseNormalAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashUseNormalAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHNORMALSTRENGTH 
    // --------------------------------------------------------------------- //
    /// How strongly normal conditioning influences the output (0-1).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float carwash:normalStrength = 0.6` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDCARWASH_API
    UsdAttribute GetCarwashNormalStrengthAttr() const;

    /// See GetCarwashNormalStrengthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashNormalStrengthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHUSEBEAUTY 
    // --------------------------------------------------------------------- //
    /// Use the beauty pass as additional conditioning (img2img style).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool carwash:useBeauty = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDCARWASH_API
    UsdAttribute GetCarwashUseBeautyAttr() const;

    /// See GetCarwashUseBeautyAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashUseBeautyAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHBEAUTYSTRENGTH 
    // --------------------------------------------------------------------- //
    /// Strength of beauty pass influence when enabled.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float carwash:beautyStrength = 0.3` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDCARWASH_API
    UsdAttribute GetCarwashBeautyStrengthAttr() const;

    /// See GetCarwashBeautyStrengthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashBeautyStrengthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHUSESTYLEMEMORY 
    // --------------------------------------------------------------------- //
    /// Maintain style consistency across frames using cognitive substrate.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool carwash:useStyleMemory = 1` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDCARWASH_API
    UsdAttribute GetCarwashUseStyleMemoryAttr() const;

    /// See GetCarwashUseStyleMemoryAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashUseStyleMemoryAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHSTYLEID 
    // --------------------------------------------------------------------- //
    /// Identifier for style memory cache. Same ID shares style across shots.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `string carwash:styleId = "default"` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    USDCARWASH_API
    UsdAttribute GetCarwashStyleIdAttr() const;

    /// See GetCarwashStyleIdAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashStyleIdAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHSTYLESTRENGTH 
    // --------------------------------------------------------------------- //
    /// How strongly to apply cached style (0-2).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float carwash:styleStrength = 1` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDCARWASH_API
    UsdAttribute GetCarwashStyleStrengthAttr() const;

    /// See GetCarwashStyleStrengthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashStyleStrengthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHTEMPORALBLEND 
    // --------------------------------------------------------------------- //
    /// Blend with previous frames for animation coherence.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool carwash:temporalBlend = 1` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDCARWASH_API
    UsdAttribute GetCarwashTemporalBlendAttr() const;

    /// See GetCarwashTemporalBlendAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashTemporalBlendAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHTEMPORALSTRENGTH 
    // --------------------------------------------------------------------- //
    /// Blend strength with previous frame (0-1).
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `float carwash:temporalStrength = 0.3` |
    /// | C++ Type | float |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Float |
    USDCARWASH_API
    UsdAttribute GetCarwashTemporalStrengthAttr() const;

    /// See GetCarwashTemporalStrengthAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashTemporalStrengthAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHUSEMOTIONVECTORS 
    // --------------------------------------------------------------------- //
    /// Use motion vectors for motion-compensated temporal blending.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool carwash:useMotionVectors = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDCARWASH_API
    UsdAttribute GetCarwashUseMotionVectorsAttr() const;

    /// See GetCarwashUseMotionVectorsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashUseMotionVectorsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHSTRICTDETERMINISM 
    // --------------------------------------------------------------------- //
    /// Enable strict batch-invariant mode.
    /// Requires batch_invariant_ops for full determinism.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool carwash:strictDeterminism = 1` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDCARWASH_API
    UsdAttribute GetCarwashStrictDeterminismAttr() const;

    /// See GetCarwashStrictDeterminismAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashStrictDeterminismAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHVERIFYOUTPUT 
    // --------------------------------------------------------------------- //
    /// Compute and log output hash for determinism verification.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool carwash:verifyOutput = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    USDCARWASH_API
    UsdAttribute GetCarwashVerifyOutputAttr() const;

    /// See GetCarwashVerifyOutputAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashVerifyOutputAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHCOMFYHOST 
    // --------------------------------------------------------------------- //
    /// Hostname of the ComfyUI server.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `string carwash:comfyHost = "localhost"` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    USDCARWASH_API
    UsdAttribute GetCarwashComfyHostAttr() const;

    /// See GetCarwashComfyHostAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashComfyHostAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHCOMFYPORT 
    // --------------------------------------------------------------------- //
    /// Port of the ComfyUI server.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int carwash:comfyPort = 8188` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    USDCARWASH_API
    UsdAttribute GetCarwashComfyPortAttr() const;

    /// See GetCarwashComfyPortAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashComfyPortAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHWORKFLOWPATH 
    // --------------------------------------------------------------------- //
    /// Optional path to custom ComfyUI workflow JSON.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `string carwash:workflowPath = ""` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    USDCARWASH_API
    UsdAttribute GetCarwashWorkflowPathAttr() const;

    /// See GetCarwashWorkflowPathAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashWorkflowPathAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // CARWASHTIMEOUT 
    // --------------------------------------------------------------------- //
    /// Maximum time to wait for ComfyUI response.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `int carwash:timeout = 120` |
    /// | C++ Type | int |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Int |
    USDCARWASH_API
    UsdAttribute GetCarwashTimeoutAttr() const;

    /// See GetCarwashTimeoutAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDCARWASH_API
    UsdAttribute CreateCarwashTimeoutAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
