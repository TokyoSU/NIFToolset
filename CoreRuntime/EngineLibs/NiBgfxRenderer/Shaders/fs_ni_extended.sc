$input v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

#define MAX_MAPS 11
#define MAX_LIGHTS 8
#define MAX_PROJECTED_EFFECTS 3

SAMPLER2DARRAY(s_baseTexture, 0);
SAMPLER2DARRAY(s_darkTexture, 1);
SAMPLER2D(s_detailTexture, 2);
SAMPLER2D(s_glossTexture, 3);
SAMPLER2D(s_glowTexture, 4);
SAMPLER2D(s_bumpTexture, 5);
SAMPLER2D(s_normalTexture, 6);
SAMPLER2D(s_parallaxTexture, 7);
SAMPLER2D(s_decalTexture0, 8);
SAMPLER2D(s_decalTexture1, 9);
SAMPLER2D(s_decalTexture2, 10);
SAMPLER2D(s_envTexture2D, 11);
SAMPLERCUBE(s_envTextureCube, 12);
SAMPLER2D(s_projectedTexture0, 13);
SAMPLER2D(s_projectedTexture1, 14);
SAMPLER2D(s_projectedTexture2, 15);

uniform vec4 u_materialAmbient;
uniform vec4 u_materialDiffuse;
uniform vec4 u_materialSpecular;
uniform vec4 u_materialEmissive;
uniform vec4 u_alphaParams;
uniform vec4 u_textureParams;
uniform vec4 u_mapParams[MAX_MAPS];
uniform vec4 u_mapTransform0[MAX_MAPS];
uniform vec4 u_mapTransform1[MAX_MAPS];
uniform vec4 u_bumpParams;
uniform vec4 u_cameraPosition;
uniform vec4 u_cameraDirection;
uniform vec4 u_sceneAmbient;
uniform vec4 u_lightCount;
uniform vec4 u_lightPositionType[MAX_LIGHTS];
uniform vec4 u_lightDirectionRange[MAX_LIGHTS];
uniform vec4 u_lightDiffuseDimmer[MAX_LIGHTS];
uniform vec4 u_lightAmbientFalloff[MAX_LIGHTS];
uniform vec4 u_lightSpecularSpot[MAX_LIGHTS];
uniform vec4 u_lightSpotParams[MAX_LIGHTS];
// x=enabled, y=sampler stage, z=technique (0 hard, 1 PCF, 2 VSM), w=VSM power
uniform vec4 u_lightShadowParams[MAX_LIGHTS];
// x=bias, y=1/width, z=1/height, w=VSM epsilon
uniform vec4 u_lightShadowExtra[MAX_LIGHTS];
uniform vec4 u_lightShadowMatrix0[MAX_LIGHTS];
uniform vec4 u_lightShadowMatrix1[MAX_LIGHTS];
uniform vec4 u_lightShadowMatrix2[MAX_LIGHTS];
uniform vec4 u_lightShadowMatrix3[MAX_LIGHTS];
// x=enabled, y=light index, z=slice count, w=shadow sampler stage
uniform vec4 u_pssmParams;
uniform vec4 u_pssmSplitDistances[4];
uniform vec4 u_pssmSplitRows[64];
uniform vec4 u_pssmViewports[16];
uniform vec4 u_pssmTransitionRows[4];
// x=transition enabled, y=transition world-size, z=noise sampler stage, w=reserved
uniform vec4 u_pssmTransitionParams;
uniform vec4 u_fogColor;
uniform vec4 u_fogParams;
uniform vec4 u_envParams;
uniform vec4 u_envTransform0;
uniform vec4 u_envTransform1;
uniform vec4 u_envTransform2;
uniform vec4 u_projectedParams[MAX_PROJECTED_EFFECTS];
uniform vec4 u_projectedTransform0[MAX_PROJECTED_EFFECTS];
uniform vec4 u_projectedTransform1[MAX_PROJECTED_EFFECTS];
uniform vec4 u_projectedTransform2[MAX_PROJECTED_EFFECTS];
uniform vec4 u_projectedClipPlane[MAX_PROJECTED_EFFECTS];
// NiExtendedMaterial array terrain parameters.
// info: x=layer count, y=alpha blur radius, z=edge softness, w=unused.
// alphaInfo: xy=inverse alpha dimensions.
uniform vec4 u_extendedTerrainInfo;
uniform vec4 u_extendedAlphaInfo;
uniform vec4 u_extendedLayerData[32];

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

