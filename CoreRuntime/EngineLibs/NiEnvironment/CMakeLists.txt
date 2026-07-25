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
add_library(NiEnvironment ${FILE_SOURCES} ${FILE_HEADERS})

# Set include directories
target_include_directories(NiEnvironment
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Export macro for DLL visibility (important on Windows)
target_compile_definitions(NiEnvironment
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIENVIRONMENT_EXPORT>
        NI_USE_PCH
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIENVIRONMENT_IMPORT>
)

# Precompiled header
target_precompile_headers(NiEnvironment
    PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/NiEnvironmentPCH.h>"
)

# Link dependencies
target_link_libraries(NiEnvironment
    PUBLIC
        NiMain
        NiFloodgate
        NiMesh
        NiAnimation
)

# Set C++ standard (inherit from root or override here)
set_target_properties(NiEnvironment PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(NiEnvironment)