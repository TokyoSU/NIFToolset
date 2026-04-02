if(NI_BUILD_SYSTEM)
    add_subdirectory("NiSystem")
endif()

if(NI_BUILD_MAIN)
    add_subdirectory("NiMain")
endif()

if(NI_BUILD_FLOODGATE)
    add_subdirectory("NiFloodgate")
endif()

if(NI_BUILD_MESH)
    add_subdirectory("NiMesh")
endif()

if(NI_BUILD_ANIMATION)
    add_subdirectory("NiAnimation")
endif()

if(NI_BUILD_COLLISION)
    add_subdirectory("NiCollision")
endif()

if(NI_BUILD_PARTICLE)
    add_subdirectory("NiParticle")
endif()

if(NI_BUILD_PORTAL)
    add_subdirectory("NiPortal")
endif()

if(NI_BUILD_TERRAIN)
    add_subdirectory("NiTerrain")
endif()

if(NI_BUILD_ENVIRONMENT)
    add_subdirectory("NiEnvironment")
endif()

if(NI_BUILD_DECORATION)
    add_subdirectory("NiDecoration")
endif()

if(NI_BUILD_LIGHTPREPASS)
    add_subdirectory("NiLightPrePass")
endif()

if(NI_BUILD_RENDERER)
    if(NI_RENDERER STREQUAL "DX9")
        add_subdirectory("NiDX9Renderer")
    elseif(NI_RENDERER STREQUAL "DX10")
        message(WARNING "[NIFToolset] DX10 renderer is not yet implemented. Skipping NiDX10Renderer.")
    elseif(NI_RENDERER STREQUAL "DX11")
        message(WARNING "[NIFToolset] DX11 renderer is not yet implemented. Skipping NiDX11Renderer.")
    else()
        message(FATAL_ERROR "[NIFToolset] Unknown renderer: '${NI_RENDERER}'. Valid values: DX9, DX10, DX11.")
    endif()
endif()