vec2 mapUV(int slot, vec4 uv01, vec4 uv23, vec4 uv45, vec4 uv67)
{
    vec2 uv = selectUV(u_mapParams[slot].y, uv01, uv23, uv45, uv67);
    vec3 p = vec3(uv, 1.0);
    return vec2(dot(u_mapTransform0[slot].xyz, p),
                dot(u_mapTransform1[slot].xyz, p));
}

vec3 safeNormalize(vec3 value, vec3 fallback)
{
    float len2 = dot(value, value);
    return len2 > 1e-8 ? value * inversesqrt(len2) : fallback;
}

vec3 projectEnvironment(vec3 source)
{
    vec4 p = vec4(source, 1.0);
    return vec3(dot(p, u_envTransform0),
                dot(p, u_envTransform1),
                dot(p, u_envTransform2));
}

vec3 projectEffect(int slot, vec3 source)
{
    vec4 p = vec4(source, 1.0);
    return vec3(dot(p, u_projectedTransform0[slot]),
                dot(p, u_projectedTransform1[slot]),
                dot(p, u_projectedTransform2[slot]));
}

vec4 sampleProjectedTexture(int slot, vec2 uv)
{
    if (slot == 0) return texture2D(s_projectedTexture0, uv);
    if (slot == 1) return texture2D(s_projectedTexture1, uv);
    return texture2D(s_projectedTexture2, uv);
}

vec4 sampleShadow2DStage(int stage, vec2 uv)
{
    // Stages 0/1 are Texture2DArray terrain resources in NiExtendedMaterial
    // and are deliberately never assigned to a shadow receiver.
    if (stage == 2) return texture2D(s_detailTexture, uv);
    if (stage == 3) return texture2D(s_glossTexture, uv);
    if (stage == 4) return texture2D(s_glowTexture, uv);
    if (stage == 5) return texture2D(s_bumpTexture, uv);
    if (stage == 6) return texture2D(s_normalTexture, uv);
    if (stage == 7) return texture2D(s_parallaxTexture, uv);
    if (stage == 8) return texture2D(s_decalTexture0, uv);
    if (stage == 9) return texture2D(s_decalTexture1, uv);
    if (stage == 10) return texture2D(s_decalTexture2, uv);
    if (stage == 11) return texture2D(s_envTexture2D, uv);
    if (stage == 13) return texture2D(s_projectedTexture0, uv);
    if (stage == 14) return texture2D(s_projectedTexture1, uv);
    if (stage == 15) return texture2D(s_projectedTexture2, uv);
    return vec4(1.0, 1.0, 1.0, 1.0);
}

vec4 transformShadowPosition(vec3 worldPos, vec4 row0, vec4 row1,
    vec4 row2, vec4 row3)
{
    vec4 p = vec4(worldPos, 1.0);
    // Gamebryo's generated shaders use mul(WorldPos, Matrix). The packed
    // constant data is four matrix rows, so reconstruct each mathematical
    // column explicitly instead of relying on backend matrix layout rules.
    return vec4(
        p.x * row0.x + p.y * row1.x + p.z * row2.x + p.w * row3.x,
        p.x * row0.y + p.y * row1.y + p.z * row2.y + p.w * row3.y,
        p.x * row0.z + p.y * row1.z + p.z * row2.z + p.w * row3.z,
        p.x * row0.w + p.y * row1.w + p.z * row2.w + p.w * row3.w);
}

