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
// Emergent Game Technologies, Calabasas, California 91302
// http://www.emergent.net

#include "efdNetworkPCH.h"

#include <efdNetwork/NetErrorMessage.h>

using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(NetErrorMessage);

//------------------------------------------------------------------------------------------------
NetErrorMessage::NetErrorMessage() :
    NetMessage(kCID_INVALID, 0, kCAT_NetErrorMessage)
{
    // Intentionally left blank
}

//------------------------------------------------------------------------------------------------
NetErrorMessage::NetErrorMessage(const ConnectionID& cid, efd::UInt32 senderNetID) :
    NetMessage(cid, senderNetID, kCAT_NetErrorMessage)
{
    // Intentionally left blank
}

//------------------------------------------------------------------------------------------------
NetErrorMessage::~NetErrorMessage()
{
    // Intentionally left blank
}

//------------------------------------------------------------------------------------------------
void NetErrorMessage::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_netErr, ar);
    Serializer::SerializeObject(m_netErrString, ar);
    Serializer::SerializeObject(m_sysErr, ar);
    Serializer::SerializeObject(m_sysErrString, ar);
}

//------------------------------------------------------------------------------------------------
