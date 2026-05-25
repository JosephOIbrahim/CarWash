# HdCarWash Plugin CMakeLists.txt

# ==============================================================================
# Source Files
# ==============================================================================

set(HDCARWASH_SOURCES
    debugCodes.cpp
    tokens.cpp
    rendererPlugin.cpp
    renderDelegate.cpp
    renderPass.cpp
    renderBuffer.cpp
    mesh.cpp
    camera.cpp
    rasterizer.cpp
    comfyClient.cpp
)

set(HDCARWASH_HEADERS
    api.h
    tokens.h
    debugCodes.h
    rendererPlugin.h
    renderDelegate.h
    renderPass.h
    renderBuffer.h
    mesh.h
    camera.h
    rasterizer.h
    comfyClient.h
)

# ==============================================================================
# Library Target
# ==============================================================================

add_library(hdCarWash SHARED
    ${HDCARWASH_SOURCES}
    ${HDCARWASH_HEADERS}
)

target_include_directories(hdCarWash
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# ==============================================================================
# Link USD/Hydra Libraries (via Houdini targets)
# ==============================================================================

# Houdini provides two interface targets:
#   - Houdini: Core Houdini libraries (OP, SOP, GU, etc.)
#   - HoudiniThirdParty: Bundled dependencies including USD/Pixar (hd, tf, sdf, etc.)
# We need both for a Hydra render delegate.
target_link_libraries(hdCarWash
    PUBLIC
        Houdini
        HoudiniThirdParty
)

# ==============================================================================
# Compile Definitions
# ==============================================================================

target_compile_definitions(hdCarWash
    PRIVATE
        HDCARWASH_EXPORTS
        $<$<BOOL:${HDCARWASH_ENABLE_LTX2}>:HDCARWASH_LTX2_ENABLED>
        $<$<BOOL:${HDCARWASH_ENABLE_COSMOS}>:HDCARWASH_COSMOS_ENABLED>
        $<$<BOOL:${HDCARWASH_ENABLE_DETERMINISM}>:HDCARWASH_DETERMINISM_ENABLED>
)

# ==============================================================================
# Installation
# ==============================================================================

install(
    TARGETS hdCarWash
    LIBRARY DESTINATION plugin/usd/hdCarWash/lib
    RUNTIME DESTINATION plugin/usd/hdCarWash/lib
)

install(
    FILES ${HDCARWASH_HEADERS}
    DESTINATION include/hdCarWash
)
