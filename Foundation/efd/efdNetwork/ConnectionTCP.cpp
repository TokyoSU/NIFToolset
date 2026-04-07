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

#include <efdNetwork/ConnectionTCP.h>
#include <efdNetwork/HostInfo.h>
#include <efd/StreamMessage.h>
#include <efdNetwork/Net.h>
#include <efd/IConfigSection.h>
#include <efdNetwork/NetVersion.h>

//------------------------------------------------------------------------------------------------
using namespace efd;

// NOTE: If you change kTCPVersionValue you should update the same/similar value in ConnectionUSB
// and ConnectionEnet.
static const char* kTCPVersionName = "NetTransportTCP";
static const char* kTCPVersionValue = "3.2.1";
static const char* kNetTransport = "NetTransport";
static const char* kMaxSendMessages = "MaxSendMessages";
static const char* kMaxSendBytes = "MaxSendBytes";
static const efd::UInt32 kMaxSendMessagesDefault = 64;
static const efd::UInt32 kMaxSendBytesDefault = 8192;
static const efd::UInt32 kMaxConnectRetries = 600; // 10 seconds at 60hz

//------------------------------------------------------------------------------------------------
ConnectionTCP::ConnectionTCP()
    : m_spSocket(NULL)
    , m_versionsValid(false)
    , m_pMessageCallback(NULL)
    , m_pConnectionCallback(NULL)
    , m_versionsSent(false)
    , m_connectionState(EE_CONN_NOT_CONNECTED)
    , m_isIncomingConnection(false)
    , m_qos(kQOS_Invalid)
    , m_pMessageFactory(NULL)
    , m_isListening(false)
    , m_retryCount(0)
    , m_maxSendMessages(kMaxSendMessagesDefault)
    , m_maxSendBytes(kMaxSendBytesDefault)
{
}

//------------------------------------------------------------------------------------------------
ConnectionTCP::ConnectionTCP(const ConnectionTCP& rhs, ConnectionID remoteConnectionID)
    : m_spSocket(rhs.m_spSocket)
    , m_versionsValid(false)
    , m_pMessageCallback(rhs.m_pMessageCallback)
    , m_pConnectionCallback(rhs.m_pConnectionCallback)
    , m_versionsSent(false)
    , m_connectionState(EE_CONN_ACCEPTED)
    , m_isIncomingConnection(true)
    , m_qos(rhs.m_qos)
    , m_pMessageFactory(rhs.m_pMessageFactory)
    , m_remoteConnectionID(remoteConnectionID)
    , m_isListening(false)
    , m_retryCount(0)
    , m_maxSendMessages(kMaxSendMessagesDefault)
    , m_maxSendBytes(kMaxSendBytesDefault)
{
    EE_ASSERT(m_spSocket);
    EE_ASSERT(m_pMessageCallback);
    EE_ASSERT(m_pConnectionCallback);
    EE_ASSERT(m_remoteConnectionID != kCID_INVALID);
    EE_ASSERT(m_remoteConnectionID.GetIP() != 0);
    EE_ASSERT(m_remoteConnectionID.GetLocalPort() != 0);
    EE_ASSERT(m_remoteConnectionID.GetRemotePort() != 0);
}


//------------------------------------------------------------------------------------------------
ConnectionTCP::ConnectionTCP(
    MessageFactory* pMessageFactory,
    QualityOfService qos,
    Socket* pSocket,
    INetCallback* pMessageCallback,
    bool isIncomingConnection)
    : m_spSocket(pSocket)
    , m_versionsValid(false)
    , m_pMessageCallback(pMessageCallback)
    , m_pConnectionCallback(NULL)
    , m_versionsSent(false)
    , m_connectionState(EE_CONN_NOT_CONNECTED)
    , m_isIncomingConnection(isIncomingConnection)
    , m_qos(qos)
    , m_pMessageFactory(pMessageFactory)
    , m_isListening(true)
    , m_retryCount(0)
    , m_maxSendMessages(kMaxSendMessagesDefault)
    , m_maxSendBytes(kMaxSendBytesDefault)
{
    EE_ASSERT(m_spSocket);
    EE_ASSERT(pMessageCallback);
    m_pConnectionCallback = m_spSocket->GetCallback();
    EE_ASSERT(m_pConnectionCallback);
    m_remoteConnectionID = m_spSocket->GetConnectionID();
}

