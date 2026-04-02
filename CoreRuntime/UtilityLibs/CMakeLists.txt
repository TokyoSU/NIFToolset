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

if(NI_BUILD_INPUT)
    add_subdirectory("NiInput")
endif()