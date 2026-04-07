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

#include <efd/IDs.h>
#include <efd/StreamMessage.h>
#include <efd/Metrics.h>

#include <efdNetwork/HostInfo.h>
#include <efdNetwork/NetLib.h>
#include <efdNetwork/NetListener.h>
#include <efdNetwork/NetTransportTCP.h>
#include <efdNetwork/NetworkMessages.h>
#include <efd/QOSCompare.h>

#include <efdNetwork/efdNetworkSDM.h>

//------------------------------------------------------------------------------------------------
using namespace efd;

const ConnectionID NetLib::ms_localConnectionAddress(efd::QOS_LOCAL, INADDR_LOOPBACK, 0, 0);

// Note: This inserts efdNetwork into the static data manager chain.  It must be placed in a
// compilation unit that is always used when the library is used (if it is only placed in
// efdNetworkSDM.cpp, it will be removed by the linker)
static efdNetworkSDM efdNetworkSDMObject;


//------------------------------------------------------------------------------------------------
NetLib::NetLib()
    : m_bufferStatus(efd::kMSGID_NetworkBufferOK)
    , m_pISection(NULL)
{
    m_spMessageList = EE_NEW NetMessageList();
    EE_ASSERT(m_spMessageList);
}


//------------------------------------------------------------------------------------------------
NetLib::~NetLib()
{
    Shutdown();

    // optional; this map should empty itself in its own destructor
    m_localListeners.clear();
    m_remoteListeners.clear();

    if (m_spMessageList)
    {
        m_spMessageList->clear();
    }

    m_spMessageList = NULL;

    m_producerMap.clear();


    m_remoteListeners.clear();
    m_transportMap.clear();
    m_catToQualityOfServiceMap.clear();
    HostInfo::FlushCache();
}

//------------------------------------------------------------------------------------------------
void NetLib::Configure(ISection* pISection)
{
    m_pISection = pISection;
}

//------------------------------------------------------------------------------------------------
void NetLib::Shutdown()
{
    // cause all transports to be deleted before we tear everything else down
    for (RegisteredTransportMap::iterator transportPos = m_transportMap.begin();
         transportPos != m_transportMap.end();
         ++transportPos)
    {
        transportPos->second->OnShutdown();
    }
    m_transportMap.clear();
}


//------------------------------------------------------------------------------------------------
void NetLib::AddLocalConsumer(
    const Category& destCategory,
    INetCallback* pCallback)
{
    INetCallbackSetPtr spCallbackSet = NULL;
    if (!m_localListeners.find(destCategory,spCallbackSet))
    {
        spCallbackSet = EE_NEW INetCallbackSet();
        m_localListeners[destCategory] = spCallbackSet;
    }
    EE_ASSERT(spCallbackSet);
    EE_ASSERT(pCallback);
    spCallbackSet->insert(pCallback);

    EE_LOG(efd::kCategoryTrace, efd::ILogger::kLVL2,
        ("%s: add local consumer 0x%08X netid=%d",
        destCategory.ToString().c_str(), pCallback, GetNetID()));
}


//------------------------------------------------------------------------------------------------
void NetLib::RemoveLocalConsumer(const Category& destCategory, INetCallback* pCallback)
{
    INetCallbackSetPtr spCallbackSet = NULL;
    if (m_localListeners.find(destCategory,spCallbackSet))
    {
        EE_ASSERT(spCallbackSet);
        INetCallbackSet::iterator callbackSetIter = spCallbackSet->begin();
        while (callbackSetIter != spCallbackSet->end())
        {
            if ((*callbackSetIter) == pCallback)
            {
                EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
                    ("Callback 0x%08X Unsubscribed from %s",
                    pCallback,
                    destCategory.ToString().c_str()));
                INetCallbackSet::iterator eraseIter = callbackSetIter++;
                spCallbackSet->erase(eraseIter);
            }
            else
            {
                ++callbackSetIter;
            }
        }
        if (spCallbackSet->empty())
        {
            m_localListeners.erase(destCategory);
        }
    }

    EE_LOG(efd::kCategoryTrace, efd::ILogger::kLVL2,
        ("%s: remove local consumer 0x%08X netid=%d",
        destCategory.ToString().c_str(), pCallback, GetNetID()));
}


//------------------------------------------------------------------------------------------------
void NetLib::RemoveLocalConsumer(INetCallback* pCallback)
{
    InternalRemoveCallback(pCallback);
}

//------------------------------------------------------------------------------------------------
void NetLib::RemoveCallback(INetCallback* pCallback)
{
    // remove from local data structure
    RemoveLocalConsumer(pCallback);

    // remove from all connections
    for (RegisteredTransportMap::iterator transportPos = m_transportMap.begin();
        transportPos != m_transportMap.end();
        ++transportPos)
    {
        transportPos->second->RemoveCallback(pCallback, this);
    }
}


//------------------------------------------------------------------------------------------------
void NetLib::RemoveAllLocalConsumers()
{
    m_localListeners.clear();
}


//------------------------------------------------------------------------------------------------
INetCallbackSetPtr NetLib::GetLocalConsumers(const Category &destCategory)
{
    efd::INetCallbackSetPtr spINetCallbackSet;
    m_localListeners.find(destCategory, spINetCallbackSet);
    return spINetCallbackSet;
}


//------------------------------------------------------------------------------------------------
void NetLib::AddRemoteConsumer(const Category& destCategory, const ConnectionID& cid)
{
    ConnectionIDSetPtr spConnectionIDSet = NULL;
    EE_ASSERT(cid.GetQualityOfService() != QOS_INVALID);
    if (!m_remoteListeners.find(destCategory,spConnectionIDSet))
    {
        spConnectionIDSet = EE_NEW ConnectionIDSet();
        m_remoteListeners[destCategory] = spConnectionIDSet;
        MapCategoryToQualityOfService(destCategory,cid.GetQualityOfService());
    }
    else
    {
        EE_ASSERT(GetTransport(destCategory) == cid.GetQualityOfService()
            || GetTransport(destCategory) == QOS_INVALID);
        if (GetTransport(destCategory) == QOS_INVALID)
        {
            MapCategoryToQualityOfService(destCategory,cid.GetQualityOfService());
        }
    }
    EE_ASSERT(spConnectionIDSet);
    spConnectionIDSet->insert(cid);

    EE_LOG(efd::kCategoryTrace, efd::ILogger::kLVL2,
        ("%s: add remote consumer %s netid=%d",
        destCategory.ToString().c_str(), cid.ToString().c_str(), GetNetID()));
}


//------------------------------------------------------------------------------------------------
void NetLib::RemoveRemoteConsumer(const Category& destCategory, const ConnectionID& cid)
{
    ConnectionIDSetPtr spConnectionIDSet = NULL;
    if (m_remoteListeners.find(destCategory,spConnectionIDSet))
    {
        EE_ASSERT(spConnectionIDSet);
        ConnectionIDSet::iterator callbackSetIter = spConnectionIDSet->begin();
        while (callbackSetIter != spConnectionIDSet->end())
        {
            if ((*callbackSetIter) == cid)
            {
                EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
                    ("%s Unsubscribed from %s",
                    cid.ToString().c_str(),
                    destCategory.ToString().c_str()));
                ConnectionIDSet::iterator eraseIter = callbackSetIter++;
                spConnectionIDSet->erase(eraseIter);
            }
            else
            {
                ++callbackSetIter;
            }
        }
        if (spConnectionIDSet->empty())
        {
            m_remoteListeners.erase(destCategory);
        }
    }

    EE_LOG(efd::kCategoryTrace, efd::ILogger::kLVL2,
        ("%s: remove remote consumer %s netid=%d",
        destCategory.ToString().c_str(), cid.ToString().c_str(), GetNetID()));
}


