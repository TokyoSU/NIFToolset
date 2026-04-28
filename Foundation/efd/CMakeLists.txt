# --- efd (core) ---
file(GLOB_RECURSE EFD_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/efd/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/efd/win32/*.cpp"
)
file(GLOB_RECURSE EFD_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/efd/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/efd/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/efd/*.inl"
    "${CMAKE_CURRENT_SOURCE_DIR}/efd/win32/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/efd/win32/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/efd/win32/*.inl"
)
if(NOT WIN32)
    list(FILTER EFD_SOURCES EXCLUDE REGEX ".*/efd/win32/.*")
    list(FILTER EFD_HEADERS EXCLUDE REGEX ".*/efd/win32/.*")
endif()

# --- efdLogServer ---
file(GLOB_RECURSE EFDLOGSERVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/efdLogServer/*.cpp"
)
file(GLOB_RECURSE EFDLOGSERVER_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/efdLogServer/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/efdLogServer/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/efdLogServer/*.inl"
)

# --- efdNetwork ---
file(GLOB_RECURSE EFDNETWORK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/efdNetwork/*.cpp"
)
file(GLOB_RECURSE EFDNETWORK_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/efdNetwork/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/efdNetwork/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/efdNetwork/*.inl"
)
if(NOT UNIX)
    list(FILTER EFDNETWORK_SOURCES EXCLUDE REGEX ".*/efdNetwork/linux/.*")
    list(FILTER EFDNETWORK_HEADERS EXCLUDE REGEX ".*/efdNetwork/linux/.*")
endif()

# --- efdXML ---
file(GLOB_RECURSE EFDXML_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/efdXML/*.cpp"
)
file(GLOB_RECURSE EFDXML_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/efdXML/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/efdXML/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/efdXML/*.inl"
)

# Add library — type is controlled by BUILD_SHARED_LIBS (ON = DLL, OFF = static)
add_library(efd
    ${EFD_SOURCES}          ${EFD_HEADERS}
    ${EFDLOGSERVER_SOURCES} ${EFDLOGSERVER_HEADERS}
    ${EFDNETWORK_SOURCES}   ${EFDNETWORK_HEADERS}
    ${EFDXML_SOURCES}       ${EFDXML_HEADERS}
)

# Set include directories
target_include_directories(efd
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Link Windows system libraries
if(WIN32)
    target_link_libraries(efd PUBLIC ws2_32 Rpcrt4)
endif()

# Export macro for DLL visibility (important on Windows)
target_compile_definitions(efd
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:EE_EFD_EXPORT>
        $<$<BOOL:${BUILD_SHARED_LIBS}>:EE_EFDLOGSERVICE_EXPORT>
        $<$<BOOL:${BUILD_SHARED_LIBS}>:EE_EFDNETWORK_EXPORT>
        EE_USE_PCH
    PUBLIC
        NOMINMAX
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:USEDLL>
)

# Precompiled header
target_precompile_headers(efd
    PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/efd/efdPCH.h>"
)

# Set C++ standard (inherit from root or override here)
set_target_properties(efd PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

ni_install_target(efd
    HEADER_DIRS efd efdLogServer efdNetwork efdXML
)