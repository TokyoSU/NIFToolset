find_package(bgfx CONFIG REQUIRED)

add_library(NiBgfxRenderer
    BgfxDataStreamFactory.cpp
    BgfxDataStreamFactory.h
    BgfxRenderer.cpp
    BgfxRenderer.h
    NiBgfxContext.cpp
    NiBgfxContext.h
    NiBgfxMath.h
    NiBgfxRendererLibType.h
)

target_include_directories(NiBgfxRenderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(NiBgfxRenderer
    PUBLIC
        NiMain
        NiMesh
        bgfx::bgfx
)

if(TARGET NiParticle)
    target_link_libraries(NiBgfxRenderer PRIVATE NiParticle)
    target_compile_definitions(NiBgfxRenderer PRIVATE
        NIBGFX_ENABLE_PARTICLE_INSTANCING=1)
endif()

target_compile_definitions(NiBgfxRenderer
    PRIVATE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIBGFXRENDERER_EXPORT>
        NI_USE_PCH
    INTERFACE
        $<$<BOOL:${BUILD_SHARED_LIBS}>:NIBGFXRENDERER_IMPORT>
)

set_target_properties(NiBgfxRenderer PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# bgfx shader source is canonical; shaderc creates backend-specific binaries.
# The bgfx CMake package exports both the host shaderc executable target and
# the include path containing bgfx_shader.sh. Using those exported values keeps
# vcpkg native and cross-triplet layouts transparent to this project.
if(NOT TARGET bgfx::shaderc)
    message(FATAL_ERROR
        "bgfx::shaderc is unavailable. Enable the bgfx[tools] vcpkg feature.")
endif()

if(NOT DEFINED BGFX_SHADER_INCLUDE_PATH OR
   NOT EXISTS "${BGFX_SHADER_INCLUDE_PATH}/bgfx_shader.sh")
    message(FATAL_ERROR
        "The bgfx package did not expose a valid BGFX_SHADER_INCLUDE_PATH.")
endif()

set(NIBGFX_SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
set(NIBGFX_SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/Shaders/bgfx")
set(NIBGFX_VARYING_DEF "${NIBGFX_SHADER_SOURCE_DIR}/varying.def.sc")

function(nibgfx_compile_shader OUTPUT_VAR SOURCE TYPE BACKEND PLATFORM PROFILE)
    get_filename_component(_name "${SOURCE}" NAME_WE)
    # Optional positional arguments: output basename, then shaderc defines.
    # This lets one source produce type-specialized variants without copying
    # hundreds of lines of shader code.
    if(ARGC GREATER 6)
        set(_name "${ARGV6}")
    endif()
    set(_define_args)
    if(ARGC GREATER 7)
        list(APPEND _define_args --define "${ARGV7}")
    endif()

    set(_out_dir "${NIBGFX_SHADER_BINARY_DIR}/${BACKEND}")
    set(_output "${_out_dir}/${_name}.bin")

    add_custom_command(
        OUTPUT "${_output}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_out_dir}"
        COMMAND bgfx::shaderc
            -f "${SOURCE}"
            -o "${_output}"
            --type "${TYPE}"
            --platform "${PLATFORM}"
            -p "${PROFILE}"
            --varyingdef "${NIBGFX_VARYING_DEF}"
            -i "${BGFX_SHADER_INCLUDE_PATH}"
            ${_define_args}
        DEPENDS "${SOURCE}" "${NIBGFX_VARYING_DEF}" bgfx::shaderc
        VERBATIM
        COMMENT "Compiling bgfx ${BACKEND} ${TYPE} shader ${_name}"
    )
    set(${OUTPUT_VAR} "${_output}" PARENT_SCOPE)
endfunction()

set(NIBGFX_COMPILED_SHADERS)
if(WIN32)
    nibgfx_compile_shader(_vs_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_basic.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_vs_inst_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_instanced.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_vs_particle_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_particle.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_vs_skin_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_skinned.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_vs_skin_shadow_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_skinned_shadow.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_vs_terrain_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_terrain.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_fs_terrain_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment dx11 windows s_5_0)
    nibgfx_compile_shader(_fs_terrain_cube_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment dx11 windows s_5_0 fs_ni_terrain_cube TERRAIN_SHADOW_CUBE=1)
    nibgfx_compile_shader(_fs_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment dx11 windows s_5_0)
    nibgfx_compile_shader(_fs_particle_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment dx11 windows s_5_0 fs_ni_particle SOFT_PARTICLES=1)
    nibgfx_compile_shader(_fs_soft_depth_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_soft_depth.sc" fragment dx11 windows s_5_0)
    nibgfx_compile_shader(_fs_soft_depth_opaque_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_soft_depth.sc" fragment dx11 windows s_5_0 fs_ni_soft_depth_opaque SOFT_DEPTH_OPAQUE=1)
    nibgfx_compile_shader(_vs_lpp_light_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_lpp_light.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_fs_lpp_g_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment dx11 windows s_5_0 fs_ni_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_lpp_f_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment dx11 windows s_5_0 fs_ni_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_lpp_light_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment dx11 windows s_5_0 fs_ni_lpp_light LPP_LIGHT=1)
    nibgfx_compile_shader(_fs_terrain_lpp_g_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment dx11 windows s_5_0 fs_ni_terrain_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_terrain_lpp_f_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment dx11 windows s_5_0 fs_ni_terrain_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_deco_lpp_g_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment dx11 windows s_5_0 fs_ni_decoration_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_deco_lpp_f_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment dx11 windows s_5_0 fs_ni_decoration_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_ext_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_extended.sc" fragment dx11 windows s_5_0)
    nibgfx_compile_shader(_fs_deco_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment dx11 windows s_5_0)
    nibgfx_compile_shader(_vs_sky_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_sky.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_fs_sky_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_sky.sc" fragment dx11 windows s_5_0)
    nibgfx_compile_shader(_vs_vsm_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_vsm_blur.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_fs_vsm_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_vsm_blur.sc" fragment dx11 windows s_5_0)
    nibgfx_compile_shader(_vs_shadow_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_shadow.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_vs_inst_shadow_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_instanced_shadow.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_fs_shadow_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_shadow.sc" fragment dx11 windows s_5_0)
    nibgfx_compile_shader(_vs_copy_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_copy.sc" vertex dx11 windows s_5_0)
    nibgfx_compile_shader(_fs_copy_dx11 "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_copy.sc" fragment dx11 windows s_5_0)
    list(APPEND NIBGFX_COMPILED_SHADERS "${_vs_dx11}" "${_vs_inst_dx11}" "${_vs_particle_dx11}" "${_vs_skin_dx11}" "${_vs_skin_shadow_dx11}" "${_vs_terrain_dx11}" "${_fs_terrain_dx11}" "${_fs_terrain_cube_dx11}" "${_fs_dx11}" "${_fs_ext_dx11}" "${_fs_deco_dx11}" "${_vs_sky_dx11}" "${_fs_sky_dx11}" "${_vs_vsm_dx11}" "${_fs_vsm_dx11}" "${_vs_shadow_dx11}" "${_vs_inst_shadow_dx11}" "${_fs_shadow_dx11}" "${_vs_copy_dx11}" "${_fs_copy_dx11}" "${_fs_particle_dx11}" "${_fs_soft_depth_dx11}" "${_fs_soft_depth_opaque_dx11}")
    list(APPEND NIBGFX_COMPILED_SHADERS "${_vs_lpp_light_dx11}" "${_fs_lpp_g_dx11}" "${_fs_lpp_f_dx11}" "${_fs_lpp_light_dx11}" "${_fs_terrain_lpp_g_dx11}" "${_fs_terrain_lpp_f_dx11}" "${_fs_deco_lpp_g_dx11}" "${_fs_deco_lpp_f_dx11}")

    # Keep the OpenGL backend usable as an explicit/fallback Windows backend.
    nibgfx_compile_shader(_vs_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_basic.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_vs_inst_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_instanced.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_vs_particle_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_particle.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_vs_skin_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_skinned.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_vs_skin_shadow_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_skinned_shadow.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_vs_terrain_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_terrain.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_fs_terrain_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment glsl windows 150)
    nibgfx_compile_shader(_fs_terrain_cube_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment glsl windows 150 fs_ni_terrain_cube TERRAIN_SHADOW_CUBE=1)
    nibgfx_compile_shader(_fs_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment glsl windows 150)
    nibgfx_compile_shader(_fs_particle_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment glsl windows 150 fs_ni_particle SOFT_PARTICLES=1)
    nibgfx_compile_shader(_fs_soft_depth_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_soft_depth.sc" fragment glsl windows 150)
    nibgfx_compile_shader(_fs_soft_depth_opaque_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_soft_depth.sc" fragment glsl windows 150 fs_ni_soft_depth_opaque SOFT_DEPTH_OPAQUE=1)
    nibgfx_compile_shader(_vs_lpp_light_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_lpp_light.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_fs_lpp_g_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment glsl windows 150 fs_ni_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_lpp_f_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment glsl windows 150 fs_ni_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_lpp_light_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment glsl windows 150 fs_ni_lpp_light LPP_LIGHT=1)
    nibgfx_compile_shader(_fs_terrain_lpp_g_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment glsl windows 150 fs_ni_terrain_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_terrain_lpp_f_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment glsl windows 150 fs_ni_terrain_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_deco_lpp_g_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment glsl windows 150 fs_ni_decoration_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_deco_lpp_f_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment glsl windows 150 fs_ni_decoration_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_ext_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_extended.sc" fragment glsl windows 150)
    nibgfx_compile_shader(_fs_deco_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment glsl windows 150)
    nibgfx_compile_shader(_vs_sky_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_sky.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_fs_sky_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_sky.sc" fragment glsl windows 150)
    nibgfx_compile_shader(_vs_vsm_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_vsm_blur.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_fs_vsm_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_vsm_blur.sc" fragment glsl windows 150)
    nibgfx_compile_shader(_vs_shadow_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_shadow.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_vs_inst_shadow_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_instanced_shadow.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_fs_shadow_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_shadow.sc" fragment glsl windows 150)
    nibgfx_compile_shader(_vs_copy_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_copy.sc" vertex glsl windows 150)
    nibgfx_compile_shader(_fs_copy_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_copy.sc" fragment glsl windows 150)
    list(APPEND NIBGFX_COMPILED_SHADERS "${_vs_gl}" "${_vs_inst_gl}" "${_vs_particle_gl}" "${_vs_skin_gl}" "${_vs_skin_shadow_gl}" "${_vs_terrain_gl}" "${_fs_terrain_gl}" "${_fs_terrain_cube_gl}" "${_fs_gl}" "${_fs_ext_gl}" "${_fs_deco_gl}" "${_vs_sky_gl}" "${_fs_sky_gl}" "${_vs_vsm_gl}" "${_fs_vsm_gl}" "${_vs_shadow_gl}" "${_vs_inst_shadow_gl}" "${_fs_shadow_gl}" "${_vs_copy_gl}" "${_fs_copy_gl}" "${_fs_particle_gl}" "${_fs_soft_depth_gl}" "${_fs_soft_depth_opaque_gl}")
    list(APPEND NIBGFX_COMPILED_SHADERS "${_vs_lpp_light_gl}" "${_fs_lpp_g_gl}" "${_fs_lpp_f_gl}" "${_fs_lpp_light_gl}" "${_fs_terrain_lpp_g_gl}" "${_fs_terrain_lpp_f_gl}" "${_fs_deco_lpp_g_gl}" "${_fs_deco_lpp_f_gl}")

    # Keep alternative Windows bgfx backends usable without changing source.
    # shaderc treats SPIR-V as a Vulkan/Linux profile regardless of the host OS.
    nibgfx_compile_shader(_vs_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_basic.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_inst_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_instanced.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_particle_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_particle.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_skin_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_skinned.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_skin_shadow_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_skinned_shadow.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_terrain_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_terrain.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_terrain_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_fs_terrain_cube_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment spirv linux spirv fs_ni_terrain_cube TERRAIN_SHADOW_CUBE=1)
    nibgfx_compile_shader(_fs_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_fs_particle_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment spirv linux spirv fs_ni_particle SOFT_PARTICLES=1)
    nibgfx_compile_shader(_fs_soft_depth_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_soft_depth.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_fs_soft_depth_opaque_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_soft_depth.sc" fragment spirv linux spirv fs_ni_soft_depth_opaque SOFT_DEPTH_OPAQUE=1)
    nibgfx_compile_shader(_vs_lpp_light_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_lpp_light.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_lpp_g_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment spirv linux spirv fs_ni_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_lpp_f_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment spirv linux spirv fs_ni_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_lpp_light_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment spirv linux spirv fs_ni_lpp_light LPP_LIGHT=1)
    nibgfx_compile_shader(_fs_terrain_lpp_g_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment spirv linux spirv fs_ni_terrain_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_terrain_lpp_f_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment spirv linux spirv fs_ni_terrain_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_deco_lpp_g_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment spirv linux spirv fs_ni_decoration_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_deco_lpp_f_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment spirv linux spirv fs_ni_decoration_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_ext_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_extended.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_fs_deco_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_vs_sky_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_sky.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_sky_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_sky.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_vs_vsm_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_vsm_blur.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_vsm_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_vsm_blur.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_vs_shadow_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_shadow.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_inst_shadow_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_instanced_shadow.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_shadow_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_shadow.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_vs_copy_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_copy.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_copy_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_copy.sc" fragment spirv linux spirv)
    list(APPEND NIBGFX_COMPILED_SHADERS "${_vs_spv}" "${_vs_inst_spv}" "${_vs_particle_spv}" "${_vs_skin_spv}" "${_vs_skin_shadow_spv}" "${_vs_terrain_spv}" "${_fs_terrain_spv}" "${_fs_terrain_cube_spv}" "${_fs_spv}" "${_fs_ext_spv}" "${_fs_deco_spv}" "${_vs_sky_spv}" "${_fs_sky_spv}" "${_vs_vsm_spv}" "${_fs_vsm_spv}" "${_vs_shadow_spv}" "${_vs_inst_shadow_spv}" "${_fs_shadow_spv}" "${_vs_copy_spv}" "${_fs_copy_spv}" "${_fs_particle_spv}" "${_fs_soft_depth_spv}" "${_fs_soft_depth_opaque_spv}")
    list(APPEND NIBGFX_COMPILED_SHADERS "${_vs_lpp_light_spv}" "${_fs_lpp_g_spv}" "${_fs_lpp_f_spv}" "${_fs_lpp_light_spv}" "${_fs_terrain_lpp_g_spv}" "${_fs_terrain_lpp_f_spv}" "${_fs_deco_lpp_g_spv}" "${_fs_deco_lpp_f_spv}")
else()
    nibgfx_compile_shader(_vs_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_basic.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_vs_inst_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_instanced.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_vs_particle_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_particle.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_vs_skin_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_skinned.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_vs_skin_shadow_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_skinned_shadow.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_vs_terrain_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_terrain.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_fs_terrain_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment glsl linux 150)
    nibgfx_compile_shader(_fs_terrain_cube_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment glsl linux 150 fs_ni_terrain_cube TERRAIN_SHADOW_CUBE=1)
    nibgfx_compile_shader(_fs_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment glsl linux 150)
    nibgfx_compile_shader(_fs_particle_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment glsl linux 150 fs_ni_particle SOFT_PARTICLES=1)
    nibgfx_compile_shader(_fs_soft_depth_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_soft_depth.sc" fragment glsl linux 150)
    nibgfx_compile_shader(_fs_soft_depth_opaque_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_soft_depth.sc" fragment glsl linux 150 fs_ni_soft_depth_opaque SOFT_DEPTH_OPAQUE=1)
    nibgfx_compile_shader(_vs_lpp_light_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_lpp_light.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_fs_lpp_g_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment glsl linux 150 fs_ni_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_lpp_f_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment glsl linux 150 fs_ni_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_lpp_light_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment glsl linux 150 fs_ni_lpp_light LPP_LIGHT=1)
    nibgfx_compile_shader(_fs_terrain_lpp_g_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment glsl linux 150 fs_ni_terrain_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_terrain_lpp_f_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment glsl linux 150 fs_ni_terrain_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_deco_lpp_g_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment glsl linux 150 fs_ni_decoration_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_deco_lpp_f_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment glsl linux 150 fs_ni_decoration_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_ext_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_extended.sc" fragment glsl linux 150)
    nibgfx_compile_shader(_fs_deco_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment glsl linux 150)
    nibgfx_compile_shader(_vs_sky_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_sky.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_fs_sky_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_sky.sc" fragment glsl linux 150)
    nibgfx_compile_shader(_vs_vsm_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_vsm_blur.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_fs_vsm_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_vsm_blur.sc" fragment glsl linux 150)
    nibgfx_compile_shader(_vs_shadow_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_shadow.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_vs_inst_shadow_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_instanced_shadow.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_fs_shadow_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_shadow.sc" fragment glsl linux 150)
    nibgfx_compile_shader(_vs_copy_gl "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_copy.sc" vertex glsl linux 150)
    nibgfx_compile_shader(_fs_copy_gl "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_copy.sc" fragment glsl linux 150)
    list(APPEND NIBGFX_COMPILED_SHADERS "${_vs_gl}" "${_vs_inst_gl}" "${_vs_particle_gl}" "${_vs_skin_gl}" "${_vs_skin_shadow_gl}" "${_vs_terrain_gl}" "${_fs_terrain_gl}" "${_fs_terrain_cube_gl}" "${_fs_gl}" "${_fs_ext_gl}" "${_fs_deco_gl}" "${_vs_sky_gl}" "${_fs_sky_gl}" "${_vs_vsm_gl}" "${_fs_vsm_gl}" "${_vs_shadow_gl}" "${_vs_inst_shadow_gl}" "${_fs_shadow_gl}" "${_vs_copy_gl}" "${_fs_copy_gl}" "${_fs_particle_gl}" "${_fs_soft_depth_gl}" "${_fs_soft_depth_opaque_gl}")
    list(APPEND NIBGFX_COMPILED_SHADERS "${_vs_lpp_light_gl}" "${_fs_lpp_g_gl}" "${_fs_lpp_f_gl}" "${_fs_lpp_light_gl}" "${_fs_terrain_lpp_g_gl}" "${_fs_terrain_lpp_f_gl}" "${_fs_deco_lpp_g_gl}" "${_fs_deco_lpp_f_gl}")

    nibgfx_compile_shader(_vs_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_basic.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_inst_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_instanced.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_particle_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_particle.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_skin_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_skinned.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_skin_shadow_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_skinned_shadow.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_terrain_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_terrain.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_terrain_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_fs_terrain_cube_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment spirv linux spirv fs_ni_terrain_cube TERRAIN_SHADOW_CUBE=1)
    nibgfx_compile_shader(_fs_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_fs_particle_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment spirv linux spirv fs_ni_particle SOFT_PARTICLES=1)
    nibgfx_compile_shader(_fs_soft_depth_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_soft_depth.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_fs_soft_depth_opaque_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_soft_depth.sc" fragment spirv linux spirv fs_ni_soft_depth_opaque SOFT_DEPTH_OPAQUE=1)
    nibgfx_compile_shader(_vs_lpp_light_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_lpp_light.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_lpp_g_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment spirv linux spirv fs_ni_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_lpp_f_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment spirv linux spirv fs_ni_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_lpp_light_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_basic.sc" fragment spirv linux spirv fs_ni_lpp_light LPP_LIGHT=1)
    nibgfx_compile_shader(_fs_terrain_lpp_g_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment spirv linux spirv fs_ni_terrain_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_terrain_lpp_f_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_terrain.sc" fragment spirv linux spirv fs_ni_terrain_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_deco_lpp_g_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment spirv linux spirv fs_ni_decoration_lpp_gbuffer LPP_GBUFFER=1)
    nibgfx_compile_shader(_fs_deco_lpp_f_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment spirv linux spirv fs_ni_decoration_lpp_final LPP_FINAL=1)
    nibgfx_compile_shader(_fs_ext_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_extended.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_fs_deco_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_decoration.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_vs_sky_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_sky.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_sky_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_sky.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_vs_vsm_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_vsm_blur.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_vsm_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_vsm_blur.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_vs_shadow_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_shadow.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_vs_inst_shadow_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_instanced_shadow.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_shadow_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_shadow.sc" fragment spirv linux spirv)
    nibgfx_compile_shader(_vs_copy_spv "${NIBGFX_SHADER_SOURCE_DIR}/vs_ni_copy.sc" vertex spirv linux spirv)
    nibgfx_compile_shader(_fs_copy_spv "${NIBGFX_SHADER_SOURCE_DIR}/fs_ni_copy.sc" fragment spirv linux spirv)
    list(APPEND NIBGFX_COMPILED_SHADERS "${_vs_spv}" "${_vs_inst_spv}" "${_vs_particle_spv}" "${_vs_skin_spv}" "${_vs_skin_shadow_spv}" "${_vs_terrain_spv}" "${_fs_terrain_spv}" "${_fs_terrain_cube_spv}" "${_fs_spv}" "${_fs_ext_spv}" "${_fs_deco_spv}" "${_vs_sky_spv}" "${_fs_sky_spv}" "${_vs_vsm_spv}" "${_fs_vsm_spv}" "${_vs_shadow_spv}" "${_vs_inst_shadow_spv}" "${_fs_shadow_spv}" "${_vs_copy_spv}" "${_fs_copy_spv}" "${_fs_particle_spv}" "${_fs_soft_depth_spv}" "${_fs_soft_depth_opaque_spv}")
    list(APPEND NIBGFX_COMPILED_SHADERS "${_vs_lpp_light_spv}" "${_fs_lpp_g_spv}" "${_fs_lpp_f_spv}" "${_fs_lpp_light_spv}" "${_fs_terrain_lpp_g_spv}" "${_fs_terrain_lpp_f_spv}" "${_fs_deco_lpp_g_spv}" "${_fs_deco_lpp_f_spv}")
endif()

add_custom_target(NiBgfxShaders DEPENDS ${NIBGFX_COMPILED_SHADERS})
add_dependencies(NiBgfxRenderer NiBgfxShaders)

target_compile_definitions(NiBgfxRenderer PRIVATE
    NIBGFX_DEFAULT_SHADER_ROOT="${NIBGFX_SHADER_BINARY_DIR}"
)

install(DIRECTORY "${NIBGFX_SHADER_BINARY_DIR}/"
    DESTINATION "bin/Shaders/bgfx"
    OPTIONAL
)
install(DIRECTORY "${NIBGFX_SHADER_SOURCE_DIR}/"
    DESTINATION "share/NIFToolset/Shaders/bgfx-source"
    FILES_MATCHING PATTERN "*.sc"
)

ni_install_target(NiBgfxRenderer)
