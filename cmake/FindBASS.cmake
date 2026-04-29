if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_BASS_ARCH "x64")
else()
    set(_BASS_ARCH "Win32")
endif()

get_filename_component(_BASS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../Extern/bass" ABSOLUTE)
set(_BASS_LIB_DIR "${_BASS_ROOT}/lib/${_BASS_ARCH}")

find_path(BASS_INCLUDE_DIR
    NAMES bass.h
    HINTS "${_BASS_ROOT}/include"
    NO_DEFAULT_PATH
)

find_library(BASS_LIBRARY
    NAMES bass
    HINTS "${_BASS_LIB_DIR}"
    NO_DEFAULT_PATH
)

find_library(BASS_FX_LIBRARY
    NAMES bass_fx
    HINTS "${_BASS_LIB_DIR}"
    NO_DEFAULT_PATH
)

find_library(BASS_MIX_LIBRARY
    NAMES bassmix
    HINTS "${_BASS_LIB_DIR}"
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BASS DEFAULT_MSG
    BASS_LIBRARY
    BASS_FX_LIBRARY
    BASS_MIX_LIBRARY
    BASS_INCLUDE_DIR
)

mark_as_advanced(BASS_INCLUDE_DIR BASS_LIBRARY BASS_FX_LIBRARY BASS_MIX_LIBRARY)

if(BASS_FOUND AND NOT TARGET BASS::BASS)
    add_library(BASS::BASS SHARED IMPORTED)
    set_target_properties(BASS::BASS PROPERTIES
        IMPORTED_LOCATION             "${_BASS_LIB_DIR}/bass.dll"
        IMPORTED_IMPLIB               "${BASS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${BASS_INCLUDE_DIR}"
    )
endif()

if(BASS_FOUND AND NOT TARGET BASS::FX)
    add_library(BASS::FX SHARED IMPORTED)
    set_target_properties(BASS::FX PROPERTIES
        IMPORTED_LOCATION             "${_BASS_LIB_DIR}/bass_fx.dll"
        IMPORTED_IMPLIB               "${BASS_FX_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${BASS_INCLUDE_DIR}"
    )
endif()

if(BASS_FOUND AND NOT TARGET BASS::Mix)
    add_library(BASS::Mix SHARED IMPORTED)
    set_target_properties(BASS::Mix PROPERTIES
        IMPORTED_LOCATION             "${_BASS_LIB_DIR}/bassmix.dll"
        IMPORTED_IMPLIB               "${BASS_MIX_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${BASS_INCLUDE_DIR}"
    )
endif()