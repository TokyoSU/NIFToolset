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

#include "efdLogServicePCH.h"

#include <efdLogServer/LogService.h>
#include <efdLogServer/LogServiceMessages.h>

#include <efd/ServiceManager.h>
#include <efd/IDs.h>
#include <efd/ConfigManager.h>
#include <efd/EnumManager.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(LogService);

//------------------------------------------------------------------------------------------------
EE_HANDLER_WRAP(
    LogService,
    HandleGetLogDestinationsRequest,
    efd::LogServiceRequest,
    kMSGID_GetLogDestinationsRequest);

//------------------------------------------------------------------------------------------------
EE_HANDLER_WRAP(
    LogService,
    HandleGetModulesRequest,
    efd::LogServiceRequest,
    kMSGID_GetModulesRequest);

//------------------------------------------------------------------------------------------------
EE_HANDLER_WRAP(
    LogService,
    HandleStartLogStreamRequest,
    efd::BaseStartLogStreamRequest,
    kMSGID_StartLogStreamRequest);

//------------------------------------------------------------------------------------------------
EE_HANDLER_WRAP(
    LogService,
    HandleStopLogStreamRequest,
    efd::LogLevelRequest,
    kMSGID_StopLogStreamRequest);

//------------------------------------------------------------------------------------------------
EE_HANDLER_WRAP(
    LogService,
    HandleGetLogLevelRequest,
    efd::BaseGetLogLevelRequest,
    kMSGID_GetLogLevelRequest);

//------------------------------------------------------------------------------------------------
EE_HANDLER_WRAP(
    LogService,
    HandleGetMsgLogLevelRequest,
    efd::BaseGetMsgLogLevelRequest,
    kMSGID_GetMsgLogLevelRequest);

//------------------------------------------------------------------------------------------------
EE_HANDLER_WRAP(
    LogService,
    HandleSetLogLevelRequest,
    efd::BaseSetLogLevelRequest,
    kMSGID_SetLogLevelRequest);

//------------------------------------------------------------------------------------------------
EE_HANDLER_WRAP(
    LogService,
    HandleSetMsgLogLevelRequest,
    efd::BaseSetMsgLogLevelRequest,
    kMSGID_SetMsgLogLevelRequest);


//------------------------------------------------------------------------------------------------
LogService::LogService(efd::ClassID messageService)
: IMessageHelperSystemService(efd::kCLASSID_LogService, messageService)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 800;
}

//------------------------------------------------------------------------------------------------
const char* LogService::GetDisplayName() const
{
    return "LogService";
}

