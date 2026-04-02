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

#include <functional>
#include <algorithm>

#include <efd/IConfigManager.h>
#include <efd/MessageService.h>
#include <efd/StreamMessage.h>
#include <efd/ConfigManager.h>
#include <efd/IDs.h>
#include <efd/ServiceManager.h>
#include <efd/EnumManager.h>

#include <efdNetwork/INetLib.h>
#include <efdNetwork/NetService.h>
#include <efdNetwork/NetCategory.h>
#include <efdNetwork/NameResolutionService.h>
#include <efdNetwork/ChannelManagerService.h>
#include <efd/QOSCompare.h>

#include <efdNetwork/NetLib.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
void ChannelManagerService::HandleNetMessage(
    const IMessage* pIncomingMessage,
    const ConnectionID& i_source)
{
    switch (pIncomingMessage->GetClassID())
    {
        case kMSGID_ConnectionAcceptedMsg:
            EE_LOG(efd::kChannelManager, ILogger::kLVL1,
                ("accepted %s", i_source.ToString().c_str()));
            AddConnection(i_source);
            break;

        case kMSGID_ConnectionConnectedMsg:
            EE_LOG(efd::kChannelManager, ILogger::kLVL1,
                ("connected %s", i_source.ToString().c_str()));
            AddConnection(i_source);
            break;

        case kMSGID_ConnectionDisconnectedMsg:
            EE_LOG(efd::kChannelManager, ILogger::kLVL1,
                ("disconnected %s", i_source.ToString().c_str()));
            RemoveConnection(i_source);
            break;

        case kMSGID_ConnectionDataReceivedMsg:
        {
            const EnvelopeMessage* pEnvelopeMessage =
                EE_DYNAMIC_CAST(EnvelopeMessage, pIncomingMessage);
            EE_ASSERT(pEnvelopeMessage->GetSenderConnection() == i_source);
            HandleMessage(const_cast<EnvelopeMessage*>(pEnvelopeMessage));
        } break;

        case kMSGID_ConnectionFailedToAcceptMsg:
            EE_LOG(efd::kChannelManager, ILogger::kLVL1,
                ("Failed To Accept %s", i_source.ToString().c_str()));
            break;
        default:
            EE_LOG(efd::kChannelManager, ILogger::kLVL1,
                ("unhandled message type 0x%08X from %s",
                pIncomingMessage->GetClassID(), i_source.ToString().c_str()));
            break;
    }
}

EE_IMPLEMENT_CONCRETE_CLASS_INFO(ChannelManagerService);

const /*static*/ char* ChannelManagerService::kConfigSection = "ChannelManager";
const /*static*/ char* ChannelManagerService::kHostname = "Hostname";
const /*static*/ char* ChannelManagerService::kPort = "Port";
const /*static*/ char* ChannelManagerService::kFailInUse = "FailInUse";
const /*static*/ char* ChannelManagerService::kQualityOfService = "QualityOfService";
const /*static*/ char* ChannelManagerService::kQOS_Reliable = "QOS_RELIABLE";
const /*static*/ char* ChannelManagerService::kQOS_Unreliable = "QOS_UNRELIABLE";
const /*static*/ char* ChannelManagerService::kQOS_Connectionless = "QOS_CONNECTIONLESS";
const /*static*/ char* ChannelManagerService::kQOS_Invalid = "QOS_INVALID";

const /*static*/ efd::UInt16 ChannelManagerService::defaultListenPort = 13215;
static const char* defaultHostname = "127.0.0.1";
static const efd::QualityOfService defaultQualityOfService = QOS_RELIABLE;

//0 is kNetID_Invalid
static const efd::UInt32 ServerStartNetID = kNetID_Unassigned+1;
static const efd::UInt32 ClientStartNetID = kNetID_Client+1;

//------------------------------------------------------------------------------------------------
efd::QualityOfService ChannelManagerService::StringToQualityOfService(
    const efd::utf8string& stringName,
    EnumManager* pEnumManager)
{
    QualityOfService foundQOS = QOSCompare::GetPhysicalQOS(pEnumManager, stringName);
    if (foundQOS == QOS_INVALID)
    {
        foundQOS = QOSCompare::GetVirtualQOS(pEnumManager, stringName);
    }
    return foundQOS;
}