//------------------------------------------------------------------------------------------------
ConnectionTCP::ConnectionTCP(
    MessageFactory* pMessageFactory,
    QualityOfService qos,
    INetCallback* pMessageCallback,
    INetCallback* pConnectionCallback)
    : m_spSocket(NULL)
    , m_versionsValid(false)
    , m_pMessageCallback(pMessageCallback)
    , m_pConnectionCallback(pConnectionCallback)
    , m_versionsSent(false)
    , m_connectionState(EE_CONN_NOT_CONNECTED)
    , m_isIncomingConnection(false)
    , m_qos(qos)
    , m_pMessageFactory(pMessageFactory)
    , m_isListening(false)
    , m_retryCount(0)
    , m_maxSendMessages(kMaxSendMessagesDefault)
    , m_maxSendBytes(kMaxSendBytesDefault)
{
    m_spSocket = EE_NEW TCPSocket(m_qos, 0,pConnectionCallback);
    EE_ASSERT(m_spSocket);
    EE_ASSERT(pMessageCallback);
    EE_ASSERT(pConnectionCallback);
}

//------------------------------------------------------------------------------------------------
ConnectionTCP::~ConnectionTCP()
{
    Close();
    m_spSocket = NULL;
}

//------------------------------------------------------------------------------------------------
void ConnectionTCP::Configure(ISection* pISection)
{
    if (!pISection)
    {
        return;
    }

    ISection* pNetTransportSection = pISection->FindSection(kNetTransport);
    if (pNetTransportSection)
    {
        efd::utf8string temp = pNetTransportSection->FindValue(kMaxSendMessages);
        if (!temp.empty())
        {
            m_maxSendMessages = (efd::UInt32)atoi(temp.c_str());
        }
        temp = pNetTransportSection->FindValue(kMaxSendBytes);
        if (!temp.empty())
        {
            m_maxSendBytes = (efd::UInt32)atoi(temp.c_str());
        }
    }
}

//------------------------------------------------------------------------------------------------
bool ConnectionTCP::IsValid()
{
    return m_versionsValid && m_spSocket;
}


//------------------------------------------------------------------------------------------------
bool ConnectionTCP::VersionChecked()
{
    return m_versionsValid;
}


//------------------------------------------------------------------------------------------------
bool ConnectionTCP::ValidateVersions(Archive& ar, ConnectionID& o_versionConnectionID)
{
    NetVersion version;
    Serializer::SerializeObject(version, ar);

    NetVersion expected;
    expected.AddVersionString(kTCPVersionName);
    expected.AddVersionString(kTCPVersionValue);

    if (version == expected)
    {
        m_versionsValid = true;
        if (m_qos & NET_UDP)
        {
            bool needResponse = false;
            ConnectionID remoteConnectionID;
            Serializer::SerializeObject(needResponse, ar);
            Serializer::SerializeObject(remoteConnectionID, ar);

            if (needResponse)
            {
                o_versionConnectionID = remoteConnectionID;
            }
            else
            {
                remoteConnectionID = GetRemoteConnectionID();
            }
            m_connectionState = EE_CONN_CONNECTED;
            if (needResponse)
            {
                // the other side still needs a response, so we send one
                SendVersions(false, remoteConnectionID);
            }
        }
        return true;
    }

    EE_LOG(efd::kNetMessage,
        efd::ILogger::kERR1,
        ("Error: Failed to validate version numbers on connection %s.   Connection Closing.",
        GetRemoteConnectionID().ToString().c_str()));

    EE_LOG_METRIC_COUNT_FMT(kNetwork, ("RECEIVE.ERROR.%u.0x%llX",
        remoteConnectionID.GetQualityOfService(),
        remoteConnectionID.GetValue()));

    return false;
}

