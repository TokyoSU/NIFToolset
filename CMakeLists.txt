# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)

# Enable multi-threaded compilation for MSVC (/MP) and
# explicit parallel job count for Ninja.
if(MSVC)
    # /MP: compile multiple source files in parallel within a single target.
    # Omit the number to let MSVC use all logical cores automatically.
    add_compile_options(/MP)
endif()

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
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Set ON for DLL (.dll), OFF for static (.lib) — default to shared
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Define global preprocessor definitions for all targets based on build configuration
add_compile_definitions(
    $<$<CONFIG:Debug>:NIDEBUG>
    $<$<CONFIG:Debug>:_LIB>
    $<$<CONFIG:Release>:NIRELEASE>
    $<$<CONFIG:Release>:_LIB>
    $<$<CONFIG:RelWithDebInfo>:NIRELEASE>
    $<$<CONFIG:RelWithDebInfo>:NIDEBUG>
    $<$<CONFIG:RelWithDebInfo>:_LIB>
    $<$<CONFIG:MinSizeRel>:NIRELEASE>
    $<$<CONFIG:MinSizeRel>:_LIB>
)

# Set default install location to <source>/install
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Default install path" FORCE)
endif()

# Include install helper function (ni_install_target)
include("${CMAKE_SOURCE_DIR}/cmake/NiInstall.cmake")

# Include sub-projects.
add_subdirectory ("NiAnimation")
add_subdirectory ("NiAudio")
add_subdirectory ("NiCollision")
add_subdirectory ("NiCursor")
add_subdirectory ("NiDX9Renderer")
add_subdirectory ("NiD3DXEffectShaderLib")
add_subdirectory ("NiEntity")
add_subdirectory ("NiFloodgate")
add_subdirectory ("NiFont")
add_subdirectory ("NiInput")
add_subdirectory ("NiMain")
add_subdirectory ("NiMesh")
add_subdirectory ("NiParticle")
add_subdirectory ("NiPortal")
add_subdirectory ("NiSystem")
add_subdirectory ("NiTerrain")
