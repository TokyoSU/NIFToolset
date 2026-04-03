# Collect all source and header files in this directory
file(GLOB_RECURSE FILE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE FILE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

find_package(tinyxml CONFIG REQUIRED)

# Add library — type is controlled by BUILD_SHARED_LIBS (ON = DLL, OFF = static)
add_library(NiEntity ${FILE_SOURCES} ${FILE_HEADERS})

# Set include directories
target_include_directories(NiEntity
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Export macro for DLL visibility (important on Windows)
target_compile_definitions(NiEntity
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIENTITY_EXPORT>
        NI_USE_PCH
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIENTITY_IMPORT>
)

# Link dependencies
target_link_libraries(NiEntity
    PUBLIC
        NiMain
        NiFloodgate
        NiMesh
        NiAnimation
        unofficial-tinyxml::unofficial-tinyxml
)

if(NI_BUILD_RENDERER)
    if(NI_RENDERER STREQUAL "DX9")
        target_link_libraries(NiEntity PUBLIC NiDX9Renderer)
    elseif(NI_RENDERER STREQUAL "DX10")
        target_link_libraries(NiEntity PUBLIC NiD3D10Renderer)
    elseif(NI_RENDERER STREQUAL "DX11")
        target_link_libraries(NiEntity PUBLIC ecrD3D11Renderer)
    endif()
endif()

# Set C++ standard (inherit from root or override here)
set_target_properties(NiEntity PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(NiEntity)