$input a_position
$output v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

void main()
{
    vec4 localPos = vec4(a_position, 1.0);
    vec4 worldPos = mul(u_model[0], localPos);
    vec4 clipPos = mul(u_modelViewProj, localPos);
    gl_Position = clipPos;

    v_color0 = vec4(1.0, 1.0, 1.0, 1.0);
    v_texcoord01 = vec4(0.0, 0.0, 0.0, 0.0);
    v_texcoord23 = vec4(0.0, 0.0, 0.0, 0.0);
    v_texcoord45 = vec4(0.0, 0.0, 0.0, 0.0);
    // The light pass needs the pre-raster projected position to reproduce
    // NiLightPrePass::LPPScreenUV independent of render-target dimensions.
    v_texcoord67 = clipPos;
    v_worldPos = worldPos.xyz;
    v_worldNormal = vec3(0.0, 0.0, 1.0);
    v_worldTangent = vec3(1.0, 0.0, 0.0);
    v_worldBitangent = vec3(0.0, 1.0, 0.0);
}
