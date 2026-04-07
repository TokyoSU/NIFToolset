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

#include <efd/Metrics.h>
#include <efd/ILogger.h>
#include <efd/MessageService.h>
#include <efd/ServiceManager.h>
#include <efd/NetMessage.h>
#include <efd/StreamMessage.h>
#include <efd/IBase.h>
#include <efd/IConfigManager.h>
#include <efd/ServiceDiscoveryMessage.h>
#include <efd/IDs.h>

#include <efdNetwork/NetService.h>
#include <efdNetwork/NetListener.h>
#include <efdNetwork/INetLib.h>
#include <efdNetwork/ChannelManagerService.h>

#include <efdNetwork/NetLib.h>
#include <efd/QOSCompare.h>
#include <efd/EnumManager.h>
#include <efdNetwork/NetErrorMessage.h>
#include <functional>
#include <algorithm>

//------------------------------------------------------------------------------------------------
using namespace efd;

const char* NetService::kConfigSection = "NetService";
const char* NetService::kNetLibType = "NetLibType";
const char* NetService::kNetLib = "NetLib";
const char* NetService::kWaitBeforeReconnect = "WaitBeforeReconnect";
const char* NetService::kAutoConnect = "AutoConnect";
const char* NetService::kAutoReconnect = "AutoReconnect";
const char* NetService::kAssignDefaultNetID = "AssignDefaultNetID";
const char* NetService::kWaitTicksOnShutdown = "WaitTicksOnShutdown";

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NetListener);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NetService);

//------------------------------------------------------------------------------------------------
void NetService::SetConnectionStatus(QualityOfService qos, ConnectionStatus status)
{
    EE_ASSERT(!m_ConfigList.empty());
    for (ChannelManagerConfigList::iterator it = m_ConfigList.begin();
        it != m_ConfigList.end();
        ++it)
    {
        ChannelManagerConfigPtr spConfig = *it;
        if (QOSCompare(spConfig->m_qualityOfService) == qos)
        {
            spConfig->m_connectionStatus = status;
        }
    }
}

//------------------------------------------------------------------------------------------------
void NetService::HandleNetMessage(
    const IMessage* pIncomingMessage,
    const ConnectionID& senderConnectionID)
{
    //check to see if this is a NetMessage w/o an envelope
    switch (pIncomingMessage->GetClassID())
    {
    case kMSGID_ConnectionAcceptedMsg:
        if (QOSCompare(QOS_RELIABLE) == senderConnectionID.GetQualityOfService())
        {
            EE_FAIL("This case should never happen!  NetService does not Listen for connections.");
        }
        return;

    case kMSGID_ConnectionConnectedMsg:
        {
            efd::UInt32 ip = senderConnectionID.GetIP();
            efd::UInt16 remotePort = senderConnectionID.GetRemotePort();
            QualityOfService senderQOS = senderConnectionID.GetQualityOfService();

            efd::utf8string ipStr = m_spNetLib->IPToString(ip);

            if (QOSCompare(QOS_RELIABLE) == senderQOS)
            {
                m_channelManagerIP = ipStr;
                m_channelManagerListenPort = remotePort;

                SetChannelManagerConnectionID(senderConnectionID);
                EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
                    ("%s> Set ChannelManager connection ID to %s.",
                    __FUNCTION__, senderConnectionID.ToString().c_str()));

                SetConnectionStatus(senderQOS, kCS_Connected);
            }
            else
            {
                EE_ASSERT(m_channelManagerNetID != kNetID_Unassigned);
                SetChannelManagerConnectionID(senderConnectionID);
                NetIDQosPair netIDQos(m_channelManagerNetID,senderQOS);
                m_NetIDQosLookupMap[netIDQos]=senderConnectionID;
                CheckPendingSubscriptions();
                SetConnectionStatus(senderQOS, kCS_AnnounceNetID);
            }
            // we do not send connection notification here, we must wait until we have received all
            // subscriptions from the other side
        }
        return;

    case kMSGID_ConnectionDisconnectedMsg:
        {
            UInt32 cmNetID = GetChannelManagerNetID();
            m_spNetLib->RemoveRemoteConsumer(senderConnectionID);
            SendConnectionUpdate<kMSGID_NetServiceConnectionClosed>(
                m_spMessageService,
                senderConnectionID,
                cmNetID);
            // if the CM has disconnected, we attempt to reconnect to it

            if (QOSCompare(QOS_RELIABLE) == senderConnectionID.m_qos)
            {
                if (m_connectionStatus == kCS_Canceled)
                {
                    EE_LOG(efd::kNetMessage,
                        efd::ILogger::kLVL1,
                        ("%s> kMSGID_ConnectionDisconnectedMsg m_connectionStatus = "
                        "kCS_Disconnected",
                        __FUNCTION__));
                    m_connectionStatus = kCS_Disconnected;
                }
                else
                {
                    EE_LOG(efd::kNetMessage,
                        efd::ILogger::kLVL1,
                        ("%s> kMSGID_ConnectionDisconnectedMsg m_connectionStatus = kCS_Failed",
                        __FUNCTION__));
                    m_connectionStatus = kCS_Failed;
                }

                efd::TimeType curTime = m_pServiceManager->GetServiceManagerTime();
                // attempt to reconnect at the specified time in the future
                m_reconnectTime = curTime + m_reconnectWaitTime;
                EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
                    ("%s> Set ChannelManager connection ID to %s because %s closed.",
                    __FUNCTION__, kCID_INVALID.ToString().c_str(),
                    senderConnectionID.ToString().c_str()));
                SetChannelManagerConnectionID(kCID_INVALID);
                SetNetID(kNetID_Unassigned);
                // close all connections because we lost our reliable connection
                EE_ASSERT(!m_ConfigList.empty());
            }
            else
            {
                // iterate through netid/qos to cid map so we can remove stale connections
                NetIDQosPairToConnectionIDMap::iterator it = m_NetIDQosLookupMap.begin();
                while (it != m_NetIDQosLookupMap.end())
                {
                    if ((*it).second == senderConnectionID)
                    {
                        NetIDQosPairToConnectionIDMap::iterator eraseIt = it++;
                        m_NetIDQosLookupMap.erase(eraseIt);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }

            for (ChannelManagerConfigList::iterator it = m_ConfigList.begin();
                it != m_ConfigList.end();
                ++it)
            {
                ChannelManagerConfigPtr spConfig = *it;
                if (QOSCompare(spConfig->m_qualityOfService) == senderConnectionID.m_qos)
                {
                    if (spConfig->m_connectionStatus == kCS_Canceled)
                    {
                        spConfig->m_connectionStatus = kCS_Disconnected;
                    }
                    else
                    {
                        spConfig->m_connectionStatus = kCS_Failed;
                    }
                    if (spConfig->m_connectionID != kCID_INVALID)
                    {
                        m_spNetLib->CloseConnection(spConfig->m_connectionID);
                    }
                    spConfig->m_connectionID = kCID_INVALID;
                }
            }
        }

        return;

    case kMSGID_ConnectionFailedToConnectMsg:
        {
            EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
                ("%s> Connection Failed %s",
                __FUNCTION__, senderConnectionID.ToString().c_str()));

            SendConnectionUpdate<kMSGID_NetServiceConnectionFailed>(
                m_spMessageService,
                senderConnectionID,
                kNetID_Unassigned);

            EE_ASSERT(!m_ConfigList.empty());
            SetConnectionStatus(senderConnectionID.m_qos, kCS_Failed);

            // if we haven't received our NetID yet, try to reconnect
            // we don't need check for ChannelManagerService because we already tried to connect
            if (QOSCompare(QOS_RELIABLE) == senderConnectionID.m_qos)
            {
                EE_LOG(efd::kNetMessage,
                    efd::ILogger::kLVL1,
                    ("%s> kMSGID_ConnectionFailedToConnectMsg m_connectionStatus = kCS_Failed "
                    "for %s",
                    __FUNCTION__,
                    senderConnectionID.ToString().c_str()));
                m_connectionStatus = kCS_Failed;
                efd::TimeType curTime = m_pServiceManager->GetServiceManagerTime();
                // attempt to reconnect at the specified time in the future
                m_reconnectTime = curTime + m_reconnectWaitTime;
            }
        }
        return;

    case kMSGID_ConnectionDataReceivedMsg:
        {
            const EnvelopeMessage* pConstMessage =
                EE_DYNAMIC_CAST(const EnvelopeMessage, pIncomingMessage);
            EnvelopeMessage* pEnvelopeMessage = const_cast<EnvelopeMessage*>(pConstMessage);
            // make sure we have the proper kind of message
            EE_ASSERT(pEnvelopeMessage);

            // check to see if the message originated from us through the ChannelManager
            if (pEnvelopeMessage->GetSenderNetID() == GetNetID() &&
                senderConnectionID == GetChannelManagerConnectionID(
                senderConnectionID.GetQualityOfService()) &&
                !m_pChannelManagerService)
            {
                EE_LOG(efd::kNetMessage, efd::ILogger::kLVL3,
                    ("%s> Discarding message from self received from ChannelManager %s, "
                    "cat = %s, connection = %s",
                    __FUNCTION__,
                    senderConnectionID.ToString().c_str(),
                    pEnvelopeMessage->GetDestinationCategory().ToString().c_str(),
                    senderConnectionID.ToString().c_str()));
                return;
            }
            // process the message...
            const Category& cat = pEnvelopeMessage->GetDestinationCategory();
            CheckPendingSubscriptions(
                pEnvelopeMessage->GetDestinationCategory(),
                pEnvelopeMessage->GetQualityOfService());

            // first, check if a system message...
            if (cat == kCAT_NetSystem)
            {
                // Call the system message handler
                HandleSystemMessage(pEnvelopeMessage);
            }
            else
            {
                // Anything on a different Category can be handled by the MessageService for
                // local delivery
                m_spMessageService->HandleNetMessage(pIncomingMessage, senderConnectionID);
            }
        }
        return;

    case kMSGID_NetErrorMessage:
        {
            // Forward the message, so subscribers can handle it.
            m_spMessageService->SendImmediate(pIncomingMessage, kCAT_NetErrorMessage);
        }
        return;

    default:
        // Error getting message contents
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("Error: Received message on %s, but %s type is not recognized",
            senderConnectionID.ToString().c_str(), pIncomingMessage->GetDescription().c_str()));

        // tick off another error occurred
        EE_LOG_METRIC_COUNT(kNetwork, "MESSAGE.RECEIVE.SYSTEM_MESSAGE.ERROR");

        return;
    }

}

