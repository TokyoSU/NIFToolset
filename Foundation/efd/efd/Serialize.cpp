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

#include <efd/Serialize.h>

//------------------------------------------------------------------------------------------------
void efd::Serializer::SerializeRawBytes(
    efd::UInt8* i_pData,
    efd::UInt32 i_cbData,
    efd::Archive& io_archive)
{
    efd::UInt8* pBuffer = io_archive.GetBytes(i_cbData);
    if (pBuffer)
    {
        if (io_archive.IsPacking())
        {
            memcpy(pBuffer, i_pData, i_cbData);
        }
        else
        {
            memcpy(i_pData, pBuffer, i_cbData);
        }
    }
}

//------------------------------------------------------------------------------------------------
void efd::Serializer::SerializeStringBuffer(
    char* io_string,
    efd::UInt32 i_bufferSize,
    efd::Archive& io_archive)
{
    EE_ASSERT_MESSAGE(io_string, ("SerializeStringBuffer string pointer cannot be NULL"));
    if (io_archive.IsPacking())
    {
        efd::UInt32 length = efd::Strlen(io_string) + 1;
        efd::UInt8* pBuffer = io_archive.GetBytes(length);
        if (pBuffer)
        {
            memcpy(pBuffer, io_string, length);
        }
    }
    else
    {
        char* pBuffer = (char*)io_archive.PeekBytes(io_archive.GetCurrentPosition(), 0);
        efd::UInt32 maxSize = io_archive.GetRemainingSize();
        for (efd::UInt32 i = 0; i < maxSize && i < i_bufferSize; ++i)
        {
            io_string[i] = pBuffer[i];
            if (0 == pBuffer[i])
            {
                // reached the NULL so we're done, remove the size used from the buffer.
                io_archive.GetBytes(i+1);
                return;
            }
        }
        // if here, ran out of space before finishing
        io_archive.RaiseError();
        // to be safe, make sure the out buffer is NULL terminated even though we're raising
        // an error.
        efd::UInt32 end = ((maxSize > i_bufferSize) ? i_bufferSize : maxSize) - 1;
        io_string[end] = 0;
    }
}

