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

#include <efdNetwork/HostInfo.h>
#include <errno.h>

using namespace efd;

/*static*/ efd::map< efd::utf8string, efd::SmartPointer<HostInfo> > HostInfo::m_dnsCache;

HostInfo::HostInfo()
{
    char sName[HOST_NAME_LENGTH+1];
    memset(sName,0,sizeof(sName));
    gethostname(sName,HOST_NAME_LENGTH);
    m_hostName = sName;

    struct hostent* hostPtr = gethostbyname(sName);
    if (hostPtr == NULL)
    {
        m_errorMessage = strerror(errno);
        m_ipAddressStr = "127.0.0.1";
        m_ipAddressNum = 0x0100007f;
    }
    else
    {
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

HostInfo::HostInfo(const efd::utf8string& hostName)
{
    // attempt Retrieve host by address
    m_ipAddressNum = inet_addr(hostName.c_str());
    if (m_ipAddressNum != static_cast<efd::UInt32>(-1))
    {
        m_ipAddressStr = hostName;

        struct hostent* hostPtr = gethostbyaddr((char *)&m_ipAddressNum, sizeof(m_ipAddressNum), AF_INET);
        if (hostPtr == NULL)
        {
            m_errorMessage = strerror(errno);
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
            m_errorMessage = strerror(errno);
            return;
        }
        struct in_addr *addr_ptr;
        // the first address in the list of host addresses
        addr_ptr = (struct in_addr *)*hostPtr->h_addr_list;
        // changed the address format to the Internet address in standard dot notation
        m_ipAddressStr = inet_ntoa(*addr_ptr);
        m_ipAddressNum = addr_ptr->s_addr;
    }

    // Retrieve host by address
    m_ipAddressNum = inet_addr(hostName.c_str());
    if (m_ipAddressNum != static_cast<efd::UInt32>(-1))
    {
        m_ipAddressStr = hostName;

        struct hostent* hostPtr = gethostbyaddr((char *)&m_ipAddressNum, sizeof(m_ipAddressNum), AF_INET);
        if (hostPtr == NULL)
        {
            m_errorMessage = strerror(errno);
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
            m_errorMessage = strerror(errno);
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

/*static*/ efd::utf8string HostInfo::IPToString(efd::UInt32 ip)
{
    struct in_addr addr_ptr;
    // if address is in host order
    addr_ptr.s_addr = htonl(ip);
    return inet_ntoa(addr_ptr);
}

/*static*/ efd::utf8string HostInfo::NetworkOrderIPToString(efd::UInt32 networkOrderIP)
{
    struct in_addr addr_ptr;
    // if address is in network order
    addr_ptr.s_addr = networkOrderIP;
    return inet_ntoa(addr_ptr);
}