//------------------------------------------------------------------------------------------------
efd::utf8string ChannelManagerService::QualityOfServiceToString(efd::QualityOfService qos)
{
    //DT32269 This function should use EnumManager (and be in QOSCompare).
    if (qos == QOS_RELIABLE)
        return kQOS_Reliable;
    if (qos == QOS_UNRELIABLE)
        return kQOS_Unreliable;
    if (qos == QOS_CONNECTIONLESS)
        return kQOS_Connectionless;
    return kQOS_Invalid;
}

//------------------------------------------------------------------------------------------------
/* static */ void ChannelManagerService::ReadConfig(
    IConfigManager* pConfigManager,
    ChannelManagerConfigList& o_result)
{
    // we need a config manager to continue
    efd::UInt32 channelManagerIndex = 0;
    const ISection* pSection = NULL;
    // At the moment we only support one connection of each QualityOfService to ChannelManager
    efd::set< efd::QualityOfService > usedQOS;
    if (pConfigManager)
    {
        do
        {
            efd::utf8string channelManagerSection;

            if (channelManagerIndex > 0)
            {
                channelManagerSection.sprintf(
                    "%s%d",
                    ChannelManagerService::kConfigSection,
                    channelManagerIndex);
            }
            else
            {
                channelManagerSection = ChannelManagerService::kConfigSection;
            }

            pSection = pConfigManager->GetConfiguration()->FindSection(
                channelManagerSection.c_str());
            if (pSection)
            {
                ChannelManagerConfigPtr spConfig = EE_NEW ChannelManagerConfig;
                // set the default values in case they are missing in the config file
                spConfig->m_port = defaultListenPort;
                spConfig->m_hostname = defaultHostname;
                spConfig->m_index = channelManagerIndex;
                spConfig->m_qualityOfService = defaultQualityOfService;

                // Find the ChannelManager parameters, overriding the default values
                efd::utf8string temp = pSection->FindValue(ChannelManagerService::kPort);
                if (!temp.empty())
                {
                    spConfig->m_port = (efd::UInt16)atoi(temp.c_str());
                }

                spConfig->m_failInUse = pSection->IsTrue(ChannelManagerService::kFailInUse);

                temp = pSection->FindValue(ChannelManagerService::kHostname);
                if (!temp.empty())
                {
                    spConfig->m_hostname = temp;
                }

                utf8string qualityOfService
                    = pSection->FindValue(ChannelManagerService::kQualityOfService);
                if (!qualityOfService.empty())
                {
                    ServiceManager* pServiceManager = pConfigManager->GetServiceManager();
                    EnumManager* pEnumManager = NULL;
                    if (pServiceManager)
                    {
                        pEnumManager = pServiceManager->GetSystemServiceAs<EnumManager>();
                    }
                    spConfig->m_qualityOfService =
                        StringToQualityOfService(qualityOfService, pEnumManager);
                    if (spConfig->m_qualityOfService == QOS_INVALID)
                    {
                        EE_FAIL_MESSAGE(("%s> '%s' is not a valid QulaityOfService!!!!",
                            __FUNCTION__, qualityOfService.c_str()));
                        continue;
                    }
                }
                if (usedQOS.find(spConfig->m_qualityOfService) == usedQOS.end())
                {
                    o_result.push_back(spConfig);
                    usedQOS.insert(spConfig->m_qualityOfService);
                }
                else
                {
                    EE_LOG(efd::kChannelManager, ILogger::kLVL0, ("%s> Duplicate QualityOfService"
                        " entry ChannelManager%d: %s:%d %s",
                        __FUNCTION__,
                        spConfig->m_index,
                        spConfig->m_hostname.c_str(),
                        spConfig->m_port,
                        qualityOfService.c_str()));
                }
            }
        // NOTE: The following while loop is a bit tricky.  We must always increment
        // channelManagerIndex when evaluating this loop condition or we can enter an infinite loop
        // in certain error cases, so we place that term first to ensure it always happens.
        //DT32315 Remove support for config parameter ChannelManager without a number.
        } while (++channelManagerIndex == 1 || pSection);
    }

    // If there were no channel manager configurations in the config file, create
    // one with reasonable default parameters.
    if (o_result.empty())
    {
        ChannelManagerConfigPtr spConfig = EE_NEW ChannelManagerConfig(
            0, // index
            defaultHostname,
            defaultListenPort,
            defaultQualityOfService);

        o_result.push_back(spConfig);
    }
}

