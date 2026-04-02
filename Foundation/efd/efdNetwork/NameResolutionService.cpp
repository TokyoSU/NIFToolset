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

#include <efdNetwork/NameResolutionService.h>
#include <efd/UniversalID.h>
#include <efd/Category.h>
#include <efd/MessageService.h>
#include <efdNetwork/NetService.h>
#include <efdNetwork/INetLib.h>
#include <efdNetwork/NetLib.h>
#include <efdNetwork/NameResolutionMessage.h>
#include <efd/ConfigManager.h>
#include <efd/efdClassIDs.h>
#include <efd/ServiceManager.h>
#include <efd/QOSCompare.h>
#include <efd/EnumManager.h>
#include <efdNetwork/NetErrorMessage.h>

using namespace efd;

static const efd::Category kCat_NameResolutionRequest =
    Category(UniversalID::ECU_Any, kNetID_Any, kBASEID_NameResolutionRequest);
static const efd::Category kCat_NameResolutionResponse =
    Category(UniversalID::ECU_Any, kNetID_Unassigned, kBASEID_NameResolutionResponse);

/*static*/ efd::UInt16 NameResolutionService::m_responseID = 1;

static const char* kNameResolutionServiceConfig = "NameResolutionService";
static const char* kAnnouncePort = "AnnouncePort";

static const efd::UInt16 kAnnounceListenPort = 42221;

/// Timeout value for setting no timeout on lookup requests
const /*static*/ efd::TimeType NameResolutionService::kNameLookup_NoTimeout = -1.0;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(NameResolutionService);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NameResolutionMessage);

#define OWN_NETLIB

//------------------------------------------------------------------------------------------------
NameResolutionService::NameResolutionService()
    : m_announcePort(kAnnounceListenPort)
    , m_inetAddrAny("0.0.0.0")
    , m_inetAddrBroadcast("255.255.255.255")
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 2400;

#ifdef OWN_NETLIB

    m_netLibType = NetService::kNetLib;

#endif //OWN_NETLIB
}

//------------------------------------------------------------------------------------------------
NameResolutionService::~NameResolutionService()
{
}

