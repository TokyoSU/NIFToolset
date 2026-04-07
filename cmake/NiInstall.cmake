# ni_install_target(<TARGET_NAME> [HEADER_DIRS <dir1> <dir2> ...])
#
# Installs a Ni* library into:
#   ${CMAKE_INSTALL_PREFIX}/bin/<arch>/<config>     <- DLLs
#   ${CMAKE_INSTALL_PREFIX}/lib/<arch>/<config>     <- import libs / static libs
#   ${CMAKE_INSTALL_PREFIX}/include                 <- headers (.h, .hpp, .inl)
#
# HEADER_DIRS  Optional list of subdirectory names (relative to the target's
#              source directory) whose headers to install.  Each directory is
#              installed as a named subdirectory under include/, preserving the
#              module/ prefix required by #include <module/header.h>.
#              When omitted the entire source directory is installed as before.
#
# <arch>   = Win32 | x64  (derived from pointer size)
# <config> = Debug | Release | RelWithDebInfo | MinSizeRel  (resolved at install time)
function(ni_install_target TARGET_NAME)
    cmake_parse_arguments(_NI "" "" "HEADER_DIRS" ${ARGN})

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_arch "x64")
    else()
        set(_arch "Win32")
    endif()

    install(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION "bin/${_arch}/$<CONFIG>"
        LIBRARY DESTINATION "lib/${_arch}/$<CONFIG>"
        ARCHIVE DESTINATION "lib/${_arch}/$<CONFIG>"
    )

    get_target_property(_src_dir ${TARGET_NAME} SOURCE_DIR)

    if(_NI_HEADER_DIRS)
        foreach(_dir IN LISTS _NI_HEADER_DIRS)
            install(DIRECTORY "${_src_dir}/${_dir}"
                DESTINATION "include"
                FILES_MATCHING
                    PATTERN "*.h"
                    PATTERN "*.hpp"
                    PATTERN "*.inl"
            )
        endforeach()
    else()
        install(DIRECTORY "${_src_dir}/"
            DESTINATION "include"
            FILES_MATCHING
                PATTERN "*.h"
                PATTERN "*.hpp"
                PATTERN "*.inl"
            PATTERN "CMakeFiles" EXCLUDE
        )
    endif()
endfunction()