//------------------------------------------------------------------------------------------------
ChannelManagerService::ChannelManagerService(const efd::utf8string& netLibType)
    : m_spNetLib(NULL)
    , m_needListen(0)
    , m_curNetID(ServerStartNetID)
    , m_curClientNetID(ClientStartNetID)
    , m_netLibType(netLibType)
    , m_readNetLibConfig(true)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 5800;

    INetLib::RegisterNetLib<NetLib>(NetService::kNetLib);
    // if netLibType is passed in we force creation of the type instead of reading the config value
    if (!m_netLibType.empty())
    {
        m_readNetLibConfig = false;
    }
    else
    {
        m_netLibType = NetService::kNetLib;
    }
}

//------------------------------------------------------------------------------------------------
ChannelManagerService::ChannelManagerService(INetLib* pNetLib)
    : m_spNetLib(pNetLib)
    , m_needListen(0)
    , m_curNetID(ServerStartNetID)
    , m_curClientNetID(ClientStartNetID)
    , m_netLibType(NetService::kNetLib)
    , m_readNetLibConfig(true)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 5800;

    INetLib::RegisterNetLib<NetLib>(NetService::kNetLib);
    m_readNetLibConfig = false;
}


//------------------------------------------------------------------------------------------------
ChannelManagerService::~ChannelManagerService()
{
    m_spNetLib = NULL;
}

//------------------------------------------------------------------------------------------------
const char* ChannelManagerService::GetDisplayName() const
{
    return "ChannelManager";
}

//------------------------------------------------------------------------------------------------
SyncResult ChannelManagerService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    EE_UNUSED_ARG(pDependencyRegistrar);

    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    EE_ASSERT(m_spMessageService);
    if (pConfigManager)
    {
        const ISection *pSection = pConfigManager->GetConfiguration()->FindSection(
            NetService::kConfigSection);

        if (pSection)
        {
            if (m_readNetLibConfig)
            {
                efd::utf8string netLibType = pSection->FindValue(NetService::kNetLibType);
                if (!netLibType.empty())
                {
                    m_netLibType = netLibType;
                }
            }
        }
    }
    if (!m_spNetLib)
    {
        const ISection* pConfigSection = pConfigManager->GetConfiguration();
        m_spNetLib = INetLib::CreateNetLib(m_netLibType, m_spMessageService, pConfigSection);
    }

    if (!m_spNetLib)
    {
        EE_LOG(efd::kChannelManager, efd::ILogger::kERR0,
            ("Failed to create a NetLib of type '%s', ensure INetLib::RegisterNetLib was called",
            m_netLibType.c_str()));
        return efd::SyncResult_Failure;
    }

    // initialize INetLib
    if (INetLib::StartNet() <= 0)
    {
        return efd::SyncResult_Failure;
    }

    // set the NetID for the INetLib ChannelManager owns
    m_spNetLib->SetNetID(AssignNetID());

    RegisterMessageWrapperFactory< NetMessage, kMSGID_Resubscribe >(m_spMessageService);
    RegisterMessageWrapperFactory< IMessage, kMSGID_ResubscribeComplete >(m_spMessageService);
    RegisterMessageFactory<RequestNetIDMessage>(m_spMessageService);
    RegisterMessageFactory<AnnounceNetIDMessage>(m_spMessageService);
    // used by ConnectionTCP to send version info.  Cannot be registered by ConnectionTCP until
    // MessageFactory refactor is complete
    RegisterMessageWrapperFactory< StreamMessage, kMSGID_UnreliableVersion >(m_spMessageService);

    RegisterMessageWrapperFactory< StreamMessage, kMSGID_SubscribeExternal >(m_spMessageService);
    RegisterMessageWrapperFactory< StreamMessage, kMSGID_UnsubscribeExternal >(m_spMessageService);
    RegisterMessageWrapperFactory< StreamMessage, kMSGID_BeginCategoryProduction >(
        m_spMessageService);
    RegisterMessageWrapperFactory< StreamMessage, kMSGID_EndCategoryProduction >(
        m_spMessageService);
    RegisterMessageWrapperFactory< EnvelopeMessage, kMSGID_SendToProducer >(m_spMessageService);

    // read in QOS mapping from config file
    EnumManager* pEnumManager = m_pServiceManager->GetSystemServiceAs<EnumManager>();
    QOSCompare::ReadConfig(pConfigManager, pEnumManager);

    // read ChannelManager config data
    if (m_ConfigList.empty())
    {
        ChannelManagerService::ReadConfig(pConfigManager, m_ConfigList);
    }
    m_needListen = static_cast<efd::UInt8>(m_ConfigList.size());
    EE_ASSERT_MESSAGE(!m_ConfigList.empty(), ("Failed to read ChannelManager config information!"));

    return efd::SyncResult_Success;
}


