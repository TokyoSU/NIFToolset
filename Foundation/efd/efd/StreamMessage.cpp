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

#include <efd/StreamMessage.h>
#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>

//------------------------------------------------------------------------------------------------
using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(StreamMessage);


//------------------------------------------------------------------------------------------------
StreamMessage::StreamMessage()
: IMessage()
{
}

//------------------------------------------------------------------------------------------------
StreamMessage::~StreamMessage()
{
}

//------------------------------------------------------------------------------------------------
void StreamMessage::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);

    if (ar.IsPacking())
    {
        Serializer::SerializeConstObject(m_archive.GetUsedBuffer(), ar);
    }
    else
    {
        SmartBuffer sb;
        Serializer::SerializeObject(sb, ar);
        m_archive = Archive(Archive::Unpacking, sb);
    }
}

//------------------------------------------------------------------------------------------------
void StreamMessage::ResetForUnpacking() const
{
    if (m_archive.IsPacking())
    {
        // Create a new buffer for unpacking using just the used portion or the existing archive:
        m_archive = Archive(Archive::Unpacking, m_archive.GetUsedBuffer());
    }
    else
    {
        // Create a new archive starting over at beginning of the old archive's buffer:
        m_archive = Archive(Archive::Unpacking, m_archive.GetEntireBuffer());
    }
}

