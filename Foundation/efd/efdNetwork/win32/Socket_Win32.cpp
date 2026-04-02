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
#include <efdNetwork/Socket.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
void Socket::setKeepAlive(int aliveToggle)
{
    if (EE_SOCKET_ERROR(setsockopt(
        m_socketId,
        SOL_SOCKET,
        SO_KEEPALIVE,
        (char *)&aliveToggle,
        sizeof(aliveToggle))))
    {
        EE_LOG(efd::kNetMessage,
            efd::ILogger::kERR1,
            ("Socket Error: ALIVE option:%s", getErrorMessage().c_str()));
        EE_LOG_METRIC_COUNT_FMT(kSocket, ("INIT.ERROR.%u", m_qos));
        return;
    }
}

//------------------------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------------------------
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
