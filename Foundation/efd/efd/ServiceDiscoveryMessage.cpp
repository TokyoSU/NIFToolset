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

#include <efd/ServiceDiscoveryMessage.h>


//------------------------------------------------------------------------------------------------
using namespace efd;
using namespace efd;
using namespace efd;


EE_IMPLEMENT_CONCRETE_CLASS_INFO(ServiceDiscoveryRequest);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ServiceDiscoveryResponse);


//------------------------------------------------------------------------------------------------
ServiceDiscoveryRequest::ServiceDiscoveryRequest()
    : m_context(0)
    , m_respondTo()
{
}


//------------------------------------------------------------------------------------------------
ServiceDiscoveryRequest::ServiceDiscoveryRequest(efd::UInt32 i_userData, efd::Category i_callback)
    : m_context(i_userData)
    , m_respondTo(i_callback)
{
}

//------------------------------------------------------------------------------------------------
void ServiceDiscoveryRequest::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_context, ar);
    Serializer::SerializeObject(m_respondTo, ar);
}

//------------------------------------------------------------------------------------------------
ServiceDiscoveryResponse::ServiceDiscoveryResponse()
    : m_discoveredService(efd::kInvalidClassID)
    , m_context(0)
    , m_servicePrivateChannel()
{
}

//------------------------------------------------------------------------------------------------
ServiceDiscoveryResponse::ServiceDiscoveryResponse(
    efd::SSID i_discoveredService,
    efd::UInt32 i_context,
    efd::Category i_discoveredChannel)
    : m_discoveredService(i_discoveredService)
    , m_context(i_context)
    , m_servicePrivateChannel(i_discoveredChannel)
{
}

//------------------------------------------------------------------------------------------------
void ServiceDiscoveryResponse::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_discoveredService, ar);
    Serializer::SerializeObject(m_context, ar);
    Serializer::SerializeObject(m_servicePrivateChannel, ar);
}

//------------------------------------------------------------------------------------------------
