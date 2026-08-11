$input a_position, a_normal, a_tangent, a_bitangent, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_texcoord4, a_color0, i_data0, i_data1, i_data2
$output v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

// Gamebryo packs INSTANCETRANSFORMS as three matrix columns, not rows:
//   i_data0 = (R00, R10, R20, Tx)
//   i_data1 = (R01, R11, R21, Ty)
//   i_data2 = (R02, R12, R22, Tz)
// Reconstruct the same transform used by the original HLSL row-vector path.
vec3 instancePoint(vec3 p, vec4 col0, vec4 col1, vec4 col2)
{
    return vec3(
        col0.x * p.x + col1.x * p.y + col2.x * p.z + col0.w,
        col0.y * p.x + col1.y * p.y + col2.y * p.z + col1.w,
        col0.z * p.x + col1.z * p.y + col2.z * p.z + col2.w);
}

vec3 instanceVector(vec3 v, vec4 col0, vec4 col1, vec4 col2)
{
    return vec3(
        col0.x * v.x + col1.x * v.y + col2.x * v.z,
        col0.y * v.x + col1.y * v.y + col2.y * v.z,
        col0.z * v.x + col1.z * v.y + col2.z * v.z);
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
