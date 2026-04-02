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
#include <efdNetwork/efdNetworkPCH.h>

#include <efd/Metrics.h>
#include <efd/ILogger.h>
#include <efd/DataStream.h>

#include <efdNetwork/HostInfo.h>
#include <efdNetwork/TCPSocket.h>

#include <sys/poll.h>

using namespace efd;

const int MSG_HEADER_LEN = 6;

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

efd::UInt32 Socket::getError()
{
    return errno;
}

efd::utf8string Socket::getErrorMessage()
{
    return strerror(errno);
}

efd::utf8string Socket::getErrorMessage(efd::UInt32 errorNum)
{
    return strerror(errorNum);
}


Socket::Socket(efd::UInt16 portNumber, QualityOfService qos)
    : m_socketId(INVALID_SOCKET)
    , m_blocking(false)
    , m_qos(qos)
{
    int socketType = SOCK_STREAM;
    if (m_qos & NET_UDP)
    {
        socketType = SOCK_DGRAM;
    }
    EE_ASSERT((m_qos & NET_UDP) || (m_qos & NET_TCP));

    if ((m_socketId=socket(AF_INET,socketType,0)) == INVALID_SOCKET)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return;
    }
    setSocketBlocking(m_blocking);


    /*
    set the initial address of client that shall be communicated with to
    any address as long as they are using the same port number.
    The m_remoteAddr structure is used in the future for storing the actual
    address of client applications with which communication is going
    to start
    */
    m_localAddr.sin_family = AF_INET;
    m_localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_localAddr.sin_port = htons(portNumber);
    memset(m_localAddr.sin_zero,0,sizeof(m_localAddr.sin_zero));
    m_remoteAddr.sin_family = AF_INET;
    m_remoteAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_remoteAddr.sin_port = 0;
    memset(m_remoteAddr.sin_zero,0,sizeof(m_localAddr.sin_zero));
    EE_ASSERT(*((efd::UInt64*)(&m_localAddr.sin_zero[0])) == 0);
    EE_ASSERT(*((efd::UInt64*)(&m_remoteAddr.sin_zero[0])) == 0);
}


Socket::Socket(SOCKET socketId, struct sockaddr_in& localAddr, struct sockaddr_in& remoteAddr, QualityOfService qos)
    : m_socketId(socketId)
    , m_blocking(false)
    , m_localAddr(localAddr)
    , m_remoteAddr(remoteAddr)
    , m_qos(qos)
{
    setSocketBlocking(m_blocking);
    memset(m_localAddr.sin_zero,0,sizeof(m_localAddr.sin_zero));
    memset(m_remoteAddr.sin_zero,0,sizeof(m_localAddr.sin_zero));
    EE_ASSERT(*((efd::UInt64*)(&m_localAddr.sin_zero[0])) == 0);
    EE_ASSERT(*((efd::UInt64*)(&m_remoteAddr.sin_zero[0])) == 0);
}


Socket::~Socket()
{
    int retVal = close(m_socketId);
    EE_UNUSED_ARG(retVal);
}


void Socket::setDebug(int debugToggle)
{
    if (SOCKET_ERROR == setsockopt(m_socketId, SOL_SOCKET, SO_DEBUG, (char *)&debugToggle, sizeof(debugToggle)))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: DEBUG option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return;
    }
}


void Socket::setReuseAddr(int reuseToggle)
{
    if (setsockopt(m_socketId,SOL_SOCKET,SO_REUSEADDR,(char *)&reuseToggle,sizeof(reuseToggle)) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: REUSEADDR option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return;
    }
}


void Socket::setKeepAlive(int aliveToggle)
{
    if (setsockopt(m_socketId,SOL_SOCKET,SO_KEEPALIVE,(char *)&aliveToggle,sizeof(aliveToggle)) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: ALIVE option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return;
    }
}


