file(GLOB_RECURSE FILE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE FILE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.inl"
)

add_library(NiSettingsDialog ${FILE_SOURCES} ${FILE_HEADERS})

target_include_directories(NiSettingsDialog
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_definitions(NiSettingsDialog
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NISETTINGSDIALOG_EXPORT>
        NI_USE_PCH
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NISETTINGSDIALOG_IMPORT>
)

target_precompile_headers(NiSettingsDialog
    PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/NiSettingsDialogPCH.h>"
)

target_link_libraries(NiSettingsDialog
    PUBLIC
        NiMain
)

set_target_properties(NiSettingsDialog PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(NiSettingsDialog)