float shadowCompare2D(int stage, vec4 lightProjPos, vec4 viewport,
    vec4 params, vec4 extra)
{
    if (abs(lightProjPos.w) <= 1e-6)
        return 1.0;

    vec3 shadowCoord = lightProjPos.xyz / lightProjPos.w;
    vec2 uv = shadowCoord.xy * 0.5 + vec2(0.5, 0.5);
    uv.y = 1.0 - uv.y;

    // PSSM matrices are pre-scaled into their atlas cells. The viewport is
    // only a border test, matching NiStandardMaterial's PSSM fragments.
    vec2 shadowTest = (uv + viewport.zw) * viewport.xy;
    vec2 borderError = saturate(shadowTest) - shadowTest;
    if (abs(borderError.x) > 0.00001 || abs(borderError.y) > 0.00001)
        return 1.0;

    float depth = saturate(shadowCoord.z) - extra.x;
    int shadowTechnique = int(params.z + 0.5);

    if (shadowTechnique == 2)
    {
        vec4 moments = sampleShadow2DStage(stage, uv);
        float mean = moments.r;
        if (depth <= mean)
            return 1.0;

        float variance = min(1.0, max(0.0,
            moments.g - mean * mean + max(extra.w, 0.0)));
        float delta = depth - mean;
        float probability = variance / max(variance + delta * delta, 1e-6);
        return pow(saturate(probability), max(params.w, 0.0001));
    }

    if (shadowTechnique == 1)
    {
        vec2 invSize = max(extra.yz, vec2(1e-7, 1e-7));
        vec2 mapSize = 1.0 / invSize;
        vec2 lerps = fract(uv * mapSize);
        float s00 = depth <= sampleShadow2DStage(stage, uv).r ? 1.0 : 0.0;
        float s10 = depth <= sampleShadow2DStage(stage, uv + vec2(invSize.x, 0.0)).r ? 1.0 : 0.0;
        float s01 = depth <= sampleShadow2DStage(stage, uv + vec2(0.0, invSize.y)).r ? 1.0 : 0.0;
        float s11 = depth <= sampleShadow2DStage(stage, uv + invSize).r ? 1.0 : 0.0;
        return mix(mix(s00, s10, lerps.x), mix(s01, s11, lerps.x), lerps.y);
    }

    return depth <= sampleShadow2DStage(stage, uv).r ? 1.0 : 0.0;
}

float pssmDistanceAt(int index)
{
    int vectorIndex = index / 4;
    int componentIndex = index - vectorIndex * 4;
    vec4 distances = u_pssmSplitDistances[0];
    if (vectorIndex == 1) distances = u_pssmSplitDistances[1];
    else if (vectorIndex == 2) distances = u_pssmSplitDistances[2];
    else if (vectorIndex == 3) distances = u_pssmSplitDistances[3];
    if (componentIndex == 0) return distances.x;
    if (componentIndex == 1) return distances.y;
    if (componentIndex == 2) return distances.z;
    return distances.w;
}

