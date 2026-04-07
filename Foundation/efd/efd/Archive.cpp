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

#include <efd/Archive.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
Archive::Archive()
    : m_fPacking(true)
    , m_fError(false)
    , m_buffer()
    , m_CurrentPos(0)
    , m_endianness(efd::Endian_NetworkOrder)
{}

//------------------------------------------------------------------------------------------------
Archive::Archive(_Packing)
    : m_fPacking(true)
    , m_fError(false)
    , m_buffer()
    , m_CurrentPos(0)
    , m_endianness(efd::Endian_NetworkOrder)
{
}

//------------------------------------------------------------------------------------------------
Archive::Archive(_Packing, void* i_pBuffer, efd::UInt32 i_cbBuffer)
    : m_fPacking(true)
    , m_fError(false)
    , m_buffer(Borrow, i_pBuffer, i_cbBuffer)
    , m_CurrentPos(0)
    , m_endianness(efd::Endian_NetworkOrder)
{
}

//------------------------------------------------------------------------------------------------
Archive::Archive(_Packing, const SmartBuffer& i_buffer)
    : m_fPacking(true)
    , m_fError(false)
    , m_buffer(i_buffer)
    , m_CurrentPos(0)
    , m_endianness(efd::Endian_NetworkOrder)
{
}

//------------------------------------------------------------------------------------------------
Archive::Archive(_Unpacking, const void* i_pBuffer, efd::UInt32 i_cbBuffer)
    : m_fPacking(false)
    , m_fError(false)
    // that we are unpacking is our contract to honor the const-ness of this buffer making this
    // cast safe. SmartBuffer cannot make any such promises itself so we do the cast here.
    , m_buffer(Borrow, const_cast<void*>(i_pBuffer), i_cbBuffer)
    , m_CurrentPos(0)
    , m_endianness(efd::Endian_NetworkOrder)
{
}

//------------------------------------------------------------------------------------------------
Archive::Archive(_Unpacking, const SmartBuffer& i_buffer)
    : m_fPacking(false)
    , m_fError(false)
    , m_buffer(i_buffer)
    , m_CurrentPos(0)
    , m_endianness(efd::Endian_NetworkOrder)
{
}

//------------------------------------------------------------------------------------------------
Archive::~Archive()
{
}

//------------------------------------------------------------------------------------------------
void Archive::RaiseError()
{
    // Only assert the first time an error is raised. As most serialization code doesn't check for
    // errors (which is by design) you are likely to hit many asserts after the initial problem
    // that are caused by the initial problem.
    EE_ASSERT_MESSAGE(m_fError, ("Archive serialization failed. "
        "This typically means there is a mis-match between data and code. "
        "Alternately you might be using seperate pack and unpack code which does not match."));
    m_fError = true;
}

//------------------------------------------------------------------------------------------------
efd::UInt8* Archive::GetBytes(efd::UInt32 i_cbSize)
{
    efd::UInt8* pResult = PeekBytes(GetCurrentPosition(), i_cbSize);
    if (pResult)
    {
        m_CurrentPos += i_cbSize;
    }
    return pResult;
}

//------------------------------------------------------------------------------------------------
efd::UInt8* Archive::PeekBytes(efd::UInt32 i_cbOffset, efd::UInt32 i_cbSize)
{
    // It's not safe to use archives in the error state so they always return NULL
    if (GetError())
    {
        return NULL;
    }

    efd::UInt32 cbSizeRequired = i_cbOffset + i_cbSize;

    if (m_buffer.GetSize() < cbSizeRequired)
    {
        // be careful to only call m_buffer.Grow() when packing, it doesn't make sense to grow an
        // unpack buffer even though the source buffer might think its growable.
        if (IsUnpacking() || !m_buffer.Grow(cbSizeRequired))
        {
            RaiseError();
            return NULL;
        }
    }

    return m_buffer.GetBuffer() + i_cbOffset;
}

//------------------------------------------------------------------------------------------------
Archive Archive::MakeWindow(efd::UInt32 i_cbSize)
{
    Archive result;
    result.m_fPacking = m_fPacking;
    result.m_fError = m_fError; // keep in mind we might start off already in an error state
    result.m_endianness = m_endianness;

    // Check for needed size growing our buffer if needed. This also ensures the archive is not
    // already in an error state (and will enter the error state if needed).
    UInt8* pBuffer = PeekBytes(m_CurrentPos, i_cbSize);
    if (pBuffer)
    {
        // Lets make it impossible to leave the window of memory totally untouched by zeroing it
        // out. This is just for safety in case the caller forgets to fill in the entire.
        if (IsPacking())
        {
            memset(pBuffer, 0, i_cbSize);
        }

        result.m_buffer = m_buffer.MakeWindow(m_CurrentPos, i_cbSize);
        // Now actually advance our current position:
        m_CurrentPos += i_cbSize;
    }
    else
    {
        // Don't call RaiseError on result as we don't want to assert again. PeekBytes will have
        // asserted already if that was the first error hit.
        result.m_fError = true;
        result.m_buffer = m_buffer.MakeWindow(m_CurrentPos, 0);
    }

    return result;
}

//------------------------------------------------------------------------------------------------
bool Archive::CheckForUnderflow()
{
    EE_ASSERT(IsUnpacking());
    if (0 != GetRemainingSize())
    {
        RaiseError();
        return false;
    }
    return true;
}

