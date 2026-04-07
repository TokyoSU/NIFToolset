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

#include "NiTerrainDataSnapshot.h"

//--------------------------------------------------------------------------------------------------
NiTerrainDataSnapshot::CellSnapshot::CellSnapshot():
    m_pkHeightmap(NULL),
    m_uiStoredData(0)
{
    
}

//--------------------------------------------------------------------------------------------------
NiTerrainDataSnapshot::CellSnapshot::~CellSnapshot()
{
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDataSnapshot::CellSnapshot::MarkContainsData(efd::UInt32 uiBufferType)
{
    m_uiStoredData |= 1 << uiBufferType;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainDataSnapshot::CellSnapshot::ContainsData(efd::UInt32 uiBufferType) const
{
    return (m_uiStoredData & (1 << uiBufferType)) != 0;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDataSnapshot::CellSnapshot::AppendData(HeightMapBuffer* pkHeightmap)
{
    m_pkHeightmap = pkHeightmap;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDataSnapshot::CellSnapshot::AppendData(efd::UInt32 uiSurfaceIndex, 
    SurfaceMaskBuffer* pkMask)
{
    m_kSurfaceMasks[uiSurfaceIndex] = pkMask;
}

//--------------------------------------------------------------------------------------------------
HeightMapBuffer* NiTerrainDataSnapshot::CellSnapshot::GetHeightmap() const
{
    return m_pkHeightmap;
}

//--------------------------------------------------------------------------------------------------
SurfaceMaskBuffer* NiTerrainDataSnapshot::CellSnapshot::GetSurfaceMask(efd::UInt32 uiSurfaceIndex)
    const
{
    SurfaceMaskMap::const_iterator kIter = m_kSurfaceMasks.find(uiSurfaceIndex);
    if (kIter != m_kSurfaceMasks.end())
        return kIter->second;
    else
        return NULL;
}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
NiTerrainDataSnapshot::NiTerrainDataSnapshot()
{
}

//--------------------------------------------------------------------------------------------------
NiTerrainDataSnapshot::~NiTerrainDataSnapshot()
{
    CellMap::iterator kIter;
    for (kIter = m_kCells.begin(); kIter != m_kCells.end(); ++kIter)
    {
        EE_DELETE(kIter->second);
    }
    m_kCells.clear();
}

//--------------------------------------------------------------------------------------------------
NiTerrainDataSnapshot::CellSnapshot* NiTerrainDataSnapshot::GetCellSnapshot(CellID kCellID)
{
    CellSnapshot* pkCell = NULL;
    CellMap::const_iterator kIter = m_kCells.find(kCellID);
    if (kIter != m_kCells.end())
    {
        pkCell = kIter->second;
    }
    else
    {
        pkCell = EE_NEW CellSnapshot();
        m_kCells[kCellID] = pkCell;

        // Add this cell to the stack
        m_kCellStack.push_back(kCellID);
    }
    return pkCell;
}

//--------------------------------------------------------------------------------------------------
const NiTerrainDataSnapshot::CellSnapshot* NiTerrainDataSnapshot::GetCellSnapshot(CellID kCellID) 
    const
{
    CellMap::const_iterator kIter = m_kCells.find(kCellID);
    if (kIter != m_kCells.end())
        return kIter->second;
    else
        return NULL;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainDataSnapshot::ContainsData(CellID kCellID, BufferType::Value eBufferType) const
{
    const CellSnapshot* pkCellShot = GetCellSnapshot(kCellID);
    if (pkCellShot)
        return pkCellShot->ContainsData(eBufferType);
    else
        return false;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDataSnapshot::AppendData(CellID kCellID, HeightMapBuffer* pkHeightmap)
{
    CellSnapshot* pkCellShot = GetCellSnapshot(kCellID);
    pkCellShot->AppendData(pkHeightmap);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainDataSnapshot::AppendData(CellID kCellID, efd::UInt32 uiSurfaceIndex, 
    SurfaceMaskBuffer* pkMask)
{
    CellSnapshot* pkCellShot = GetCellSnapshot(kCellID);
    pkCellShot->AppendData(uiSurfaceIndex, pkMask);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainDataSnapshot::FetchData(CellID kCellID, HeightMapBuffer*& pkHeightmap) const
{
    const CellSnapshot* pkCellShot = GetCellSnapshot(kCellID);
    if (pkCellShot)
    {
        pkHeightmap = pkCellShot->GetHeightmap();
        return pkHeightmap != NULL;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainDataSnapshot::FetchData(CellID kCellID, efd::UInt32 uiSurfaceIndex, 
    SurfaceMaskBuffer*& pkMask) const
{
    const CellSnapshot* pkCellShot = GetCellSnapshot(kCellID);
    if (pkCellShot)
    {
        pkMask = pkCellShot->GetSurfaceMask(uiSurfaceIndex);
        return pkMask != NULL;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainDataSnapshot::FetchSurfaceMasks(CellID kCellID, 
    const CellSnapshot::SurfaceMaskMap*& pkMasks) const
{
    const CellSnapshot* pkCellShot = GetCellSnapshot(kCellID);
    if (pkCellShot)
    {
        pkMasks = &pkCellShot->GetSurfaceMasks();
        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------------------------