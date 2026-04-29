# Collect all source and header files in this directory
file(GLOB_RECURSE FILE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE FILE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

# Add library — type is controlled by BUILD_SHARED_LIBS (ON = DLL, OFF = static)
add_library(NiSettingsDialog ${FILE_SOURCES} ${FILE_HEADERS})

# Set include directories
target_include_directories(NiSettingsDialog
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CoreRuntime/EngineLibs/NiDX9Renderer>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CoreRuntime/EngineLibs/NiD3D10Renderer>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CoreRuntime/EngineLibs>
        $<INSTALL_INTERFACE:include>
)

# Export macro for DLL visibility (important on Windows)
target_compile_definitions(NiSettingsDialog
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NISETTINGSDIALOG_EXPORT>
        NI_USE_PCH
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NISETTINGSDIALOG_IMPORT>
)

# Link dependencies
target_link_libraries(NiSettingsDialog
    PUBLIC
        NiMain
)

# NiRendererSettings.h unconditionally includes NiDX9SystemDesc.h and NiD3D10SystemDesc.h.
# The active renderer is linked (PUBLIC) so its include dirs — including dxsdk-d3dx — propagate
# transitively. For non-active renderers we only add include paths; their SDK headers
# (DXGI, D3D10, D3D11) come from the Windows SDK and need no extra setup.
if(NI_RENDERER STREQUAL "DX9")
    target_link_libraries(NiSettingsDialog PUBLIC NiDX9Renderer)
elseif(NI_RENDERER STREQUAL "DX10")
    target_link_libraries(NiSettingsDialog PUBLIC NiD3D10Renderer)
    # NiDX9SystemDesc.h is still included: provide DX9 headers + dxsdk-d3dx path.
    find_package(dxsdk-d3dx CONFIG REQUIRED)
    get_target_property(_d3dx9_inc Microsoft::D3DX9 INTERFACE_INCLUDE_DIRECTORIES)
    get_filename_component(_d3dx9_parent_inc "${_d3dx9_inc}" DIRECTORY)
    target_include_directories(NiSettingsDialog PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CoreRuntime/EngineLibs/NiDX9Renderer>
        $<BUILD_INTERFACE:${_d3dx9_parent_inc}>
    )
elseif(NI_RENDERER STREQUAL "DX11")
    target_link_libraries(NiSettingsDialog PUBLIC ecrD3D11Renderer)
    find_package(dxsdk-d3dx CONFIG REQUIRED)
    get_target_property(_d3dx9_inc Microsoft::D3DX9 INTERFACE_INCLUDE_DIRECTORIES)
    get_filename_component(_d3dx9_parent_inc "${_d3dx9_inc}" DIRECTORY)
    target_include_directories(NiSettingsDialog PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/CoreRuntime/EngineLibs/NiDX9Renderer>
        $<BUILD_INTERFACE:${_d3dx9_parent_inc}>
    )
endif()

# Set C++ standard (inherit from root or override here)
set_target_properties(NiSettingsDialog PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(NiSettingsDialog)