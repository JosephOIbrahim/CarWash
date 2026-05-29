//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDCARWASH_API_H
#define USDCARWASH_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDCARWASH_API
#   define USDCARWASH_API_TEMPLATE_CLASS(...)
#   define USDCARWASH_API_TEMPLATE_STRUCT(...)
#   define USDCARWASH_LOCAL
#else
#   if defined(USDCARWASH_EXPORTS)
#       define USDCARWASH_API ARCH_EXPORT
#       define USDCARWASH_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDCARWASH_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDCARWASH_API ARCH_IMPORT
#       define USDCARWASH_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDCARWASH_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDCARWASH_LOCAL ARCH_HIDDEN
#endif

#endif
