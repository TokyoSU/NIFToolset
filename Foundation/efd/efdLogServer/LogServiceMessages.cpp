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

#include "efdLogServicePCH.h"

#include <efdLogServer/LogServiceMessages.h>


//------------------------------------------------------------------------------------------------
using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(LogServiceRequest);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(GetModulesResponse);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(GetLogLevelResponse);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(GetMsgLogLevelResponse);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ClientLogServiceRequest);

//------------------------------------------------------------------------------------------------
void LogServiceRequest::Set(
    efd::UInt32 netid,
    const efd::Category& i_callback)
{
    m_netid = netid;
    m_callback = i_callback;
}

//------------------------------------------------------------------------------------------------
void LogServiceRequest::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_netid, ar);
    Serializer::SerializeObject(m_callback, ar);
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(LogServiceResponse);

//------------------------------------------------------------------------------------------------
LogServiceResponse::LogServiceResponse()
    : m_netid(0)
    , m_result(lsr_Unknown)
{
}

//------------------------------------------------------------------------------------------------
void LogServiceResponse::Set(
    efd::UInt32 i_netid,
    LogServiceResponse::Result i_result)
{
    m_netid = i_netid;
    m_result = i_result;
}

//------------------------------------------------------------------------------------------------
// convert this class into a stream of atomic types.
void LogServiceResponse::Serialize(efd::Archive& ar)
{
    StreamMessage::Serialize(ar);
    Serializer::SerializeObject(m_netid, ar);
    Serializer::SerializeObject(m_result, ar);
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(LogEntriesMessage);

//------------------------------------------------------------------------------------------------
LogEntriesMessage::LogEntriesMessage()
{
}

//------------------------------------------------------------------------------------------------
LogEntriesMessage::LogEntriesMessage(
    efd::UInt32 netId,
    efd::UInt64 pid,
    const efd::vector< efd::utf8string > &entries)
    : m_netID(netId)
    , m_pid(pid)
    , m_entries(entries)
{
}

//------------------------------------------------------------------------------------------------
void LogEntriesMessage::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);

    Serializer::SerializeObject(m_netID, ar);
    Serializer::SerializeObject(m_pid, ar);

    SR_ResizableArray<>::Serialize(m_entries, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
ClientLogServiceRequest::ClientLogServiceRequest()
   : efd::EnvelopeMessage()
   , m_callback(efd::kCAT_INVALID)
   , m_target(efd::kCAT_INVALID)
{
}

//------------------------------------------------------------------------------------------------
ClientLogServiceRequest::ClientLogServiceRequest(
    efd::IMessagePtr payload,
    const efd::Category& targetCategory,
    const efd::Category& callback)
    : efd::EnvelopeMessage(payload, targetCategory)
    , m_callback(callback)
    , m_target(targetCategory)
{
}

//------------------------------------------------------------------------------------------------
ClientLogServiceRequest::~ClientLogServiceRequest()
{
}

//------------------------------------------------------------------------------------------------
void ClientLogServiceRequest::Serialize(efd::Archive& ar)
{
    EnvelopeMessage::Serialize(ar);
    Serializer::SerializeObject(m_target, ar);
    Serializer::SerializeObject(m_callback, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void LogLevelRequest::Set(
    efd::UInt32 netId,
    const efd::Category& callback,
    const efd::utf8string& dest)
{
    LogServiceRequest::Set(netId, callback);
    m_destination = dest;
}

//------------------------------------------------------------------------------------------------
void LogLevelRequest::Serialize(efd::Archive& ar)
{
    LogServiceRequest::Serialize(ar);
    Serializer::SerializeObject(m_destination, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void BaseGetLogLevelRequest::Set(
    efd::UInt16 moduleId,
    efd::UInt32 netId,
    const efd::Category& callback,
    const efd::utf8string& dest)
{
    LogLevelRequest::Set(netId, callback, dest);
    m_moduleId = moduleId;
}

//------------------------------------------------------------------------------------------------
void BaseGetLogLevelRequest::Serialize(efd::Archive& ar)
{
    LogLevelRequest::Serialize(ar);
    Serializer::SerializeObject(m_moduleId, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void BaseGetMsgLogLevelRequest::Set(const efd::Category& category,
         efd::ClassID msgClassID,
         efd::UInt32 netId,
         const efd::Category& callback,
         const efd::utf8string& dest)
{
    LogLevelRequest::Set(netId, callback, dest);
    m_category = category;
    m_msgClassID = msgClassID;
}

//------------------------------------------------------------------------------------------------
void BaseGetMsgLogLevelRequest::Serialize(efd::Archive& ar)
{
    LogLevelRequest::Serialize(ar);
    Serializer::SerializeObject(m_category, ar);
    Serializer::SerializeObject(m_msgClassID, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void BaseSetLogLevelRequest::Set(efd::UInt16 moduleId,
         efd::UInt8 levelId,
         efd::UInt32 netId,
         const efd::Category& callback,
         const efd::utf8string& dest)
{
    LogLevelRequest::Set(netId, callback, dest);
    m_moduleId = moduleId;
    m_levelId = levelId;
}

//------------------------------------------------------------------------------------------------
void BaseSetLogLevelRequest::Serialize(efd::Archive& ar)
{
    LogLevelRequest::Serialize(ar);
    Serializer::SerializeObject(m_moduleId, ar);
    Serializer::SerializeObject(m_levelId, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void BaseStartLogStreamRequest::Set(
    efd::UInt16 moduleId,
    efd::UInt8 levelId,
    efd::UInt32 netId,
    const efd::utf8string& streamName,
    const efd::Category& callback)
{
    LogLevelRequest::Set(netId, callback, streamName);
    m_moduleId = moduleId;
    m_levelId = levelId;
}

//------------------------------------------------------------------------------------------------
void BaseStartLogStreamRequest::Serialize(efd::Archive& ar)
{
    LogLevelRequest::Serialize(ar);
    Serializer::SerializeObject(m_moduleId, ar);
    Serializer::SerializeObject(m_levelId, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void BaseSetMsgLogLevelRequest::Set(
    const efd::Category& category,
    efd::ClassID msgClassID,
    efd::UInt8 levelId,
    efd::UInt32 netId,
    const efd::Category& callback,
    const efd::utf8string& dest)
{
    LogLevelRequest::Set(netId, callback, dest);
    m_msgClassID = msgClassID;
    m_levelId = levelId;
    m_category = category;
}

//------------------------------------------------------------------------------------------------
void BaseSetMsgLogLevelRequest::Serialize(efd::Archive& ar)
{
    LogLevelRequest::Serialize(ar);
    Serializer::SerializeObject(m_category, ar);
    Serializer::SerializeObject(m_msgClassID, ar);
    Serializer::SerializeObject(m_levelId, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void NameSetResponse::Set(
    const NameSet& names,
    efd::UInt32 netId,
    LogServiceResponse::Result result)
{
    LogServiceResponse::Set(netId, result);
    m_names = names;
}

//------------------------------------------------------------------------------------------------
void NameSetResponse::Serialize(efd::Archive& ar)
{
    LogServiceResponse::Serialize(ar);
    SR_StdSet<>::Serialize(m_names, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void GetModulesResponse::Set(
    const NameMap& names,
    efd::UInt32 netId,
    LogServiceResponse::Result result)
{
    LogServiceResponse::Set(netId, result);
    m_names = names;
}

//------------------------------------------------------------------------------------------------
void GetModulesResponse::Serialize(efd::Archive& ar)
{
    LogServiceResponse::Serialize(ar);
    SR_StdMap<>::Serialize(m_names, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void GetLogLevelResponse::Set(
    efd::UInt16 moduleId,
    efd::UInt8 levelId,
    efd::UInt32 netId,
    LogServiceResponse::Result result)
{
    LogServiceResponse::Set(netId, result);
    m_moduleId = moduleId;
    m_levelId = levelId;
}

//------------------------------------------------------------------------------------------------
void GetLogLevelResponse::Serialize(efd::Archive& ar)
{
    LogServiceResponse::Serialize(ar);
    Serializer::SerializeObject(m_moduleId, ar);
    Serializer::SerializeObject(m_levelId, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
GetMsgLogLevelResponse::GetMsgLogLevelResponse()
{
}

//------------------------------------------------------------------------------------------------
GetMsgLogLevelResponse::GetMsgLogLevelResponse(
    const efd::Category& category,
    efd::ClassID msgClassID,
    efd::UInt8 levelId,
    efd::UInt32 netId,
    LogServiceResponse::Result result)
    : m_category(category)
    , m_msgClassID(msgClassID)
    , m_levelId(levelId)
{
    Set(netId, result);
}

//------------------------------------------------------------------------------------------------
void GetMsgLogLevelResponse::Serialize(efd::Archive& ar)
{
    LogServiceResponse::Serialize(ar);
    Serializer::SerializeObject(m_category, ar);
    Serializer::SerializeObject(m_msgClassID, ar);
    Serializer::SerializeObject(m_levelId, ar);
}