//------------------------------------------------------------------------------------------------
efd::SyncResult LogService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    pDependencyRegistrar->AddDependency<MessageService>();

    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>(m_messageServiceId);
    EE_ASSERT(m_spMessageService);

    m_pLogger = efd::GetLogger();
    EE_ASSERT(m_pLogger);

    LoadConfig();

    return efd::IMessageHelperSystemService::OnPreInit(pDependencyRegistrar);
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult LogService::OnTick()
{
    // If we have any defined net destinations publish any buffered messages.
    for (LogDestinationMap::const_iterator it = m_netDestinations.begin();
        it != m_netDestinations.end();
        ++it)
    {
        it->second->Publish();
    }

    return efd::AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult LogService::OnShutdown()
{
    if (m_spMessageService)
    {
        m_spMessageService = NULL;
    }

    return efd::IMessageHelperSystemService::OnShutdown();
}

//------------------------------------------------------------------------------------------------
void LogService::HandleGetLogDestinationsRequest(
    const efd::LogServiceRequest *pRequest,
    efd::Category targetChannel)
{
    EE_ASSERT(pRequest->m_callback.IsValid());
    if (pRequest->m_netid != 0 && pRequest->m_netid != m_netid)
    {
        // not ours to handle
        return;
    }

    efd::set<efd::utf8string> names;
    m_pLogger->GetDestinationNames(names);

    GetLogDestinationsResponse* pResponse = EE_NEW GetLogDestinationsResponse();
    pResponse->Set(
        names,
        m_netid,
        LogServiceResponse::lsr_Success);

    m_spMessageService->Send(pResponse, pRequest->m_callback);
}

//------------------------------------------------------------------------------------------------
void LogService::HandleGetModulesRequest(
    const efd::LogServiceRequest *pRequest,
    efd::Category targetChannel)
{
    EE_ASSERT(pRequest->m_callback.IsValid());
    if (pRequest->m_netid != 0 && pRequest->m_netid != m_netid)
    {
        // not ours to handle
        return;
    }

    efd::GetModulesResponse::NameMap nameMap;
    m_pLogger->GetModules(nameMap);

    GetModulesResponse* pResponse = EE_NEW GetModulesResponse();
    pResponse->Set(nameMap, m_netid, LogServiceResponse::lsr_Success);

    m_spMessageService->Send(pResponse, pRequest->m_callback);
}

//------------------------------------------------------------------------------------------------
void LogService::HandleGetLogLevelRequest(
    const efd::BaseGetLogLevelRequest *pRequest,
    efd::Category targetChannel)
{
    EE_ASSERT(pRequest->m_callback.IsValid());
    if (pRequest->m_netid != 0 && pRequest->m_netid != m_netid)
    {
        // not ours to handle
        return;
    }

    utf8string dest;
    if (pRequest->m_destination.length() == 0)
    {
        dest = m_pLogger->GetDefaultDestination()->GetName();
    }
    else
    {
        dest = pRequest->m_destination;
    }

    efd::Logger* logger = EE_DYNAMIC_CAST(efd::Logger, m_pLogger);

    GetLogLevelResponse* pResponse = EE_NEW GetLogLevelResponse();
    pResponse->Set(
        pRequest->m_moduleId,
        logger->GetLogLevel(pRequest->m_moduleId, dest),
        m_netid,
        LogServiceResponse::lsr_Success);

    m_spMessageService->Send(pResponse, pRequest->m_callback);
}

//------------------------------------------------------------------------------------------------
void LogService::HandleGetMsgLogLevelRequest(
    const efd::BaseGetMsgLogLevelRequest *pRequest,
    efd::Category targetChannel)
{
    EE_ASSERT(pRequest->m_callback.IsValid());
    if (pRequest->m_netid != 0 && pRequest->m_netid != m_netid)
    {
        // not ours to handle
        return;
    }

    utf8string dest;
    if (pRequest->m_destination.length() == 0)
    {
        dest = m_pLogger->GetDefaultDestination()->GetName();
    }
    else
    {
        dest = pRequest->m_destination;
    }

    GetMsgLogLevelResponse* pResponse = EE_NEW GetMsgLogLevelResponse(
        pRequest->m_category,
        pRequest->m_msgClassID,
        ILogger::kLogMask_None,
        m_netid,
        LogServiceResponse::lsr_Success);

    m_spMessageService->Send(pResponse, pRequest->m_callback);
}

//------------------------------------------------------------------------------------------------
void LogService::HandleSetLogLevelRequest(
    const efd::BaseSetLogLevelRequest *pRequest,
    efd::Category targetChannel)
{
    if (pRequest->m_netid != 0 && pRequest->m_netid != m_netid)
    {
        // not ours to handle
        return;
    }
    utf8string dest;
    if (pRequest->m_destination.length() == 0)
    {
        dest = m_pLogger->GetDefaultDestination()->GetName();
    }
    else
    {
        dest = pRequest->m_destination;
    }

    efd::Logger* logger = EE_DYNAMIC_CAST(efd::Logger, m_pLogger);
    logger->SetLogLevel(
        pRequest->m_moduleId,
        pRequest->m_levelId,
        dest);

    // If a callback was requested then send it
    if (pRequest->m_callback.IsValid())
    {
        SetLogLevelResponse* pResponse = EE_NEW SetLogLevelResponse();
        pResponse->Set(
            m_netid,
            LogServiceResponse::lsr_Success);

        m_spMessageService->Send(pResponse, pRequest->m_callback);
    }
}

//------------------------------------------------------------------------------------------------
void LogService::HandleSetMsgLogLevelRequest(
    const efd::BaseSetMsgLogLevelRequest *pRequest,
    efd::Category targetChannel)
{
    if (pRequest->m_netid != 0 && pRequest->m_netid != m_netid)
    {
        // not ours to handle
        return;
    }
    utf8string dest;
    if (pRequest->m_destination.length() == 0)
    {
        dest = m_pLogger->GetDefaultDestination()->GetName();
    }
    else
    {
        dest = pRequest->m_destination;
    }

    SetMsgLogLevelResponse* pResponse = EE_NEW SetMsgLogLevelResponse();
    pResponse->Set(
        m_netid,
        LogServiceResponse::lsr_Success);

    // If a callback was requested then send it
    if (pRequest->m_callback.IsValid())
    {
        m_spMessageService->Send(pResponse, pRequest->m_callback);
    }
}

//------------------------------------------------------------------------------------------------
void LogService::HandleStartLogStreamRequest(
    const efd::BaseStartLogStreamRequest *pRequest,
    efd::Category targetChannel)
{
    EE_ASSERT(pRequest->m_callback.IsValid());
    if (pRequest->m_netid != 0 && pRequest->m_netid != m_netid)
    {
        // not ours to handle
        return;
    }

    // add the new destination
    NetDestination* logDest = EE_NEW NetDestination(
        pRequest->m_destination,
        pRequest->m_callback,
        m_spMessageService);

    if (!m_pLogger->AddDest(logDest))
    {
        // replace the existing destination with the updated one.
        m_pLogger->RemoveDest(pRequest->m_destination);
        m_netDestinations.erase(pRequest->m_destination);
        m_pLogger->AddDest(logDest);
    }

    efd::Logger* logger = EE_DYNAMIC_CAST(efd::Logger, m_pLogger);
    logger->SetLogLevel(
        pRequest->m_moduleId,
        pRequest->m_levelId,
        pRequest->m_destination);

    m_netDestinations[pRequest->m_destination] = logDest;

    StartLogStreamResponse* pResponse = EE_NEW StartLogStreamResponse();
    pResponse->Set(
        m_netid,
        LogServiceResponse::lsr_Success);

    m_spMessageService->Send(pResponse, pRequest->m_callback);
}

//------------------------------------------------------------------------------------------------
void LogService::HandleStopLogStreamRequest(
    const efd::LogLevelRequest *pRequest,
    efd::Category targetChannel)
{
    if (pRequest->m_netid != 0 && pRequest->m_netid != m_netid)
    {
        // not ours to handle
        return;
    }
    m_pLogger->RemoveDest(pRequest->m_destination);
    m_netDestinations.erase(pRequest->m_destination);

    // If a callback was requested send it
    if (pRequest->m_callback.IsValid())
    {
        StopLogStreamResponse* pResponse = EE_NEW StopLogStreamResponse();
        pResponse->Set(
            m_netid,
            LogServiceResponse::lsr_Success);

        m_spMessageService->Send(pResponse, pRequest->m_callback);
    }
}

//------------------------------------------------------------------------------------------------
void LogService::LoadConfig()
{
    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    EnumManager* pEnumManager = m_pServiceManager->GetSystemServiceAs<EnumManager>();
    m_pLogger->ReadConfig(pConfigManager, pEnumManager);
}
