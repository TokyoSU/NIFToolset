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

#include <efd/SmartBuffer.h>
#include <efd/EEHelpers.h>
#include <efd/Serialize.h>
#include <efd/File.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
SmartBuffer::SmartBuffer()
    : m_pMasterBuffer(NULL)
    , m_cbStart(0)
    , m_cbSize(0)
{
}

//------------------------------------------------------------------------------------------------
SmartBuffer::SmartBuffer(efd::UInt32 i_cbInitialSize)
    : m_pMasterBuffer(NULL)
    , m_cbStart(0)
    , m_cbSize(i_cbInitialSize)
{
    m_pMasterBuffer = EE_NEW GrowableBuffer(i_cbInitialSize);
    m_pMasterBuffer->IncRefCount();
}

//------------------------------------------------------------------------------------------------
SmartBuffer::SmartBuffer(_Borrow, void* i_pData, efd::UInt32 i_cbData)
    : m_cbStart(0)
    , m_cbSize(i_cbData)
{
    m_pMasterBuffer = EE_NEW GrowableBuffer(efd::Borrow, i_pData, i_cbData);
    m_pMasterBuffer->IncRefCount();
}

//------------------------------------------------------------------------------------------------
SmartBuffer::SmartBuffer(_Adopt, void* i_pData, efd::UInt32 i_cbData)
    : m_cbStart(0)
    , m_cbSize(i_cbData)
{
    m_pMasterBuffer = EE_NEW GrowableBuffer(efd::Adopt, i_pData, i_cbData);
    m_pMasterBuffer->IncRefCount();
}

//------------------------------------------------------------------------------------------------
SmartBuffer::SmartBuffer(const SmartBuffer& i_rhs)
    : m_pMasterBuffer(i_rhs.m_pMasterBuffer)
    , m_cbStart(i_rhs.m_cbStart)
    , m_cbSize(i_rhs.m_cbSize)
{
    if (m_pMasterBuffer)
    {
        m_pMasterBuffer->IncRefCount();
    }
}

//------------------------------------------------------------------------------------------------
SmartBuffer::SmartBuffer(GrowableBuffer* sourceBuffer)
    : m_pMasterBuffer(sourceBuffer)
    , m_cbStart(0)
    , m_cbSize(0)
{
    if (EE_VERIFY(sourceBuffer))
    {
        sourceBuffer->IncRefCount();
        m_cbSize = sourceBuffer->GetSize();
    }
}

//------------------------------------------------------------------------------------------------
SmartBuffer::~SmartBuffer()
{
    if (m_pMasterBuffer)
    {
        m_pMasterBuffer->DecRefCount();
    }
}

//------------------------------------------------------------------------------------------------
SmartBuffer& SmartBuffer::operator=(const SmartBuffer& i_rhs)
{
    if (m_pMasterBuffer)
    {
        m_pMasterBuffer->DecRefCount();
    }
    m_pMasterBuffer = i_rhs.m_pMasterBuffer;
    m_cbStart = i_rhs.m_cbStart;
    m_cbSize = i_rhs.m_cbSize;
    if (m_pMasterBuffer)
    {
        m_pMasterBuffer->IncRefCount();
    }
    return *this;
}

//------------------------------------------------------------------------------------------------
void SmartBuffer::ReleaseMasterBuffer()
{
    if (m_pMasterBuffer)
    {
        m_pMasterBuffer->DecRefCount();
    }
    m_cbStart = 0;
    m_cbSize = 0;
}

//------------------------------------------------------------------------------------------------
void SmartBuffer::Adopt(void* i_pData, efd::UInt32 i_cbData)
{
    ReleaseMasterBuffer();
    m_pMasterBuffer = EE_NEW GrowableBuffer(efd::Adopt, i_pData, i_cbData);
    m_pMasterBuffer->IncRefCount();
    m_cbSize = i_cbData;
}

//------------------------------------------------------------------------------------------------
void SmartBuffer::Borrow(void* i_pData, efd::UInt32 i_cbData)
{
    ReleaseMasterBuffer();
    m_pMasterBuffer = EE_NEW GrowableBuffer(efd::Borrow, i_pData, i_cbData);
    m_pMasterBuffer->IncRefCount();
    m_cbSize = i_cbData;
}