void Socket::setLingerSeconds(int seconds)
{
    struct linger lingerOption;

    if (seconds > 0)
    {
        lingerOption.l_linger = seconds;
        lingerOption.l_onoff = 1;
    }
    else lingerOption.l_onoff = 0;

    if (setsockopt(m_socketId,SOL_SOCKET,SO_LINGER,(char *)&lingerOption,sizeof(struct linger)) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: LINGER option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return;
    }
}


void Socket::setLingerOnOff(bool lingerOn)
{
    struct linger lingerOption;

    if (lingerOn) lingerOption.l_onoff = 1;
    else lingerOption.l_onoff = 0;

    if (setsockopt(m_socketId,SOL_SOCKET,SO_LINGER,(char *)&lingerOption,sizeof(struct linger)) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: LINGER option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return;
    }
}


void Socket::setSendBufSize(int sendBufSize)
{
    if (setsockopt(m_socketId,SOL_SOCKET,SO_SNDBUF,(char *)&sendBufSize,sizeof(sendBufSize)) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: SENDBUFSIZE option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return;
    }
}


void Socket::setReceiveBufSize(int receiveBufSize)
{
    if (setsockopt(m_socketId,SOL_SOCKET,SO_RCVBUF,(char *)&receiveBufSize,sizeof(receiveBufSize)) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: RCVBUF option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return;
    }
}


void Socket::setSocketBlocking(bool blocking)
{
    m_blocking = blocking;
    int flags = fcntl(m_socketId,F_GETFD,O_NONBLOCK);
    if (m_blocking)
    {
        flags = flags & ~O_NONBLOCK;
    }
    else
    {
        flags = flags | O_NONBLOCK;
    }

    if (fcntl(m_socketId,F_SETFL,flags) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: Blocking option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return;
    }

}


void Socket::setSocketBroadcast(int broadcast)
{
    if (setsockopt(m_socketId,SOL_SOCKET,SO_BROADCAST,(char *)&broadcast,sizeof(broadcast)) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: setSocketBroadcast: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return;
    }

}


int Socket::getDebug()
{
    int myOption;
    socklen_t myOptionLen = sizeof(myOption);

    if (getsockopt(m_socketId,SOL_SOCKET,SO_DEBUG,(char*)&myOption,&myOptionLen) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: get DEBUG option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return SOCKET_ERROR;
    }

    return myOption;
}


int Socket::getReuseAddr()
{
    int myOption;
    socklen_t myOptionLen = sizeof(myOption);

    if (getsockopt(m_socketId,SOL_SOCKET,SO_REUSEADDR,(char *)&myOption,&myOptionLen) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: get REUSEADDR option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return SOCKET_ERROR;
    }

    return myOption;
}


int Socket::getKeepAlive()
{
    int myOption;
    socklen_t myOptionLen = sizeof(myOption);

    if (getsockopt(m_socketId,SOL_SOCKET,SO_KEEPALIVE,(char *)&myOption,&myOptionLen) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: get KEEPALIVE option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return SOCKET_ERROR;
    }
    return myOption;
}


int Socket::getLingerSeconds()
{
    struct linger lingerOption;
    socklen_t myOptionLen = sizeof(struct linger);

    if (getsockopt(m_socketId,SOL_SOCKET,SO_LINGER,(char *)&lingerOption,&myOptionLen) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: get LINER option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return SOCKET_ERROR;
    }

    return lingerOption.l_linger;
}


bool Socket::getLingerOnOff()
{
    struct linger lingerOption;
    socklen_t myOptionLen = sizeof(struct linger);

    if (getsockopt(m_socketId,SOL_SOCKET,SO_LINGER,(char *)&lingerOption,&myOptionLen) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: get LINER option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return false;
    }

    if (lingerOption.l_onoff == 1)
        return true;
    else
        return false;
}


