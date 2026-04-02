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

#include <efd/DataStream.h>
#include <efd/Asserts.h>
#include <efd/ILogger.h>
#include <efd/File.h>
#include <efd/efdLogIDs.h>
#include <efd/Serialize.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
DataStream::DataStream(UInt32 initialSize)
{
    m_buffer = EE_ALLOC(UInt8, initialSize);
    m_maxUsableBufferSize = initialSize;
    m_pCurrentStreamPos = m_buffer;
    m_usedBytes = 0;

    m_spStreamInfo = EE_NEW StreamInfo();
    m_readOnly = false;
    m_ownBuffer = true;
}

//------------------------------------------------------------------------------------------------
DataStream::DataStream(efd::UInt8* pBuffer, UInt32 size)
{
    m_buffer = pBuffer;
    m_maxUsableBufferSize = size;
    m_pCurrentStreamPos = m_buffer;
    m_usedBytes = size;

    m_spStreamInfo = EE_NEW StreamInfo();
    m_readOnly = true;
    m_ownBuffer = false;
}

//------------------------------------------------------------------------------------------------
DataStream::DataStream(const DataStream& cpy, bool readOnly)
{
    // drw. don't optimize by sharing buffer references.
    m_buffer = EE_ALLOC(UInt8, cpy.m_maxUsableBufferSize);
    memcpy(m_buffer, cpy.m_buffer, cpy.m_usedBytes);
    m_maxUsableBufferSize = cpy.m_maxUsableBufferSize;
    m_pCurrentStreamPos = m_buffer + (cpy.m_pCurrentStreamPos - cpy.m_buffer);
    m_usedBytes = cpy.m_usedBytes;

    m_spStreamInfo = cpy.m_spStreamInfo;
    m_readOnly = readOnly;  // set if this copy is read only (default is read/write)
    m_ownBuffer = true;
}


//------------------------------------------------------------------------------------------------
DataStream::~DataStream()
{
    if (m_ownBuffer)
    {
        EE_FREE(m_buffer);
    }
    m_buffer = NULL;
    // leave maxBufferSize alone to indicate a deleted DataStream

    m_spStreamInfo = NULL;
}

//------------------------------------------------------------------------------------------------
void DataStream::Grow(efd::UInt32 minSizeRemaining)
{
    UInt32 offset = static_cast<UInt32>(m_pCurrentStreamPos - m_buffer);
    efd::UInt32 growSize = efd::Max((m_usedBytes + minSizeRemaining),
        m_maxUsableBufferSize + kDATA_STREAM_GROW_SIZE);

    m_buffer = static_cast<UInt8*>(EE_REALLOC(m_buffer, growSize));

    // For now, we are not worrying about running out of memory.
    m_pCurrentStreamPos = m_buffer + offset;
    m_maxUsableBufferSize = growSize;
}

//------------------------------------------------------------------------------------------------
void DataStream::ReadFromFile(const efd::utf8string& fileName)
{
    File* pkFile = File::GetFile(fileName.c_str(), File::READ_ONLY);
    efd::UInt32 uiSize = 0;
    if (pkFile)
    {
        uiSize = pkFile->GetFileSize();

        // make sure we have enough buffer space left
        Grow(uiSize);
        pkFile->Read(m_buffer, uiSize);
        EE_DELETE pkFile;
        m_usedBytes = uiSize;
        Reset();
        m_pCurrentStreamPos += uiSize;
     }
}


//------------------------------------------------------------------------------------------------
void DataStream::WriteToFile(const efd::utf8string& fileName) const
{
    File* pkFile = File::GetFile(fileName.c_str(), File::WRITE_ONLY);
    if (pkFile)
    {
        pkFile->Seek(0, File::SO_BEGIN);
        pkFile->Write(m_buffer, m_usedBytes);
        pkFile->Flush();
        EE_DELETE pkFile;
    }
}


