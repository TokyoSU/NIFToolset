// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include "NiEnvironmentPCH.h"
#include "NiAtmosphere.h"

#include <NiShaderFactory.h>
#include <NiSingleShaderMaterial.h>
#include <NiStencilProperty.h>
#include <NiZBufferProperty.h>

#include "NiSkyMaterial.h"

//---------------------------------------------------------------------------
NiImplementRTTI(NiAtmosphere, NiNode);

//---------------------------------------------------------------------------
NiAtmosphere::NiAtmosphere()
{  
    // Initialise the settings to their defaults:
    InitialiseExtraData();
    LoadDefaultConfiguration();
}

//---------------------------------------------------------------------------
NiAtmosphere::~NiAtmosphere()
{    
}

//---------------------------------------------------------------------------
void NiAtmosphere::DoUpdate(NiUpdateProcess& kUpdate)
{
    EE_UNUSED_ARG(kUpdate);

    // Update the sky parameters over time
    if (m_bAtmosphereSettingsChanged)
    {
        UpdateShaderConstants();
        m_bAtmosphereSettingsChanged = false;
    }
    UpdateShaderVariables();
}

//---------------------------------------------------------------------------
void NiAtmosphere::UpdateDownwardPass(NiUpdateProcess& kUpdate)
{        
    DoUpdate(kUpdate);
    NiNode::UpdateDownwardPass(kUpdate); 
}

//---------------------------------------------------------------------------
void NiAtmosphere::UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate)
{    
    if (GetSelectiveUpdateTransforms())
    {
        DoUpdate(kUpdate);
    }

    NiNode::UpdateSelectedDownwardPass(kUpdate);
}

//---------------------------------------------------------------------------
void NiAtmosphere::UpdateRigidDownwardPass(NiUpdateProcess& kUpdate)
{
    if (GetSelectiveUpdateTransforms())
    {
        DoUpdate(kUpdate);
    } 
    
    NiNode::UpdateRigidDownwardPass(kUpdate);
}

//---------------------------------------------------------------------------
void NiAtmosphere::InitialiseExtraData()
{
    float afTemp[4] = {0,0,0,0};

    m_spAtmosphericScatteringConstants = NiNew NiFloatsExtraData(4,afTemp);
    m_spRGBInvWavelength4 = NiNew NiFloatsExtraData(3,afTemp);
    m_spHDRExposure = NiNew NiFloatExtraData(0.0f);
    m_spUpVector = NiNew NiFloatsExtraData(3,afTemp);
    m_spPlanetDimensions = NiNew NiFloatsExtraData(4,afTemp);
    m_spScaleDepth = NiNew NiFloatsExtraData(4,afTemp);
    m_spFrameData = NiNew NiFloatsExtraData(4,afTemp);
    m_spNumSamplesFloat = NiNew NiFloatExtraData(0.0f);;
    m_spNumSamplesInt = NiNew NiIntegerExtraData(0);
}

//---------------------------------------------------------------------------
void NiAtmosphere::AttachExtraData(NiAVObject* pkObject)
{
    pkObject->AddExtraData(NiSkyMaterial::SC_ATMOSPHERICSCATTERINGCONST, 
        m_spAtmosphericScatteringConstants);
    pkObject->AddExtraData(NiSkyMaterial::SC_RGBINVWAVELENGTH4, 
        m_spRGBInvWavelength4);
    pkObject->AddExtraData(NiSkyMaterial::SC_HDREXPOSURE,
        m_spHDRExposure);
    pkObject->AddExtraData(NiSkyMaterial::SC_UPVECTOR, 
        m_spUpVector);
    pkObject->AddExtraData(NiSkyMaterial::SC_PLANETDIMENSIONS, 
        m_spPlanetDimensions);
    pkObject->AddExtraData(NiSkyMaterial::SC_SCALEDEPTH, 
        m_spScaleDepth);
    pkObject->AddExtraData(NiSkyMaterial::SC_FRAMEDATA, 
        m_spFrameData);
    pkObject->AddExtraData(NiSkyMaterial::SC_NUMSAMPLESINT, 
        m_spNumSamplesInt);
    pkObject->AddExtraData(NiSkyMaterial::SC_NUMSAMPLESFLOAT, 
        m_spNumSamplesFloat);
}

