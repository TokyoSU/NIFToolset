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
    add_subdirectory("NiBgfxRenderer")
endif()
