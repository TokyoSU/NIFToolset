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

//--------------------------------------------------------------------------------------------------
inline void NiTerrainSectorSelectorPager::ClearPagers()
{
    m_kPagers.clear();
}

//--------------------------------------------------------------------------------------------------
inline NiTerrainSectorSelectorPager::SectorRequest::SectorRequest()
    : m_iTargetLOD(-1)
    , m_fPriorityMetric(FLT_MAX)
{
}

//--------------------------------------------------------------------------------------------------
inline NiTerrainSectorSelectorPager::Pager::Pager()
    : m_bIsActive(true)
    , m_bIsDirty(true)
    , m_bIsDynamic(false)
    , m_fPriority(0.0f)
{    
}

//--------------------------------------------------------------------------------------------------
inline NiTerrainSectorSelectorPager::Pager::~Pager()
{
}

//--------------------------------------------------------------------------------------------------
inline bool NiTerrainSectorSelectorPager::Pager::IsActive() const
{
    return m_bIsActive;
}

//--------------------------------------------------------------------------------------------------
inline bool NiTerrainSectorSelectorPager::Pager::IsDynamic() const
{
    return m_bIsDynamic;
}

//--------------------------------------------------------------------------------------------------
inline bool NiTerrainSectorSelectorPager::Pager::IsDirty() const
{
    return m_bIsDirty;
}

//--------------------------------------------------------------------------------------------------
inline bool NiTerrainSectorSelectorPager::Pager::IsModeDirty() const
{
    return m_bModeChanged;
}

//--------------------------------------------------------------------------------------------------
inline efd::Float32 NiTerrainSectorSelectorPager::Pager::GetPriority() const
{
    return m_fPriority;
}

//--------------------------------------------------------------------------------------------------
inline const NiTerrainSectorSelectorPager::SectorRequestSet& 
    NiTerrainSectorSelectorPager::Pager::GetRequests() const
{
    return m_kSectorRequests;
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainSectorSelectorPager::Pager::CalculateRequests()
{
    CalculateRequests_Internal();
    m_bIsDirty = false;
    m_bModeChanged = false;
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainSectorSelectorPager::Pager::AppendRequest(efd::SInt16 iSectorX, 
    efd::SInt16 iSectorY, efd::SInt32 iTargetLOD, efd::Float32 fPriority)
{
    NiTerrainSector::SectorID kSectorID;
    NiTerrainSector::GenerateSectorID(iSectorX, iSectorY, kSectorID);

    // Update the existing requests
    SectorRequest& kRequest = m_kSectorRequests[kSectorID];
    kRequest.m_fPriorityMetric = efd::Min(kRequest.m_fPriorityMetric, fPriority);
    kRequest.m_iTargetLOD = efd::Max(kRequest.m_iTargetLOD, iTargetLOD);
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainSectorSelectorPager::Pager::ClearRequests()
{
    m_kSectorRequests.clear();
}

//--------------------------------------------------------------------------------------------------