//------------------------------------------------------------------------------------------------
AsyncResult ChannelManagerService::OnInit()
{
    if (!INetLib::IsNetReady())
    {
        return efd::AsyncResult_Pending;
    }

    // we want to receive system messages
    m_spNetLib->AddLocalConsumer(kCAT_NetSystem, this);
    m_spNetLib->BeginCategoryProduction(kCAT_NetSystem, QOS_RELIABLE);

    EE_LOG(efd::kChannelManager, ILogger::kLVL0, ("%s> Complete. using NetID=%d",
        __FUNCTION__, m_spNetLib->GetNetID()));
    return efd::AsyncResult_Complete;
}


//------------------------------------------------------------------------------------------------
AsyncResult ChannelManagerService::OnTick()
{
    m_spNetLib->Tick();

    // If we have any configs that are not yet listening, set them up. We track this
    // with a separate var so we don't have to do a find_if on the list every tick.
    if (m_needListen > 0)
    {
        // Run through each configuration one time; for any of them that are still
        // invalid, try to listen.
        for (ChannelManagerConfigList::iterator configIt = m_ConfigList.begin();
            configIt != m_ConfigList.end();
            ++configIt)
        {
            ChannelManagerConfigPtr spConfig = *configIt;

            if (spConfig->m_connectionID != kCID_INVALID)
            {
                continue;
            }

            spConfig->m_connectionID = m_spNetLib->Listen(
                spConfig->m_port,
                spConfig->m_qualityOfService,
                this);

            if (spConfig->m_connectionID == kCID_INVALID)
            {
                EE_LOG(efd::kChannelManager, efd::ILogger::kLVL1,
                    ("%s> Listening for QOS %d connections on port %d failed",
                    __FUNCTION__,
                    spConfig->m_qualityOfService, spConfig->m_port));
                StreamMessagePtr spStreamMessage =
                    EE_NEW MessageWrapper<StreamMessage, kMSGID_ChannelManagerPortInUse>;
                (*spStreamMessage) << spConfig->m_port;
                (*spStreamMessage) << spConfig->m_qualityOfService;

                Category channelManagerCat =
                    m_spMessageService->GetServicePublicCategory(ChannelManagerService::CLASS_ID);
                m_spMessageService->SendImmediate(spStreamMessage, channelManagerCat);
                if (spConfig->m_failInUse)
                {
                    return efd::AsyncResult_Failure;
                }
            }
            else
            {
                --m_needListen;
                EE_LOG(efd::kChannelManager, efd::ILogger::kLVL2,
                    ("%s> Listening for QOS %d connections on port %d",
                    __FUNCTION__,
                    spConfig->m_qualityOfService,
                    spConfig->m_port));

            }
        }
    }

    return efd::AsyncResult_Pending;
}


