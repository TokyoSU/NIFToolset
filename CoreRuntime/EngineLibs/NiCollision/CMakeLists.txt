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
add_library(NiCollision ${FILE_SOURCES} ${FILE_HEADERS})

# Set include directories
target_include_directories(NiCollision
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Export macro for DLL visibility (important on Windows)
target_compile_definitions(NiCollision
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NICOLLISION_EXPORT>
        NI_USE_PCH
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NICOLLISION_IMPORT>
)

# Link dependencies
target_link_libraries(NiCollision
    PUBLIC
        NiMain
        NiFloodgate
        NiMesh
)

# Set C++ standard (inherit from root or override here)
set_target_properties(NiCollision PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(NiCollision)