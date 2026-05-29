//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDCARWASH_TOKENS_H
#define USDCARWASH_TOKENS_H

/// \file usdCarWash/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "./api.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdCarWashTokensType
///
/// \link UsdCarWashTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdCarWashTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdCarWashTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdCarWashTokens->carwash);
/// \endcode
struct UsdCarWashTokensType {
    USDCARWASH_API UsdCarWashTokensType();
    /// \brief "carwash"
    /// 
    /// CarWash attribute namespace
    const TfToken carwash;
    /// \brief "carwash:beautyStrength"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashBeautyStrength;
    /// \brief "carwash:cfg"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashCfg;
    /// \brief "carwash:comfyHost"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashComfyHost;
    /// \brief "carwash:comfyPort"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashComfyPort;
    /// \brief "carwash:denoise"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashDenoise;
    /// \brief "carwash:depthStrength"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashDepthStrength;
    /// \brief "carwash:negativePrompt"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashNegativePrompt;
    /// \brief "carwash:normalStrength"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashNormalStrength;
    /// \brief "carwash:prompt"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashPrompt;
    /// \brief "carwash:seed"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashSeed;
    /// \brief "carwash:steps"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashSteps;
    /// \brief "carwash:strictDeterminism"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashStrictDeterminism;
    /// \brief "carwash:styleId"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashStyleId;
    /// \brief "carwash:styleStrength"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashStyleStrength;
    /// \brief "carwash:temporalBlend"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashTemporalBlend;
    /// \brief "carwash:temporalStrength"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashTemporalStrength;
    /// \brief "carwash:timeout"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashTimeout;
    /// \brief "carwash:useBeauty"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashUseBeauty;
    /// \brief "carwash:useDepth"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashUseDepth;
    /// \brief "carwash:useMotionVectors"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashUseMotionVectors;
    /// \brief "carwash:useNormal"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashUseNormal;
    /// \brief "carwash:useStyleMemory"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashUseStyleMemory;
    /// \brief "carwash:verifyOutput"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashVerifyOutput;
    /// \brief "carwash:workflowPath"
    /// 
    /// UsdCarWashCarWashRenderSettingsAPI
    const TfToken carwashWorkflowPath;
    /// \brief "CarWashRenderSettingsAPI"
    /// 
    /// Schema identifer and family for UsdCarWashCarWashRenderSettingsAPI
    const TfToken CarWashRenderSettingsAPI;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdCarWashTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdCarWashTokensType
extern USDCARWASH_API TfStaticData<UsdCarWashTokensType> UsdCarWashTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
