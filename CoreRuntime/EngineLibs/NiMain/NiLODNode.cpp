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

#include "NiLODNode.h"
#include "NiRangeLODData.h"
#include "NiCullingProcess.h"
#include "NiStream.h"

NiImplementRTTI(NiLODNode, NiSwitchNode, NiTypeMask::NiLODNode);

namespace
{
    bool GetOrderedLODChildren(const NiLODNode* pkLOD, unsigned int uiCount,
        NiAVObject** ppkOrderedChildren)
    {
        EE_ASSERT(pkLOD);
        EE_ASSERT(ppkOrderedChildren);

        if (pkLOD->GetArrayCount() != uiCount)
            return false;

        for (unsigned int i = 0; i < uiCount; i++)
        {
            char acExpectedName[32];
            NiSprintf(acExpectedName, sizeof(acExpectedName), "lodobj%d", i);

            NiAVObject* pkMatchedChild = NULL;
            for (unsigned int j = 0; j < uiCount; j++)
            {
                NiAVObject* pkChild = pkLOD->GetAt(j);
                if (!pkChild)
                    return false;

                const char* pcChildName = pkChild->GetName();
                if (pcChildName && NiStricmp(pcChildName, acExpectedName) == 0)
                {
                    if (pkMatchedChild)
                        return false;

                    pkMatchedChild = pkChild;
                }
            }

            if (!pkMatchedChild)
                return false;

            ppkOrderedChildren[i] = pkMatchedChild;
        }

        return true;
    }

    bool IsStoredFarToNear(const NiRangeLODData* pkRangeData)
    {
        EE_ASSERT(pkRangeData);

        const unsigned int uiRangeCount = pkRangeData->GetNumRanges();
        if (uiRangeCount < 2)
            return false;

        float fPrevNear;
        float fPrevFar;
        pkRangeData->GetRange(0, fPrevNear, fPrevFar);

        bool bSawDecrease = false;
        for (unsigned int i = 1; i < uiRangeCount; i++)
        {
            float fNear;
            float fFar;
            pkRangeData->GetRange(i, fNear, fFar);

            if (fNear > fPrevNear)
                return false;

            if (fNear < fPrevNear)
                bSawDecrease = true;

            fPrevNear = fNear;
        }

        return bSawDecrease;
    }

    void ReverseRangeOrder(NiRangeLODData* pkRangeData)
    {
        EE_ASSERT(pkRangeData);

        const unsigned int uiRangeCount = pkRangeData->GetNumRanges();
        for (unsigned int i = 0; i < uiRangeCount / 2; i++)
        {
            float fNearA;
            float fFarA;
            float fNearB;
            float fFarB;
            const unsigned int uiOther = uiRangeCount - 1 - i;

            pkRangeData->GetRange(i, fNearA, fFarA);
            pkRangeData->GetRange(uiOther, fNearB, fFarB);

            pkRangeData->SetRange(i, fNearB, fFarB);
            pkRangeData->SetRange(uiOther, fNearA, fFarA);
        }
    }
}

int  NiLODNode::ms_iGlobalLOD = -1;

//--------------------------------------------------------------------------------------------------
NiLODNode::NiLODNode()
{
    m_iIndex = 0;
    m_bLODActive = true;
}

//--------------------------------------------------------------------------------------------------
NiLODNode::~NiLODNode()
{
}

//--------------------------------------------------------------------------------------------------
void NiLODNode::UpdateWorldData()
{
    NiSwitchNode::UpdateWorldData();

    if (m_spLODData)
    {
        m_spLODData->UpdateWorldData(this);
    }
}

