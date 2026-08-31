$input v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

#define MAX_LIGHTS 8
#define MAX_TERRAIN_LAYERS 4

SAMPLER2D(s_terrainLowDiffuse, 0);
SAMPLER2D(s_terrainLowNormal, 1);
SAMPLER2D(s_terrainBlend, 2);
SAMPLER2D(s_terrainBase0, 3);
SAMPLER2D(s_terrainNormal0, 4);
SAMPLER2D(s_terrainSpec0, 5);
SAMPLER2D(s_terrainBase1, 6);
SAMPLER2D(s_terrainNormal1, 7);
SAMPLER2D(s_terrainSpec1, 8);
SAMPLER2D(s_terrainBase2, 9);
SAMPLER2D(s_terrainNormal2, 10);
SAMPLER2D(s_terrainSpec2, 11);
SAMPLER2D(s_terrainBase3, 12);
SAMPLER2D(s_terrainNormal3, 13);
SAMPLER2D(s_terrainSpec3, 14);
#ifdef TERRAIN_SHADOW_CUBE
SAMPLERCUBE(s_terrainShadow, 15);
#else
SAMPLER2D(s_terrainShadow, 15);
#endif

uniform vec4 u_materialAmbient;
uniform vec4 u_materialDiffuse;
uniform vec4 u_materialSpecular;
uniform vec4 u_materialEmissive;
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
uniform vec4 u_lightShadowParams[MAX_LIGHTS];
uniform vec4 u_lightShadowExtra[MAX_LIGHTS];
uniform vec4 u_lightShadowMatrix0[MAX_LIGHTS];
uniform vec4 u_lightShadowMatrix1[MAX_LIGHTS];
uniform vec4 u_lightShadowMatrix2[MAX_LIGHTS];
uniform vec4 u_lightShadowMatrix3[MAX_LIGHTS];
uniform vec4 u_pssmParams;
uniform vec4 u_pssmSplitDistances[4];
uniform vec4 u_pssmSplitRows[64];
uniform vec4 u_pssmViewports[16];
uniform vec4 u_pssmTransitionRows[4];
uniform vec4 u_pssmTransitionParams;
// x=enabled, y=selected light index, z=PSSM, w=cube shadow variant.
uniform vec4 u_terrainShadowParams;
uniform vec4 u_fogColor;
uniform vec4 u_fogParams;

// layerFeatures0 = base, normal, parallax, detail
// layerFeatures1 = distribution, specular, enabled, reserved
uniform vec4 u_terrainLayerFeatures0[MAX_TERRAIN_LAYERS];
uniform vec4 u_terrainLayerFeatures1[MAX_TERRAIN_LAYERS];
uniform vec4 u_terrainLayerScale;
uniform vec4 u_terrainDistRamp;
uniform vec4 u_terrainParallaxStrength;
uniform vec4 u_terrainSpecPower;
uniform vec4 u_terrainSpecIntensity;
uniform vec4 u_terrainDetailScale;
// blend scale.xy, blend offset.zw
uniform vec4 u_terrainTexCoord0;
// low-detail scale.xy, low-detail offset.zw
uniform vec4 u_terrainTexCoord1;
// low-detail texture sizes.xy, low-detail specular power/intensity.zw
uniform vec4 u_terrainLowDetail;
// threshold, morph distance, morph mode, layer count
uniform vec4 u_terrainMorph;
uniform vec4 u_terrainStitching;
// adjusted terrain eye.xyz, reserved
uniform vec4 u_terrainEye;
// debug mode, render mode, low diffuse enabled, low normal enabled
uniform vec4 u_terrainDebug;
// x = disable lighting, y = Gamebryo NDL_UpdateTime.
uniform vec4 u_terrainRenderParams;

vec3 safeNormalize(vec3 value, vec3 fallback)
{
    float len2 = dot(value, value);
    return len2 > 1e-8 ? value * inversesqrt(len2) : fallback;
}

vec4 sampleLayerBase(int layer, vec2 uv)
{
    if (layer == 0) return texture2D(s_terrainBase0, uv);
    if (layer == 1) return texture2D(s_terrainBase1, uv);
    if (layer == 2) return texture2D(s_terrainBase2, uv);
    return texture2D(s_terrainBase3, uv);
}