//------------------------------------------------------------------------------------------------
NetService::NetService(efd::utf8string netLibType)
    : m_spNetLib(NULL)
    , m_channelManagerListenPort(0)
    , m_channelManagerNetID(kNetID_Unassigned)
    , m_autoReconnectToChannelManager(true)
    , m_connectionStatus(kCS_Canceled)
    , m_reconnectTime(0.0)
    , m_reconnectWaitTime(10.0)
    , m_netLibType(netLibType)
    , m_readNetLibConfig(true)
    , m_pChannelManagerService(NULL)
    , m_assignClientNetID(true)
    , m_waitTicksOnShutdown(60)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 5700;

    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL0,
        ("Creating a NetService instance 0x%08X", this));

    INetLib::RegisterNetLib<NetLib>(kNetLib);

    // if netLibType is passed in we force creation of the type instead of reading the config value
    if (!m_netLibType.empty())
    {
        m_readNetLibConfig = false;
    }
    else
    {
        m_netLibType = kNetLib;
    }
}

//------------------------------------------------------------------------------------------------
NetService::NetService(INetLib* pNetLib)
    : m_spNetLib(pNetLib)
    , m_channelManagerListenPort(0)
    , m_channelManagerNetID(kNetID_Unassigned)
    , m_autoReconnectToChannelManager(true)
    , m_connectionStatus(kCS_Canceled)
    , m_reconnectTime(0.0)
    , m_reconnectWaitTime(10.0)
    , m_readNetLibConfig(true)
    , m_pChannelManagerService(NULL)
    , m_assignClientNetID(true)
    , m_waitTicksOnShutdown(60)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 5700;

    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL0,
        ("Creating a NetService instance 0x%08X", this));

    INetLib::RegisterNetLib<NetLib>(kNetLib);
    m_readNetLibConfig = false;
}

//------------------------------------------------------------------------------------------------
NetService::~NetService()
{
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL0,
        ("Destroying a NetService instance 0x%08X", this));
    m_spNetLib = NULL;
}


//------------------------------------------------------------------------------------------------
void NetService::Subscribe(
    const Category& cat,
    INetCallback* pCallback)
{
    if (!pCallback)
    {
        pCallback = this;
    }
    EE_ASSERT(cat.IsValid());

    CategoryCallback categoryCallback(cat,pCallback);
    CategoryRefcount::iterator it= m_subscriberRefcount.find(categoryCallback);
    if (it != m_subscriberRefcount.end())
    {
        ++((*it).second);
    }
    else
    {
        m_spNetLib->AddLocalConsumer(cat, pCallback);
        if (m_spNetLib->GetNetID() != kNetID_Unassigned)
        {
            m_spNetLib->SendAddLocalConsumerRequest(cat);
        }
        NonAtomicRefCount refCount(1);
        m_subscriberRefcount[categoryCallback] = refCount;
    }
}


//------------------------------------------------------------------------------------------------
void NetService::Unsubscribe(const Category& cat, INetCallback* pCallback)
{
    if (!pCallback)
    {
        pCallback = this;
    }
    EE_ASSERT(cat.IsValid());
    CategoryCallback categoryCallback(cat,pCallback);
    CategoryRefcount::iterator it= m_subscriberRefcount.find(categoryCallback);
    if (it != m_subscriberRefcount.end())
    {
        --((*it).second);
    }
    else
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("%s> Tried to Unsubscribe %s not subscribed by callback 0x%08X",
            __FUNCTION__, cat.ToString().c_str(),pCallback));
        return;
    }

    if (((*it).second) <= 0)
    {
        if (m_spNetLib->GetNetID() != kNetID_Unassigned)
        {
            m_spNetLib->SendRemoveLocalConsumerRequest(cat);
        }
        m_spNetLib->RemoveLocalConsumer(cat, pCallback);
        m_subscriberRefcount.erase(it);
    }
}


//------------------------------------------------------------------------------------------------
void NetService::Unsubscribe(INetCallback* callback)
{
    m_spNetLib->RemoveLocalConsumer(callback);
}


