$input v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldTangent

#include <bgfx_shader.sh>

SAMPLER2D(s_baseTexture, 0);
uniform vec4 u_materialDiffuse;
uniform vec4 u_alphaParams;
uniform vec4 u_mapParams[11];
uniform vec4 u_mapTransform0[11];
uniform vec4 u_mapTransform1[11];
uniform vec4 u_textureParams;
uniform vec4 u_cameraPosition;
uniform vec4 u_shadowWriteParams;

vec2 selectUV(float index, vec4 uv01, vec4 uv23, vec4 uv45, vec4 uv67)
{
    if (index < 0.5) return uv01.xy;
    if (index < 1.5) return uv01.zw;
    if (index < 2.5) return uv23.xy;
    if (index < 3.5) return uv23.zw;
    if (index < 4.5) return uv45.xy;
    if (index < 5.5) return uv45.zw;
    if (index < 6.5) return uv67.xy;
    return uv67.zw;
}

vec2 baseUV(vec4 uv01, vec4 uv23, vec4 uv45, vec4 uv67)
{
    vec2 uv = selectUV(u_mapParams[0].y, uv01, uv23, uv45, uv67);
    vec3 p = vec3(uv, 1.0);
    return vec2(dot(u_mapTransform0[0].xyz, p),
        dot(u_mapTransform1[0].xyz, p));
}

bool alphaPass(float value, float refValue, int testMode)
{
    if      (testMode == 0) return true;
    else if (testMode == 1) return value <  refValue;
    else if (testMode == 2) return abs(value - refValue) <= (0.5 / 255.0);
    else if (testMode == 3) return value <= refValue;
    else if (testMode == 4) return value >  refValue;
    else if (testMode == 5) return abs(value - refValue) >  (0.5 / 255.0);
    else if (testMode == 6) return value >= refValue;
    return false;
}

void main()
{
    if (u_alphaParams.y > 0.5)
    {
        float opacity = u_materialDiffuse.a;
        if (u_mapParams[0].x > 0.5)
            opacity *= texture2D(s_baseTexture, baseUV(v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67)).a;

        int sourceMode = int(u_textureParams.y + 0.5);
        int lightingMode = int(u_textureParams.z + 0.5);
        if (sourceMode == 1 || (sourceMode == 2 && lightingMode != 0))
            opacity *= v_color0.a;

        if (!alphaPass(opacity, u_alphaParams.x,
            int(u_alphaParams.z + 0.5)))
        {
            discard;
        }
    }

    int mode = int(u_shadowWriteParams.x + 0.5);
    float depth;
    if (mode == 2)
    {
        // NiPointShadowWriteMaterial::WriteDepthToColor stores radial
        // world-space distance from the point-light/shadow camera.
        depth = length(v_worldPos - u_cameraPosition.xyz);
    }
    else
    {
        depth = v_worldTangent.x / max(abs(v_worldTangent.y), 1e-8);
    }

    if (mode == 1)
        gl_FragColor = vec4(depth, depth * depth, 1.0, 1.0);
    else
        gl_FragColor = vec4(depth, 1.0, 1.0, 1.0);
}