//------------------------------------------------------------------------------------------------
void NetLib::RemoveRemoteConsumer(const ConnectionID& cid)
{
    CategoryToConnectionIDMap::iterator it = m_remoteListeners.begin();
    while (it != m_remoteListeners.end())
    {
        ConnectionIDSetPtr spCallbackSet = (*it).second;
        EE_ASSERT(spCallbackSet);
        ConnectionIDSet::iterator callbackSetIter = spCallbackSet->begin();
        while (callbackSetIter != spCallbackSet->end())
        {
            if ((*callbackSetIter) == cid)
            {
                EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
                    ("Warning! %s still subscribed to %s",
                    cid.ToString().c_str(),
                    (*it).first.ToString().c_str()));
                ConnectionIDSet::iterator eraseIter = callbackSetIter++;
                spCallbackSet->erase(eraseIter);
            }
            else
            {
                ++callbackSetIter;
            }
        }
        if (spCallbackSet->empty())
        {
            CategoryToConnectionIDMap::iterator eraseIter = it++;
            m_remoteListeners.erase(eraseIter);
        }
        else
        {
            ++it;
        }
    }
}


//------------------------------------------------------------------------------------------------
ConnectionIDSetPtr NetLib::GetRemoteConsumers(const Category& destCategory)
{
    efd::ConnectionIDSetPtr spConnectionIDSet;
    m_remoteListeners.find(destCategory, spConnectionIDSet);
    return spConnectionIDSet;
}


//------------------------------------------------------------------------------------------------
void NetLib::RemoveAllRemoteConsumers()
{
    m_remoteListeners.clear();
}


//------------------------------------------------------------------------------------------------
efd::Bool NetLib::HasRemoteConsumers(const Category& destCategory)
{
    efd::ConnectionIDSetPtr spConnectionIDSet = GetRemoteConsumers(destCategory);
    return (spConnectionIDSet != NULL);
}


//------------------------------------------------------------------------------------------------
// this should be called from network background thread
void NetLib::QueueMessage(NetMessage *pMessage)
{
    if (!pMessage)
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR0,
            ("Error: NULL Message - Failed to post a new Message from one of our transports"));

        // tick off another error occurred
        EE_LOG_METRIC_COUNT_FMT(kNetwork, ("RECEIVE.ERROR.%u.0x%llX",
            SafeGetTransport(pMessage->GetDestinationCategory()),
            pMessage->GetSenderConnection().GetValue()));

        return;
    }

    EE_MESSAGE_BREAK(
        pMessage,
        IMessage::mdf_BreakOnNetQueue,
        ("%s| Net Queue for destination %s",
        pMessage->GetDescription().c_str(),
        pMessage->GetDestinationCategory().ToString().c_str()));

    // Insert new message at the end of the list (messages processed at tick)
    m_spMessageList->push_back(pMessage);

    // Tick off a received message by QoS & connection ID
    EE_LOG_METRIC_COUNT_FMT(kNetwork, ("RECEIVE.MESSAGE.%u.0x%llX",
        SafeGetTransport(pMessage->GetDestinationCategory()),
        pMessage->GetSenderConnection().GetValue()));

    // tick off another incoming message (by category)
    EE_LOG_METRIC_COUNT_FMT(kNetwork, ("RECEIVE.CATEGORY.0x%llX",
        pMessage->GetDestinationCategory().GetValue()));
}


//------------------------------------------------------------------------------------------------
void NetLib::DeliverQueuedMessages()
{
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL3,
        ("NetLib::DeliverQueuedMessages, %d incoming messages on queue",
        m_spMessageList->size()));

    // iterate through incoming messages and deliver to local message service
    //    or process ourselves (if system message for us)...
    while (!m_spMessageList->empty())
    {
        NetMessagePtr spMessage;
        // Grab the first element in the  Message list
        m_spMessageList->pop_front(spMessage);

        EE_MESSAGE_BREAK(
            spMessage,
            IMessage::mdf_BreakOnFirstNetDelivery,
            ("%s| First Net Delivery", spMessage->GetDescription().c_str()));

        // process the message...
        const Category& destCategory = spMessage->GetDestinationCategory();

        INetCallbackSetPtr localListeners = GetLocalConsumers(destCategory);

        if (localListeners)
        {
            // check to see if we know about this Category
            QualityOfService foundQOS = GetTransport(destCategory);
            if (foundQOS == QOS_INVALID)
            {
                // record which QOS this Category is associated with
                MapCategoryToQualityOfService(
                    destCategory,
                    spMessage->GetSenderConnection().GetQualityOfService());
            }

            EE_ASSERT(!localListeners->empty());
            EnvelopeMessagePtr spEnvelope = EE_DYNAMIC_CAST(EnvelopeMessage, spMessage);
            if (spEnvelope)
            {
                EE_MESSAGE_TRACE(
                    spEnvelope,
                    efd::kNetMessage,
                    ILogger::kLVL1,
                    ILogger::kLVL2,
                    ("Received %s on %s %s NetID=%d",
                    spEnvelope->GetChildDescription().c_str(),
                    destCategory.ToString().c_str(),
                    spMessage->GetSenderConnection().ToString().c_str(),
                    spMessage->GetSenderNetID()));
            }
            else
            {
                EE_MESSAGE_TRACE(
                    spMessage,
                    efd::kNetMessage,
                    ILogger::kLVL1,
                    ILogger::kLVL2,
                    ("Received (non-envelope) %s on %s %s NetID=%d",
                    spMessage->GetDescription().c_str(),
                    destCategory.ToString().c_str(),
                    spMessage->GetSenderConnection().ToString().c_str(),
                    spMessage->GetSenderNetID()));
            }
            // call callback for each registered listener
            for (INetCallbackSet::iterator it = localListeners->begin();
                it != localListeners->end();
                ++it)
            {
                INetCallback* pCallback = *it;

                EE_MESSAGE_BREAK(
                    spMessage,
                    IMessage::mdf_BreakOnEachNetDelivery,
                    ("%s| Net Delivery to target 0x%08X on channel %s",
                    spMessage->GetDescription().c_str(),
                    pCallback,
                    destCategory.ToString().c_str()));

                pCallback->HandleNetMessage(spMessage, spMessage->GetSenderConnection());
            }
        }
        else
        {
            // if here, received a message for which we don't have a listener...
            EnvelopeMessagePtr spEnvelope = EE_DYNAMIC_CAST(EnvelopeMessage, spMessage);
            if (spEnvelope)
            {
                EE_MESSAGE_TRACE(
                    spMessage,
                    efd::kNetMessage,
                    ILogger::kERR1,
                    ILogger::kERR3,
                    ("Received envelope with contents %s on %s %s NetID=%d with _NO_ listeners",
                    spEnvelope->GetChildDescription().c_str(),
                    destCategory.ToString().c_str(),
                    spMessage->GetSenderConnection().ToString().c_str(),
                    spMessage->GetSenderNetID()));
            }
            else
            {
                EE_MESSAGE_TRACE(
                    spMessage,
                    efd::kNetMessage,
                    ILogger::kERR1,
                    ILogger::kERR3,
                    ("Received %s on %s %s NetID=%d with _NO_ listeners",
                    spMessage->GetDescription().c_str(),
                    destCategory.ToString().c_str(),
                    spMessage->GetSenderConnection().ToString().c_str(),
                    spMessage->GetSenderNetID()));
            }

            // tick off another error occurred
            EE_LOG_METRIC_COUNT_FMT(kNetwork, ("RECEIVE.ERROR.%u.0x%llX",
                SafeGetTransport(spMessage->GetDestinationCategory()),
                spMessage->GetSenderConnection().GetValue()));

        }
    }
}


