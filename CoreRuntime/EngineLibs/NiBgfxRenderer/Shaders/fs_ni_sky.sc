$input v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

#define MAX_SKY_STAGES 5

SAMPLERCUBE(s_skyStage0, 0);
SAMPLERCUBE(s_skyStage1, 1);
SAMPLERCUBE(s_skyStage2, 2);
SAMPLERCUBE(s_skyStage3, 3);
SAMPLERCUBE(s_skyStage4, 4);

uniform vec4 u_cameraPosition;
uniform vec4 u_fogColor;
uniform vec4 u_skyStageConfig[MAX_SKY_STAGES];
uniform vec4 u_skyStageModifier[MAX_SKY_STAGES];
uniform vec4 u_skyGradientParams[MAX_SKY_STAGES];
uniform vec4 u_skyGradientHorizon[MAX_SKY_STAGES];
uniform vec4 u_skyGradientZenith[MAX_SKY_STAGES];
uniform vec4 u_skyOrientation0[MAX_SKY_STAGES];
uniform vec4 u_skyOrientation1[MAX_SKY_STAGES];
uniform vec4 u_skyOrientation2[MAX_SKY_STAGES];
uniform vec4 u_skyAtmosScattering;
uniform vec4 u_skyRgbInvWavelength;
uniform vec4 u_skyScaleDepth;
uniform vec4 u_skyPlanetDimensions;
uniform vec4 u_skyFrameData;
uniform vec4 u_skyUpMode;
uniform vec4 u_skySunSamples;

vec3 safeNormalize(vec3 value, vec3 fallback)
{
    float len2 = dot(value, value);
    return len2 > 1e-8 ? value * inversesqrt(len2) : fallback;
}

vec4 sampleSkyCube(int stage, vec3 direction)
{
    if (stage == 0) return textureCube(s_skyStage0, direction);
    if (stage == 1) return textureCube(s_skyStage1, direction);
    if (stage == 2) return textureCube(s_skyStage2, direction);
    if (stage == 3) return textureCube(s_skyStage3, direction);
    return textureCube(s_skyStage4, direction);
}

