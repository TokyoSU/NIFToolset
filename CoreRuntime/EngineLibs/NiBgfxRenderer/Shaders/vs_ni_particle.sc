$input a_position, a_texcoord0, i_data0, i_data1, i_data2
$output v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

uniform vec4 u_particleCameraRight;
uniform vec4 u_particleCameraUp;

void main()
{
    // Shared quad corners are [-1,+1]. Gamebryo defines positive sprite
    // rotation as clockwise in the camera plane.
    float s = sin(i_data2.x);
    float c = cos(i_data2.x);
    vec2 corner = a_position.xy;
    vec2 rotated = vec2(
        corner.x * c + corner.y * s,
       -corner.x * s + corner.y * c);

    vec3 modelRight = u_particleCameraRight.xyz;
    vec3 modelUp = u_particleCameraUp.xyz;
    vec3 modelNormal = normalize(cross(modelRight, modelUp));
    vec3 modelPos = i_data0.xyz + i_data0.w *
        (modelRight * rotated.x + modelUp * rotated.y);

    vec4 worldPos = mul(u_model[0], vec4(modelPos, 1.0));
    gl_Position = mul(u_modelViewProj, vec4(modelPos, 1.0));
    v_worldPos = worldPos.xyz;
    v_worldNormal = normalize(mul(u_model[0], vec4(modelNormal, 0.0)).xyz);

    // The legacy facing-quad generator only emits POSITION/NORMAL/COLOR/UV.
    // Preserve the generic renderer's fallback tangent basis for bump maps.
    v_worldTangent = normalize(mul(u_model[0], vec4(1.0, 0.0, 0.0, 0.0)).xyz);
    v_worldBitangent = normalize(mul(u_model[0], vec4(0.0, 1.0, 0.0, 0.0)).xyz);

    v_color0 = i_data1;
    v_texcoord01 = vec4(a_texcoord0, 0.0, 0.0);
    v_texcoord23 = vec4(0.0, 0.0, 0.0, 0.0);
    v_texcoord45 = vec4(0.0, 0.0, 0.0, 0.0);
    v_texcoord67 = vec4(0.0, 0.0, 0.0, 0.0);
}
