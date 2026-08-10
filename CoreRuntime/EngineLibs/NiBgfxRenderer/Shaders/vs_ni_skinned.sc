$input a_position, a_normal, a_tangent, a_bitangent, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_texcoord4, a_texcoord5, a_texcoord6, a_texcoord7, a_color0, a_indices, a_weight
$output v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

// Gamebryo's standard material contract supports at most 30 palette-local
// bones per draw. Each NiMatrix3x4 is uploaded as three vec4 rows.
uniform vec4 u_skinBones[90];

vec4 skinRow(int row, vec4 indices, vec4 weight)
{
    float w3 = 1.0 - weight.x - weight.y - weight.z;
    int i0 = int(indices.x + 0.5) * 3 + row;
    int i1 = int(indices.y + 0.5) * 3 + row;
    int i2 = int(indices.z + 0.5) * 3 + row;
    int i3 = int(indices.w + 0.5) * 3 + row;
    return u_skinBones[i0] * weight.x
         + u_skinBones[i1] * weight.y
         + u_skinBones[i2] * weight.z
         + u_skinBones[i3] * w3;
}

void main()
{
    vec4 r0 = skinRow(0, a_indices, a_weight);
    vec4 r1 = skinRow(1, a_indices, a_weight);
    vec4 r2 = skinRow(2, a_indices, a_weight);
    vec4 p = vec4(a_position, 1.0);
    vec3 worldPos = vec3(dot(r0, p), dot(r1, p), dot(r2, p));
    vec3 worldNormal = vec3(dot(r0.xyz, a_normal), dot(r1.xyz, a_normal), dot(r2.xyz, a_normal));
    vec3 worldTangent = vec3(dot(r0.xyz, a_tangent), dot(r1.xyz, a_tangent), dot(r2.xyz, a_tangent));
    vec3 worldBitangent = vec3(dot(r0.xyz, a_bitangent), dot(r1.xyz, a_bitangent), dot(r2.xyz, a_bitangent));

    gl_Position = mul(u_viewProj, vec4(worldPos, 1.0));
    v_worldPos = worldPos;
    v_worldNormal = normalize(worldNormal);
    v_worldTangent = normalize(worldTangent);
    v_worldBitangent = normalize(worldBitangent);
    v_color0 = a_color0;
    v_texcoord01 = vec4(a_texcoord0, a_texcoord1);
    v_texcoord23 = vec4(a_texcoord2, a_texcoord3);
    v_texcoord45 = vec4(a_texcoord4, a_texcoord5);
    v_texcoord67 = vec4(a_texcoord6, a_texcoord7);
}
