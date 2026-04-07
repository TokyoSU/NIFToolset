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

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING

//--------------------------------------------------------------------------------------------------
inline NiAVObject* NiPSysGravityModifier::GetGravityObj() const
{
    return m_pkGravityObj;
}

//--------------------------------------------------------------------------------------------------
inline const NiPoint3& NiPSysGravityModifier::GetGravityAxis() const
{
    return m_kGravityAxis;
}

//--------------------------------------------------------------------------------------------------
inline float NiPSysGravityModifier::GetDecay() const
{
    return m_fDecay;
}

//--------------------------------------------------------------------------------------------------
inline float NiPSysGravityModifier::GetStrength() const
{
    return m_fStrength;
}

//--------------------------------------------------------------------------------------------------
inline NiPSysGravityModifier::ForceType NiPSysGravityModifier::GetType() const
{
    return m_eType;
}

//--------------------------------------------------------------------------------------------------
inline float NiPSysGravityModifier::GetTurbulence()
{
    return m_fTurbulence;
}

//--------------------------------------------------------------------------------------------------
inline float NiPSysGravityModifier::GetTurbulenceScale()
{
    return m_fScale;
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
