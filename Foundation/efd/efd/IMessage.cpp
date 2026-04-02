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

#include <efd/IMessage.h>
#include <efd/BitUtils.h>
#include <efd/EEMath.h>
#include <efd/Serialize.h>

using namespace efd;

/*static*/ IMessage::ParentClassMap* IMessage::ms_pParentClassMap = NULL;

//------------------------------------------------------------------------------------------------
void IMessage::DeleteParentClassMap()
{
    if (ms_pParentClassMap != NULL)
        EE_DELETE ms_pParentClassMap;
    ms_pParentClassMap = NULL;
}

//------------------------------------------------------------------------------------------------
// Utility function to creates a new unique message ID
static efd::UInt32 AssignUniqueIdentifier()
{
    // Rand only returns 15 bits of randomness on most platforms.
    EE_COMPILETIME_ASSERT(RAND_MAX >= 0x7fff);
    // rand can return 0, if it does, twice in a row (unlikely) try again
    efd::UInt32 retVal = 0;
    while (retVal == 0)
    {
        retVal = ((efd::UInt32)efd::Rand()) << 16;
        retVal |= (efd::UInt32)efd::Rand();
    }
    return retVal;
}

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(IMessage);

//------------------------------------------------------------------------------------------------
IMessage::IMessage()
    : m_flags(0)
    , m_uniqueIdentifier(0)
{
}

//------------------------------------------------------------------------------------------------
void IMessage::Serialize(efd::Archive& ar)
{
    // Just in case, force our unique identifier to be created:
    if (ar.IsPacking()) GetUniqueIdentifier();

    Serializer::SerializeObject(m_uniqueIdentifier, ar);
    Serializer::SerializeObject(m_flags, ar);
}

//------------------------------------------------------------------------------------------------
bool IMessage::CheckDebugFlag(efd::UInt32 i_flag) const
{
    return efd::BitUtils::AllBitsAreSet(m_flags, i_flag);
}

//------------------------------------------------------------------------------------------------
efd::utf8string IMessage::GetDescription() const
{
    efd::utf8string description(
        efd::Formatted,
        "%s: 0x%08X",
        IMessage::ClassIDToString(GetClassID()).c_str(),
        GetUniqueIdentifier());

    return description;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 IMessage::GetUniqueIdentifier() const
{
    if (!m_uniqueIdentifier)
    {
        m_uniqueIdentifier = AssignUniqueIdentifier();
    }

    return m_uniqueIdentifier;
}

//------------------------------------------------------------------------------------------------
efd::utf8string IMessage::ClassIDToString(efd::ClassID type)
{
    return efd::utf8string(efd::Formatted, "<MsgType=(0x%08X)>", type);
}

//------------------------------------------------------------------------------------------------
const efd::Category& IMessage::GetSenderCategory() const
{
    return efd::kCAT_LocalMessage;
}

//------------------------------------------------------------------------------------------------
const efd::Category& IMessage::GetDestinationCategory() const
{
    return efd::kCAT_LocalMessage;
}

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(DirectedMessage);

//------------------------------------------------------------------------------------------------
const efd::Category& DirectedMessage::GetSenderCategory() const
{
    return m_senderCategory;
}

//------------------------------------------------------------------------------------------------
const efd::Category& DirectedMessage::GetDestinationCategory() const
{
    return m_destinationCategory;
}

//------------------------------------------------------------------------------------------------
void DirectedMessage::Serialize(efd::Archive& io_ar)
{
    IMessage::Serialize(io_ar);

    Serializer::SerializeObject(m_senderCategory, io_ar);
    Serializer::SerializeObject(m_destinationCategory, io_ar);
}