//------------------------------------------------------------------------------------------------
void NetService::UnsubscribeAll()
{
    m_spNetLib->RemoveAllLocalConsumers();
    m_spNetLib->AddLocalConsumer(kCAT_NetSystem, this);
}


//------------------------------------------------------------------------------------------------
void NetService::SendRemote(IMessage* pMessage, const Category &cat, QualityOfService virtualQOS)
{
    QualityOfService defaultQOS = QOSCompare::LookupPhysical(virtualQOS);
    EE_ASSERT(cat.IsValid());
    if (defaultQOS == QOS_INVALID)
    {
        defaultQOS = m_spNetLib->GetTransport(cat);
        if (defaultQOS == QOS_INVALID)
        {
            EE_FAIL_MESSAGE(("%s| %s has no QOS, you must either call BeginCategoryProduction, or "
                "specify a valid QualityOfService in defaultQOS.",
                pMessage->GetDescription().c_str(),
                cat.ToString().c_str()));
            return;
        }
    }

    // If the QOS is total order or we do not yet know about any consumers for this Category
    // let the ChannelManager decide who to send this message to.
    // If we have a ChannelManager pointer, let it always do the send.
    if (m_pChannelManagerService ||
        defaultQOS & NET_TOTAL_ORDER ||
        !m_spNetLib->HasRemoteConsumers(cat))
    {
        ConnectionID cmConnectionID = GetChannelManagerConnectionID(defaultQOS);

        if (cmConnectionID != kCID_INVALID)
        {
            // we have a valid connection to a ChannelManager
            SendRemote(pMessage, cat, cmConnectionID);
        }
    }
    else
    {
        CheckPendingSubscriptions(cat, defaultQOS);
        m_spNetLib->SendRemote(pMessage, cat, defaultQOS);
    }
}


//------------------------------------------------------------------------------------------------
void NetService::SendRemote(IMessage* pMessage, const Category& cat, const ConnectionID& cid)
{
    EE_ASSERT(cat.IsValid());
    CheckPendingSubscriptions(cat, cid.GetQualityOfService());
    if (!m_pChannelManagerService)
    {
        m_spNetLib->SendRemote(pMessage, cat, cid);
    }
    else
    {
        // envelope message and deliver to ChannelManager directly
        EnvelopeMessagePtr spEnvelopeMessage =
            EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_ConnectionDataReceivedMsg>;
        EE_ASSERT(spEnvelopeMessage);
        spEnvelopeMessage->SetChild(pMessage);
        spEnvelopeMessage->SetDestinationCategory(cat);
        spEnvelopeMessage->SetSenderNetID(GetNetID());
        spEnvelopeMessage->SetSenderConnection(cid);
        spEnvelopeMessage->SetQualityOfService(cid.GetQualityOfService());
        m_pChannelManagerService->HandleMessage(spEnvelopeMessage);
    }
}


//------------------------------------------------------------------------------------------------
void NetService::ProducerSendRemote(
    IMessage* pMessage,
    const Category& categoryProduced,
    QualityOfService virtualQOS)
{
    QualityOfService defaultQOS = QOSCompare::LookupPhysical(virtualQOS);
    EE_ASSERT(categoryProduced.IsValid());
    // only ChannelManager uses INetLib::ProducerSendRemote
    // we just Send the message to the ChannelManager because only
    // ChannelManager can know who to deliver to w/o a race condition
    //m_spNetLib->ProducerSendRemote(pMessage, categoryProduced);
    EnvelopeMessagePtr spInnerEnvelopeMessage =
        EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_SendToProducer>;
    EE_ASSERT(spInnerEnvelopeMessage);
    spInnerEnvelopeMessage->SetChild(pMessage);
    spInnerEnvelopeMessage->SetDestinationCategory(categoryProduced);
    EE_MESSAGE_TRACE(
        pMessage,
        efd::kMessageTrace,
        efd::ILogger::kLVL0,
        efd::ILogger::kLVL2,
        ("%s| sending to producer of %s",
        pMessage->GetDescription().c_str(),
        categoryProduced.ToString().c_str()));
    SendRemote(
        spInnerEnvelopeMessage,
        kCAT_SendToProducer,
        GetChannelManagerConnectionID(defaultQOS));
}

//------------------------------------------------------------------------------------------------
void NetService::CreateNetLib(efd::utf8string netLibType)
{
    const ISection* pConfigSection = NULL;
    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    if (pConfigManager)
    {
        pConfigSection = pConfigManager->GetConfiguration();
    }

    m_spNetLib = INetLib::CreateNetLib(netLibType, m_spMessageService, pConfigSection);
}

//------------------------------------------------------------------------------------------------
void NetService::OnServiceRegistered(efd::IAliasRegistrar* pAliasRegistrar)
{
    pAliasRegistrar->AddIdentity<INetService>();
    INetService::OnServiceRegistered(pAliasRegistrar);
}

