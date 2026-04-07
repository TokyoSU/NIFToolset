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

//--------------------------------------------------------------------------------------------------
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any
//     purpose is hereby granted without fee, provided that the above
//     copyright notice appear in all copies and that both that copyright
//     notice and this permission notice appear in supporting documentation.
// The author or Addison-Welsey Longman make no representations about the
//     suitability of this software for any purpose. It is provided "as is"
//     without express or implied warranty.
//--------------------------------------------------------------------------------------------------

// Precompiled Header
#include "efdPCH.h"

#include <efd/RTLib.h>
#include <efd/FixedSizeAllocator.h>
#include <efd/Asserts.h>
#include <efd/UniversalTypes.h>
#include <efd/IAllocator.h>
#include <efd/ILogger.h>
#include <algorithm>

using namespace efd;

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Chunk::Init
// Initializes a chunk object
//--------------------------------------------------------------------------------------------------

void FixedSizeAllocator::Chunk::Init(IAllocator* pkAllocator,
    size_t stBlockSize, unsigned char ucBlocks)
{
    EE_ASSERT(pkAllocator);
    EE_ASSERT(stBlockSize > 0);
    EE_ASSERT(ucBlocks > 0);
    // Overflow check
    EE_ASSERT((stBlockSize * ucBlocks) / stBlockSize == ucBlocks);

    size_t stSizeInBytes = sizeof(unsigned char)*stBlockSize*ucBlocks;
    size_t stAlignment = EE_MEM_ALIGNMENT_DEFAULT;

    m_pucData = reinterpret_cast<unsigned char*>(pkAllocator->Allocate(
        stSizeInBytes,
        stAlignment,
        MemHint::USAGE_POOL,
        EE_MET_ALIGNEDMALLOC,
        __FILE__, __LINE__, __FUNCTION__));

#if defined(EE_EFD_CONFIG_DEBUG)
    if (m_pucData == 0)
        EE_OUTPUT_DEBUG_STRING("WARNING: Failed to allocate pool memory.  "
            "No more small allocator chunks can be created.\n");
#endif

    EE_ASSERT(m_pucData);

    Reset(stBlockSize, ucBlocks);
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Chunk::Reset
// Clears an already allocated chunk
//--------------------------------------------------------------------------------------------------

void FixedSizeAllocator::Chunk::Reset(size_t stBlockSize,
    unsigned char ucBlocks)
{
    EE_ASSERT(stBlockSize > 0);
    EE_ASSERT(ucBlocks > 0);
    // Overflow check
    EE_ASSERT((stBlockSize * ucBlocks) / stBlockSize == ucBlocks);

    m_ucFirstAvailableBlock = 0;
    m_ucBlocksAvailable = ucBlocks;

    unsigned char i = 0;
    unsigned char* p = m_pucData;
    for (; i != ucBlocks; p += stBlockSize)
    {
        *p = ++i;
    }
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Chunk::Release
// Releases the data managed by a chunk
//--------------------------------------------------------------------------------------------------

void FixedSizeAllocator::Chunk::Release(IAllocator* pkAllocator)
{
    EE_ASSERT(pkAllocator);

    pkAllocator->Deallocate(m_pucData, EE_MET_ALIGNEDFREE, 0);
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Chunk::Allocate
// Allocates a block from a chunk
//--------------------------------------------------------------------------------------------------

void* FixedSizeAllocator::Chunk::Allocate(size_t stBlockSize)
{
    if (!m_ucBlocksAvailable)
        return 0;

    EE_ASSERT((m_ucFirstAvailableBlock * stBlockSize) / stBlockSize ==
        m_ucFirstAvailableBlock);

    unsigned char* pucResult =
        m_pucData + (m_ucFirstAvailableBlock * stBlockSize);
    m_ucFirstAvailableBlock = *pucResult;
    --m_ucBlocksAvailable;

    return pucResult;
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Chunk::Deallocate
// Dellocates a block from a chunk
//--------------------------------------------------------------------------------------------------

void FixedSizeAllocator::Chunk::Deallocate(void* p, size_t stBlockSize)
{
    EE_ASSERT(p >= m_pucData);

    unsigned char* toRelease = static_cast<unsigned char*>(p);
    // Alignment check
    EE_ASSERT((toRelease - m_pucData) % stBlockSize == 0);

    *toRelease = m_ucFirstAvailableBlock;
    m_ucFirstAvailableBlock = static_cast<unsigned char>(
        (toRelease - m_pucData) / stBlockSize);
    // Truncation check
    EE_ASSERT(m_ucFirstAvailableBlock == (toRelease - m_pucData) / stBlockSize);

    ++m_ucBlocksAvailable;
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Chunk::HasAvailable
//--------------------------------------------------------------------------------------------------
bool FixedSizeAllocator::Chunk::HasAvailable(unsigned char ucNumBlocks) const
{
    return m_ucBlocksAvailable == ucNumBlocks;
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Chunk::HasAvailable
//--------------------------------------------------------------------------------------------------
bool FixedSizeAllocator::Chunk::HasBlock(unsigned char * p,
    size_t stChunkLength) const
{
    return (m_pucData <= p) && (p < m_pucData + stChunkLength);
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Chunk::HasAvailable
//--------------------------------------------------------------------------------------------------
bool FixedSizeAllocator::Chunk::IsFilled(void) const
{
    return (0 == m_ucBlocksAvailable);
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::FixedSizeAllocator
//--------------------------------------------------------------------------------------------------

FixedSizeAllocator::FixedSizeAllocator()
    : m_pkAllocator(0)
    , m_stBlockSize(0)
    , m_pkChunks(0)
    , m_stNumChunks(0)
    , m_stMaxNumChunks(0)
    , m_pkAllocChunk(0)
    , m_pkDeallocChunk(0)
    , m_pkEmptyChunk(0)
    , m_kCriticalSection("FxdAllc")
{
}

//--------------------------------------------------------------------------------------------------
// Creates a FixedSizeAllocator object of a fixed block size
//--------------------------------------------------------------------------------------------------
void FixedSizeAllocator::Init(IAllocator* pkAllocator,
    size_t stBlockSize,
    size_t stOptimalChunkSize)
{
    EE_ASSERT(pkAllocator);

    m_pkAllocator = pkAllocator;
    m_stNumChunks  = 0;
    m_stMaxNumChunks = 0;
    m_pkChunks = NULL;
    m_pkAllocChunk = NULL;
    m_stBlockSize = stBlockSize;
    size_t stNumBlocks = stOptimalChunkSize / stBlockSize;

    if (stNumBlocks > UCHAR_MAX)
        stNumBlocks = UCHAR_MAX;
    else if (stNumBlocks == 0)
        stNumBlocks = 8 * stBlockSize;

    m_ucNumBlocks = static_cast<unsigned char>(stNumBlocks);
    EE_ASSERT(m_ucNumBlocks == stNumBlocks);
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::~FixedSizeAllocator
//--------------------------------------------------------------------------------------------------

FixedSizeAllocator::~FixedSizeAllocator()
{
    EE_ASSERT(m_pkAllocator);

    for (size_t i = 0; i < m_stNumChunks; ++i)
    {
       // Note: This test has been commented out because the MemTracker
       // will report any leaks.
       // EE_ASSERT(i->m_ucBlocksAvailable == m_ucNumBlocks);
       m_pkChunks[i].Release(m_pkAllocator);
    }

    m_pkAllocator->Deallocate(m_pkChunks, EE_MET_ALIGNEDFREE, 0);
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Push_Back
// Adds a chunk to the end of the chunks array
//--------------------------------------------------------------------------------------------------
void FixedSizeAllocator::Push_Back(Chunk& kChunk)
{
    size_t stCount = m_stNumChunks;
    Reserve(m_stNumChunks+1);
    m_pkChunks[stCount] = kChunk;
    m_stNumChunks++;
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Push_Back
// Adds a chunk to the end of the chunks array
//--------------------------------------------------------------------------------------------------
void FixedSizeAllocator::Pop_Back()
{
    --m_stNumChunks;
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Reserve
// Guarantees space for a certain number of chunks
//--------------------------------------------------------------------------------------------------
void FixedSizeAllocator::Reserve(size_t stNewSize)
{
    EE_ASSERT(m_pkAllocator);

    if (stNewSize > m_stMaxNumChunks)
    {
        size_t stNewSizeInBytes = stNewSize*sizeof(Chunk);
        size_t stCurrentSizeInBytes = m_stMaxNumChunks*sizeof(Chunk);
        size_t stAlignment = EE_MEM_ALIGNMENT_DEFAULT;

        m_pkChunks = reinterpret_cast<Chunk*>(m_pkAllocator->Reallocate(
            m_pkChunks,
            stNewSizeInBytes,
            stAlignment,
            MemHint::USAGE_POOL,
            EE_MET_ALIGNEDREALLOC,
            stCurrentSizeInBytes,
            __FILE__, __LINE__, __FUNCTION__));

        EE_ASSERT(m_pkChunks);

        m_stMaxNumChunks = stNewSize;
    }
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Allocate
// Allocates a block of fixed size
//--------------------------------------------------------------------------------------------------

void* FixedSizeAllocator::Allocate(PerThreadSmallBlockCache* pkCache)
{
    void* pvResult;
    if (pkCache)
    {
        if (pkCache->ppHeadBlock == NULL)
            FillCache(pkCache);
        pvResult = pkCache->ppHeadBlock;
        pkCache->ppHeadBlock = (void**)(*pkCache->ppHeadBlock);
        --pkCache->stSize;
    }
    else
    {
        m_kCriticalSection.Lock();
        pvResult = AllocateInternal();
        m_kCriticalSection.Unlock();
    }

    return pvResult;
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::Deallocate
// Deallocates a block previously allocated with Allocate
// (undefined behavior if called with the wrong pointer)
//--------------------------------------------------------------------------------------------------

void FixedSizeAllocator::Deallocate(void* p, PerThreadSmallBlockCache* pkCache)
{
    EE_ASSERT(p);
    if (pkCache)
    {
        if (pkCache->stSize >= MaxCacheCount)
            ReleaseCache(pkCache);
        void** pp = (void**)p;
        *pp = pkCache->ppHeadBlock;
        pkCache->ppHeadBlock = pp;
        ++pkCache->stSize;
    }
    else
    {
        m_kCriticalSection.Lock();
        DeallocateInternal(p);
        m_kCriticalSection.Unlock();
    }
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::VicinityFind (internal)
// Finds the chunk corresponding to a pointer, using an efficient search
//--------------------------------------------------------------------------------------------------

FixedSizeAllocator::Chunk* FixedSizeAllocator::VicinityFind(void* p)
{
    EE_ASSERT(m_stNumChunks != 0);
    EE_ASSERT(m_pkDeallocChunk);

    const size_t stChunkLength = m_ucNumBlocks * m_stBlockSize;

    Chunk* lo = m_pkDeallocChunk;
    Chunk* hi = m_pkDeallocChunk + 1;
    Chunk* loBound = &m_pkChunks[0];
    Chunk* hiBound = &m_pkChunks[m_stNumChunks - 1] + 1;

    if (hi == hiBound)
        hi = 0;

    for (;;)
    {
        if (lo)
        {
            if (lo->HasBlock((unsigned char*)p, stChunkLength))
                return lo;

            if (lo == loBound)
            {
                lo = NULL;
                if (NULL == hi)
                    break;
            }
            else
            {
                --lo;
            }
        }

        if (hi)
        {
            if (hi->HasBlock((unsigned char*)p, stChunkLength))
                return hi;
            if (++hi == hiBound)
            {
                hi = NULL;
                if (NULL == lo)
                    break;
            }
        }
    }

    EE_FAIL("Could not find pointer p in FixedSizeAllocator::VicinityFind()");
    return 0;
}

//--------------------------------------------------------------------------------------------------
// FixedSizeAllocator::DoDeallocate (internal)
// Performs deallocation. Assumes m_pkDeallocChunk points to the correct chunk
//--------------------------------------------------------------------------------------------------

void FixedSizeAllocator::DoDeallocate(void* p)
{
    EE_ASSERT(m_pkDeallocChunk->m_pucData <= p);
    EE_ASSERT(m_pkDeallocChunk->HasBlock(static_cast<unsigned char *>(p),
        m_ucNumBlocks * m_stBlockSize));

    // prove either m_pkEmptyChunk points nowhere, or points to a truly empty
    // Chunk.
    EE_ASSERT((NULL == m_pkEmptyChunk) ||
           (m_pkEmptyChunk->HasAvailable(m_ucNumBlocks)));

    // call into the chunk, will adjust the inner list but won't release memory
    m_pkDeallocChunk->Deallocate(p, m_stBlockSize);

    if (m_pkDeallocChunk->HasAvailable(m_ucNumBlocks))
    {
        EE_ASSERT(m_pkEmptyChunk != m_pkDeallocChunk);

        // m_pkDeallocChunk is empty, but a Chunk is only released if there
        // are 2 empty chunks.  Since m_pkEmptyChunk may only point to a
        // previously cleared Chunk, if it points to something else
        // besides m_pkDeallocChunk, then FixedSizeAllocator currently has 2 empty
        // Chunks.
        if (NULL != m_pkEmptyChunk)
        {
            // If last Chunk is empty, just change what m_pkDeallocChunk
            // points to, and release the last.  Otherwise, swap an empty
            // Chunk with the last, and then release it.
            Chunk* pkLastChunk = &m_pkChunks[m_stNumChunks - 1];

            if (pkLastChunk == m_pkDeallocChunk)
            {
                m_pkDeallocChunk = m_pkEmptyChunk;
            }
            else if (pkLastChunk != m_pkEmptyChunk)
            {
                EE_STL_NAMESPACE::swap(*m_pkEmptyChunk, *pkLastChunk);
            }

            EE_ASSERT(pkLastChunk->HasAvailable(m_ucNumBlocks));
            pkLastChunk->Release(m_pkAllocator);
            Pop_Back();
            m_pkAllocChunk = m_pkDeallocChunk;
        }
        m_pkEmptyChunk = m_pkDeallocChunk;
    }

    // prove either m_pkEmptyChunk points nowhere, or points to a truly empty
    // Chunk.
    EE_ASSERT((NULL == m_pkEmptyChunk) ||
           (m_pkEmptyChunk->HasAvailable(m_ucNumBlocks)));
}

//--------------------------------------------------------------------------------------------------

void* FixedSizeAllocator::AllocateInternal()
{
    // Prove that the empty chunk points to either nothing or
    // a chunk with no elements allocated.
    EE_ASSERT((NULL == m_pkEmptyChunk) ||
        (m_pkEmptyChunk->HasAvailable(m_ucNumBlocks)));

    if (m_pkAllocChunk && m_pkAllocChunk->IsFilled())
        m_pkAllocChunk = NULL;

    // Recycle the empty chunk if possible
    if (NULL != m_pkEmptyChunk)
    {
        m_pkAllocChunk = m_pkEmptyChunk;
        m_pkEmptyChunk = NULL;
    }

    if (m_pkAllocChunk == 0)
    {
        for (size_t i = 0; i < m_stNumChunks; ++i)
        {
            if (!m_pkChunks[i].IsFilled())
            {
                m_pkAllocChunk = &m_pkChunks[i];
                break;
            }
        }

        // If no alloc chunk has space,
        // add an alloc chunk
        if (NULL == m_pkAllocChunk)
        {
            // Initialize
            Reserve(m_stNumChunks + 1);
            Chunk newChunk;
            newChunk.Init(m_pkAllocator, m_stBlockSize, m_ucNumBlocks);
            Push_Back(newChunk);
            m_pkAllocChunk = &m_pkChunks[m_stNumChunks - 1];
            m_pkDeallocChunk = &m_pkChunks[0];
        }
    }
    EE_ASSERT(m_pkAllocChunk != 0);
    EE_ASSERT(!m_pkAllocChunk->IsFilled());

    // Prove that the empty chunk points to either nothing or
    // a chunk with no elements allocated.
    EE_ASSERT((NULL == m_pkEmptyChunk) ||
        (m_pkEmptyChunk->HasAvailable(m_ucNumBlocks)));


    void* pvMem = m_pkAllocChunk->Allocate(m_stBlockSize);

    return pvMem;
}

//--------------------------------------------------------------------------------------------------

void FixedSizeAllocator::DeallocateInternal(void* p)
{
    EE_ASSERT(m_stNumChunks != 0);
    EE_ASSERT(&m_pkChunks[0] <= m_pkDeallocChunk);
    EE_ASSERT(&m_pkChunks[m_stNumChunks - 1] >= m_pkDeallocChunk);

    m_pkDeallocChunk  = VicinityFind(p);
    EE_ASSERT(m_pkDeallocChunk);

    DoDeallocate(p);
}

//--------------------------------------------------------------------------------------------------

void FixedSizeAllocator::FillCache(PerThreadSmallBlockCache* pkCache)
{
    EE_ASSERT(pkCache);
    EE_ASSERT(m_stBlockSize >= sizeof(void*));
    m_kCriticalSection.Lock();
    for (size_t st = 0; st < CacheRefillCount; ++st)
    {
        // Write the pointer to the existing chain into the new block, and make it the new head
        void** ppNewBlock = (void**)AllocateInternal();
        *ppNewBlock = (void*)(pkCache->ppHeadBlock);
        pkCache->ppHeadBlock = ppNewBlock;
        ++pkCache->stSize;
    }
    m_kCriticalSection.Unlock();
}

//--------------------------------------------------------------------------------------------------

void FixedSizeAllocator::ReleaseCache(PerThreadSmallBlockCache* pkCache, size_t stReleaseCount)
{
    EE_ASSERT(pkCache);
    EE_ASSERT(m_stBlockSize >= sizeof(void*));
    EE_ASSERT(pkCache->stSize >= stReleaseCount);
    m_kCriticalSection.Lock();
    for (size_t st = 0; st < stReleaseCount; ++st)
    {
        void* p = pkCache->ppHeadBlock;
        pkCache->ppHeadBlock = (void**)(*pkCache->ppHeadBlock);
        DeallocateInternal(p);
        --pkCache->stSize;
    }
    m_kCriticalSection.Unlock();
}

//--------------------------------------------------------------------------------------------------

void FixedSizeAllocator::ReleaseAllElementsInCache(PerThreadSmallBlockCache* pkCache)
{
    EE_ASSERT(pkCache);
    if (pkCache->stSize >= sizeof(void*))
    {
        ReleaseCache(pkCache, pkCache->stSize);
    }
}

//--------------------------------------------------------------------------------------------------
