$input a_position, a_normal, a_tangent, a_bitangent, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_texcoord4, a_color0, i_data0, i_data1, i_data2
$output v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

vec3 instancePoint(vec3 p, vec4 row0, vec4 row1, vec4 row2)
{
    return vec3(
        dot(row0.xyz, p) + row0.w,
        dot(row1.xyz, p) + row1.w,
        dot(row2.xyz, p) + row2.w);
}

vec3 instanceVector(vec3 v, vec4 row0, vec4 row1, vec4 row2)
{
    return vec3(
        dot(row0.xyz, v),
        dot(row1.xyz, v),
        dot(row2.xyz, v));
}

void main()
{
    vec3 worldPos = instancePoint(a_position, i_data0, i_data1, i_data2);
    gl_Position = mul(u_viewProj, vec4(worldPos, 1.0));
    v_worldPos = worldPos;
    v_worldNormal = normalize(instanceVector(a_normal, i_data0, i_data1, i_data2));
    v_worldTangent = normalize(instanceVector(a_tangent, i_data0, i_data1, i_data2));
    v_worldBitangent = normalize(instanceVector(a_bitangent, i_data0, i_data1, i_data2));
    v_color0 = a_color0;
    v_texcoord01 = vec4(a_texcoord0, a_texcoord1);
    v_texcoord23 = vec4(a_texcoord2, a_texcoord3);
    v_texcoord45 = vec4(a_texcoord4, 0.0, 0.0);
    v_texcoord67 = vec4(0.0, 0.0, 0.0, 0.0);
}
