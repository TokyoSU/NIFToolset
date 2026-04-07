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

#include <efdNetwork/BridgeService.h>
#include <efdNetwork/ChannelManagerService.h>
#include <efdNetwork/ChannelManagerConfig.h>
#include <efd/NetMessage.h>
#include <efd/ConfigManager.h>
#include <efd/ServiceManager.h>
#include <efdNetwork/NetLib.h>

using namespace efd;
namespace efd
{

class BridgeProxy : public efd::MemObject, public efd::INetCallback
{
public:
    BridgeProxy(BridgeService* pBridgeService)
        : m_pBridgeService(pBridgeService)
    {
    }
    virtual void HandleNetMessage(
        const IMessage* pIncomingMessage,
        const ConnectionID& senderConnectionID)
    {
        EE_ASSERT(m_pBridgeService);
        m_pBridgeService->HandleInNetMessage(pIncomingMessage, senderConnectionID);
    }
protected:
    BridgeService* m_pBridgeService;
};

}

EE_IMPLEMENT_CONCRETE_CLASS_INFO(BridgeService);

const /*static*/ char* BridgeService::kConfigSection = "BridgeService";

//------------------------------------------------------------------------------------------------
BridgeService::BridgeService(const efd::utf8string& netLibType, const efd::utf8string& inNetLibType)
    : NetService(netLibType)
    , m_inReady(false)
    , m_outReady(false)
    , m_inPort(12210)
    , m_inNetLibType(inNetLibType)
    , m_readInNetLibConfig(true)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 5700;

    m_pInBridgeProxy = EE_NEW BridgeProxy(this);
    if (!m_inNetLibType.empty())
    {
        m_readInNetLibConfig = false;
    }
    else
    {
        m_inNetLibType = NetService::kNetLib;
    }
    AutoReconnectToChannelManager(false);
}

//------------------------------------------------------------------------------------------------
BridgeService::~BridgeService()
{
    EE_DELETE m_pInBridgeProxy;
}

//------------------------------------------------------------------------------------------------
const char* BridgeService::GetDisplayName() const
{
    return "BridgeService";
}

//------------------------------------------------------------------------------------------------
efd::SyncResult BridgeService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    EE_ASSERT(m_pServiceManager);

    efd::SyncResult retVal = NetService::OnPreInit(pDependencyRegistrar);
    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    if (pConfigManager)
    {
        const ISection *pSection =
            pConfigManager->GetConfiguration()->FindSection(BridgeService::kConfigSection);

        if (pSection)
        {
            if (m_readInNetLibConfig)
            {
                efd::utf8string netLibType = pSection->FindValue(NetService::kNetLibType);
                if (!netLibType.empty())
                {
                    m_inNetLibType = netLibType;
                }
            }
            efd::utf8string temp = pSection->FindValue(ChannelManagerService::kPort);
            if (!temp.empty())
            {
                m_inPort = (efd::UInt16)atoi(temp.c_str());
            }
        }
    }

    EE_ASSERT(m_spMessageService);
    const ISection* pConfigSection = pConfigManager->GetConfiguration();
    m_spInNetLib = INetLib::CreateNetLib(m_inNetLibType, m_spMessageService, pConfigSection);

    return retVal;
}