//---------------------------------------------------------------------------
void NiAtmosphere::DetachExtraData(NiAVObject* pkObject)
{
    pkObject->RemoveExtraData(NiSkyMaterial::SC_ATMOSPHERICSCATTERINGCONST); 
    pkObject->RemoveExtraData(NiSkyMaterial::SC_RGBINVWAVELENGTH4); 
    pkObject->RemoveExtraData(NiSkyMaterial::SC_HDREXPOSURE);
    pkObject->RemoveExtraData(NiSkyMaterial::SC_UPVECTOR); 
    pkObject->RemoveExtraData(NiSkyMaterial::SC_PLANETDIMENSIONS); 
    pkObject->RemoveExtraData(NiSkyMaterial::SC_SCALEDEPTH); 
    pkObject->RemoveExtraData(NiSkyMaterial::SC_FRAMEDATA); 
    pkObject->RemoveExtraData(NiSkyMaterial::SC_NUMSAMPLESINT); 
    pkObject->RemoveExtraData(NiSkyMaterial::SC_NUMSAMPLESFLOAT);
}

//---------------------------------------------------------------------------
void NiAtmosphere::LoadDefaultConfiguration()
{ 
    // Set the sky settings to their defaults
    m_bAtmosphereSettingsChanged = true;

    // Default algorithm config:
    m_uiNumSamples = 5;

    // Geometry of Atmosphere:
    m_kUpVector = NiPoint3::UNIT_Z;
    m_fPlanetRadius = 10.0f;
    m_fAtmosphereRadius = 10.25f;
    m_fScaleDepth = 0.25; // Altitude % where atmosphere has average density

    // Default RGB wavelengths
    m_kRGBWavelengths = NiPoint3 (
        0.650f, // (um) RED 
        0.570f, // (um) GREEN
        0.475f); // (um) BLUE

    // Default scattering constants
    m_fRayleighConstant = 0.0025f;
    m_fMieConstant = 0.0015f;

    // Configure HG function
    m_fPhaseConstant = -0.99f;

    // Default Sun Intensity
    m_fSunIntensity = 15.0f; 

    // Default Sun Size
    m_fSunSizeMultiplier = 1.0f; 

    // Default HDR Exposure value
    m_fHDRExposure = 2.0f;
}

//---------------------------------------------------------------------------
void NiAtmosphere::UpdateShaderConstants()
{ 
    // These values only ever change when the sky settings change. 

    // Algorithm constants:
    m_spNumSamplesFloat->SetValue((float)m_uiNumSamples);
    m_spNumSamplesInt->SetValue(m_uiNumSamples);

    // Geometrical/Planetary Constants:
    m_spUpVector->SetValue(0, m_kUpVector.x);
    m_spUpVector->SetValue(1, m_kUpVector.y);
    m_spUpVector->SetValue(2, m_kUpVector.z);
    // Radius of Planet and Atmosphere
    m_spPlanetDimensions->SetValue(0, m_fAtmosphereRadius);
    m_spPlanetDimensions->SetValue(1, NiSqr(m_fAtmosphereRadius));
    m_spPlanetDimensions->SetValue(2, m_fPlanetRadius);
    m_spPlanetDimensions->SetValue(3, NiSqr(m_fPlanetRadius));
    // Scale calculations of the radiai
    float fScale = 1 / (m_fAtmosphereRadius - m_fPlanetRadius);
    m_spScaleDepth->SetValue(0, fScale);
    m_spScaleDepth->SetValue(1, m_fScaleDepth);
    m_spScaleDepth->SetValue(2, fScale / m_fScaleDepth);
    m_spScaleDepth->SetValue(3, m_fSunSizeMultiplier);

    // Scattering Constants
    m_spAtmosphericScatteringConstants->SetValue(0, // fKrESun
        m_fRayleighConstant * m_fSunIntensity); 
    m_spAtmosphericScatteringConstants->SetValue(1, // fKmESun
        m_fMieConstant * m_fSunIntensity); 
    m_spAtmosphericScatteringConstants->SetValue(2, // fKr4PI
        m_fRayleighConstant * 4.0f * NI_PI); 
    m_spAtmosphericScatteringConstants->SetValue(3, // fKm4PI
        m_fMieConstant * 4.0f * NI_PI); 

    // RGB Wavelength values:
    NiPoint3 kRGBInvWavelength4;
    kRGBInvWavelength4.x = 1.0f / NiPow(m_kRGBWavelengths.x, 4.0f);
    kRGBInvWavelength4.y = 1.0f / NiPow(m_kRGBWavelengths.y, 4.0f);
    kRGBInvWavelength4.z = 1.0f / NiPow(m_kRGBWavelengths.z, 4.0f);
    m_spRGBInvWavelength4->SetValue(0, kRGBInvWavelength4.x);
    m_spRGBInvWavelength4->SetValue(1, kRGBInvWavelength4.y);
    m_spRGBInvWavelength4->SetValue(2, kRGBInvWavelength4.z);
    
    // HDR Exposure
    m_spHDRExposure->SetValue(m_fHDRExposure);
}

