//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "./carWashRenderSettingsAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdCarWashCarWashRenderSettingsAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdCarWashCarWashRenderSettingsAPI::~UsdCarWashCarWashRenderSettingsAPI()
{
}

/* static */
UsdCarWashCarWashRenderSettingsAPI
UsdCarWashCarWashRenderSettingsAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdCarWashCarWashRenderSettingsAPI();
    }
    return UsdCarWashCarWashRenderSettingsAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdCarWashCarWashRenderSettingsAPI::_GetSchemaKind() const
{
    return UsdCarWashCarWashRenderSettingsAPI::schemaKind;
}

/* static */
bool
UsdCarWashCarWashRenderSettingsAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdCarWashCarWashRenderSettingsAPI>(whyNot);
}

/* static */
UsdCarWashCarWashRenderSettingsAPI
UsdCarWashCarWashRenderSettingsAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdCarWashCarWashRenderSettingsAPI>()) {
        return UsdCarWashCarWashRenderSettingsAPI(prim);
    }
    return UsdCarWashCarWashRenderSettingsAPI();
}

/* static */
const TfType &
UsdCarWashCarWashRenderSettingsAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdCarWashCarWashRenderSettingsAPI>();
    return tfType;
}

/* static */
bool 
UsdCarWashCarWashRenderSettingsAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdCarWashCarWashRenderSettingsAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashPromptAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashPrompt);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashPromptAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashPrompt,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashNegativePromptAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashNegativePrompt);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashNegativePromptAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashNegativePrompt,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashSeedAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashSeed);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashSeedAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashSeed,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashStepsAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashSteps);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashStepsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashSteps,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashCfgAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashCfg);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashCfgAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashCfg,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashDenoiseAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashDenoise);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashDenoiseAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashDenoise,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashUseDepthAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashUseDepth);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashUseDepthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashUseDepth,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashDepthStrengthAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashDepthStrength);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashDepthStrengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashDepthStrength,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashUseNormalAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashUseNormal);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashUseNormalAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashUseNormal,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashNormalStrengthAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashNormalStrength);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashNormalStrengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashNormalStrength,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashUseBeautyAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashUseBeauty);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashUseBeautyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashUseBeauty,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashBeautyStrengthAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashBeautyStrength);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashBeautyStrengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashBeautyStrength,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashUseStyleMemoryAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashUseStyleMemory);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashUseStyleMemoryAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashUseStyleMemory,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashStyleIdAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashStyleId);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashStyleIdAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashStyleId,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashStyleStrengthAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashStyleStrength);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashStyleStrengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashStyleStrength,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashTemporalBlendAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashTemporalBlend);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashTemporalBlendAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashTemporalBlend,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashTemporalStrengthAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashTemporalStrength);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashTemporalStrengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashTemporalStrength,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashUseMotionVectorsAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashUseMotionVectors);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashUseMotionVectorsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashUseMotionVectors,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashStrictDeterminismAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashStrictDeterminism);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashStrictDeterminismAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashStrictDeterminism,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashVerifyOutputAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashVerifyOutput);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashVerifyOutputAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashVerifyOutput,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashComfyHostAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashComfyHost);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashComfyHostAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashComfyHost,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashComfyPortAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashComfyPort);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashComfyPortAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashComfyPort,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashWorkflowPathAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashWorkflowPath);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashWorkflowPathAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashWorkflowPath,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::GetCarwashTimeoutAttr() const
{
    return GetPrim().GetAttribute(UsdCarWashTokens->carwashTimeout);
}

UsdAttribute
UsdCarWashCarWashRenderSettingsAPI::CreateCarwashTimeoutAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdCarWashTokens->carwashTimeout,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdCarWashCarWashRenderSettingsAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdCarWashTokens->carwashPrompt,
        UsdCarWashTokens->carwashNegativePrompt,
        UsdCarWashTokens->carwashSeed,
        UsdCarWashTokens->carwashSteps,
        UsdCarWashTokens->carwashCfg,
        UsdCarWashTokens->carwashDenoise,
        UsdCarWashTokens->carwashUseDepth,
        UsdCarWashTokens->carwashDepthStrength,
        UsdCarWashTokens->carwashUseNormal,
        UsdCarWashTokens->carwashNormalStrength,
        UsdCarWashTokens->carwashUseBeauty,
        UsdCarWashTokens->carwashBeautyStrength,
        UsdCarWashTokens->carwashUseStyleMemory,
        UsdCarWashTokens->carwashStyleId,
        UsdCarWashTokens->carwashStyleStrength,
        UsdCarWashTokens->carwashTemporalBlend,
        UsdCarWashTokens->carwashTemporalStrength,
        UsdCarWashTokens->carwashUseMotionVectors,
        UsdCarWashTokens->carwashStrictDeterminism,
        UsdCarWashTokens->carwashVerifyOutput,
        UsdCarWashTokens->carwashComfyHost,
        UsdCarWashTokens->carwashComfyPort,
        UsdCarWashTokens->carwashWorkflowPath,
        UsdCarWashTokens->carwashTimeout,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
