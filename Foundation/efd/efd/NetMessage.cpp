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

#include <efd/NetMessage.h>
#include <efd/efdLogIDs.h>
#include <efd/Metrics.h>

//------------------------------------------------------------------------------------------------
using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(efd::EnvelopeMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(efd::NetMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(efd::AssignNetIDMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(efd::RequestNetIDMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(efd::AnnounceNetIDMessage);

const int EnvelopeMessage::kDESTINATION_OFFSET = 0;
const int EnvelopeMessage::kSIZE_BEFORE_MESSAGE_OFFSET = 24;

//------------------------------------------------------------------------------------------------
EnvelopeMessage::EnvelopeMessage(
    IMessage *pMessage,
    const Category &cat,
    efd::UInt32 senderNetID,
    const ConnectionID& sender)
    : NetMessage(sender, senderNetID, cat)
    , m_spMessage(pMessage)
    , m_contentsClassID(efd::k_invalidMessageClassID)
    , m_childIdentity(0)
{
    if (pMessage)
    {
        m_contentsClassID = pMessage->GetClassID();
        m_childIdentity = pMessage->GetUniqueIdentifier();

        // Transfer debug flags from the message to the envelope.
        // Many of the debug flags only apply to the INetLib envelope that always gets created.
        SetDebugFlags(GetDebugFlags() | pMessage->GetDebugFlags());

        EE_MESSAGE_TRACE(
            this,
            efd::kMessageTrace,
            efd::ILogger::kLVL1,
            efd::ILogger::kLVL2,
            ("%s| placed into envelope 0x%08X",
            pMessage->GetDescription().c_str(), GetUniqueIdentifier()));
    }
}


//------------------------------------------------------------------------------------------------
EnvelopeMessage::EnvelopeMessage(
    efd::ClassID idChildType,
    efd::SmartBuffer& childStreamedContents,
    const Category &cat)
    : NetMessage(kCID_INVALID, 0, cat)
    , m_spMessage()
    , m_contentsClassID(idChildType)
    , m_childIdentity(0)
    , m_subMessage(childStreamedContents)
    , m_qos(QOS_INVALID)
{
    // You must specify a valid content type, stream, and size.
    EE_ASSERT(m_contentsClassID);
    EE_ASSERT(m_subMessage.GetBuffer());

    EE_MESSAGE_TRACE(
        this,
        efd::kMessageTrace,
        efd::ILogger::kLVL1,
        efd::ILogger::kLVL2,
        ("%s| created with pre-streamed payload of type %s",
        GetDescription().c_str(), IMessage::ClassIDToString(idChildType).c_str()));
}

//------------------------------------------------------------------------------------------------
EnvelopeMessage::EnvelopeMessage()
    : NetMessage()
    , m_spMessage(NULL)
    , m_contentsClassID(0)
    , m_childIdentity(0)
    , m_subMessage()
    , m_qos(QOS_INVALID)
{}

//------------------------------------------------------------------------------------------------
EnvelopeMessage::~EnvelopeMessage()
{
    m_spMessage = NULL;
}

//------------------------------------------------------------------------------------------------
void EnvelopeMessage::InflateContents(MessageFactory* pMessageFactory) const
{
    // only inflate if the child message is currently NULL
    if (m_spMessage)
    {
        return;
    }

    if (m_subMessage.GetBuffer() == 0)
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR1,
            ("Error: Attempted to call InflateContents on an envelope with no stream."));
        return;
    }

    efd::Archive ar(Archive::Unpacking, m_subMessage);

    efd::ClassID cid;
    Serializer::SerializeObject(cid, ar);
    EE_ASSERT(m_contentsClassID == cid);

    if (cid != kInvalidClassID)
    {
        m_spMessage = pMessageFactory->CreateObject(cid);

        if (!m_spMessage)
        {
            EE_LOG(efd::kMessageTrace, efd::ILogger::kERR1,
                ("%s| failed to factory child 0x%08X of type %s",
                GetDescription().c_str(),
                m_childIdentity,
                IMessage::ClassIDToString(cid).c_str()));
        }
        else
        {
            // fill the message from stream
            Serializer::SerializeObject(*m_spMessage, ar);
            EE_MESSAGE_TRACE(
                this,
                efd::kMessageTrace,
                efd::ILogger::kLVL1, efd::ILogger::kLVL2,
                ("%s: 0x%08X| inflating %s sender NetID=%d",
                IMessage::ClassIDToString(GetClassID()).c_str(),
                GetUniqueIdentifier(),
                m_spMessage->GetDescription().c_str(),
                GetSenderNetID()
           ));
        }
    }
    else
    {
        EE_LOG(efd::kMessageTrace, efd::ILogger::kERR1,
            ("%s| 0x%08X Invalid child class id",
            GetDescription().c_str(),
            m_childIdentity));
    }
}

//------------------------------------------------------------------------------------------------
const IMessage* EnvelopeMessage::GetContents(MessageFactory* pMessageFactory) const
{
    if (!m_spMessage)
    {
        InflateContents(pMessageFactory);
    }
    return m_spMessage;
}


//------------------------------------------------------------------------------------------------
efd::ClassID EnvelopeMessage::GetContentsClassID() const
{
    return m_contentsClassID;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 EnvelopeMessage::GetContentsUniqueIdentifier() const
{
    //!< Identity of contained message for logging non-inflated messages
    return m_childIdentity;
}


//------------------------------------------------------------------------------------------------
efd::utf8string EnvelopeMessage::GetChildDescription() const
{
    efd::utf8string description(efd::Formatted, "%s: 0x%08X",
        IMessage::ClassIDToString(GetContentsClassID()).c_str(), GetContentsUniqueIdentifier());
    return description;
}


//------------------------------------------------------------------------------------------------
void EnvelopeMessage::Serialize(efd::Archive& io_ar)
{
    // NOTE NOTE NOTE: if the format of EnvelopeMessage changes IN ANY WAY then you will get
    // crashes and/or asserts. This is because legacy binaries will send envelopes in the legacy
    // format and we might still receive those envelopes over "connectionless" connections such
    // as UDP or Broadcast UDP. This is a big deal because the NameResolutionServer, used by
    // AssetController, sends broadcast UDP. Unfortunately our connectionless protocols have no
    // freaking version control whatsoever. Someday this will bite us hard and we'll be forced to
    // find a way to version envelope message, until then we are simply locking in stone the
    // serialized format of EnvelopeMessage. Also note that the format of sub-messages can be
    // changed arbitrarily so long as the 32 size is treated the same by envelope message itself.
    // Of course the format of any message actually sent over broadcast UDP shouldn't change ever.

    METRICS_ONLY(UInt32 initialPos = io_ar.GetCurrentPosition();)

    // ************ HUGE NOTE HERE!!! *****************
    // if you change the ordering or location of m_destination, be sure to update
    // kDESTINATION_OFFSET
    // this can be used by specific INetLib implementations for the routing of the packets.
    // Also, don't put any "ifdef" or unknown size calls between here and the sub-message serialize.
    efd::Category cat = GetDestinationCategory();
    Serializer::SerializeObject(cat, io_ar);  // writes 8 bytes
    SetDestinationCategory(cat);
    if (!GetDestinationCategory().IsValid())
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR1,
            ("Error: Envelope serialized with invalid category, %s.",
            GetDestinationCategory().ToString().c_str()));
    }

    Serializer::SerializeObject(m_senderNetID, io_ar);  // writes 4 bytes
    Serializer::SerializeObject(m_contentsClassID, io_ar); // writes 4 bytes

    METRICS_ONLY(UInt32 contentSize = 0;)

    if (io_ar.IsPacking())
    {
        if (m_spMessage)
        {
            // reserve space to serialize the size written
            Archive arSizeWindow = io_ar.MakeWindow(sizeof(UInt32));

            UInt32 bufferStartPos = io_ar.GetCurrentPosition();
            Serializer::SerializeConstObject(m_spMessage->GetClassID(), io_ar); // writes 4 bytes
            Serializer::SerializeObject(*m_spMessage, io_ar);   // sub message can pack however

            // Go back and write the real size into the reserved window:
            UInt32 size = io_ar.GetCurrentPosition() - bufferStartPos;
            Serializer::SerializeObject(size, arSizeWindow);

            METRICS_ONLY(contentSize = size;)
        }
        else if (m_subMessage.GetBuffer())
        {
            // serializing a smart buffer will write the size of the buffer then the buffer.
            Serializer::SerializeObject(m_subMessage, io_ar);
            METRICS_ONLY(contentSize = m_subMessage.GetSize();)
        }
        else
        {
            Serializer::SerializeConstObject(4, io_ar);  // size of a class ID
            Serializer::SerializeConstObject(kInvalidClassID, io_ar);
            METRICS_ONLY(contentSize = 4;)
        }
    }
    else
    {
        // We don't want to unpack the message until we need to, we might be routing a message
        // type that we cannot factory locally. Instead just store the buffer containing the
        // child message. This holds a reference to the original buffer memory.
        Serializer::SerializeObject(m_subMessage, io_ar);
        METRICS_ONLY(contentSize = m_subMessage.GetSize();)
    }

    // We pack in one of three ways but then unpack in just one way. This is highly error prone
    // as each of these three methods makes various assumptions that might not hold true. To help
    // debug possible issues with this we can serialize a sentinel value to check for misalignment.
    // Unfortunately we cannot always have this on because: 1) legacy binaries might receive our
    // envelopes when using broadcast (i.e. NameResolutionService) and would assert or crash if the
    // envelope pack format changes, and 2) we would assert if receiving data from a legacy binary
    // for the same reason. Still, when testing this magic value can be helpful, just be sure to
    // isolate yourself from any legacy NameResolutionService instances (for example, configure a
    // different port if using this service).
    //Serializer::SerializeMagicValue(0xFEDCBA98, io_ar);

    Serializer::SerializeObject(m_childIdentity, io_ar); // 4 bytes

    // moved down here due to the GetDestinationCategory() note above.
    IMessage::Serialize(io_ar);

    METRICS_ONLY(if (io_ar.IsPacking()))
    {
        EE_LOG_METRIC_FMT(kNetwork,
            ("ENVELOPE.HEADER.BYTES.%u.0x%llX", m_senderConnection.GetQualityOfService(),
                m_senderNetID),
            io_ar.GetCurrentPosition() - initialPos - contentSize);

        EE_LOG_METRIC_FMT(kNetwork,
            ("ENVELOPE.TOTAL.BYTES.%u.0x%llX",
                m_senderConnection.GetQualityOfService(),
                m_senderNetID),
            io_ar.GetCurrentPosition() - initialPos);
    }
}

//------------------------------------------------------------------------------------------------
NetMessage::NetMessage():
    m_senderConnection(kCID_INVALID),
    m_senderNetID(0)
{
}

//------------------------------------------------------------------------------------------------
NetMessage::NetMessage(const ConnectionID& cid, efd::UInt32 senderNetID, Category cat):
    m_senderConnection(cid),
    m_senderNetID(senderNetID)
{
    SetDestinationCategory(cat);
}

//------------------------------------------------------------------------------------------------
NetMessage::~NetMessage()
{
}

//------------------------------------------------------------------------------------------------
void NetMessage::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    efd::Category cat = GetDestinationCategory();
    Serializer::SerializeObject(cat, ar);
    SetDestinationCategory(cat);

    Serializer::SerializeObject(m_senderConnection, ar);
    Serializer::SerializeObject(m_senderNetID, ar);
}

