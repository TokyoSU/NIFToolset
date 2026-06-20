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
#include "NiTerrainSectorSelectorDefault.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiTerrainSectorSelectorDefault, NiTerrainSectorSelector, NiTypeMask::NiTerrainSectorSelectorDefault);
//--------------------------------------------------------------------------------------------------
NiTerrainSectorSelectorDefault::NiTerrainSectorSelectorDefault(NiTerrain* pkTerrain):
    NiTerrainSectorSelector(pkTerrain),
    m_kFirstSectorIndex(0,0),
    m_kTerrainExtent(1,1)
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorSelectorDefault::NiTerrainSectorSelectorDefault(NiTerrain* pkTerrain,
    NiIndex kFirstSectorIndex, NiIndex kTerrainExtent):
    NiTerrainSectorSelector(pkTerrain)
{
    m_kFirstSectorIndex = kFirstSectorIndex;
    m_kTerrainExtent = kTerrainExtent;
}

//--------------------------------------------------------------------------------------------------
NiTerrainSectorSelectorDefault::~NiTerrainSectorSelectorDefault()
{
    ClearSelection();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorSelectorDefault::UpdateSectorSelection()
{
    return BuildSectorList();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainSectorSelectorDefault::BuildSectorList()
{
    NiInt16 sEndX = (NiInt16)(m_kFirstSectorIndex.x + m_kTerrainExtent.x);
    NiInt16 sEndY = (NiInt16)(m_kFirstSectorIndex.y + m_kTerrainExtent.y);

    for (NiInt16 sY = (NiInt16)m_kFirstSectorIndex.y; sY < sEndY; ++sY)
    {
        for (NiInt16 sX = (NiInt16)m_kFirstSectorIndex.x; sX < sEndX; ++sX)
        {
            AddToSelection(sX, sY, m_pkTerrain->GetNumLOD());
        }
    }

    return m_kSelectedSector.size() > 0;
}

//--------------------------------------------------------------------------------------------------