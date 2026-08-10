$input a_position, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_texcoord4, a_texcoord5, a_texcoord6, a_texcoord7, a_color0
$output v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldTangent

#include <bgfx_shader.sh>

void main()
{
    vec4 worldPos = mul(u_model[0], vec4(a_position, 1.0));
    vec4 projected = mul(u_modelViewProj, vec4(a_position, 1.0));
    gl_Position = projected;
    v_worldPos = worldPos.xyz;
    v_worldTangent = vec3(projected.z, projected.w, 0.0);
    v_color0 = a_color0;
    v_texcoord01 = vec4(a_texcoord0, a_texcoord1);
    v_texcoord23 = vec4(a_texcoord2, a_texcoord3);
    v_texcoord45 = vec4(a_texcoord4, a_texcoord5);
    v_texcoord67 = vec4(a_texcoord6, a_texcoord7);
}