//------------------------------------------------------------------------------------------------
efd::SInt32 ConnectionTCP::SendVersions(
    bool needResponse,
    const ConnectionID& responseConnectionID)
{
    EE_ASSERT(m_spSocket);

    NetVersion expected;
    expected.AddVersionString(kTCPVersionName);
    expected.AddVersionString(kTCPVersionValue);

    /// UDP can only have whole messages sent across
    if (m_qos & NET_UDP)
    {
        efd::StreamMessagePtr spMessage =
            EE_NEW MessageWrapper<StreamMessage, kMSGID_UnreliableVersion>;
        Archive& ar = spMessage->GetArchive();
        Serializer::SerializeObject(expected, ar);
        Serializer::SerializeObject(needResponse, ar);
        Serializer::SerializeConstObject(responseConnectionID, ar);

        efd::EnvelopeMessagePtr spEnvelopeMessage =
            EE_NEW MessageWrapper< EnvelopeMessage, kMSGID_ConnectionDataReceivedMsg>;
        spEnvelopeMessage->SetDestinationCategory(kCAT_NetSystem);
        spEnvelopeMessage->SetQualityOfService(GetTransportType());
        spEnvelopeMessage->SetChild(spMessage);

        if (Send(spEnvelopeMessage, m_remoteConnectionID))
        {
            m_versionsSent = true;
            return spMessage->GetBufferSize();
        }
        else
        {
            return SetLastError(EE_SOCKET_ERROR_UNKNOWN);
        }
    }
    else
    {
        if (m_sendVersionBuffer.GetSize() == 0)
        {
            Archive ar;
            Serializer::SerializeObject(expected, ar);
            m_sendVersionBuffer = ar.GetUsedBuffer();
        }

        // if here, have a valid net data object, go ahead and send it...
        // Shove it through the TCP Socket
        efd::SInt32 numBytes = m_spSocket->Send(m_sendVersionBuffer);
        if (numBytes > 0)
            m_versionsSent = true;
        return numBytes;
    }
}

//------------------------------------------------------------------------------------------------
efd::SInt32 ConnectionTCP::ReceiveVersions(
    efd::EnvelopeMessage* pEnvelopeMessage,
    ConnectionID& versionConnectionID)
{
    EE_ASSERT(pEnvelopeMessage);
    const IMessage* pMessage = pEnvelopeMessage->GetContents(m_pMessageFactory);
    if (!pMessage)
    {
        return SetLastError(EE_SOCKET_ERROR_UNKNOWN);
    }

    efd::StreamMessagePtr spStreamMessage =
        EE_DYNAMIC_CAST(efd::StreamMessage, const_cast<IMessage*>(pMessage));

    if (!spStreamMessage)
    {
        return SetLastError(EE_SOCKET_ERROR_UNKNOWN);
    }

    EE_ASSERT(spStreamMessage->GetClassID() == kMSGID_UnreliableVersion);

    spStreamMessage->ResetForUnpacking();
    if (!ValidateVersions(spStreamMessage->GetArchive(), versionConnectionID))
    {
        return SetLastError(EE_SOCKET_ERROR_UNKNOWN);
    }
    return spStreamMessage->GetBufferSize();
}


