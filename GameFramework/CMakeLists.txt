# --- Platform service selection ---
# Set NI_PLATFORM_SERVICE_SDL3=ON to use SDL3 for windowing/events instead of Win32.
option(NI_PLATFORM_SERVICE_SDL3 "Use SDL3 as the platform windowing/event service (default: Win32)" OFF)

if(NI_PLATFORM_SERVICE_SDL3)
    find_package(SDL3 CONFIG REQUIRED)
    message(STATUS "[ecr] Platform service: SDL3")
else()
    message(STATUS "[ecr] Platform service: Win32")
endif()

# --- ecr (core) ---
file(GLOB_RECURSE ECR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/ecr/*.cpp"
)
file(GLOB_RECURSE ECR_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/ecr/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecr/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ecr/*.inl"
)

# Add library — type is controlled by BUILD_SHARED_LIBS (ON = DLL, OFF = static)
add_library(ecr
    ${ECR_SOURCES}          ${ECR_HEADERS}
)

# Set include directories
target_include_directories(ecr
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CoreRuntime/EngineLibs/NiD3D10Renderer>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CoreRuntime/EngineLibs/ecrD3D11Renderer>
        $<INSTALL_INTERFACE:include>
)

# Export macro for DLL visibility (important on Windows)
target_compile_definitions(ecr
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:EE_ECR_EXPORT>
        EE_USE_PCH
    PUBLIC
        NOMINMAX
        $<$<BOOL:${NI_PLATFORM_SERVICE_SDL3}>:EE_PLATFORM_SERVICE_SDL3>
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:USEDLL>
)

# Link dependencies
target_link_libraries(ecr
    PUBLIC
        efd
        NiMesh
        NiAnimation
        NiParticle
        NiCollision
)

# Link SDL3 only when it is the selected platform service
if(NI_PLATFORM_SERVICE_SDL3)
    target_link_libraries(ecr PUBLIC SDL3::SDL3)
endif()

# Renderer-specific link (only one renderer target exists per build)
if(NI_RENDERER STREQUAL "DX9")
    target_link_libraries(ecr PUBLIC NiDX9Renderer)
elseif(NI_RENDERER STREQUAL "DX10")
    target_link_libraries(ecr PUBLIC NiD3D10Renderer)
elseif(NI_RENDERER STREQUAL "DX11")
    target_link_libraries(ecr PUBLIC ecrD3D11Renderer)
endif()

# Precompiled header (ecrPCH.h already pulls in egfPCH.h)
target_precompile_headers(ecr
    PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/ecr/ecrPCH.h>"
)

# Set C++ standard (inherit from root or override here)
set_target_properties(ecr PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(ecr
    HEADER_DIRS ecr
)
