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

//--------------------------------------------------------------------------------------------------
inline NiTerrainResourceManager::NiTerrainResourceManager(NiTerrain* pkOwner):
    m_pkTerrain(pkOwner),
    m_uiActiveObjects(0)
{
}

//--------------------------------------------------------------------------------------------------
inline NiUInt32 NiTerrainResourceManager::GetNumActiveObjects()
{
    m_kMutex.Lock();
    NiUInt32 uiActiveObjects = m_uiActiveObjects;
    m_kMutex.Unlock();

    return uiActiveObjects;
}

//--------------------------------------------------------------------------------------------------
inline NiTerrain* NiTerrainResourceManager::GetTerrain()
{
    EE_ASSERT(m_pkTerrain);
    return m_pkTerrain;
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainResourceManager::RegisterResource(NiRefObject* pkObject)
{
    if (pkObject)
    {
        m_kMutex.Lock();

        // Notify the listener
        NotifyRegister(pkObject);

        // Adjust active object count
        m_uiActiveObjects++;

        // Make sure this object hangs around until release
        pkObject->IncRefCount();

        m_kMutex.Unlock();
    }
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainResourceManager::DeregisterResource(NiRefObject* pkObject)
{
    m_kMutex.Lock();

    // Notify the listener
    NotifyDeregister(pkObject);

    // Make sure that this object is still with us!
    EE_ASSERT(pkObject->GetRefCount() > 0);

    // Decrease the reference count
    pkObject->DecRefCount();

    // Check for underflow
    EE_ASSERT(m_uiActiveObjects > 0);
    // Adjust active object count
    m_uiActiveObjects--;

    m_kMutex.Unlock();
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainResourceManager::NotifyAllocTexture(TextureType::Value ePurpose, 
    const NiTexture* pkTexture)
{
    Listener::AllocationEventArgs kEventArgs;
    kEventArgs.m_eventType = Listener::AllocationEventArgs::TEXTURE_ALLOC;
    kEventArgs.m_textureArgs.m_ePurpose = ePurpose;
    kEventArgs.m_textureArgs.m_pkTexture = pkTexture;

    m_kListener.m_kAllocationEvent.Raise(GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainResourceManager::NotifyReleaseTexture(TextureType::Value ePurpose,
    const NiTexture* pkTexture)
{
    Listener::AllocationEventArgs kEventArgs;
    kEventArgs.m_eventType = Listener::AllocationEventArgs::TEXTURE_RELEASE;
    kEventArgs.m_textureArgs.m_ePurpose = ePurpose;
    kEventArgs.m_textureArgs.m_pkTexture = pkTexture;

    m_kListener.m_kAllocationEvent.Raise(GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainResourceManager::NotifyAllocStream(StreamType::Value ePurpose, 
    const NiDataStream* pkStream, efd::UInt32 uiLOD)
{
    Listener::AllocationEventArgs kEventArgs;
    kEventArgs.m_eventType = Listener::AllocationEventArgs::STREAM_ALLOC;
    kEventArgs.m_streamAllocArgs.m_ePurpose = ePurpose;
    kEventArgs.m_streamAllocArgs.m_uiLOD = uiLOD;
    kEventArgs.m_streamAllocArgs.m_pkStream = pkStream;

    m_kListener.m_kAllocationEvent.Raise(GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainResourceManager::NotifyReleaseStream(StreamType::Value ePurpose, 
    const NiDataStream* pkStream)
{
    Listener::AllocationEventArgs kEventArgs;
    kEventArgs.m_eventType = Listener::AllocationEventArgs::STREAM_RELEASE;
    kEventArgs.m_streamReleaseArgs.m_ePurpose = ePurpose;
    kEventArgs.m_streamReleaseArgs.m_pkStream = pkStream;

    m_kListener.m_kAllocationEvent.Raise(GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainResourceManager::NotifyAllocBuffer(efd::UInt32 uiBufferSize, void* pvBuffer)
{
    Listener::AllocationEventArgs kEventArgs;
    kEventArgs.m_eventType = Listener::AllocationEventArgs::BUFFER_ALLOC;
    kEventArgs.m_bufferAllocArgs.m_uiBufferSize = uiBufferSize;
    kEventArgs.m_bufferAllocArgs.m_pvBuffer = pvBuffer;

    m_kListener.m_kAllocationEvent.Raise(GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainResourceManager::NotifyReleaseBuffer(void* pvBuffer)
{
    Listener::AllocationEventArgs kEventArgs;
    kEventArgs.m_eventType = Listener::AllocationEventArgs::BUFFER_RELEASE;
    kEventArgs.m_bufferReleaseArgs.m_pvBuffer = pvBuffer;

    m_kListener.m_kAllocationEvent.Raise(GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainResourceManager::NotifyRegister(const NiRefObject* pkObject)
{
    Listener::RegistrationEventArgs kEventArgs;
    kEventArgs.m_eventType = Listener::RegistrationEventArgs::OBJECT_REGISTERED;
    kEventArgs.m_pkObject = pkObject;

    m_kListener.m_kRegistrationEvent.Raise(GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainResourceManager::NotifyDeregister(const NiRefObject* pkObject)
{
    Listener::RegistrationEventArgs kEventArgs;
    kEventArgs.m_eventType = Listener::RegistrationEventArgs::OBJECT_DEREGISTERED;
    kEventArgs.m_pkObject = pkObject;

    m_kListener.m_kRegistrationEvent.Raise(GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
template<typename T> inline 
T* NiTerrainResourceManager::CreateBuffer(efd::UInt32 uiCount)
{
    return (T*)CreateBuffer(uiCount * sizeof(T));
}

//--------------------------------------------------------------------------------------------------
inline NiTerrainStandardResourceManager::BufferData::BufferData(efd::UInt32 uiSize):
    m_pkNextFree(NULL),
    m_pkMoreRecentlyUsed(NULL),
    m_pkLessRecentlyUsed(NULL),
    m_uiSize(uiSize)
{
    // Allocate the buffer
    m_pucBuffer = EE_ALLOC(efd::UInt8, uiSize + sizeof(InlineBufferData));

    // Set the inline buffer data
    InlineBufferData* pkInline = (InlineBufferData*)m_pucBuffer;
    pkInline->m_pkBufferData = this;
}

//--------------------------------------------------------------------------------------------------
inline NiTerrainStandardResourceManager::BufferData::~BufferData()
{
    EE_FREE(m_pucBuffer);
}

//--------------------------------------------------------------------------------------------------
inline void* NiTerrainStandardResourceManager::BufferData::GetBuffer()
{
    return m_pucBuffer + sizeof(InlineBufferData);
}

//--------------------------------------------------------------------------------------------------
inline efd::UInt32 NiTerrainStandardResourceManager::BufferData::GetSize()
{
    return m_uiSize;
}

//--------------------------------------------------------------------------------------------------
inline NiTerrainStandardResourceManager::BufferData* 
    NiTerrainStandardResourceManager::BufferData::GetMoreRecentlyUsed()
{
    return m_pkMoreRecentlyUsed;
}

//--------------------------------------------------------------------------------------------------
inline NiTerrainStandardResourceManager::BufferData* 
    NiTerrainStandardResourceManager::BufferData::GetLessRecentlyUsed()
{
    return m_pkLessRecentlyUsed;
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainStandardResourceManager::BufferData::SetMoreRecentlyUsed(BufferData* pkNext)
{
    EE_ASSERT(m_pkMoreRecentlyUsed == NULL);
    m_pkMoreRecentlyUsed = pkNext;
    pkNext->m_pkLessRecentlyUsed = this;
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainStandardResourceManager::BufferData::BeginUsing()
{
    // Remove this buffer from the recently used list
    if (m_pkLessRecentlyUsed)
    {
        m_pkLessRecentlyUsed->m_pkMoreRecentlyUsed = m_pkMoreRecentlyUsed;
    }
    if (m_pkMoreRecentlyUsed)
    {
        m_pkMoreRecentlyUsed->m_pkLessRecentlyUsed = m_pkLessRecentlyUsed;
    }
    m_pkLessRecentlyUsed = NULL;
    m_pkMoreRecentlyUsed = NULL;
}

//--------------------------------------------------------------------------------------------------
inline NiTerrainStandardResourceManager::BufferData* 
    NiTerrainStandardResourceManager::BufferData::GetNextFree()
{   
    return m_pkNextFree;
}

//--------------------------------------------------------------------------------------------------
inline void NiTerrainStandardResourceManager::BufferData::SetNextFree(BufferData* pkNext)
{
    m_pkNextFree = pkNext;
}

//--------------------------------------------------------------------------------------------------
inline NiTerrainStandardResourceManager::BufferData* 
    NiTerrainStandardResourceManager::BufferData::FetchBufferData(void* pvBuffer)
{
    // Find the inline buffer data
    efd::UInt8* pucBuffer = (efd::UInt8*)pvBuffer;
    pucBuffer -= sizeof(InlineBufferData);
    InlineBufferData* pkInline = (InlineBufferData*)pucBuffer;

    // Find the buffer data object
    EE_ASSERT(pkInline->m_pkBufferData->GetBuffer() == pvBuffer);
    return pkInline->m_pkBufferData; 
}

//--------------------------------------------------------------------------------------------------