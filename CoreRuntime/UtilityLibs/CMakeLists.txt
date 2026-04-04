
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

# NiInput is only required for the DX9 renderer,
# so only force disable it if that renderer is selected.
# Otherwise, disable it to avoid unnecessary compilation and dependencies.
if(NI_BUILD_INPUT)
    if(NI_RENDERER STREQUAL "DX9")
        add_subdirectory("NiInput")
    else()
        message(WARNING "[NIFToolset] NiInput is only required for the DX9 renderer and will be skipped for renderer '${NI_RENDERER}'.")
    endif()
endif()

# SDL3 is only required for NiApplication, so only force disable it if NiApplication is enabled.
if(NI_BUILD_SDL3)
    find_package(SDL3 CONFIG REQUIRED)
    if(NI_RENDERER STREQUAL "DX9")
        message(WARNING "[NIFToolset] SDL3 + DX9 support for NiApplication.")
    elseif(NI_RENDERER STREQUAL "DX10")
        message(WARNING "[NIFToolset] SDL3 + DX10 support for NiApplication.")
    elseif(NI_RENDERER STREQUAL "DX11")
        message(WARNING "[NIFToolset] SDL3 + DX11 support for NiApplication.")
    else()
        message(WARNING "[NIFToolset] SDL3 + unknown renderer support for NiApplication, unknown renderer '${NI_RENDERER}' !")
    endif()
    add_subdirectory("NiApplication")
else()
    message(STATUS "[NIFToolset] SDL3 support disabled, NiApplication skipped.")
endif()
