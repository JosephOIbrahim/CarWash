//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "./carWashRenderSettingsAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateCarwashPromptAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashPromptAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashNegativePromptAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashNegativePromptAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashSeedAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashSeedAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashStepsAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashStepsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashCfgAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashCfgAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashDenoiseAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashDenoiseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashUseDepthAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashUseDepthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashDepthStrengthAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashDepthStrengthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashUseNormalAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashUseNormalAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashNormalStrengthAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashNormalStrengthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashUseBeautyAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashUseBeautyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashBeautyStrengthAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashBeautyStrengthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashUseStyleMemoryAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashUseStyleMemoryAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashStyleIdAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashStyleIdAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashStyleStrengthAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashStyleStrengthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashTemporalBlendAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashTemporalBlendAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashTemporalStrengthAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashTemporalStrengthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashUseMotionVectorsAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashUseMotionVectorsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashStrictDeterminismAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashStrictDeterminismAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashVerifyOutputAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashVerifyOutputAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashComfyHostAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashComfyHostAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashComfyPortAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashComfyPortAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashWorkflowPathAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashWorkflowPathAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateCarwashTimeoutAttr(UsdCarWashCarWashRenderSettingsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCarwashTimeoutAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}

static std::string
_Repr(const UsdCarWashCarWashRenderSettingsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdCarWash.CarWashRenderSettingsAPI(%s)",
        primRepr.c_str());
}

struct UsdCarWashCarWashRenderSettingsAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdCarWashCarWashRenderSettingsAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdCarWashCarWashRenderSettingsAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdCarWashCarWashRenderSettingsAPI::CanApply(prim, &whyNot);
    return UsdCarWashCarWashRenderSettingsAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdCarWashCarWashRenderSettingsAPI()
{
    typedef UsdCarWashCarWashRenderSettingsAPI This;

    UsdCarWashCarWashRenderSettingsAPI_CanApplyResult::Wrap<UsdCarWashCarWashRenderSettingsAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("CarWashRenderSettingsAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("CanApply", &_WrapCanApply, (arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetCarwashPromptAttr",
             &This::GetCarwashPromptAttr)
        .def("CreateCarwashPromptAttr",
             &_CreateCarwashPromptAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashNegativePromptAttr",
             &This::GetCarwashNegativePromptAttr)
        .def("CreateCarwashNegativePromptAttr",
             &_CreateCarwashNegativePromptAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashSeedAttr",
             &This::GetCarwashSeedAttr)
        .def("CreateCarwashSeedAttr",
             &_CreateCarwashSeedAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashStepsAttr",
             &This::GetCarwashStepsAttr)
        .def("CreateCarwashStepsAttr",
             &_CreateCarwashStepsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashCfgAttr",
             &This::GetCarwashCfgAttr)
        .def("CreateCarwashCfgAttr",
             &_CreateCarwashCfgAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashDenoiseAttr",
             &This::GetCarwashDenoiseAttr)
        .def("CreateCarwashDenoiseAttr",
             &_CreateCarwashDenoiseAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashUseDepthAttr",
             &This::GetCarwashUseDepthAttr)
        .def("CreateCarwashUseDepthAttr",
             &_CreateCarwashUseDepthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashDepthStrengthAttr",
             &This::GetCarwashDepthStrengthAttr)
        .def("CreateCarwashDepthStrengthAttr",
             &_CreateCarwashDepthStrengthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashUseNormalAttr",
             &This::GetCarwashUseNormalAttr)
        .def("CreateCarwashUseNormalAttr",
             &_CreateCarwashUseNormalAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashNormalStrengthAttr",
             &This::GetCarwashNormalStrengthAttr)
        .def("CreateCarwashNormalStrengthAttr",
             &_CreateCarwashNormalStrengthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashUseBeautyAttr",
             &This::GetCarwashUseBeautyAttr)
        .def("CreateCarwashUseBeautyAttr",
             &_CreateCarwashUseBeautyAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashBeautyStrengthAttr",
             &This::GetCarwashBeautyStrengthAttr)
        .def("CreateCarwashBeautyStrengthAttr",
             &_CreateCarwashBeautyStrengthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashUseStyleMemoryAttr",
             &This::GetCarwashUseStyleMemoryAttr)
        .def("CreateCarwashUseStyleMemoryAttr",
             &_CreateCarwashUseStyleMemoryAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashStyleIdAttr",
             &This::GetCarwashStyleIdAttr)
        .def("CreateCarwashStyleIdAttr",
             &_CreateCarwashStyleIdAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashStyleStrengthAttr",
             &This::GetCarwashStyleStrengthAttr)
        .def("CreateCarwashStyleStrengthAttr",
             &_CreateCarwashStyleStrengthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashTemporalBlendAttr",
             &This::GetCarwashTemporalBlendAttr)
        .def("CreateCarwashTemporalBlendAttr",
             &_CreateCarwashTemporalBlendAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashTemporalStrengthAttr",
             &This::GetCarwashTemporalStrengthAttr)
        .def("CreateCarwashTemporalStrengthAttr",
             &_CreateCarwashTemporalStrengthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashUseMotionVectorsAttr",
             &This::GetCarwashUseMotionVectorsAttr)
        .def("CreateCarwashUseMotionVectorsAttr",
             &_CreateCarwashUseMotionVectorsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashStrictDeterminismAttr",
             &This::GetCarwashStrictDeterminismAttr)
        .def("CreateCarwashStrictDeterminismAttr",
             &_CreateCarwashStrictDeterminismAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashVerifyOutputAttr",
             &This::GetCarwashVerifyOutputAttr)
        .def("CreateCarwashVerifyOutputAttr",
             &_CreateCarwashVerifyOutputAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashComfyHostAttr",
             &This::GetCarwashComfyHostAttr)
        .def("CreateCarwashComfyHostAttr",
             &_CreateCarwashComfyHostAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashComfyPortAttr",
             &This::GetCarwashComfyPortAttr)
        .def("CreateCarwashComfyPortAttr",
             &_CreateCarwashComfyPortAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashWorkflowPathAttr",
             &This::GetCarwashWorkflowPathAttr)
        .def("CreateCarwashWorkflowPathAttr",
             &_CreateCarwashWorkflowPathAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCarwashTimeoutAttr",
             &This::GetCarwashTimeoutAttr)
        .def("CreateCarwashTimeoutAttr",
             &_CreateCarwashTimeoutAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("__repr__", ::_Repr)
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace {

WRAP_CUSTOM {
}

}
