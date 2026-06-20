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
#include "NiTerrainSectorSelector.h"

//--------------------------------------------------------------------------------------------------
NiImplementRootRTTI(NiTerrainSectorSelector, NiTypeMask::NiTerrainSectorSelector);
//--------------------------------------------------------------------------------------------------
NiTerrainSectorSelector::NiTerrainSectorSelector(NiTerrain* pkTerrain):
    m_pkTerrain(pkTerrain),
    m_bUseSectorCatalogue(true)
{
    EE_ASSERT(pkTerrain);
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorSelector::~NiTerrainSectorSelector()
{
    ClearSelection();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorSelector::CheckSectorValidForSelection(
    const NiTerrainSectorSelector::LoadingInfo& kSectorInfo)
{
    EE_ASSERT(kSectorInfo.m_iTargetLoD <= (NiInt32)m_pkTerrain->GetNumLOD());

    if (kSectorInfo.m_iTargetLoD >= 0 && 
        !CheckSectorIsOnDisk(kSectorInfo.m_sIndexX, kSectorInfo.m_sIndexY))
        return false;

    NiTerrainSector* pkSector = 
        m_pkTerrain->GetSector(kSectorInfo.m_sIndexX, kSectorInfo.m_sIndexY);

    if (!pkSector)
        return true;

    if (pkSector->GetSectorData()->GetHighestLoadedLOD() != kSectorInfo.m_iTargetLoD
        || kSectorInfo.m_iTargetLoD == -1)
        return true;

    return false;
}
//--------------------------------------------------------------------------------------------------
void NiTerrainSectorSelector::AddToSelection(NiTerrainSector::SectorID kSectorID, 
    NiInt32 iTargetLOD)
{
    NiInt16 sIndexX;
    NiInt16 sIndexY;
    NiTerrainSector::GenerateSectorIndex(kSectorID, sIndexX, sIndexY);

    return AddToSelection(sIndexX, sIndexY, iTargetLOD);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorSelector::AddToSelection(NiInt16 sIndexX, NiInt16 sIndexY, NiInt32 iTargetLOD)
{
    EE_ASSERT(iTargetLOD >= -1);

    // Have we already requested that this sector should be loaded to this LOD?
    NiTerrainSector::SectorID kSectorID;
    NiTerrainSector::GenerateSectorID(sIndexX, sIndexY, kSectorID);
    NiInt32 iRequestedLOD = -1;

    // If we don't find the sector, the requested LOD will still remain as -1 (unload)
    m_kSectorDetailLevels.find(kSectorID, iRequestedLOD);

    if (iRequestedLOD == iTargetLOD)
        return;

    // Is this sector actually a valid candidate for selection?
    NiTerrainSectorSelector::LoadingInfo kSectorData;
    kSectorData.m_sIndexX = sIndexX;
    kSectorData.m_sIndexY = sIndexY;
    kSectorData.m_iTargetLoD = iTargetLOD;

    if (!CheckSectorValidForSelection(kSectorData))
        return;

    // Have we already added this sector to the selection?
    {
        for (LoadingInfoListType::iterator kIter = m_kSelectedSector.begin(); 
            kIter != m_kSelectedSector.end(); kIter++)
        {
            LoadingInfo& kSelectedSectorInfo = *kIter;
            if (kSelectedSectorInfo.m_sIndexX == sIndexX && 
                kSelectedSectorInfo.m_sIndexY == sIndexY)
            {
                EE_ASSERT(kSelectedSectorInfo.m_iTargetLoD != iTargetLOD);

                // Yes, this sector is in the selection. Modify it!
                m_kSectorDetailLevels[kSectorID] = iTargetLOD;
                kSelectedSectorInfo.m_iTargetLoD = iTargetLOD;

                return;
            }
        }
    }

    // This sector isn't in the selection yet; add it.
    m_kSectorDetailLevels[kSectorID] = iTargetLOD;
    m_kSelectedSector.push_back(kSectorData);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorSelector::ClearSelection()
{
    m_kSelectedSector.clear();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorSelector::Resubmit(const LoadingInfo& kInfo)
{
#ifdef EE_ASSERTS_ARE_ENABLED
    NiTerrainSector::SectorID kSectorID;
    NiTerrainSector::GenerateSectorID(kInfo.m_sIndexX, kInfo.m_sIndexY, kSectorID);
    NiInt32 iRequestedLOD;
    m_kSectorDetailLevels.find(kSectorID, iRequestedLOD);
    EE_ASSERT(kInfo.m_iTargetLoD == iRequestedLOD);
#endif

    m_kSelectedSector.push_back(kInfo);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorSelector::CheckSectorIsOnDisk(NiInt16 sSectorX, NiInt16 sSectorY)
{    
    // Fetch the cached value 
    NiUInt32 uiSectorExists = 0;
    NiUInt32 uiSectorKey = ((NiUInt32)sSectorX << 16) + (NiUInt16)sSectorY;
    bool bCacheHit = m_kSectorCatalogue.find(uiSectorKey, uiSectorExists);
    
    if (!bCacheHit || !m_bUseSectorCatalogue)
    {
        // Check if the sector is on disk, and cache the result of the check
        uiSectorExists = m_pkTerrain->IsSectorOnDisk(sSectorX, sSectorY);
        m_kSectorCatalogue[uiSectorKey] = uiSectorExists;
    }

    return uiSectorExists != 0;
}

//--------------------------------------------------------------------------------------------------