//------------------------------------------------------------------------------------------------
void NetLib::Send(
    IMessage *pMessage,
    const Category &destCategory,
    efd::QualityOfService virtualQOS)
{
    QualityOfService defaultQOS = QOSCompare::LookupPhysical(virtualQOS);
    // set type so that locally queued message has correct message type
    EE_ASSERT(destCategory.IsValid());

    EnvelopeMessagePtr spEnvelopeMsg =
        EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_ConnectionDataReceivedMsg>;
    spEnvelopeMsg->SetChild(pMessage);
    spEnvelopeMsg->SetDestinationCategory(destCategory);
    spEnvelopeMsg->SetSenderNetID(GetNetID());
    // set connection private cat so that DeliverQueuedMessages knows we intend to loop back this
    // message
    spEnvelopeMsg->SetSenderConnection(ms_localConnectionAddress);
    // Queue for local delivery
    QueueMessage(spEnvelopeMsg);
    // Also send remotely
    SendRemote(pMessage, destCategory, defaultQOS);
}


//------------------------------------------------------------------------------------------------
void NetLib::Forward(EnvelopeMessage* pEnvelopeMessage, QualityOfService virtualQOS)
{
    QualityOfService defaultQOS = QOSCompare::LookupPhysical(virtualQOS);
    Category destCategory = pEnvelopeMessage->GetDestinationCategory();
    QualityOfService qualityOfService = GetTransport(destCategory);
    if (qualityOfService == QOS_INVALID)
    {
        // we must at least know the quality of service to be used to send the message
        if (defaultQOS != QOS_INVALID)
        {
            qualityOfService = defaultQOS;
            MapCategoryToQualityOfService(destCategory, defaultQOS);
        }
        else
        {
            EE_FAIL_MESSAGE(("%s| %s %s has no QOS, did you forget to call "
                "BeginCategoryProduction?",
                pEnvelopeMessage->GetDescription().c_str(),
                pEnvelopeMessage->GetChildDescription().c_str(),
                destCategory.ToString().c_str()));
            return;
        }
    }
    // we can't send on a different QOS than the Category of this message has already been sent on
    EE_ASSERT(defaultQOS == QOS_INVALID || qualityOfService == defaultQOS);
    pEnvelopeMessage->SetQualityOfService(qualityOfService);

    ConnectionIDSetPtr spListenerList = GetRemoteConsumers(destCategory);
    if (!spListenerList)
    {
        EE_MESSAGE_TRACE(
            pEnvelopeMessage,
            efd::kNetMessage,
            ILogger::kERR1,
            ILogger::kLVL2,
            ("%s| %s %s has no remote listeners (netid=%d)",
            pEnvelopeMessage->GetDescription().c_str(),
            pEnvelopeMessage->GetChildDescription().c_str(),
            destCategory.ToString().c_str(), GetNetID()));
        return;
    }

    // get which registered transport matches the requested transport
    INetTransportPtr spTransport;
    if (m_transportMap.find(qualityOfService, spTransport) == false)
    {
        // if here, error...  There is no transport registered for this qualityOfService
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("%s| %s no transport found for category %s, QOS %d",
            pEnvelopeMessage->GetDescription().c_str(),
            pEnvelopeMessage->GetChildDescription().c_str(),
            destCategory.ToString().c_str(),
            qualityOfService));

        // tick off another error occurred
        EE_LOG_METRIC_COUNT_FMT(kNetwork, ("SEND.ERROR.%u.0x%llX",
            SafeGetTransport(pEnvelopeMessage->GetDestinationCategory()),
            pEnvelopeMessage->GetSenderConnection().GetValue()));

        return;
    }

    // now, finally, send message to transport with list of listeners...
    EE_MESSAGE_TRACE(
        pEnvelopeMessage,
        efd::kNetMessage,
        ILogger::kLVL1,
        ILogger::kLVL2,
        ("%s| %s Sending To Transport destination %s, QOS %d",
        pEnvelopeMessage->GetDescription().c_str(),
        pEnvelopeMessage->GetChildDescription().c_str(),
        destCategory.ToString().c_str(),
        qualityOfService));
    bool retval = spTransport->Send(pEnvelopeMessage, spListenerList);

    // Check for a send error
    if (!retval)
    {
        EE_MESSAGE_TRACE(
            pEnvelopeMessage,
            efd::kNetMessage,
            ILogger::kERR1,
            ILogger::kLVL2,
            ("%s| Sending payload %s to QualityOfService %d FAILED!",
            pEnvelopeMessage->GetDescription().c_str(),
            pEnvelopeMessage->GetChildDescription().c_str(),
            qualityOfService));
        // Check to see if an error occurred last time
        if (spTransport->GetLastResult())
        {
            // Save the last status
            spTransport->SetLastResult(retval);
            NetworkStatusPtr spStatus =
                EE_NEW MessageWrapper<NetworkStatus, efd::kMSGID_NetworkFailure>();
            spStatus->Set(
                qualityOfService,
                m_netID);

            // Insert new message at the end of the list (messages processed at tick)
            NetMessagePtr spMessage =
                EE_NEW EnvelopeMessage(spStatus, kCAT_NetEvent, m_netID);
            ConnectionID localCID(QOS_LOCAL,0,0,0);
            spMessage->SetSenderConnection(localCID);
            m_spMessageList->push_back(spMessage);
        }
    }
    else
    {
        // Check to see if an error occurred last time
        if (!spTransport->GetLastResult())
        {
            // Save the last status
            spTransport->SetLastResult(retval);
            NetworkStatusPtr spStatus =
                EE_NEW MessageWrapper<NetworkStatus, efd::kMSGID_NetworkBufferOK>();
            spStatus->Set(
                qualityOfService,
                m_netID);

            // Insert new message at the end of the list (messages processed at tick)
            NetMessagePtr spMessage =
                EE_NEW EnvelopeMessage(spStatus, kCAT_NetEvent, m_netID);
            ConnectionID localCID(QOS_LOCAL,0,0,0);
            spMessage->SetSenderConnection(localCID);
            m_spMessageList->push_back(spMessage);
        }
    }
}


