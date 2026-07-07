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

#include "NiCamera.h"
#include "NiRangeLODData.h"
#include "NiMath.h"
#include "NiLODNode.h"
#include "NiStream.h"

NiImplementRTTI(NiRangeLODData, NiLODData, NiTypeMask::NiRangeLODData);

namespace
{
    bool GetAggregateChildBoundCenter(NiLODNode* pkLOD, NiPoint3& kCenter)
    {
        EE_ASSERT(pkLOD);

        NiBound kMergedBound;
        bool bHasBound = false;

        for (unsigned int i = 0; i < pkLOD->GetArrayCount(); i++)
        {
            NiAVObject* pkChild = pkLOD->GetAt(i);
            if (!pkChild || !pkChild->IsVisualObject())
                continue;

            if (!bHasBound)
            {
                kMergedBound = pkChild->GetWorldBound();
                bHasBound = true;
            }
            else
            {
                kMergedBound.Merge(&pkChild->GetWorldBound());
            }
        }

        if (!bHasBound)
            return false;

        kCenter = kMergedBound.GetCenter();
        return true;
    }
}

//--------------------------------------------------------------------------------------------------
NiRangeLODData::NiRangeLODData()
{
    m_kCenter = NiPoint3::ZERO;
    m_uiNumRanges = 0;
    m_pkRanges = NULL;
}

//--------------------------------------------------------------------------------------------------
NiRangeLODData::~NiRangeLODData()
{
	if (m_pkRanges)
	{
		NiFree(m_pkRanges);
		m_pkRanges = nullptr;
	}
}

//--------------------------------------------------------------------------------------------------
void NiRangeLODData::UpdateWorldData(NiLODNode* pkLOD)
{
    EE_ASSERT(pkLOD);

    const NiTransform& kWorld = pkLOD->GetWorldTransform();

    if (m_kCenter == NiPoint3::ZERO &&
        GetAggregateChildBoundCenter(pkLOD, m_kWorldCenter))
    {
    }
    else
    {
        m_kWorldCenter = kWorld.m_Rotate * (kWorld.m_fScale * m_kCenter) +
            kWorld.m_Translate;
    }

    for (unsigned int i = 0; i < m_uiNumRanges; i++)
    {
		auto& kRange = m_pkRanges[i];
        kRange.m_fWorldNear = kWorld.m_fScale * kRange.m_fNear;
        kRange.m_fWorldFar = kWorld.m_fScale * kRange.m_fFar;
    }
}

//--------------------------------------------------------------------------------------------------
NiLODData* NiRangeLODData::Duplicate()
{
    NiRangeLODData* pkData = NiNew NiRangeLODData;
    EE_ASSERT(pkData);

    pkData->SetCenter(m_kCenter);
    pkData->SetNumRanges(m_uiNumRanges);

    unsigned int uiDestSize = sizeof(m_pkRanges[0]) * m_uiNumRanges;
    NiMemcpy(pkData->m_pkRanges, m_pkRanges, uiDestSize);

    return NiStaticCast(NiLODData, pkData);
}

//--------------------------------------------------------------------------------------------------
void NiRangeLODData::SetNumRanges(unsigned int uiNumRanges)
{
    // Check to see if the ranges has changed
    if (uiNumRanges == m_uiNumRanges)
        return;

    // Check for 0 ranges;
    if (uiNumRanges == 0)
    {
        NiFree(m_pkRanges);
        m_pkRanges = NULL;
        m_uiNumRanges = 0;
        return;
    }

    // Allocate the new ranges
    Range* pkNewRanges = NiAlloc(Range, uiNumRanges);
    EE_ASSERT(pkNewRanges);

    // Copy over the old range Values
    if (m_pkRanges)
    {
        unsigned int uiDestSize = sizeof(pkNewRanges[0]) * NiMin((int)m_uiNumRanges, (int)uiNumRanges);
        NiMemcpy(pkNewRanges, m_pkRanges, uiDestSize);
    }

    // Delete the old ranges
    NiFree(m_pkRanges);

    m_pkRanges = pkNewRanges;
    m_uiNumRanges = uiNumRanges;
}

