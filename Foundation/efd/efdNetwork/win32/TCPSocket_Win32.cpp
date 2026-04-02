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

#include <efdNetwork/HostInfo.h>
#include <efdNetwork/TCPSocket.h>

using namespace efd;

const int MSG_HEADER_LEN = 6;

typedef int socklen_t;
#define EAGAIN WSAEWOULDBLOCK
#define EINPROGRESS WSAEWOULDBLOCK
#define EALREADY WSAEALREADY
#define EISCONN WSAEISCONN
#define ECONNRESET WSAECONNRESET

//-------------------------------------------------------------------------------------------------
void Socket::setSocketBlocking(bool blocking)
{
    m_blocking = blocking;
    int blockingVal = m_blocking ? 0 : 1;

    if (ioctlsocket(m_socketId,FIONBIO,(unsigned long *)&blockingVal) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: Blocking option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return;
    }
}


//--------------------------------------------------------------------------------------------------
void Socket::setSocketBroadcast(int broadcast)
{
    if (setsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_BROADCAST,
        (char *)&broadcast,
        sizeof(broadcast)) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: setSocketBroadcast: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return;
    }
}


//--------------------------------------------------------------------------------------------------
void Socket::setSocketNoDelay(int noDelay)
{
    if (setsockopt(
        m_socketId,
        IPPROTO_TCP,
        TCP_NODELAY,
        (char *)&noDelay,
        sizeof(noDelay)) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: setSocketNoDelay: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return;
    }
}


//-------------------------------------------------------------------------------------------------
TCPSocket::TCPSocket(
    efd::QualityOfService qos,
    SOCKET socketId,
    struct sockaddr_in& localAddr,
    struct sockaddr_in& remoteAddr,
    INetCallback* pCallback)
    : Socket(socketId, localAddr, remoteAddr, qos, pCallback)
{
}


//--------------------------------------------------------------------------------------------------
TCPSocket::TCPSocket(
    efd::QualityOfService qos,
    efd::UInt16 listenPort,
    INetCallback* pCallback)
    : Socket(listenPort, qos, pCallback)
{
}


//--------------------------------------------------------------------------------------------------
TCPSocket::~TCPSocket()
{
    EE_FREE(m_pSendBuffer);
}

//--------------------------------------------------------------------------------------------------
bool TCPSocket::Bind()
{
    if (bind(m_socketId,(struct sockaddr *)&m_localAddr,sizeof(struct sockaddr_in))== SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: bind() (port %hu) %s",
            htons(m_localAddr.sin_port), getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("BIND.ERROR.%u", m_qos));
        return false;
    }
    EE_LOG(efd::kNetMessage,
        efd::ILogger::kLVL2,
        ("TCPSocket bind(): 0x%08X (port %hu)",
        m_socketId,
        htons(m_localAddr.sin_port)));
    return true;
}


