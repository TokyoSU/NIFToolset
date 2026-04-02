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
inline NiAVObject* NiPSysMeshUpdateModifier::GetMeshAt(unsigned int uiIndex)
    const
{
    if (uiIndex < m_kMeshes.GetSize())
    {
        return m_kMeshes.GetAt(uiIndex);
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
inline unsigned int NiPSysMeshUpdateModifier::GetMeshCount() const
{
    return m_kMeshes.GetSize();
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
