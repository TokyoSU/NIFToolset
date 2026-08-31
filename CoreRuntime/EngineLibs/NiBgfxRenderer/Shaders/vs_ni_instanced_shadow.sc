$input a_position, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_texcoord4, a_color0, i_data0, i_data1, i_data2
$output v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldTangent

#include <bgfx_shader.sh>

void main()
{
    vec3 p = a_position;
    // INSTANCETRANSFORMS contains the three columns of Gamebryo's
    // world rotation with translation stored in each column's W component.
    vec3 world = vec3(
        i_data0.x * p.x + i_data1.x * p.y + i_data2.x * p.z + i_data0.w,
        i_data0.y * p.x + i_data1.y * p.y + i_data2.y * p.z + i_data1.w,
        i_data0.z * p.x + i_data1.z * p.y + i_data2.z * p.z + i_data2.w);
    vec4 projected = mul(u_viewProj, vec4(world, 1.0));
    gl_Position = projected;
    v_worldPos = world;
    v_worldTangent = vec3(projected.z, projected.w, 0.0);
    v_color0 = a_color0;
    v_texcoord01 = vec4(a_texcoord0, a_texcoord1);
    v_texcoord23 = vec4(a_texcoord2, a_texcoord3);
    v_texcoord45 = vec4(a_texcoord4, 0.0, 0.0);
    v_texcoord67 = vec4(0.0, 0.0, 0.0, 0.0);
}
