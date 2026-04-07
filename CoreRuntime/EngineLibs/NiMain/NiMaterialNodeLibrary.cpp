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

// Precompiled Header
#include "NiMainPCH.h"

#include "NiMaterialNodeLibrary.h"

//--------------------------------------------------------------------------------------------------
NiMaterialNodeLibrary::NiMaterialNodeLibrary(unsigned short usVersion)
{
    m_usVersion = usVersion;
    m_kNodes.SetSize(100);
}

//--------------------------------------------------------------------------------------------------
unsigned short NiMaterialNodeLibrary::GetVersion() const
{
    return m_usVersion;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiMaterialNodeLibrary::GetNodeCount() const
{
    return m_kNodes.GetSize();
}

//--------------------------------------------------------------------------------------------------
NiMaterialNode* NiMaterialNodeLibrary::GetAttachableNodeByName(
    const NiFixedString& kName, bool bClone)
{
    for (unsigned int ui = 0; ui < GetNodeCount(); ui++)
    {
        NiMaterialNode* pkFrag = GetNode(ui);
        if (pkFrag && pkFrag->GetName() == kName)
        {
            if (bClone)
                return pkFrag->Clone();
            else
                return pkFrag;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiMaterialNode* NiMaterialNodeLibrary::GetNode(unsigned int uiNode)
{
    EE_ASSERT(uiNode < GetNodeCount());
    return m_kNodes.GetAt(uiNode);
}

//--------------------------------------------------------------------------------------------------
void NiMaterialNodeLibrary::AddNode(NiMaterialNode* pkFrag)
{
    EE_ASSERT(pkFrag);
    m_kNodes.Add(pkFrag);
}

//--------------------------------------------------------------------------------------------------