//------------------------------------------------------------------------------------------------
efd::SInt32 ConnectionTCP::ReceiveVersions(ConnectionID& senderConnectionID)
{
    /// UDP can only have whole messages sent across
    if (m_qos & NET_UDP)
    {
        static UInt32 tickCount = 0;
        if (tickCount%5 == 0)
        {
            // we are unreliable so send our versions again to make sure they got through
            SendVersions(true, GetRemoteConnectionID());
        }
        // Listening an incoming connections have data pulled off of the socket by NetTransportTCP
        if (!m_isListening && !m_isIncomingConnection)
        {
            efd::EnvelopeMessagePtr spEnvelopeMessage;
            efd::SInt32 numBytes = ReceiveMessage(spEnvelopeMessage, senderConnectionID);
            if (spEnvelopeMessage)
            {
                ConnectionID versionConnectionID;
                senderConnectionID = spEnvelopeMessage->GetSenderConnection();
                return ReceiveVersions(spEnvelopeMessage,versionConnectionID);
            }
            return numBytes;
        }
        else
        {
            return SetLastError(EE_SOCKET_NO_DATA);
        }
    }
    else
    {
        SmartBuffer buffer;
        ConnectionID versionConnectionID;
        SInt32 numBytes = ReceiveStream(buffer, senderConnectionID);
        if (numBytes > 0)
        {
            Archive ar(Archive::Unpacking, buffer);
            if (!ValidateVersions(ar, versionConnectionID))
            {
                EE_LOG(efd::kNetMessage,
                    efd::ILogger::kERR1,
                    ("Connect (%s) Failed to validate version information!",
                    senderConnectionID.ToString().c_str()));
                return SetLastError(EE_SOCKET_CONNECTION_FAILED);
            }
            else
            {
                return buffer.GetSize();
            }
        }
        else
        {
            return numBytes;
        }
    }
}

//------------------------------------------------------------------------------------------------
efd::SInt32 ConnectionTCP::ReceiveStream(SmartBuffer& o_buffer, ConnectionID& senderConnectionID)
{
    if (!m_spSocket)
    {
        return SetLastError(EE_SOCKET_ERROR_UNKNOWN);
    }

    // receive from the client
    efd::SInt32 numBytes = 0;

    // Attempt to read a message from the socket
    if (m_qos & NET_RELIABLE)
    {
        numBytes = m_spSocket->Receive(o_buffer);
        senderConnectionID = GetRemoteConnectionID();
    }
    else
    {
        numBytes = m_spSocket->ReceiveFrom(o_buffer, senderConnectionID);
    }
    if (numBytes == EE_SOCKET_NO_DATA)
        return SetLastError(EE_SOCKET_NO_DATA);
    if (numBytes == EE_SOCKET_ERROR_UNKNOWN)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Error: Failed to read from socket.  Connection Closing..."));
        EE_LOG_METRIC_COUNT_FMT(kNetwork, ("RECEIVE.ERROR.%u.0x%llX",
            senderConnectionID.GetQualityOfService(),
            senderConnectionID.GetValue()));

        // The Socket is blocking so we had an error and should close the socket...
        return SetLastError(EE_SOCKET_ERROR_UNKNOWN);
    }
    if (numBytes == EE_SOCKET_CONNECTION_CLOSED)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kLVL1,
            ("Connection %s closed gracefully.",
            senderConnectionID.ToString().c_str()));
        return SetLastError(EE_SOCKET_CONNECTION_CLOSED);
    }
    if (numBytes == EE_SOCKET_SHUTDOWN)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kLVL1,
            ("Connection %s shutdown in progress.",
            senderConnectionID.ToString().c_str()));
        return SetLastError(EE_SOCKET_SHUTDOWN);
    }
    // we should be handling any error conditions
    EE_ASSERT(numBytes > 0);
    return numBytes;
}

