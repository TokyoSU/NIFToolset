# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.16)
project ("NIFTools")

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Get the platform we're building for and print it out.
if(WIN32)
    set(PLATFORM_SPECIFIC "Windows")
elseif(APPLE)
    set(PLATFORM_SPECIFIC "macOS")
else()
    set(PLATFORM_SPECIFIC "Linux")
endif()
message(STATUS "Building for ${PLATFORM_SPECIFIC}")

# Options when building the project:
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

# Include sub-projects.