//------------------------------------------------------------------------------------------------
efd::SyncResult NetService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    EE_ASSERT(m_pServiceManager);
    pDependencyRegistrar->AddDependency<IConfigManager>();
    pDependencyRegistrar->AddDependency<ChannelManagerService>(sdf_Optional);

    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    if (pConfigManager)
    {
        const ISection *pSection = pConfigManager->GetConfiguration()->FindSection(kConfigSection);

        if (pSection)
        {
            efd::utf8string temp;
            if (m_readNetLibConfig)
            {
                temp = pSection->FindValue(kNetLibType);
                if (!temp.empty())
                {
                    m_netLibType = temp;
                }
            }

            temp = pSection->FindValue(kWaitBeforeReconnect);
            // already set to a reasonable default
            if (!temp.empty())
            {
                m_reconnectWaitTime = (efd::UInt16)atoi(temp.c_str());
            }

            efd::Bool tempBool;
            if (!pSection->FindValue(kAutoConnect).empty())
            {
                tempBool = pSection->IsTrue(kAutoConnect, true);
                AutoConnectToChannelManager(tempBool);
            }

            tempBool = pSection->IsTrue(kAutoReconnect, true);
            AutoReconnectToChannelManager(tempBool);

            tempBool = pSection->IsTrue(kAssignDefaultNetID, true);
            m_assignClientNetID = tempBool;

            temp = pSection->FindValue(kWaitTicksOnShutdown);
            // already set to a reasonable default
            if (!temp.empty())
            {
                m_waitTicksOnShutdown = (efd::UInt32)atoi(temp.c_str());
            }
        }
    }

    // read in QOS mapping from config file
    EnumManager* pEnumManager = m_pServiceManager->GetSystemServiceAs<EnumManager>();
    QOSCompare::ReadConfig(pConfigManager, pEnumManager);

    if (!m_spMessageService)
    {
        m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    }

    if (!m_spMessageService)
    {
        // The net service requires a Message service to run.  Both would need to be added at
        // the same time so if I can't find it just fail.
        EE_LOG(efd::kMessage, efd::ILogger::kERR0,
            ("%s> Unable to find MessageService!!!!", __FUNCTION__));
        return efd::SyncResult_Failure;
    }

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
void NetService::NotifyMessageService()
{
    EE_ASSERT(m_spMessageService);
    m_spMessageService->SetNetService(this);
    // make sure that MessageService gets producer send messages
    Subscribe(kCAT_SendToProducer, m_spMessageService);
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult NetService::OnInit()
{
    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    if (!m_spNetLib)
    {
        if (!m_pChannelManagerService)
        {
            const ISection* pConfigSection = pConfigManager->GetConfiguration();
            m_spNetLib = INetLib::CreateNetLib(m_netLibType, m_spMessageService, pConfigSection);
            if (!m_spNetLib)
            {
                EE_LOG(efd::kChannelManager, efd::ILogger::kERR0,
                    ("Failed to create a NetLib of type '%s', "
                    "ensure INetLib::RegisterNetLib was called",
                    m_netLibType.c_str()));

                return efd::AsyncResult_Failure;
            }
        }
        else
        {
            m_spNetLib = m_pChannelManagerService->GetNetLib();
            if (!m_spNetLib)
            {
                // ChannelManager will have already logged about the failure to create the net lib
                return efd::AsyncResult_Failure;
            }
        }

        if (m_assignClientNetID && !m_pChannelManagerService)
        {
            SetNetID(kNetID_Client);
        }

        // tell MessageService we are ready to process subscriptions
        NotifyMessageService();

        if (m_ConfigList.empty())
        {
            ChannelManagerService::ReadConfig(pConfigManager, m_ConfigList);
        }

        // initialize INetLib
        if (INetLib::StartNet() <= 0)
        {
            EE_LOG(efd::kNetwork, ILogger::kERR0, ("NetService::OnInit failed INetLib::StartNet."));
            return efd::AsyncResult_Failure;
        }

        // The calls to RegisterMessageWrapperFactory here are for networking system internal use
        // only. Begin NetService internal message factory registration.
        RegisterMessageFactory<ServiceDiscoveryRequest>(m_spMessageService);
        RegisterMessageFactory<ServiceDiscoveryResponse>(m_spMessageService);
        RegisterMessageWrapperFactory< StreamMessage, kMSGID_SubscribeExternal >(
            m_spMessageService);
        RegisterMessageWrapperFactory< StreamMessage, kMSGID_UnsubscribeExternal >(
            m_spMessageService);
        RegisterMessageWrapperFactory< StreamMessage, kMSGID_BeginCategoryProduction >(
            m_spMessageService);
        RegisterMessageWrapperFactory< StreamMessage, kMSGID_EndCategoryProduction >(
            m_spMessageService);
        RegisterMessageWrapperFactory< StreamMessage, kMSGID_NetServiceConnectionEstablished >(
            m_spMessageService);
        RegisterMessageWrapperFactory< StreamMessage, kMSGID_NetServiceConnectionClosed >(
            m_spMessageService);
        RegisterMessageWrapperFactory< NetMessage, kMSGID_ConnectionDataReceivedMsg >(
            m_spMessageService);
        RegisterMessageWrapperFactory< EnvelopeMessage, kMSGID_RemoteWrapper >(m_spMessageService);
        RegisterMessageWrapperFactory< EnvelopeMessage, kMSGID_SendToProducer >(m_spMessageService);
        RegisterMessageWrapperFactory< NetMessage, kMSGID_Resubscribe >(m_spMessageService);
        RegisterMessageWrapperFactory< IMessage, kMSGID_ResubscribeComplete >(m_spMessageService);
        RegisterMessageFactory<RequestNetIDMessage>(m_spMessageService);
        RegisterMessageFactory<AnnounceNetIDMessage>(m_spMessageService);
        RegisterMessageWrapperFactory<AnnounceNetIDMessage, kMSGID_AckNetID>(m_spMessageService);
        RegisterMessageWrapperFactory< NetMessage, kMSGID_ConnectionFailedToConnectMsg >(
            m_spMessageService);
        RegisterMessageWrapperFactory< NetMessage, kMSGID_ConnectionConnectedMsg >(
            m_spMessageService);
        RegisterMessageWrapperFactory< NetMessage, kMSGID_ConnectionFailedToAcceptMsg >(
            m_spMessageService);
        RegisterMessageWrapperFactory< NetMessage, kMSGID_ConnectionAcceptedMsg >(
            m_spMessageService);
        RegisterMessageWrapperFactory< NetMessage, kMSGID_ConnectionDisconnectedMsg >(
            m_spMessageService);
        RegisterMessageWrapperFactory< StreamMessage, kMSGID_UnreliableVersion >(
            m_spMessageService);
        RegisterMessageFactory<AssignNetIDMessage>(m_spMessageService);
        RegisterMessageWrapperFactory<AssignNetIDMessage, kMSGID_ChannelManagerClientJoin>(
            m_spMessageService);
        RegisterMessageWrapperFactory<AssignNetIDMessage, kMSGID_ChannelManagerClientLeave>(
            m_spMessageService);
        // End NetService internal message factory registration.

        m_spNetLib->AddLocalConsumer(kCAT_NetSystem, this);
        BeginCategoryProduction(kCAT_NetSystem, QOS_RELIABLE, this);
    }

    if (!INetLib::IsNetReady())
    {
        return efd::AsyncResult_Pending;
    }

    EE_LOG(efd::kNetwork, ILogger::kLVL0,
        ("%s> Complete. using NetID=%d",
        __FUNCTION__,
        m_spNetLib->GetNetID()));
    return efd::AsyncResult_Complete;
}


//------------------------------------------------------------------------------------------------
efd::AsyncResult NetService::OnShutdown()
{
    static UInt32 tickWaited = 0;
    UInt32 queuedMessages = m_spNetLib->QueryOutgoingQueueSize();

    if (queuedMessages && (tickWaited++ < m_waitTicksOnShutdown))
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
            ("%s waiting for %d messages to be sent. Tick %d of %d",
            __FUNCTION__,
            queuedMessages,
            tickWaited,
            m_waitTicksOnShutdown));
        return OnTick();
    }

    m_spMessageService = NULL;

    if (!m_pChannelManagerService)
    {
        m_spNetLib->Shutdown();
        INetLib::StopNet();
        QOSCompare::Cleanup();
    }

    return efd::AsyncResult_Complete;
}


