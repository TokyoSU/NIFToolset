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

#include <efd/ILogger.h>

#include <efdNetwork/HostInfo.h>
#include <efdNetwork/Socket.h>

using namespace efd;

const int MSG_HEADER_LEN = 6;

//------------------------------------------------------------------------------------------------
Socket::Socket()
: m_socketId(INVALID_SOCKET)
, m_blocking(false)
, m_qos(kQOS_Invalid)
, m_pCallback(NULL)
, m_SendOffset(0)
, m_ReceiveOffset(0)
, m_ReceiveSize(0)
, m_pSendBuffer(NULL)
, m_pReceiveBuffer(NULL)
{

    /*
    set the initial address of client that shall be communicated with to
    any address as long as they are using the same port number.
    The m_remoteAddr structure is used in the future for storing the actual
    address of client applications with which communication is going
    to start
    */
    m_localAddr.sin_family = AF_INET;
    m_localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_localAddr.sin_port = 0;
    memset(m_localAddr.sin_zero,0,sizeof(m_localAddr.sin_zero));
    m_remoteAddr.sin_family = AF_INET;
    m_remoteAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_remoteAddr.sin_port = 0;
    memset(m_remoteAddr.sin_zero,0,sizeof(m_localAddr.sin_zero));
    EE_ASSERT(*((efd::UInt64*)(&m_localAddr.sin_zero[0])) == 0);
    EE_ASSERT(*((efd::UInt64*)(&m_remoteAddr.sin_zero[0])) == 0);
}

//------------------------------------------------------------------------------------------------
Socket::Socket(efd::UInt16 portNumber, QualityOfService qos, INetCallback* pCallback)
: m_socketId(INVALID_SOCKET)
, m_blocking(false)
, m_qos(qos)
, m_pCallback(pCallback)
, m_SendOffset(0)
, m_ReceiveOffset(0)
, m_ReceiveSize(0)
, m_pSendBuffer(NULL)
, m_pReceiveBuffer(NULL)
{
    int socketType = SOCK_STREAM;
    if (m_qos & NET_UDP)
    {
        socketType = SOCK_DGRAM;
    }
    EE_ASSERT((m_qos & NET_UDP) || (m_qos & NET_TCP));

    m_socketId = socket(AF_INET, socketType, 0);

    if (EE_SOCKET_INVALID(m_socketId))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: %s", getErrorMessage().c_str()));

        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", qos));

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

//------------------------------------------------------------------------------------------------
Socket::Socket(
    SOCKET socketId,
    struct sockaddr_in& localAddr,
    struct sockaddr_in& remoteAddr,
    QualityOfService qos,
    INetCallback* pCallback)
    : m_socketId(socketId)
    , m_blocking(false)
    , m_localAddr(localAddr)
    , m_remoteAddr(remoteAddr)
    , m_qos(qos)
    , m_pCallback(pCallback)
    , m_SendOffset(0)
    , m_ReceiveOffset(0)
    , m_ReceiveSize(0)
    , m_pSendBuffer(NULL)
    , m_pReceiveBuffer(NULL)
{
    setSocketBlocking(m_blocking);
    EE_ASSERT(m_remoteAddr.sin_addr.s_addr != 0);
//     memset(m_localAddr.sin_zero,0,sizeof(m_localAddr.sin_zero));
//     memset(m_remoteAddr.sin_zero,0,sizeof(m_localAddr.sin_zero));
//     EE_ASSERT(*((efd::UInt64*)(&m_localAddr.sin_zero[0])) == 0);
//     EE_ASSERT(*((efd::UInt64*)(&m_remoteAddr.sin_zero[0])) == 0);
}

//------------------------------------------------------------------------------------------------
Socket::~Socket()
{
    EE_SOCKET_CLOSE(m_socketId);
}

//------------------------------------------------------------------------------------------------
void Socket::setReuseAddr(int reuseToggle)
{
    if (EE_SOCKET_ERROR(setsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_REUSEADDR,
        (char *)&reuseToggle,
        sizeof(reuseToggle))))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: REUSEADDR option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return;
    }
}

//------------------------------------------------------------------------------------------------
void Socket::setLingerSeconds(int seconds)
{
    struct linger lingerOption;

    if (seconds > 0)
    {
        lingerOption.l_linger = EE_SOCKET_LINGER(seconds);
        lingerOption.l_onoff = 1;
    }
    else lingerOption.l_onoff = 0;

    if (EE_SOCKET_ERROR(setsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_LINGER,
        (char *)&lingerOption,
        sizeof(struct linger))))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: LINGER option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return;
    }
}

//------------------------------------------------------------------------------------------------
void Socket::setLingerOnOff(bool lingerOn)
{
    struct linger lingerOption;

    if (lingerOn) lingerOption.l_onoff = 1;
    else lingerOption.l_onoff = 0;

    if (EE_SOCKET_ERROR(setsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_LINGER,
        (char *)&lingerOption,
        sizeof(struct linger))))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: LINGER option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return;
    }
}

