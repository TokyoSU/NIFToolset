// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#include "NiTerrainPCH.h"
#include "NiTerrain.h"
#include "NiTerrainSector.h"
#include "NiTerrainStreamLocks.h"

//---------------------------------------------------------------------------
NiTerrainStreamLocks::NiTerrainStreamLocks()
{
    for (NiUInt32 ui = 0; ui < 5; ++ui)
        m_aucCurrentLockMask[ui] = 0;
}
//---------------------------------------------------------------------------
NiTerrainStreamLocks::NiTerrainStreamLocks(NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask, 
    NiUInt32 uiLevel)
{
    AcquireLocks(pkSector, ucLockMask, uiLevel);
    for (NiUInt32 ui = 0; ui < 5; ++ui)
        m_aucCurrentLockMask[ui] = ucLockMask;
}
//---------------------------------------------------------------------------
NiTerrainStreamLocks::NiTerrainStreamLocks(NiTerrainCell* pkCell, 
    NiUInt8 ucLockMask)
{
    AcquireLocks(pkCell, ucLockMask);
    for (NiUInt32 ui = 0; ui < 5; ++ui)
        m_aucCurrentLockMask[ui] = ucLockMask;
}
//---------------------------------------------------------------------------
NiTerrainStreamLocks::~NiTerrainStreamLocks()
{
    ReleaseLocks();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetPositionIterator(NiTerrainCell* pkCell, 
    NiUInt8 ucLockMask,
    NiTerrainPositionRandomAccessIterator& kPosIter)
{
    VerifyLock(POSITION, pkCell, ucLockMask);
    
    const NiTerrainSector* pkSector = pkCell->GetContainingSector();
    NiUInt32 uiRegionID = 0;
    
    if (!pkCell->HasDynamicVertexStreams())
        uiRegionID = pkCell->GetRegionID();

    kPosIter = NiTerrainPositionRandomAccessIterator(
        &m_kPositionLock, uiRegionID, 
        pkSector->GetConfiguration());
    
    return kPosIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetPositionIterator(NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask,
    NiTerrainPositionRandomAccessIterator& kPosIter,
    NiUInt32 uiLevel)
{
    VerifyLock(POSITION, pkSector, ucLockMask, uiLevel);
    
    NiTerrainCell* pkCell = 
        pkSector->GetCell(pkSector->GetCellOffset(uiLevel));
    
    kPosIter = NiTerrainPositionRandomAccessIterator(
        &m_kPositionLock, pkCell->GetRegionID(),
        pkSector->GetConfiguration());

    return kPosIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetNormalIterator(NiTerrainCell* pkCell, 
    NiUInt8 ucLockMask,
    NiTerrainNormalRandomAccessIterator& kNormalIter)
{
    VerifyLock(NORMAL, pkCell, ucLockMask);

    if (!m_kNormalLock.IsLocked())
        return false;

    const NiTerrainSector* pkSector = pkCell->GetContainingSector();
    NiUInt32 uiRegionID = 0;
    
    if (!pkCell->HasDynamicVertexStreams())
        uiRegionID = pkCell->GetRegionID();

    kNormalIter = NiTerrainNormalRandomAccessIterator(
        &m_kNormalLock, uiRegionID,
        pkSector->GetConfiguration());
    
    return kNormalIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetNormalIterator(NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask,
    NiTerrainNormalRandomAccessIterator& kNormalIter,
    NiUInt32 uiLevel)
{
    VerifyLock(NORMAL, pkSector, ucLockMask, uiLevel);

    if (!m_kNormalLock.IsLocked())
        return false;

    NiTerrainCell* pkCell = 
        pkSector->GetCell(pkSector->GetCellOffset(uiLevel));
    
    kNormalIter = NiTerrainNormalRandomAccessIterator(
        &m_kPositionLock, pkCell->GetRegionID(),
        pkSector->GetConfiguration());

    return kNormalIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetTangentIterator(NiTerrainCell* pkCell, 
    NiUInt8 ucLockMask,
    NiTerrainTangentRandomAccessIterator& kTangentIter)
{
    VerifyLock(TANGENT, pkCell, ucLockMask);

    if (!m_kTangentLock.IsLocked())
        return false;

    const NiTerrainSector* pkSector = pkCell->GetContainingSector();
    NiUInt32 uiRegionID = 0;
    
    if (!pkCell->HasDynamicVertexStreams())
        uiRegionID = pkCell->GetRegionID();
    
    kTangentIter = NiTerrainTangentRandomAccessIterator(
        &m_kTangentLock, uiRegionID,
        pkSector->GetConfiguration());
    
    return kTangentIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetTangentIterator(NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask,
    NiTerrainTangentRandomAccessIterator& kTangentIter,
    NiUInt32 uiLevel)
{
    VerifyLock(TANGENT, pkSector, ucLockMask, uiLevel);

    if (!m_kTangentLock.IsLocked())
        return false;

    NiTerrainCell* pkCell = pkSector->GetCell(pkSector->GetCellOffset(uiLevel));
    
    kTangentIter = NiTerrainTangentRandomAccessIterator(
        &m_kPositionLock, pkCell->GetRegionID(),
        pkSector->GetConfiguration());

    return kTangentIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetIndexIterator(NiTerrainCell* pkCell,
    NiUInt8 ucLockMask, 
    NiTStridedRandomAccessIterator<NiUInt16>& kIndexIter,
    NiUInt8 ucRegion)
{
    VerifyLock(INDEX, pkCell, ucLockMask);
    
    kIndexIter = m_kIndexLock.begin_region<NiUInt16>(ucRegion);

    return kIndexIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetIndexIterator(NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask,
    NiTStridedRandomAccessIterator<NiUInt16>& kIndexIter,
    NiUInt8 ucRegion)
{
    VerifyLock(INDEX, pkSector, ucLockMask);

    kIndexIter = m_kIndexLock.begin_region<NiUInt16>(ucRegion);

    return kIndexIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetIndexIterator(NiTerrainCell* pkCell, 
    NiUInt8 ucLockMask,
    NiTStridedRandomAccessIterator<NiUInt32>& kIndexIter,
    NiUInt8 ucRegion)
{
    VerifyLock(INDEX, pkCell, ucLockMask);
    
    kIndexIter = m_kIndexLock.begin_region<NiUInt32>(ucRegion);

    return kIndexIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetIndexIterator(NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask,
    NiTStridedRandomAccessIterator<NiUInt32>& kIndexIter,
    NiUInt8 ucRegion)
{
    VerifyLock(INDEX, pkSector, ucLockMask);

    kIndexIter = m_kIndexLock.begin_region<NiUInt32>(ucRegion);

    return kIndexIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetUVIterator(NiTerrainCell* pkCell, 
    NiUInt8 ucLockMask,
    NiTStridedRandomAccessIterator<NiPoint2>& kUVIter)
{
    VerifyLock(TEXCOORD, pkCell, ucLockMask);
    
    kUVIter = m_kUVLock.begin<NiPoint2>();
    
    return kUVIter.Exists();
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::GetUVIterator(NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask,
    NiTStridedRandomAccessIterator<NiPoint2>& kUVIter)
{
    VerifyLock(TEXCOORD, pkSector, ucLockMask);

    kUVIter = m_kUVLock.begin<NiPoint2>();
    
    return kUVIter.Exists();
}
//---------------------------------------------------------------------------
void NiTerrainStreamLocks::ReleaseLocks()
{
    m_kIndexLock.Unlock();
    m_kUVLock.Unlock();
    m_kPositionLock.Unlock();
    m_kNormalLock.Unlock();
    m_kTangentLock.Unlock();   
}
//---------------------------------------------------------------------------
void NiTerrainStreamLocks::ReleaseLock(StreamType eStream)
{
    switch(eStream)
    {
    case POSITION:
        m_kPositionLock.Unlock();
        break;
    case NORMAL:
        m_kNormalLock.Unlock();
        break;
    case TANGENT:
        m_kTangentLock.Unlock();
        break;
    case INDEX:
        m_kIndexLock.Unlock();
        break;
    case TEXCOORD:
        m_kUVLock.Unlock();
        break;
    default:
        break;
    }
}
//---------------------------------------------------------------------------
void NiTerrainStreamLocks::AcquireLocks(NiTerrainCell* pkCell, 
    NiUInt8 ucLockMask)
{    
    const NiTerrainSector* pkSector = 
        pkCell->GetContainingSector();

    NiDataStream* pkPosStream = NULL;
    NiDataStream* pkNormTangStream = NULL;
    NiDataStream* pkIndStream = NULL;
    NiDataStream* pkUVStream = NULL;

    pkPosStream = pkCell->GetStream(NiTerrain::StreamType::POSITION);
    pkNormTangStream = pkCell->GetStream(NiTerrain::StreamType::NORMAL_TANGENT);
    pkIndStream = pkCell->GetStream(NiTerrain::StreamType::INDEX);
    pkUVStream = pkCell->GetStream(NiTerrain::StreamType::TEXTURE_COORD);

    m_kPositionLock = NiDataStreamLock(pkPosStream, 0, ucLockMask);
    m_kNormalLock = NiDataStreamLock(pkNormTangStream, 0, ucLockMask);
    m_kTangentLock = NiDataStreamLock(pkNormTangStream, sizeof(float) * 
        pkSector->GetConfiguration().GetNumNormalComponents(pkCell->GetLevel() == 
        pkCell->GetContainingSector()->GetSectorData()->GetNumLOD()), 
        ucLockMask);
    m_kIndexLock = NiDataStreamLock(pkIndStream, 0, ucLockMask);
    m_kUVLock = NiDataStreamLock(pkUVStream, 0, ucLockMask);

}
//---------------------------------------------------------------------------
void NiTerrainStreamLocks::AcquireLocks(NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask, 
    NiUInt32 uiLevel)
{
    NiDataStream* pkPosStream;
    NiDataStream* pkNormTangStream;
    NiDataStream* pkIndStream;
    NiDataStream* pkUVStream;

    pkPosStream =
        pkSector->GetSectorData()->GetStaticPositionStream(uiLevel);
    pkNormTangStream =
        pkSector->GetSectorData()->GetStaticNormalTangentStream(uiLevel);
    pkIndStream = pkSector->GetSectorData()->GetStaticIndexStream();
    pkUVStream = pkSector->GetSectorData()->GetStaticUVStream();

    m_kPositionLock = NiDataStreamLock(pkPosStream, 0, ucLockMask);
    m_kNormalLock = NiDataStreamLock(pkNormTangStream, 0, ucLockMask);
    m_kTangentLock = NiDataStreamLock(pkNormTangStream, sizeof(float) * 
        pkSector->GetConfiguration().GetNumNormalComponents(uiLevel == 
        pkSector->GetSectorData()->GetNumLOD()), 
        ucLockMask);
    m_kIndexLock = NiDataStreamLock(pkIndStream, 0, ucLockMask);
    m_kUVLock = NiDataStreamLock(pkUVStream, 0, ucLockMask);
}
//---------------------------------------------------------------------------
void NiTerrainStreamLocks::AcquireLock(StreamType eStreamType, 
    NiTerrainCell* pkCell, 
    NiUInt8 ucLockMask)
{
    const NiTerrainSector* pkSector = pkCell->GetContainingSector();
    NiDataStream* pkStream = NULL;
    switch (eStreamType)
    { 
    case POSITION:        
        pkStream = pkCell->GetStream(NiTerrain::StreamType::POSITION);
        m_kPositionLock = NiDataStreamLock(pkStream, 0, ucLockMask);
        break;
    case NORMAL:
        pkStream = pkCell->GetStream(NiTerrain::StreamType::NORMAL_TANGENT);
        m_kNormalLock = NiDataStreamLock(pkStream, 0, ucLockMask);
        break;
    case TANGENT:
        pkStream = pkCell->GetStream(NiTerrain::StreamType::NORMAL_TANGENT);
        m_kTangentLock = NiDataStreamLock(pkStream, sizeof(float) * 
            pkSector->GetConfiguration().GetNumNormalComponents(pkCell->GetLevel() == 
            pkCell->GetContainingSector()->GetSectorData()->GetNumLOD()), 
            ucLockMask);
        break;
    case INDEX:
        pkStream = pkCell->GetStream(NiTerrain::StreamType::INDEX);
        m_kIndexLock = NiDataStreamLock(pkStream, 0, ucLockMask);
        break;
    case TEXCOORD:
        pkStream = pkCell->GetStream(NiTerrain::StreamType::TEXTURE_COORD);
        m_kUVLock = NiDataStreamLock(pkStream, 0, ucLockMask);
        break;
    default:
        break;
    }

    m_aucCurrentLockMask[eStreamType] = ucLockMask;
}
//---------------------------------------------------------------------------
void NiTerrainStreamLocks::AcquireLock(StreamType eStreamType, 
    NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask, NiUInt32 uiLevel)
{
    NiDataStream* pkStream;
    switch (eStreamType)
    {
    case POSITION:
        pkStream = pkSector->GetSectorData()->GetStaticPositionStream(uiLevel);
        m_kPositionLock = NiDataStreamLock(pkStream, 0, ucLockMask);
        break;
    case NORMAL:
        pkStream = 
            pkSector->GetSectorData()->GetStaticNormalTangentStream(uiLevel);
        m_kNormalLock = NiDataStreamLock(pkStream, 0, ucLockMask);
        break;
    case TANGENT:
        pkStream = 
            pkSector->GetSectorData()->GetStaticNormalTangentStream(uiLevel);
        m_kTangentLock = NiDataStreamLock(pkStream, 0, ucLockMask);
        break;
    case INDEX:
        pkStream = pkSector->GetSectorData()->GetStaticIndexStream();
        m_kIndexLock = NiDataStreamLock(pkStream, 0, ucLockMask);
        break;
    case TEXCOORD:
        pkStream = pkSector->GetSectorData()->GetStaticUVStream();
        m_kUVLock = NiDataStreamLock(pkStream, 0, ucLockMask);
        break;
    default:
        break;
    }

    m_aucCurrentLockMask[eStreamType] = ucLockMask;
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::IsLockInitialized(StreamType eStreamType, 
    NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask,
    NiUInt32 uiLevel)
{
    bool bResult = false;
    const NiTerrainSectorData* pkSectorData = pkSector->GetSectorData();

    switch(eStreamType)
    {
    case POSITION:
        if (pkSectorData->GetStaticPositionStream(uiLevel) != m_kPositionLock.GetDataStream())
        {
            return false;
        }
        bResult = m_kPositionLock.DataStreamExists() && m_kPositionLock.IsLocked();
        break;
    case NORMAL:
        if (pkSectorData->GetStaticNormalTangentStream(uiLevel) != m_kNormalLock.GetDataStream())
        {
            return false;
        }
        bResult = m_kNormalLock.DataStreamExists() && m_kNormalLock.IsLocked();
        break;
    case TANGENT:
        if (pkSectorData->GetStaticNormalTangentStream(uiLevel) != 
            m_kTangentLock.GetDataStream())
        {
            return false;
        }
        bResult = m_kTangentLock.DataStreamExists() && m_kTangentLock.IsLocked();
        break;
    case INDEX:
        if (pkSectorData->GetStaticIndexStream() != m_kIndexLock.GetDataStream())
        {
            return false;
        }
        bResult = m_kIndexLock.DataStreamExists() && m_kIndexLock.IsLocked();
        break;
    case TEXCOORD:
        if (pkSectorData->GetStaticUVStream() != m_kUVLock.GetDataStream())
        {
            return false;
        }
        bResult = m_kUVLock.DataStreamExists() && m_kUVLock.IsLocked();
        break;
    default:
        break;
    }

    return bResult && m_aucCurrentLockMask[eStreamType] == ucLockMask;
}
//---------------------------------------------------------------------------
bool NiTerrainStreamLocks::IsLockInitialized(StreamType eStreamType, 
    NiTerrainCell* pkCell, 
    NiUInt8 ucLockMask)
{
    bool bResult = false;
    switch(eStreamType)
    {
    case POSITION:
        if (pkCell->GetStream(NiTerrain::StreamType::POSITION) != 
            m_kPositionLock.GetDataStream())
        {
            return false;
        }
        bResult = m_kPositionLock.DataStreamExists() && m_kPositionLock.IsLocked();
        break;
    case NORMAL:
        if (pkCell->GetStream(NiTerrain::StreamType::NORMAL_TANGENT) != 
            m_kNormalLock.GetDataStream())
        {
            return false;
        }
        
        bResult = m_kNormalLock.DataStreamExists() && m_kNormalLock.IsLocked();
        break;
    case TANGENT:
        if (pkCell->GetStream(NiTerrain::StreamType::NORMAL_TANGENT) != 
            m_kTangentLock.GetDataStream())
        {
            return false;
        }
        
        bResult = m_kTangentLock.DataStreamExists() && m_kTangentLock.IsLocked();
        break;
    case INDEX:
        if (pkCell->GetStream(NiTerrain::StreamType::INDEX) != 
            m_kIndexLock.GetDataStream())
        {
            return false;
        }
        
        bResult = m_kIndexLock.DataStreamExists() && m_kIndexLock.IsLocked();
        break;
    case TEXCOORD:
        if (pkCell->GetStream(NiTerrain::StreamType::TEXTURE_COORD) != 
            m_kUVLock.GetDataStream())
        {
            return false;
        }
        
        bResult = m_kUVLock.DataStreamExists() && m_kUVLock.IsLocked();
        break;
    default:
        break;
    }

    return bResult && m_aucCurrentLockMask[eStreamType] == ucLockMask;
}
//---------------------------------------------------------------------------
void NiTerrainStreamLocks::VerifyLock(StreamType eStreamType, 
    NiTerrainCell* pkCell, 
    NiUInt8 ucLockMask)
{
    if (!IsLockInitialized(eStreamType, pkCell, ucLockMask))
    {
        switch (eStreamType)
        {
        case POSITION:
            m_kPositionLock.Unlock();
            AcquireLock(eStreamType, pkCell, ucLockMask);
            break;
        case NORMAL:
            m_kNormalLock.Unlock();
            AcquireLock(eStreamType, pkCell, ucLockMask);
            break;
        case TANGENT:
            m_kTangentLock.Unlock();
            AcquireLock(eStreamType, pkCell, ucLockMask);
            break;
        case INDEX:
            m_kIndexLock.Unlock();
            AcquireLock(eStreamType, pkCell, ucLockMask);
            break;
        case TEXCOORD:
            m_kUVLock.Unlock();
            AcquireLock(eStreamType, pkCell, ucLockMask);
            break;
        default:
            break;
        }
    }
}
//---------------------------------------------------------------------------
void NiTerrainStreamLocks::VerifyLock(StreamType eStreamType, 
    NiTerrainSector* pkSector, 
    NiUInt8 ucLockMask, 
    NiUInt32 uiLevel)
{
    if (!IsLockInitialized(eStreamType, pkSector, ucLockMask, uiLevel))
    {
        switch (eStreamType)
        {
        case POSITION:
            m_kPositionLock.Unlock();
            AcquireLock(eStreamType, pkSector, ucLockMask, uiLevel);
            break;
        case NORMAL:
            m_kNormalLock.Unlock();
            AcquireLock(eStreamType, pkSector, ucLockMask, uiLevel);
            break;
        case TANGENT:
            m_kTangentLock.Unlock();
            AcquireLock(eStreamType, pkSector, ucLockMask, uiLevel);
            break;
        case INDEX:
            m_kIndexLock.Unlock();
            AcquireLock(eStreamType, pkSector, ucLockMask, uiLevel);
            break;
        case TEXCOORD:
            m_kUVLock.Unlock();
            AcquireLock(eStreamType, pkSector, ucLockMask, uiLevel);
            break;
        default:
            break;
        }
    }
}
//---------------------------------------------------------------------------