//------------------------------------------------------------------------------------------------
void NetLib::Forward(EnvelopeMessage* pEnvelopeMessage, const ConnectionID& cid)
{
    EE_ASSERT(cid.m_qos != QOS_INVALID);
    INetTransportPtr spTransport;
    if (!m_transportMap.find(cid.m_qos,spTransport))
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("%s| %s no transport found for %s",
            pEnvelopeMessage->GetDescription().c_str(),
            pEnvelopeMessage->GetChildDescription().c_str(),
            cid.ToString().c_str()));
        return;
    }

    EE_ASSERT(spTransport);
    EE_MESSAGE_TRACE(
        pEnvelopeMessage,
        efd::kNetMessage,
        ILogger::kLVL1,
        ILogger::kLVL2,
        ("%s| Sending payload %s to Transport for %s",
        pEnvelopeMessage->GetDescription().c_str(),
        pEnvelopeMessage->GetChildDescription().c_str(),
        cid.ToString().c_str()));
    bool retval = spTransport->Send(pEnvelopeMessage, cid);

    // Check for a send error
    if (!retval)
    {
        EE_MESSAGE_TRACE(
            pEnvelopeMessage,
            efd::kNetMessage,
            ILogger::kERR1,
            ILogger::kLVL2,
            ("%s| Sending payload %s to Transport for %s FAILED!",
            pEnvelopeMessage->GetDescription().c_str(),
            pEnvelopeMessage->GetChildDescription().c_str(),
            cid.ToString().c_str()));
        // Check to see if an error occurred last time
        if (spTransport->GetLastResult())
        {
            // Save the last status
            spTransport->SetLastResult(retval);
            NetworkStatusPtr spStatus =
                EE_NEW MessageWrapper<NetworkStatus, efd::kMSGID_NetworkFailure>();
            spStatus->Set(cid, m_netID);

            // Insert new message at the end of the list (messages processed at tick)
            NetMessagePtr spMessage =
                EE_NEW EnvelopeMessage(spStatus, kCAT_NetEvent, m_netID);
            ConnectionID localCID(QOS_LOCAL,0,0,0);
            spMessage->SetSenderConnection(localCID);
            m_spMessageList->push_back(spMessage);
        }
    }
    else
    {
        // Check to see if an error occurred last time
        if (!spTransport->GetLastResult())
        {
            // Save the last status
            spTransport->SetLastResult(retval);
            NetworkStatusPtr spStatus =
                EE_NEW MessageWrapper<NetworkStatus, efd::kMSGID_NetworkBufferOK>();
            spStatus->Set(cid, m_netID);

            // Insert new message at the end of the list (messages processed at tick)
            NetMessagePtr spMessage =
                EE_NEW EnvelopeMessage(spStatus, kCAT_NetEvent, m_netID);
            ConnectionID localCID(QOS_LOCAL,0,0,0);
            spMessage->SetSenderConnection(localCID);
            m_spMessageList->push_back(spMessage);
        }
    }
}


//------------------------------------------------------------------------------------------------
void NetLib::SendRemote(
    IMessage* pMessage,
    const Category& destCategory,
    QualityOfService virtualQOS)
{
    QualityOfService defaultQOS = QOSCompare::LookupPhysical(virtualQOS);
    EE_ASSERT(destCategory.IsValid());
    EnvelopeMessagePtr spEnvelopeMsg =
        EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_RemoteWrapper>;
    EE_ASSERT(spEnvelopeMsg);
    spEnvelopeMsg->SetChild(pMessage);
    spEnvelopeMsg->SetDestinationCategory(destCategory);
    spEnvelopeMsg->SetSenderNetID(GetNetID());

    Forward(spEnvelopeMsg, defaultQOS);
}


//------------------------------------------------------------------------------------------------
void NetLib::SendRemote(
    IMessage* pMessageToSend,
    const Category& destCategory,
    const ConnectionID& cid)
{
    EE_ASSERT(cid.m_qos != QOS_INVALID);
    EE_ASSERT(destCategory.IsValid());
    EnvelopeMessagePtr spEnvelopeMsg =
        EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_RemoteWrapper>;
    EE_ASSERT(spEnvelopeMsg);
    spEnvelopeMsg->SetChild(pMessageToSend);
    spEnvelopeMsg->SetDestinationCategory(destCategory);
    spEnvelopeMsg->SetSenderNetID(GetNetID());

    Forward(spEnvelopeMsg, cid);
}


//------------------------------------------------------------------------------------------------
void NetLib::SendTo(
    IMessage* pMessageToSend,
    const Category& destCategory,
    const ConnectionID& sourceConnectionID,
    const efd::utf8string& strServerAddress,
    efd::UInt16 portServer)
{
    EE_ASSERT(sourceConnectionID.m_qos != QOS_INVALID);
    EE_ASSERT(sourceConnectionID.m_qos & NET_UDP);
    EE_ASSERT(sourceConnectionID.m_qos & NET_CONNECTIONLESS);
    EE_ASSERT(destCategory.IsValid());
    EnvelopeMessagePtr spEnvelopeMsg =
        EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_RemoteWrapper>;
    EE_ASSERT(spEnvelopeMsg);
    spEnvelopeMsg->SetChild(pMessageToSend);
    spEnvelopeMsg->SetDestinationCategory(destCategory);
    spEnvelopeMsg->SetSenderNetID(GetNetID());

    INetTransportPtr spTransport;
    if (!m_transportMap.find(sourceConnectionID.GetQualityOfService(),spTransport))
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("SendTo: %s| %s no transport found for %s",
            spEnvelopeMsg->GetDescription().c_str(),
            spEnvelopeMsg->GetChildDescription().c_str(),
            sourceConnectionID.ToString().c_str()));
        return;
    }

    EE_ASSERT(spTransport);
    EE_MESSAGE_TRACE(
        spEnvelopeMsg,
        efd::kNetMessage,
        ILogger::kLVL1,
        ILogger::kLVL2,
        ("%s| SendTo Sending payload %s to Transport for %s",
        spEnvelopeMsg->GetDescription().c_str(),
        spEnvelopeMsg->GetChildDescription().c_str(),
        sourceConnectionID.ToString().c_str()));
    NetTransportTCP* pTransportTCP = (NetTransportTCP*)((INetTransport*)spTransport);
    pTransportTCP->SendTo(sourceConnectionID, spEnvelopeMsg, strServerAddress, portServer);
}


//------------------------------------------------------------------------------------------------
void NetLib::SendAllRemote(IMessage* pMessageToSend, const Category& destCategory)
{
    EE_ASSERT(destCategory.IsValid());
    EnvelopeMessagePtr spEnvelopeMsg =
        EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_RemoteWrapper>;
    EE_ASSERT(spEnvelopeMsg);
    spEnvelopeMsg->SetChild(pMessageToSend);
    spEnvelopeMsg->SetDestinationCategory(destCategory);
    spEnvelopeMsg->SetSenderNetID(GetNetID());
    ForwardAllRemote(spEnvelopeMsg);
}

