# ── Library build options ────────────────────────────────────────────────────
# External libs configuration
option(NI_BUILD_SDL3 "Build with SDL3 support" ON) # Used for NiApplication, if not found, dont enable NiApplication !
# Engine libs
option(NI_BUILD_SYSTEM    "Build NiSystem"    ON)
option(NI_BUILD_MAIN      "Build NiMain"      ON)
option(NI_BUILD_FLOODGATE "Build NiFloodgate" ON)
option(NI_BUILD_MESH      "Build NiMesh"      ON)
option(NI_BUILD_ANIMATION "Build NiAnimation" ON)
option(NI_BUILD_COLLISION "Build NiCollision" ON)
option(NI_BUILD_PARTICLE  "Build NiParticle"  ON)
option(NI_BUILD_PORTAL    "Build NiPortal"    ON)
option(NI_BUILD_TERRAIN      "Build NiTerrain"         ON)
option(NI_BUILD_ENVIRONMENT  "Build NiEnvironment"     ON)
option(NI_BUILD_DECORATION   "Build NiDecoration"      ON)
option(NI_BUILD_LIGHTPREPASS "Build NiLightPrePass"    ON)
option(NI_BUILD_RENDERER     "Build the renderer library" ON)
# Utility libs
option(NI_BUILD_AUDIO     "Build NiAudio"     ON)
option(NI_BUILD_CURSOR    "Build NiCursor"    ON)
option(NI_BUILD_ENTITY    "Build NiEntity"    ON)
option(NI_BUILD_FONT      "Build NiFont"      ON)
option(NI_BUILD_INPUT     "Build NiInput"     ON)
# Shader libs
option(NI_BUILD_D3DXEFFECT_SHADERLIB "Build NiD3DXEffectShaderLib" ON)

# ── Dependency auto-activation ────────────────────────────────────────────────
# If DEPENDENT is ON and a required option is OFF, force it ON.
# Each call lists ALL transitive deps so a single pass is always sufficient.
function(ni_force_deps DEPENDENT)
    if(${DEPENDENT})
        foreach(_dep IN LISTS ARGN)
            if(NOT ${_dep})
                message(STATUS "[NIFToolset] ${DEPENDENT} requires ${_dep} — enabling automatically.")
                set(${_dep} ON CACHE BOOL "" FORCE)
            endif()
        endforeach()
    endif()
endfunction()

ni_force_deps(NI_BUILD_MAIN       NI_BUILD_SYSTEM)
ni_force_deps(NI_BUILD_FLOODGATE  NI_BUILD_SYSTEM NI_BUILD_MAIN)
ni_force_deps(NI_BUILD_MESH       NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE)
ni_force_deps(NI_BUILD_ANIMATION  NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH)
ni_force_deps(NI_BUILD_COLLISION  NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH)
ni_force_deps(NI_BUILD_RENDERER   NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH)
ni_force_deps(NI_BUILD_PARTICLE   NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH NI_BUILD_ANIMATION)
ni_force_deps(NI_BUILD_PORTAL     NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH)
ni_force_deps(NI_BUILD_ENTITY     NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH NI_BUILD_ANIMATION NI_BUILD_RENDERER)
ni_force_deps(NI_BUILD_TERRAIN      NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH NI_BUILD_ANIMATION NI_BUILD_COLLISION NI_BUILD_RENDERER NI_BUILD_ENTITY)
ni_force_deps(NI_BUILD_ENVIRONMENT  NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH NI_BUILD_RENDERER NI_BUILD_ANIMATION)
ni_force_deps(NI_BUILD_DECORATION   NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH)
ni_force_deps(NI_BUILD_LIGHTPREPASS NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH NI_BUILD_ANIMATION NI_BUILD_COLLISION NI_BUILD_RENDERER NI_BUILD_ENTITY NI_BUILD_TERRAIN NI_BUILD_DECORATION)
ni_force_deps(NI_BUILD_AUDIO        NI_BUILD_SYSTEM NI_BUILD_MAIN)
ni_force_deps(NI_BUILD_CURSOR     NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH)
ni_force_deps(NI_BUILD_FONT       NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH)
ni_force_deps(NI_BUILD_D3DXEFFECT_SHADERLIB NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH NI_BUILD_RENDERER)
ni_force_deps(NI_BUILD_INPUT NI_BUILD_SYSTEM NI_BUILD_MAIN NI_BUILD_FLOODGATE NI_BUILD_MESH NI_BUILD_RENDERER)

add_subdirectory("EngineLibs")
add_subdirectory("UtilityLibs")
add_subdirectory("ShaderLibs")