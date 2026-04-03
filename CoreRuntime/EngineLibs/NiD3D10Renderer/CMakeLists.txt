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

# Expose the parent of dxsdk-d3dx's include directory so that consumers
# using <dxsdk-d3dx/D3DX10.h> can resolve it without the CMake target.
get_target_property(_d3dx10_inc Microsoft::D3DX10 INTERFACE_INCLUDE_DIRECTORIES)
get_filename_component(_d3dx10_parent_inc "${_d3dx10_inc}" DIRECTORY)

# Add library — type is controlled by BUILD_SHARED_LIBS (ON = DLL, OFF = static)
add_library(NiD3D10Renderer ${FILE_SOURCES} ${FILE_HEADERS})

# Set include directories
target_include_directories(NiD3D10Renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<BUILD_INTERFACE:${_d3dx10_parent_inc}>
        $<INSTALL_INTERFACE:include>
)

# Export macro for DLL visibility (important on Windows)
target_compile_definitions(NiD3D10Renderer
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NID3D10RENDERER_EXPORT>
        NI_USE_PCH
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NID3D10RENDERER_IMPORT>
)

# Link dependencies
target_link_libraries(NiD3D10Renderer
    PUBLIC
        NiMain
        NiFloodgate
        NiMesh
        d3dcompiler
        Microsoft::D3DX10
)

# Set C++ standard (inherit from root or override here)
set_target_properties(NiD3D10Renderer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(NiD3D10Renderer)