//------------------------------------------------------------------------------------------------
efd::SInt32 ConnectionTCP::ReceiveMessage(
    EnvelopeMessagePtr& spEnvelopeMessage,
    ConnectionID& remoteConnectionID)
{
    SmartBuffer buffer;
    efd::SInt32 numBytes = ReceiveStream(buffer, remoteConnectionID);
    if (numBytes <= 0)
    {
        return numBytes;
    }

    spEnvelopeMessage = EE_NEW MessageWrapper<EnvelopeMessage, kMSGID_ConnectionDataReceivedMsg>;

    // Read the data from the stream into the message
    Archive ar(Archive::Unpacking, buffer);
    Serializer::SerializeObject(*spEnvelopeMessage, ar);
    if (ar.GetError())
    {
        spEnvelopeMessage = NULL;
        return SetLastError(EE_SOCKET_ERROR_UNKNOWN);
    }

    EE_MESSAGE_BREAK(
        spEnvelopeMessage,
        IMessage::mdf_BreakOnNetReceive,
        ("%s| Net Receive from process %d on connection %s to category %s",
        spEnvelopeMessage->GetDescription().c_str(),
        spEnvelopeMessage->GetSenderNetID(),
        GetRemoteConnectionID().ToString().c_str(),
        spEnvelopeMessage->GetDestinationCategory().ToString().c_str()));

    // Set the name of the transport connection that received the message
    spEnvelopeMessage->SetSenderConnection(remoteConnectionID);
    spEnvelopeMessage->SetQualityOfService(m_qos);

    EE_LOG_METRIC_COUNT_FMT(kNetwork, ("RECEIVE.MESSAGE.%u.0x%llX",
        spEnvelopeMessage->GetQualityOfService(),
        spEnvelopeMessage->GetSenderConnection().GetValue()));

    EE_LOG_METRIC_FMT(kNetwork, ("RECEIVE.BYTES.%u.0x%llX",
            spEnvelopeMessage->GetQualityOfService(),
            spEnvelopeMessage->GetSenderConnection().GetValue()),
        numBytes);

    return numBytes;
}


//------------------------------------------------------------------------------------------------
efd::SInt32 ConnectionTCP::SendMessages()
{
    SInt32 maxBytesPerTick = (SInt32)m_maxSendBytes;
    UInt32 maxMessagesPerTick = m_maxSendMessages;

    while ((!m_messageQueue.empty()) && maxMessagesPerTick-- && maxBytesPerTick > 0)
    {
        MessageToSend messageToSend;
        m_messageQueue.pop_front(messageToSend);
        int retVal = InternalSend(messageToSend);

        // if InternalSend returns 0 it means there is no socket to send to
        if (retVal == EE_SOCKET_CONNECTION_CLOSED)
        {
            EE_LOG(efd::kNetMessage,
                efd::ILogger::kERR2,
                ("ConnectionTCP::SendMessages %d in queue, socket closed!",
                (int)m_messageQueue.size()));
            m_messageQueue.clear();
            return SetLastError(EE_SOCKET_CONNECTION_CLOSED);
        }

        if (retVal == EE_SOCKET_MESSAGE_QUEUED)
        {
            m_messageQueue.push_front(messageToSend);
            return (int)m_messageQueue.size();
        }

        if (retVal < 0)
        {
            return SetLastError(EE_SOCKET_ERROR_UNKNOWN);
        }

        maxBytesPerTick -= retVal;
    }

    if (!m_messageQueue.empty())
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR2,
            ("ConnectionTCP::SendMessages %d in queue", (int)m_messageQueue.size()));
    }

    return (int)m_messageQueue.size();
}

//------------------------------------------------------------------------------------------------
efd::UInt32 ConnectionTCP::QueryOutgoingQueueSize()
{
    efd::UInt32 queueSize = (efd::UInt32)m_messageQueue.size();
    if (m_spSocket)
    {
        if (m_spSocket->SendPending())
        {
            ++queueSize;
        }
        return queueSize;
    }
    else
        return 0;
}

//------------------------------------------------------------------------------------------------
ConnectionID ConnectionTCP::GetRemoteConnectionID() const
{
    if (m_remoteConnectionID == kCID_INVALID)
    {
        EE_ASSERT(m_spSocket);
        return m_spSocket->GetConnectionID();
    }
    else
    {
        return m_remoteConnectionID;
    }
}