//------------------------------------------------------------------------------------------------
efd::AsyncResult NetService::OnTick()
{
    if (!m_pChannelManagerService)
    {
        m_spNetLib->Tick();
    }

    switch (m_connectionStatus)
    {
    case kCS_WaitConnect:
        m_connectionStatus = kCS_Connecting;

        EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
            ("Auto-connecting to ChannelManager"));
        m_connectionStatus = kCS_Connecting;
        ConnectToDefaultChannelManager();
        break;
    case kCS_Failed:
        if (m_autoReconnectToChannelManager)
        {
            efd::TimeType curTime = m_pServiceManager->GetServiceManagerTime();
            if (m_reconnectTime < curTime)
            {
                m_connectionStatus = kCS_Connecting;

                EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
                    ("Connection to ChannelManager failed, retrying."));

                if (m_channelManagerIP.empty())
                {
                    ConnectToDefaultChannelManager();
                }
                else
                {
                    ConnectToChannelManager(
                        QOS_RELIABLE,
                        m_channelManagerIP,
                        m_channelManagerListenPort);
                }
            }
        }
        break;

    case kCS_ReliableConnected:
        {
            EE_ASSERT(GetNetID() != kNetID_Unassigned);
            EE_ASSERT(GetNetID() != kNetID_Client);
            EE_ASSERT(GetChannelManagerConnectionID(QOS_RELIABLE) != kCID_INVALID);
            bool allConnected = true;
            EE_ASSERT(!m_ConfigList.empty());
            for (ChannelManagerConfigList::iterator it = m_ConfigList.begin();
                it != m_ConfigList.end();
                ++it)
            {
                ChannelManagerConfigPtr spConfig = *it;
                // Connections wait until reliable connection is established
                switch (spConfig->m_connectionStatus)
                {
                case kCS_Connected:
                    break;

                case kCS_AnnounceNetID:
                    {
                        if (GetNetID() != kNetID_Unassigned &&
                            !m_pChannelManagerService)
                        {
                            m_spNetLib->SendNetIDAnnouncement(
                                spConfig->m_connectionID,
                                GetChannelManagerConnectionID(spConfig->m_qualityOfService));
                            QualityOfService physicalQOS =
                                QOSCompare::LookupPhysical(spConfig->m_qualityOfService);
                            if (!(physicalQOS & NET_RELIABLE))
                            {
                                spConfig->m_lastAttempt = 0;
                                // wait some number of frames before next announcement
                                //DT32277 make min and max frame delay
                                // between NetID announcements config param
                                spConfig->m_nextAttempt = efd::Rand() % 15;
                            }
                        }
                        spConfig->m_connectionStatus = kCS_WaitNetIDAck;
                    }
                    break;

                case kCS_WaitNetIDAck:
                    {
                        QualityOfService physicalQOS =
                            QOSCompare::LookupPhysical(spConfig->m_qualityOfService);
                        if (!(physicalQOS & NET_RELIABLE))
                        {
                            // check to see if we should send another announcement yet
                            if (spConfig->m_lastAttempt >= spConfig->m_nextAttempt)
                            {
                                spConfig->m_connectionStatus = kCS_AnnounceNetID;
                            }
                            else
                            {
                                ++(spConfig->m_lastAttempt);
                            }
                        }
                        allConnected = false;
                    }
                    break;

                case kCS_Connecting:
                    {
                        allConnected = false;
                    }
                    break;

                default:
                    {
                        allConnected = false;
                        spConfig->m_connectionStatus = kCS_Connecting;
                        spConfig->m_connectionID = ConnectToChannelManager(
                            spConfig->m_qualityOfService,
                            spConfig->m_hostname,
                            spConfig->m_port);
                    }
                    break;
                }
            }
            if (allConnected)
            {
                EE_LOG(efd::kNetMessage, efd::ILogger::kLVL0,
                    ("%s> All Connections to ChannelManager complete.",__FUNCTION__));
                m_connectionStatus = kCS_Connected;
                m_spMessageService->SetSendRemote(true);
                UInt32 cmNetID = GetChannelManagerNetID();
                AssignNetIDMessagePtr spResponseMessage =
                    EE_NEW AssignNetIDMessage(
                    GetChannelManagerConnectionID(QOS_RELIABLE),
                    cmNetID,
                    GetNetID());
                Category netServiceCat =
                    m_spMessageService->GetServicePublicCategory(INetService::CLASS_ID);
                m_spMessageService->SendLocal(spResponseMessage, netServiceCat);

                for (ChannelManagerConfigList::iterator it = m_ConfigList.begin();
                    it != m_ConfigList.end();
                    ++it)
                {
                    ChannelManagerConfigPtr spConfig = *it;
                    QualityOfService connectedQos = spConfig->m_qualityOfService;
                    QualityOfService physicalQOS = QOSCompare::LookupPhysical(connectedQos);
                    ConnectionID cid = GetChannelManagerConnectionID(physicalQOS);
                    EE_ASSERT(cid != kCID_INVALID);
                    SendConnectionUpdate<kMSGID_NetServiceConnectionEstablished>(
                        m_spMessageService,
                        cid,
                        cmNetID);
                }
            }
        }
        break;

    case kCS_Canceled:
    case kCS_Connected:
    case kCS_Connecting:
    default:
        break;
    }

    return efd::AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
void NetService::CheckPendingSubscriptions(Category catIDToCheck, QualityOfService qos)
{
    QualityOfService existingQOS =
        m_spNetLib->GetTransport(catIDToCheck);
    if (existingQOS == QOS_INVALID)
    {
        m_spNetLib->MapCategoryToQualityOfService(
            catIDToCheck,
            qos);
    }
    CheckPendingSubscriptions(catIDToCheck);
}

//------------------------------------------------------------------------------------------------
void NetService::CheckPendingSubscriptions()
{
    // This function is dreadfully inefficient, but should only be called
    // when new connections are established
    efd::list< Category > categoryList;
    CategoryToPendingSubscriptionListMap::iterator it = m_pendingSubscriptions.begin();
    for (; it != m_pendingSubscriptions.end();++it)
    {
        categoryList.push_back(it->first);
    }

    efd::list< Category >::iterator listIt = categoryList.begin();
    for (; listIt != categoryList.end(); ++listIt)
    {
        CheckPendingSubscriptions(*listIt);
    }
}

//------------------------------------------------------------------------------------------------
void NetService::CheckPendingSubscriptions(Category catIDToCheck)
{
    CategoryToPendingSubscriptionListMap::iterator it = m_pendingSubscriptions.find(catIDToCheck);
    if (it == m_pendingSubscriptions.end())
    {
        // this Category has no pending subscriptions
        return;
    }

    QualityOfService qualityOfService = m_spNetLib->GetTransport(catIDToCheck);
    if (qualityOfService == QOS_INVALID)
    {
        // no QualityOfService is yet registered for this Category
        return;
    }

    // found the list and we have a QualityOfService
    PendingSubscriptionList& pendingSubscriptionList = it->second;
    for (PendingSubscriptionList::iterator listIt = pendingSubscriptionList.begin();
        listIt != pendingSubscriptionList.end();
        /*++listIt*/)
    {
        bool subscriptionComplete = false;
        // Someone must be producing this category before we can actually map the subscription
        ConnectionID cidToSubscribe;
        NetIDQosPair netIDQosPair(
            (*listIt).m_netID,
            qualityOfService);
        if (m_NetIDQosLookupMap.find(netIDQosPair, cidToSubscribe))
        {
            if (listIt->m_subscriptionType == kMSGID_SubscribeExternal)
            {
                m_spNetLib->AddRemoteConsumer(catIDToCheck, cidToSubscribe);
            }
            else
            {
                m_spNetLib->RemoveRemoteConsumer(catIDToCheck, cidToSubscribe);
            }
            subscriptionComplete = true;
        }
        PendingSubscriptionList::iterator eraseIt = listIt;
        ++listIt;
        if (subscriptionComplete)
        {
            pendingSubscriptionList.erase(eraseIt);
        }
    }
    if (pendingSubscriptionList.empty())
    {
        m_pendingSubscriptions.erase(catIDToCheck);
    }
}