//--------------------------------------------------------------------------------------------------
efd::SInt32 TCPSocket::Connect(const efd::utf8string& serverNameOrAddr, efd::UInt16 portServer)
{
    /*
    when this method is called, a client socket has been built already,
    so we have the m_socketId and m_portNumber ready.

    a HostInfo instance is created, no matter how the server's name is
    given (such as www.yuchen.net) or the server's address is given (such
    as 169.56.32.35), we can use this HostInfo instance to get the
    IP address of the server
    */

    HostInfo* pServerInfo = NULL;
    HostInfo::ResolveNameOrIP(serverNameOrAddr, pServerInfo);
    EE_ASSERT(pServerInfo);

    UInt32 ipAddr = pServerInfo->GetHostIPAddressNum();
    EE_ASSERT((m_remoteAddr.sin_addr.s_addr == htonl(INADDR_ANY))
        || (m_remoteAddr.sin_addr.s_addr == ipAddr));

    // Store the IP address and socket port number
    m_remoteAddr.sin_family = AF_INET;
    m_remoteAddr.sin_addr.s_addr = ipAddr;
    m_remoteAddr.sin_port = htons(portServer);

    EE_ASSERT(*((efd::UInt64*)(&m_localAddr.sin_zero[0])) == 0);
    EE_ASSERT(*((efd::UInt64*)(&m_remoteAddr.sin_zero[0])) == 0);

    int retVal = connect(m_socketId,(struct sockaddr *)&m_remoteAddr,sizeof(m_remoteAddr));
    efd::UInt32 errorCode = getError();
    EE_LOG(efd::kNetMessage,
        efd::ILogger::kLVL2,
        ("TCPSocket connect:(%s:%d,0x%08X:0x%04X) local port=%d socket=0x%08X",
        serverNameOrAddr.c_str(),
        portServer,
        ntohl(m_remoteAddr.sin_addr.s_addr),
        ntohs(m_remoteAddr.sin_port),
        getLocalPort(),
        m_socketId));
    if (retVal == SOCKET_ERROR)
    {
        if (errorCode == EINPROGRESS || errorCode == EALREADY || errorCode == WSAEINVAL)
        {
            return EE_SOCKET_CONNECTION_IN_PROGRESS;
        }
        if (errorCode != EISCONN)
        {
            EE_LOG(efd::kNetMessage,
                efd::ILogger::kERR1,
                ("TCPSocket Error: connect(%s:%d,0x%08X:0x%04X) local port=%d socket=0x%08X:%s",
                serverNameOrAddr.c_str(),
                portServer,
                ntohl(m_remoteAddr.sin_addr.s_addr),
                ntohs(m_remoteAddr.sin_port),
                getLocalPort(),
                m_socketId,
                getErrorMessage().c_str()));
            EE_LOG_METRIC_COUNT_FMT(kSocket, ("CONNECT.ERROR.%u", m_qos));
            return EE_SOCKET_CONNECTION_FAILED;
        }
    }
    else
    {
        EE_ASSERT(retVal == 0);
    }

    // grab the local port number
    socklen_t len = sizeof (m_localAddr);
    retVal = getsockname (m_socketId, (struct sockaddr *)&m_localAddr, &len);
    if (retVal == 0)
    {
        return EE_SOCKET_CONNECTION_COMPLETE;
    }
    else
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: getsockname returned %d", retVal));
        return EE_SOCKET_CONNECTION_FAILED;
    }
}

//--------------------------------------------------------------------------------------------------
void TCPSocket::Shutdown()
{
}

//--------------------------------------------------------------------------------------------------
Socket* TCPSocket::CreateSocket(SOCKET newSocket, struct sockaddr_in clientAddress)
{
    return EE_NEW TCPSocket(m_qos, newSocket, m_localAddr, clientAddress, m_pCallback);
}