//------------------------------------------------------------------------------------------------
void NetLib::ForwardAllRemote(EnvelopeMessage* pEnvelopeMessage)
{
    for (RegisteredTransportMap::iterator transportPos = m_transportMap.begin();
        transportPos != m_transportMap.end();
        ++transportPos)
    {
        INetTransportPtr spTransport = transportPos->second;
        EE_ASSERT(NULL != spTransport);
        //DT32275 we need a way of sending even unreliable messages to all
        // unreliable connections don't get subscription messages
        if (spTransport->GetTransportType() & NET_RELIABLE)
        {
            bool retval = spTransport->Send(pEnvelopeMessage);

            // Check for a send error
            if (!retval)
            {
                EE_MESSAGE_TRACE(
                    pEnvelopeMessage,
                    efd::kNetMessage,
                    ILogger::kERR1,
                    ILogger::kLVL2,
                    ("%s| Sending payload %s to Transport %d FAILED!",
                    pEnvelopeMessage->GetDescription().c_str(),
                    pEnvelopeMessage->GetChildDescription().c_str(),
                    spTransport->GetTransportType()));
                // Check to see if an error occurred last time
                if (spTransport->GetLastResult())
                {
                    // Save the last status
                    spTransport->SetLastResult(retval);
                    NetworkStatusPtr spStatus =
                        EE_NEW MessageWrapper<NetworkStatus, efd::kMSGID_NetworkFailure>();
                    efd::QualityOfService qos = transportPos->first;
                    spStatus->Set(qos, m_netID);

                    // Insert new message at the end of the list (messages processed at tick)
                    NetMessagePtr spMessage =
                        EE_NEW EnvelopeMessage(spStatus, kCAT_NetEvent, m_netID);
                    ConnectionID localCID(QOS_LOCAL,0,0,0);
                    spMessage->SetSenderConnection(localCID);
                    m_spMessageList->push_back(spMessage);
                }
            }
            else
            {
                // Check to see if an error occurred last time
                if (!spTransport->GetLastResult())
                {
                    // Save the last status
                    spTransport->SetLastResult(retval);
                    NetworkStatusPtr spStatus =
                        EE_NEW MessageWrapper<NetworkStatus, efd::kMSGID_NetworkBufferOK>();
                    efd::QualityOfService qos = transportPos->first;
                    spStatus->Set(qos, m_netID);

                    // Insert new message at the end of the list (messages processed at tick)
                    NetMessagePtr spMessage =
                        EE_NEW EnvelopeMessage(spStatus, kCAT_NetEvent, m_netID);
                    ConnectionID localCID(QOS_LOCAL,0,0,0);
                    spMessage->SetSenderConnection(localCID);
                    m_spMessageList->push_back(spMessage);
                }
            }
        }
    }
}


//------------------------------------------------------------------------------------------------
void NetLib::RegisterTransport(INetTransport* pTrans, QualityOfService virtualQOS)
{
    QualityOfService qualityOfService = QOSCompare::LookupPhysical(virtualQOS);
    INetTransportPtr spTransport;
    EE_ASSERT(pTrans);
    pTrans->SetMessageCallback(this);
    if (m_transportMap.find(qualityOfService, spTransport) == true)
    {
        // There is already a transport registered for this qualityOfService
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR0,
            ("Error: Attempt to register a duplicate transport qualityOfService %d.  "
            "Duplicate registration ignored.",
            qualityOfService));

        // tick off another error occurred
        EE_LOG_METRIC_COUNT_FMT(kNetwork, ("TRANSPORT.ERROR.%u", virtualQOS));

        return;
    }
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
        ("New transport registered for %d.",
        qualityOfService));
    if (m_pISection)
    {
        pTrans->Configure(m_pISection);
    }
    m_transportMap[ qualityOfService ] = pTrans;
}


//------------------------------------------------------------------------------------------------
void NetLib::RegisterTransport(QualityOfService qualityOfService)
{
    INetTransportPtr spTransport;
    // create transport if necessary
    if (!m_transportMap.find(qualityOfService, spTransport))
    {
        spTransport = NetLib::CreateTransport(m_pMessageFactory, qualityOfService);
        RegisterTransport(spTransport,spTransport->GetTransportType());
    }
}

//------------------------------------------------------------------------------------------------
INetTransportPtr NetLib::CreateTransport(
    MessageFactory* pMessageFactory,
    QualityOfService qualityOfService)
{
    QualityOfService physicalQOS = QOSCompare::LookupPhysical(qualityOfService);
    INetTransportPtr spTransport;
    switch (physicalQOS)
    {
    case kQOS_TCPTotalOrder:
        spTransport = EE_NEW NetTransportTCP(pMessageFactory, kQOS_TCPTotalOrder);
        spTransport->OnInit();
        break;

    case kQOS_TCP:
        spTransport = EE_NEW NetTransportTCP(pMessageFactory, kQOS_TCP);
        spTransport->OnInit();
        break;

    case kQOS_Reliable:
        spTransport = EE_NEW NetTransportTCP(pMessageFactory, kQOS_Reliable);
        spTransport->OnInit();
        break;

    case kQOS_Unreliable:
        spTransport = EE_NEW NetTransportTCP(pMessageFactory, kQOS_Unreliable);
        spTransport->OnInit();
        break;

    case kQOS_UnreliableConnectionless:
        spTransport = EE_NEW NetTransportTCP(pMessageFactory, kQOS_UnreliableConnectionless);
        spTransport->OnInit();
        break;

    default:
        EE_FAIL_MESSAGE(
            ("Unknown physical QOS %d, virtual QOS %d!", physicalQOS, qualityOfService));
        break;
    }
    return spTransport;
}


//------------------------------------------------------------------------------------------------
void NetLib::UnregisterTransport(QualityOfService qualityOfService)
{
    INetTransportPtr   spTransport;
    if (m_transportMap.find(qualityOfService, spTransport) == false)
    {
        // There is no transport registered for this qualityOfService
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR0,
            ("Error: Attempt to unregister a transport (qualityOfService %d) that was never "
            "registered.  Unregister attempt ignored.",
            qualityOfService));

        // tick off another error occurred
        EE_LOG_METRIC_COUNT_FMT(kNetwork, ("TRANSPORT.ERROR.%u", qualityOfService));

        return;
    }
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
        ("Unregistering transport for qualityOfService %d.",
        qualityOfService));
    m_transportMap.erase(qualityOfService);
}


//------------------------------------------------------------------------------------------------
void NetLib::UnregisterTransport(INetTransport* pTrans)
{
    QualityOfService qualityOfService = QOS_INVALID;

    for (RegisteredTransportMap::iterator transportPos = m_transportMap.begin();
        transportPos != m_transportMap.end();
        ++transportPos)
    {
        // iterate through map of registered transports to find specified one...
        qualityOfService = transportPos->first;
        INetTransportPtr spTransport = transportPos->second;
        if (spTransport == pTrans)
        {
            // if here, we found a match...
            EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
                ("Unregistering transport for qualityOfService %d.",
                transportPos->first));
            m_transportMap.erase(transportPos);
            return;
        }
    }
    // if here, we have an error...  There is no transport registered for specified ptr
    EE_LOG(efd::kNetMessage, efd::ILogger::kERR0,
        ("Error: Attempt to unregister a transport (qualityOfService %d) that was never "
        "registered.  Unregister attempt ignored.",
        qualityOfService));

    // tick off another error occurred
    EE_LOG_METRIC_COUNT_FMT(kNetwork, ("TRANSPORT.ERROR.%u", pTrans->GetTransportType()));
}

