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

//---------------------------------------------------------------------------
inline void NiEnvironment::SetAutoCalcFogColor(bool bEnable)
{
    m_bAutoCalcFogColor = bEnable;
}

//---------------------------------------------------------------------------
inline bool NiEnvironment::GetAutoCalcFogColor()
{
    return m_bAutoCalcFogColor;
}

//---------------------------------------------------------------------------
inline void NiEnvironment::SetAutoSetBackgroundColor(bool bEnable)
{
    m_bAutoSetBackgroundColor = bEnable;
}

//---------------------------------------------------------------------------
inline bool NiEnvironment::GetAutoSetBackgroundColor()
{
    return m_bAutoSetBackgroundColor;
}

//--------------------------------------------------------------------------------------------------
inline void NiEnvironment::SetSunElevationAngle(float fAngle)
{
    // Adjust angle so 0-180 is above the horizon and 180-360 is below
    fAngle -= 180.0f;

    NiInt32 fullCircle = (NiInt32)(fAngle / 360.0f);
    fAngle = (fAngle - fullCircle * 360.0f) * NI_TWO_PI / 360.0f;
    
    if (fAngle < 0.0f)
        fAngle = NI_TWO_PI + fAngle;

    m_fSunElevationAngle = NiClamp(fAngle, 0.0f, NI_TWO_PI);
    m_bSunSettingsChanged = true;
}

//--------------------------------------------------------------------------------------------------
inline float NiEnvironment::GetSunElevationAngle()
{
    return m_fSunElevationAngle;
}

//--------------------------------------------------------------------------------------------------
inline void NiEnvironment::SetSunAzimuthAngle(float fAngle)
{
    NiInt32 fullCircle = (NiInt32)(fAngle / 360.0f);
    fAngle = (fAngle - fullCircle * 360.0f) * NI_TWO_PI / 360.0f;
    
    if (fAngle < 0.0f)
        fAngle = NI_TWO_PI + fAngle;

    m_fSunAzimuthAngle = NiClamp(fAngle, 0.0f, NI_TWO_PI);
    m_bSunSettingsChanged = true;
}

//--------------------------------------------------------------------------------------------------
inline float NiEnvironment::GetSunAzimuthAngle()
{
    return m_fSunAzimuthAngle;
}

//--------------------------------------------------------------------------------------------------
inline void NiEnvironment::SetUseSunAngles(bool bUseAngles)
{
    m_bUseSunAnglesRotation = bUseAngles;
}

//--------------------------------------------------------------------------------------------------
inline bool NiEnvironment::GetUseSunAngles()
{
    return m_bUseSunAnglesRotation;
}

//--------------------------------------------------------------------------------------------------
inline void NiEnvironment::SetFogColor(NiColorA kColor)
{
    NiFogProperty* pkFog = GetFogProperty();
    EE_ASSERT(pkFog);
    NiColor kFogColor(kColor.r, kColor.g, kColor.b);
    pkFog->SetFogColor(kFogColor);
}

//---------------------------------------------------------------------------