//------------------------------------------------------------------------------------------------
AsyncResult ChannelManagerService::OnShutdown()
{
    m_spNetLib->Shutdown();
    INetLib::StopNet();
    QOSCompare::Cleanup();

    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void ChannelManagerService::CheckPendingSubscriptions(Category catIDToCheck, QualityOfService qos)
{
    QualityOfService existingQOS = m_spNetLib->GetTransport(catIDToCheck);
    if (existingQOS == QOS_INVALID)
    {
        m_spNetLib->MapCategoryToQualityOfService(catIDToCheck, qos);
    }
    CheckPendingSubscriptions(catIDToCheck);
}

//------------------------------------------------------------------------------------------------
void ChannelManagerService::CheckPendingSubscriptions()
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
void ChannelManagerService::CheckPendingSubscriptions(Category catIDToCheck)
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
void ChannelManagerService::HandleMessage(const efd::EnvelopeMessage* pEnvelopeMessage)
{
    efd::ClassID msgClassID = pEnvelopeMessage->GetContentsClassID();
    ConnectionID senderConnectionID = pEnvelopeMessage->GetSenderConnection();

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

                // if a connection to the NetID subscribing has not yet come up,
                // queue the subscription request to be processed after an appropriate
                // connection has been established
                if (pMessage->GetClassID() == kMSGID_SubscribeExternal)
                {
                    m_spNetLib->AddLocalConsumer(catID, this);
                    m_spNetLib->SendAddLocalConsumerRequest(catID);
                }
                else
                {
                    m_spNetLib->RemoveLocalConsumer(catID, this);
                    m_spNetLib->SendRemoveLocalConsumerRequest(catID);
                }

                PendingSubscriptionList& pendingSubscriptionList = m_pendingSubscriptions[catID];
                pendingSubscriptionList.push_back(
                    PendingSubscription(netID,pMessage->GetClassID()));
                CheckPendingSubscriptions(catID);
            }
        }
        return;

    case kMSGID_Resubscribe:
        {
            EE_LOG(efd::kChannelManager, efd::ILogger::kLVL2,
                ("Received Resubscribe request from %s", senderConnectionID.ToString().c_str()));
            m_spNetLib->SendAddLocalConsumerRequests(senderConnectionID);
            m_spNetLib->SendCategoryProductionMessages(senderConnectionID);
            IMessage* pResubscribeCompleteMessage =
                EE_NEW MessageWrapper<IMessage, kMSGID_ResubscribeComplete>;
            SendRemote(
                pResubscribeCompleteMessage,
                kCAT_NetSystem,
                senderConnectionID);
        }
        return;

    case kMSGID_ResubscribeComplete:
        {
            EE_LOG(efd::kChannelManager, efd::ILogger::kLVL1,
                ("CM Received ResubscribeComplete message from %s",
                senderConnectionID.ToString().c_str()));
            EE_ASSERT(m_spMessageService);
            NetService::SendConnectionUpdate<kMSGID_ChannelManagerConnectionAccepted>(
                m_spMessageService, senderConnectionID,pEnvelopeMessage->GetSenderNetID());
            // let everyone know that a client has joined the channel manager
            AssignNetIDMessagePtr spNetIDMessage =
                EE_NEW MessageWrapper<AssignNetIDMessage, kMSGID_ChannelManagerClientJoin>;
            spNetIDMessage->SetAssignedNetID(pEnvelopeMessage->GetSenderNetID());
            SendRemote(spNetIDMessage, kCAT_NetSystem, QOS_RELIABLE);
        }
        break;

    case kMSGID_AnnounceNetID:
        {
            const IMessage* pLocalMessage = pEnvelopeMessage->GetContents(m_spMessageService);
            EE_ASSERT(pLocalMessage);
            const AnnounceNetIDMessage* pNetIdAnnounceMessage =
                EE_DYNAMIC_CAST(AnnounceNetIDMessage, pLocalMessage);
            EE_ASSERT(pNetIdAnnounceMessage);
            UInt32 senderNetID = pEnvelopeMessage->GetSenderNetID();
            ConnectionID remoteConnectionID = pNetIdAnnounceMessage->GetAnnouncedConnectionID();

            // Send ack that we received the NetID announcement
            AnnounceNetIDMessagePtr spNetIDAck =
                EE_NEW MessageWrapper< AnnounceNetIDMessage, kMSGID_AckNetID >;
            spNetIDAck->SetAnnouncedConnectionID(remoteConnectionID);

            // find the reliable connection to that NetID
            NetIDQosPair netIDReliableQos(senderNetID, QOSCompare::LookupPhysical(QOS_RELIABLE));
            ConnectionID reliableConnectionID;
            if (m_NetIDQosLookupMap.find(netIDReliableQos, reliableConnectionID))
            {
                // send the ack
                SendRemote(spNetIDAck,kCAT_NetSystem, reliableConnectionID);
            }
            EE_ASSERT(reliableConnectionID != kCID_INVALID);

            NetIDQosPair netIDQos(senderNetID, senderConnectionID.GetQualityOfService());
            m_NetIDQosLookupMap[netIDQos] = senderConnectionID;
            m_connectionIdToNetIDMap[senderConnectionID] = senderNetID;
            CheckPendingSubscriptions();
        }
        break;

    case kMSGID_BeginCategoryProduction:
    case kMSGID_EndCategoryProduction:
        {
            const IMessage* pMessage = pEnvelopeMessage->GetContents(m_spMessageService);
            const StreamMessage* pSystemMessage = EE_DYNAMIC_CAST(const StreamMessage, pMessage);
            EE_ASSERT(pSystemMessage);
            pSystemMessage->ResetForUnpacking();

            Category catID = 0;
            efd::UInt32 qualityOfService;
            *pSystemMessage >> catID;
            *pSystemMessage >> qualityOfService;

            if (QOS_INVALID == qualityOfService)
            {
                EE_LOG(efd::kChannelManager, efd::ILogger::kERR0,
                    ("%s> bogus qos(%d) in %s NetID=%d",
                    __FUNCTION__,
                    qualityOfService,
                    pMessage->GetDescription().c_str(),
                    pEnvelopeMessage->GetSenderNetID()));
                return;
            }

            EE_LOG(efd::kChannelManager, ILogger::kLVL2,
                ("ChannelManager Received %s for %s, from %s NetID=%d",
                pMessage->GetDescription().c_str(),
                catID.ToString().c_str(),
                senderConnectionID.ToString().c_str(),
                pEnvelopeMessage->GetSenderNetID()));

            if (pMessage->GetClassID() == kMSGID_BeginCategoryProduction)
            {
                m_spNetLib->ProducerAssociate(senderConnectionID, catID);
                CheckPendingSubscriptions(catID, qualityOfService);
                m_spNetLib->BeginCategoryProduction(catID, qualityOfService);
                m_spNetLib->SendBeginCategoryProduction(catID, qualityOfService);
            }
            else
            {
                m_spNetLib->EndCategoryProduction(catID);
                m_spNetLib->SendEndCategoryProduction(catID);
                m_spNetLib->ProducerDeassociate(senderConnectionID, catID);
            }
        }
        return;

    case kMSGID_RequestNetID:
        {
            efd::UInt32 netIDToAssign = AssignClientNetID();
            EE_LOG(efd::kChannelManager,
                ILogger::kLVL1,
                ("%s>kMSGID_RequestNetID Assigning NetID %d to %s",
                __FUNCTION__, netIDToAssign, senderConnectionID.ToString().c_str()));
            const IMessage* pMessage = pEnvelopeMessage->GetContents(m_spMessageService);
            const RequestNetIDMessage* pRequestMessage =
                EE_DYNAMIC_CAST(RequestNetIDMessage, pMessage);
            EE_ASSERT(pRequestMessage);
            AssignNetIDMessagePtr spResponseMessage =
                EE_NEW AssignNetIDMessage(
                senderConnectionID,
                m_spNetLib->GetNetID(),
                netIDToAssign);
            SendRemote(
                spResponseMessage,
                pRequestMessage->GetPrivateResponseChannel(),
                senderConnectionID);
        }
        return;

    case kMSGID_SendToProducer:
        {
            const IMessage* pMessage = pEnvelopeMessage->GetContents(m_spMessageService);
            EE_ASSERT(pMessage);
            EnvelopeMessage* pInnerEnvelopeMessage =
                EE_DYNAMIC_CAST(EnvelopeMessage, const_cast<IMessage*>(pMessage));
            EE_ASSERT(pInnerEnvelopeMessage);
            EE_LOG(efd::kChannelManager, ILogger::kLVL2,
                ("SendToProducer %s for %s, from %s NetID=%d",
                pEnvelopeMessage->GetChildDescription().c_str(),
                pInnerEnvelopeMessage->GetDestinationCategory().ToString().c_str(),
                senderConnectionID.ToString().c_str(),
                pEnvelopeMessage->GetSenderNetID()));
            CheckPendingSubscriptions(
                pInnerEnvelopeMessage->GetDestinationCategory(),
                pEnvelopeMessage->GetQualityOfService());
            m_spNetLib->ProducerForward(const_cast<EnvelopeMessage*>(pEnvelopeMessage),
                pInnerEnvelopeMessage->GetDestinationCategory());
            // if we are configured to pass messages directly to this process instead of over the
            // network, hand the message to NetService
            DeliverLocalNetMessage(pEnvelopeMessage, pEnvelopeMessage->GetSenderConnection());
        }
        return;

    default:
        {
            EE_LOG(efd::kChannelManager, ILogger::kLVL2,
                ("Forwarding message %s for %s, from %s NetID=%d",
                pEnvelopeMessage->GetChildDescription().c_str(),
                pEnvelopeMessage->GetDestinationCategory().ToString().c_str(),
                senderConnectionID.ToString().c_str(),
                pEnvelopeMessage->GetSenderNetID()));
            CheckPendingSubscriptions(
                pEnvelopeMessage->GetDestinationCategory(),
                pEnvelopeMessage->GetQualityOfService());
            m_spNetLib->Forward(
                const_cast<EnvelopeMessage*>(pEnvelopeMessage),
                pEnvelopeMessage->GetQualityOfService());
            // if we are configured to pass messages directly to this process instead of over the
            // network, hand the message to NetService
            DeliverLocalNetMessage(pEnvelopeMessage, pEnvelopeMessage->GetSenderConnection());
        }
        return;
    }
}