//------------------------------------------------------------------------------------------------
const char* NameResolutionService::GetDisplayName() const
{
    return "NameResolutionService";
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::SetAnnouncePort(efd::UInt16 announcePort)
{
    m_announcePort = announcePort;
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::HandleNetMessage(
    const IMessage* pIncomingMessage,
    const ConnectionID& senderConnectionID)
{
    EE_UNUSED_ARG(senderConnectionID);
    //check to see if this is a NetMessage w/o an envelope
    switch (pIncomingMessage->GetClassID())
    {
    case kMSGID_ConnectionAcceptedMsg:
        {
            EE_LOG(efd::kTesting, efd::ILogger::kLVL1,
                ("NameResolutionService Accepted: %s",
                senderConnectionID.ToString().c_str()));
        }
        return;

    case kMSGID_ConnectionConnectedMsg:
        {
            EE_LOG(efd::kTesting, efd::ILogger::kLVL1,
                ("NameResolutionService Connected: %s",
                senderConnectionID.ToString().c_str()));
        }
        return;

    case kMSGID_ConnectionDisconnectedMsg:
        {
            m_spNetLib->RemoveRemoteConsumer(senderConnectionID);
            EE_LOG(efd::kTesting, efd::ILogger::kLVL1,
                ("NameResolutionService Disconnected: %s",
                senderConnectionID.ToString().c_str()));
            if (m_listenCID == senderConnectionID)
            {
                m_listenCID = kCID_INVALID;
            }
            else if (m_pritaveListenCID == senderConnectionID)
            {
                m_pritaveListenCID = kCID_INVALID;
            }
        }
        return;

    case kMSGID_ConnectionFailedToConnectMsg:
        {
            EE_LOG(efd::kTesting, efd::ILogger::kLVL1,
                ("NameResolutionService FailedToConnect: %s",
                senderConnectionID.ToString().c_str()));
        }
        return;

    case kMSGID_ConnectionDataReceivedMsg:
        {
            const EnvelopeMessage* pConstMessage =
                EE_DYNAMIC_CAST(const EnvelopeMessage, pIncomingMessage);
            EnvelopeMessage* pEnvelopeMessage = const_cast<EnvelopeMessage*>(pConstMessage);
            // make sure we have the proper kind of message
            EE_ASSERT(pEnvelopeMessage);
            HandleDataMessage(pEnvelopeMessage, senderConnectionID);
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
            ("Error: Received message on cat %s, but %s type is not recognized",
            senderConnectionID.ToString().c_str(),
            pIncomingMessage->GetDescription().c_str()));
        return;
    }
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::AnnounceService(
    efd::UInt32 classID,
    const efd::utf8string& name,
    const efd::utf8string& extraInfo,
    efd::UInt16 port)
{
    AnnounceService(classID, name, port, GetInternetAddressAny(), extraInfo);
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::AnnounceService(
    efd::UInt32 classID,
    const efd::utf8string& name,
    efd::UInt16 port,
    const efd::utf8string& hostname,
    const efd::utf8string& extraInfo)
{
    // Check whether we have bound to the listen port before announcing anything
    if (m_listenCID == kCID_INVALID)
    {
        m_listenCID =
            m_spNetLib->Listen(m_inetAddrAny, m_announcePort, QOS_CONNECTIONLESS, this);
        if (m_listenCID != kCID_INVALID)
        {
            //DT32274 implement request forwarding to localhost; some process
            /// on this machine has already bound to the default port and must act as a proxy for
            /// other processes on the same machine
            m_spNetLib->RegisterEventHandler(kMSGID_ConnectionDataReceivedMsg, this, m_listenCID);
            EE_LOG(efd::kTesting, efd::ILogger::kLVL1,
                ("NameResolutionRequest: Bound to announce port %d",
                m_announcePort
             ));
        }
    }

    NameResolutionMessagePtr spNameResolutionMessage =
        EE_NEW MessageWrapper<NameResolutionMessage, kCLASSID_NameResolutionResponse>;
    spNameResolutionMessage->SetServiceClassID(classID);
    spNameResolutionMessage->SetName(name);
    spNameResolutionMessage->SetPort(port);
    spNameResolutionMessage->SetHostname(hostname);
    spNameResolutionMessage->SetResponseCategory(kCat_NameResolutionRequest);
    spNameResolutionMessage->SetExtraInfo(extraInfo);

    // insert into the list of authoritative responses we answer queries out of
    m_authoritativeResponses.insert(spNameResolutionMessage);
    // we don't actually send out an announcement unless we are asked
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::CancelAnnounce(efd::UInt32 classID, efd::utf8string name)
{
    NameResolutionMessageSet::iterator it = m_authoritativeResponses.begin();

    while (it != m_authoritativeResponses.end())
    {
        NameResolutionMessage* pResponseMessage = (*it);
        EE_ASSERT(pResponseMessage);
        if ((pResponseMessage->GetServiceClassID()==classID || classID==0)
            && (pResponseMessage->GetName()==name || name.empty()))
        {
            NameResolutionMessageSet::iterator removal = it;
            ++it;
            m_authoritativeResponses.erase (removal);
        }
        else
        {
            ++it;
        }
    }
}

//------------------------------------------------------------------------------------------------
efd::Category NameResolutionService::LookupService(efd::UInt32 classID, efd::TimeType tmo)
{
    return LookupService("", classID, tmo);
}

//------------------------------------------------------------------------------------------------
efd::Category NameResolutionService::LookupService(efd::utf8string name, efd::TimeType tmo)
{
    return LookupService(name, 0, tmo);
}

//------------------------------------------------------------------------------------------------
efd::Category NameResolutionService::LookupService(
    efd::utf8string name,
    efd::UInt32 classID,
    efd::TimeType tmo)
{
    efd::Category responseCategory = GenerateResponseCategory();
    SendLocalResponses(name, classID, responseCategory);
    SendRemoteRequest(name, classID, responseCategory, tmo);
    return responseCategory;
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::CancelLookup(efd::Category responseCategory)
{
    NameResolutionMessageList::iterator it = m_outstandingRequests.begin();

    while (it != m_outstandingRequests.end())
    {
        Category respCat = (*it)->GetResponseCategory();
        if (respCat == responseCategory)
        {
            NameResolutionMessageList::iterator eraseIt = it;
            ++it;
            SendRemoteResponsesDone(responseCategory);
            m_inFlightRequests.erase(responseCategory);
            m_outstandingRequests.erase(eraseIt);
        }
        else
        {
            ++it;
        }
    }
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::ClearCache()
{
    m_cachedResponses.clear();
}

//------------------------------------------------------------------------------------------------
efd::Category NameResolutionService::GenerateResponseCategory()
{
    Category responseCategory = kCat_NameResolutionResponse;
    responseCategory.SetNetID((m_responseID++) << 1);
    EE_ASSERT(m_inFlightRequests.find(responseCategory) == m_inFlightRequests.end());
    NameResolutionMessageSetPtr spMessagesAlreadySent =
        EE_NEW efd::RefCountedMemObj<NameResolutionMessageSet>;
    m_inFlightRequests[responseCategory] = spMessagesAlreadySent;
    return responseCategory;
}


//------------------------------------------------------------------------------------------------
void NameResolutionService::AddToCache(
    NameResolutionMessage* pNameResolutionMessage,
    const ConnectionID& senderConnectionID)
{
    // iterate outstanding local requests and locally send this message if it hasn't already been
    // sent
    NameResolutionMessageSetPtr spMessagesAlreadySent;
    Category destinationCategory = pNameResolutionMessage->GetResponseCategory();
    if (m_inFlightRequests.find(destinationCategory,spMessagesAlreadySent))
    {
        // check the ip address for m_inetAddrAny and set to the ip address of the sender
        if (pNameResolutionMessage->GetHostname() == m_inetAddrAny)
        {
            pNameResolutionMessage->SetHostname(
                m_spNetLib->IPToString(senderConnectionID.GetIP()));
        }

        if (spMessagesAlreadySent->find(pNameResolutionMessage) == spMessagesAlreadySent->end())
        {
            // this message has not yet been sent
            m_spMessageService->SendLocal(pNameResolutionMessage, destinationCategory);
            spMessagesAlreadySent->insert(pNameResolutionMessage);
        }
    }

    // add message to local cache
    m_cachedResponses.insert(pNameResolutionMessage);
}


//------------------------------------------------------------------------------------------------
void NameResolutionService::SendRemoteResponses(
    NameResolutionMessage* pNameResolutionMessage,
    ConnectionID senderConnectionID)
{
    EE_ASSERT(pNameResolutionMessage);
    // if we send responses it means we must be listening
    EE_ASSERT(m_listenCID != kCID_INVALID);
    Category responseCategory = pNameResolutionMessage->GetResponseCategory();

    // construct the appropriate response ConnectionID based on the request message
    efd::UInt16 responsePort = pNameResolutionMessage->GetResponsePort();
    efd::UInt32 responseIP = pNameResolutionMessage->GetResponseIP();
    if (!responseIP)
    {
        responseIP = senderConnectionID.GetIP();
    }
    efd::utf8string hostname = m_spNetLib->IPToString(responseIP);

    // look up all matching messages in cache and send them to the requestor
    for (NameResolutionMessageSet::iterator it = m_authoritativeResponses.begin();
        it != m_authoritativeResponses.end();
        ++it)
    {
        NameResolutionMessage* pResponseMessage = (*it);
        EE_ASSERT(pResponseMessage);
        if ((pResponseMessage->GetServiceClassID() == pNameResolutionMessage->GetServiceClassID()
            || pNameResolutionMessage->GetServiceClassID() == 0)
            && (pResponseMessage->GetName() == pNameResolutionMessage->GetName()
            || pNameResolutionMessage->GetName().empty()))
        {
            pResponseMessage->m_requestNumber = pNameResolutionMessage->m_requestNumber;
            m_spNetLib->SendTo(
                pResponseMessage,
                responseCategory,
                m_listenCID,
                hostname,
                responsePort);
        }
    }
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::HandleDataMessage(
    EnvelopeMessage* pEnvelopeMessage,
    const ConnectionID& senderConnectionID)
{
    // NOTE: Will come back NULL if payload is some old message type that can't be factoried
    const IMessage* pChildMessage = pEnvelopeMessage->GetContents(m_spMessageService);

    NameResolutionMessage* pNameResolutionMessage
        = EE_DYNAMIC_CAST(NameResolutionMessage, const_cast<IMessage*>(pChildMessage));

    if (pNameResolutionMessage)
    {
        switch (pNameResolutionMessage->GetClassID())
        {
        case kCLASSID_NameResolutionRequest:
            {
                EE_LOG(efd::kTesting, efd::ILogger::kLVL1,
                    ("NameResolutionRequest: %s",
                    senderConnectionID.ToString().c_str()));
                SendRemoteResponses(pNameResolutionMessage, senderConnectionID);
            }
            return;

        case kCLASSID_NameResolutionResponse:
            {
                EE_ASSERT(kCat_NameResolutionResponse.GetBaseID() ==
                    pEnvelopeMessage->GetDestinationCategory().GetBaseID());
                pNameResolutionMessage->SetResponseCategory(
                    pEnvelopeMessage->GetDestinationCategory());
                EE_LOG(efd::kTesting, efd::ILogger::kLVL1,
                    ("NameResponse: 0x%08X %s %s:%d %s %s\n",
                    pNameResolutionMessage->GetServiceClassID(),
                    pNameResolutionMessage->GetName().c_str(),
                    pNameResolutionMessage->GetHostname().c_str(),
                    pNameResolutionMessage->GetPort(),
                    pEnvelopeMessage->GetDestinationCategory().ToString().c_str(),
                    senderConnectionID.ToString().c_str()));
                AddToCache(pNameResolutionMessage, senderConnectionID);
            }
            return;
        }
    }
    // Some random legacy version message or something, just ignore it.
    EE_UNUSED_ARG(senderConnectionID);
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::SendRemoteRequestMessage(
    NameResolutionMessage* pNameResolutionMessage,
    bool newRequest)
{
    if (m_pritaveListenCID == kCID_INVALID)
    {
        m_pendingRequests.push_back(pNameResolutionMessage);
    }
    else
    {
        efd::TimeType curTime = m_pServiceManager->GetServiceManagerTime();
        if (newRequest)
        {
            pNameResolutionMessage->SetTimestamp(curTime);
            // add to list with timeout so we know when to call SendRemoteResponsesDone and
            // clean up memory associated with this request
            m_outstandingRequests.push_back(pNameResolutionMessage);
        }
        pNameResolutionMessage->SetResponsePort(m_pritaveListenCID.GetLocalPort());
        pNameResolutionMessage->SetResponseIP(m_pritaveListenCID.GetIP());
        pNameResolutionMessage->m_lastSend = curTime;
        // give each message a random delay
        pNameResolutionMessage->m_delay =
            5*(((efd::TimeType)efd::Rand()) /((efd::TimeType)RAND_MAX));
        // if the delay is too small make it bigger
        if (pNameResolutionMessage->m_delay < 0.4f)
        {
            pNameResolutionMessage->m_delay *= 2.0f;
        }
        Category responseCategory = pNameResolutionMessage->GetResponseCategory();

        m_spNetLib->SendTo(
            pNameResolutionMessage,
            responseCategory,
            m_pritaveListenCID,
            m_inetAddrBroadcast,
            m_announcePort);
        pNameResolutionMessage->m_requestNumber++;
    }
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::SendLocalResponses(
    efd::utf8string name,
    efd::UInt32 classID,
    Category responseCategory)
{
    // iterate outstanding local requests and locally send this message if it hasn't already been
    // sent
    NameResolutionMessageSetPtr spMessagesAlreadySent;
    m_inFlightRequests.find(responseCategory,spMessagesAlreadySent);
    EE_ASSERT(spMessagesAlreadySent);

    for (NameResolutionMessageSet::iterator it = m_cachedResponses.begin();
        it != m_cachedResponses.end();
        ++it)
    {
        NameResolutionMessagePtr spNameResolutionMessage = *it;
        if ((spNameResolutionMessage->GetName() == name)
            && (spNameResolutionMessage->GetServiceClassID() == classID)
            || (spNameResolutionMessage->GetName() == name)
            && (spNameResolutionMessage->GetServiceClassID() == 0)
            || (spNameResolutionMessage->GetName() == "")
            && (spNameResolutionMessage->GetServiceClassID() == classID))
        {
            if (m_spMessageService)
            {
                m_spMessageService->SendLocal(spNameResolutionMessage, responseCategory);
            }
            else
            {
                m_pendingResponses.push_back(spNameResolutionMessage);
            }
            spMessagesAlreadySent->insert(spNameResolutionMessage);
        }
    }
    SendLocalResponsesDone(responseCategory);
    m_inFlightRequests[responseCategory] = spMessagesAlreadySent;
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::SendRemoteRequest(
    efd::utf8string name,
    efd::UInt32 classID,
    Category responseCategory,
    efd::TimeType tmo)
{
    NameResolutionMessagePtr spNameResolutionMessage =
        EE_NEW MessageWrapper<NameResolutionMessage, kCLASSID_NameResolutionRequest >;
    spNameResolutionMessage->SetName(name);
    spNameResolutionMessage->SetServiceClassID(classID);
    spNameResolutionMessage->SetResponseCategory(responseCategory);
    spNameResolutionMessage->SetRequestTimeout(tmo);
    SendRemoteRequestMessage(spNameResolutionMessage, true);
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::SendLocalResponsesDone(Category responseCategory)
{
    NameResolutionMessagePtr spNameResolutionMessage =
        EE_NEW MessageWrapper<NameResolutionMessage, kCLASSID_NameResolutionLocalDone>;
    spNameResolutionMessage->SetResponseCategory(responseCategory);
    if (m_spMessageService)
    {
        m_spMessageService->SendLocal(spNameResolutionMessage, responseCategory);
    }
    else
    {
        m_pendingResponses.push_back(spNameResolutionMessage);
    }
}

//------------------------------------------------------------------------------------------------
void NameResolutionService::SendRemoteResponsesDone(Category responseCategory)
{
    IMessage* spDoneMessage = EE_NEW MessageWrapper<IMessage, kCLASSID_NameResolutionRemoteDone>;
    m_spMessageService->SendLocal(spDoneMessage, responseCategory);
}

//------------------------------------------------------------------------------------------------
SyncResult NameResolutionService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    EE_UNUSED_ARG(pDependencyRegistrar);

    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    EE_ASSERT(m_spMessageService);
    if (!m_spMessageService)
    {
        EE_LOG(efd::kNameResolutionService, ILogger::kERR0,
            ("%s> Failed. MessageService not found", __FUNCTION__));
        return efd::SyncResult_Failure;
    }
#ifdef OWN_NETLIB

    EE_ASSERT(m_pServiceManager);
    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    if (pConfigManager)
    {
        const ISection *pSection =
            pConfigManager->GetConfiguration()->FindSection(NetService::kConfigSection);

        if (pSection)
        {
            efd::utf8string netLibType = pSection->FindValue(NetService::kNetLibType);
            if (!netLibType.empty())
            {
                m_netLibType = netLibType;
            }
        }

        pSection =
            pConfigManager->GetConfiguration()->FindSection(kNameResolutionServiceConfig);

        if (pSection)
        {
            efd::utf8string temp = pSection->FindValue(kAnnouncePort);
            if (!temp.empty())
            {
                m_announcePort = (efd::UInt16)atoi(temp.c_str());;
            }
        }
    }
    m_spNetLib = EE_NEW NetLib();

    // initialize INetLib
    if (INetLib::StartNet() <= 0)
    {
        return efd::SyncResult_Failure;
    }

    // read in QOS mapping from config file
    EnumManager* pEnumManager = m_pServiceManager->GetSystemServiceAs<EnumManager>();
    QOSCompare::ReadConfig(pConfigManager, pEnumManager);

#else
    m_spNetService = m_pServiceManager->GetSystemServiceAs<NetService>();
    EE_ASSERT(m_spNetService);
    if (!m_spNetService)
    {
        EE_LOG(efd::kNameResolutionService, ILogger::kERR0,
            ("%s> Failed. NetService not found", __FUNCTION__));
        return efd::SyncResult_Failure;
    }
    m_spNetLib = m_spNetService->GetNetLib();
#endif //OWN_NETLIB

    EE_ASSERT(m_spNetLib);
    if (!m_spNetLib)
    {
        EE_LOG(efd::kNameResolutionService, ILogger::kERR0,
            ("%s> Failed. INetLib not found", __FUNCTION__));
        return efd::SyncResult_Failure;
    }
    RegisterMessageWrapperFactory<NameResolutionMessage, kCLASSID_NameResolutionRequest>(
        m_spMessageService);
    RegisterMessageWrapperFactory<NameResolutionMessage, kCLASSID_NameResolutionResponse>(
        m_spMessageService);

    return efd::SyncResult_Success;
}


//------------------------------------------------------------------------------------------------
AsyncResult NameResolutionService::OnInit()
{
    if (!INetLib::IsNetReady())
    {
        return efd::AsyncResult_Pending;
    }

    //look for queued requests that have not gone remotely yet
    for (NameResolutionMessageList::iterator it = m_pendingResponses.begin();
        it != m_pendingResponses.end();
        ++it)
    {
        m_spMessageService->SendLocal(*it, (*it)->GetResponseCategory());
    }
    m_pendingResponses.clear();

    if (m_pritaveListenCID == kCID_INVALID)
    {
        // binding/listening on port 0 allows the os to pick the port for us
        m_pritaveListenCID = m_spNetLib->Listen(m_inetAddrAny, 0, QOS_CONNECTIONLESS,this);
        EE_ASSERT(m_pritaveListenCID != kCID_INVALID);
        m_spNetLib->RegisterEventHandler(
            kMSGID_ConnectionDataReceivedMsg,
            this,
            m_pritaveListenCID);
        EE_LOG(efd::kTesting, efd::ILogger::kLVL1,
            ("NameResolutionRequest: Bound to response port %d",
            m_announcePort
         ));

    }
    return efd::AsyncResult_Complete;
}


//------------------------------------------------------------------------------------------------
AsyncResult NameResolutionService::OnTick()
{
#ifdef OWN_NETLIB
    m_spNetLib->Tick();
#endif //OWN_NETLIB

    // there should not be any pendingResponses after OnInit completes, ever
    EE_ASSERT(m_pendingResponses.size() == 0);

    // Iterate pending requests to see if any have expired, send SendRemoteResponsesDone if they
    // have
    efd::TimeType curTime = m_pServiceManager->GetServiceManagerTime();
    NameResolutionMessageList::iterator it = m_outstandingRequests.begin();

    while (it != m_outstandingRequests.end())
    {
        efd::TimeType sendTime = (*it)->GetTimestamp();
        efd::TimeType timeout = (*it)->GetRequestTimeout();
        if (timeout != NameResolutionService::kNameLookup_NoTimeout &&
           (sendTime + timeout) <= curTime)
        {
            Category responseCategory = (*it)->GetResponseCategory();
            NameResolutionMessageList::iterator eraseIt = it;
            ++it;
            SendRemoteResponsesDone(responseCategory);
            m_inFlightRequests.erase(responseCategory);
            m_outstandingRequests.erase(eraseIt);
        }
        else
        {
            ++it;
        }
    }

    // if it has been long enough sine the last time we sent a request, send another
    // we need to keep spamming requests since we are using an unreliable protocol

    // try to keep the number of broadcast messages sent per tick down
    static const UInt32 maxPerTick = 1; //DT32271 Move to config paramater.
    UInt32 broadcastMessagesSent = 0;
    it = m_outstandingRequests.begin();
    while (broadcastMessagesSent < maxPerTick && it != m_outstandingRequests.end())
    {
        NameResolutionMessagePtr spNameResolutionMessage = *it;
        efd::TimeType sendDelay = spNameResolutionMessage->m_delay;
        efd::TimeType lastSendTime = spNameResolutionMessage->m_lastSend;
        if ((lastSendTime + sendDelay) < curTime)
        {
            SendRemoteRequestMessage(spNameResolutionMessage, false);
            ++broadcastMessagesSent;
            // reorder the list so that next tick someone else gets a chance to go first
            NameResolutionMessageList::iterator eraseIter = it;
            ++it;
            m_outstandingRequests.erase(eraseIter);
            m_outstandingRequests.push_back(spNameResolutionMessage);
        }
        else
        {
            ++it;
        }
    }

    //look for queued requests that have not gone remotely yet
    // only send one per tick to not flood the network with broadcast requests
    NameResolutionMessagePtr spNameResolutionMessage;
    m_pendingRequests.pop_front(spNameResolutionMessage);
    if (spNameResolutionMessage)
    {
        SendRemoteRequestMessage(spNameResolutionMessage, true);
    }
    return efd::AsyncResult_Pending;
}


//------------------------------------------------------------------------------------------------
AsyncResult NameResolutionService::OnShutdown()
{
    if (m_listenCID != kCID_INVALID)
    {
        m_spNetLib->CloseConnection(m_listenCID);
    }
    if (m_pritaveListenCID != kCID_INVALID)
    {
        m_spNetLib->CloseConnection(m_pritaveListenCID);
    }

#ifdef OWN_NETLIB
    m_spNetLib->Shutdown();
    INetLib::StopNet();
#endif //OWN_NETLIB

    m_spNetLib->RemoveLocalConsumer(this);
    m_spMessageService = NULL;
    m_spNetService = NULL;
    m_spNetLib = NULL;
    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
const utf8string& NameResolutionService::GetInternetAddressAny() const
{
    return m_inetAddrAny;
}

//------------------------------------------------------------------------------------------------
const utf8string& NameResolutionService::GetInternetAddressBroadcast() const
{
    return m_inetAddrBroadcast;
}

//------------------------------------------------------------------------------------------------