//------------------------------------------------------------------------------------------------
void NetService::ConnectToDefaultChannelManager()
{
    if (m_ConfigList.empty())
    {
        IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
        ChannelManagerService::ReadConfig(pConfigManager, m_ConfigList);
        EE_ASSERT_MESSAGE(!m_ConfigList.empty(),
            ("%s> Failed to read ChannelManager config information!",
            __FUNCTION__));
    }

    for (ChannelManagerConfigList::iterator it = m_ConfigList.begin();
        it != m_ConfigList.end();
        ++it)
    {
        ChannelManagerConfigPtr spConfig = *it;
        //start reliable connection right away.  Other connections wait until reliable connection
        // is established
        if ((QOSCompare(QOS_RELIABLE) == spConfig->m_qualityOfService)
            && (spConfig->m_connectionStatus != kCS_Connected))
        {
            ConnectToChannelManager(
                spConfig->m_qualityOfService,
                spConfig->m_hostname,
                spConfig->m_port);
        }
    }
}


//------------------------------------------------------------------------------------------------
efd::ConnectionID NetService::ConnectToChannelManager(
    QualityOfService virtualQOS,
    const efd::utf8string& cmIPAddress,
    efd::UInt16 cmPort)
{
    QualityOfService qualityOfService = QOSCompare::LookupPhysical(virtualQOS);
    if (m_pChannelManagerService)
    {
        NetMessagePtr spNetMessage =
            EE_NEW MessageWrapper<NetMessage, kMSGID_ConnectionConnectedMsg>;
        ConnectionID cid = ConnectionID(qualityOfService,0);
        spNetMessage->SetSenderConnection(cid);
        HandleNetMessage(spNetMessage, cid);
        if (QOSCompare(virtualQOS) == QOS_RELIABLE)
        {
            // we need to trigger the net id assignment
            AssignNetIDMessagePtr spAssignNetIDMessage =
                EE_NEW AssignNetIDMessage(cid, m_spNetLib->GetNetID(), m_spNetLib->GetNetID());
            EnvelopeMessagePtr spEnvelopeMessage =
                EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_ConnectionDataReceivedMsg>;
            EE_ASSERT(spEnvelopeMessage);
            spEnvelopeMessage->SetChild(spAssignNetIDMessage);
            spEnvelopeMessage->SetDestinationCategory(kCAT_NetSystem);
            spEnvelopeMessage->SetSenderNetID(m_spNetLib->GetNetID());
            spEnvelopeMessage->SetSenderConnection(cid);

            HandleNetMessage(spEnvelopeMessage, cid);
            IMessage* pResubscribeCompleteMessage =
                EE_NEW MessageWrapper<IMessage, kMSGID_ResubscribeComplete>;
            spEnvelopeMessage->SetChild(pResubscribeCompleteMessage);
            HandleNetMessage(spEnvelopeMessage, cid);
        }

        return cid;
    }

    DisconnectFromChannelManager(qualityOfService);

    if (QOSCompare(QOS_RELIABLE) == qualityOfService)
    {
        EE_ASSERT(cmIPAddress.empty() == false);

        m_channelManagerIP = cmIPAddress;
        m_channelManagerListenPort = cmPort;
        EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
            ("%s> m_connectionStatus = kCS_Connecting",__FUNCTION__));
        m_connectionStatus = kCS_Connecting;
    }
    else
    {
        // we can't even attempt any qos connection until the reliable connection is established
        EE_ASSERT(m_channelManagerNetID != kNetID_Unassigned);
    }

    if (m_spNetLib)
    {
        return m_spNetLib->Connect(cmIPAddress, cmPort, qualityOfService, this);
    }
    else
    {
        if (QOSCompare(QOS_RELIABLE) == qualityOfService)
        {
            EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
                ("%s> No INetLib m_connectionStatus = kCS_Failed",__FUNCTION__));
            m_connectionStatus = kCS_Failed;
        }
        return kCID_INVALID;
    }
}

//------------------------------------------------------------------------------------------------
void NetService::DisconnectFromChannelManager()
{
    for (ChannelManagerConfigList::iterator it = m_ConfigList.begin();
        it != m_ConfigList.end();
        ++it)
    {
        ChannelManagerConfigPtr spConfig = *it;
        EE_ASSERT(spConfig);
        DisconnectFromChannelManager(spConfig->m_qualityOfService);
    }
}

//------------------------------------------------------------------------------------------------
void NetService::DisconnectFromChannelManager(QualityOfService virtualQOS)
{
    QualityOfService qualityOfService = QOSCompare::LookupPhysical(virtualQOS);
    if (QOSCompare(QOS_RELIABLE) == qualityOfService)
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
            ("%s> m_connectionStatus = kCS_Canceled",__FUNCTION__));
        m_connectionStatus = kCS_Canceled;
    }

    ConnectionID cmConnectionID = GetChannelManagerConnectionID(qualityOfService);
    if (cmConnectionID != kCID_INVALID)
    {
        // iterate through netid/qos to cid map so we can remove stale connections
        NetIDQosPairToConnectionIDMap::iterator it = m_NetIDQosLookupMap.begin();
        while (it != m_NetIDQosLookupMap.end())
        {
            if ((*it).second == cmConnectionID)
            {
                NetIDQosPairToConnectionIDMap::iterator eraseIt = it++;
                m_NetIDQosLookupMap.erase(eraseIt);
            }
            else
            {
                ++it;
            }
        }
        if (!m_pChannelManagerService)
        {
            m_spNetLib->CloseConnection(cmConnectionID);
        }
        else
        {
            NetMessagePtr spCloseMessage =
                EE_NEW MessageWrapper< NetMessage, kMSGID_ConnectionDisconnectedMsg>;
            spCloseMessage->SetSenderConnection(cmConnectionID);
            HandleNetMessage(spCloseMessage, cmConnectionID);
        }
        EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
            ("%s> Set ChannelManager connection ID to %s because %s closed.",
            __FUNCTION__, kCID_INVALID.ToString().c_str(),
            cmConnectionID.ToString().c_str()));
        ConnectionID emptyConnectionID(qualityOfService, 0ULL);
        SetChannelManagerConnectionID(emptyConnectionID);
        if (QOSCompare(QOS_RELIABLE) == qualityOfService)
            SetNetID(kNetID_Unassigned);
    }
}

//------------------------------------------------------------------------------------------------
void NetService::AutoReconnectToChannelManager(bool autoReconnect)
{
    m_autoReconnectToChannelManager = autoReconnect;
}

//------------------------------------------------------------------------------------------------
void NetService::AutoConnectToChannelManager(bool autoConnect)
{
    if (autoConnect && m_connectionStatus == kCS_Canceled)
    {
        m_connectionStatus = kCS_WaitConnect;
    }
    else if (!autoConnect && m_connectionStatus == kCS_WaitConnect)
    {
        m_connectionStatus = kCS_Canceled;
    }
}

//------------------------------------------------------------------------------------------------
void NetService::AssignClientNetID(bool assignClientNetID)
{
    m_assignClientNetID = assignClientNetID;
}

