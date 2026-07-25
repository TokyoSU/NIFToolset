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
inline NiMultiTargetTransformController::NiMultiTargetTransformController() :
    m_usLegacyExtraTargetCount(0), m_ppkLegacyExtraTargets(NULL)
{
}

//--------------------------------------------------------------------------------------------------
inline unsigned short
NiMultiTargetTransformController::GetLegacyExtraTargetCount() const
{
    return m_usLegacyExtraTargetCount;
}

//--------------------------------------------------------------------------------------------------
inline NiAVObject* NiMultiTargetTransformController::GetLegacyExtraTargetAt(
    unsigned short usIndex) const
{
    return usIndex < m_usLegacyExtraTargetCount && m_ppkLegacyExtraTargets
        ? m_ppkLegacyExtraTargets[usIndex] : NULL;
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
