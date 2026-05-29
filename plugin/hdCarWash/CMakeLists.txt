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
)

# Phase 1+ sources (added incrementally)
if(FALSE)  # Enable as phases complete
    list(APPEND HDCARWASH_SOURCES
        mesh.cpp
        camera.cpp
        instancer.cpp
        rasterizer.cpp
    )
    list(APPEND HDCARWASH_HEADERS
        mesh.h
        camera.h
        instancer.h
        rasterizer.h
    )
endif()

# Phase 2+ sources (ComfyUI integration). comfyClient.{cpp,h} exist on disk and
# are hardened, but are NOT yet wired into the render pass (_Execute), so they are
# excluded from the default build. Flip HDCARWASH_ENABLE_COMFYUI to ON once the
# client is instantiated in renderDelegate/renderPass. comfyClient.cpp uses Winsock,
# so it also requires linking ws2_32 (handled below when enabled).
option(HDCARWASH_ENABLE_COMFYUI "Compile the ComfyUI client (comfyClient.cpp)" OFF)
if(HDCARWASH_ENABLE_COMFYUI)
    list(APPEND HDCARWASH_SOURCES comfyClient.cpp)
    list(APPEND HDCARWASH_HEADERS comfyClient.h)
endif()

# Phase 3+ sources (Cognitive Engine)
if(FALSE)
    list(APPEND HDCARWASH_SOURCES
        cognitiveEngine.cpp
        substrateReader.cpp
        substrateWriter.cpp
        contextAssembler.cpp
    )
    list(APPEND HDCARWASH_HEADERS
        cognitiveEngine.h
        substrateReader.h
        substrateWriter.h
        contextAssembler.h
    )
endif()

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

# comfyClient.cpp uses Winsock (WSAStartup/socket/select/recv); link ws2_32 only
# when the ComfyUI client is compiled in.
if(HDCARWASH_ENABLE_COMFYUI AND WIN32)
    target_link_libraries(hdCarWash PRIVATE ws2_32)
endif()

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

# The dll lands at usd/hdCarWash/lib/hdCarWash.dll so that plugInfo.json's
# LibraryPath ("lib/hdCarWash.dll", resolved relative to the manifest dir
# usd/hdCarWash) points at the actual binary. On Windows a SHARED library is
# a RUNTIME artifact (.dll); LIBRARY covers the import .lib on other configs.
install(
    TARGETS hdCarWash
    RUNTIME DESTINATION usd/hdCarWash/lib
    LIBRARY DESTINATION usd/hdCarWash/lib
)

install(
    FILES ${HDCARWASH_HEADERS}
    DESTINATION include/hdCarWash
)