//------------------------------------------------------------------------------------------------
void ChannelManagerService::AddConnection(const ConnectionID& i_source)
{
    // we want to receive all messages from this connection
    m_spNetLib->RegisterEventHandler(
        kMSGID_ConnectionDataReceivedMsg,
        this,
        i_source);

    if (QOSCompare(QOS_RELIABLE) == i_source.GetQualityOfService())
    {
        // Assign NetID to process that just connected
        efd::UInt32 netIDToAssign = AssignNetID();
        NetIDQosPair netIDQos(netIDToAssign, i_source.GetQualityOfService());
        m_NetIDQosLookupMap[netIDQos] = i_source;
        m_connectionIdToNetIDMap[i_source] = netIDToAssign;

        EE_LOG(efd::kChannelManager,
            ILogger::kLVL1,
            ("%s> kMSGID_RequestNetID Assigning NetID %d to %s",
            __FUNCTION__, netIDToAssign, i_source.ToString().c_str()));
        m_spNetLib->SendAddLocalConsumerRequests(i_source);
        m_spNetLib->SendCategoryProductionMessages(i_source);
        AssignNetIDMessagePtr spMessage =
            EE_NEW AssignNetIDMessage(i_source, m_spNetLib->GetNetID(), netIDToAssign);
        SendRemote(spMessage, kCAT_NetSystem, i_source);
        IMessage* pResubscribeCompleteMessage =
            EE_NEW MessageWrapper<IMessage, kMSGID_ResubscribeComplete>;
        SendRemote(pResubscribeCompleteMessage, kCAT_NetSystem, i_source);
    }
}

