//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "./tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdCarWashTokensType::UsdCarWashTokensType() :
    carwash("carwash", TfToken::Immortal),
    carwashBeautyStrength("carwash:beautyStrength", TfToken::Immortal),
    carwashCfg("carwash:cfg", TfToken::Immortal),
    carwashComfyHost("carwash:comfyHost", TfToken::Immortal),
    carwashComfyPort("carwash:comfyPort", TfToken::Immortal),
    carwashDenoise("carwash:denoise", TfToken::Immortal),
    carwashDepthStrength("carwash:depthStrength", TfToken::Immortal),
    carwashNegativePrompt("carwash:negativePrompt", TfToken::Immortal),
    carwashNormalStrength("carwash:normalStrength", TfToken::Immortal),
    carwashPrompt("carwash:prompt", TfToken::Immortal),
    carwashSeed("carwash:seed", TfToken::Immortal),
    carwashSteps("carwash:steps", TfToken::Immortal),
    carwashStrictDeterminism("carwash:strictDeterminism", TfToken::Immortal),
    carwashStyleId("carwash:styleId", TfToken::Immortal),
    carwashStyleStrength("carwash:styleStrength", TfToken::Immortal),
    carwashTemporalBlend("carwash:temporalBlend", TfToken::Immortal),
    carwashTemporalStrength("carwash:temporalStrength", TfToken::Immortal),
    carwashTimeout("carwash:timeout", TfToken::Immortal),
    carwashUseBeauty("carwash:useBeauty", TfToken::Immortal),
    carwashUseDepth("carwash:useDepth", TfToken::Immortal),
    carwashUseMotionVectors("carwash:useMotionVectors", TfToken::Immortal),
    carwashUseNormal("carwash:useNormal", TfToken::Immortal),
    carwashUseStyleMemory("carwash:useStyleMemory", TfToken::Immortal),
    carwashVerifyOutput("carwash:verifyOutput", TfToken::Immortal),
    carwashWorkflowPath("carwash:workflowPath", TfToken::Immortal),
    CarWashRenderSettingsAPI("CarWashRenderSettingsAPI", TfToken::Immortal),
    allTokens({
        carwash,
        carwashBeautyStrength,
        carwashCfg,
        carwashComfyHost,
        carwashComfyPort,
        carwashDenoise,
        carwashDepthStrength,
        carwashNegativePrompt,
        carwashNormalStrength,
        carwashPrompt,
        carwashSeed,
        carwashSteps,
        carwashStrictDeterminism,
        carwashStyleId,
        carwashStyleStrength,
        carwashTemporalBlend,
        carwashTemporalStrength,
        carwashTimeout,
        carwashUseBeauty,
        carwashUseDepth,
        carwashUseMotionVectors,
        carwashUseNormal,
        carwashUseStyleMemory,
        carwashVerifyOutput,
        carwashWorkflowPath,
        CarWashRenderSettingsAPI
    })
{
}

TfStaticData<UsdCarWashTokensType> UsdCarWashTokens;

PXR_NAMESPACE_CLOSE_SCOPE
