$input a_position, a_texcoord0
$output v_texcoord01

#include <bgfx_shader.sh>

void main()
{
    // NiVSMBlurMaterial uses the normal Gamebryo transform pipeline for its
    // screen-filling quad. Keep only the varying consumed by the blur pixel
    // shader so bgfx's VS/FS interface hashes match exactly.
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_texcoord01 = vec4(a_texcoord0, 0.0, 0.0);
}
