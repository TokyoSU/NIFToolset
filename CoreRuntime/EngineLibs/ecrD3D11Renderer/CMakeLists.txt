# Collect all source and header files in this directory
file(GLOB_RECURSE FILE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE FILE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

find_package(directxtk CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)

# Add library — type is controlled by BUILD_SHARED_LIBS (ON = DLL, OFF = static)
add_library(ecrD3D11Renderer ${FILE_SOURCES} ${FILE_HEADERS})

# Set include directories
target_include_directories(ecrD3D11Renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:include>
)

# Export macro for DLL visibility (important on Windows)
target_compile_definitions(ecrD3D11Renderer
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:EE_ECRD3D11RENDERER_EXPORT>
        EE_USE_PCH
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:EE_ECRD3D11RENDERER_IMPORT>
)

# Precompiled header
target_precompile_headers(ecrD3D11Renderer
    PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/ecrD3D11Renderer/ecrD3D11RendererPCH.h>"
)

# Link dependencies
target_link_libraries(ecrD3D11Renderer
    PUBLIC
        NiMain
        NiFloodgate
        NiMesh
        Microsoft::DirectXTK
        Microsoft::DirectXTex
        d3d11
        dxgi
        d3dcompiler
        windowscodecs
        ole32
        uuid
)

# Set C++ standard (inherit from root or override here)
set_target_properties(ecrD3D11Renderer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(ecrD3D11Renderer)
