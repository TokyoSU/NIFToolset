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

#include "NiTerrainPCH.h"
#include "NiTerrain.h"
#include "NiTerrainSectorSelectorPager.h"

#include <algorithm>

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainSectorSelectorPager, NiTerrainSectorSelector);
//--------------------------------------------------------------------------------------------------
NiTerrainSectorSelectorPager::NiTerrainSectorSelectorPager(NiTerrain* pkTerrain):
    NiTerrainSectorSelector(pkTerrain)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorSelectorPager::~NiTerrainSectorSelectorPager()
{
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorSelectorPager::RegisterPager(Pager* pkPager)
{
    m_kPagers.push_back(pkPager);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorSelectorPager::DeregisterPager(Pager* pkPager)
{
    m_kPagers.erase(
        std::remove(m_kPagers.begin(), m_kPagers.end(), pkPager), 
        m_kPagers.end());
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorSelectorPager::UpdateSectorSelection()
{
    PagerList::iterator kIter;

    // Check for any static regions that might be dirty
    bool bIsStaticDataDirty = false;
    for (kIter = m_kPagers.begin(); kIter != m_kPagers.end(); ++kIter)
    {
        Pager* pkPager = *kIter;
        bIsStaticDataDirty |= (!pkPager->IsDynamic() && pkPager->IsDirty() ||
            pkPager->IsModeDirty());
    }

    // Initialize the static region array if necessary
    if (bIsStaticDataDirty)
        m_kStaticSectorRequests.clear();

    // Fetch the latest sector requests from the pagers
    SectorRequestSet kDynamicRequests;
    for (kIter = m_kPagers.begin(); kIter != m_kPagers.end(); ++kIter)
    {
        // Check if the pager is active
        Pager* pkPager = *kIter;
        if (!pkPager->IsActive())
            continue;

        // Determine if the pager is static or dynamic
        SectorRequestSet* pkRequestSet = &kDynamicRequests;
        if (!pkPager->IsDynamic())
        {
            if (!bIsStaticDataDirty)
                continue;

            pkRequestSet = &m_kStaticSectorRequests;
        }

        // Calculate this pager's set of requests
        if (pkPager->IsDynamic() || pkPager->IsDirty())
            pkPager->CalculateRequests();
        const SectorRequestSet& kPagerRequests = pkPager->GetRequests();

        // Merge the requests into the parent set
        MergeSectorSet(*pkRequestSet, kPagerRequests, pkPager->GetPriority());
    }

    // Merge dynamic and static requests together
    MergeSectorSet(kDynamicRequests, m_kStaticSectorRequests);

    // Work out what requests need to be added to remove unnecessary sectors and not request
    // sectors that are already available.
    return GenerateStreamingRequests(kDynamicRequests);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorSelectorPager::SortByPriority(const CompleteSectorRequest& kRequestA, 
    const CompleteSectorRequest& kRequestB)
{
    return kRequestA.second.m_fPriorityMetric < kRequestB.second.m_fPriorityMetric;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorSelectorPager::GenerateStreamingRequests(SectorRequestSet& kRequests)
{
    efd::map<NiTerrainSector::SectorID, NiInt32>::iterator kIter;
    
    // Look for any sectors that are currently loaded, but don't need to be
    for (kIter = m_kSectorDetailLevels.begin(); kIter != m_kSectorDetailLevels.end(); ++kIter)
    {
        // Check if this sector exists in our list of requests
        if (kRequests.find(kIter->first) == kRequests.end())
        {
            // We do not need this sector anymore, unload it
            AddToSelection(kIter->first, -1);
        }
    }

    // Sort all the requests into priority order
    efd::vector<CompleteSectorRequest> kSortedRequests(kRequests.begin(), kRequests.end());
    std::sort(kSortedRequests.begin(), kSortedRequests.end(), SortByPriority);

    // Append all the requests to the list
    efd::vector<CompleteSectorRequest>::iterator kRequestIter;
    for (kRequestIter = kSortedRequests.begin(); 
        kRequestIter != kSortedRequests.end(); 
        ++kRequestIter)
    {
        AddToSelection(kRequestIter->first, kRequestIter->second.m_iTargetLOD);
    }

    return m_kSelectedSector.size() > 0;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorSelectorPager::MergeSectorSet(SectorRequestSet& kSetA, 
    const SectorRequestSet& kSetB, efd::Float32 fSetBPriority)
{
    SectorRequestSet::const_iterator kIter;
    for (kIter = kSetB.begin(); kIter != kSetB.end(); ++kIter)
    {
        const SectorRequest& kMergeRequest = kIter->second;
        SectorRequest& kCurRequest = kSetA[kIter->first];

        kCurRequest.m_fPriorityMetric = efd::Min(
            kCurRequest.m_fPriorityMetric,
            kMergeRequest.m_fPriorityMetric + fSetBPriority);
        kCurRequest.m_iTargetLOD = efd::Max(
            kCurRequest.m_iTargetLOD,
            kMergeRequest.m_iTargetLOD);
    }
}

//--------------------------------------------------------------------------------------------------