//------------------------------------------------------------------------------------------------
void DataStream::ReadRawBuffer(void *pToBuffer, size_t dataSize) const
{
    if (!dataSize)
        return;
    EE_ASSERT(pToBuffer);
    if ((m_pCurrentStreamPos - m_buffer) + dataSize > m_usedBytes)
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR1,
            ("Error, request to read buffer past the end of the written data"));
        EE_ASSERT(false);
        return;
    }

     memcpy(pToBuffer, m_pCurrentStreamPos, dataSize);
     m_pCurrentStreamPos += dataSize;
}


//------------------------------------------------------------------------------------------------
void DataStream::WriteRawBuffer(const void *pFromBuffer, size_t dataSize)
{
    if (m_readOnly)
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR1,
            ("Error, write request to a read-only stream"));
        return;     // if this stream is a read-only clone
    }

    UInt32 freeBytes = m_maxUsableBufferSize - m_usedBytes;
    if (freeBytes < dataSize)
    {
        Grow(dataSize);
    }

     memcpy(m_pCurrentStreamPos, pFromBuffer, dataSize);
     m_pCurrentStreamPos += dataSize;
     UInt32 upToHere = static_cast<UInt32>(m_pCurrentStreamPos - m_buffer);

     // extend the used section if needed. This allows for "weird" seek back and overwrite.
     if (m_usedBytes < upToHere) m_usedBytes = upToHere;
}


//------------------------------------------------------------------------------------------------
void* DataStream::GetRawBufferForWriting(size_t dataSize)
{
    if (m_readOnly)
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR1,
            ("Error, write request to a read-only stream"));
        return NULL;     // if this stream is a read-only clone
    }

    UInt32 freeBytes = m_maxUsableBufferSize - m_usedBytes;
    if (freeBytes < dataSize)
    {
        Grow(dataSize);
    }

    void* pBuffer = m_pCurrentStreamPos;

    m_pCurrentStreamPos += dataSize;
    UInt32 upToHere = static_cast<UInt32>(m_pCurrentStreamPos - m_buffer);

    // extend the used section if needed. This allows for "weird" seek back and overwrite.
    if (m_usedBytes < upToHere)
    {
        m_usedBytes = upToHere;
    }
    return pBuffer;
}


//------------------------------------------------------------------------------------------------
void DataStream::SetRawBufferSize(size_t dataActuallyWritten)
{
    m_usedBytes = dataActuallyWritten;
    // set the current position to the end of the buffer written
    Seek(m_usedBytes);
}


//------------------------------------------------------------------------------------------------
void* DataStream::GetRawBufferForReading()
{
    return m_buffer;
}

//------------------------------------------------------------------------------------------------
// Copy a datastream into me.
void DataStream::Write(const DataStream& dsSource)
{
    if (m_readOnly)
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR1,
            ("Error, write request to a read-only stream"));
        return;     // if this stream is a read-only clone
    }

    // Take the buffer data of the new stream (&val) and concatenate it to our
    // current buffer data.

    //  how big the new stream is (so we can read it back in later)
    // size_t is 64bit on 64bit platforms, we do not support strings that have
    // sizes larger than 32bits
    efd::UInt32 size = static_cast<efd::UInt32>(dsSource.GetSize());

    // Note: we first stream out into the current stream's buffer.
    Write(size);

    // Take the entire contents.
    WriteRawBuffer(dsSource.m_buffer, dsSource.GetSize());

    // For legacy reasons, and if we ever go back to taking lazy copies, or sharing buffer
    // segments.  The source is now read-only.  It would cause all sorts of problems if the
    // original owner continues to write to that stream, so we disallow it.
    dsSource.SetReadOnly();
}

