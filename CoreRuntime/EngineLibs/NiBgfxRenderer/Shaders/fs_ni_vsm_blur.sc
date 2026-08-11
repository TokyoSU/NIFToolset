$input v_texcoord01

#include <bgfx_shader.sh>

SAMPLER2D(s_baseTexture, 0);
// NiVSMBlurMaterial can apply a Base map transform on the vertical pass.
// These uniforms are already populated by BindMaterialAndTexture.
uniform vec4 u_mapTransform0[11];
uniform vec4 u_mapTransform1[11];
// x/y = inverse source texture size, z = kernel size, w = direction
// (0 = horizontal, 1 = vertical).
uniform vec4 u_vsmBlurParams;

void main()
{
    float kernel = clamp(floor(u_vsmBlurParams.z + 0.5), 2.0, 16.0);
    float halfKernel = kernel * 0.5;
    float invTotalWeights = 1.0 / (kernel * kernel);
    vec2 stepUV = u_vsmBlurParams.w > 0.5
        ? vec2(0.0, u_vsmBlurParams.y)
        : vec2(u_vsmBlurParams.x, 0.0);
    vec3 baseUv = vec3(v_texcoord01.xy, 1.0);
    vec2 transformedUv = vec2(
        dot(u_mapTransform0[0].xyz, baseUv),
        dot(u_mapTransform1[0].xyz, baseUv));
    vec2 uv = transformedUv - stepUV * halfKernel;
    float blurWeight = 2.0;
    vec4 totalColor = vec4(0.0, 0.0, 0.0, 0.0);

    // This is the same triangular kernel used by NiVSMBlurMaterial's
    // HorzBlurSample/VertBlurSample nodes, generalized to the supported
    // even kernel sizes instead of emitting one branch per pair of taps.
    for (int tap = 0; tap < 16; ++tap)
    {
        if (float(tap) < kernel)
        {
            totalColor += texture2D(s_baseTexture, uv) *
                blurWeight * invTotalWeights;
            uv += stepUV;
            blurWeight -= sign(float(tap + 1) - halfKernel) * 4.0;
        }
    }

    gl_FragColor = totalColor;
}