//------------------------------------------------------------------------------------------------
void BridgeService::NotifyMessageService()
{
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult BridgeService::OnTick()
{
    if (!m_spInNetLib)
    {
        return efd::AsyncResult_Complete;
    }
    // deliver any queued incoming messages
    if (m_inReady && m_inMessageQueue.size())
    {
        efd::list<EnvelopeMessagePtr>::iterator it = m_inMessageQueue.begin();
        for (;it != m_inMessageQueue.end(); ++it)
        {
            EnvelopeMessagePtr spEnvelopeMessage = (*it);
            m_spInNetLib->ForwardAllRemote(spEnvelopeMessage);
        }
        m_inMessageQueue.clear();
    }

    // deliver any queued outgoing messages
    if (m_outReady && m_outMessageQueue.size())
    {
        efd::list<EnvelopeMessagePtr>::iterator it = m_outMessageQueue.begin();
        for (;it != m_outMessageQueue.end(); ++it)
        {
            EnvelopeMessagePtr spEnvelopeMessage = (*it);
            m_spNetLib->ForwardAllRemote(spEnvelopeMessage);
        }
        m_outMessageQueue.clear();
    }

    // attempt to listen if we are not already
    if (m_inListenCID == kCID_INVALID)
    {
        m_inListenCID = m_spInNetLib->Listen(m_inPort, QOS_RELIABLE, m_pInBridgeProxy);
        if (m_inListenCID != kCID_INVALID)
        {
            EE_LOG(efd::kChannelManager, efd::ILogger::kLVL2,
                ("%s> Listening for QOS %d connections on port %d",
                __FUNCTION__,
                QOS_RELIABLE,
                m_inPort));
        }
    }

    NetService::OnTick();
    m_spInNetLib->Tick();
    return efd::AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
void BridgeService::HandleInNetMessage(
    const IMessage* pIncomingMessage,
    const ConnectionID& senderConnectionID)
{
    //check to see if this is a NetMessage w/o an envelope
    switch (pIncomingMessage->GetClassID())
    {
    case kMSGID_ConnectionAcceptedMsg:
        m_inReady = true;
        m_outMessageQueue.clear();
        m_spInNetLib->RegisterEventHandler(
            kMSGID_ConnectionDataReceivedMsg,
            m_pInBridgeProxy,
            senderConnectionID);
        m_connectionStatus = kCS_Failed;
        AutoReconnectToChannelManager(true);
        ConnectToDefaultChannelManager();
        break;
    case kMSGID_ConnectionConnectedMsg:
        {
            EE_FAIL("This case should never happen!  HandleInNetMessage does not Connect.");
        }
        break;

    case kMSGID_ConnectionDisconnectedMsg:
        {
            m_inReady = false;
            m_outReady = false;
            m_spInNetLib->UnregisterEventHandler(
                kMSGID_ConnectionDataReceivedMsg,
                m_pInBridgeProxy,
                senderConnectionID);
            DisconnectFromChannelManager();
            AutoReconnectToChannelManager(false);
            m_spNetLib->CloseAllConnections();
        }
        break;

    case kMSGID_ConnectionFailedToAcceptMsg:
        {
            EE_LOG(efd::kMessage, efd::ILogger::kERR2,
                ("%s> kMSGID_ConnectionFailedToAcceptMsg", __FUNCTION__));
        }
        break;

    case kMSGID_ConnectionFailedToConnectMsg:
        {
            EE_FAIL("This case should never happen!  HandleInNetMessage does not Connect.");
        }
        break;

    case kMSGID_ConnectionDataReceivedMsg:
        const EnvelopeMessage* pConstMessage =
            EE_DYNAMIC_CAST(const EnvelopeMessage, pIncomingMessage);
        EnvelopeMessage* pEnvelopeMessage = const_cast<EnvelopeMessage*>(pConstMessage);
        if (m_outReady)
        {
            m_spNetLib->ForwardAllRemote(pEnvelopeMessage);
        }
        else
        {
            /// hopefully these messages will only be subscription messages that get forwarded on
            /// once the other end of the connection is established
            m_outMessageQueue.push_back(pEnvelopeMessage);
        }
        break;
    }
}

//------------------------------------------------------------------------------------------------
void BridgeService::HandleNetMessage(
    const IMessage* pIncomingMessage,
    const ConnectionID& senderConnectionID)
{
    //check to see if this is a NetMessage w/o an envelope
    switch (pIncomingMessage->GetClassID())
    {
    case kMSGID_ConnectionFailedToAcceptMsg:
        {
            EE_FAIL_MESSAGE(("This case should never happen!  %s does not Listen.",__FUNCTION__));
        }
        break;

    case kMSGID_ConnectionFailedToConnectMsg:
        {
            EE_LOG(efd::kMessage, efd::ILogger::kERR2,
                ("%s> kMSGID_ConnectionFailedToAcceptMsg", __FUNCTION__));
        }
        break;
    case kMSGID_ConnectionDataReceivedMsg:
        {
            const EnvelopeMessage* pConstMessage =
                EE_DYNAMIC_CAST(const EnvelopeMessage, pIncomingMessage);
            EnvelopeMessage* pEnvelopeMessage = const_cast<EnvelopeMessage*>(pConstMessage);
            if (m_inReady)
            {
                m_spInNetLib->ForwardAllRemote(pEnvelopeMessage);
            }
            else
            {
                /// hopefully these messages will only be subscription messages that get forwarded
                /// on once the other end of the connection is established
                m_inMessageQueue.push_back(pEnvelopeMessage);
            }
        }
        // we don't want this message to be handled by NetService
        return;
    case kMSGID_ConnectionConnectedMsg:
        m_outReady = true;
        m_inMessageQueue.clear();
        m_spNetLib->RegisterEventHandler(
            kMSGID_ConnectionDataReceivedMsg,
            this,
            senderConnectionID);
        AutoReconnectToChannelManager(false);
        break;
    case kMSGID_ConnectionDisconnectedMsg:
        //m_inReady = false;
        m_outReady = false;
        m_spNetLib->UnregisterEventHandler(
            kMSGID_ConnectionDataReceivedMsg,
            this,
            senderConnectionID);
        break;
    default:
        EE_LOG(
            efd::kMessage,
            efd::ILogger::kERR2,
            ("%s> received unhandled message 0x%08X",
            __FUNCTION__,
            pIncomingMessage->GetClassID()));
        break;
    }
    NetService::HandleNetMessage(pIncomingMessage,senderConnectionID);
}

