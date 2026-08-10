$input a_position, a_texcoord0
$output v_texcoord01

#include <bgfx_shader.sh>

void main()
{
    gl_Position = vec4(a_position, 1.0);
    v_texcoord01 = vec4(a_texcoord0, 0.0, 0.0);
}