//------------------------------------------------------------------------------------------------
void NetService::HandleSystemMessage(EnvelopeMessage* pEnvelopeMessage)
{
    // tick off another system message occurred
    EE_LOG_METRIC_COUNT(kNetwork, "MESSAGE.RECEIVE.SYSTEM_MESSAGE");

    const IMessage* pLocalMessage = pEnvelopeMessage->GetContents(m_spMessageService);
    // if you hit this assert it means that the factory function for the message type was not
    // registerd
    EE_ASSERT(pLocalMessage);
    if (!pLocalMessage)
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("Invalid system message (type 0x%08X) received",
            pEnvelopeMessage->GetContentsClassID()));
        return;
    }

    // switch based on the message type and process the messages
    ConnectionID senderConnectionID = pEnvelopeMessage->GetSenderConnection();
    EE_ASSERT(senderConnectionID.m_qos != QOS_INVALID);
    efd::ClassID msgClassID = pLocalMessage->GetClassID();
    switch (msgClassID)
    {
    case kMSGID_SubscribeExternal:
    case kMSGID_UnsubscribeExternal:
        {
            // we only accept subscriptions over the reliable QOS
            if (!(QOSCompare(QOS_RELIABLE) == senderConnectionID.GetQualityOfService()))
                break;

            const IMessage* pMessage = pEnvelopeMessage->GetContents(m_spMessageService);
            EE_ASSERT(pMessage);
            const StreamMessage* pSystemMessage = EE_DYNAMIC_CAST(const StreamMessage, pMessage);
            EE_ASSERT(pSystemMessage);
            pSystemMessage->ResetForUnpacking();

            UInt8 numCats = 0;
            *pSystemMessage >> numCats;
            Category catID;
            UInt32 netID = pEnvelopeMessage->GetSenderNetID();

            for (UInt8 curCat = 0; curCat < numCats; curCat++)
            {
                *pSystemMessage >> catID;

                EE_LOG(efd::kChannelManager, efd::ILogger::kLVL2,
                    ("Received %s for cat %s, from %s NetID=%d",
                    pMessage->GetDescription().c_str(), catID.ToString().c_str(),
                    senderConnectionID.ToString().c_str(), netID));

                PendingSubscriptionList& pendingSubscriptionList = m_pendingSubscriptions[catID];
                pendingSubscriptionList.push_back(
                    PendingSubscription(netID,pMessage->GetClassID()));
                CheckPendingSubscriptions(catID);
            }
        }
        break;

    case kMSGID_Resubscribe:
        EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
            ("NetService Received Resubscribe message from %s",
            senderConnectionID.ToString().c_str()));
        Resubscribe(senderConnectionID);
        break;

    case kMSGID_ResubscribeComplete:
        EE_ASSERT(QOSCompare(QOS_RELIABLE) == senderConnectionID.GetQualityOfService());
        m_connectionStatus = kCS_ReliableConnected;

        EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
            ("NetService Received ResubscribeComplete message from %s, "
            "setting m_connectionStatus = kCS_ReliableConnected.",
            senderConnectionID.ToString().c_str()));
        Subscribe(kCAT_SendToProducer, m_spMessageService);
        break;

    case kMSGID_AnnounceNetID:
        {
            EE_ASSERT(QOSCompare(QOS_RELIABLE) == senderConnectionID.GetQualityOfService());
            const AnnounceNetIDMessage* pNetIdAnnounceMessage =
                EE_DYNAMIC_CAST(AnnounceNetIDMessage, pLocalMessage);
            EE_ASSERT(pNetIdAnnounceMessage);
            UInt32 senderNetID = pEnvelopeMessage->GetSenderNetID();
            ConnectionID newConnectionID = pNetIdAnnounceMessage->GetAnnouncedConnectionID();
            QualityOfService newQulaityOfService = newConnectionID.GetQualityOfService();
            ConnectionID remoteConnectionID(
                newConnectionID.GetQualityOfService(),
                senderConnectionID.m_ipInfo.ip,
                newConnectionID.GetRemotePort(), // swap remote and local ports
                newConnectionID.GetLocalPort());
            // make sure we have a matching connection
            //DT32278 Make sure remoteConnectionID exists before using.

            EE_ASSERT(newConnectionID == senderConnectionID ||
                (QOSCompare(QOS_RELIABLE) != newQulaityOfService));
            EE_ASSERT(senderConnectionID == GetChannelManagerConnectionID(QOS_RELIABLE)
                || QOSCompare(QOS_RELIABLE) == newQulaityOfService);

            NetIDQosPair netIDQos(senderNetID, newQulaityOfService);
            m_NetIDQosLookupMap[netIDQos]=remoteConnectionID;
            m_channelManagerNetID = senderNetID;
            CheckPendingSubscriptions();
        }
        break;

    case kMSGID_AckNetID:
        {
            const AnnounceNetIDMessage* pNetIdAnnounceMessage =
                EE_DYNAMIC_CAST(AnnounceNetIDMessage, pLocalMessage);
            EE_ASSERT(pNetIdAnnounceMessage);
            ConnectionID announcedCID = pNetIdAnnounceMessage->GetAnnouncedConnectionID();
            QualityOfService connectionQOS = announcedCID.GetQualityOfService();
            SetConnectionStatus(connectionQOS, kCS_Connected);
        }
        break;

    case kMSGID_SetNetID:
        {
            // ignore SetNetID message except from default QOS
            EE_ASSERT(QOSCompare(QOS_RELIABLE) == senderConnectionID.GetQualityOfService());

            if (senderConnectionID != GetChannelManagerConnectionID(
                senderConnectionID.GetQualityOfService()))
            {
                // only the ChannelManager we connected to can set our NetID
                EE_LOG(efd::kNetMessage, efd::ILogger::kERR0,
                    ("Received SetNetID message from invalid source %s, expected %s",
                    senderConnectionID.ToString().c_str(),
                    GetChannelManagerConnectionID(
                        senderConnectionID.GetQualityOfService()).ToString().c_str()));
                break;
            }

            EE_ASSERT(m_spMessageService);
            const AssignNetIDMessage* pAssignNetIDMessage =
                EE_DYNAMIC_CAST(AssignNetIDMessage, pLocalMessage);
            EE_ASSERT(pAssignNetIDMessage);
            m_channelManagerNetID = pEnvelopeMessage->GetSenderNetID();
            EE_ASSERT(pAssignNetIDMessage->GetAssignedNetID() != kNetID_Unassigned);
            EE_ASSERT(pAssignNetIDMessage->GetAssignedNetID() != kNetID_Client);
            SetNetID(pAssignNetIDMessage->GetAssignedNetID());
            EE_LOG(efd::kNetMessage,
                efd::ILogger::kLVL1,
                ("%s> Received NetID.",
                __FUNCTION__));

            NetIDQosPair netIDQos(m_channelManagerNetID,senderConnectionID.GetQualityOfService());
            m_NetIDQosLookupMap[netIDQos]=senderConnectionID;
            // Since we are the reliable QOS, now send our subscription information
            Resubscribe(senderConnectionID);

            EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
                ("Net ID configured from SetNetID message as %d.", GetNetID()));
        }
        break;

    case  kMSGID_BeginCategoryProduction:
    case  kMSGID_EndCategoryProduction:
        {
            const IMessage* pMessage = pEnvelopeMessage->GetContents(m_spMessageService);
            const StreamMessage* pSystemMessage = EE_DYNAMIC_CAST(const StreamMessage, pMessage);
            if (pSystemMessage)
            {
                pSystemMessage->ResetForUnpacking();

                Category catID = 0;
                efd::UInt32 qualityOfService;
                *pSystemMessage >> catID;
                *pSystemMessage >> qualityOfService;
                if (pMessage->GetClassID() == kMSGID_BeginCategoryProduction)
                {
                    m_spNetLib->ProducerAssociate(senderConnectionID, catID);
                    CheckPendingSubscriptions(catID, qualityOfService);
                }
                else
                {
                    m_spNetLib->ProducerDeassociate(senderConnectionID, catID);
                }
            }
        }
        break;

    case kMSGID_ChannelManagerClientJoin:
    case kMSGID_ChannelManagerClientLeave:
        {
            const IMessage* pMessage = pEnvelopeMessage->GetContents(m_spMessageService);
            Category netLocalCategory = MessageService::GetServicePublicCategory(CLASS_ID);
            m_spMessageService->SendLocal(pMessage, netLocalCategory);
        }
        break;

    default:
        EE_FAIL("Invaild system message type!");
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("Received UNKNOWN system message type: %u (0x%08x)", msgClassID, msgClassID));
        break;
    }
}


