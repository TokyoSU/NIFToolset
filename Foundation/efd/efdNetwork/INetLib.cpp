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
#include <efdNetwork/INetLib.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
efd::UInt32 INetLib::ms_netInitCount = 0;
NetLibFactory* INetLib::ms_pNetLibFactory = NULL;

//------------------------------------------------------------------------------------------------
void INetLib::Configure(const ISection* pISection)
{
    EE_UNUSED_ARG(pISection);
}

//------------------------------------------------------------------------------------------------
void INetLib::SendNetIDAnnouncement(
    const ConnectionID& newConnectionID,
    const ConnectionID& destConnectionID)
{
    efd::AnnounceNetIDMessagePtr spNetIdAnnounceMessage = EE_NEW AnnounceNetIDMessage(
        destConnectionID,
        GetNetID(),
        newConnectionID);
    EE_ASSERT(spNetIdAnnounceMessage);
    SendRemote(spNetIdAnnounceMessage, kCAT_NetSystem, destConnectionID);
}

//------------------------------------------------------------------------------------------------
void INetLib::SetMessageFactory(MessageFactory* pMessageFactory)
{
    m_pMessageFactory = pMessageFactory;
}

//------------------------------------------------------------------------------------------------
INetLibPtr INetLib::CreateNetLib(
    const efd::utf8string& netLibType,
    MessageFactory* pMessageFactory,
    const efd::ISection* pSection)
{
    if (ms_pNetLibFactory == NULL)
        ms_pNetLibFactory = EE_NEW NetLibFactory();
    INetLibPtr spNetLib = ms_pNetLibFactory->CreateObject(netLibType);
    if (spNetLib)
    {
        spNetLib->SetMessageFactory(pMessageFactory);

        if (pSection)
        {
            spNetLib->Configure(pSection);
        }
    }
    return spNetLib;
}

//------------------------------------------------------------------------------------------------
efd::ConnectionID INetLib::GetChannelManagerConnectionID(QualityOfService qualityOfService)
{
    ConnectionID foundConnectionID;
    m_channelManagerQOSToIDMap.find(qualityOfService, foundConnectionID);
    return foundConnectionID;
}

//------------------------------------------------------------------------------------------------
void INetLib::SetChannelManagerConnectionID(const efd::ConnectionID& CMConnectionID)
{
    m_channelManagerQOSToIDMap[CMConnectionID.GetQualityOfService()] = CMConnectionID;
}

//------------------------------------------------------------------------------------------------
void INetLib::_SDMShutdown()
{
    if (ms_pNetLibFactory != NULL)
        EE_DELETE ms_pNetLibFactory;
    ms_pNetLibFactory = NULL;
}