//------------------------------------------------------------------------------------------------
void DataStream::Write(const DataStream &dsSource, size_t offset, size_t size)
{
    if (m_readOnly)
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR1,
            ("Error, write request to a read-only stream"));
        return;     // if this stream is a read-only clone
    }

    // Take the buffer data of the new stream (&val) and concatenate it to our current buffer
    // data.

    // Take the entire contents.
    WriteRawBuffer(dsSource.m_buffer + offset, size);

    // For legacy reasons, and if we ever go back to taking lazy copies, or sharing buffer
    // segments.  The source is now read-only.  It would cause all sorts of problems if the
    // original owner continues to write to that stream, so we disallow it.
    dsSource.SetReadOnly();
}

//------------------------------------------------------------------------------------------------
// Read a data stream from me
void DataStream::Read(DataStream& dest) const
{
    efd::UInt32 extractSize;
    Read(extractSize);     // get size of stream we contain

    // make sure we are not reading more data than is left in the stream
    if ((m_pCurrentStreamPos - m_buffer) + extractSize > m_usedBytes)
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR1,
            ("Error, request to read buffer past the end of the written data"));
        EE_ASSERT(false);
        return;
    }

    UInt8 *tmp = EE_ALLOC(UInt8, extractSize);

    ReadRawBuffer(tmp, extractSize);

    // Fill up the destination
    dest.WriteRawBuffer(tmp, extractSize);
    dest.Reset();

    EE_FREE(tmp);
}

//------------------------------------------------------------------------------------------------
void DataStream::Write(const IStreamable& val)
{
    efd::Archive ar;
    efd::Serializer::SerializeConstObject(val, ar);
    efd::SmartBuffer result = ar.GetUsedBuffer();
    WriteRawBuffer(result.GetBuffer(), result.GetSize());
}

//------------------------------------------------------------------------------------------------
void DataStream::Read(IStreamable& val) const
{
    // Borrow the data stream's buffer:
    Archive ar(Archive::Unpacking, GetRawBuffer(), GetRawBytesRemaining());
    // Read out the data via Serializer:
    Serializer::SerializeObject(val, ar);
    // advance the data stream by the amount used:
    SeekFromCurrentPos(ar.GetUsedSize());
}

//------------------------------------------------------------------------------------------------
// Read a data stream from me
void DataStream::Skip(const DataStream& i_dsDest) const
{
    efd::UInt32 extractSize;
    Read(extractSize);     // get size of stream we contain
    SeekFromCurrentPos(extractSize);
    EE_UNUSED_ARG(i_dsDest);
}


//------------------------------------------------------------------------------------------------
bool DataStream::Seek(size_t offsetPosition) const
{
    if (offsetPosition > m_usedBytes)
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR1,
            ("Error, seek beyond end of DataStream"));
        return false;
    }

    m_pCurrentStreamPos = m_buffer + offsetPosition;

    return true;
}


//------------------------------------------------------------------------------------------------
bool DataStream::ReadAt(size_t offsetPosition, void *pToBuffer, size_t dataSize)
{
    if (offsetPosition + dataSize > m_maxUsableBufferSize) return false;

    Seek(offsetPosition);
    ReadRawBuffer(pToBuffer, dataSize);
    return true;
}


//------------------------------------------------------------------------------------------------
bool DataStream::WriteAt(size_t offsetPosition, const void *pFromBuffer, size_t dataSize)
{
    if (m_readOnly || offsetPosition + dataSize > m_maxUsableBufferSize) return false;

    Seek(offsetPosition);
    WriteRawBuffer(pFromBuffer,dataSize);
    return true;
}


//------------------------------------------------------------------------------------------------
// stream support methods
//------------------------------------------------------------------------------------------------

StreamInfo::StreamInfo()
: m_variant(0)
, m_version(0)
{
#if OLD_BEHAVIOR
    // set default endianness for platform this code is running on
    m_endian = IsHostLittleEndian() ? Endian_Little : Endian_Big;
#else
    // As of 12/11/07 we've decided to initialize streams to assume they are on a native big
    // endian platform.
    m_endian = Endian_NetworkOrder;
#endif
}


//------------------------------------------------------------------------------------------------
StreamInfo::~StreamInfo()
{
}