//------------------------------------------------------------------------------------------------
bool ConnectionTCP::Send(EnvelopeMessage* pMessage, const ConnectionID& cid)
{
    // Stream the message into a binary stream
    Archive ar;
    // Really we should support different envelope types by first streaming the envelope class id.
    // Unfortunately we can't do this without breaking broadcast cases (old binaries receiving the
    // new data would crash).
    //Serializer::SerializeConstObject(pMessage->GetClassID(), ar);
    Serializer::SerializeObject(*pMessage, ar);
    if (ar.GetError())
    {
        return false;
    }

    MessageToSend messageToSend(pMessage, ar.GetUsedBuffer(), cid);
    EE_ASSERT(messageToSend.m_spMessageToSend);
    EE_ASSERT(messageToSend.m_dataToSend.GetBuffer());

    //DT32261 Do better detection of broadcast ip.
    if (cid.GetIP() == INADDR_BROADCAST)
    {
        m_spSocket->setSocketBroadcast(1);
    }

    if (m_messageQueue.empty())
    {
        int retVal = InternalSend(messageToSend);
        if (retVal == EE_SOCKET_MESSAGE_QUEUED)
        {
            m_messageQueue.push_front(messageToSend);
            return true;
        }

        if (retVal < 0)
        {
            return false;
        }
    }
    else
    {
        m_messageQueue.push_back(messageToSend);
    }
    return true;
}

//------------------------------------------------------------------------------------------------
efd::SInt32 ConnectionTCP::InternalSend(MessageToSend& messageToSend)
{
    EE_ASSERT(messageToSend.m_spMessageToSend);
    EnvelopeMessagePtr pMessage = messageToSend.m_spMessageToSend;

    // Make sure the transport data has a valid socket
    if (m_spSocket)
    {
        EE_MESSAGE_BREAK(
            pMessage,
            IMessage::mdf_BreakOnNetSend,
            ("%s| %s Net Send to %s on transport %s",
            pMessage->GetDescription().c_str(),
            pMessage->GetChildDescription().c_str(),
            pMessage->GetDestinationCategory().ToString().c_str(),
            GetRemoteConnectionID().ToString().c_str()));

        EE_LOG_METRIC_COUNT_FMT(kNetwork, ("SEND.MESSAGE.%u.0x%llX",
            pMessage->GetQualityOfService(),
            pMessage->GetSenderConnection().GetValue()));

        EE_LOG_METRIC_FMT(kNetwork, ("SEND.BYTES.%u.0x%llX",
                pMessage->GetQualityOfService(),
                pMessage->GetSenderConnection().GetValue()),
            messageToSend.m_dataToSend.GetSize());

        // if here, have a valid net data object, go ahead and send it...
        // Shove it through the TCP Socket
        int retVal = 0;
        if (m_qos & NET_UDP)
        {
            ConnectionID cid = GetRemoteConnectionID();
            EE_ASSERT(m_remoteConnectionID != kCID_INVALID);
            retVal = m_spSocket->SendTo(
                messageToSend.m_dataToSend,
                messageToSend.m_destCid);
        }
        else
        {
            retVal = m_spSocket->Send(messageToSend.m_dataToSend);
        }
        if (retVal == EE_SOCKET_MESSAGE_QUEUED)
        {
            return SetLastError(EE_SOCKET_MESSAGE_QUEUED);
        }

        if (retVal <= 0)
        {
            EE_LOG(efd::kMessageTrace, efd::ILogger::kERR1,
                ("%s| Failed Send on connection %s",
                pMessage->GetDescription().c_str(), GetRemoteConnectionID().ToString().c_str()));
            return SetLastError(EE_SOCKET_MESSAGE_NOT_SENT);
        }
        return retVal;
    }

    EE_LOG(efd::kMessageTrace, efd::ILogger::kERR1,
        ("%s| m_spSocket is NULL on transport %s",
        pMessage->GetDescription().c_str(), GetRemoteConnectionID().ToString().c_str()));

    return SetLastError(EE_SOCKET_CONNECTION_CLOSED);
}

//------------------------------------------------------------------------------------------------
void ConnectionTCP::Close()
{
    if (!(m_qos & NET_UDP) || !m_isIncomingConnection)
    {
        if (m_spSocket)
        {
            EE_ASSERT(m_spSocket->GetConnectionID() == m_remoteConnectionID
                || m_remoteConnectionID == kCID_INVALID);
            m_spSocket->Shutdown();
        }
    }
}


