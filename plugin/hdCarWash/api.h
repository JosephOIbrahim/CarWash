// Copyright 2026 Joseph O. Ibrahim. All rights reserved.
// SPDX-License-Identifier: Proprietary
//
// HdCarWash — Cognitively-Aware AI Render Delegate
// api.h — DLL export/import macros

#ifndef HD_CARWASH_API_H
#define HD_CARWASH_API_H

#include "pxr/pxr.h"
#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HDCARWASH_API
#   define HDCARWASH_API_TEMPLATE_CLASS(...)
#   define HDCARWASH_API_TEMPLATE_STRUCT(...)
#   define HDCARWASH_LOCAL
#else
#   if defined(HDCARWASH_EXPORTS)
#       define HDCARWASH_API ARCH_EXPORT
#       define HDCARWASH_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDCARWASH_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HDCARWASH_API ARCH_IMPORT
#       define HDCARWASH_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDCARWASH_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HDCARWASH_LOCAL ARCH_HIDDEN
#endif

// Version info
#define HDCARWASH_VERSION_MAJOR 0
#define HDCARWASH_VERSION_MINOR 1
#define HDCARWASH_VERSION_PATCH 0
#define HDCARWASH_VERSION_STRING "0.1.0"

// Feature flags (set by CMake)
// HDCARWASH_LTX2_ENABLED      — LTX-2 primary target
// HDCARWASH_COSMOS_ENABLED    — Cosmos WFM integration hooks
// HDCARWASH_DETERMINISM_ENABLED — Batch-invariant determinism

#endif // HD_CARWASH_API_H