//------------------------------------------------------------------------------------------------
void ChannelManagerService::SendRemote(
    IMessage* pMessageToSend,
    const Category& destCategory,
    const ConnectionID& cid)
{
    // if this message is destined for a local connection, don't send it through NetLib
    if (cid.m_id == 0)
    {
        EnvelopeMessagePtr spEnvelopeMessage =
            EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_ConnectionDataReceivedMsg>;
        EE_ASSERT(spEnvelopeMessage);
        spEnvelopeMessage->SetChild(pMessageToSend);
        spEnvelopeMessage->SetDestinationCategory(destCategory);
        spEnvelopeMessage->SetSenderNetID(kNetID_Any);
        spEnvelopeMessage->SetSenderConnection(cid);
        spEnvelopeMessage->SetQualityOfService(cid.GetQualityOfService());
        DeliverLocalNetMessage(spEnvelopeMessage, cid);
    }
    else
    {
        // send normally
        m_spNetLib->SendRemote(pMessageToSend, destCategory, cid);
    }
}

//------------------------------------------------------------------------------------------------
void ChannelManagerService::SendRemote(
    IMessage* pMessageToSend,
    const Category& destCategory,
    QualityOfService qos)
{
    // send to ourselves
    EnvelopeMessagePtr spEnvelopeMessage =
        EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_ConnectionDataReceivedMsg>;
    EE_ASSERT(spEnvelopeMessage);
    spEnvelopeMessage->SetChild(pMessageToSend);
    spEnvelopeMessage->SetDestinationCategory(destCategory);
    spEnvelopeMessage->SetSenderNetID(kNetID_Any);
    spEnvelopeMessage->SetSenderConnection(ConnectionID(qos,0));
    spEnvelopeMessage->SetQualityOfService(qos);

    DeliverLocalNetMessage(spEnvelopeMessage, ConnectionID(qos,0));
    // send normally
    m_spNetLib->SendRemote(pMessageToSend, destCategory, qos);
}

//------------------------------------------------------------------------------------------------
void ChannelManagerService::DeliverLocalNetMessage(
    const IMessage* pMessage,
    const ConnectionID& sourceCID)
{
    efd::set< NetService* >::iterator it = m_netServiceSet.begin();
    for (; it != m_netServiceSet.end(); ++it)
    {
        (*it)->HandleNetMessage(pMessage, sourceCID);
    }
}

