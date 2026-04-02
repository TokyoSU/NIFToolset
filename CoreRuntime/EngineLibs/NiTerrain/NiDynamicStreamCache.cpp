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

#include "NiDynamicStreamCache.h"

//--------------------------------------------------------------------------------------------------
NiDynamicStreamCache::NiDynamicStreamCache(NiTerrainResourceManager* pkResourceManager) :
    m_pkResourceManager(pkResourceManager)
{
    for (NiUInt8 ucPos = 0; ucPos < StreamType::NUM_STREAM_TYPES; ++ucPos)
        m_auiCurrentPosition[ucPos] = 0;
}

//--------------------------------------------------------------------------------------------------
NiDynamicStreamCache::~NiDynamicStreamCache()
{
    for (NiUInt8 ucStreamType = 0; ucStreamType < StreamType::NUM_STREAM_TYPES; ++ucStreamType)
    {
        StreamType::Value eResourceType = StreamType::Value(ucStreamType | StreamType::DYNAMIC);
        for (NiUInt32 ui = 0; ui < m_akStreams[ucStreamType].GetSize(); ++ui)
        {
            NiDataStream* pkStream = m_akStreams[ucStreamType].GetAt(ui);
            m_pkResourceManager->ReleaseStream(eResourceType, pkStream);
            m_akStreams[ucStreamType].SetAt(ui, 0);
        }
    }
}

//--------------------------------------------------------------------------------------------------
NiDataStream* NiDynamicStreamCache::RequestStream(StreamType::Value eStreamType)
{
    // Do we need to grow?
    if (m_auiCurrentPosition[eStreamType] ==m_akStreams[eStreamType].GetSize())
    {
        // Now initialize the array of streams
        NiDataStream* pkStream;
        m_akStreams[eStreamType].SetSize(m_auiCurrentPosition[eStreamType] +
            m_akStreams[eStreamType].GetGrowBy());

        StreamType::Value eResourceType = StreamType::Value(eStreamType | StreamType::DYNAMIC);
        for (NiUInt32 ui = m_auiCurrentPosition[eStreamType];
            ui < m_akStreams[eStreamType].GetAllocatedSize(); ++ui)
        {
            pkStream = m_pkResourceManager->CreateStream(eResourceType, 0);
            m_akStreams[eStreamType].SetAt(ui, pkStream);
        }
    }

    return m_akStreams[eStreamType][m_auiCurrentPosition[eStreamType]++];
}

//--------------------------------------------------------------------------------------------------
bool NiDynamicStreamCache::InitializeStreamCache(StreamType::Value eStreamType,
    NiUInt32 uiInitialSize, NiUInt32 uiGrowBy)
{
    EE_ASSERT(m_akStreams[eStreamType].GetSize() == 0); 
    StreamType::Value eResourceType = StreamType::Value(eStreamType | StreamType::DYNAMIC);

    // Now initialise the array of streams
    NiDataStream* pkStream;
    m_akStreams[eStreamType].SetSize(uiInitialSize);
    m_akStreams[eStreamType].SetGrowBy(uiGrowBy);
    for (NiUInt32 ui = 0; ui < uiInitialSize; ++ui)
    {
        pkStream = m_pkResourceManager->CreateStream(eResourceType, 0);

        if (!pkStream || !pkStream->GetSize())
        {
            // Could not allocate a data stream, may be out of memory?
            m_akStreams[eStreamType].SetSize(ui);
            return false;
        }

        m_akStreams[eStreamType].SetAt(ui, pkStream);
    }

    // All streams allocated successfully
    return true;
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiDynamicStreamCache::GetCurrentSize(StreamType::Value eStreamType)
{
    return m_auiCurrentPosition[eStreamType];
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiDynamicStreamCache::GetMaxSize(StreamType::Value eStreamType)
{
    return m_akStreams[eStreamType].GetSize();
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiDynamicStreamCache::GetNumAvailable(StreamType::Value eStreamType)
{
    return m_akStreams[eStreamType].GetSize() -
        m_auiCurrentPosition[eStreamType];
}