//------------------------------------------------------------------------------------------------
void Socket::setSendBufSize(int sendBufSize)
{
    if (EE_SOCKET_ERROR(setsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_SNDBUF,
        (char *)&sendBufSize,
        sizeof(sendBufSize))))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: SENDBUFSIZE option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return;
    }
}

//------------------------------------------------------------------------------------------------
void Socket::setReceiveBufSize(int receiveBufSize)
{
    if (EE_SOCKET_ERROR(setsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_RCVBUF,
        (char *)&receiveBufSize,
        sizeof(receiveBufSize))))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: RCVBUF option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return;
    }
}

//------------------------------------------------------------------------------------------------
int Socket::getReuseAddr()
{
    int myOption;
    socklen_t myOptionLen = sizeof(myOption);

    if (EE_SOCKET_ERROR(getsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_REUSEADDR,
        (char *)&myOption,
        &myOptionLen)))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: get REUSEADDR option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return SOCKET_ERROR;
    }

    return myOption;
}

//------------------------------------------------------------------------------------------------
int Socket::getKeepAlive()
{
#if defined(EE_PLATFORM_XBOX360)
    return 0;
#else
    int myOption;
    socklen_t myOptionLen = sizeof(myOption);

    if (EE_SOCKET_ERROR(getsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_KEEPALIVE,
        (char *)&myOption,
        &myOptionLen)))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: get KEEPALIVE option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return SOCKET_ERROR;
    }
    return myOption;
#endif
}

//------------------------------------------------------------------------------------------------
int Socket::getLingerSeconds()
{
    struct linger lingerOption;
    socklen_t myOptionLen = sizeof(struct linger);

    if (EE_SOCKET_ERROR(getsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_LINGER,
        (char *)&lingerOption,
        &myOptionLen)))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: get LINER option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return SOCKET_ERROR;
    }

    return lingerOption.l_linger;
}

//------------------------------------------------------------------------------------------------
bool Socket::getLingerOnOff()
{
    struct linger lingerOption;
    socklen_t myOptionLen = sizeof(struct linger);

    if (EE_SOCKET_ERROR(getsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_LINGER,
        (char *)&lingerOption,
        &myOptionLen)))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: get LINER option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return false;
    }

    if (lingerOption.l_onoff == 1)
        return true;
    else
        return false;
}

//------------------------------------------------------------------------------------------------
int Socket::getSendBufSize()
{
    int sendBuf;
    socklen_t myOptionLen = sizeof(sendBuf);

    if (EE_SOCKET_ERROR(getsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_SNDBUF,
        (char *)&sendBuf,
        &myOptionLen)))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: get SNDBUF option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return SOCKET_ERROR;
    }
    return sendBuf;
}

//------------------------------------------------------------------------------------------------
int Socket::getReceiveBufSize()
{
    int rcvBuf;
    socklen_t myOptionLen = sizeof(rcvBuf);

    if (EE_SOCKET_ERROR(getsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_RCVBUF,
        (char *)&rcvBuf,
        &myOptionLen)))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: get RCVBUF option: %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return SOCKET_ERROR;
    }
    return rcvBuf;
}

//------------------------------------------------------------------------------------------------
bool Socket::Bind()
{
    if (EE_SOCKET_ERROR(bind(
        m_socketId,
        (struct sockaddr *)&m_localAddr,
        sizeof(struct sockaddr_in))))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: bind() (port %hu) %s",
            htons(m_localAddr.sin_port), getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("BIND.ERROR.%u", m_qos));
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------------------------
void Socket::Shutdown()
{
}

//------------------------------------------------------------------------------------------------
bool Socket::SendPending()
{
    return m_pSendBuffer != NULL;
}

//------------------------------------------------------------------------------------------------
Socket* Socket::Accept()
{
    Socket* retSocket = NULL;
    SOCKET newSocket;   // the new socket file descriptor returned by the accept system call

    // the length of the client's address
    socklen_t clientAddressLen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddress;    // Address of the client that sent data

    // Accepts a new client connection and stores its socket file descriptor
    newSocket = accept(m_socketId, (struct sockaddr *)&clientAddress,&clientAddressLen);

    if (!EE_SOCKET_INVALID(newSocket))
    {
        // Create and return the new Socket object
        retSocket = CreateSocket(newSocket, clientAddress);
    }
    return retSocket;
}

//------------------------------------------------------------------------------------------------
void Socket::Listen(efd::UInt32 totalNumPorts)
{
    if (listen(m_socketId,totalNumPorts) == SOCKET_ERROR)
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: listen() %s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("LISTEN.ERROR.%u", m_qos));
        return;
    }
    EE_LOG(efd::kNetMessage,
        efd::ILogger::kLVL2,
        ("Socket listen(): 0x%08X (port %hu)",
        m_socketId,
        htons(m_localAddr.sin_port)));
}

//------------------------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------------------------
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
