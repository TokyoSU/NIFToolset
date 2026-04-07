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
// Emergent Game Technologies, Calabasas, California 91302
// http://www.emergent.net
#include "egfPCH.h"

#include <egf/BreakpointMessages.h>

//------------------------------------------------------------------------------------------------
using namespace efd;
using namespace egf;


//------------------------------------------------------------------------------------------------
// StartDebuggerRequest
//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(StartDebuggerRequest);

//------------------------------------------------------------------------------------------------
StartDebuggerRequest::StartDebuggerRequest()
    : m_host("localhost")
    , m_port(9000)
    , m_callback(0)
{
}


//------------------------------------------------------------------------------------------------
StartDebuggerRequest::StartDebuggerRequest(
    const efd::utf8string& host,
    efd::UInt32 port,
    const efd::Category &callback)
    : m_host(host)
    , m_port(port)
    , m_callback(callback.GetValue())
{
}


//------------------------------------------------------------------------------------------------
StartDebuggerRequest::~StartDebuggerRequest()
{
}

//------------------------------------------------------------------------------------------------
void StartDebuggerRequest::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_host, ar);
    Serializer::SerializeObject(m_port, ar);
    Serializer::SerializeObject(m_callback, ar);
}

//------------------------------------------------------------------------------------------------
// BreakpointRequest
//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(BreakpointRequest);

//------------------------------------------------------------------------------------------------
BreakpointRequest::BreakpointRequest()
    : m_behaviorName("")
    , m_callback(0)
{
}


//------------------------------------------------------------------------------------------------
BreakpointRequest::BreakpointRequest(const efd::utf8string& behaviorName,
                                     const efd::Category& callback)
    : m_behaviorName(behaviorName)
    , m_callback(callback.GetValue())
{
}

//------------------------------------------------------------------------------------------------
BreakpointRequest::~BreakpointRequest()
{
}

//------------------------------------------------------------------------------------------------
void BreakpointRequest::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_behaviorName, ar);
    Serializer::SerializeObject(m_callback, ar);
}


//------------------------------------------------------------------------------------------------
// BreakpointResponse
//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(BreakpointResponse);

//------------------------------------------------------------------------------------------------
BreakpointResponse::BreakpointResponse()
    : m_result(bp_Unknown)
{
}

//------------------------------------------------------------------------------------------------
BreakpointResponse::~BreakpointResponse()
{
}

//------------------------------------------------------------------------------------------------
BreakpointResponse::BreakpointResponse(
    BreakpointResponse::Result result)
    : m_result(result)
{
}

//------------------------------------------------------------------------------------------------
void BreakpointResponse::Serialize(efd::Archive& ar)
{
    StreamMessage::Serialize(ar);
    Serializer::SerializeObject(m_result, ar);
}

//------------------------------------------------------------------------------------------------