//--------------------------------------------------------------------------------------------------
void NiLODNode::PostLinkObject(NiStream& kStream)
{
    NiSwitchNode::PostLinkObject(kStream);

    NiRangeLODData* pkRangeData = NiDynamicCast(NiRangeLODData, m_spLODData);
    if (!pkRangeData)
        return;

    const unsigned int uiRangeCount = pkRangeData->GetNumRanges();
    if (uiRangeCount == 0 || uiRangeCount != GetArrayCount())
        return;

    NiAVObject** ppkOrderedChildren = NiAlloc(NiAVObject*, uiRangeCount);
    EE_ASSERT(ppkOrderedChildren);

    if (GetOrderedLODChildren(this, uiRangeCount, ppkOrderedChildren))
    {
        for (unsigned int i = 0; i < uiRangeCount; i++)
            m_kChildren.SetAt(i, ppkOrderedChildren[i]);
    }

    NiFree(ppkOrderedChildren);

    if (IsStoredFarToNear(pkRangeData))
        ReverseRangeOrder(pkRangeData);
}

//--------------------------------------------------------------------------------------------------
void NiLODNode::OnVisible(NiCullingProcess& kCuller)
{
    if (m_bLODActive)
    {
        if (m_spLODData)
        {
            // Check to see if there is a global LOD setting. If this is used
            // then override what the LOD Data has indicated.
            if (ms_iGlobalLOD >= 0)
            {
                m_iIndex = m_spLODData->GetLODIndex(ms_iGlobalLOD);
            }
            else
            {
                m_iIndex = m_spLODData->GetLODLevel(kCuller.GetLODCamera(), this);
            }

            // Scan backwards to make sure we aren't selecting past the end
            // of our children.
            while ((m_iIndex >= 0) && ((m_iIndex >= (int)m_kChildren.GetSize()) || (m_kChildren.GetAt(m_iIndex) == NULL)))
            {
                m_iIndex--;
            }
        }
    }

    NiSwitchNode::OnVisible(kCuller);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiLODNode);

//--------------------------------------------------------------------------------------------------
void NiLODNode::CopyMembers(NiLODNode* pkDest,
    NiCloningProcess& kCloning)
{
    NiSwitchNode::CopyMembers(pkDest, kCloning);

    if (m_spLODData)
        pkDest->SetLODData(m_spLODData->Duplicate());
    else
        pkDest->SetLODData(NULL);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiLODNode);

//--------------------------------------------------------------------------------------------------
void NiLODNode::LoadBinary(NiStream& kStream)
{
    NiSwitchNode::LoadBinary(kStream);

    // Read the Link ID for the LOD data.
    kStream.ReadLinkID();
}

//--------------------------------------------------------------------------------------------------
void NiLODNode::LinkObject(NiStream& kStream)
{
    NiSwitchNode::LinkObject(kStream);

    m_spLODData = (NiLODData*)kStream.GetObjectFromLinkID();
}

//--------------------------------------------------------------------------------------------------
bool NiLODNode::RegisterStreamables(NiStream& kStream)
{
    if (!NiSwitchNode::RegisterStreamables(kStream))
        return false;

    if (m_spLODData)
    {
        m_spLODData->RegisterStreamables(kStream);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiLODNode::SaveBinary(NiStream& kStream)
{
    NiSwitchNode::SaveBinary(kStream);

    kStream.SaveLinkID(m_spLODData);
}

//--------------------------------------------------------------------------------------------------
bool NiLODNode::IsEqual(NiObject* pkObject)
{
    if (!NiSwitchNode::IsEqual(pkObject))
        return false;

    NiLODNode* pkLOD = NiStaticCast(NiLODNode, pkObject);
    NiLODData* pkObjectData = pkLOD->GetLODData();

    // Check that both have LOD data
    if ((m_spLODData && !pkObjectData) || (!m_spLODData && pkObjectData))
        return false;

    // Both have LOD data of the correct type and they are
    // not equal
    if (m_spLODData && pkObjectData &&
        (m_spLODData->GetRTTI() == pkObjectData->GetRTTI()) &&
        !m_spLODData->IsEqual(pkObjectData))
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiLODNode::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiSwitchNode::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(NiLODNode::ms_RTTI.GetName()));

    if (m_spLODData)
    {
        m_spLODData->GetViewerStrings(pkStrings);
    }
    else
    {
        char cStr[16];
        NiSprintf(cStr, sizeof(cStr), "NULL LOD Data");
        pkStrings->Add(cStr);
    }
}

//--------------------------------------------------------------------------------------------------