//------------------------------------------------------------------------------------------------
IConnection* NetLib::GetTransportData(const ConnectionID& cid)
{
    EE_ASSERT(cid.m_qos != QOS_INVALID);
    INetTransportPtr spTransport;
    if (!m_transportMap.find(cid.m_qos,spTransport))
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("no transport found for %s",
            cid.ToString().c_str()));
        return NULL;
    }
    IConnection* pConnection = spTransport->GetTransportData(cid);
    return pConnection;
}


//------------------------------------------------------------------------------------------------
QualityOfService NetLib::GetTransport(const Category& destCategory)
{
    EE_ASSERT(destCategory.IsValid());
    QualityOfService foundQOS;
    if (m_catToQualityOfServiceMap.find(destCategory, foundQOS) == true)
    {
        EE_ASSERT(foundQOS != QOS_INVALID);
        return foundQOS;
    }
    else
    {
        EE_LOG(efd::kNetMessage, ILogger::kLVL3,
            ("%s> Category %s not found!",
            __FUNCTION__,
            destCategory.ToString().c_str()));
        return QOS_INVALID;
    }
}

//------------------------------------------------------------------------------------------------
QualityOfService NetLib::SafeGetTransport(const Category& destCategory)
{
    if (!destCategory.IsValid())
    {
        return QOS_INVALID;
    }

    return GetTransport(destCategory);
}

//------------------------------------------------------------------------------------------------
void NetLib::CloseConnection(const ConnectionID& cid)
{
    EE_ASSERT(cid.m_qos != QOS_INVALID);
    INetTransportPtr spTransport;
    m_transportMap.find(cid.GetQualityOfService(),spTransport);
    EE_ASSERT(spTransport);
    spTransport->CloseConnection(cid);
}


//------------------------------------------------------------------------------------------------
void NetLib::MapCategoryToQualityOfService(
    const Category& destCategory,
    QualityOfService virtualQOS)
{
    QualityOfService qualityOfService = QOSCompare::LookupPhysical(virtualQOS);
    EE_ASSERT(qualityOfService != kQOS_Invalid);
    QualityOfService foundQOS;
    if (m_catToQualityOfServiceMap.find(destCategory, foundQOS) == true)
    {
        if (foundQOS == qualityOfService)
        {
            EE_LOG(efd::kCategoryTrace, efd::ILogger::kLVL3,
                ("%s: category already registered for QOS %d",
                destCategory.ToString().c_str(), qualityOfService));
        }
        else
        {
            EE_FAIL_MESSAGE(("%s: Attempt to re-register a category for a different QOS %d, "
                "previous QOS %d.  "
                "Duplicate registration ignored.",
                destCategory.ToString().c_str(), qualityOfService, foundQOS));
        }

        // tick off another error occurred
        EE_LOG_METRIC_COUNT_FMT(kNetwork, ("TRANSPORT.ERROR.%u", virtualQOS));

        return;
    }

    EE_LOG(efd::kCategoryTrace, efd::ILogger::kLVL2,
        ("%s %s QOS %d",
        __FUNCTION__,
        destCategory.ToString().c_str(),
        qualityOfService));
    m_catToQualityOfServiceMap[ destCategory ] = qualityOfService;

    RegisterTransport(qualityOfService);
}

//------------------------------------------------------------------------------------------------
void NetLib::BeginCategoryProduction(
    const Category& destCategory,
    QualityOfService virtualQOS)
{
    MapCategoryToQualityOfService(destCategory, virtualQOS);
    m_categoryProductionSet.insert(destCategory);
}

//------------------------------------------------------------------------------------------------
void NetLib::EndCategoryProduction(const Category& destCategory)
{
    CategorySet::iterator it = m_categoryProductionSet.find(destCategory);
    if (it != m_categoryProductionSet.end())
    {
        EE_LOG(efd::kCategoryTrace, efd::ILogger::kLVL2,
            ("%s: EndCategoryProduction",
            destCategory.ToString().c_str()));

        m_categoryProductionSet.erase(it);
    }
    else
    {
        EE_LOG(efd::kCategoryTrace, efd::ILogger::kERR2,
            ("%s: Attempt to end production category ignored, category not produced",
            destCategory.ToString().c_str()));

        return;
    }
}


//------------------------------------------------------------------------------------------------
efd::AsyncResult NetLib::Tick()
{
    DeliverQueuedMessages();

    // Call OnTick for all registered transports
    for (RegisteredTransportMap::iterator transportPos = m_transportMap.begin();
        transportPos != m_transportMap.end();
        ++transportPos)
    {
        INetTransportPtr spTransport = transportPos->second;
        EE_ASSERT(NULL != spTransport);
        spTransport->OnTick();
    }
    return efd::AsyncResult_Pending;
}


//------------------------------------------------------------------------------------------------
void NetLib::RegisterEventHandler(
    efd::UInt32 eventType,
    INetCallback* pCallback,
    const ConnectionID& cid,
    bool consume)
{
    IConnection* pConnection = GetTransportData(cid);
    if (!pConnection && (cid.GetQualityOfService() & NET_CONNECTIONLESS))
    {
        ConnectionID realConnectionID =
            ConnectionID(cid.GetQualityOfService(),0,cid.GetLocalPort(),0);
        pConnection = GetTransportData(realConnectionID);
    }
    EE_ASSERT(pConnection);
    switch (eventType)
    {
    case kMSGID_ConnectionDataReceivedMsg:
        pConnection->SetMessageCallback(pCallback);
        break;

    case kMSGID_ConnectionAcceptedMsg:
    case kMSGID_ConnectionConnectedMsg:
    case kMSGID_ConnectionDisconnectedMsg:
    case kMSGID_ConnectionFailedToConnectMsg:
        pConnection->SetConnectionCallback(pCallback);
        break;
    }

    EE_UNUSED_ARG(consume);
}


//------------------------------------------------------------------------------------------------
void NetLib::UnregisterEventHandler(
    efd::UInt32 eventType,
    INetCallback* pCallback,
    const ConnectionID& cid)
{
    IConnection* pConnection = GetTransportData(cid);
    if (!pConnection)
        return;
    switch (eventType)
    {
    case kMSGID_ConnectionDataReceivedMsg:
        if (pConnection->GetConnectionCallback() == pCallback)
        {
            pConnection->SetMessageCallback(this);
        }
        break;

    case kMSGID_ConnectionAcceptedMsg:
    case kMSGID_ConnectionConnectedMsg:
    case kMSGID_ConnectionDisconnectedMsg:
    case kMSGID_ConnectionFailedToConnectMsg:
        if (pConnection->GetConnectionCallback() == pCallback)
        {
            pConnection->SetConnectionCallback(this);
        }
        break;
    }
}


