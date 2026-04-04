file(GLOB_RECURSE FILE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE FILE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

# Build as a shared library (DLL) so it can be exported via NIAPPLICATION_ENTRY.
add_library(NiApplication SHARED ${FILE_SOURCES} ${FILE_HEADERS})

# NIAPPLICATION_EXPORT is defined only when compiling the library itself;
# consumers of the DLL get NIAPPLICATION_IMPORT through the INTERFACE property.
target_compile_definitions(NiApplication
    PRIVATE   NIAPPLICATION_EXPORT
    INTERFACE NIAPPLICATION_IMPORT
)

target_include_directories(NiApplication
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(NiApplication PROPERTIES
    CXX_STANDARD          20
    CXX_STANDARD_REQUIRED ON
)

# Common dependencies
target_link_libraries(NiApplication
    PUBLIC
        NiMain
        NiMesh
        NiSystem
        NiAnimation
        SDL3::SDL3
)

# Renderer-specific dependency and compile definition
if(NI_RENDERER STREQUAL "DX9")
    target_link_libraries(NiApplication PUBLIC NiDX9Renderer)
    target_compile_definitions(NiApplication PUBLIC NI_RENDERER_DX9)
elseif(NI_RENDERER STREQUAL "DX10")
    target_link_libraries(NiApplication PUBLIC NiD3D10Renderer)
    target_compile_definitions(NiApplication PUBLIC NI_RENDERER_DX10)
elseif(NI_RENDERER STREQUAL "DX11")
    target_link_libraries(NiApplication PUBLIC ecrD3D11Renderer)
    target_compile_definitions(NiApplication PUBLIC NI_RENDERER_DX11)
else()
    message(WARNING "[NiApplication] Unknown renderer '${NI_RENDERER}', no renderer linked.")
endif()

ni_install_target(NiApplication)
