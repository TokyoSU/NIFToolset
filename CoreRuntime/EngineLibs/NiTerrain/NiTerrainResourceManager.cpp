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

#include "NiTerrainResourceManager.h"
#include "NiTerrain.h"

//--------------------------------------------------------------------------------------------------
NiImplementRootRTTI(NiTerrainResourceManager);
NiImplementRTTI(NiTerrainStandardResourceManager, NiTerrainResourceManager);

//--------------------------------------------------------------------------------------------------
NiTerrainResourceManager::~NiTerrainResourceManager()   
{
    // All objects should be released by allocator destruction
    EE_ASSERT(m_uiActiveObjects == 0);
}

//--------------------------------------------------------------------------------------------------
NiSourceTexture* NiTerrainResourceManager::CreateTexture(
    TextureType::Value eType, NiPixelData* pkPixelData)
{
    EE_UNUSED_ARG(eType);
    EE_UNUSED_ARG(pkPixelData);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
NiRenderedTexture* NiTerrainResourceManager::CreateRenderedTexture(TextureType::Value eType)
{
    EE_UNUSED_ARG(eType);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainResourceManager::ReleaseTexture(TextureType::Value eType, 
    NiTexture* pkTexture)
{
    EE_UNUSED_ARG(eType);
    EE_UNUSED_ARG(pkTexture);

    /// Allow the blend texture to be released automatically
}

//--------------------------------------------------------------------------------------------------
NiDataStream* NiTerrainResourceManager::CreateStream(StreamType::Value eType, NiUInt32 uiLODLevel)
{
    EE_UNUSED_ARG(eType);
    EE_UNUSED_ARG(uiLODLevel);

    return NULL;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainResourceManager::ReleaseStream(StreamType::Value eType, NiDataStream* pkStream)
{
    EE_UNUSED_ARG(eType);
    EE_UNUSED_ARG(pkStream);
}

//--------------------------------------------------------------------------------------------------
void* NiTerrainResourceManager::CreateBuffer(efd::UInt32 uiNumBytes)
{
    EE_UNUSED_ARG(uiNumBytes);
    return NULL;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainResourceManager::ReleaseBuffer(void* pvBuffer)
{
    EE_UNUSED_ARG(pvBuffer);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainResourceManager::ReleaseAllUnusedBuffers()
{
    // Do nothing
}

//--------------------------------------------------------------------------------------------------
void NiTerrainResourceManager::SetBufferMemoryUsageThreshold(efd::UInt64 uiThreshold)
{
    EE_UNUSED_ARG(uiThreshold);
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
NiTerrainResourceManager::Listener::Listener()
{
    // Do Nothing
}

//--------------------------------------------------------------------------------------------------
NiTerrainResourceManager::Listener::~Listener()
{
    // Do Nothing
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
NiTerrainStandardResourceManager::NiTerrainStandardResourceManager(NiTerrain* pkOwner):
    NiTerrainResourceManager(pkOwner),
    m_pkLeastRecentlyUsedBuffer(NULL),
    m_pkMostRecentlyUsedBuffer(NULL),
    m_uiBufferUsageThreshold(/*10MB*/ (10 * 1024 * 1024)),
    m_uiBufferUsage(0)
{
    // Initialize the texture formats:
    NiTexture::FormatPrefs* pkFormat;

    // Blend mask format:
    pkFormat = &m_akTextureFormats[TextureType::BLEND_MASK];
    pkFormat->m_eAlphaFmt = NiTexture::FormatPrefs::SMOOTH;
    pkFormat->m_eMipMapped = NiTexture::FormatPrefs::MIP_DEFAULT;
    pkFormat->m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    m_abTextureIsStatic[TextureType::BLEND_MASK] = false;

    pkFormat = &m_akTextureFormats[TextureType::LOWDETAIL_DIFFUSE];
    pkFormat->m_eAlphaFmt = NiTexture::FormatPrefs::SMOOTH;
    pkFormat->m_eMipMapped = NiTexture::FormatPrefs::MIP_DEFAULT;
    pkFormat->m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    m_abTextureIsStatic[TextureType::LOWDETAIL_DIFFUSE] = !NiTerrain::InToolMode();

    pkFormat = &m_akTextureFormats[TextureType::LOWDETAIL_NORMAL];
    pkFormat->m_eAlphaFmt = NiTexture::FormatPrefs::SMOOTH;
    pkFormat->m_eMipMapped = NiTexture::FormatPrefs::MIP_DEFAULT;
    pkFormat->m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    m_abTextureIsStatic[TextureType::LOWDETAIL_NORMAL] = false;

    pkFormat = &m_akTextureFormats[TextureType::TERRAIN_HEIGHTMAP];
    pkFormat->m_eAlphaFmt = NiTexture::FormatPrefs::SMOOTH;
    pkFormat->m_eMipMapped = NiTexture::FormatPrefs::MIP_DEFAULT;
    pkFormat->m_ePixelLayout = NiTexture::FormatPrefs::SINGLE_COLOR_16;
    m_abTextureIsStatic[TextureType::TERRAIN_HEIGHTMAP] = false;
}

//--------------------------------------------------------------------------------------------------
NiTerrainStandardResourceManager::~NiTerrainStandardResourceManager()
{
    EmptyBufferPool();
    EE_ASSERT(m_uiBufferUsage == 0);
}

//--------------------------------------------------------------------------------------------------
NiSourceTexture* NiTerrainStandardResourceManager::CreateTexture(
    TextureType::Value eType, NiPixelData* pkPixelData)
{
    NiSourceTexturePtr pkResult = NULL;

    // Describe the format preferences of a texture used on the terrain
    NiTexture::FormatPrefs kFormatPrefs = m_akTextureFormats[eType];
    bool bStatic = m_abTextureIsStatic[eType];

    // Create the texture
    pkResult = NiSourceTexture::Create_ThreadSafe(pkPixelData, kFormatPrefs, bStatic);
    EE_ASSERT(pkResult);

    // Register the resource and return
    RegisterResource(pkResult);

    // Notify the listener
    NotifyAllocTexture(eType, pkResult);

    // Verify that the static flag was adhered to. If this assertion is hit, then 
    // something in the renderer code has deleted the source pixel data of this texture
    // even though it has been marked as static.
    EE_VERIFY(bStatic || (pkResult->GetSourcePixelData() != 0));

    return pkResult;
}

//--------------------------------------------------------------------------------------------------
NiRenderedTexture* NiTerrainStandardResourceManager::CreateRenderedTexture(TextureType::Value eType)
{
    NiRenderedTexture* pkResult = NULL;
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();

    switch (eType)
    {
    case TextureType::LOWDETAIL_DIFFUSE:        
        {
            NiTexture::FormatPrefs kFormat;
            kFormat.m_eAlphaFmt = NiTexture::FormatPrefs::SMOOTH;
            kFormat.m_eMipMapped = NiTexture::FormatPrefs::YES;
            kFormat.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
            NiUInt32 uiTextureSize = GetTerrain()->GetLowDetailTextureSize();
            pkResult = NiRenderedTexture::Create(uiTextureSize, uiTextureSize, pkRenderer, kFormat);
        }break;
    default:
        {
            EE_FAIL("NiTerrain::CreateRenderedTexture - Invalid texture type");
        }
    }

    // Register the resource and return
    RegisterResource(pkResult);

    // Notify the listener
    NotifyAllocTexture(eType, pkResult);

    return pkResult;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStandardResourceManager::ReleaseTexture(TextureType::Value eType, 
    NiTexture* pkTexture)
{
    // Don't bother releasing if the object doesn't exist
    if (!pkTexture)
        return;

    // Notify the listener
    NotifyReleaseTexture(eType, pkTexture);

    // De-register the resource
    DeregisterResource(pkTexture);
}

//--------------------------------------------------------------------------------------------------
NiDataStream* NiTerrainStandardResourceManager::CreateStream(StreamType::Value eType, 
    NiUInt32 uiLODLevel)
{
    NiDataStream* pkResult = NULL;

    // Determine the size of the data stream
    NiUInt32 uiBlockWidthInVerts = GetTerrain()->GetCellSize() + 1;
    NiUInt32 uiEntriesPerRegion = uiBlockWidthInVerts * uiBlockWidthInVerts;
    NiUInt32 uiNumRegions = 1;

    if (eType & StreamType::DYNAMIC)
    {
        // Only allocate high detail type streams (in the case of dynamic)
        uiLODLevel = GetTerrain()->GetNumLOD();
    }
    else if (eType == StreamType::INDEX)
    {
        // When creating an index stream the LOD level is special and instead 
        // contains the number of entries to create.
        uiEntriesPerRegion = uiLODLevel;
    }
    else if (eType == StreamType::POSITION ||
        eType == StreamType::NORMAL_TANGENT)
    {
        // Count the number of blocks at this level of detail
        uiNumRegions = 1 << uiLODLevel;
        uiNumRegions *= uiNumRegions;
    }
    NiUInt32 uiNumStreamEntries = uiNumRegions * uiEntriesPerRegion;

    // Determine the format of the data stream and access mode
    NiDataStreamElementSet kElementSet;
    NiUInt8 uiAccessMode = 0;
    NiDataStream::Usage eUsage = NiDataStream::USAGE_VERTEX;
    switch (eType & (~StreamType::DYNAMIC))
    {
    case StreamType::POSITION:
        {
            kElementSet.AddElement(GetPositionStreamFormat(uiLODLevel));
            uiAccessMode = NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_READ;
        } break;
    case StreamType::NORMAL_TANGENT:
        {
            NiDataStreamElement::Format kNormalFormat = GetNormalStreamFormat(uiLODLevel);
            if (kNormalFormat != NiDataStreamElement::F_UNKNOWN)
                kElementSet.AddElement(kNormalFormat);
            NiDataStreamElement::Format kTangentFormat = GetTangentStreamFormat(uiLODLevel);
            if (kTangentFormat != NiDataStreamElement::F_UNKNOWN)
                kElementSet.AddElement(kTangentFormat);
            uiAccessMode = NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_READ;
        } break;
    case StreamType::INDEX:
        {
            NiDataStreamElement::Format eIndexType;
            if (uiEntriesPerRegion > USHRT_MAX)
            {
                eIndexType = NiDataStreamElement::F_UINT32_1;
            }
            else
            {
                eIndexType = NiDataStreamElement::F_UINT16_1;
            }
            kElementSet.AddElement(eIndexType);
            uiAccessMode = NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_READ;
            eUsage = NiDataStream::USAGE_VERTEX_INDEX;
        } break;
    case StreamType::TEXTURE_COORD:
        {
            kElementSet.AddElement(NiDataStreamElement::F_FLOAT32_2);
            uiAccessMode = NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_READ;
        } break;
    default:
        {
            EE_FAIL("NiTerrain::CreateStream - Invalid stream type");
        }
    }

    // If this stream does not store anything, then lets not create it
    if (kElementSet.GetSize() == 0)
        return NULL;

    // Select the appropriate CPU write access
    if (eType & StreamType::DYNAMIC || (eType & ~StreamType::DYNAMIC) == StreamType::NORMAL_TANGENT)
        uiAccessMode |= NiDataStream::ACCESS_CPU_WRITE_MUTABLE;
    else
        uiAccessMode |= NiDataStream::ACCESS_CPU_WRITE_STATIC;

    // Create the data stream:
    pkResult = NiDataStream::CreateDataStream(kElementSet, uiNumStreamEntries, uiAccessMode, 
        eUsage, false);

    if (pkResult)
    {
        EE_ASSERT(pkResult->GetTotalCount() == uiNumStreamEntries);

        // Generate regions within the stream for each block
        NiUInt32 uiRegionIndex = 0;
        for (NiUInt32 ui = 0; ui < uiNumRegions; ++ui)
        {
            NiDataStream::Region kRegion;
            kRegion.SetStartIndex(uiRegionIndex);
            kRegion.SetRange(uiEntriesPerRegion);
            uiRegionIndex += uiEntriesPerRegion;

            pkResult->AddRegion(kRegion);
        }
    }

    // Register the resource
    RegisterResource(pkResult);

    // Notify the listener
    NotifyAllocStream(eType, pkResult, uiLODLevel);

    return pkResult;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStandardResourceManager::ReleaseStream(StreamType::Value eType, 
    NiDataStream* pkStream)
{
    // Don't bother releasing if the object doesn't exist
    if (!pkStream)
        return;

    // Notify the listener
    NotifyReleaseStream(eType, pkStream);

    // De-register the resource
    DeregisterResource(pkStream);
}

//--------------------------------------------------------------------------------------------------
void* NiTerrainStandardResourceManager::CreateBuffer(efd::UInt32 uiNumBytes)
{
    m_kMutex.Lock();

    // Find a buffer that will suit this request
    BufferData* pkResult = NULL;
    FreePoolType::iterator kIter = m_kFreeBufferPool.find(uiNumBytes);
    if (kIter != m_kFreeBufferPool.end() && kIter->second)
    {
        pkResult = kIter->second;
        m_kFreeBufferPool[uiNumBytes] = pkResult->GetNextFree();
        
        // Take this buffer off the free list
        pkResult->SetNextFree(NULL);

        // Take this buffer off the least recently used list
        BeginUsingBuffer(pkResult);
    }

    // If we didn't find a buffer then create one
    if (!pkResult)
    {
        // Setup the new buffer
        pkResult = EE_NEW BufferData(uiNumBytes);

        // Register the buffer
        RegisterResource(pkResult);

        // Update the buffer usage
        m_uiBufferUsage += uiNumBytes;
    }

    m_kMutex.Unlock();

    // Notify the listener
    NotifyAllocBuffer(uiNumBytes, pkResult->GetBuffer());

    // Check if we're meeting our usage targets
    EnforceBufferUsage();

    return pkResult->GetBuffer();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStandardResourceManager::ReleaseBuffer(void* pvBuffer)
{
    // Ignore when the buffer doesn't exist
    if (!pvBuffer)
        return;

    // Notify the listener
    NotifyReleaseBuffer(pvBuffer);

    // Fetch the buffer data
    BufferData* pkBuffer = BufferData::FetchBufferData(pvBuffer);

    // Add it back to the free pool
    {
        m_kMutex.Lock();

        FreePoolType::iterator kIter = m_kFreeBufferPool.find(pkBuffer->GetSize());
        if (kIter != m_kFreeBufferPool.end())
            pkBuffer->SetNextFree(kIter->second);
        m_kFreeBufferPool[pkBuffer->GetSize()] = pkBuffer;

        FinishUsingBuffer(pkBuffer);

        m_kMutex.Unlock();
    }

    // Check if we're meeting our usage targets
    EnforceBufferUsage();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStandardResourceManager::EmptyBufferPool()
{
    m_kMutex.Lock();
    FreePoolType::iterator kIter;
    for (kIter = m_kFreeBufferPool.begin(); kIter != m_kFreeBufferPool.end(); ++kIter)
    {
        BufferData* pkBuffer = kIter->second;
        while(pkBuffer)
        {
            // Update the buffer usage
            m_uiBufferUsage -= pkBuffer->GetSize();

            BufferData* pkNextBuffer = pkBuffer->GetNextFree();
            DeregisterResource(pkBuffer);
            pkBuffer = pkNextBuffer;
        }
    }
    m_kFreeBufferPool.clear();
    m_kMutex.Unlock();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStandardResourceManager::ReleaseAllUnusedBuffers()
{
    EmptyBufferPool();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStandardResourceManager::SetBufferMemoryUsageThreshold(efd::UInt64 uiThreshold)
{
    m_uiBufferUsageThreshold = uiThreshold;
    EnforceBufferUsage();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStandardResourceManager::EnforceBufferUsage()
{
    // Don't do anything if the usage is set to 0 (no threshold) or we are currently below the
    // threshold
    if (m_uiBufferUsageThreshold == 0 || m_uiBufferUsageThreshold > m_uiBufferUsage)
        return;

    m_kMutex.Lock();
    
    // -- We need to free some memory up
    while (m_uiBufferUsage >= m_uiBufferUsageThreshold)
    {
        // Fetch the buffer we are going to free
        BufferData* pkBuffer = m_pkLeastRecentlyUsedBuffer;
        if (!pkBuffer)
            break;

        // Remove this buffer from the heap
        BeginUsingBuffer(pkBuffer);

        // Remove this buffer from the list of buffers for it's size
        FreePoolType::iterator kIter = m_kFreeBufferPool.find(pkBuffer->GetSize());
        EE_ASSERT(kIter != m_kFreeBufferPool.end());
        EE_ASSERT(pkBuffer->GetNextFree() == NULL);

        // Skip to the one before the end of the list
        BufferData* pkPrevious = kIter->second;
        while (pkPrevious && pkPrevious->GetNextFree() != pkBuffer)
            pkPrevious = pkPrevious->GetNextFree();
        
        if (pkPrevious == NULL)
            kIter->second = NULL;
        else
            pkPrevious->SetNextFree(NULL);

        // Update the buffer usage
        m_uiBufferUsage -= pkBuffer->GetSize();

        // Destroy this buffer
        DeregisterResource(pkBuffer);
    }

    m_kMutex.Unlock();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStandardResourceManager::BeginUsingBuffer(BufferData* pkBuffer)
{
    EE_ASSERT(m_kMutex.GetOwningThreadID() == efd::GetCurrentThreadId());

    // Remove this buffer from the least recently used list
    if (pkBuffer == m_pkLeastRecentlyUsedBuffer)
        m_pkLeastRecentlyUsedBuffer = pkBuffer->GetMoreRecentlyUsed();
    if (pkBuffer == m_pkMostRecentlyUsedBuffer)
        m_pkMostRecentlyUsedBuffer = pkBuffer->GetLessRecentlyUsed();
    pkBuffer->BeginUsing();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStandardResourceManager::FinishUsingBuffer(BufferData* pkBuffer)
{
    EE_ASSERT(m_kMutex.GetOwningThreadID() == efd::GetCurrentThreadId());

    if (m_pkLeastRecentlyUsedBuffer == NULL)
    {
        // This is the only buffer not being used
        m_pkLeastRecentlyUsedBuffer = pkBuffer;
        m_pkMostRecentlyUsedBuffer = pkBuffer;
    }
    else
    {
        // Add this buffer to the end of the usage queue
        m_pkMostRecentlyUsedBuffer->SetMoreRecentlyUsed(pkBuffer);
        m_pkMostRecentlyUsedBuffer = pkBuffer;
    }
}

//--------------------------------------------------------------------------------------------------
NiDataStreamElement::Format NiTerrainStandardResourceManager::GetPositionStreamFormat(
    NiUInt32 uiLODLevel)
{
    const NiTerrainConfiguration kConfig = GetTerrain()->GetConfiguration();
    bool bHighDetail = uiLODLevel == GetTerrain()->GetNumLOD();
    NiUInt32 uiNumComponents = kConfig.GetNumPositionComponents(bHighDetail);
    switch (uiNumComponents)
    {
    case 3:
        return NiDataStreamElement::F_FLOAT32_3;
    case 4:
        return NiDataStreamElement::F_FLOAT32_4;
    default:
        EE_FAIL("Invalid number of position components");
    }

    // We should never get here
    return NiDataStreamElement::F_FLOAT32_3;
}

//--------------------------------------------------------------------------------------------------
NiDataStreamElement::Format NiTerrainStandardResourceManager::GetNormalStreamFormat(
    NiUInt32 uiLODLevel)
{
    const NiTerrainConfiguration kConfig = GetTerrain()->GetConfiguration();
    bool bHighDetail = uiLODLevel == GetTerrain()->GetNumLOD();
    NiUInt32 uiNumComponents = kConfig.GetNumNormalComponents(bHighDetail);
    switch (uiNumComponents)
    {
    case 0: 
        return NiDataStreamElement::F_UNKNOWN;
    case 2:
        return NiDataStreamElement::F_FLOAT32_2;
    case 3:
        return NiDataStreamElement::F_FLOAT32_3;
    default:
        EE_FAIL("Invalid number of normal components");
    }

    // We should never get here
    return NiDataStreamElement::F_FLOAT32_3;
}

//--------------------------------------------------------------------------------------------------
NiDataStreamElement::Format NiTerrainStandardResourceManager::GetTangentStreamFormat(
    NiUInt32 uiLODLevel)
{
    const NiTerrainConfiguration kConfig = GetTerrain()->GetConfiguration();
    bool bHighDetail = uiLODLevel == GetTerrain()->GetNumLOD();
    NiUInt32 uiNumComponents = kConfig.GetNumTangentComponents(bHighDetail);
    switch (uiNumComponents)
    {
    case 0: 
        return NiDataStreamElement::F_UNKNOWN;
    case 2:
        return NiDataStreamElement::F_FLOAT32_2;
    case 3:
        return NiDataStreamElement::F_FLOAT32_3;
    default:
        EE_FAIL("Invalid number of tangent components");
    }

    // We should never get here
    return NiDataStreamElement::F_FLOAT32_3;
}

//--------------------------------------------------------------------------------------------------
