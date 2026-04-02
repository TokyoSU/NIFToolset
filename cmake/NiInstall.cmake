# ni_install_target(<TARGET_NAME>)
#
# Installs a Ni* library into:
#   ${CMAKE_INSTALL_PREFIX}/bin/<arch>/<config>     <- DLLs
#   ${CMAKE_INSTALL_PREFIX}/lib/<arch>/<config>     <- import libs / static libs
#   ${CMAKE_INSTALL_PREFIX}/include                 <- headers (.h, .hpp, .inl)
#
# <arch>   = Win32 | x64  (derived from pointer size)
# <config> = Debug | Release | RelWithDebInfo | MinSizeRel  (resolved at install time)
function(ni_install_target TARGET_NAME)
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
    install(DIRECTORY "${_src_dir}/"
        DESTINATION "include"
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
            PATTERN "*.inl"
        PATTERN "CMakeFiles" EXCLUDE
    )
endfunction()