vec4 sampleLayerNormal(int layer, vec2 uv)
{
    if (layer == 0) return texture2D(s_terrainNormal0, uv);
    if (layer == 1) return texture2D(s_terrainNormal1, uv);
    if (layer == 2) return texture2D(s_terrainNormal2, uv);
    return texture2D(s_terrainNormal3, uv);
}

vec4 sampleLayerSpec(int layer, vec2 uv)
{
    if (layer == 0) return texture2D(s_terrainSpec0, uv);
    if (layer == 1) return texture2D(s_terrainSpec1, uv);
    if (layer == 2) return texture2D(s_terrainSpec2, uv);
    return texture2D(s_terrainSpec3, uv);
}

float component4(vec4 value, int index)
{
    if (index == 0) return value.x;
    if (index == 1) return value.y;
    if (index == 2) return value.z;
    return value.w;
}

float terrainMorphValue(vec3 localPos)
{
    int mode = int(u_terrainMorph.z + 0.5);
    if (mode == 0)
    {
        vec2 span = max(abs(u_terrainStitching.zw - u_terrainStitching.xy), vec2(1e-5, 1e-5));
        vec2 normalizedPos = (localPos.xy - u_terrainStitching.xy) / span;
        return (normalizedPos.x < 0.0 || normalizedPos.y < 0.0 ||
                normalizedPos.x > 1.0 || normalizedPos.y > 1.0) ? 1.0 : 0.0;
    }

    float d;
    if (mode == 1)
    {
        d = distance(localPos.xy, u_terrainEye.xy);
    }
    else if (mode == 2)
    {
        d = distance(localPos.xy, u_terrainEye.xy);
        d = max(d, u_terrainEye.z);
    }
    else
    {
        d = distance(localPos, u_terrainEye.xyz);
    }
    return saturate((d - u_terrainMorph.x) / max(u_terrainMorph.y, 1e-5));
}

vec4 terrainShadowTransform(vec3 worldPos, vec4 row0, vec4 row1,
    vec4 row2, vec4 row3)
{
    vec4 p = vec4(worldPos, 1.0);
    return vec4(
        p.x * row0.x + p.y * row1.x + p.z * row2.x + p.w * row3.x,
        p.x * row0.y + p.y * row1.y + p.z * row2.y + p.w * row3.y,
        p.x * row0.z + p.y * row1.z + p.z * row2.z + p.w * row3.z,
        p.x * row0.w + p.y * row1.w + p.z * row2.w + p.w * row3.w);
}

#ifndef TERRAIN_SHADOW_CUBE
float terrainShadowCompare(vec4 lightProjPos, vec4 viewport,
    vec4 params, vec4 extra)
{
    if (abs(lightProjPos.w) <= 1e-6)
        return 1.0;

    vec3 shadowCoord = lightProjPos.xyz / lightProjPos.w;
    vec2 uv = shadowCoord.xy * 0.5 + vec2(0.5, 0.5);
    uv.y = 1.0 - uv.y;
    vec2 shadowTest = (uv + viewport.zw) * viewport.xy;
    vec2 borderError = saturate(shadowTest) - shadowTest;
    if (abs(borderError.x) > 0.00001 || abs(borderError.y) > 0.00001)
        return 1.0;

    float depth = saturate(shadowCoord.z) - extra.x;
    int shadowTechnique = int(params.z + 0.5);
    if (shadowTechnique == 2)
    {
        vec4 moments = texture2D(s_terrainShadow, uv);
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
        float s00 = depth <= texture2D(s_terrainShadow, uv).r ? 1.0 : 0.0;
        float s10 = depth <= texture2D(s_terrainShadow, uv + vec2(invSize.x, 0.0)).r ? 1.0 : 0.0;
        float s01 = depth <= texture2D(s_terrainShadow, uv + vec2(0.0, invSize.y)).r ? 1.0 : 0.0;
        float s11 = depth <= texture2D(s_terrainShadow, uv + invSize).r ? 1.0 : 0.0;
        return mix(mix(s00, s10, lerps.x), mix(s01, s11, lerps.x), lerps.y);
    }
    return depth <= texture2D(s_terrainShadow, uv).r ? 1.0 : 0.0;
}