vec4 pssmRowAt(int index)
{
    if (index == 0) return u_pssmSplitRows[0];
    if (index == 1) return u_pssmSplitRows[1];
    if (index == 2) return u_pssmSplitRows[2];
    if (index == 3) return u_pssmSplitRows[3];
    if (index == 4) return u_pssmSplitRows[4];
    if (index == 5) return u_pssmSplitRows[5];
    if (index == 6) return u_pssmSplitRows[6];
    if (index == 7) return u_pssmSplitRows[7];
    if (index == 8) return u_pssmSplitRows[8];
    if (index == 9) return u_pssmSplitRows[9];
    if (index == 10) return u_pssmSplitRows[10];
    if (index == 11) return u_pssmSplitRows[11];
    if (index == 12) return u_pssmSplitRows[12];
    if (index == 13) return u_pssmSplitRows[13];
    if (index == 14) return u_pssmSplitRows[14];
    if (index == 15) return u_pssmSplitRows[15];
    if (index == 16) return u_pssmSplitRows[16];
    if (index == 17) return u_pssmSplitRows[17];
    if (index == 18) return u_pssmSplitRows[18];
    if (index == 19) return u_pssmSplitRows[19];
    if (index == 20) return u_pssmSplitRows[20];
    if (index == 21) return u_pssmSplitRows[21];
    if (index == 22) return u_pssmSplitRows[22];
    if (index == 23) return u_pssmSplitRows[23];
    if (index == 24) return u_pssmSplitRows[24];
    if (index == 25) return u_pssmSplitRows[25];
    if (index == 26) return u_pssmSplitRows[26];
    if (index == 27) return u_pssmSplitRows[27];
    if (index == 28) return u_pssmSplitRows[28];
    if (index == 29) return u_pssmSplitRows[29];
    if (index == 30) return u_pssmSplitRows[30];
    if (index == 31) return u_pssmSplitRows[31];
    if (index == 32) return u_pssmSplitRows[32];
    if (index == 33) return u_pssmSplitRows[33];
    if (index == 34) return u_pssmSplitRows[34];
    if (index == 35) return u_pssmSplitRows[35];
    if (index == 36) return u_pssmSplitRows[36];
    if (index == 37) return u_pssmSplitRows[37];
    if (index == 38) return u_pssmSplitRows[38];
    if (index == 39) return u_pssmSplitRows[39];
    if (index == 40) return u_pssmSplitRows[40];
    if (index == 41) return u_pssmSplitRows[41];
    if (index == 42) return u_pssmSplitRows[42];
    if (index == 43) return u_pssmSplitRows[43];
    if (index == 44) return u_pssmSplitRows[44];
    if (index == 45) return u_pssmSplitRows[45];
    if (index == 46) return u_pssmSplitRows[46];
    if (index == 47) return u_pssmSplitRows[47];
    if (index == 48) return u_pssmSplitRows[48];
    if (index == 49) return u_pssmSplitRows[49];
    if (index == 50) return u_pssmSplitRows[50];
    if (index == 51) return u_pssmSplitRows[51];
    if (index == 52) return u_pssmSplitRows[52];
    if (index == 53) return u_pssmSplitRows[53];
    if (index == 54) return u_pssmSplitRows[54];
    if (index == 55) return u_pssmSplitRows[55];
    if (index == 56) return u_pssmSplitRows[56];
    if (index == 57) return u_pssmSplitRows[57];
    if (index == 58) return u_pssmSplitRows[58];
    if (index == 59) return u_pssmSplitRows[59];
    if (index == 60) return u_pssmSplitRows[60];
    if (index == 61) return u_pssmSplitRows[61];
    if (index == 62) return u_pssmSplitRows[62];
    if (index == 63) return u_pssmSplitRows[63];
    return vec4(0.0, 0.0, 0.0, 0.0);
}

vec4 pssmViewportAt(int index)
{
    if (index == 0) return u_pssmViewports[0];
    if (index == 1) return u_pssmViewports[1];
    if (index == 2) return u_pssmViewports[2];
    if (index == 3) return u_pssmViewports[3];
    if (index == 4) return u_pssmViewports[4];
    if (index == 5) return u_pssmViewports[5];
    if (index == 6) return u_pssmViewports[6];
    if (index == 7) return u_pssmViewports[7];
    if (index == 8) return u_pssmViewports[8];
    if (index == 9) return u_pssmViewports[9];
    if (index == 10) return u_pssmViewports[10];
    if (index == 11) return u_pssmViewports[11];
    if (index == 12) return u_pssmViewports[12];
    if (index == 13) return u_pssmViewports[13];
    if (index == 14) return u_pssmViewports[14];
    if (index == 15) return u_pssmViewports[15];
    return vec4(1.0, 1.0, 0.0, 0.0);
}

float pssmTransitionNoise(vec3 worldPos)
{
    if (u_pssmTransitionParams.x <= 0.5)
        return 0.0;

    vec4 p = vec4(worldPos, 1.0);
    vec2 scr = vec2(
        p.x * u_pssmTransitionRows[0].x + p.y * u_pssmTransitionRows[1].x +
            p.z * u_pssmTransitionRows[2].x + p.w * u_pssmTransitionRows[3].x,
        p.x * u_pssmTransitionRows[0].y + p.y * u_pssmTransitionRows[1].y +
            p.z * u_pssmTransitionRows[2].y + p.w * u_pssmTransitionRows[3].y);
    // Match the original Gamebryo PSSM path: sample the transition generator's
    // 128x128 NiNoiseTexture. The renderer dynamically lends an unused 2D
    // sampler stage to this draw and stores that stage in params.z.
    int noiseStage = int(u_pssmTransitionParams.z + 0.5);
    float noise = sampleShadow2DStage(noiseStage, scr).r;
    return noise * u_pssmTransitionParams.y;
}