//------------------------------------------------------------------------------------------------
RequestNetIDMessage::RequestNetIDMessage()
{
}

//------------------------------------------------------------------------------------------------
RequestNetIDMessage::RequestNetIDMessage(
    const ConnectionID& connectionPrivateCat,
    efd::UInt32 senderNetID,
    efd::Category privateResponseChannel)
    : NetMessage(connectionPrivateCat, senderNetID)
    , m_privateResponseChannel(privateResponseChannel)
{
}

//------------------------------------------------------------------------------------------------
void RequestNetIDMessage::Serialize(efd::Archive& ar)
{
    NetMessage::Serialize(ar);
    Serializer::SerializeObject(m_privateResponseChannel, ar);
}

//------------------------------------------------------------------------------------------------
AssignNetIDMessage::AssignNetIDMessage()
: m_assignedNetID(kNetID_Unassigned)
{
}

//------------------------------------------------------------------------------------------------
AssignNetIDMessage::AssignNetIDMessage(
    const ConnectionID& connectionPrivateCat,
    efd::UInt32 senderNetID,
    efd::UInt32 assignedNetID)
    : NetMessage(connectionPrivateCat, senderNetID)
    , m_assignedNetID(assignedNetID)
{
}

//------------------------------------------------------------------------------------------------
void AssignNetIDMessage::Serialize(efd::Archive& ar)
{
    NetMessage::Serialize(ar);
    Serializer::SerializeObject(m_assignedNetID, ar);
}

//------------------------------------------------------------------------------------------------
AnnounceNetIDMessage::AnnounceNetIDMessage()
{
}

//------------------------------------------------------------------------------------------------
AnnounceNetIDMessage::AnnounceNetIDMessage(
    const ConnectionID& cid,
    efd::UInt32 senderNetID,
    const ConnectionID& announcedConnectionID)
    : NetMessage(cid, senderNetID)
    , m_announcedConnectionID(announcedConnectionID)
{
}

//------------------------------------------------------------------------------------------------
void AnnounceNetIDMessage::Serialize(efd::Archive& ar)
{
    NetMessage::Serialize(ar);
    Serializer::SerializeObject(m_announcedConnectionID, ar);
}

//------------------------------------------------------------------------------------------------