float terrainPssmDistanceAt(int index)
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

vec4 terrainPssmRowAt(int index)
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

vec4 terrainPssmViewportAt(int index)
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

float terrainPssmTransitionNoise(vec3 worldPos)
{
    if (u_pssmTransitionParams.x <= 0.5)
        return 0.0;
    vec4 p = vec4(worldPos, 1.0);
    vec2 scr = vec2(
        p.x * u_pssmTransitionRows[0].x + p.y * u_pssmTransitionRows[1].x +
            p.z * u_pssmTransitionRows[2].x + p.w * u_pssmTransitionRows[3].x,
        p.x * u_pssmTransitionRows[0].y + p.y * u_pssmTransitionRows[1].y +
            p.z * u_pssmTransitionRows[2].y + p.w * u_pssmTransitionRows[3].y);
    float noise = fract(sin(dot(scr, vec2(12.9898, 78.233))) * 43758.5453);
    return noise * u_pssmTransitionParams.y;
}

#endif

float evaluateTerrainShadow(int lightIndex, vec3 worldPos)
{
    if (u_terrainShadowParams.x <= 0.5 ||
        lightIndex != int(u_terrainShadowParams.y + 0.5))
    {
        return 1.0;
    }

    vec4 params = u_lightShadowParams[lightIndex];
    vec4 extra = u_lightShadowExtra[lightIndex];
#ifdef TERRAIN_SHADOW_CUBE
    // Point-light shadow maps store radial world-space depth in a cube map.
    // The z flip and bias scaling mirror NiStandardMaterial's ShadowCubeMap
    // fragment exactly.
    float lightType = u_lightPositionType[lightIndex].w;
    if (!(lightType > 0.5 && lightType < 1.5))
        return 1.0;
    vec3 viewVector = worldPos - u_lightPositionType[lightIndex].xyz;
    viewVector.z = -viewVector.z;
    float distanceToLight = length(viewVector);
    vec3 lookupVector = safeNormalize(viewVector, vec3(0.0, 0.0, 1.0));
    float storedDepth = textureCube(s_terrainShadow, lookupVector).r;
    float receiverDepth = distanceToLight * extra.x;
    return storedDepth == 0.0 || storedDepth > receiverDepth ? 1.0 : 0.0;
#else
    if (u_terrainShadowParams.z > 0.5 && u_pssmParams.x > 0.5)
    {
        int sliceCount = int(u_pssmParams.z + 0.5);
        if (sliceCount <= 0)
            return 1.0;
        float cameraDistance = abs(dot(worldPos - u_cameraPosition.xyz,
            safeNormalize(u_cameraDirection.xyz, vec3(0.0, 0.0, 1.0))));
        cameraDistance += terrainPssmTransitionNoise(worldPos);
        int slice = 0;
        for (int i = 0; i < 16; ++i)
        {
            if (i < sliceCount - 1 && terrainPssmDistanceAt(i) < cameraDistance)
                ++slice;
        }
        if (slice < 0) slice = 0;
        if (slice >= sliceCount) slice = sliceCount - 1;
        int base = slice * 4;
        vec4 lightProjPos = terrainShadowTransform(worldPos,
            terrainPssmRowAt(base + 0), terrainPssmRowAt(base + 1),
            terrainPssmRowAt(base + 2), terrainPssmRowAt(base + 3));
        return terrainShadowCompare(lightProjPos,
            terrainPssmViewportAt(slice), params, extra);
    }

    vec4 lightProjPos = terrainShadowTransform(worldPos,
        u_lightShadowMatrix0[lightIndex], u_lightShadowMatrix1[lightIndex],
        u_lightShadowMatrix2[lightIndex], u_lightShadowMatrix3[lightIndex]);
    return terrainShadowCompare(lightProjPos, vec4(1.0, 1.0, 0.0, 0.0),
        params, extra);
#endif
}

