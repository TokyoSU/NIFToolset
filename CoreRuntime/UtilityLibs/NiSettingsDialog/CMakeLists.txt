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
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/CoreRuntime/EngineLibs/NiDX9Renderer>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/CoreRuntime/EngineLibs/NiD3D10Renderer>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/CoreRuntime/EngineLibs>
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

find_package(dxsdk-d3dx CONFIG REQUIRED)
get_target_property(_d3dx9_inc Microsoft::D3DX9 INTERFACE_INCLUDE_DIRECTORIES)
get_filename_component(_d3dx9_parent_inc "${_d3dx9_inc}" DIRECTORY)

# Link dependencies
target_link_libraries(NiSettingsDialog
    PUBLIC
        NiMain
        NiDX9Renderer
        NiD3D10Renderer
        ecrD3D11Renderer
)

target_include_directories(NiSettingsDialog PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/CoreRuntime/EngineLibs/NiDX9Renderer>
    $<BUILD_INTERFACE:${_d3dx9_parent_inc}>
)

# Set C++ standard (inherit from root or override here)
set_target_properties(NiSettingsDialog PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(NiSettingsDialog)