//------------------------------------------------------------------------------------------------
void ConnectionTCP::RemoveCallback(
    INetCallback* pCallbackToRemove,
    INetCallback* pReplacementCallback)
{
    if (m_spSocket && m_spSocket->GetCallback() == pCallbackToRemove)
    {
        m_spSocket->SetCallback(pReplacementCallback);
    }
    if (m_pMessageCallback == pCallbackToRemove)
        m_pMessageCallback = pReplacementCallback;
    if (m_pConnectionCallback == pCallbackToRemove)
        m_pConnectionCallback = pReplacementCallback;
}


//------------------------------------------------------------------------------------------------
efd::SmartPointer< ConnectionTCP > ConnectionTCP::Listen(
    MessageFactory* pMessageFactory,
    efd::QualityOfService qos,
    efd::UInt16 portListen,
    INetCallback* pMessageCallback,
    INetCallback* pConnectionCallback)
{
    // open socket on the local host(server) and show its configuration
    SocketPtr spSocket = EE_NEW TCPSocket(qos, portListen, pConnectionCallback);

    // bind the server to the socket
    if (!spSocket->Bind())
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kLVL1,
            ("Server binding process failed! port = %d", portListen));
        return NULL;
    }
    EE_LOG(efd::kNetMessage,
        efd::ILogger::kLVL1,
        ("Finished the server binding process. port = %d", portListen));

    // Set the TCP Socket non-blocking
    spSocket->setSocketBlocking(false);

    // server starts to wait for client calls
    if (qos & NET_TCP) // udp does not need or want listen called
    {
        spSocket->Listen();
    }
    EE_LOG(efd::kNetMessage,
        efd::ILogger::kLVL1,
        ("Server now awaiting accepting client connections on port %d", portListen));

    return EE_NEW ConnectionTCP(pMessageFactory, qos, spSocket, pMessageCallback, false);
}


//------------------------------------------------------------------------------------------------
efd::SmartPointer< ConnectionTCP > ConnectionTCP::Connect(
    MessageFactory* pMessageFactory,
    efd::QualityOfService qos,
    const efd::utf8string& strServerAddress,
    efd::UInt16 portServer,
    INetCallback* pMessageCallback,
    INetCallback* pConnectionCallback)
{
    ConnectionTCPPtr spTransportData =
        EE_NEW ConnectionTCP(pMessageFactory, qos, pMessageCallback, pConnectionCallback);

    EE_ASSERT(spTransportData);
    EE_ASSERT(spTransportData->m_spSocket);

    Socket* pSocket = spTransportData->m_spSocket;

    // connect to the server.
    efd::SInt32 retVal = EE_SOCKET_CONNECTION_COMPLETE;

    // only actually call connect if we are not in connectionless mode
    if (!(qos & NET_CONNECTIONLESS))
    {
        retVal = pSocket->Connect(strServerAddress, portServer);
    }
    else
    {
        pSocket->Bind();
        retVal = EE_SOCKET_CONNECTION_COMPLETE;
        spTransportData->m_connectionState = EE_CONN_SEND_VERSIONS;
    }

    if (retVal == EE_SOCKET_CONNECTION_IN_PROGRESS ||
        retVal == EE_SOCKET_CONNECTION_COMPLETE)
    {
        spTransportData->m_remoteConnectionID = pSocket->GetConnectionID();
        return spTransportData;
    }
    else
    {
        return NULL;
    }
}


