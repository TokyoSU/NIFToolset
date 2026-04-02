# VCPKG_ROOT is resolved from the environment variable.
# Override it per-user via CMakeUserPresets.json "environment" field.
if(NOT DEFINED ENV{VCPKG_ROOT} OR "$ENV{VCPKG_ROOT}" STREQUAL "")
    message(WARNING
        "VCPKG_ROOT environment variable is not set.\n"
        "vcpkg integration will be skipped.\n"
        "Set it in CMakeUserPresets.json using the 'environment' field."
    )
    return()
endif()

set(_vcpkg_toolchain "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

if(NOT EXISTS "${_vcpkg_toolchain}")
    message(FATAL_ERROR
        "vcpkg toolchain not found at: ${_vcpkg_toolchain}\n"
        "Check that VCPKG_ROOT points to a valid vcpkg installation."
    )
endif()

include("${_vcpkg_toolchain}")