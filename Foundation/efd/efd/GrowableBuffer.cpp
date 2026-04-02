// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 2006-2009 Todd Berkebile.
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include "efdPCH.h"

#include <efd/GrowableBuffer.h>
#include <efd/EEHelpers.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
// GrowableBuffer
//------------------------------------------------------------------------------------------------
GrowableBuffer::GrowableBuffer()
    : m_pData(NULL)
    , m_cbData(0)
    , m_fOwnsBuffer(true)
    , m_fCanGrow(true)
{
}

//------------------------------------------------------------------------------------------------
GrowableBuffer::GrowableBuffer(efd::UInt32 i_cbInitialSize)
: m_pData(NULL)
, m_cbData(0)
, m_fOwnsBuffer(true)
, m_fCanGrow(true)
{
    GrowExact(i_cbInitialSize);
}

//------------------------------------------------------------------------------------------------
GrowableBuffer::GrowableBuffer(_Borrow, void* i_pData, efd::UInt32 i_cbData)
    : m_pData(static_cast<efd::UInt8*>(i_pData))
    , m_cbData(i_cbData)
    , m_fOwnsBuffer(false)
    , m_fCanGrow(false)
{
}

//------------------------------------------------------------------------------------------------
GrowableBuffer::GrowableBuffer(_Adopt, void* i_pData, efd::UInt32 i_cbData)
    : m_pData(static_cast<efd::UInt8*>(i_pData))
    , m_cbData(i_cbData)
    , m_fOwnsBuffer(true)
    , m_fCanGrow(true)
{
}

//------------------------------------------------------------------------------------------------
GrowableBuffer::~GrowableBuffer()
{
    FreeBuffer();
}

//------------------------------------------------------------------------------------------------
efd::UInt32 GrowableBuffer::Grow(efd::UInt32 i_cbSize)
{
    GrowExact(GetAllocSizeFromMinSize(i_cbSize));
    return m_cbData;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 GrowableBuffer::GrowExact(efd::UInt32 i_cbSize)
{
    if (i_cbSize > m_cbData)
    {
        efd::UInt8* pNewData = EE_EXTERNAL_NEW efd::UInt8[i_cbSize];
        if (m_pData)
        {
            memcpy(pNewData, m_pData, m_cbData);
            FreeBuffer();
        }
        m_pData = pNewData;
        m_cbData = i_cbSize;
    }
    return m_cbData;
}

//------------------------------------------------------------------------------------------------
void GrowableBuffer::Adopt(void* i_pData, efd::UInt32 i_cbData)
{
    FreeBuffer();
    m_pData = static_cast<efd::UInt8*>(i_pData);
    m_cbData = i_cbData;
    m_fOwnsBuffer = true;
    m_fCanGrow = true;
}

//------------------------------------------------------------------------------------------------
void GrowableBuffer::Borrow(void* i_pData, efd::UInt32 i_cbData)
{
    FreeBuffer();
    m_pData = static_cast<efd::UInt8*>(i_pData);
    m_cbData = i_cbData;
    m_fOwnsBuffer = false;
    m_fCanGrow = false;
}

//------------------------------------------------------------------------------------------------
efd::UInt8* GrowableBuffer::Orphan()
{
    if (m_fOwnsBuffer)
    {
        m_fOwnsBuffer = false;
        m_fCanGrow = false;
        return m_pData;
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
void GrowableBuffer::FreeBuffer()
{
    if (m_fOwnsBuffer)
    {
        EE_EXTERNAL_DELETE [] m_pData;
    }
    m_pData = NULL;
    m_cbData = 0;
}

//------------------------------------------------------------------------------------------------
// NOTE: On Intel architecture this could be optimized with the BSR asm instruction.
efd::UInt32 GrowableBuffer::RoundUpToNextPowerOfTwo(efd::UInt32 num)
{
    // Zero will return one
    if (num)
    {
        // Subtract 1 to catch cases where uiNum is already a power of 2.
        num -= 1;

        // Propagate the leftmost active bit across the field. This will give us
        // a power of 2 - 1. e.g. 0xFF is 255 which is 2^8 - 1
        num |= num >> 1;
        num |= num >> 2;
        num |= num >> 4;
        num |= num >> 8;
        num |= num >> 16;
    }

    // Add one to round up to a power of 2 and return.
    return num + 1;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 GrowableBuffer::GetAllocSizeFromMinSize(efd::UInt32 i_cbMinSize)
{
    // We want to allocate chunks that fit nicely into our memory pool. We also want to avoid
    // allocating too often.
    static const efd::UInt32 k_cbMinSize = 128;             // allocate at least 128 bytes
    static const efd::UInt32 k_cbMaxSize = 1024*16;         // limit for doubling region
    static const efd::UInt32 k_cbLargeGrowthSize = 1024*4;  // size to grow if over k_cbMaxSize

    // small buffers use the min size:
    if (i_cbMinSize <= k_cbMinSize)
    {
        return k_cbMinSize;
    }
    // big buffers round up to an even unit of k_cbLargeGrowthSize:
    if (i_cbMinSize >= k_cbMaxSize)
    {
        // Pad by at least half the growth size but allocate an even sized unit:
        efd::UInt32 units = (i_cbMinSize + k_cbLargeGrowthSize - 1) / k_cbLargeGrowthSize;
        return k_cbLargeGrowthSize * units;
    }

    // mid-sized buffers grow to the next highest power of two:
    return RoundUpToNextPowerOfTwo(i_cbMinSize);
}

//------------------------------------------------------------------------------------------------
