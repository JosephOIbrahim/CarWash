# usdCarWash — CarWashRenderSettingsAPI USD schema library
#
# Builds the generated C++ schema classes (carWashRenderSettingsAPI, tokens)
# so the API schema is usable from C++ and registered with the USD plug
# registry. The schema *resource* plugin (plugInfo.json + generatedSchema.usda
# under resources/) is what makes the CarWash tab appear in Houdini's Render
# Settings LOP — see deploy_hdcarwash.py, which ships it alongside the delegate.
#
# NOTE: the generated sources (carWashRenderSettingsAPI.{cpp,h}, tokens.{cpp,h},
# wrap*.cpp, generatedSchema.*) were produced by usdGenSchema. They must be
# regenerated against the target Houdini's USD before a clean build (run
# usdGenSchema on schema/carWashRenderSettingsAPI.usda). The Python wrapping
# (wrap*.cpp) needs the generated module.cpp that usdGenSchema emits; until
# regeneration produces it, we build only the C++ schema classes.

set(USDCARWASH_SOURCES
    carWashRenderSettingsAPI.cpp
    tokens.cpp
)

set(USDCARWASH_HEADERS
    api.h
    carWashRenderSettingsAPI.h
    tokens.h
)

add_library(usdCarWash SHARED
    ${USDCARWASH_SOURCES}
    ${USDCARWASH_HEADERS}
)

target_include_directories(usdCarWash
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# USD/Pixar libraries come via Houdini's bundled third-party target, same as
# the hdCarWash delegate.
target_link_libraries(usdCarWash
    PUBLIC
        Houdini
        HoudiniThirdParty
)

target_compile_definitions(usdCarWash
    PRIVATE
        USDCARWASH_EXPORTS
        PXR_USE_NAMESPACES
)

# Suppress the generated-code warnings that usdGenSchema output triggers.
if(MSVC)
    set_source_files_properties(carWashRenderSettingsAPI.cpp tokens.cpp
        PROPERTIES COMPILE_FLAGS "/wd4244 /wd4267 /wd4456 /wd4100"
    )
endif()

install(
    TARGETS usdCarWash
    LIBRARY DESTINATION plugin/usd/usdCarWash/lib
    RUNTIME DESTINATION plugin/usd/usdCarWash/lib
)

install(
    FILES ${USDCARWASH_HEADERS}
    DESTINATION include/usdCarWash
)

# Ship the schema resource plugin so the CarWash tab registers.
install(
    FILES resources/plugInfo.json
    DESTINATION plugin/usd/usdCarWash
)
install(
    FILES resources/generatedSchema.usda
    DESTINATION plugin/usd/usdCarWash/resources
)