//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "./tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdCarWashTokens->name.GetString(); });

void wrapUsdCarWashTokens()
{
    pxr_boost::python::class_<UsdCarWashTokensType, pxr_boost::python::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, carwash);
    _ADD_TOKEN(cls, carwashBeautyStrength);
    _ADD_TOKEN(cls, carwashCfg);
    _ADD_TOKEN(cls, carwashComfyHost);
    _ADD_TOKEN(cls, carwashComfyPort);
    _ADD_TOKEN(cls, carwashDenoise);
    _ADD_TOKEN(cls, carwashDepthStrength);
    _ADD_TOKEN(cls, carwashNegativePrompt);
    _ADD_TOKEN(cls, carwashNormalStrength);
    _ADD_TOKEN(cls, carwashPrompt);
    _ADD_TOKEN(cls, carwashSeed);
    _ADD_TOKEN(cls, carwashSteps);
    _ADD_TOKEN(cls, carwashStrictDeterminism);
    _ADD_TOKEN(cls, carwashStyleId);
    _ADD_TOKEN(cls, carwashStyleStrength);
    _ADD_TOKEN(cls, carwashTemporalBlend);
    _ADD_TOKEN(cls, carwashTemporalStrength);
    _ADD_TOKEN(cls, carwashTimeout);
    _ADD_TOKEN(cls, carwashUseBeauty);
    _ADD_TOKEN(cls, carwashUseDepth);
    _ADD_TOKEN(cls, carwashUseMotionVectors);
    _ADD_TOKEN(cls, carwashUseNormal);
    _ADD_TOKEN(cls, carwashUseStyleMemory);
    _ADD_TOKEN(cls, carwashVerifyOutput);
    _ADD_TOKEN(cls, carwashWorkflowPath);
    _ADD_TOKEN(cls, CarWashRenderSettingsAPI);
}