//---------------------------------------------------------------------------
void NiAtmosphere::UpdateShaderVariables()
{
    // These values change according to the position of the scene camera,
    // and the sun's position in the sky.

    m_spFrameData->SetValue(0, m_fPhaseConstant);
    m_spFrameData->SetValue(1, m_fPhaseConstant * m_fPhaseConstant);

    // Update the camera's height above the surface
    float fCameraHeight = 0.00f;
    m_spFrameData->SetValue(2, fCameraHeight);
    m_spFrameData->SetValue(3, fCameraHeight * fCameraHeight);    
}

//---------------------------------------------------------------------------
NiColor NiAtmosphere::CalculateToneMapping(const NiColor& kInput)
{
    NiColor kResult;

    kResult.r = 1.0f - NiExp(kInput.r * -m_fHDRExposure);
    kResult.g = 1.0f - NiExp(kInput.g * -m_fHDRExposure);
    kResult.b = 1.0f - NiExp(kInput.b * -m_fHDRExposure);

    return kResult;
}

//---------------------------------------------------------------------------
NiColor NiAtmosphere::CalculateSkyColor(const NiPoint3& kRayleighScattering,
    const NiPoint3& kMieScattering, const NiPoint3& kDirection, 
    const NiPoint3& kSunDirection)
{
    float fCos = kSunDirection.Dot(kDirection) / kDirection.Length();
    float fG = m_fPhaseConstant;
    float fG2 = fG * fG;

    float fRayleighPhase = 0.75f * (1.0f + 1.0f * fCos*fCos);
    float fMiePhase = 1.5f * ((1.0f - fG2) / (2.0f + fG2)) * 
        (m_fSunSizeMultiplier * fCos*fCos) / 
        pow(1.0f + fG2 - 2.0f*fG*fCos, 1.5f);

    NiColor kResult;
    kResult.r = fRayleighPhase * kRayleighScattering.x + 
        fMiePhase * kMieScattering.x;
    kResult.g = fRayleighPhase * kRayleighScattering.y + 
        fMiePhase * kMieScattering.y;
    kResult.b = fRayleighPhase * kRayleighScattering.z + 
        fMiePhase * kMieScattering.z;
    return kResult;
}