//------------------------------------------------------------------------------------------------
efd::SInt32 ConnectionTCP::CompleteConnection()
{
    ConnectionID remoteConnectionID = GetRemoteConnectionID();
    efd::utf8string strServerAddress = HostInfo::IPToString(remoteConnectionID.GetIP());
    efd::UInt16 portServer = remoteConnectionID.GetRemotePort();

    switch (m_connectionState)
    {
    case EE_CONN_NOT_CONNECTED:
        {
            ++m_retryCount;
            if (m_retryCount > kMaxConnectRetries)
            {
                return SetLastError(EE_SOCKET_CONNECTION_TIMED_OUT);
            }

            efd::SInt32 numBytes = m_spSocket->Connect(strServerAddress, portServer);
            if (numBytes == EE_SOCKET_CONNECTION_COMPLETE)
            {
                EE_LOG(efd::kNetMessage,
                    efd::ILogger::kLVL1,
                    ("Connected to %s!", GetRemoteConnectionID().ToString().c_str()));
                m_connectionState = EE_CONN_SEND_VERSIONS;
                return SetLastError(EE_SOCKET_CONNECTION_IN_PROGRESS);
            }
            return numBytes;
        }
    case EE_CONN_ACCEPTED:
        EE_ASSERT(m_isIncomingConnection);
        m_connectionState = EE_CONN_SEND_VERSIONS;
        // fall through
    case EE_CONN_SEND_VERSIONS:
        {
            efd::SInt32 numBytes = SendVersions(true,GetRemoteConnectionID());
            if (numBytes < 0 && numBytes != EE_SOCKET_MESSAGE_QUEUED)
            {
                EE_LOG_METRIC_COUNT_FMT(kNetwork, ("CONNECTION.ERROR.%u.0x%llX",
                    m_qos,
                    GetRemoteConnectionID().GetValue()));

                EE_LOG(efd::kNetMessage,
                    efd::ILogger::kERR1,
                    ("Connect (%s) Failed to send version information!",
                    GetRemoteConnectionID().ToString().c_str()));

                return SetLastError(EE_SOCKET_CONNECTION_FAILED);
            }
            else
            {
                m_connectionState = EE_CONN_RECEIVE_VERSIONS;
                return SetLastError(EE_SOCKET_CONNECTION_IN_PROGRESS);
            }
        }
    case EE_CONN_RECEIVE_VERSIONS:
        {
            ConnectionID senderConnectionID;
            // Attempt to receive version strings
            efd::SInt32 numBytes = ReceiveVersions(senderConnectionID);
            if (numBytes > 0)
            {
                m_connectionState = EE_CONN_CONNECTED;
                return SetLastError(EE_SOCKET_CONNECTION_COMPLETE);
            }
            else
            {
                return numBytes;
            }
        }
    case EE_CONN_CONNECTED:
        return SetLastError(EE_SOCKET_CONNECTION_COMPLETE);
    case EE_CONN_CONNECTION_FAILED:
        EE_FAIL("ConnectionTCP::Connect called after connection failure!");
        return SetLastError(EE_SOCKET_CONNECTION_FAILED);
    default:
        EE_FAIL("ConnectionTCP::Connect unhandled state!");
        return SetLastError(EE_SOCKET_CONNECTION_FAILED);
    }
}


//------------------------------------------------------------------------------------------------
IConnectionPtr ConnectionTCP::Accept()
{
    SocketPtr spSocket = (Socket *) m_spSocket->Accept();
    if (spSocket)
    {
        ConnectionTCPPtr spTCPData =
            EE_NEW ConnectionTCP(m_pMessageFactory, m_qos, spSocket, m_pMessageCallback, true);
        spTCPData->m_isIncomingConnection = true;
        spTCPData->m_connectionState = EE_CONN_ACCEPTED;
        IConnectionPtr spIConnection = (ConnectionTCP*)spTCPData;
        return spIConnection;
    }
    return NULL;
}


//------------------------------------------------------------------------------------------------
NetMessagePtr ConnectionTCP::GetStatusMessage()
{
    if (m_isIncomingConnection)
    {
        if (m_connectionState == EE_CONN_CONNECTED)
        {
            return EE_NEW MessageWrapper<NetMessage, kMSGID_ConnectionAcceptedMsg>;
        }
        else
        {
            return EE_NEW MessageWrapper<NetMessage, kMSGID_ConnectionFailedToAcceptMsg>;
        }
    }
    else
    {
        if (m_connectionState == EE_CONN_CONNECTED)
        {
            return EE_NEW MessageWrapper<NetMessage, kMSGID_ConnectionConnectedMsg>;
        }
        else
        {
            return EE_NEW MessageWrapper<NetMessage, kMSGID_ConnectionFailedToConnectMsg>;
        }
    }
}


