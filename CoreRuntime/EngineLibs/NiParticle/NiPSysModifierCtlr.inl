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

#include "NiParticleSystem.h"

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING

//--------------------------------------------------------------------------------------------------
inline const NiFixedString& NiPSysModifierCtlr::GetModifierName() const
{
    return m_kModifierName;
}

//--------------------------------------------------------------------------------------------------
inline NiPSysModifier* NiPSysModifierCtlr::GetModifierPointer() const
{
    return m_pkModifier;
}

//--------------------------------------------------------------------------------------------------
inline void NiPSysModifierCtlr::GetModifierPointerFromName()
{
    EE_ASSERT(m_pkTarget && m_kModifierName.Exists());
    m_pkModifier = ((NiParticleSystem*) m_pkTarget)->GetModifierByName(
        m_kModifierName);
    EE_ASSERT(m_pkModifier);
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
