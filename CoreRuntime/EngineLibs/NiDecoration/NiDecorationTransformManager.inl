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
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

//------------------------------------------------------------------------------------------------
inline void NiDecorationTransformManager::AssertCellConsistency()
{
#if defined(EE_ASSERTS_ARE_ENABLED) && FALSE
    DoAssertCellConsistency();
#endif
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationTransformManager::AssertCellDebugTransforms(NiDecorationCell* pkCell)
{
    // This should be used for in house testing only. It assumes two sectors:
    //  (0, 0) -> All instance transforms will have their Z set less than 0.0f
    //  (x, x) -> All instance transforms will have their Z set greater than 0.0f
#if defined(EE_ASSERTS_ARE_ENABLED) && FALSE
    DoAssertCellDebugTransforms(pkCell);
#else
    EE_UNUSED_ARG(pkCell);
#endif
}

//------------------------------------------------------------------------------------------------
inline const NiDecorationTransformManager::RegionIDList& 
NiDecorationTransformManager::GetInstanceRegions() const
{
    return m_kRegions;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationTransformManager::GetNumMaxCells() const
{
    return m_uiMaxCellCount;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationTransformManager::GetInstancesPerCell() const
{
    return m_uiInstancesPerCell;
}

//------------------------------------------------------------------------------------------------
inline NiTransform* NiDecorationTransformManager::GetTransformStream()
{
    return m_pkTransforms;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationTransformManager::GetTransformCount()
{
    return m_uiTransformCount;
}

//------------------------------------------------------------------------------------------------
inline const NiDecorationTransformManager::CellSet& NiDecorationTransformManager::GetChangedCells() 
    const
{
    return m_kCellsToUpload;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationTransformManager::MarkAllCellsUploaded()
{
    m_kCellsToUpload.clear();
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationTransformManager::GetVisibleCellCount(RegionID kRegionID)
{
    return GetRegion(kRegionID).m_range;
}

//------------------------------------------------------------------------------------------------
inline bool NiDecorationTransformManager::GetFirstCellIndex(RegionID kRegionID, 
    NiUInt32& uiFirstCellIndex)
{
    Region& kRegion = GetRegion(kRegionID);
    uiFirstCellIndex = kRegion.m_start;

    return kRegion.m_range > 0;
}

//------------------------------------------------------------------------------------------------
inline NiDecorationTransformManager::CellIndex::CellIndex(NiUInt32 uiX, NiUInt32 uiY)
    : m_uiX(uiX)
    , m_uiY(uiY)
{
}

//------------------------------------------------------------------------------------------------

