# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

# Auto-detect vcpkg toolchain BEFORE project() — required for CMake GUI and
# command-line builds that do not pass -DCMAKE_TOOLCHAIN_FILE explicitly.
# Priority:
#   1. Already set by user (CMake GUI / -DCMAKE_TOOLCHAIN_FILE=...)
#   2. VCPKG_ROOT environment variable
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE
            "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "vcpkg toolchain file" FORCE)
        message(STATUS "[vcpkg] Toolchain auto-detected from VCPKG_ROOT: ${CMAKE_TOOLCHAIN_FILE}")
    else()
        message(WARNING
            "[vcpkg] VCPKG_ROOT is not set and CMAKE_TOOLCHAIN_FILE was not provided. "
            "Set the VCPKG_ROOT environment variable or pass "
            "-DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake")
    endif()
endif()

project ("NIFToolset")

# Register custom Find modules (FindBASS.cmake, etc.)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Set ON for DLL (.dll), OFF for static (.lib) — default to shared
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(ENABLE_SDM_SHUTDOWN_ASSERT "Enable SDM shutdown assert" OFF)

# bgfx is the only renderer backend supported by this tree.
add_compile_definitions(NI_RENDERER_BGFX)

# Use native MSVC STL instead of stlport
add_compile_definitions(EE_USE_NATIVE_STL) # stlport is no longer supported, but this definition is still used in some places to conditionally compile code that depends on the STL.
if(ENABLE_SDM_SHUTDOWN_ASSERT)
    add_compile_definitions(ENABLE_SDM_SHUTDOWN_ASSERT)
endif()
add_compile_definitions(EE_GAMEBRYO_VERSION="4.0.0.57") # Gamebryo version used for conditional compilation (e.g. to exclude code for older versions)

# Enable multi-threaded compilation for MSVC (/MP) and
# explicit parallel job count for Ninja.
if(MSVC)
    # /MP: compile multiple source files in parallel within a single target.
    # Omit the number to let MSVC use all logical cores automatically.
    # /Zc:__cplusplus: make __cplusplus report the selected C++ language
    # standard. bx/bgfx requires this on MSVC and rejects the legacy 199711L
    # value exposed without the switch.
    # /Zc:preprocessor: enable the conforming MSVC preprocessor. bx also
    # requires this mode and rejects the legacy preprocessor implementation.
    add_compile_options(/MP /Zc:__cplusplus /Zc:preprocessor)
endif()

# Define global preprocessor definitions for all targets based on build configuration
add_compile_definitions(
    $<$<CONFIG:Debug>:NIDEBUG>
    $<$<CONFIG:Debug>:EE_EFD_CONFIG_DEBUG>
    $<$<CONFIG:Debug>:EE_CONFIG_DEBUG>
    $<$<CONFIG:Debug>:_LIB>
    $<$<CONFIG:Release>:NIRELEASE>
    $<$<CONFIG:Release>:EE_EFD_CONFIG_RELEASE>
    $<$<CONFIG:Release>:EE_CONFIG_RELEASE>
    $<$<CONFIG:Release>:_LIB>
    $<$<CONFIG:RelWithDebInfo>:NIRELEASE>
    $<$<CONFIG:RelWithDebInfo>:NIDEBUG>
    $<$<CONFIG:RelWithDebInfo>:EE_EFD_CONFIG_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:EE_EFD_CONFIG_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:EE_CONFIG_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:EE_CONFIG_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:_LIB>
    $<$<CONFIG:MinSizeRel>:NIRELEASE>
    $<$<CONFIG:MinSizeRel>:EE_EFD_CONFIG_RELEASE>
    $<$<CONFIG:MinSizeRel>:EE_CONFIG_RELEASE>
    $<$<CONFIG:MinSizeRel>:_LIB>
)

# Set default install location to <source>/install
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Default install path" FORCE)
endif()

# Include install helper function (ni_install_target)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/NiInstall.cmake")

# Include sub-projects
add_subdirectory("Foundation")
add_subdirectory("CoreRuntime")
add_subdirectory("GameFramework")
add_subdirectory("Bindings")