int Socket::getSendBufSize()
{
    int sendBuf;
    socklen_t myOptionLen = sizeof(sendBuf);

    if (getsockopt(m_socketId,SOL_SOCKET,SO_SNDBUF,(char *)&sendBuf,&myOptionLen) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: get SNDBUF option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return SOCKET_ERROR;
    }
    return sendBuf;
}


int Socket::getReceiveBufSize()
{
    int rcvBuf;
    socklen_t myOptionLen = sizeof(rcvBuf);

    if (getsockopt(m_socketId,SOL_SOCKET,SO_RCVBUF,(char *)&rcvBuf,&myOptionLen) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: get RCVBUF option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "INIT.COUNT.ERROR", 1);
        return SOCKET_ERROR;
    }
    return rcvBuf;
}


TCPSocket::TCPSocket(efd::QualityOfService qos, SOCKET socketId, struct sockaddr_in& localAddr, struct sockaddr_in& remoteAddr, INetCallback* pCallback)
    : Socket(socketId, localAddr, remoteAddr, qos)
    , m_pCallback(pCallback)
    , m_SendOffset(0)
    , m_ReceiveOffset(0)
    , m_ReceiveSize(0)
    , m_pSendBuffer(NULL)
    , m_pReceiveBuffer(NULL)
{
}


TCPSocket::TCPSocket(efd::QualityOfService qos, efd::UInt16 listenPort, INetCallback* pCallback)
    : Socket(listenPort, qos)
    , m_pCallback(pCallback)
    , m_SendOffset(0)
    , m_ReceiveOffset(0)
    , m_ReceiveSize(0)
    , m_pSendBuffer(NULL)
    , m_pReceiveBuffer(NULL)
{
}


TCPSocket::~TCPSocket()
{
    EE_FREE(m_pSendBuffer);
}