//---------------------------------------------------------------------------
void NiAtmosphere::CalculateScattering(const NiPoint3& kDirection, 
    const NiPoint3& kSunDirection, NiPoint3& kRayleighScattering, 
    NiPoint3& kMieScattering)
{
    // Split the constants up
    float fOuterRadius = m_spPlanetDimensions->GetValue(0);
    float fOuterRadius2 = m_spPlanetDimensions->GetValue(1);
    float fInnerRadius = m_spPlanetDimensions->GetValue(2);
    float fInnerRadius2 = m_spPlanetDimensions->GetValue(3);

    float fKrESun = m_spAtmosphericScatteringConstants->GetValue(0);
    float fKmESun = m_spAtmosphericScatteringConstants->GetValue(1);
    float fKr4PI = m_spAtmosphericScatteringConstants->GetValue(2);
    float fKm4PI = m_spAtmosphericScatteringConstants->GetValue(3);

    float fScale = m_spScaleDepth->GetValue(0);
    float fScaleDepth = m_spScaleDepth->GetValue(1);
    float fScaleOverScaleDepth = m_spScaleDepth->GetValue(2);

    // Convert our planar representation into a celestial/spherical 
    // representation (calculate the far point of the ray passing 
    // through the atmosphere)
    float fCosTheta = kDirection.Dot(-m_kUpVector);
    float fFar = CalculateDistanceToEdgeOfAtmosphere(
        fCosTheta,
        fOuterRadius, fOuterRadius2, 
        fInnerRadius, fInnerRadius2);
    
    // for atmosphere's sake, position can be a single position on
    // the planet, and the sun with orbit over it
    float fCameraHeight = fInnerRadius;
    NiPoint3 kPosCamera = m_kUpVector * fCameraHeight;
    
    // Calculate the closest intersection of the ray with the outer 
    // atmosphere (which is the near point of the ray passing through 
    // the atmosphere)
    float fNear = 0.0f;
    
    // Calculate the ray's starting position, then calculate its 
    // scattering offset
    NiPoint3 kPosScatterStart = kPosCamera + kDirection * fNear;
    float fHeight = kPosScatterStart.Length();
    float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fCameraHeight));
    float fStartAngle = kDirection.Dot(kPosScatterStart) / fHeight;
    float fStartOffset = fDepth * 
        InternalScaleFunction(fStartAngle, fScaleDepth);
    
    // Initialize the scattering loop variables
    float fSampleLength = fFar / float(m_uiNumSamples);
    float fScaledLength = fSampleLength * fScale;
    NiPoint3 kDirSample = kDirection * fSampleLength;
    NiPoint3 kPosSample = kPosScatterStart + kDirSample * 0.5;

    // Now loop through the sample rays
    NiPoint3 kColScatter = NiPoint3(0.0, 0.0, 0.0);
    for (NiUInt32 ui = 0; ui < m_uiNumSamples; ui++)
    {
        fHeight = kPosSample.Length();
        fDepth = NiExp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
        
        float fLightAngle = kPosSample.Dot(-kSunDirection) / fHeight;
        float fCameraAngle = kDirection.Dot(kPosSample) / fHeight;
        float fScatter = (fStartOffset + fDepth * 
            (InternalScaleFunction(fLightAngle, fScaleDepth) - 
            InternalScaleFunction(fCameraAngle, fScaleDepth)));
        
        NiPoint3 kColAttenuate;
        kColAttenuate.x = NiExp(-fScatter * 
            (m_spRGBInvWavelength4->GetValue(0) * fKr4PI + fKm4PI));
        kColAttenuate.y = NiExp(-fScatter * 
            (m_spRGBInvWavelength4->GetValue(1) * fKr4PI + fKm4PI));
        kColAttenuate.z = NiExp(-fScatter * 
            (m_spRGBInvWavelength4->GetValue(2) * fKr4PI + fKm4PI));
        kColScatter += kColAttenuate * (fDepth * fScaledLength);
        kPosSample += kDirSample;
    }

    // Finally, scale the Mie and Rayleigh colors and set up 
    // the varying variables for the pixel shader
    kMieScattering.x = kColScatter.x * fKmESun;
    kMieScattering.y = kColScatter.y * fKmESun;
    kMieScattering.z = kColScatter.z * fKmESun;

    kRayleighScattering.x = 
        kColScatter.x * (m_spRGBInvWavelength4->GetValue(0) * fKrESun);    
    kRayleighScattering.y = 
        kColScatter.y * (m_spRGBInvWavelength4->GetValue(1) * fKrESun);    
    kRayleighScattering.z = 
        kColScatter.z * (m_spRGBInvWavelength4->GetValue(2) * fKrESun);    
}

//---------------------------------------------------------------------------
NiColor NiAtmosphere::GetSkyColorInDirection(const NiPoint3& kDirection, 
    const NiPoint3& kSunDirection, bool bIncludeMieScattering)
{
    NiPoint3 kRayleighScattering;
    NiPoint3 kMieScattering;

    // Calculate scattering in that direction
    CalculateScattering(kDirection, kSunDirection, kRayleighScattering, 
        kMieScattering);

    // Remove mie scattering from equation if required
    if (!bIncludeMieScattering)
        kMieScattering = NiPoint3::ZERO;

    // Calculate final color in that direction;
    NiColor kSkyColor = CalculateSkyColor(kRayleighScattering, kMieScattering, 
        kDirection, kSunDirection);

    // Perform HDR tone mapping on final output
    return CalculateToneMapping(kSkyColor);
}

//---------------------------------------------------------------------------
float NiAtmosphere::CalculateDistanceToEdgeOfAtmosphere(float fCosTheta, 
    float fRA, float fRA2, float fRE, float fRE2)
{
    EE_UNUSED_ARG(fRA);

    // Convert our planar representation into a celestial/spherical 
    // representation (calculate the far point of the ray passing 
    // through the atmosphere)
    return ((2 * fRE * fCosTheta) + sqrt( pow((-2 * fRE * fCosTheta),2) - 
        (4 * (fRE2 - fRA2))))/2;
}

//---------------------------------------------------------------------------
float NiAtmosphere::InternalScaleFunction(float fCos, float fScaleDepth)
{
    // As defined in GPU Gems 2 by Sean O'neil
    float x = 1.0f - fCos;
    return fScaleDepth * 
        NiExp(-0.00287f + x*(0.459f + x*(3.83f + x*(-6.80f + x*5.25f))));
}

//---------------------------------------------------------------------------