//------------------------------------------------------------------------------------------------
bool NetService::BeginCategoryProduction(
    const Category& cat,
    efd::QualityOfService virtualQOS,
    INetCallback* pCallback)
{
    EE_ASSERT(cat.IsValid());
    EE_ASSERT(virtualQOS != QOS_INVALID);

    if (!pCallback)
    {
        pCallback = this;
    }

    QualityOfService qualityOfService = QOSCompare::LookupPhysical(virtualQOS);
    CategoryCallback categoryCallback(cat,pCallback);
    CategoryRefcount::iterator it= m_producersRefcount.find(categoryCallback);
    if (it != m_producersRefcount.end())
    {
        ++((*it).second);
    }
    else
    {
        // a Category can only be associated with a single QualityOfService
        // if you hit this assert it means that somewhere else has registered the same Category
        // with a different QOS
        EE_ASSERT(m_spNetLib->GetTransport(cat) == QOS_INVALID
            || (m_spNetLib->GetTransport(cat) == qualityOfService));

        m_spNetLib->BeginCategoryProduction(cat, qualityOfService);

        // we can't send messages yet if we have not yet received our NetID
        // category interest will automatically be communicated when connection is established
        if (m_spNetLib->GetNetID() != kNetID_Unassigned)
        {
            m_spNetLib->SendBeginCategoryProduction(cat, qualityOfService);
        }
        NonAtomicRefCount refCount(1);
        m_producersRefcount[categoryCallback] = refCount;
    }
    return true;
}


//------------------------------------------------------------------------------------------------
bool NetService::EndCategoryProduction(const Category& cat, INetCallback* pCallback)
{
    EE_ASSERT(cat.IsValid());

    if (!pCallback)
    {
        pCallback = this;
    }

    CategoryCallback categoryCallback(cat,pCallback);
    CategoryRefcount::iterator it= m_producersRefcount.find(categoryCallback);
    if (it != m_producersRefcount.end())
    {
        --((*it).second);
        EE_ASSERT(m_spNetLib->GetTransport(cat) != QOS_INVALID);
    }
    else
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("%s> Tried to end %sno being produced by callback 0x%08X",
            __FUNCTION__, cat.ToString().c_str(),pCallback));
        return false;
    }

    if (((*it).second) <= 0)
    {
        if (m_spNetLib->GetNetID() != kNetID_Unassigned)
        {
            m_spNetLib->SendEndCategoryProduction(cat);
        }
        m_spNetLib->EndCategoryProduction(cat);
        m_producersRefcount.erase(it);
    }
    return true;
}


//------------------------------------------------------------------------------------------------
void NetService::Resubscribe(const ConnectionID& cid)
{
    if (!m_pChannelManagerService)
    {
        // Send the new connection the categories we are subscribed to
        m_spNetLib->SendAddLocalConsumerRequests(cid);
        m_spNetLib->SendCategoryProductionMessages(cid);
    }
    IMessage* pMessage = EE_NEW MessageWrapper<IMessage, kMSGID_ResubscribeComplete>;
    SendRemote(pMessage, kCAT_NetSystem, cid);
}


//------------------------------------------------------------------------------------------------
efd::UInt32 NetService::GetNetID() const
{
    return m_spNetLib->GetNetID();
}


//------------------------------------------------------------------------------------------------
void NetService::SetNetID(efd::UInt32 netID)
{
    if (!m_pChannelManagerService)
    {
        m_spNetLib->SetNetID(netID);
    }
}


//------------------------------------------------------------------------------------------------
efd::ConnectionID NetService::GetChannelManagerConnectionID(QualityOfService virtualQOS)
{
    QualityOfService qualityOfService = QOSCompare::LookupPhysical(virtualQOS);
    if (m_spNetLib)
    {
        return m_spNetLib->GetChannelManagerConnectionID(qualityOfService);
    }
    else
    {
        return kCID_INVALID;
    }
}


//------------------------------------------------------------------------------------------------
void NetService::SetChannelManagerConnectionID(const efd::ConnectionID& cmConnectionID)
{
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
        ("%s> Actually setting ChannelManager connection ID to %s.",
        __FUNCTION__, cmConnectionID.ToString().c_str()));
    m_spNetLib->SetChannelManagerConnectionID(cmConnectionID);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 NetService::GetNextIndexValue()
{
    // We need to ensure the new index is unique. We maintain the list in ascending
    // index order, so by adding 1 to the last index we can be confident it hasn't
    // been used.
    return m_ConfigList.empty() ? 1 : m_ConfigList.back()->m_index + 1;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 NetService::AddConfig(ChannelManagerConfigPtr spConfig)
{
    // We override whatever gets passed in as an index in order to ensure uniqueness
    spConfig->m_index = GetNextIndexValue();
    m_ConfigList.push_back(spConfig);
    return spConfig->m_index;
}

//------------------------------------------------------------------------------------------------
// Predicate for searching the config list for a particular index value
struct MatchConfigIndex
{
    bool operator()(const ChannelManagerConfigPtr& spConfig, efd::UInt32 val) const
    {
        return spConfig->m_index == val;
    }
};

//------------------------------------------------------------------------------------------------
bool NetService::RemoveConfig(efd::UInt32 index)
{
    ChannelManagerConfigList::iterator it = find_if(m_ConfigList.begin(),
        m_ConfigList.end(),
        [index](const ChannelManagerConfigPtr& spConfig)
        {
            return spConfig->m_index == index;
        });

    if (it != m_ConfigList.end())
    {
        RemoveConfig(it);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
void NetService::RemoveConfig(ChannelManagerConfigList::iterator it)
{
    ChannelManagerConfigPtr spConfig = *it;
    EE_ASSERT(spConfig);

    // If the connection ID is already invalidated, the connection may have been
    // closed by the client, and there is no need to close it again.
    if (spConfig->m_connectionID != kCID_INVALID)
    {
        m_spNetLib->CloseConnection(spConfig->m_connectionID);
    }

    m_ConfigList.remove(spConfig);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 NetService::AddChannelManagerConnection(
    efd::utf8string hostname,
    efd::UInt16 port,
    efd::QualityOfService qos)
{
    efd::UInt32 index = GetNextIndexValue();
    ChannelManagerConfigPtr spConfig = EE_NEW ChannelManagerConfig(
        index,
        hostname,
        port,
        qos);

    return AddConfig(spConfig);
}
