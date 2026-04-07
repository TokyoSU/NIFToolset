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

#include <efdNetwork/NameResolutionMessage.h>
#include <efd/Serialize.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
NameResolutionMessage::NameResolutionMessage()
    : m_classID(0)
    //, m_name
    , m_port(0)
    //, m_hostname;
    //, m_responseCategory;
    , m_responsePort(0)
    , m_responseIP(0)
    , m_timestamp(0.0f)
    , m_lastSend(0.0f)
    , m_delay(0.0f)
    , m_requestNumber(0)
{
}

//------------------------------------------------------------------------------------------------
NameResolutionMessage::~NameResolutionMessage()
{
}

//------------------------------------------------------------------------------------------------
void NameResolutionMessage::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_classID, ar);
    Serializer::SerializeObject(m_name, ar);
    Serializer::SerializeObject(m_port, ar);
    Serializer::SerializeObject(m_hostname, ar);
    Serializer::SerializeObject(m_responseCategory, ar);
    Serializer::SerializeObject(m_responseIP, ar);
    Serializer::SerializeObject(m_responsePort, ar);
    Serializer::SerializeObject(m_timestamp, ar);
    Serializer::SerializeObject(m_requestNumber, ar);
    if (ar.IsPacking() || GetClassID() == kCLASSID_NameResolutionResponse)
    {
        Serializer::SerializeObject(m_extraInfo, ar);
    }
}

//------------------------------------------------------------------------------------------------
bool NameResolutionMessage::operator==(const NameResolutionMessage& a) const
{
    return (m_classID == a.m_classID
        && m_name == a.m_name
        && m_port == a.m_port
        && m_hostname == a.m_hostname
        // ignore response category as it can change
        //&& m_responseCategory == a.m_responseCategory
        && m_responsePort == a.m_responsePort
        && m_responseIP == a.m_responseIP
        && m_timestamp == a.m_timestamp
        && m_extraInfo == a.m_extraInfo);
}

//------------------------------------------------------------------------------------------------
bool NameResolutionMessage::operator<(const NameResolutionMessage& a) const
{
    if (m_classID > a.m_classID)
        return false;
    if (m_name > a.m_name)
        return false;
    if (m_port > a.m_port)
        return false;
    if (m_hostname > a.m_hostname)
        return false;
    // ignore response category as it can change
    //if (m_responseCategory > a.m_responseCategory)
    //    return false;
    if (m_responsePort > a.m_responsePort)
        return false;
    if (m_responseIP > a.m_responseIP)
        return false;
    if (m_timestamp > a.m_timestamp)
        return false;
    if (m_extraInfo > a.m_extraInfo)
        return false;

    if ((*this) == a)
        return false;
    return true;
}

//------------------------------------------------------------------------------------------------