//------------------------------------------------------------------------------------------------
void ChannelManagerService::RemoveConnection(const ConnectionID& i_source)
{
    m_spNetLib->RemoveRemoteConsumer(i_source);

    UInt32 senderNetID = kNetID_Unassigned;
    m_connectionIdToNetIDMap.find(i_source, senderNetID);
    EE_ASSERT(m_spMessageService);
    NetService::SendConnectionUpdate<kMSGID_ChannelManagerConnectionClosed>(
        m_spMessageService, i_source, senderNetID);
    m_connectionIdToNetIDMap.erase(i_source);
    AssignNetIDMessagePtr spNetIDMessage =
        EE_NEW MessageWrapper<AssignNetIDMessage, kMSGID_ChannelManagerClientLeave>;
    spNetIDMessage->SetAssignedNetID(senderNetID);
    SendRemote(spNetIDMessage, kCAT_NetSystem, QOS_RELIABLE);

}

//------------------------------------------------------------------------------------------------
void ChannelManagerService::AnnounceService(
    const efd::utf8string& name,
    const efd::utf8string& extraInfo)
{
    // Use the NameResolutionService to announce each connection that is available for this Channel
    // Manager.  The announced name is the passed in 'name' parameter + ":QOS_NAME".
    NameResolutionService* pNameService =
        m_pServiceManager->GetSystemServiceAs<NameResolutionService>();
    EE_ASSERT (pNameService);

    if (!m_ConfigList.empty())
    {
        ChannelManagerConfigList::iterator configIt = m_ConfigList.begin();
        for (; configIt != m_ConfigList.end(); ++configIt)
        {
            ChannelManagerConfigPtr spConfig = *configIt;

            utf8string announce = name + ":" +
                QualityOfServiceToString(spConfig->m_qualityOfService);
            pNameService->AnnounceService(
                ChannelManagerService::CLASS_ID,
                announce,
                extraInfo,
                spConfig->m_port);

            EE_LOG(efd::kChannelManager, efd::ILogger::kLVL1,
                ("%s> Announced '%s' on port %d",
                __FUNCTION__,
                announce.c_str(), spConfig->m_port));
        }
    }
}

//------------------------------------------------------------------------------------------------
void ChannelManagerService::CancelAnnounceService(const efd::utf8string& name)
{
    NameResolutionService* pNameService =
        m_pServiceManager->GetSystemServiceAs<NameResolutionService>();
    EE_ASSERT (pNameService);

    if (!m_ConfigList.empty())
    {
        ChannelManagerConfigList::iterator configIt = m_ConfigList.begin();
        for (; configIt != m_ConfigList.end(); ++configIt)
        {
            ChannelManagerConfigPtr spConfig = *configIt;

            utf8string announce = name + ":" +
                QualityOfServiceToString(spConfig->m_qualityOfService);
            pNameService->CancelAnnounce(
                ChannelManagerService::CLASS_ID,
                announce);

            EE_LOG(efd::kChannelManager, efd::ILogger::kLVL1,
                ("%s> Canceled announceing '%s' on port %d",
                __FUNCTION__,
                announce.c_str(), spConfig->m_port));
        }
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 ChannelManagerService::AssignNetID()
{
    return m_curNetID++;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 ChannelManagerService::AssignClientNetID()
{
    return m_curClientNetID++;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 ChannelManagerService::GetNextIndexValue()
{
    // We need to ensure the new index is unique. We maintain the list in ascending
    // index order, so by adding 1 to the last index we can be confident it hasn't
    // been used.
    return m_ConfigList.empty() ? 1 : m_ConfigList.back()->m_index + 1;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 ChannelManagerService::AddConfig(ChannelManagerConfigPtr spConfig)
{
    // We override whatever gets passed in as an index in order to ensure uniqueness
    spConfig->m_index = GetNextIndexValue();
    m_ConfigList.push_back(spConfig);
    ++m_needListen;
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
bool ChannelManagerService::RemoveConfig(efd::UInt32 index)
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
void ChannelManagerService::RemoveConfig(ChannelManagerConfigList::iterator it)
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
efd::UInt32 ChannelManagerService::AddChannelManagerConnection(
    const efd::utf8string& hostname, efd::UInt16 port, efd::QualityOfService qos)
{
    efd::UInt32 index = GetNextIndexValue();
    ChannelManagerConfigPtr spConfig = EE_NEW ChannelManagerConfig(
        index,
        hostname,
        port,
        qos);

    return AddConfig(spConfig);
}

//------------------------------------------------------------------------------------------------
INetLib* ChannelManagerService::GetNetLib()
{
    return m_spNetLib;
}

//------------------------------------------------------------------------------------------------
void ChannelManagerService::AddNetService(NetService* pNetService)
{
    m_netServiceSet.insert(pNetService);
    pNetService->SetChannelManager(this);
}

//------------------------------------------------------------------------------------------------
