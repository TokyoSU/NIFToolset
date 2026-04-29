file(GLOB_RECURSE FILE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE FILE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

# Add library — type is controlled by BUILD_SHARED_LIBS (ON = DLL, OFF = static)
add_library(NiApplication ${FILE_SOURCES} ${FILE_HEADERS})

target_compile_definitions(NiApplication
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIAPPLICATION_EXPORT>
        $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:EE_EFD_NO_IMPORT>
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIAPPLICATION_IMPORT>
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
        NiEnvironment
        NiParticle
        SDL3::SDL3
)

target_link_libraries(NiApplication
    PUBLIC
        NiDX9Renderer
        NiD3D10Renderer
        ecrD3D11Renderer
)

target_compile_definitions(NiApplication PRIVATE
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:EE_ECRD3D11RENDERER_NO_IMPORT>)

ni_install_target(NiApplication)
