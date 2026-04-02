# Collect all source and header files in this directory
file(GLOB_RECURSE FILE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE FILE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

find_package(BASS REQUIRED)

# Add library — type is controlled by BUILD_SHARED_LIBS (ON = DLL, OFF = static)
add_library(NiAudio ${FILE_SOURCES} ${FILE_HEADERS})

# Set include directories
target_include_directories(NiAudio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Export macro for DLL visibility (important on Windows)
target_compile_definitions(NiAudio
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIAUDIO_EXPORT>
        NI_USE_PCH
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIAUDIO_IMPORT>
)

# Link dependencies
target_link_libraries(NiAudio
    PUBLIC
        NiMain
        BASS::BASS
        BASS::FX
        BASS::Mix
)

# Auto-copy bass.dll next to NiAudio output
add_custom_command(TARGET NiAudio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/Extern/bass/lib/bass.dll"
        "$<TARGET_FILE_DIR:NiAudio>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/Extern/bass/lib/bass_fx.dll"
        "$<TARGET_FILE_DIR:NiAudio>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/Extern/bass/lib/bassmix.dll"
        "$<TARGET_FILE_DIR:NiAudio>"
    COMMENT "Copying all bass dll to output directory"
)

# Set C++ standard (inherit from root or override here)
set_target_properties(NiAudio PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(NiAudio)