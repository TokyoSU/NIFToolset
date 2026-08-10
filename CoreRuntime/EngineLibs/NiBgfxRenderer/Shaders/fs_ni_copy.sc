$input v_texcoord01

#include <bgfx_shader.sh>

SAMPLER2D(s_copyTexture, 0);

void main()
{
    gl_FragColor = texture2D(s_copyTexture, v_texcoord01.xy);
}