bool TCPSocket::Bind()
{
    // Set SO_REUSEADDR so we can rebind after a restart without waiting for TIME_WAIT
    setReuseAddr(1);

    if (bind(m_socketId,(struct sockaddr *)&m_localAddr,sizeof(struct sockaddr_in))== SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: bind() (port %hu) %s",
            htons(m_localAddr.sin_port), getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "BIND.COUNT.ERROR", 1);
        return false;
    }
    return true;
}


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
    if (retVal == SOCKET_ERROR)
    {
        efd::UInt32 errorCode = getError();
        if (errorCode == EINPROGRESS || errorCode == EALREADY)
        {
            // we must actually poll to see if the socket is writable to tell if it is connected
            struct pollfd fds;
            fds.fd = m_socketId;
            fds.events = POLLOUT;
            fds.revents = 0;
            nfds_t nfds = 1;
            // do not block, just check to see if there is data waiting
            int retVal = poll(&fds, nfds,0);
            if (retVal == 0)
            {
                return EE_SOCKET_CONNECTION_IN_PROGRESS;
            }
            else if (retVal < 0)
            {
                EE_LOG(efd::kNetMessage,
                    efd::ILogger::kERR1,
                    ("TCPSocket Error: poll returned an error (%s:%d):%s",
                    pServerInfo->GetHostIPAddressStr(),
                    portServer,
                    getErrorMessage().c_str()));
                return EE_SOCKET_CONNECTION_FAILED;
            }
        }
        else if (errorCode != EISCONN)
        {
            EE_LOG(efd::kNetMessage,
                efd::ILogger::kERR1,
                ("TCPSocket Error: connect(%s:%d,0x%08X:0x%04X) socket=0x%08X:%s",
                serverNameOrAddr.c_str(),
                portServer,
                ntohl(m_remoteAddr.sin_addr.s_addr),
                ntohs(m_remoteAddr.sin_port),
                m_socketId,
                getErrorMessage().c_str()));
            EE_LOG_METRIC(kSocket, "CONNECT.COUNT.ERROR", 1);
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

/*virtual*/ void TCPSocket::Shutdown()
{
}

TCPSocket* TCPSocket::Accept()
{
    TCPSocket* retSocket = NULL;
    SOCKET newSocket;   // the new socket file descriptor returned by the accept system call

    // the length of the client's address
    socklen_t clientAddressLen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddress;    // Address of the client that sent data

    // Accepts a new client connection and stores its socket file descriptor
    newSocket = accept(m_socketId, (struct sockaddr *)&clientAddress,&clientAddressLen);

    if (newSocket != INVALID_SOCKET)
    {
        // Create and return the new TCPSocket object
        retSocket = EE_NEW TCPSocket(m_qos, newSocket, m_localAddr, clientAddress, m_pCallback);
    }
    return retSocket;
}

void TCPSocket::Listen(efd::UInt32 totalNumPorts)
{
    if (listen(m_socketId,totalNumPorts) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: listen() %s", getErrorMessage().c_str()));
        EE_LOG_METRIC(kSocket, "LISTEN.COUNT.ERROR", 1);
        return;
    }
}


#define MAX_DATAGRAM_SIZE 1500

efd::SInt32 TCPSocket::SendTo(efd::DataStream* pMessageStream, efd::ConnectionID destinationConnectionID)
{
    EE_ASSERT(m_qos & NET_UDP);

    efd::SInt32 messageSize = (efd::SInt32)pMessageStream->GetSize();
    int numBytes = 0;  // the number of bytes sent

    // Reset the stream
    pMessageStream->Reset();

    m_pSendBuffer = (efd::UInt8*)pMessageStream->GetRawBufferForReading();
    EE_ASSERT(m_pSendBuffer);
    EE_ASSERT(messageSize <= MAX_DATAGRAM_SIZE);

    EE_LOG_METRIC(kSocket, "SENDTO.COUNT", 1);

    struct sockaddr_in remoteAddr;    // Address of the remote side of the connection
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_addr.s_addr = htonl(destinationConnectionID.GetIP());
    remoteAddr.sin_port = htons(destinationConnectionID.GetRemotePort());
    memset(remoteAddr.sin_zero,0,sizeof(remoteAddr.sin_zero));
    efd::SInt32 datasize = sizeof(remoteAddr);

    // Sends the message to the connected host
    numBytes = sendto(m_socketId, (char*)m_pSendBuffer, messageSize, 0, (const sockaddr*)&remoteAddr, datasize);
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
            ("TCPSocket Error: send() %s %s", GetConnectionID().ToString().c_str(), getErrorMessage(errorCode).c_str()));
        EE_LOG_METRIC(kSocket, "SEND.COUNT.ERROR", 1);
        m_pSendBuffer = NULL;
        return EE_SOCKET_ERROR_UNKNOWN;
    }
    EE_ASSERT(messageSize == numBytes);
    m_pSendBuffer = NULL;
    return numBytes;
}

