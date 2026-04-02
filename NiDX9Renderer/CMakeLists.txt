# Collect all source and header files in this directory
file(GLOB_RECURSE FILE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE FILE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

find_package(dxsdk-d3dx CONFIG REQUIRED)

# Add library — type is controlled by BUILD_SHARED_LIBS (ON = DLL, OFF = static)
add_library(NiDX9Renderer ${FILE_SOURCES} ${FILE_HEADERS})

# Set include directories
target_include_directories(NiDX9Renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Export macro for DLL visibility (important on Windows)
target_compile_definitions(NiDX9Renderer
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIDX9RENDERER_EXPORT>
        NI_USE_PCH
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIDX9RENDERER_IMPORT>
)

# Link dependencies
target_link_libraries(NiDX9Renderer
    PUBLIC
        NiMain
        NiFloodgate
        NiMesh
        Microsoft::D3DX9
)

# Set C++ standard (inherit from root or override here)
set_target_properties(NiDX9Renderer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(NiDX9Renderer)