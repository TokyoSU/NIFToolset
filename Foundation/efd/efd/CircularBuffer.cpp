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

#include "efdPCH.h"

#include <efd/CircularBuffer.h>
#include <efd/MemoryDefines.h>
#include <efd/StringUtilities.h>

using namespace efd;

CircularBuffer::CircularBuffer(efd::UInt32 maxBytes)
    : m_readPos(0)
    , m_writePos(0)
    , m_maxBytes(maxBytes)
{
    m_pBuffer = EE_ALLOC(UInt8, maxBytes);
}
CircularBuffer::~CircularBuffer()
{
    EE_FREE(m_pBuffer);
}

efd::UInt32 CircularBuffer::BytesAvailableToRead()
{
    if (IsFull())
        return m_maxBytes-1;
    UInt32 bytesAvailable = BytesAvailableToRead(m_readPos);
    if (m_readPos > m_writePos)
        bytesAvailable += BytesAvailableToRead(0);
    return bytesAvailable;
}

efd::UInt32 CircularBuffer::BytesAvailableToRead(UInt32 readPos)
{
    EE_ASSERT(m_maxBytes > readPos);
    EE_ASSERT(m_maxBytes >= m_writePos);
    if (IsEmpty())
        return 0;

    efd::UInt32 endPos = (readPos < m_writePos) ? m_writePos : m_maxBytes;
    return (m_writePos == readPos) ? 0 : endPos - readPos;
}

efd::UInt32 CircularBuffer::BytesAvailableToWrite()
{
    if (IsEmpty())
        return m_maxBytes-1;
    UInt32 bytesAvailable = BytesAvailableToWrite(m_writePos);
    // check for wrap around space
    if ((m_writePos != 0)
        &&(m_readPos != 0)
        && (m_writePos > m_readPos))
        bytesAvailable += BytesAvailableToWrite(0);
    EE_ASSERT(m_maxBytes > bytesAvailable);
    return bytesAvailable;
}

efd::UInt32 CircularBuffer::BytesAvailableToWrite(UInt32 writePos)
{
    EE_ASSERT(m_maxBytes > m_readPos);
    EE_ASSERT(m_maxBytes >= writePos);
    // buffer full
    if (IsFull())
        return 0;
    efd::UInt32 endPos;
    if ((m_readPos!=0) && writePos <= (m_readPos-1))
    {
        // read pos ahead of write pos
        endPos = (m_readPos-1);
    }
    else
    {
        // write pos ahead of read pos
        endPos = (m_readPos != 0) ? m_maxBytes : m_maxBytes - 1;
    }
    EE_ASSERT(m_maxBytes >= endPos);
    EE_ASSERT(endPos >= 0);
    return endPos - writePos;
}

efd::UInt32 CircularBuffer::Write(efd::UInt8* buffer, efd::UInt32 size)
{
    efd::UInt32 bytesAvailable = BytesAvailableToWrite();
    if (bytesAvailable == 0)
        return 0;
    efd::UInt32 bytesToEnd = BytesAvailableToWrite(m_writePos);
    efd::UInt32 sizeToWrite = (bytesToEnd < size) ? bytesToEnd : size;
    EE_ASSERT(sizeToWrite < m_maxBytes);
    efd::Memcpy(m_pBuffer+m_writePos,buffer,sizeToWrite);
    AdvanceWrite(sizeToWrite);
    efd::UInt32 sizeWritten = sizeToWrite;
    // check for wrap around case
    if (bytesAvailable > bytesToEnd
        && sizeToWrite < size)
    {
        //advance buffer by size already written
        buffer+=sizeToWrite;
        // calculate remaining size
        UInt32 bytesRemaining = bytesAvailable - bytesToEnd;
        UInt32 sizeRemaining = size - sizeToWrite;
        sizeToWrite = (bytesRemaining < sizeRemaining) ? bytesRemaining : sizeRemaining;
        EE_ASSERT(m_writePos == 0);
        m_writePos = 0;
        efd::Memcpy(m_pBuffer+m_writePos,buffer,sizeToWrite);
        AdvanceWrite(sizeToWrite);
        sizeWritten += sizeToWrite;
    }
    EE_ASSERT(sizeWritten <= bytesAvailable);
    EE_ASSERT(sizeWritten < m_maxBytes);
    return sizeWritten;
}