//------------------------------------------------------------------------------------------------
efd::UInt8* SmartBuffer::Orphan()
{
    if (m_pMasterBuffer)
    {
        return m_pMasterBuffer->Orphan();
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
bool SmartBuffer::Grow(efd::UInt32 i_cbNewSize)
{
    if (m_cbSize < i_cbNewSize)
    {
        if (!CanGrow())
        {
            return false;
        }

        if (!m_pMasterBuffer)
        {
            m_pMasterBuffer = EE_NEW GrowableBuffer();
            m_pMasterBuffer->IncRefCount();
        }

        m_pMasterBuffer->Grow(i_cbNewSize);
        m_cbSize = i_cbNewSize;
    }
    return true;
}

//------------------------------------------------------------------------------------------------
SmartBuffer SmartBuffer::Clone()
{
    SmartBuffer result;

    if (m_pMasterBuffer)
    {
        result.m_pMasterBuffer = EE_NEW GrowableBuffer();
        result.m_pMasterBuffer->IncRefCount();
        result.m_pMasterBuffer->GrowExact(m_cbSize);
        result.m_cbSize = m_cbSize;
        memcpy(result.GetBuffer(), GetBuffer(), m_cbSize);
    }

    return result;
}

//------------------------------------------------------------------------------------------------
SmartBuffer SmartBuffer::MakeWindow(efd::UInt32 i_start, efd::UInt32 i_size)
{
    // -1 is a special value that means "from i_start to the end"
    if (0xFFFFFFFF == i_size)
    {
        EE_ASSERT(m_cbSize >= i_start);
        i_size = m_cbSize - i_start;
    }

    EE_ASSERT(i_start + i_size <= m_cbSize);

    SmartBuffer result;
    if (m_pMasterBuffer)
    {
        result.m_pMasterBuffer = m_pMasterBuffer;
        result.m_pMasterBuffer->IncRefCount();
        result.m_cbStart = m_cbStart + i_start;
        result.m_cbSize = i_size;
    }
    return result;
}

//------------------------------------------------------------------------------------------------
const SmartBuffer SmartBuffer::MakeWindow(efd::UInt32 i_start, efd::UInt32 i_size) const
{
    return const_cast<SmartBuffer*>(this)->MakeWindow(i_start, i_size);
}

//------------------------------------------------------------------------------------------------
void SmartBuffer::Shrink()
{
    if (m_cbSize)
    {
        EE_ASSERT(m_pMasterBuffer);
        GrowableBuffer* pNewBuff = EE_NEW GrowableBuffer();
        pNewBuff->GrowExact(m_cbSize);
        memcpy(pNewBuff->GetBuffer(), GetBuffer(), m_cbSize);
        m_pMasterBuffer->DecRefCount();
        m_pMasterBuffer = pNewBuff;
        m_pMasterBuffer->IncRefCount();
        m_cbStart = 0;
    }
    else
    {
        // We might have a zero byte window into a buffer, release it just in case.
        ReleaseMasterBuffer();
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt8* SmartBuffer::GetBuffer()
{
    if (m_pMasterBuffer)
    {
        return m_pMasterBuffer->GetBuffer() + m_cbStart;
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
const efd::UInt8* SmartBuffer::GetBuffer() const
{
    if (m_pMasterBuffer)
    {
        return m_pMasterBuffer->GetBuffer() + m_cbStart;
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 SmartBuffer::GetSize() const
{
    return m_cbSize;
}

//------------------------------------------------------------------------------------------------
bool SmartBuffer::CanGrow() const
{
    if (m_pMasterBuffer)
    {
        return m_pMasterBuffer->CanGrow();
    }
    return true;
}

//------------------------------------------------------------------------------------------------
void SmartBuffer::Serialize(Archive& io_ar)
{
    // NOTE: efd::EnvelopeMessage relies on smart buffers using 32bits to serialize the size.

    if (io_ar.IsPacking())
    {
        Serializer::SerializeObject(m_cbSize, io_ar);
        Serializer::SerializeRawBytes(GetBuffer(), m_cbSize, io_ar);
    }
    else
    {
        efd::UInt32 size = EE_UINT32_MAX;
        Serializer::SerializeObject(size, io_ar);
        // Security check: verify size is valid before growing the buffer. If the archive is from
        // an untrusted source we don't want hacked data to consume all memory and cause a crash.
        if (size > io_ar.GetRemainingSize())
        {
            io_ar.RaiseError();
            return;
        }
        // Just make a window into the archive's buffer, we can hold a ref to it's memory.
        *this = io_ar.GetRemainingBuffer().MakeWindow(0, size);
        // advance the buffer past the section we are holding a reference to.
        io_ar.GetBytes(size);
        // The window should be exactly the requested size:
        EE_ASSERT(m_cbSize == size);
    }
}

//------------------------------------------------------------------------------------------------
bool SmartBuffer::ReadFromFile(const efd::utf8string& fileName)
{
    File* pkFile = File::GetFile(fileName.c_str(), File::READ_ONLY);
    if (pkFile)
    {
        efd::UInt32 uiSize = pkFile->GetFileSize();
        if (Grow(uiSize))
        {
            pkFile->Read(GetBuffer(), uiSize);
        }
        EE_DELETE pkFile;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool SmartBuffer::WriteToFile(const efd::utf8string& fileName) const
{
    const efd::UInt8* pData = GetBuffer();
    if (pData)
    {
        File* pkFile = File::GetFile(fileName.c_str(), File::WRITE_ONLY);
        if (pkFile)
        {
            pkFile->Seek(0, File::SO_BEGIN);
            pkFile->Write(pData, GetSize());
            pkFile->Flush();
            EE_DELETE pkFile;
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool SmartBuffer::IsEqual(const SmartBuffer& other)
{
    if (m_cbSize != other.m_cbSize)
    {
        return false;
    }
    if (m_pMasterBuffer == other.m_pMasterBuffer && m_cbStart == other.m_cbStart)
    {
        return true;
    }
    return 0 == memcmp(GetBuffer(), other.GetBuffer(), m_cbSize);
}

//------------------------------------------------------------------------------------------------