void main()
{
    vec2 uvIn = v_texcoord01.xy;
    vec2 maskUV = uvIn * u_terrainTexCoord0.xy + u_terrainTexCoord0.zw;
    vec2 lowCommonUV = uvIn * u_terrainTexCoord1.xy + u_terrainTexCoord1.zw;

    vec2 lowSize = max(u_terrainLowDetail.xy, vec2(1.0, 1.0));
    float lowDiffuseBorder = 1.0 / lowSize.x;
    float lowDiffuseScale = 1.0 - 2.0 * lowDiffuseBorder;
    vec2 lowDiffuseUV = lowCommonUV * lowDiffuseScale + vec2(lowDiffuseBorder * 1.5, lowDiffuseBorder * 1.5);
    float lowNormalBorder = 0.5 / lowSize.y;
    float lowNormalScale = 1.0 - 2.0 * lowNormalBorder;
    vec2 lowNormalUV = lowCommonUV * lowNormalScale + vec2(lowNormalBorder, lowNormalBorder);

    vec3 baseNormal = safeNormalize(v_worldNormal, vec3(0.0, 0.0, 1.0));
    vec3 tangent = safeNormalize(v_worldTangent, vec3(1.0, 0.0, 0.0));
    vec3 bitangent = safeNormalize(v_worldBitangent, cross(baseNormal, tangent));
    mat3 tbn = mat3(tangent, bitangent, baseNormal);
    vec3 viewDir = safeNormalize(u_cameraPosition.xyz - v_worldPos, vec3(0.0, 0.0, 1.0));
    vec3 tangentView = vec3(dot(viewDir, tangent), dot(viewDir, bitangent), dot(viewDir, baseNormal));
    float tangentViewDenom = max(abs(tangentView.z), 0.15);

    vec4 mask = texture2D(s_terrainBlend, maskUV);
    vec3 layerColor0 = vec3(1.0, 1.0, 1.0); vec3 layerColor1 = vec3(1.0, 1.0, 1.0);
    vec3 layerColor2 = vec3(1.0, 1.0, 1.0); vec3 layerColor3 = vec3(1.0, 1.0, 1.0);
    vec3 layerNormal0 = baseNormal; vec3 layerNormal1 = baseNormal;
    vec3 layerNormal2 = baseNormal; vec3 layerNormal3 = baseNormal;
    vec3 layerSpec0 = vec3(0.0, 0.0, 0.0); vec3 layerSpec1 = vec3(0.0, 0.0, 0.0);
    vec3 layerSpec2 = vec3(0.0, 0.0, 0.0); vec3 layerSpec3 = vec3(0.0, 0.0, 0.0);

    for (int layer = 0; layer < MAX_TERRAIN_LAYERS; ++layer)
    {
        if (u_terrainLayerFeatures1[layer].z <= 0.5)
            continue;

        vec2 layerUV = lowCommonUV * component4(u_terrainLayerScale, layer);
        if (u_terrainLayerFeatures0[layer].z > 0.5)
        {
            float height = sampleLayerNormal(layer, layerUV).a;
            float strength = component4(u_terrainParallaxStrength, layer);
            layerUV += tangentView.xy * ((height - 0.5) * strength / tangentViewDenom);
        }

        vec4 baseSample = vec4(1.0, 1.0, 1.0, 0.0);
        if (u_terrainLayerFeatures0[layer].x > 0.5 ||
            u_terrainLayerFeatures1[layer].x > 0.5)
        {
            baseSample = sampleLayerBase(layer, layerUV);
        }

        if (u_terrainLayerFeatures1[layer].x > 0.5)
        {
            float adjusted = component4(mask, layer) *
                (1.0 + component4(u_terrainDistRamp, layer) * baseSample.a);
            if (layer == 0) mask.x = adjusted;
            else if (layer == 1) mask.y = adjusted;
            else if (layer == 2) mask.z = adjusted;
            else mask.w = adjusted;
        }

        vec3 color = u_terrainLayerFeatures0[layer].x > 0.5 ? baseSample.rgb : vec3(1.0, 1.0, 1.0);
        vec3 normal = baseNormal;
        if (u_terrainLayerFeatures0[layer].y > 0.5)
        {
            vec3 tangentNormal = sampleLayerNormal(layer, layerUV).rgg * 2.0 - 1.0;
            tangentNormal.z = sqrt(max(1.0 - tangentNormal.x * tangentNormal.x -
                tangentNormal.y * tangentNormal.y, 0.0));
            normal = safeNormalize(mul(tbn, tangentNormal), baseNormal);
        }

        vec3 specular = vec3(0.0, 0.0, 0.0);
        if (u_terrainLayerFeatures1[layer].y > 0.5)
            specular = sampleLayerSpec(layer, layerUV).rgb;

        if (u_terrainLayerFeatures0[layer].w > 0.5)
        {
            float detail = sampleLayerSpec(layer,
                layerUV * component4(u_terrainDetailScale, layer)).a;
            color *= 2.0 * detail;
        }

        if (layer == 0) { layerColor0 = color; layerNormal0 = normal; layerSpec0 = specular; }
        else if (layer == 1) { layerColor1 = color; layerNormal1 = normal; layerSpec1 = specular; }
        else if (layer == 2) { layerColor2 = color; layerNormal2 = normal; layerSpec2 = specular; }
        else { layerColor3 = color; layerNormal3 = normal; layerSpec3 = specular; }
    }

    float maskSum = dot(mask, vec4(1.0, 1.0, 1.0, 1.0));
    if (maskSum > 1.0)
        mask /= maskSum;
    maskSum = dot(mask, vec4(1.0, 1.0, 1.0, 1.0));
    float defaultMask = max(1.0 - maskSum, 0.0);

    vec3 highColor = layerColor0 * mask.x + layerColor1 * mask.y +
        layerColor2 * mask.z + layerColor3 * mask.w + vec3(defaultMask, defaultMask, defaultMask);
    vec3 highNormal = safeNormalize(layerNormal0 * mask.x + layerNormal1 * mask.y +
        layerNormal2 * mask.z + layerNormal3 * mask.w + baseNormal * defaultMask, baseNormal);
    vec3 highSpecular = layerSpec0 * mask.x + layerSpec1 * mask.y +
        layerSpec2 * mask.z + layerSpec3 * mask.w;
    float highGloss = dot(highSpecular, vec3(0.299, 0.587, 0.114));

    vec4 specEnabled = vec4(u_terrainLayerFeatures1[0].y,
        u_terrainLayerFeatures1[1].y, u_terrainLayerFeatures1[2].y,
        u_terrainLayerFeatures1[3].y);
    vec4 specMask = mask * specEnabled;
    float specMaskSum = dot(specMask, vec4(1.0, 1.0, 1.0, 1.0));
    if (specMaskSum > 0.0)
        specMask /= specMaskSum;
    float highSpecPower = dot(u_terrainSpecPower, specMask);
    float highSpecIntensity = dot(u_terrainSpecIntensity, mask);

    float morph = v_color0.x;
    vec3 lowColor = highColor;
    vec3 lowNormal = baseNormal;
    float lowGloss = highGloss;
    float lowSpecPower = max(u_terrainLowDetail.z, 1.0);
    float lowSpecIntensity = u_terrainLowDetail.w;

    if (u_terrainDebug.z > 0.5)
    {
        vec4 low = texture2D(s_terrainLowDiffuse, lowDiffuseUV);
        lowColor = low.rgb;
        lowGloss = low.a * lowSpecIntensity;
    }
    if (u_terrainDebug.w > 0.5)
    {
        vec3 n = texture2D(s_terrainLowNormal, lowNormalUV).rgb;
        n.rg = n.rg * 2.0 - 1.0;
        lowNormal = safeNormalize(mul(tbn, n), baseNormal);
    }

    vec3 albedo = u_terrainDebug.z > 0.5 ? mix(highColor, lowColor, morph) : highColor;
    vec3 normal = u_terrainDebug.w > 0.5 ?
        safeNormalize(mix(highNormal, lowNormal, morph), highNormal) : highNormal;
    float gloss = u_terrainDebug.z > 0.5 ? mix(highGloss, lowGloss, morph) : highGloss;
    float specPower = u_terrainDebug.z > 0.5 ? mix(highSpecPower, lowSpecPower, morph) : highSpecPower;
    float specIntensity = u_terrainDebug.z > 0.5 ? mix(highSpecIntensity, lowSpecIntensity, morph) : highSpecIntensity;
    vec3 specularColor = u_terrainDebug.z > 0.5 ? mix(highSpecular, lowColor, morph) : highSpecular;

    vec3 ambientAccum = u_sceneAmbient.rgb * u_materialAmbient.rgb;
    vec3 diffuseAccum = vec3(0.0, 0.0, 0.0);
    vec3 specularAccum = vec3(0.0, 0.0, 0.0);
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
                attenuation = pow(attenuation, max(u_lightAmbientFalloff[i].w, 1.0));
            }
            if (type > 1.5)
            {
                vec3 spotDir = safeNormalize(u_lightDirectionRange[i].xyz, vec3(0.0, 0.0, 1.0));
                float cone = dot(-L, spotDir);
                float coneRange = max(u_lightSpotParams[i].x - u_lightSpotParams[i].y, 1e-5);
                float spot = saturate((cone - u_lightSpotParams[i].y) / coneRange);
                attenuation *= pow(spot, max(u_lightSpotParams[i].z, 1.0));
            }
        }

        attenuation *= evaluateTerrainShadow(i, v_worldPos);

        float dimmer = u_lightDiffuseDimmer[i].w;
        float ndotl = max(dot(normal, L), 0.0);
        ambientAccum += u_lightAmbientFalloff[i].rgb * dimmer * attenuation;
        diffuseAccum += u_lightDiffuseDimmer[i].rgb * dimmer * ndotl * attenuation;
        if (ndotl > 0.0 && specIntensity > 0.0)
        {
            vec3 H = safeNormalize(L + viewDir, viewDir);
            float spec = pow(max(dot(normal, H), 0.0), max(specPower, 1.0));
            specularAccum += u_lightSpecularSpot[i].rgb * dimmer * attenuation * spec;
        }
    }

    vec4 color = vec4(ambientAccum + diffuseAccum * albedo +
        specularAccum * specularColor * gloss * specIntensity + u_materialEmissive.rgb, 1.0);

    if (u_fogParams.x > 0.5)
    {
        float fogDistance;
        if (int(u_fogParams.y + 0.5) == 1)
            fogDistance = length(v_worldPos - u_cameraPosition.xyz);
        else
            fogDistance = dot(v_worldPos - u_cameraPosition.xyz,
                safeNormalize(u_cameraDirection.xyz, vec3(0.0, 0.0, 1.0)));
        float fogAmount = saturate((fogDistance - u_fogParams.z) /
            max(u_fogParams.w - u_fogParams.z, 1e-5));
        color.rgb = mix(color.rgb, u_fogColor.rgb, fogAmount);
    }

    if (u_terrainRenderParams.x > 0.5)
        color.rgb = albedo;

    int renderMode = int(u_terrainDebug.y + 0.5);
    if (renderMode == 1)
    {
        // BAKE_DIFFUSE stores terrain gloss in alpha.
        color.a = gloss;
    }
    else if (renderMode == 2 && maskSum < 0.99)
    {
        // Exact PulseUnpaintedArea node behavior from NiTerrainMaterial:
        // red -> yellow as the painted mask approaches one, with a 60%
        // sinusoidal blend driven by Gamebryo's NDL_UpdateTime constant.
        vec3 gradientColor = mix(vec3(1.0, 0.0, 0.0),
            vec3(1.0, 1.0, 0.0), saturate(pow(maskSum, 11.0)));
        float pulse = ((sin(u_terrainRenderParams.y * 1.5) + 1.0) * 0.5) * 0.6;
        color.rgb = mix(color.rgb, gradientColor, pulse);
    }

    // Match NiTerrainCellShaderData's public debug modes.
    int debugMode = int(u_terrainDebug.x + 0.5);
    if (debugMode == 1)
        color = vec4(normal * 0.5 + 0.5, 1.0);
    else if (debugMode == 2)
        color = vec4(vec3(morph, morph, morph), 1.0);
    else if (debugMode == 3)
    {
        float used = saturate(u_terrainMorph.w / 4.0);
        color = vec4(mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), used), 1.0);
    }
    else if (debugMode == 4)
        color = vec4(mix(vec3(1.0, 0.0, 0.0), vec3(0.5, 0.5, 0.5), saturate(maskSum)), 1.0);
    else if (debugMode == 5)
        color = vec4(vec3(gloss, gloss, gloss), 1.0);

    gl_FragColor = color;
}