efd::UInt32 CircularBuffer::Read(efd::UInt8* buffer, efd::UInt32 size)
{
    efd::UInt32 bytesAvailable = BytesAvailableToRead();
    if (bytesAvailable == 0)
        return 0;
    efd::UInt32 bytesToEnd = BytesAvailableToRead(m_readPos);
    efd::UInt32 sizeToRead = (bytesToEnd < size) ? bytesToEnd : size;
    efd::Memcpy(buffer,m_pBuffer+m_readPos,sizeToRead);
    AdvanceRead(sizeToRead);
    efd::UInt32 sizeRead = sizeToRead;
    if (bytesAvailable > bytesToEnd
        && sizeToRead < size)
    {
        //advance buffer by size already read
        buffer+=sizeToRead;
        // calculate remaining size
        UInt32 bytesRemaining = bytesAvailable - bytesToEnd;
        UInt32 sizeRemaining = size - sizeToRead;
        sizeToRead = (bytesRemaining < sizeRemaining) ? bytesRemaining : sizeRemaining;
        EE_ASSERT(m_readPos == 0);
        efd::Memcpy(buffer,m_pBuffer+m_readPos,sizeToRead);
        AdvanceRead(sizeToRead);
        sizeRead += sizeToRead;
    }
    EE_ASSERT(sizeRead <= bytesAvailable);
    EE_ASSERT(sizeRead < m_maxBytes);
    return sizeRead;
}

bool CircularBuffer::AdvanceReadTo(UInt32 absoluteBlock)
{
    m_readPos = absoluteBlock;
    EE_ASSERT(m_maxBytes > m_readPos);
    EE_ASSERT(m_maxBytes != m_readPos);
    return true;
}
bool CircularBuffer::AdvanceRead(UInt32 blocksToAdvance)
{
    if (blocksToAdvance == 0)
        return true;
    if (BytesAvailableToRead() < blocksToAdvance)
    {
        return false;
    }
    EE_ASSERT((m_readPos + blocksToAdvance) > m_readPos // normal read
        || (m_readPos + blocksToAdvance) == m_maxBytes); //wrap around read
    m_readPos += blocksToAdvance;
    EE_ASSERT(m_maxBytes >= m_readPos);
    if (m_maxBytes <= m_readPos)
    {
        m_readPos = 0;
    }
    EE_ASSERT(m_maxBytes != m_readPos);
    return true;
}
bool CircularBuffer::AdvanceWriteTo(UInt32 absoluteBlock)
{
    m_writePos = absoluteBlock;
    EE_ASSERT(m_maxBytes >= m_writePos);
    EE_ASSERT(m_maxBytes != m_writePos);
    return true;
}
bool CircularBuffer::AdvanceWrite(UInt32 blocksToAdvance)
{
    if (blocksToAdvance == 0)
        return true;
    if (BytesAvailableToWrite() < blocksToAdvance)
    {
        return false;
    }
    EE_ASSERT(m_maxBytes > m_writePos);
    EE_ASSERT(blocksToAdvance < m_maxBytes);
    EE_ASSERT(m_maxBytes >= (m_writePos + blocksToAdvance)); //wrap around write
    m_writePos += blocksToAdvance;
    if (m_maxBytes == m_writePos)
    {
        m_writePos = 0;
    }
    EE_ASSERT(m_maxBytes > m_writePos);
    //EE_ASSERT(m_maxBytes != m_writePos);
    return true;
}

bool CircularBuffer::IsFull()
{
    return (m_writePos == (m_readPos-1)
        || ((m_writePos == m_maxBytes) && (m_readPos == 1))
        || ((m_writePos == (m_maxBytes-1)) && (m_readPos == 0)));
}

bool CircularBuffer::IsEmpty()
{
    return (m_writePos == m_readPos
        || ((m_readPos == 0) && (m_writePos == m_maxBytes)));
}

void CircularBuffer::Reset()
{
    AdvanceReadTo(0);
    AdvanceWriteTo(0);
}
efd::UInt32 CircularBuffer::GetReadPos()
{
    return m_readPos;
}
efd::UInt32 CircularBuffer::GetWritePos()
{
    return m_writePos;
}
efd::UInt32 CircularBuffer::GetMaxBytes()
{
    return m_maxBytes;
}
efd::UInt8* CircularBuffer::GetBuffer()
{
    return m_pBuffer;
}

