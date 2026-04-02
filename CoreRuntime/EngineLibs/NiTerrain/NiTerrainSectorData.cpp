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

#include "NiTerrainSectorData.h"
#include "NiTerrainCell.h"
#include "NiTerrainUtils.h"

//--------------------------------------------------------------------------------------------------
NiTerrainSectorData::NiTerrainSectorData() :
    m_uiCellSize(0),
    m_uiCellWidthInVerts(0),
    m_uiSectorSize(0),
    m_uiSectorWidthInVerts(0),
    m_bIsDeformable(false),
    m_uiNumLOD(0),
    m_sSectorIndexX(0),
    m_sSectorIndexY(0),
    m_iHighestLoadedLOD(-1),
    m_iTargetLoadedLOD(-1),
    m_pkCullingProcess(0),
    m_pkLODCamera(0),
    m_fCameraLODSqr(0.0f),
    m_fTerrainLODscale(2.8f),
    m_fTerrainLODshift(0.0f),
    m_uiTerrainLODmode(LOD_MODE_2D | LOD_MORPH_ENABLE),
    m_uiHighestVisibleLOD(NiTerrainUtils::ms_uiMAX_LOD),
    m_uiLowestVisibleLOD(0)
{
    // Ensure the array of index regions is initialized
    m_kIndexRegions.SetSize(NUM_INDEX_REGIONS);

    // Initialize the region map - maps border combinations to regions
    for (NiUInt8 uc = 0; uc < 16; ++uc)
        m_aucIndexRegionsMap[uc] = 0;

    m_aucIndexRegionsMap[NiTerrainCell::BORDER_NONE] = 0;

    m_aucIndexRegionsMap[NiTerrainCell::BORDER_BOTTOM] = 1;
    m_aucIndexRegionsMap[NiTerrainCell::BORDER_RIGHT] = 2;
    m_aucIndexRegionsMap[NiTerrainCell::BORDER_TOP] = 3;
    m_aucIndexRegionsMap[NiTerrainCell::BORDER_LEFT] = 4;

    m_aucIndexRegionsMap[NiTerrainCell::BORDER_BOTTOM | 
        NiTerrainCell::BORDER_RIGHT] = 5;
    m_aucIndexRegionsMap[NiTerrainCell::BORDER_RIGHT | 
        NiTerrainCell::BORDER_TOP] = 6;
    m_aucIndexRegionsMap[NiTerrainCell::BORDER_TOP | 
        NiTerrainCell::BORDER_LEFT] = 7;
    m_aucIndexRegionsMap[NiTerrainCell::BORDER_LEFT | 
        NiTerrainCell::BORDER_BOTTOM] = 8;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorData::~NiTerrainSectorData()
{ 
}

//--------------------------------------------------------------------------------------------------
void NiTerrainSectorData::AddToVisible(const NiTerrainCell* pkBlock) const
{
    EE_ASSERT(pkBlock);
    m_pkCullingProcess->GetVisibleSet()->Add(pkBlock->GetMesh());
}

//--------------------------------------------------------------------------------------------------

