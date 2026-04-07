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

#include <efd/OS.h>
#include <efd/ILogger.h>

#include <efdNetwork/INetLib.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
efd::UInt32 INetLib::StartNet()
{
    if (ms_netInitCount == 0)
    {
        /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
        WORD wVersionRequested = MAKEWORD(2, 2);

        WSADATA wsaData;
        int err = ::WSAStartup(wVersionRequested, &wsaData);
        if (err == 0)
        {
            EE_LOG(efd::kNetMessage, efd::ILogger::kLVL1,
                ("Network Startup Success version 0x%04X, requested 0x%04X",
                wsaData.wVersion,
                wVersionRequested));

            ++ms_netInitCount;
        }
        else
        {
            LPVOID lpMsgBuf;
            DWORD dw = GetLastError();

            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0, NULL);
            EE_LOG(efd::kNetMessage, efd::ILogger::kERR1,
                ("Network Startup failed: %s", lpMsgBuf));
            LocalFree(lpMsgBuf);
        }
    }
    else
    {
        ++ms_netInitCount;
    }

    return ms_netInitCount;
}

//------------------------------------------------------------------------------------------------
void INetLib::StopNet()
{
    --ms_netInitCount;
    if (ms_netInitCount == 0)
    {
        WSACleanup();
    }
}

//------------------------------------------------------------------------------------------------
bool INetLib::IsNetReady()
{
    return true;
}

