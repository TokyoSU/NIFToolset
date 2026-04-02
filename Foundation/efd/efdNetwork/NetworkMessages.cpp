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

#include "efdNetwork/NetworkMessages.h"


//------------------------------------------------------------------------------------------------
using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(NetworkStatus);

//------------------------------------------------------------------------------------------------
NetworkStatus::~NetworkStatus()
{
}

//------------------------------------------------------------------------------------------------
void NetworkStatus::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_qos, ar);
    Serializer::SerializeObject(m_cid, ar);
    Serializer::SerializeObject(m_netId, ar);
}

//------------------------------------------------------------------------------------------------