float evaluatePssmShadow(int lightIndex, vec3 worldPos, vec4 params, vec4 extra)
{
    int sliceCount = int(u_pssmParams.z + 0.5);
    float cameraDistance = abs(dot(worldPos - u_cameraPosition.xyz,
        safeNormalize(u_cameraDirection.xyz, vec3(0.0, 0.0, 1.0))));
    cameraDistance += pssmTransitionNoise(worldPos);

    int slice = 0;
    for (int i = 0; i < 16; ++i)
    {
        if (i < sliceCount - 1 && pssmDistanceAt(i) < cameraDistance)
            ++slice;
    }
    if (slice < 0) slice = 0;
    if (sliceCount <= 0) return 1.0;
    if (slice >= sliceCount) slice = sliceCount - 1;

    int base = slice * 4;
    vec4 lightProjPos = transformShadowPosition(worldPos,
        pssmRowAt(base + 0), pssmRowAt(base + 1),
        pssmRowAt(base + 2), pssmRowAt(base + 3));
    return shadowCompare2D(int(u_pssmParams.w + 0.5), lightProjPos,
        pssmViewportAt(slice), params, extra);
}

float evaluateLightShadow(int lightIndex, float lightType, vec3 worldPos)
{
    vec4 params = u_lightShadowParams[lightIndex];
    if (params.x <= 0.5)
        return 1.0;

    vec4 extra = u_lightShadowExtra[lightIndex];
    int stage = int(params.y + 0.5);

    if (u_pssmParams.x > 0.5 &&
        lightIndex == int(u_pssmParams.y + 0.5))
    {
        return evaluatePssmShadow(lightIndex, worldPos, params, extra);
    }

    // NiShadowTechnique uses a cube map only for point lights. Slot 12 is the
    // standard material's sole cube sampler and is reserved for that map by
    // the renderer when this branch is active.
    if (lightType > 0.5 && lightType < 1.5 && stage == 12)
    {
        vec3 viewVector = worldPos - u_lightPositionType[lightIndex].xyz;
        viewVector.z = -viewVector.z;
        float distanceToLight = length(viewVector);
        vec3 lookupVector = safeNormalize(viewVector, vec3(0.0, 0.0, 1.0));
        float storedDepth = textureCube(s_envTextureCube, lookupVector).r;
        float receiverDepth = distanceToLight * extra.x;
        return storedDepth == 0.0 || storedDepth > receiverDepth ? 1.0 : 0.0;
    }

    vec4 lightProjPos = transformShadowPosition(worldPos,
        u_lightShadowMatrix0[lightIndex], u_lightShadowMatrix1[lightIndex],
        u_lightShadowMatrix2[lightIndex], u_lightShadowMatrix3[lightIndex]);
    return shadowCompare2D(stage, lightProjPos, vec4(1.0, 1.0, 0.0, 0.0),
        params, extra);
}

vec4 sampleProjectedEffect(int slot, vec3 worldPos)
{
    vec3 projected = projectEffect(slot, worldPos);
    vec2 uv = projected.xy;
    if (u_projectedParams[slot].z > 0.5)
    {
        float q = abs(projected.z) > 1e-6 ? projected.z : 1.0;
        uv /= q;
    }
    return sampleProjectedTexture(slot, uv);
}

float projectedClipScalar(int slot, vec3 worldPos, bool invertClip)
{
    if (u_projectedParams[slot].w <= 0.5)
        return 0.0;

    vec4 plane = u_projectedClipPlane[slot];
    float distanceToPlane = dot(plane.xyz, worldPos) - plane.w;
    if (invertClip)
        return distanceToPlane > 0.0 ? 0.0 : 1.0;
    return distanceToPlane > 0.0 ? 1.0 : 0.0;
}