//--------------------------------------------------------------------------------------------------
void TCPSocket::Listen(efd::UInt32 totalNumPorts)
{
    if (listen(m_socketId,totalNumPorts) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: listen() %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("LISTEN.ERROR.%u", m_qos));
        return;
    }
    EE_LOG(efd::kNetMessage,
        efd::ILogger::kLVL2,
        ("TCPSocket listen(): 0x%08X (port %hu)",
        m_socketId,
        htons(m_localAddr.sin_port)));
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 TCPSocket::SendTo(
    const efd::SmartBuffer& buffer,
    efd::ConnectionID destinationConnectionID)
{
    EE_ASSERT(m_qos & NET_UDP);
    efd::SInt32 messageSize = (efd::SInt32)buffer.GetSize();
    int numBytes = 0;  // the number of bytes sent

    UInt8* pSendBuffer = (UInt8*)buffer.GetBuffer();
    EE_ASSERT(pSendBuffer);
    EE_ASSERT(messageSize <= MAX_DATAGRAM_SIZE);

    struct sockaddr_in remoteAddr;    // Address of the remote side of the connection
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_addr.s_addr = htonl(destinationConnectionID.GetIP());
    remoteAddr.sin_port = htons(destinationConnectionID.GetRemotePort());
    memset(remoteAddr.sin_zero,0,sizeof(remoteAddr.sin_zero));
    efd::SInt32 datasize = sizeof(remoteAddr);

    // Sends the message to the connected host
    numBytes = sendto(
        m_socketId,
        (char*)pSendBuffer,
        messageSize,
        0,
        (const sockaddr*)&remoteAddr,
        datasize);
    if (numBytes == SOCKET_ERROR)
    {
        int errorCode = getError();
        if (errorCode == EAGAIN)
        {
            // no data was sent, try again later
            return EE_SOCKET_MESSAGE_QUEUED;
        }
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: send() %s %s",
            GetConnectionID().ToString().c_str(),
            getErrorMessage(errorCode).c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("SEND.ERROR.%u", m_qos))
        return EE_SOCKET_ERROR_UNKNOWN;
    }

    EE_LOG_METRIC_COUNT_FMT(kSocket, ("SEND.DATASTREAM.%u.0x%llX", m_qos, destinationConnectionID));
    EE_LOG_METRIC_FMT(kSocket, ("SEND.BYTES.%u.0x%llX", m_qos, destinationConnectionID), numBytes); 

    EE_ASSERT(messageSize == numBytes);
    return numBytes;
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 TCPSocket::Send(const efd::SmartBuffer& buffer)
{
    EE_ASSERT(m_qos & NET_TCP);
    int messageSize = (int)buffer.GetSize();
    int numBytes = 0;  // the number of bytes sent
    int bytesToSend = 0;
    efd::UInt8* messageToSend = NULL;
    if (m_SendOffset == 0 && m_pSendBuffer == NULL)
    {
        // Copy the data into a character buffer for transmission
        // for each message to be sent, add a header which shows how long this message
        // is.  The header will be of size byte.
        EE_FREE(m_pSendBuffer);
        m_pSendBuffer = (efd::UInt8*)EE_MALLOC(messageSize + sizeof(efd::SInt32));
        EE_ASSERT(m_pSendBuffer);

        efd::SInt32* piLength = (efd::SInt32*)(m_pSendBuffer);
        *piLength = htonl(messageSize);
        // Copy the stream into the char buffer
        memcpy(m_pSendBuffer + sizeof(efd::SInt32), buffer.GetBuffer(), messageSize);
        // account for the size at the beginning of the message
        messageSize = messageSize + sizeof(efd::SInt32);
        bytesToSend = messageSize;
        messageToSend = m_pSendBuffer;
    }
    else
    {
        // account for the size at the beginning of the message
        messageSize = messageSize + sizeof(efd::SInt32);
        bytesToSend = messageSize - m_SendOffset;
        if (bytesToSend > messageSize - m_SendOffset)
        {
            bytesToSend = messageSize - m_SendOffset;
        }
        messageToSend = m_pSendBuffer + m_SendOffset;
    }
    EE_ASSERT(messageSize > 0);

    // Sends the message to the connected host
    numBytes = send(m_socketId, (char*)messageToSend, bytesToSend,0);
    if (numBytes == SOCKET_ERROR)
    {
        int errorCode = getError();
        if (errorCode == EAGAIN)
        {
            // no data was sent, try again later
            return EE_SOCKET_MESSAGE_QUEUED;
        }
        EE_FREE(m_pSendBuffer);
        m_pSendBuffer = NULL;
        m_SendOffset = 0;
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: send() %s %s",
            GetConnectionID().ToString().c_str(),
            getErrorMessage(errorCode).c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("SEND.ERROR.%u", m_qos))
        return EE_SOCKET_ERROR_UNKNOWN;
    }

    EE_LOG_METRIC_COUNT_FMT(kSocket, ("SEND.DATASTREAM.%u.0x%llX", m_qos, GetConnectionID()));
    EE_LOG_METRIC_FMT(kSocket, ("SEND.BYTES.%u.0x%llX", m_qos, GetConnectionID()), numBytes); 

    EE_ASSERT((m_SendOffset + numBytes) <= messageSize);
    if ((m_SendOffset + numBytes) == messageSize)
    {
        EE_ASSERT(buffer.GetSize() + sizeof(efd::SInt32) == (size_t)messageSize);
        EE_FREE(m_pSendBuffer);
        m_pSendBuffer = NULL;
        m_SendOffset = 0;
        return numBytes;
    }
    else
    {
        EE_ASSERT(numBytes>0);
        // some data was sent
        m_SendOffset += numBytes;
        return EE_SOCKET_MESSAGE_QUEUED;
    }
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 TCPSocket::Send(const efd::Char* pData, efd::SInt32 size)
{
    // Sends the message to the connected host
    return send(m_socketId, pData, size, 0);
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 TCPSocket::SendTo(
    const efd::Char* pData,
    efd::SInt32 size,
    const efd::utf8string& serverNameOrAddr,
    efd::UInt16 portServer)
{
    // only valid on UDP sockets
    EE_ASSERT((m_qos & NET_UDP));

    HostInfo* pServerInfo = NULL;
    HostInfo::ResolveNameOrIP(serverNameOrAddr, pServerInfo);
    EE_ASSERT(pServerInfo);

    UInt32 ipAddr = pServerInfo->GetHostIPAddressNum();

    struct sockaddr_in remoteAddr;
    memset(remoteAddr.sin_zero,0,sizeof(remoteAddr.sin_zero));

    // Store the IP address and socket port number
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_addr.s_addr = ipAddr;
    remoteAddr.sin_port = htons(portServer);

    return sendto(m_socketId, pData, size, 0, (struct sockaddr *)&remoteAddr,sizeof(remoteAddr));
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 TCPSocket::ReceiveFrom(
    efd::SmartBuffer& o_buff,
    efd::ConnectionID& senderConnectionID)
{
    // we don't know how much data is waiting

    efd::SInt32 numBytes = 0;  // The number of bytes received

    EE_VERIFY(m_receiveBuffer.Grow(MAX_DATAGRAM_SIZE));

    // copy the data directly into our own buffer (not the destination buffer)
    m_pReceiveBuffer = m_receiveBuffer.GetBuffer();

    // who is this message from?
    struct sockaddr_in fromAddr;
    efd::SInt32 datasize = sizeof(sockaddr_in);
    // retrieve the length of the message received
    numBytes = recvfrom(
        m_socketId,
        ((char*)m_pReceiveBuffer),
        MAX_DATAGRAM_SIZE,
        0,
        (struct sockaddr *)&fromAddr,
        &datasize);

    if (numBytes == 0)
    {
        // received and empty UDP message, socket is still valid
        return EE_SOCKET_NO_DATA;
    }
    if (numBytes == SOCKET_ERROR)
    {
        int errorCode = getError();
        if (errorCode == EAGAIN)
        {
            return EE_SOCKET_NO_DATA;
        }
        else if (errorCode == ECONNRESET)
        {
            // the other side of the UDP connection has closed and returned ICMP unreachable
            // populate the ConnectionID with the sender info
            efd::UInt32 hostOrderIPAddr = ntohl(fromAddr.sin_addr.s_addr);
            efd::UInt16 hostOrderRemotePort = ntohs(fromAddr.sin_port);
            senderConnectionID =
                ConnectionID(m_qos, hostOrderIPAddr, getLocalPort(), hostOrderRemotePort);
            return EE_SOCKET_CONNECTION_CLOSED;
        }
        else
        {
            // An error (other than no data available) occurred
            EE_LOG(efd::kNetMessage,
                efd::ILogger::kERR1,
                ("TCPSocket Error: recv(message size)%s %s",
                GetConnectionID().ToString().c_str(),
                getErrorMessage(errorCode).c_str()));

            EE_LOG_METRIC_COUNT_FMT(kSocket, ("RECEIVE.ERROR.%u.0x%llX",
                m_qos,
                GetConnectionID()));
            
            return EE_SOCKET_ERROR_UNKNOWN;
        }
    }

    EE_LOG_METRIC_COUNT_FMT(kSocket, ("RECEIVE.DATASTREAM.%u.0x%llX", m_qos, senderConnectionID));
    EE_LOG_METRIC_FMT(kSocket, ("RECEIVE.BYTES.%u.0x%llX", m_qos, senderConnectionID), numBytes); 

    // populate the ConnectionID with the sender info
    efd::UInt32 hostOrderIPAddr = ntohl(fromAddr.sin_addr.s_addr);
    efd::UInt16 hostOrderRemotePort = ntohs(fromAddr.sin_port);
    senderConnectionID = ConnectionID(m_qos, hostOrderIPAddr, getLocalPort(), hostOrderRemotePort);

    // pass back the buffer containing the message. We copy the smaller "used" buffer out so that
    // we maintain exclusive ownership of our receive buffer.
    o_buff = m_receiveBuffer.MakeWindow(0, numBytes).Clone();

    return numBytes;
}


//--------------------------------------------------------------------------------------------------
efd::SInt32 TCPSocket::Receive(efd::SmartBuffer& o_buff)
{
    int numBytes = 0;  // The number of bytes received

    int bytesToReceive = 0;
    efd::UInt8* messageToReceive = NULL;

    int messageSize = m_ReceiveSize + sizeof(efd::SInt32);

    if (m_pReceiveBuffer == NULL)
    {
        // retrieve the length of the message received
        EE_ASSERT(m_ReceiveOffset < sizeof(efd::SInt32));
        numBytes = recv(
            m_socketId,
            (((char*)&m_ReceiveSize) + m_ReceiveOffset),
            sizeof(efd::SInt32) - m_ReceiveOffset,
            0);

        // check for graceful disconnection
        if (numBytes == 0)
        {
            return EE_SOCKET_CONNECTION_CLOSED;
        }

        if (numBytes == SOCKET_ERROR)
        {
            int errorCode = getError();
            if (errorCode == EAGAIN)
            {
                return EE_SOCKET_NO_DATA;
            }
            else
            {
                // An error (other than no data available) occurred
                EE_LOG(efd::kNetMessage,
                    efd::ILogger::kERR1,
                    ("TCPSocket Error: recv(message size)%s %s",
                    GetConnectionID().ToString().c_str(),
                    getErrorMessage(errorCode).c_str()));

                EE_LOG_METRIC_COUNT_FMT(kSocket, ("RECEIVE.ERROR.%u.0x%llX",
                    m_qos,
                    GetConnectionID()));

                return EE_SOCKET_ERROR_UNKNOWN;
            }
        }

        EE_ASSERT((numBytes + m_ReceiveOffset) <= sizeof(efd::SInt32));

        if ((numBytes + m_ReceiveOffset) < sizeof(efd::SInt32))
        {
            m_ReceiveOffset += numBytes;
            return EE_SOCKET_NO_DATA;
        }
        else
        {
            m_ReceiveOffset = 0;
        }

        m_ReceiveSize = ntohl(m_ReceiveSize);

        m_receiveBuffer.Grow(m_ReceiveSize);
        m_pReceiveBuffer = m_receiveBuffer.GetBuffer();

        messageToReceive = m_pReceiveBuffer;
        bytesToReceive = m_ReceiveSize;
    }
    else
    {
        bytesToReceive = m_ReceiveSize - m_ReceiveOffset;
        messageToReceive = m_pReceiveBuffer + m_ReceiveOffset;
    }
    messageSize = m_ReceiveSize + sizeof(efd::SInt32);

    EE_ASSERT(m_pReceiveBuffer);
    EE_ASSERT(messageToReceive);
    numBytes = recv(m_socketId, (char*)messageToReceive, bytesToReceive, 0);

    if (numBytes == 0)
    {
        return EE_SOCKET_CONNECTION_CLOSED;
    }

    if (numBytes == SOCKET_ERROR)
    {
        int errorCode = getError();
        if (errorCode == EAGAIN)
        {
            return EE_SOCKET_NO_DATA;
        }
        else
        {
            // An error (other than no data available) occurred
            EE_LOG(efd::kNetMessage,
                efd::ILogger::kERR1,
                ("TCPSocket Error: recv(message) %s", getErrorMessage().c_str()));

            EE_LOG_METRIC_COUNT_FMT(kSocket, ("RECEIVE.ERROR.%u.0x%llX",
                m_qos,
                GetConnectionID()));

            return EE_SOCKET_ERROR_UNKNOWN;
        }
    }

    EE_LOG_METRIC_COUNT_FMT(kSocket, ("RECEIVE.DATASTREAM.%u.0x%llX", m_qos, GetConnectionID()));
    EE_LOG_METRIC_FMT(kSocket, ("RECEIVE.BYTES.%u.0x%llX", m_qos, GetConnectionID()), numBytes); 

    EE_ASSERT((m_ReceiveOffset + numBytes) <= m_ReceiveSize);
    if ((m_ReceiveOffset + numBytes) == m_ReceiveSize)
    {
        EE_ASSERT(m_pReceiveBuffer);
        // Clone the buffer
        o_buff = m_receiveBuffer.MakeWindow(0, m_ReceiveSize).Clone();
        EE_VERIFYEQUALS(m_ReceiveSize, (SInt32)o_buff.GetSize());
        m_ReceiveSize = 0;
        m_ReceiveOffset = 0;
        m_pReceiveBuffer = NULL;
        return messageSize;
    }
    else
    {
        m_ReceiveOffset += numBytes;
        return EE_SOCKET_NO_DATA;
    }
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 TCPSocket::Receive(efd::Char* pData, efd::SInt32 size)
{
    return recv(m_socketId, pData, size, 0);
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 TCPSocket::ReceiveFrom(
    efd::Char* pData,
    efd::SInt32 size,
    efd::UInt32& hostOrderIPAddr,
    efd::UInt16& hostOrderRemotePort)
{
    // only valid on UDP sockets
    EE_ASSERT((m_qos & NET_UDP));
    struct sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    memset(remoteAddr.sin_zero,0,sizeof(remoteAddr.sin_zero));

    efd::SInt32 datasize = sizeof(sockaddr_in);

    SInt32 retVal = recvfrom(m_socketId, pData, size, 0,(struct sockaddr *)&remoteAddr, &datasize);

    // Store the IP address and socket port number

    hostOrderIPAddr = remoteAddr.sin_addr.s_addr;
    hostOrderRemotePort = ntohs(remoteAddr.sin_port);
    return retVal;
}