efd::SInt32 TCPSocket::Send(efd::DataStream* pMessageStream)
{
    EE_ASSERT(m_qos & NET_TCP);

    int messageSize = (int)pMessageStream->GetSize();
    int numBytes = 0;  // the number of bytes sent
    int bytesToSend = 0;
    efd::UInt8* messageToSend = NULL;
    if (m_SendOffset == 0 && m_pSendBuffer == NULL)
    {
        // Reset the stream
        pMessageStream->Reset();

        // Cache a pointer to the Message we are sending
        m_spSendMessage = pMessageStream;

        // Copy the data into a character buffer for transmission
        // for each message to be sent, add a header which shows how long this message
        // is.  The header will be of size byte.
        EE_FREE(m_pSendBuffer);
        m_pSendBuffer = (efd::UInt8*)EE_MALLOC(messageSize + sizeof(efd::SInt32));
        EE_ASSERT(m_pSendBuffer);

        efd::SInt32* piLength = (efd::SInt32*)(m_pSendBuffer);
        *piLength = htonl(messageSize);
        // Copy the stream into the char buffer
        pMessageStream->ReadRawBuffer(m_pSendBuffer + sizeof(efd::SInt32), messageSize);
        // account for the size at the beginning of the message
        messageSize = messageSize + sizeof(efd::SInt32);
        bytesToSend = messageSize;
        messageToSend = m_pSendBuffer;
    }
    else
    {
        // we are resuming send of a partially sent message
        EE_ASSERT(m_spSendMessage == pMessageStream);
        // account for the size at the beginning of the message
        messageSize = messageSize + sizeof(efd::SInt32);
        bytesToSend = messageSize - m_SendOffset;
        if (bytesToSend > messageSize - m_SendOffset)
            bytesToSend = messageSize - m_SendOffset;
        messageToSend = m_pSendBuffer + m_SendOffset;
    }
    EE_ASSERT(messageSize > 0);

    EE_LOG_METRIC(kSocket, "SEND.COUNT", 1);

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
        m_spSendMessage = NULL;
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("TCPSocket Error: send() %s %s", GetConnectionID().ToString().c_str(), getErrorMessage(errorCode).c_str()));
        EE_LOG_METRIC(kSocket, "SEND.COUNT.ERROR", 1);
        return EE_SOCKET_ERROR_UNKNOWN;
    }

    EE_ASSERT((m_SendOffset + numBytes) <= messageSize);
    if ((m_SendOffset + numBytes) == messageSize)
    {
        EE_ASSERT(m_spSendMessage->GetSize() + sizeof(efd::SInt32) == (size_t)messageSize);
        EE_FREE(m_pSendBuffer);
        m_pSendBuffer = NULL;
        m_SendOffset = 0;
        m_spSendMessage = NULL;
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


efd::SInt32 TCPSocket::ReceiveFrom(efd::DataStreamPtr& spStream, efd::ConnectionID& senderConnectionID)
{

    // we don't know how much data is waiting

    efd::SInt32 numBytes = 0;  // The number of bytes received

    if (!m_spReceiveMessage)
    {
        m_spReceiveMessage = EE_NEW DataStream;
    }
    EE_ASSERT(m_spReceiveMessage);
    m_spReceiveMessage->Reset();

    // copy the data directly into the DataStream buffer
    m_pReceiveBuffer = (efd::UInt8*)m_spReceiveMessage->GetRawBufferForWriting(MAX_DATAGRAM_SIZE);

    // who is this message from?
    struct sockaddr_in fromAddr;
    efd::SInt32 datasize = sizeof(sockaddr_in);
    // retrieve the length of the message received
    numBytes = recvfrom(m_socketId,((char*)m_pReceiveBuffer),MAX_DATAGRAM_SIZE,0,(struct sockaddr *)&fromAddr,(socklen_t*)&datasize);

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
            senderConnectionID = ConnectionID(m_qos, hostOrderIPAddr, getLocalPort(), hostOrderRemotePort);
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

            EE_LOG_METRIC(kSocket, "RECEIVE.COUNT.ERROR", 1);
            return EE_SOCKET_ERROR_UNKNOWN;
        }
    }
    EE_LOG_METRIC(kSocket, "RECVFROM.COUNT", 1);
    // record how large the message actually was
    m_spReceiveMessage->SetRawBufferSize(numBytes);

    // populate the ConnectionID with the sender info
    efd::UInt32 hostOrderIPAddr = ntohl(fromAddr.sin_addr.s_addr);
    efd::UInt16 hostOrderRemotePort = ntohs(fromAddr.sin_port);
    senderConnectionID = ConnectionID(m_qos, hostOrderIPAddr, getLocalPort(), hostOrderRemotePort);
    // pass back the DataStream containing the message
    spStream = m_spReceiveMessage;
    // we cannot reuse the same stream because the message contained
    // within may not be created until later
    m_spReceiveMessage = NULL;
    return numBytes;
}

