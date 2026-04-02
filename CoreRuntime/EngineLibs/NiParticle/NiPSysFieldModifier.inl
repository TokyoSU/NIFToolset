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
inline NiAVObject* NiPSysFieldModifier::GetFieldObj() const
{
    return m_pkFieldObj;
}

//--------------------------------------------------------------------------------------------------
inline float NiPSysFieldModifier::GetMagnitude() const
{
    return m_fMagnitude;
}

//--------------------------------------------------------------------------------------------------
inline float NiPSysFieldModifier::GetAttenuation() const
{
    return m_fAttenuation;
}

//--------------------------------------------------------------------------------------------------
inline bool NiPSysFieldModifier::GetUseMaxDistance() const
{
    return m_bUseMaxDistance;
}

//--------------------------------------------------------------------------------------------------
inline float NiPSysFieldModifier::GetMaxDistance() const
{
    return m_fMaxDistance;
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
