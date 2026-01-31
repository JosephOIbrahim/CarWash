// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// tokens.h — TfToken definitions for AOVs, settings, and cognitive substrate

#ifndef HD_CARWASH_TOKENS_H
#define HD_CARWASH_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"

#include "api.h"

PXR_NAMESPACE_OPEN_SCOPE

// ==============================================================================
// AOV Tokens
// ==============================================================================
// Standard AOVs plus CarWash-specific semantic buffers

#define HDCARWASH_AOV_TOKENS \
    /* Standard AOVs */ \
    (color) \
    (depth) \
    (normal) \
    (primId) \
    (instanceId) \
    (elementId) \
    /* CarWash Semantic AOVs */ \
    (carwashObjectId) \
    (carwashSemanticId) \
    (carwashMotionVector) \
    (carwashEdges) \
    (carwashStabilityMask) \
    (carwashUV)

TF_DECLARE_PUBLIC_TOKENS(HdCarWashAovTokens, HDCARWASH_API, HDCARWASH_AOV_TOKENS);

// ==============================================================================
// Render Setting Tokens
// ==============================================================================

#define HDCARWASH_SETTINGS_TOKENS \
    /* Output settings */ \
    ((outputDirectory, "carwash:outputDirectory")) \
    ((outputFormat, "carwash:outputFormat")) \
    /* Backend selection (Cosmos-ready) */ \
    ((backend, "carwash:backend")) \
    ((backendLTX2, "ltx2")) \
    ((backendFlux, "flux")) \
    ((backendCosmos, "cosmos")) \
    /* ComfyUI settings */ \
    ((comfyuiServerUrl, "carwash:comfyui:serverUrl")) \
    ((comfyuiWorkflowPath, "carwash:comfyui:workflowPath")) \
    ((comfyuiTimeoutSeconds, "carwash:comfyui:timeoutSeconds")) \
    /* Determinism settings */ \
    ((seed, "carwash:seed")) \
    ((deterministicMode, "carwash:deterministicMode")) \
    ((deterministicFast, "fast")) \
    ((deterministicBalanced, "balanced")) \
    ((deterministicMaximum, "maximum")) \
    /* Cognitive substrate settings */ \
    ((substrateEnabled, "carwash:substrate:enabled")) \
    ((substrateHistoryFrames, "carwash:substrate:historyFrames")) \
    ((substrateAutoAnnotate, "carwash:substrate:autoAnnotate")) \
    /* Quality settings */ \
    ((inferenceSteps, "carwash:inferenceSteps")) \
    ((guidanceScale, "carwash:guidanceScale"))

TF_DECLARE_PUBLIC_TOKENS(HdCarWashSettingsTokens, HDCARWASH_API, HDCARWASH_SETTINGS_TOKENS);

// ==============================================================================
// Cognitive Substrate Tokens
// ==============================================================================
// Paths and attributes for the /Cognitive prim hierarchy

#define HDCARWASH_COGNITIVE_TOKENS \
    /* Root paths */ \
    ((cognitivePath, "/Cognitive")) \
    ((styleMemoryPath, "/Cognitive/StyleMemory")) \
    ((frameHistoryPath, "/Cognitive/FrameHistory")) \
    ((semanticsPath, "/Cognitive/Semantics")) \
    ((correctionsPath, "/Cognitive/Corrections")) \
    ((temporalPath, "/Cognitive/Temporal")) \
    /* Style memory attributes */ \
    ((styleEmbedding, "style:embedding")) \
    ((styleLocked, "style:locked")) \
    ((styleSourceFrame, "style:sourceFrame")) \
    ((styleConfidence, "style:confidence")) \
    /* Semantic attributes */ \
    ((semanticRole, "semantic:role")) \
    ((semanticImportance, "semantic:importance")) \
    ((semanticDriftAllowance, "semantic:driftAllowance")) \
    ((semanticEmotionalState, "semantic:emotionalState")) \
    /* Semantic roles */ \
    ((roleProtagonist, "protagonist")) \
    ((roleAntagonist, "antagonist")) \
    ((roleSupporting, "supporting")) \
    ((roleBackground, "background")) \
    ((roleKeyProp, "key_prop")) \
    ((roleEnvironment, "environment")) \
    ((roleFX, "fx")) \
    /* Frame history attributes */ \
    ((frameNumber, "frame:number")) \
    ((frameTimestamp, "frame:timestamp")) \
    ((frameLatentEmbedding, "frame:latentEmbedding")) \
    /* Temporal attributes */ \
    ((stabilityMapPath, "stability:mapPath")) \
    ((anomalyType, "anomaly:type")) \
    ((anomalySeverity, "anomaly:severity"))

TF_DECLARE_PUBLIC_TOKENS(HdCarWashCognitiveTokens, HDCARWASH_API, HDCARWASH_COGNITIVE_TOKENS);

// ==============================================================================
// Backend Abstraction Tokens (Cosmos-Ready)
// ==============================================================================
// These tokens enable swapping backends (LTX-2, Flux, Cosmos) without code changes

#define HDCARWASH_BACKEND_TOKENS \
    /* Capabilities (what the backend provides) */ \
    ((capTemporalCoherence, "capability:temporalCoherence")) \
    ((capAudioSync, "capability:audioSync")) \
    ((capObjectPermanence, "capability:objectPermanence")) \
    ((capStyleTransfer, "capability:styleTransfer")) \
    ((capBatchInvariance, "capability:batchInvariance")) \
    /* Backend status */ \
    ((statusAvailable, "available")) \
    ((statusUnavailable, "unavailable")) \
    ((statusDegraded, "degraded"))

TF_DECLARE_PUBLIC_TOKENS(HdCarWashBackendTokens, HDCARWASH_API, HDCARWASH_BACKEND_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_CARWASH_TOKENS_H
