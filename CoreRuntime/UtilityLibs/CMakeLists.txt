
if(NI_BUILD_AUDIO)
    add_subdirectory("NiAudio")
endif()

if(NI_BUILD_CURSOR)
    add_subdirectory("NiCursor")
endif()

if(NI_BUILD_ENTITY)
    add_subdirectory("NiEntity")
endif()

if(NI_BUILD_FONT)
    add_subdirectory("NiFont")
endif()

add_subdirectory("NiSettingsDialog")

if(NI_BUILD_INPUT)
    add_subdirectory("NiInput")
endif()

if(NI_BUILD_SDL3)
    find_package(SDL3 CONFIG REQUIRED)
    add_subdirectory("NiApplication")
else()
    message(STATUS "[NIFToolset] SDL3 support disabled, NiApplication skipped.")
endif()