//--------------------------------------------------------------------------------------------------
int NiRangeLODData::GetLODLevel(const NiCamera* pkCamera, NiLODNode* pkLOD) const
{
    EE_ASSERT(pkCamera);
    EE_ASSERT(pkLOD);

    NiPoint3 kWorldCenter = m_kWorldCenter;
    if (m_kCenter == NiPoint3::ZERO)
        GetAggregateChildBoundCenter(pkLOD, kWorldCenter);

    NiPoint3 kDiff = kWorldCenter - pkCamera->GetWorldLocation();
    kDiff.y = 0.0f;
    float fDist = NiAbs(kDiff.Length() * pkCamera->GetLODAdjust());

    for (unsigned int iLODLevel = 0; iLODLevel < m_uiNumRanges; iLODLevel++)
    {
        auto& kRange = m_pkRanges[iLODLevel];
        if ((fDist >= kRange.m_fWorldNear) && (fDist <= kRange.m_fWorldFar))
            return GetLODIndex(iLODLevel);
    }

    return -1;
}

//--------------------------------------------------------------------------------------------------
int NiRangeLODData::GetLODIndex(int iLODLevel) const
{
    if (iLODLevel < 0 || m_uiNumRanges == 0)
        return -1;
    return NiClamp(iLODLevel, -1, m_uiNumRanges - 1);
}

//--------------------------------------------------------------------------------------------------
// cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiRangeLODData);

//--------------------------------------------------------------------------------------------------
void NiRangeLODData::CopyMembers(NiRangeLODData* pkDest,
    NiCloningProcess&)
{
    EE_ASSERT(pkDest);

    // Set the Center use for all LOD Levels
    pkDest->m_kCenter = m_kCenter;

    pkDest->m_kWorldCenter = m_kWorldCenter;

    // Duplicate the Ranges
    pkDest->SetNumRanges(m_uiNumRanges);

    unsigned int uiDestSize = sizeof(m_pkRanges[0]) * m_uiNumRanges;
    NiMemcpy(pkDest->m_pkRanges, m_pkRanges, uiDestSize);

}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiRangeLODData);

//--------------------------------------------------------------------------------------------------
void NiRangeLODData::LoadBinary(NiStream& kStream)
{
    NiLODData::LoadBinary(kStream);

    m_kCenter.LoadBinary(kStream);

    unsigned int uiRanges;
    NiStreamLoadBinary(kStream, uiRanges);

    SetNumRanges(uiRanges);

    for (unsigned int i = 0; i < uiRanges; i++)
    {
        NiStreamLoadBinary(kStream, m_pkRanges[i].m_fNear);
        NiStreamLoadBinary(kStream, m_pkRanges[i].m_fFar);
    }
}

//--------------------------------------------------------------------------------------------------
void NiRangeLODData::LinkObject(NiStream& kStream)
{
    NiLODData::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiRangeLODData::RegisterStreamables(NiStream& kStream)
{
    return NiLODData::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiRangeLODData::SaveBinary(NiStream& kStream)
{
    NiLODData::SaveBinary(kStream);

    m_kCenter.SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uiNumRanges);

    for (unsigned int i = 0; i < m_uiNumRanges; i++)
    {
        NiStreamSaveBinary(kStream, m_pkRanges[i].m_fNear);
        NiStreamSaveBinary(kStream, m_pkRanges[i].m_fFar);
    }
}

//--------------------------------------------------------------------------------------------------
bool NiRangeLODData::IsEqual(NiObject* pkObject)
{
    NiRangeLODData* pkLOD = NiStaticCast(NiRangeLODData, pkObject);
    if (!pkLOD)
        return false;

    if (m_uiNumRanges != pkLOD->m_uiNumRanges)
        return false;

    for (unsigned int i = 0; i < m_uiNumRanges; i++)
    {
		auto& kRange1 = m_pkRanges[i];
		auto& kRange2 = pkLOD->m_pkRanges[i];
        if ((kRange2.m_fNear != kRange1.m_fNear) || (kRange2.m_fFar != kRange1.m_fFar))
            return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiRangeLODData::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    pkStrings->Add(NiGetViewerString(NiRangeLODData::ms_RTTI.GetName()));
    pkStrings->Add(m_kCenter.GetViewerString("m_kCenter"));
    pkStrings->Add(m_kWorldCenter.GetViewerString("m_kWorldCenter"));

    for (unsigned int i = 0; i < m_uiNumRanges; i++)
    {
        char* pcString = NiAlloc(char, 128);
        NiSprintf(pcString, 128, "range[%d] = %g   %g", i, m_pkRanges[i].m_fNear, m_pkRanges[i].m_fFar);
        pkStrings->Add(pcString);
    }
}

//--------------------------------------------------------------------------------------------------