efd::SInt32 TCPSocket::Receive(efd::DataStreamPtr& spStream)
{
    int numBytes = 0;  // The number of bytes received
    efd::SInt32 iNetLength = 0;

    int bytesToReceive = 0;
    efd::UInt8* messageToReceive = NULL;

    int messageSize = m_ReceiveSize + sizeof(efd::SInt32);


    if (m_ReceiveOffset == 0 && m_pReceiveBuffer == NULL)
    {
        // retrieve the length of the message received
        numBytes = recv(m_socketId,((char*)&iNetLength),sizeof(iNetLength),0);

        // check for graceful disconnection
        if (numBytes == 0)
        {
            return EE_SOCKET_CONNECTION_CLOSED;
        }

        EE_LOG_METRIC(kSocket, "RECV.COUNT", 1);

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
                EE_LOG_METRIC(kSocket, "RECEIVE.COUNT.ERROR", 1);
                return EE_SOCKET_ERROR_UNKNOWN;
            }
        }
        EE_ASSERT(numBytes == sizeof(efd::SInt32));

        m_ReceiveSize = ntohl(iNetLength);

        if (!m_spReceiveMessage)
        {
            m_spReceiveMessage = EE_NEW DataStream;
        }
        EE_ASSERT(m_spReceiveMessage);
        m_spReceiveMessage->Reset();
        m_pReceiveBuffer = (efd::UInt8*)m_spReceiveMessage->GetRawBufferForWriting(m_ReceiveSize);

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
    numBytes = recv(m_socketId,(char*)messageToReceive,bytesToReceive,0);

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
            EE_LOG_METRIC(kSocket, "RECEIVE.COUNT.ERROR", 1);
            return EE_SOCKET_ERROR_UNKNOWN;
        }
    }
    EE_ASSERT((m_ReceiveOffset + numBytes) <= m_ReceiveSize);
    if ((m_ReceiveOffset + numBytes) == m_ReceiveSize)
    {
        EE_ASSERT(m_pReceiveBuffer);
        spStream = m_spReceiveMessage;
        //spStream->SetRawBufferSize(m_ReceiveSize);
        EE_VERIFYEQUALS(m_ReceiveSize, (SInt32)spStream->GetSize());
        m_ReceiveSize = 0;
        m_ReceiveOffset = 0;
        // we cannot reuse the same stream because the message contained
        // within may not be created until later
        m_spReceiveMessage = NULL;
        m_pReceiveBuffer = NULL;
        return messageSize;
    }
    else
    {
        m_ReceiveOffset += numBytes;
        return EE_SOCKET_NO_DATA;
    }
}

unsigned short Socket::getLocalPort()
{
    if (m_localAddr.sin_port == 0)
    {
        // grab the local port number
        socklen_t len = sizeof (m_localAddr);
        getsockname (m_socketId, (struct sockaddr *)&m_localAddr, &len);
    }
    return ntohs (m_localAddr.sin_port);
}

unsigned int Socket::getLocalIP()
{
    if (m_localAddr.sin_addr.s_addr == 0)
    {
        // grab the local ip address
        socklen_t len = sizeof (m_localAddr);
        getsockname (m_socketId, (struct sockaddr *)&m_localAddr, &len);
    }
    return ntohl(m_localAddr.sin_addr.s_addr);
}

unsigned short Socket::getRemotePort()
{
    if (m_remoteAddr.sin_port == 0)
    {
        // grab the remote port number
        socklen_t len = sizeof (m_remoteAddr);
        getpeername(m_socketId, (struct sockaddr *)&m_remoteAddr, &len);
    }
    return ntohs (m_remoteAddr.sin_port);
}

unsigned int Socket::getRemoteIP()
{
    if (m_remoteAddr.sin_addr.s_addr == 0)
    {
        // grab the remote ip address
        socklen_t len = sizeof (m_remoteAddr);
        getpeername(m_socketId, (struct sockaddr *)&m_remoteAddr, &len);
    }
    return ntohl(m_remoteAddr.sin_addr.s_addr);
}

