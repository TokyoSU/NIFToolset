$input a_position, a_normal, a_tangent, a_bitangent, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_texcoord4, a_texcoord5, a_texcoord6, a_texcoord7, a_color0
$output v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

void main()
{
    vec4 worldPos = mul(u_model[0], vec4(a_position, 1.0));
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_worldPos = worldPos.xyz;
    v_worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
    v_worldTangent = normalize(mul(u_model[0], vec4(a_tangent, 0.0)).xyz);
    v_worldBitangent = normalize(mul(u_model[0], vec4(a_bitangent, 0.0)).xyz);
    v_color0 = a_color0;
    v_texcoord01 = vec4(a_texcoord0, a_texcoord1);
    v_texcoord23 = vec4(a_texcoord2, a_texcoord3);
    v_texcoord45 = vec4(a_texcoord4, a_texcoord5);
    v_texcoord67 = vec4(a_texcoord6, a_texcoord7);
}
