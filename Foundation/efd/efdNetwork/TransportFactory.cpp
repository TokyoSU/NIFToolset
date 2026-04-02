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

#include "efdNetworkPCH.h"

#include <efdNetwork/TransportFactory.h>
#include <efd/MessageFactory.h>

using namespace efd;

RegisterTransportFactory* RegisterTransportFactory::ms_firstTransportFactory = NULL;

RegisterTransportFactory::RegisterTransportFactory(
    TransportFactoryMethod* pMethod,
    efd::QualityOfService qos)
    : m_pMethod(pMethod)
    , m_qos(qos)
{
    m_nextTransportFactory = ms_firstTransportFactory;
    ms_firstTransportFactory = this;
}

RegisterTransportFactory::~RegisterTransportFactory()
{

}

TransportFactory::TransportFactory()
    : m_lastHeadTransportFactory(NULL)
{

}

TransportFactory::~TransportFactory()
{
    m_factoryMap.clear();
}

INetTransportPtr TransportFactory::CreateTransport(
    MessageFactory* pMessageFactory,
    QualityOfService qualityOfService)
{
    TransportFactoryMethod* factory = NULL;
    if (m_factoryMap.find(qualityOfService, factory))
    {
        return (*factory)(pMessageFactory, qualityOfService);
    }
    return NULL;
}

void TransportFactory::RegisterTransports()
{
    RegisterTransportFactory* curFactory = NULL;

    // Iterate all instances that are in the static list
    curFactory = RegisterTransportFactory::ms_firstTransportFactory;

    while (curFactory != NULL && curFactory != m_lastHeadTransportFactory)
    {
        // Get them registered in the map
        m_factoryMap[curFactory->m_qos] = curFactory->m_pMethod;

        // drain the list.
        curFactory = curFactory->m_nextTransportFactory;
    }
    m_lastHeadTransportFactory = RegisterTransportFactory::ms_firstTransportFactory;
}
