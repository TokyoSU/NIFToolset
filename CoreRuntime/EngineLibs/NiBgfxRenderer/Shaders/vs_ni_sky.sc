$input a_position, a_normal, a_tangent, a_bitangent, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_texcoord4, a_texcoord5, a_texcoord6, a_texcoord7, a_color0
$output v_color0, v_texcoord01, v_texcoord23, v_texcoord45, v_texcoord67, v_worldPos, v_worldNormal, v_worldTangent, v_worldBitangent

#include <bgfx_shader.sh>

uniform vec4 u_cameraPosition;
uniform vec4 u_skyAtmosScattering;
uniform vec4 u_skyRgbInvWavelength;
uniform vec4 u_skyScaleDepth;
uniform vec4 u_skyPlanetDimensions;
uniform vec4 u_skyUpMode;
uniform vec4 u_skySunSamples;

vec3 safeNormalize(vec3 value, vec3 fallback)
{
    float len2 = dot(value, value);
    return len2 > 1e-8 ? value * inversesqrt(len2) : fallback;
}

float atmosphereScale(float cosine, float scaleDepth)
{
    float x = 1.0 - cosine;
    return scaleDepth * exp(-0.00287 + x *
        (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
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

    float samples = clamp(u_skySunSamples.w, 1.0, 5.0);
    float sampleLength = farDistance / samples;
    float scaledLength = sampleLength * scale;
    vec3 sampleDirection = worldView * sampleLength;
    vec3 samplePos = cameraPos + sampleDirection * 0.5;
    vec3 scatter = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < 5; ++i)
    {
        if (float(i) >= samples)
            break;

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

void main()
{
    vec4 worldPos = mul(u_model[0], vec4(a_position, 1.0));
    vec4 projected = mul(u_modelViewProj, vec4(a_position, 1.0));

    // NiSkyMaterial destroys ordinary depth so the sky remains behind scene
    // geometry without depending on the camera's far-plane distance.
    projected.z = 0.5;
    gl_Position = projected;

    vec3 worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
    v_worldPos = worldPos.xyz;
    v_worldNormal = worldNormal;
    v_color0 = a_color0;
    v_texcoord01 = vec4(a_texcoord0, a_texcoord1);
    v_texcoord23 = vec4(a_texcoord2, a_texcoord3);
    v_texcoord45 = vec4(a_texcoord4, a_texcoord5);
    v_texcoord67 = vec4(a_texcoord6, a_texcoord7);

    // GPU_VS is distinct in the original material: expensive atmospheric
    // scattering is evaluated per vertex and interpolated to the pixel stage.
    if (u_skyUpMode.w > 0.5 && u_skyUpMode.w < 1.5)
    {
        vec3 worldView = safeNormalize(worldPos.xyz - u_cameraPosition.xyz,
            -worldNormal);
        calculateScattering(worldView, v_worldTangent, v_worldBitangent);
    }
    else
    {
        v_worldTangent = vec3(0.0, 0.0, 0.0);
        v_worldBitangent = vec3(0.0, 0.0, 0.0);
    }
}
