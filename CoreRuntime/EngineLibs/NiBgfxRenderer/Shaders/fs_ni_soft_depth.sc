$input v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

#define MAX_MAPS 11
#define MAP_BASE 0

SAMPLER2D(s_baseTexture, 0);

uniform vec4 u_materialDiffuse;
uniform vec4 u_materialEmissive;
uniform vec4 u_alphaParams;
uniform vec4 u_textureParams;
uniform vec4 u_mapParams[MAX_MAPS];
uniform vec4 u_mapTransform0[MAX_MAPS];
uniform vec4 u_mapTransform1[MAX_MAPS];
uniform vec4 u_cameraPosition;
uniform vec4 u_cameraDirection;
// x=fade distance, y=1/target width, z=1/target height, w=far plane.
uniform vec4 u_softParticleParams;

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

vec2 baseMapUV(vec4 uv01, vec4 uv23, vec4 uv45, vec4 uv67)
{
    vec2 uv = selectUV(u_mapParams[MAP_BASE].y,
        uv01, uv23, uv45, uv67);
    vec3 p = vec3(uv, 1.0);
    return vec2(dot(u_mapTransform0[MAP_BASE].xyz, p),
                dot(u_mapTransform1[MAP_BASE].xyz, p));
}

bool alphaTestPass(float alpha)
{
    if (u_alphaParams.y <= 0.5)
        return true;

    float refValue = u_alphaParams.x;
    int mode = int(u_alphaParams.z + 0.5);
    if (mode == 0) return true;
    if (mode == 1) return alpha < refValue;
    if (mode == 2) return abs(alpha - refValue) <= (0.5 / 255.0);
    if (mode == 3) return alpha <= refValue;
    if (mode == 4) return alpha > refValue;
    if (mode == 5) return abs(alpha - refValue) > (0.5 / 255.0);
    if (mode == 6) return alpha >= refValue;
    return false;
}

void main()
{
    int sourceMode = int(u_textureParams.y + 0.5);
    int lightingMode = int(u_textureParams.z + 0.5);
    bool replaceMode = int(u_textureParams.x + 0.5) == 0;

    float materialOpacity = lightingMode == 0 ?
        u_materialEmissive.a : u_materialDiffuse.a;
    if (sourceMode == 1 || (sourceMode == 2 && lightingMode != 0))
        materialOpacity = v_color0.a;

    float opacity = replaceMode ? 1.0 : materialOpacity;
    if (u_mapParams[MAP_BASE].x > 0.5)
        opacity *= texture2D(s_baseTexture, baseMapUV(v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67)).a;

    if (!alphaTestPass(opacity))
        discard;

    vec3 cameraDir = normalize(u_cameraDirection.xyz);
    float farPlane = max(u_softParticleParams.w, 1e-4);
    float viewDepth = dot(v_worldPos - u_cameraPosition.xyz, cameraDir);
    float normalizedDepth = saturate(viewDepth / farPlane);

    // The target is R32F, but writing all channels keeps this source valid for
    // backends that internally widen the attachment format.
    gl_FragColor = vec4(normalizedDepth, normalizedDepth,
        normalizedDepth, 1.0);
}
