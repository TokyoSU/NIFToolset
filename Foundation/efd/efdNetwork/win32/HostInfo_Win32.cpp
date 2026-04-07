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

#include <efdNetwork/HostInfo.h>

using namespace efd;

void detectErrorGethostbyname(int*, efd::utf8string&);
void detectErrorGethostbyaddr(int*, efd::utf8string&);

efd::map< efd::utf8string, efd::SmartPointer<HostInfo> > HostInfo::m_dnsCache;

HostInfo::HostInfo()
{
    char sName[HOST_NAME_LENGTH+1];
    memset(sName,0,sizeof(sName));
    gethostname(sName,HOST_NAME_LENGTH);
    m_hostName = sName;

    struct hostent* hostPtr = gethostbyname(sName);
    if (hostPtr == NULL)
    {
        int errorCode;
        efd::utf8string errorMsg = "";
        detectErrorGethostbyname(&errorCode,errorMsg);
        return;
    }
    struct in_addr *addr_ptr;
    // the first address in the list of host addresses
    addr_ptr = (struct in_addr *)*hostPtr->h_addr_list;
    // changed the address format to the Internet address in standard dot notation
    m_ipAddressStr = inet_ntoa(*addr_ptr);
    m_ipAddressNum = addr_ptr->s_addr;
}

HostInfo::HostInfo(const efd::utf8string& hostName)
{
    // attempt Retrieve host by address
    m_ipAddressNum = inet_addr(hostName.c_str());
    if (m_ipAddressNum != -1)
    {
        m_ipAddressStr = hostName;

        struct hostent* hostPtr =
            gethostbyaddr((char *)&m_ipAddressNum, sizeof(m_ipAddressNum), AF_INET);
        if (hostPtr == NULL)
        {
            int errorCode;
            efd::utf8string errorMsg = "";
            detectErrorGethostbyaddr(&errorCode,errorMsg);
            return;
        }
        m_hostName = hostPtr->h_name;
    }
    else
    {
        // attempt to Retrieve host by name
        struct hostent* hostPtr = gethostbyname(hostName.c_str());
        m_hostName = hostName;

        if (hostPtr == NULL)
        {
            int errorCode;
            efd::utf8string errorMsg = "";
            detectErrorGethostbyname(&errorCode,errorMsg);
            return;
        }
        struct in_addr *addr_ptr;
        // the first address in the list of host addresses
        addr_ptr = (struct in_addr *)*hostPtr->h_addr_list;
        // changed the address format to the Internet address in standard dot notation
        m_ipAddressStr = inet_ntoa(*addr_ptr);
        m_ipAddressNum = addr_ptr->s_addr;
    }
}

HostInfo::~HostInfo()
{
}

efd::utf8string HostInfo::IPToString(efd::UInt32 ip)
{
    struct in_addr addr_ptr;
    // if address is in host order
    addr_ptr.s_addr = htonl(ip);
    return inet_ntoa(addr_ptr);
}

efd::utf8string HostInfo::NetworkOrderIPToString(efd::UInt32 networkOrderIP)
{
    struct in_addr addr_ptr;
    // if address is in network order
    addr_ptr.s_addr = networkOrderIP;
    return inet_ntoa(addr_ptr);
}

//DT32324 Use proper Windows function to get error message.
//--------------------------------------------------------------------------------------------------
void detectErrorGethostbyname(int* errCode, efd::utf8string& errorMsg)
{
    *errCode = WSAGetLastError();

    if (*errCode == WSANOTINITIALISED)
        errorMsg.append("need to call WSAStartup to initialize socket system on Window system.");
    else if (*errCode == WSAENETDOWN)
        errorMsg.append("The network subsystem has failed.");
    else if (*errCode == WSAHOST_NOT_FOUND)
        errorMsg.append("Authoritative Answer Host not found.");
    else if (*errCode == WSATRY_AGAIN)
        errorMsg.append("Non-Authoritative Host not found, or server failure.");
    else if (*errCode == WSANO_RECOVERY)
        errorMsg.append("Nonrecoverable error occurred.");
    else if (*errCode == WSANO_DATA)
        errorMsg.append("Valid name, no data record of requested type.");
    else if (*errCode == WSAEINPROGRESS)
        errorMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the service "
        "provider is still processing a callback function.");
    else if (*errCode == WSAEFAULT)
        errorMsg.append("The name parameter is not a valid part of the user address space.");
    else if (*errCode == WSAEINTR)
        errorMsg.append("A blocking Windows Socket 1.1 call was canceled through "
        "WSACancelBlockingCall.");
}

void detectErrorGethostbyaddr(int* errCode, efd::utf8string& errorMsg)
{
    *errCode = WSAGetLastError();

    if (*errCode == WSANOTINITIALISED)
        errorMsg.append("A successful WSAStartup must occur before using this function.");
    if (*errCode == WSAENETDOWN)
        errorMsg.append("The network subsystem has failed.");
    if (*errCode == WSAHOST_NOT_FOUND)
        errorMsg.append("Authoritative Answer Host not found.");
    if (*errCode == WSATRY_AGAIN)
        errorMsg.append("Non-Authoritative Host not found, or server failed.");
    if (*errCode == WSANO_RECOVERY)
        errorMsg.append("Nonrecoverable error occurred.");
    if (*errCode == WSANO_DATA)
        errorMsg.append("Valid name, no data record of requested type.");
    if (*errCode == WSAEINPROGRESS)
        errorMsg.append("A blocking Windows Sockets 1.1 call is in progress, or the service "
        "provider is still processing a callback function.");
    if (*errCode == WSAEAFNOSUPPORT)
        errorMsg.append("The type specified is not supported by the Windows Sockets "
        "implementation.");
    if (*errCode == WSAEFAULT)
        errorMsg.append("The addr parameter is not a valid part of the user address space, or the"
        " len parameter is too small.");
    if (*errCode == WSAEINTR)
        errorMsg.append("A blocking Windows Socket 1.1 call was canceled through "
        "WSACancelBlockingCall.");
}
