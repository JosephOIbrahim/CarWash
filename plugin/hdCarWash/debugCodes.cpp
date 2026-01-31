// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// debugCodes.cpp — TF_DEBUG symbol registration

#include "debugCodes.h"

#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HD_CARWASH,
        "HdCarWash render delegate debugging");
}

PXR_NAMESPACE_CLOSE_SCOPE