float atmosphereScale(float cosine, float scaleDepth)
{
    float x = 1.0 - cosine;
    return scaleDepth * exp(-0.00287 + x *
        (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

float horizonBias(vec3 normal, float exponent, float bias)
{
    vec3 up = safeNormalize(u_skyUpMode.xyz, vec3(0.0, 0.0, 1.0));
    float d = clamp(dot(safeNormalize(-normal, up), up), -1.0, 1.0);
    float arc = asin(d);
    float powered = pow(max(1.0 - d, 0.0), max(exponent, 1e-5));
    return mix(arc, powered, bias);
}

vec4 sampleStageColor(int stage, int colorMap, vec3 normal)
{
    vec3 sampleDirection = safeNormalize(-normal, vec3(0.0, 0.0, 1.0));

    if (colorMap == 1) // NiSkyMaterial::ColorMap::SKYBOX
        return sampleSkyCube(stage, sampleDirection);

    if (colorMap == 2) // GRADIENT
    {
        float t = horizonBias(normal, u_skyGradientParams[stage].x,
            u_skyGradientParams[stage].y);
        return mix(u_skyGradientHorizon[stage],
            u_skyGradientZenith[stage], t);
    }

    if (colorMap == 3) // FOG
        return u_fogColor;

    if (colorMap == 4) // ORIENTED_SKYBOX
    {
        // NiSkyMaterial's original HLSL uses mul(direction, matrix), i.e.
        // a row-vector multiply. The three uniforms store matrix rows.
        vec3 transformed = vec3(
            normal.x * u_skyOrientation0[stage].x +
                normal.y * u_skyOrientation1[stage].x +
                normal.z * u_skyOrientation2[stage].x,
            normal.x * u_skyOrientation0[stage].y +
                normal.y * u_skyOrientation1[stage].y +
                normal.z * u_skyOrientation2[stage].y,
            normal.x * u_skyOrientation0[stage].z +
                normal.y * u_skyOrientation1[stage].z +
                normal.z * u_skyOrientation2[stage].z);
        return sampleSkyCube(stage,
            safeNormalize(-transformed, sampleDirection));
    }

    // CUSTOM is intentionally unsupported by NiSkyMaterial itself unless a
    // subclass supplies nodes. Preserve the base material's deterministic
    // black fallback rather than sampling an unrelated texture.
    return vec4(0.0, 0.0, 0.0, 1.0);
}

void calculateScattering(vec3 worldView, out vec3 rayleigh, out vec3 mie)
{
    vec3 up = safeNormalize(u_skyUpMode.xyz, vec3(0.0, 0.0, 1.0));
    vec3 sunDirection = safeNormalize(u_skySunSamples.xyz, -up);

    float outerRadius2 = u_skyPlanetDimensions.y;
    float innerRadius = u_skyPlanetDimensions.z;
    float innerRadius2 = u_skyPlanetDimensions.w;
    float scale = u_skyScaleDepth.x;
    float scaleDepth = u_skyScaleDepth.y;
    float scaleOverScaleDepth = u_skyScaleDepth.z;

    float cosTheta = dot(worldView, -up);
    float b = -2.0 * innerRadius * cosTheta;
    float discriminant = max(b * b - 4.0 * (innerRadius2 - outerRadius2), 0.0);
    float farDistance = (2.0 * innerRadius * cosTheta + sqrt(discriminant)) * 0.5;
    farDistance = max(farDistance, 0.0);

    vec3 cameraPos = up * innerRadius;
    float height = max(length(cameraPos), 1e-5);
    float startDepth = 1.0;
    float startAngle = dot(worldView, cameraPos) / height;
    float startOffset = startDepth * atmosphereScale(startAngle, scaleDepth);

    // Gamebryo's atmosphere node uses a fixed five-iteration shader loop;
    // fSamples controls only the segment length; the generated legacy shader
    // still executes five scattering steps.
    float samples = max(u_skySunSamples.w, 1.0);
    float sampleLength = farDistance / samples;
    float scaledLength = sampleLength * scale;
    vec3 sampleDirection = worldView * sampleLength;
    vec3 samplePos = cameraPos + sampleDirection * 0.5;
    vec3 scatter = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < 5; ++i)
    {
        float sampleHeight = max(length(samplePos), 1e-5);
        float depth = exp(scaleOverScaleDepth * (innerRadius - sampleHeight));
        float lightAngle = dot(-sunDirection, samplePos) / sampleHeight;
        float cameraAngle = dot(worldView, samplePos) / sampleHeight;
        float lightDepth = depth * atmosphereScale(lightAngle, scaleDepth);
        float cameraDepth = depth * atmosphereScale(cameraAngle, scaleDepth);
        float scatterAmount = startOffset + (lightDepth - cameraDepth);
        vec3 attenuation = exp(-scatterAmount *
            (u_skyRgbInvWavelength.xyz * u_skyAtmosScattering.z +
             u_skyAtmosScattering.w));
        scatter += attenuation * (depth * scaledLength);
        samplePos += sampleDirection;
    }

    mie = scatter * u_skyAtmosScattering.y;
    rayleigh = scatter *
        (u_skyRgbInvWavelength.xyz * u_skyAtmosScattering.x);
}

vec4 atmosphericColor(vec3 worldView, vec3 rayleigh, vec3 mie)
{
    vec3 up = safeNormalize(u_skyUpMode.xyz, vec3(0.0, 0.0, 1.0));
    vec3 sunDirection = safeNormalize(u_skySunSamples.xyz, -up);
    float cosine = dot(sunDirection, worldView) /
        max(length(worldView), 1e-5);
    float g = u_skyFrameData.x;
    float g2 = u_skyFrameData.y;
    float rayleighPhase = 0.75 * (1.0 + cosine * cosine);
    float phaseDenominator = max(1.0 + g2 - 2.0 * g * cosine, 1e-5);
    float miePhase = 1.5 * ((1.0 - g2) / max(2.0 + g2, 1e-5)) *
        (u_skyScaleDepth.w * cosine * cosine) /
        pow(phaseDenominator, 1.5);

    vec3 hdr = rayleighPhase * rayleigh + miePhase * mie;
    vec3 mapped = 1.0 - exp(-hdr * u_skyRgbInvWavelength.w);
    return vec4(mapped, 1.0);
}

vec4 calculateAtmosphere(vec3 worldView)
{
    vec3 rayleigh;
    vec3 mie;
    calculateScattering(worldView, rayleigh, mie);
    return atmosphericColor(worldView, rayleigh, mie);
}

void main()
{
    vec3 normal = safeNormalize(v_worldNormal, vec3(0.0, 0.0, 1.0));
    vec3 worldView = safeNormalize(v_worldPos - u_cameraPosition.xyz,
        -normal);

    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    if (u_skyUpMode.w > 0.5 && u_skyUpMode.w < 1.5)
        color = atmosphericColor(worldView, v_worldTangent, v_worldBitangent);
    else if (u_skyUpMode.w >= 1.5)
        color = calculateAtmosphere(worldView);

    for (int stage = 0; stage < MAX_SKY_STAGES; ++stage)
    {
        if (u_skyStageConfig[stage].w < 0.5)
            continue;

        int colorMap = int(u_skyStageConfig[stage].x + 0.5);
        int modifierSource = int(u_skyStageConfig[stage].y + 0.5);
        int blendMethod = int(u_skyStageConfig[stage].z + 0.5);
        vec4 sampleColor = sampleStageColor(stage, colorMap, normal);

        float modifier = blendMethod == 3 ? 0.5 : 1.0;
        if (modifierSource == 2) // ALPHA
            modifier = sampleColor.a;
        else if (modifierSource == 3) // CONSTANT
            modifier = u_skyStageModifier[stage].x;
        else if (modifierSource == 4) // HORIZONBIAS
            modifier = horizonBias(normal, u_skyStageModifier[stage].y,
                u_skyStageModifier[stage].z);

        if (blendMethod == 1) // MULTIPLY
            color = color * sampleColor * modifier;
        else if (blendMethod == 2) // ADD
            color = color + sampleColor * modifier;
        else if (blendMethod == 3) // INTERPOLATE
            color = mix(color, sampleColor, modifier);
    }

    gl_FragColor = color;
}