//------------------------------------------------------------------------------------------------
ConnectionID NetLib::Connect(
    const efd::utf8string& ipaddress, efd::UInt16 port,
    QualityOfService virtualQOS,
    INetCallback* callback)
{
    QualityOfService qualityOfService = QOSCompare::LookupPhysical(virtualQOS);
    // make sure the QOS we are using exists
    RegisterTransport(qualityOfService);
    INetTransportPtr spTransport;
    if (!m_transportMap.find(qualityOfService, spTransport))
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR0,
            ("Error: Failed to create/find transport for QOS %d.  Attempt ignored.",
            qualityOfService));
        return kCID_INVALID;
    }

    return spTransport->Connect(ipaddress, port, callback);

}


//------------------------------------------------------------------------------------------------
efd::ConnectionID NetLib::Listen(
    const efd::utf8string& ipaddress,
    efd::UInt16 port,
    QualityOfService virtualQOS,
    INetCallback* callback)
{
    QualityOfService qualityOfService = QOSCompare::LookupPhysical(virtualQOS);
    //HostInfo hostInfo(ip, ADDRESS);
    // make sure the QOS we are using exists
    RegisterTransport(qualityOfService);
    INetTransportPtr spTransport;
    if (!m_transportMap.find(qualityOfService, spTransport))
    {
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR0,
            ("Error: Failed to create/find transport for QOS %d.  Attempt ignored.",
            qualityOfService));
        return kCID_INVALID;
    }
    EE_UNUSED_ARG(ipaddress);
    //DT32310 Bind/Listen needs to support binding to a specific ip address.
    return spTransport->Listen(port, callback);
}


//------------------------------------------------------------------------------------------------
efd::ConnectionID NetLib::Listen(
    efd::UInt16 port,
    QualityOfService virtualQOS,
    INetCallback* callback)
{
    QualityOfService qualityOfService = QOSCompare::LookupPhysical(virtualQOS);
    //DT32310 Bind/Listen needs to support binding to a specific ip address.
    return Listen("0.0.0.0", port, qualityOfService, callback);
}


//------------------------------------------------------------------------------------------------
void NetLib::HandleNetMessage(
    const IMessage* pIncomingMessage,
    const ConnectionID& senderPrivateCat)
{
    NetMessage* pNetMessage = EE_DYNAMIC_CAST(NetMessage, const_cast<IMessage*>(pIncomingMessage));
    QueueMessage(pNetMessage);

    EE_UNUSED_ARG(senderPrivateCat);
}

//------------------------------------------------------------------------------------------------
void NetLib::ProducerAssociate(const ConnectionID& cid, const Category& categoryProduced)
{
    ConnectionIDSetPtr spConnectionIDSet = NULL;
    if (m_producerMap.find(categoryProduced, spConnectionIDSet) == false)
    {
        spConnectionIDSet = EE_NEW ConnectionIDSet();
        EE_ASSERT(spConnectionIDSet);
        m_producerMap[ categoryProduced ] = spConnectionIDSet;
    }
    EE_ASSERT(spConnectionIDSet);
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
        ("ProducerAssociate: %s Associated with %s",
        categoryProduced.ToString().c_str(),
        cid.ToString().c_str()));
    spConnectionIDSet->insert(cid);
}

//------------------------------------------------------------------------------------------------
void NetLib::ProducerDeassociate(const ConnectionID& cid, const Category& categoryProduced)
{
    ConnectionIDSetPtr spConnectionIDSet = NULL;
    if (m_producerMap.find(categoryProduced, spConnectionIDSet) == false)
    {
        // no producers of category found
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("ProducerDeassociate: No producers for %s found",
            categoryProduced.ToString().c_str()));
        return;
    }
    EE_ASSERT(spConnectionIDSet);
    ConnectionIDSet::iterator foundIt = spConnectionIDSet->find(cid);
    if (foundIt == spConnectionIDSet->end())
    {
        // no producers of category found
        EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
            ("ProducerDeassociate: %s not associated with %s",
            cid.ToString().c_str(),
            categoryProduced.ToString().c_str()));
        return;
    }
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
        ("ProducerDeassociate: %s Deassociated from %s",
        categoryProduced.ToString().c_str(),
        cid.ToString().c_str()));
    spConnectionIDSet->erase(foundIt);
}

//------------------------------------------------------------------------------------------------
void NetLib::ProducerForward(EnvelopeMessage* pMessageToSend, const Category& categoryProduced)
{
    ConnectionIDSetPtr spConnectionIDSet = NULL;
    if (m_producerMap.find(categoryProduced, spConnectionIDSet) == false)
    {
        // no producers of category found
        EE_MESSAGE_TRACE(
            pMessageToSend,
            efd::kNetMessage,
            ILogger::kERR1,
            ILogger::kLVL2,
            ("ProducerSend Sending %s: No producers for %s found",
            pMessageToSend->GetDescription().c_str(),
            categoryProduced.ToString().c_str()));
        return;
    }
    EE_ASSERT(spConnectionIDSet);
    for (ConnectionIDSet::iterator it = spConnectionIDSet->begin();
          it != spConnectionIDSet->end();
          ++it)
    {
        // send message to each producer in list
        EE_MESSAGE_TRACE(
            pMessageToSend,
            efd::kNetMessage,
            ILogger::kLVL1,
            ILogger::kLVL2,
            ("ProducerSend: Sending %s to %s on %s",
            pMessageToSend->GetDescription().c_str(),
            categoryProduced.ToString().c_str(),
            (*it).ToString().c_str()));
        Forward(pMessageToSend, *it);
    }
}

//------------------------------------------------------------------------------------------------
void NetLib::ProducerSendRemote(IMessage* pMessageToSend, const Category& categoryProduced)
{
    EnvelopeMessagePtr spInnerEnvelopeMessage =
        EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_SendToProducer>;
    EE_ASSERT(spInnerEnvelopeMessage);
    spInnerEnvelopeMessage->SetChild(pMessageToSend);
    spInnerEnvelopeMessage->SetDestinationCategory(categoryProduced);

    EnvelopeMessagePtr spEnvelopeMessage =
        EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_RemoteWrapper>;
    EE_ASSERT(spEnvelopeMessage);
    spEnvelopeMessage->SetChild(spInnerEnvelopeMessage);
    spEnvelopeMessage->SetDestinationCategory(kCAT_SendToProducer);

    ProducerForward(spEnvelopeMessage, categoryProduced);
}


//------------------------------------------------------------------------------------------------
void NetLib::CloseAllConnections()
{
    for (RegisteredTransportMap::iterator transportPos = m_transportMap.begin();
        transportPos != m_transportMap.end();
        ++transportPos)
    {
        transportPos->second->CloseAllConnections();
    }
}


//------------------------------------------------------------------------------------------------
efd::UInt32 NetLib::QueryOutgoingQueueSize(efd::ConnectionID cid)
{
    efd::UInt32 queueSize = 0;
    if (cid == kCID_INVALID)
    {
        for (RegisteredTransportMap::iterator transportPos = m_transportMap.begin();
            transportPos != m_transportMap.end();
            ++transportPos)
        {
            queueSize += transportPos->second->QueryOutgoingQueueSize();
        }
    }
    else
    {
        IConnection* pIConnection = GetTransportData(cid);
        if (pIConnection)
        {
            queueSize = pIConnection->QueryOutgoingQueueSize();
        }
    }
    return queueSize;
}