void main()
{
    const int MAP_BASE = 0;
    const int MAP_DARK = 1;
    const int MAP_DETAIL = 2;
    const int MAP_GLOSS = 3;
    const int MAP_GLOW = 4;
    const int MAP_BUMP = 5;
    const int MAP_NORMAL = 6;
    const int MAP_PARALLAX = 7;
    const int MAP_DECAL0 = 8;
    const int MAP_DECAL1 = 9;
    const int MAP_DECAL2 = 10;

    vec4 vertexColor = v_color0;
    int sourceMode = int(u_textureParams.y + 0.5);
    int lightingMode = int(u_textureParams.z + 0.5);
    bool specularEnabled = u_textureParams.w > 0.5;

    // Match NiStandardMaterial::HandleInitialSpecAmbDiffEmissiveColor.
    // Vertex color replaces (rather than modulates) the selected material
    // coefficient. In LIGHTING_E mode only emissive participates.
    vec3 materialAmbient = u_materialAmbient.rgb;
    vec3 materialDiffuse = u_materialDiffuse.rgb;
    vec3 materialEmissive = u_materialEmissive.rgb;
    float materialOpacity = lightingMode == 0 ? u_materialEmissive.a : u_materialDiffuse.a;
    if (sourceMode == 1) // SOURCE_EMISSIVE
    {
        materialEmissive = vertexColor.rgb;
        materialOpacity = vertexColor.a;
    }
    else if (sourceMode == 2 && lightingMode != 0) // SOURCE_AMB_DIFF
    {
        materialAmbient = vertexColor.rgb;
        materialDiffuse = vertexColor.rgb;
        materialOpacity = vertexColor.a;
    }

    vec3 normal = safeNormalize(v_worldNormal, vec3(0.0, 0.0, 1.0));
    vec3 tangent = safeNormalize(v_worldTangent, vec3(1.0, 0.0, 0.0));
    vec3 bitangent = safeNormalize(v_worldBitangent, cross(normal, tangent));
    mat3 tbn = mat3(tangent, bitangent, normal);
    vec3 viewDir = safeNormalize(u_cameraPosition.xyz - v_worldPos, vec3(0.0, 0.0, 1.0));

    vec2 baseUV = mapUV(MAP_BASE, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67);
    if (u_mapParams[MAP_PARALLAX].x > 0.5)
    {
        float height = texture2D(s_parallaxTexture, mapUV(MAP_PARALLAX, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67)).r;
        vec3 tangentView = vec3(dot(viewDir, tangent), dot(viewDir, bitangent), dot(viewDir, normal));
        float denom = max(abs(tangentView.z), 0.15);
        baseUV += tangentView.xy * ((height - 0.5) * u_bumpParams.z / denom);
    }

    if (u_mapParams[MAP_NORMAL].x > 0.5)
    {
        vec3 n = texture2D(s_normalTexture, mapUV(MAP_NORMAL, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67)).xyz * 2.0 - 1.0;
        normal = safeNormalize(mul(tbn, n), normal);
    }
    else if (u_mapParams[MAP_BUMP].x > 0.5)
    {
        // Legacy NiBumpMap is a height/luma map. Derivatives provide a
        // backend-neutral approximation without requiring texel-size uniforms.
        float h = texture2D(s_bumpTexture, mapUV(MAP_BUMP, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67)).r * u_bumpParams.x + u_bumpParams.y;
        vec2 grad = vec2(dFdx(h), dFdy(h));
        normal = safeNormalize(normal - tangent * grad.x - bitangent * grad.y, normal);
    }

    vec3 ambientAccum = u_sceneAmbient.rgb * materialAmbient;
    vec3 diffuseAccum = vec3(0.0, 0.0, 0.0);
    vec3 specularAccum = vec3(0.0, 0.0, 0.0);

    if (lightingMode != 0) // LIGHTING_E_A_D
    {
        int count = int(u_lightCount.x + 0.5);
        for (int i = 0; i < MAX_LIGHTS; ++i)
        {
            if (i >= count)
                break;

            float type = u_lightPositionType[i].w;
            vec3 L;
            float attenuation = 1.0;
            if (type < 0.5)
            {
                L = safeNormalize(-u_lightDirectionRange[i].xyz, vec3(0.0, 0.0, 1.0));
            }
            else
            {
                vec3 toLight = u_lightPositionType[i].xyz - v_worldPos;
                float distanceToLight = length(toLight);
                L = distanceToLight > 1e-6 ? toLight / distanceToLight : normal;
                float range = u_lightDirectionRange[i].w;
                if (range > 0.0)
                {
                    attenuation = saturate(1.0 - distanceToLight / range);
                    float falloff = max(u_lightAmbientFalloff[i].w, 1.0);
                    attenuation = pow(attenuation, falloff);
                }

                if (type > 1.5)
                {
                    vec3 spotDir = safeNormalize(u_lightDirectionRange[i].xyz, vec3(0.0, 0.0, 1.0));
                    float cone = dot(-L, spotDir);
                    float innerCos = u_lightSpotParams[i].x;
                    float outerCos = u_lightSpotParams[i].y;
                    float coneRange = max(innerCos - outerCos, 1e-5);
                    float spot = saturate((cone - outerCos) / coneRange);
                    spot = pow(spot, max(u_lightSpotParams[i].z, 1.0));
                    attenuation *= spot;
                }
            }

            attenuation *= evaluateLightShadow(i, type, v_worldPos);

            float dimmer = u_lightDiffuseDimmer[i].w;
            float ndotl = max(dot(normal, L), 0.0);
            ambientAccum += materialAmbient * u_lightAmbientFalloff[i].rgb * dimmer * attenuation;
            diffuseAccum += materialDiffuse * u_lightDiffuseDimmer[i].rgb * dimmer * ndotl * attenuation;

            if (specularEnabled && ndotl > 0.0)
            {
                vec3 H = safeNormalize(L + viewDir, viewDir);
                float spec = pow(max(dot(normal, H), 0.0), max(u_materialSpecular.w, 1.0));
                specularAccum += u_materialSpecular.rgb * u_lightSpecularSpot[i].rgb *
                    dimmer * spec * attenuation;
            }
        }
    }

    float glossFactor = 1.0;
    if (u_mapParams[MAP_GLOSS].x > 0.5)
        glossFactor = texture2D(s_glossTexture, mapUV(MAP_GLOSS, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67)).r;

    vec3 diffuseCoeff = materialEmissive;
    if (lightingMode != 0)
        diffuseCoeff += ambientAccum + diffuseAccum;

    vec3 specularCoeff = vec3(0.0, 0.0, 0.0);
    if (specularEnabled)
        specularCoeff = specularAccum * glossFactor;

    // NiExtendedMaterial intentionally replaces the complete standard
    // pre-light diffuse texture accumulator with TerrainSplatTextureArray.
    // Normal/parallax/gloss state above and post-light environment/fog below
    // still follow NiStandardMaterial.
    vec2 terrainUV = v_texcoord01.xy;
    int terrainLayerCount = min(int(u_extendedTerrainInfo.x + 0.5), 32);
    vec3 texDiffuse = vec3(1.0, 1.0, 1.0);
    if (terrainLayerCount > 0)
    {
        vec4 baseData = u_extendedLayerData[0];
        texDiffuse = texture2DArray(s_baseTexture,
            vec3(fract(terrainUV * baseData.xy), 0.0)).rgb;

        vec2 blurTexel = u_extendedAlphaInfo.xy * max(u_extendedTerrainInfo.y, 0.0);
        float edgeSoftness = max(u_extendedTerrainInfo.z, 0.001);
        for (int layer = 1; layer < 32; ++layer)
        {
            if (layer >= terrainLayerCount)
                break;

            vec4 data = u_extendedLayerData[layer];
            float z = float(layer);
            float alpha = 0.0;
            alpha += texture2DArray(s_darkTexture, vec3(terrainUV + blurTexel * vec2(-1.0, -1.0), z)).r;
            alpha += texture2DArray(s_darkTexture, vec3(terrainUV + blurTexel * vec2( 0.0, -1.0), z)).r * 2.0;
            alpha += texture2DArray(s_darkTexture, vec3(terrainUV + blurTexel * vec2( 1.0, -1.0), z)).r;
            alpha += texture2DArray(s_darkTexture, vec3(terrainUV + blurTexel * vec2(-1.0,  0.0), z)).r * 2.0;
            alpha += texture2DArray(s_darkTexture, vec3(terrainUV, z)).r * 4.0;
            alpha += texture2DArray(s_darkTexture, vec3(terrainUV + blurTexel * vec2( 1.0,  0.0), z)).r * 2.0;
            alpha += texture2DArray(s_darkTexture, vec3(terrainUV + blurTexel * vec2(-1.0,  1.0), z)).r;
            alpha += texture2DArray(s_darkTexture, vec3(terrainUV + blurTexel * vec2( 0.0,  1.0), z)).r * 2.0;
            alpha += texture2DArray(s_darkTexture, vec3(terrainUV + blurTexel * vec2( 1.0,  1.0), z)).r;
            alpha *= 1.0 / 16.0;
            if (data.w > 0.5)
                alpha = 1.0 - alpha;
            alpha = smoothstep(0.5 - edgeSoftness, 0.5 + edgeSoftness, alpha);
            float coverage = saturate(alpha * data.z);
            vec3 layerColor = texture2DArray(s_baseTexture,
                vec3(fract(terrainUV * data.xy), z)).rgb;
            texDiffuse = mix(texDiffuse, layerColor, coverage);
        }
    }

    bool replaceMode = false; // NiExtendedMaterial forces APPLY_MODULATE.
    float finalOpacity = materialOpacity;

    vec4 color = vec4(replaceMode ? texDiffuse : diffuseCoeff * texDiffuse, finalOpacity);

    if (u_envParams.x > 0.5)
    {
        int envMode = int(u_envParams.y + 0.5);
        vec3 envSource = v_worldPos;
        if (envMode == 2) // SPHERE_MAP
            envSource = reflect(-viewDir, normal);
        else if (envMode == 3) // SPECULAR_CUBE_MAP
            envSource = reflect(-viewDir, normal);
        else if (envMode == 4) // DIFFUSE_CUBE_MAP
            envSource = normal;

        vec3 projected = projectEnvironment(envSource);
        vec3 environmentColor;
        if (envMode == 3 || envMode == 4)
        {
            environmentColor = textureCube(s_envTextureCube,
                safeNormalize(projected, vec3(0.0, 0.0, 1.0))).rgb;
        }
        else
        {
            vec2 envUV = projected.xy;
            if (envMode == 1) // WORLD_PERSPECTIVE
            {
                float q = abs(projected.z) > 1e-6 ? projected.z : 1.0;
                envUV /= q;
            }
            environmentColor = texture2D(s_envTexture2D, envUV).rgb;
        }
        specularCoeff += environmentColor * glossFactor;
    }

    color.rgb += specularCoeff;

    // Glow is a post-light diffuse add in NiStandardMaterial.
    if (u_mapParams[MAP_GLOW].x > 0.5)
        color.rgb += texture2D(s_glowTexture, mapUV(MAP_GLOW, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67)).rgb;

    // Legacy fog-map effects are applied after glow. D3D's
    // D3DTOP_BLENDTEXTUREALPHA selects between CURRENT and the projected fog
    // texture using the fog texture's own alpha.
    for (int projectedIndex = 0; projectedIndex < MAX_PROJECTED_EFFECTS; ++projectedIndex)
    {
        if (u_projectedParams[projectedIndex].x <= 0.5 ||
            u_projectedParams[projectedIndex].y < 1.5)
        {
            continue;
        }

        vec4 fogSample = sampleProjectedEffect(projectedIndex, v_worldPos);
        color.rgb = mix(color.rgb, fogSample.rgb, fogSample.a);
    }

    if (u_alphaParams.y > 0.5)
    {
        float refValue = u_alphaParams.x;
        int testMode = int(u_alphaParams.z + 0.5);
        bool passTest = true;
        if      (testMode == 0) passTest = true;
        else if (testMode == 1) passTest = color.a <  refValue;
        else if (testMode == 2) passTest = abs(color.a - refValue) <= (0.5 / 255.0);
        else if (testMode == 3) passTest = color.a <= refValue;
        else if (testMode == 4) passTest = color.a >  refValue;
        else if (testMode == 5) passTest = abs(color.a - refValue) >  (0.5 / 255.0);
        else if (testMode == 6) passTest = color.a >= refValue;
        else if (testMode == 7) passTest = false;
        if (!passTest)
            discard;
    }

    if (u_fogParams.x > 0.5)
    {
        float fogDistance;
        if (int(u_fogParams.y + 0.5) == 1) // FOG_RANGE_SQ
            fogDistance = length(v_worldPos - u_cameraPosition.xyz);
        else
            fogDistance = dot(v_worldPos - u_cameraPosition.xyz,
                safeNormalize(u_cameraDirection.xyz, vec3(0.0, 0.0, 1.0)));
        float fogAmount = saturate((fogDistance - u_fogParams.z) /
            max(u_fogParams.w - u_fogParams.z, 1e-5));
        color.rgb = mix(color.rgb, u_fogColor.rgb, fogAmount);
    }

    gl_FragColor = color;
}
