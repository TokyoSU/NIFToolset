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