//------------------------------------------------------------------------------------------------
void NetLib::InternalRemoveCallback(INetCallback* pTarget)
{
    CategoryToINetCallbackMap::iterator it = m_localListeners.begin();
    while (it != m_localListeners.end())
    {
        INetCallbackSetPtr spCallbackSet = (*it).second;
        EE_ASSERT(spCallbackSet);
        INetCallbackSet::iterator callbackSetIter = spCallbackSet->begin();
        while (callbackSetIter != spCallbackSet->end())
        {
            if ((*callbackSetIter) == pTarget)
            {
                EE_LOG(efd::kNetMessage, efd::ILogger::kERR2,
                    ("Warning! Callback 0x%08X still subscribed to %s",
                    pTarget,
                    (*it).first.ToString().c_str()));
                INetCallbackSet::iterator eraseIter = callbackSetIter++;
                spCallbackSet->erase(eraseIter);
            }
            else
            {
                ++callbackSetIter;
            }
        }
        if (spCallbackSet->empty())
        {
            CategoryToINetCallbackMap::iterator eraseIter = it++;
            m_localListeners.erase(eraseIter);
        }
        else
        {
            ++it;
        }
    }
}


//------------------------------------------------------------------------------------------------
efd::Bool NetLib::HaveLocalListeners()
{
    return (m_localListeners.size() > 0);
}

//------------------------------------------------------------------------------------------------
void NetLib::SendAddLocalConsumerRequests(const ConnectionID& cid)
{
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
        ("SendAddLocalConsumerRequests for %s",
        cid.ToString().c_str()));
    for (CategoryToINetCallbackMap::iterator callbackSetMapIter = m_localListeners.begin();
        callbackSetMapIter != m_localListeners.end();
        ++callbackSetMapIter)
    {
        EE_ASSERT((*callbackSetMapIter).second);
        Category catValue = (*callbackSetMapIter).first;
        StreamMessagePtr spStreamMsg =
            CreateLocalConsumerRequest<kMSGID_SubscribeExternal>(catValue);
        EE_LOG(efd::kNetMessage, efd::ILogger::kLVL2,
            ("SendAddLocalConsumerRequests for %s: %s",
            cid.ToString().c_str(),
            catValue.ToString().c_str()));
        SendRemote(spStreamMsg, kCAT_NetSystem, cid);
    }
}


//------------------------------------------------------------------------------------------------
template<efd::UInt32 msgguid>
StreamMessagePtr NetLib::CreateLocalConsumerRequest(const Category& category)
{
    StreamMessagePtr spStreamMsg = EE_NEW MessageWrapper< StreamMessage, msgguid >;
    *spStreamMsg << UInt8(1);   // Add the number of categories
    *spStreamMsg << category;
    // don't send messages before we have been assigned a NetID
    EE_ASSERT(GetNetID() != kNetID_Unassigned);
    return spStreamMsg;
}


//------------------------------------------------------------------------------------------------
void NetLib::SendAddLocalConsumerRequest(const Category& category)
{
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL3,
        ("Sending Subscribe messages for %s",
        category.ToString().c_str()));

    StreamMessagePtr spStreamMsg =
        CreateLocalConsumerRequest< kMSGID_SubscribeExternal>(category);
    SendAllRemote(spStreamMsg, kCAT_NetSystem);
}


//------------------------------------------------------------------------------------------------
void NetLib::SendAddLocalConsumerRequest(const Category& category, ConnectionID cid)
{
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL3,
        ("Sending Subscribe messages for %s to %s",
        category.ToString().c_str(),
        cid.ToString().c_str()));

    StreamMessagePtr spStreamMsg =
        CreateLocalConsumerRequest< kMSGID_SubscribeExternal>(category);
    SendRemote(spStreamMsg, kCAT_NetSystem, cid);
}


//------------------------------------------------------------------------------------------------
void NetLib::SendRemoveLocalConsumerRequest(const Category& category)
{
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL3,
        ("Sending Unsubscribe messages for %s",
        category.ToString().c_str()));

    StreamMessagePtr spStreamMsg =
        CreateLocalConsumerRequest< kMSGID_UnsubscribeExternal >(category);
    SendAllRemote(spStreamMsg, kCAT_NetSystem);
}


//------------------------------------------------------------------------------------------------
void NetLib::SendRemoveLocalConsumerRequest(
    const Category& category,
    ConnectionID cid)
{
    EE_LOG(efd::kNetMessage, efd::ILogger::kLVL3,
        ("Sending Unsubscribe messages for %s to %s",
        category.ToString().c_str(),
        cid.ToString().c_str()));

    StreamMessagePtr spStreamMsg =
        CreateLocalConsumerRequest< kMSGID_UnsubscribeExternal>(category);
    SendRemote(spStreamMsg, kCAT_NetSystem, cid);
}


//------------------------------------------------------------------------------------------------
template<efd::UInt32 msgguid>
StreamMessagePtr NetLib::CreateCategoryProductionMessage(
    const Category& category,
    efd::QualityOfService qualityOfService)
{
    EE_ASSERT(category.IsValid());
    StreamMessagePtr spStreamMsg = EE_NEW MessageWrapper< StreamMessage, msgguid >;
    *spStreamMsg << category;
    *spStreamMsg << qualityOfService;
    return spStreamMsg;
}


//------------------------------------------------------------------------------------------------
void NetLib::SendBeginCategoryProduction(
    const Category& categoryToProduce,
    efd::QualityOfService qualityOfService)
{
    StreamMessagePtr spStreamMsg =
        CreateCategoryProductionMessage< kMSGID_BeginCategoryProduction>(
            categoryToProduce,
            qualityOfService);
    SendAllRemote(spStreamMsg, kCAT_NetSystem);
}


//------------------------------------------------------------------------------------------------
void NetLib::SendEndCategoryProduction(const Category& categoryToProduce)
{
    efd::QualityOfService qualityOfService = GetTransport(categoryToProduce);
    StreamMessagePtr spStreamMsg =
        CreateCategoryProductionMessage< kMSGID_EndCategoryProduction >(
            categoryToProduce,
            qualityOfService);
    SendAllRemote(spStreamMsg, kCAT_NetSystem);
}

//------------------------------------------------------------------------------------------------
void NetLib::SendCategoryProductionMessages(const ConnectionID& cid)
{
    // for every category we have, send a production message to the targeted connection
    for (CategorySet::iterator catPos = m_categoryProductionSet.begin();
        catPos != m_categoryProductionSet.end();
        ++catPos)
    {
        efd::Category catValue = (*catPos);
        QualityOfService qualityOfService = GetTransport(catValue);

        StreamMessagePtr spMsg =
            CreateCategoryProductionMessage< kMSGID_BeginCategoryProduction >(
            catValue,
            qualityOfService);
        SendRemote(spMsg, kCAT_NetSystem, cid);
    }
}

//------------------------------------------------------------------------------------------------
efd::utf8string NetLib::IPToString(efd::UInt32 ipAddress)
{
    return HostInfo::IPToString(ipAddress);
}
