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

#include "efdPCH.h"

#include <efd/ConnectionID.h>
#include <efd/Serialize.h>


//------------------------------------------------------------------------------------------------
efd::utf8string efd::ConnectionID::ToString() const
{
    efd::UInt32 ip = GetIP();

    return efd::utf8string(
        efd::Formatted,
        "<ConnID=0x%08X (IP=%d.%d.%d.%d),LocalPort=%d,RemotePort=%d,qos=%d>",
        ip,
        ip >> 24,
        (ip << 8) >> 24,
        (ip << 16) >> 24,
        (ip << 24) >> 24,
        GetLocalPort(),
        GetRemotePort(),
        GetQualityOfService());
}

//------------------------------------------------------------------------------------------------
void efd::ConnectionID::Serialize(efd::Archive& ar)
{
    Serializer::SerializeObject(m_id, ar);
    Serializer::SerializeObject(m_qos, ar);
}

