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

#include "egfPCH.h"

#ifndef EE_CONFIG_SHIPPING

#include <egf/RapidIterationService.h>

#include <efd/PathUtils.h>
#include <efd/ReloadManager.h>
#include <efd/ServiceManager.h>
#include <efd/AssetConfigService.h>
#include <efd/AssetApplicationInfoMsg.h>
#include <efd/AssetFactoryManager.h>
#include <efd/AssetLoadResponse.h>
#include <efdLogServer/LogService.h>
#include <efdNetwork/ChannelManagerService.h>
#include <efdNetwork/NetService.h>
#include <efdNetwork/NameResolutionService.h>
#include <egf/egfLogIDs.h>
#include <egf/Scheduler.h>
#include <egf/WorldResetRequest.h>
#include <egf/GamePauseRequest.h>
#include <egf/GameResumeRequest.h>
#include <egf/EntityLoaderMessages.h>
#include <egf/EntityLoaderService.h>
#include <egf/FlatModelManager.h>
#include <egf/FlatModelFactory.h>
#include <egf/FlatModelFactoryResponse.h>
#include <egf/SimDebugger.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(RapidIterationSystemService);

EE_HANDLER_WRAP(
    RapidIterationSystemService,
    HandleAssetLocatorResponse,
    AssetLocatorResponse,
    kMSGID_AssetLocatorResponse);

EE_HANDLER_WRAP(
    RapidIterationSystemService,
    HandleNameResolutionResponse,
    efd::NameResolutionMessage,
    kCLASSID_NameResolutionResponse);

EE_HANDLER_WRAP(
    RapidIterationSystemService,
    HandleGameControlMessage,
    IMessage,
    kMSGID_WorldResetRequest);
EE_HANDLER_WRAP(
    RapidIterationSystemService,
    HandleGameControlMessage,
    IMessage,
    kMSGID_GamePauseRequest);
EE_HANDLER_WRAP(
    RapidIterationSystemService,
    HandleGameControlMessage,
    IMessage,
    kMSGID_GameResumeRequest);

EE_HANDLER(RapidIterationSystemService, HandleAssignNetIDMessage, AssignNetIDMessage);

// Notified when a load response is processed by the AssetFactoryManager.
EE_HANDLER(RapidIterationSystemService, HandleAssetLoadResponse, AssetLoadResponse);
EE_HANDLER_SUBCLASS(
    RapidIterationSystemService,
    HandleAssetLoadResponse,
    AssetLoadResponse,
    FlatModelFactoryResponse);


//------------------------------------------------------------------------------------------------
RapidIterationSystemService::RapidIterationSystemService()
    : m_asset_lookup_callback(kCAT_INVALID)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 2080;
}


//------------------------------------------------------------------------------------------------
RapidIterationSystemService::~RapidIterationSystemService()
{
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::InitToolsMessaging(ServiceManager *pServiceManager)
{
    // Create a secondary NetService instance just for tools messaging
    EE_LOG(
        efd::kRapidIteration,
        efd::ILogger::kLVL2,
        ("%s> Registering the ToolsNetService.", __FUNCTION__));
    m_spNetService = EE_NEW NetService("NetLib");// always use default tcp netlib
    EE_ASSERT(m_spNetService != 0);

    // Create a secondary MessageService instance just for tools messaging
    EE_LOG(
        efd::kRapidIteration,
        efd::ILogger::kLVL2,
        ("%s> Registering the ToolsMessageService.", __FUNCTION__));
    m_spToolsMsgService = EE_NEW MessageService();
    EE_ASSERT(m_spToolsMsgService != 0);

    // Tools NetService uses the tools MessageService
    // must notify NetService of which MessageService to use before registering with ServiceManager
    // as RegisterSystemService can cause OnPreInit to be called
    m_spNetService->SetMessageService(m_spToolsMsgService);

    // Subscribe to the NetSerevice public Category to get the AssignNetIDMessage
    Category netLocalCategory = MessageService::GetServicePublicCategory(INetService::CLASS_ID);
    m_spToolsMsgService->Subscribe(this, netLocalCategory);

    // NetService notifies MessageService when it is ready to accept subscriptions
    pServiceManager->RegisterAlternateSystemService(m_spNetService, kCLASSID_ToolsNetService);
    pServiceManager->RegisterAlternateSystemService(
        m_spToolsMsgService,
        kCLASSID_ToolsMessageService);

    // This prevents the NetService from trying to connect to a ChannelManager automatically
    // if the program type is set to kProgType_Server.
    m_spNetService->AutoReconnectToChannelManager(false);
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::InitLogService(ServiceManager *pServiceManager)
{
    LogService* pLogService = pServiceManager->GetSystemServiceAs<LogService>();
    if (pLogService == 0)
    {
        EE_LOG(
            efd::kRapidIteration,
            efd::ILogger::kLVL2,
            ("%s> Registering the LogService.", __FUNCTION__));

        pLogService = EE_NEW LogService(kCLASSID_ToolsMessageService);

        EE_ASSERT(pLogService != 0);

        pServiceManager->RegisterSystemService(pLogService);
    }
    else
    {
        EE_LOG(
            efd::kRapidIteration,
            efd::ILogger::kLVL2,
            ("%s> LogService already exists. Not registering the LogService.", __FUNCTION__));
    }
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::InitReloadManager(ServiceManager* pServiceManager)
{
    EE_LOG(
        efd::kRapidIteration,
        efd::ILogger::kLVL2,
        ("%s> Registering the Reload Manager service.", __FUNCTION__));

    ReloadManagerPtr pService = EE_NEW ReloadManager();
    EE_ASSERT(pService != 0);

    pServiceManager->RegisterSystemService(pService);
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::InitNameResolutionService(ServiceManager* pServiceManager)
{
    EE_LOG(
        efd::kRapidIteration,
        efd::ILogger::kLVL2,
        ("%s> Registering the NameResolution service.", __FUNCTION__));

    NameResolutionServicePtr pService = EE_NEW NameResolutionService();
    EE_ASSERT(pService != 0);

    pServiceManager->RegisterSystemService(pService);
}

//------------------------------------------------------------------------------------------------
const Category& RapidIterationSystemService::GetToolbenchPublicCategory()
{
    static const Category kCAT_FromToolbench(
        UniversalID::ECU_Any,
        kNetID_Any,
        kBASEID_FromToolbench);
    return kCAT_FromToolbench;
}

//------------------------------------------------------------------------------------------------
efd::Category RapidIterationSystemService::GetPrivateCategory()
{
    return efd::Category(efd::UniversalID::ECU_Any, efd::kNetID_ISystemService, CLASS_ID);
}

//------------------------------------------------------------------------------------------------
SyncResult RapidIterationSystemService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    pDependencyRegistrar->AddDependency<ReloadManager>();
    pDependencyRegistrar->AddDependency<LogService>();

    // Register system services needed for rapid iteration
    InitToolsMessaging(m_pServiceManager);
    InitLogService(m_pServiceManager);
    InitReloadManager(m_pServiceManager);
    InitNameResolutionService(m_pServiceManager);

    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    EE_ASSERT(m_spMessageService);
    m_spAssetService = m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
    EE_ASSERT(m_spAssetService);

    // Register request message factories
    RegisterMessageWrapperFactory<IMessage, kMSGID_WorldResetRequest>(m_spToolsMsgService);
    RegisterMessageWrapperFactory<IMessage, kMSGID_GamePauseRequest>(m_spToolsMsgService);
    RegisterMessageWrapperFactory<IMessage, kMSGID_GameResumeRequest>(m_spToolsMsgService);

    // We don't consume this but the EntityLoaderService will try to send an EntityLoadResponse
    // message to our private category anyway. Ideally we'd avoid this but the RI unit tests rely
    // on this behavior. To avoid unknown factory errors, register the message factory here.
    RegisterMessageFactory<EntityLoadResult>(m_spMessageService);

    ReloadManagerPtr spReloadManager = m_pServiceManager->GetSystemServiceAs<ReloadManager>();
    EE_ASSERT(spReloadManager);

    // Note: The following code registers asset change handlers for several game framework
    // assets. This is required here to ensure these assets reload in the corrected order (FIFO).
    // Ideally we'd update the ReloadManager and AssetChangeHandler classes to support asynchronous
    // reloading of assets. An Asset change handler could indicate that a reload is in progress,
    // and also indicate all other reloads should be delayed until the current reload completes.
    // We'd also want to ensure dependencies were reloaded in the correct order, which probably
    // requires additional metadata to identify these dependencies at runtime.
    //
    // Once the ReloadManager supports asynchronous reload handling, the following asset types
    // should be reloaded using appropriate AssetChangeHandler class instances. For now, the
    // RapidIterationService is the reload handler of record for these asset types.

    // Automatically fetch any updated model (.flat) files as they are changed.
    spReloadManager->RegisterAssetChangeHandler("emergent-flat-model", this);

    // Automatically fetch any updated script files as they are changed.
    spReloadManager->RegisterAssetChangeHandler("python-behavior", this);
    spReloadManager->RegisterAssetChangeHandler("lua-behavior", this);

    // Automatically fetch any updated block files as they are changed.
    spReloadManager->RegisterAssetChangeHandler("emergent-world", this);

    m_asset_lookup_callback = GenerateAssetResponseCategory();

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
AsyncResult RapidIterationSystemService::OnInit()
{
    // Subscribe messages that will be sent from Toolbench
    m_spToolsMsgService->Subscribe(this, GetToolbenchPublicCategory());
    // Subscribe message that will be sent by the AssetLocatorService
    m_spMessageService->Subscribe(this, m_asset_lookup_callback);

    // Subscribe to our private category for AssetFactory response handling.
    m_spMessageService->Subscribe(this, GetPrivateCategory());

    // The user is allowed to override the automatic locating of the Asset Ctrl that uses
    // our NameResolutionService. This should only be required if the game and the Asset
    // Ctrl are running on different subnets, since the NameResolutionService does not work
    // across a subnet boundary.  Override is by setting AssetWeb.UseDefaultChannelMgr=true
    // and/or by setting AssetWeb.UseAssetHost=<Host>.
    AssetConfigService* pConfig = m_pServiceManager->GetSystemServiceAs<AssetConfigService>();
    EE_ASSERT (pConfig);
    utf8string value_str;
    pConfig->GetConfigValue("UseDefaultChannelMgr", value_str);
    Bool bValue=false;
    ParseHelper<Bool>::FromString(value_str, bValue);
    utf8string assetHost;
    pConfig->GetConfigValue("UseAssetHost", assetHost);

    if (bValue || assetHost.length())
    {
        m_spNetService->AutoReconnectToChannelManager(true);
        if (!assetHost.length())
        {
            m_spNetService->ConnectToDefaultChannelManager();
        }
        else
        {
            m_spNetService->ConnectToChannelManager(
                QOS_RELIABLE,
                assetHost,
                ChannelManagerService::defaultListenPort);
        }
    }
    else
    {
        // We use NameResolution service to find the ChannelManager that is announcing
        // the AssetWeb AuthorId that matches the asset web our application uses.
        utf8string authid;
        m_spAssetService->GetAssetWebAuthorId (authid);

        // the name announced by the ChannelManager includes the Quality of Service
        utf8string advertised = authid + ":" + ChannelManagerService::kQOS_Reliable;

        NameResolutionService* pNameService =
            m_pServiceManager->GetSystemServiceAs<NameResolutionService>();
        Category responseCategory = pNameService->LookupService(advertised,
            ChannelManagerService::CLASS_ID, NameResolutionService::kNameLookup_NoTimeout);
        m_spMessageService->Subscribe(this, responseCategory, QOS_CONNECTIONLESS);
    }

    // Begin sending Toolbench log messages important to rapid iteration
    LogService* pLogService = m_pServiceManager->GetSystemServiceAs<LogService>();
    efd::Category cat =
        efd::Category(efd::UniversalID::ECU_Any, efd::kNetID_Any, efd::kBASEID_ToToolbenchLogging);
    efd::StartLogStreamRequest* pRequest = EE_NEW efd::StartLogStreamRequest();
    if (pRequest)
    {
        pRequest->Set(efd::kLua, efd::ILogger::kLogMask_UptoErr2 | efd::ILogger::kLogMask_Lvl0, 0,
            "RemoteLuaLogs", cat);
        m_spToolsMsgService->Send(pRequest, pLogService->GetPrivateCategory());
    }
    efd::StartLogStreamRequestPtr spEntityErrors = EE_NEW efd::StartLogStreamRequest();
    if (spEntityErrors)
    {
        spEntityErrors->Set(efd::kEntity, efd::ILogger::kLogMask_UptoErr2, 0,
            "RemoteEntityLogs", cat);
        m_spToolsMsgService->Send(spEntityErrors, pLogService->GetPrivateCategory());
    }

    AssetFactoryManager* pAFM = m_pServiceManager->GetSystemServiceAs<AssetFactoryManager>();
    EE_ASSERT(pAFM);
    pAFM->RegisterAssetCallback(FlatModelFactory::CLASS_ID, GetPrivateCategory());

    if (SimDebugger::Instance())
        SimDebugger::Instance()->Initialize(m_pServiceManager);

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
efd::Bool RapidIterationSystemService::WaitForAssetServer (efd::TimeType timeout)
{
    EE_UNUSED_ARG (timeout);
    EE_LOG(
        efd::kRapidIteration,
        efd::ILogger::kERR3,
        ("%s> This method is Deprecated. Should remove call.", __FUNCTION__));

    return false;
}

//------------------------------------------------------------------------------------------------
AsyncResult RapidIterationSystemService::OnTick()
{
    // SimDebugger piggybacks on the RapidIteration tick
    if (SimDebugger::Instance())
        SimDebugger::Instance()->OnTick();

    if (!m_flatModelLocationList.size() && !m_assetLocationList.size())
    {
        return AsyncResult_Pending;
    }
    if (m_pendingFlatModelLoads.size())
    {
        // flat models take priority
        return AsyncResult_Pending;
    }

    // Note: we prioritize FlatModel reloads over world and script reloads. This ensures
    // consistent behavior with 3.0 where these assets were all reloaded in the same tick. Ideally,
    // the AssetController would return assets in the order in which they changed on disk,
    // eliminating the need to maintain separate lists here.
    // DT32016 Remove this once AssetController is updated to return results in chronological order.
    if (!HandleReloads(m_flatModelLocationList))
    {
        HandleReloads(m_assetLocationList);
    }

    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
bool RapidIterationSystemService::HandleReloads(AssetLocationList& locationList)
{
    if (!locationList.size())
    {
        return false;
    }

    for (AssetLocationList::iterator it = locationList.begin(); it != locationList.end(); ++it)
    {
        utf8string url = it->m_locationData.url;
        utf8string native_url = efd::PathUtils::PathMakeNative (url);
        utf8string name = it->m_locationData.name;
        utf8string assetId = it->m_assetId;

        // For Lua behavior updates, tell the Scheduler to reload the affected module
        if (it->m_locationData.tagset.find("lua-behavior") != utf8string::npos)
        {
            egf::SchedulerPtr spScheduler = m_pServiceManager->GetSystemServiceAs<Scheduler>();
            if (spScheduler != NULL)
            {
                egf::ISchedulerScripting* schedulerLua = spScheduler->GetScriptingRuntime("Lua");
                if (schedulerLua != NULL)
                {
                    schedulerLua->ReloadModule(name.c_str());
                }
            }
        }

        // For model file updates, we need to ensure the FlatModelManager is aware of the model file
        else if (it->m_locationData.tagset.find("emergent-flat-model") != utf8string::npos)
        {
            FlatModelManager* pFlatModelManager = m_pServiceManager->GetSystemServiceAs<FlatModelManager>();
            EE_ASSERT(pFlatModelManager);

            if (pFlatModelManager->FindModel(name) == 0)
            {
                // don't know this model so don't try to reload it.
                continue;
            }

            pFlatModelManager->UpdateModelLocation(name, native_url);

            m_pendingFlatModelLoads.insert(
                utf8string(Formatted, "urn:emergent-flat-model:%s", name.c_str()));

            pFlatModelManager->ReloadModel(name);
        }

        else if (it->m_locationData.tagset.find("emergent-world") != utf8string::npos)
        {
            EntityLoaderService* pELS =
                m_pServiceManager->GetSystemServiceAs<EntityLoaderService>();
            EE_ASSERT(pELS);

            pELS->ReloadWorld(assetId);
        }
    }

    locationList.clear();

    return true;
}

//------------------------------------------------------------------------------------------------
AsyncResult RapidIterationSystemService::OnShutdown()
{
    DestroySimDebugger();

    AssetFactoryManager* pAFM = m_pServiceManager->GetSystemServiceAs<AssetFactoryManager>();
    EE_ASSERT(pAFM);
    pAFM->UnregisterAssetCallback(FlatModelFactory::CLASS_ID, GetPrivateCategory());

    // unsubscribe messages
    if (m_spMessageService)
    {
        m_spMessageService->Unsubscribe(this, m_asset_lookup_callback);
        m_spMessageService->Unsubscribe(this, GetPrivateCategory());
        m_spMessageService = NULL;
    }
    if (m_spToolsMsgService)
    {
        Category netLocalCategory = MessageService::GetServicePublicCategory(INetService::CLASS_ID);
        m_spToolsMsgService->Unsubscribe(this, netLocalCategory);

        m_spToolsMsgService->Unsubscribe(this, GetToolbenchPublicCategory());
        m_spToolsMsgService = NULL;
    }

    m_spNetService = NULL;

    return AsyncResult_Complete;
}


//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::HandleAssignNetIDMessage(
    const efd::AssignNetIDMessage* pAssignNetIDMessage,
    efd::Category targetCategory)
{
    EE_UNUSED_ARG(pAssignNetIDMessage);
    efd::AssetApplicationInfoMsgPtr spAssetApplicationInfoMsg =
        EE_NEW efd::AssetApplicationInfoMsg;

    ConnectionID cid = m_spNetService->GetChannelManagerConnectionID(QOS_RELIABLE);
    spAssetApplicationInfoMsg->SetAppPort(cid.GetLocalPort());
    spAssetApplicationInfoMsg->SetApplicationName(m_pServiceManager->GetVirtualProcessName());
    spAssetApplicationInfoMsg->SetApplicationType("GAME");
    if (SimDebugger::Instance())
        spAssetApplicationInfoMsg->SetExtraInfo("SimDebugger");

    Category AssetControllerCategory(UniversalID::ECU_Any, kNetID_Any, kBASEID_AssetController);
    m_spToolsMsgService->Send(spAssetApplicationInfoMsg, AssetControllerCategory);
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::HandleGameControlMessage(
    const efd::IMessage* pMessage,
    efd::Category targetChannel)
{
    switch (pMessage->GetClassID())
    {
    case kMSGID_WorldResetRequest:
        {
            // forward this to the entity loader service to handle
            EntityLoaderService* pLoader =
                m_pServiceManager->GetSystemServiceAs<EntityLoaderService>();
            EE_ASSERT(pLoader);

            pLoader->OnWorldResetRequest();
        }
        break;

    case kMSGID_GamePauseRequest:
        {
            // pause the Scheduler
            Scheduler* pScheduler = m_pServiceManager->GetSystemServiceAs<Scheduler>();
            if (pScheduler)
                pScheduler->PauseScheduler(true);
        }
        break;

    case kMSGID_GameResumeRequest:
        {
            // pause the Scheduler
            Scheduler* pScheduler = m_pServiceManager->GetSystemServiceAs<Scheduler>();
            if (pScheduler)
                pScheduler->PauseScheduler(false);
        }
        break;
    }
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::HandleAssetChange(
    const efd::utf8string& assetId,
    const efd::utf8string& tags)
{
    if ((tags.find("emergent-flat-model") != utf8string::npos) ||
        (tags.find("python-behavior") != utf8string::npos) ||
        (tags.find("lua-behavior") != utf8string::npos) ||
        (tags.find("emergent-world") != utf8string::npos))
    {
        // Just do a locate on the asset to cause the new version to be fetched.
        m_spAssetService->AssetLocate(assetId, m_asset_lookup_callback);
    }
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::HandleAssetLocatorResponse(
    const efd::AssetLocatorResponse* pMessage,
    efd::Category targetChannel)
{
    const AssetLocatorResponse::AssetURLMap& assets = pMessage->GetAssetURLMap();
    AssetLocatorResponse::AssetURLMap::const_iterator it = assets.begin();
    for (; it != assets.end(); ++it)
    {
        AssetLocation location;
        location.m_assetId = it->first;
        location.m_locationData = it->second;

        if (it->second.tagset.find("emergent-flat-model") != utf8string::npos)
        {
            m_flatModelLocationList.push_back(location);
        }
        else
        {
            m_assetLocationList.push_back(location);
        }
    }
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::HandleNameResolutionResponse(
    const efd::NameResolutionMessage* pNameResolutionMessage,
    efd::Category targetChannel)
{
    // connect to the ChannelManager indicated by the response message
    m_spNetService->AutoReconnectToChannelManager (true);
    m_spNetService->ConnectToChannelManager(
        QOS_RELIABLE,
        pNameResolutionMessage->GetHostname(),
        pNameResolutionMessage->GetPort());

    m_spMessageService->Unsubscribe(this, targetChannel);

    NameResolutionService* pNameService =
        m_pServiceManager->GetSystemServiceAs<NameResolutionService>();
    pNameService->CancelLookup(targetChannel);
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::HandleAssetLoadResponse(
    const efd::AssetLoadResponse* pMsg,
    efd::Category targetChannel)
{
    // If it's not a reload message, ignore it.
    if (!pMsg->GetIsReload())
    {
        return;
    }

    if (m_pendingFlatModelLoads.find(pMsg->GetURN()) != m_pendingFlatModelLoads.end())
    {
        m_pendingFlatModelLoads.erase(pMsg->GetURN());
    }
}

//------------------------------------------------------------------------------------------------
const char* RapidIterationSystemService::GetDisplayName() const
{
    return "RapidIterationSystemService";
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::CreateSimDebugger(egf::SimDebugger* pInstance/*=NULL*/)
{
    if (pInstance != NULL)
        SimDebugger::SetInstance(pInstance);
    else
        SimDebugger::SetInstance(EE_NEW SimDebugger());

    if (m_pServiceManager->CheckSystemServiceState(RapidIterationSystemService::CLASS_ID) >
        ServiceManager::kSysServState_Initializing)
    {
        SimDebugger::Instance()->Initialize(m_pServiceManager);
    }
}

//------------------------------------------------------------------------------------------------
void RapidIterationSystemService::DestroySimDebugger()
{
    if (SimDebugger::Instance())
    {
        SimDebugger::Instance()->Shutdown();
        SimDebugger::Release();
    }
}

#endif // !defined(EE_CONFIG_SHIPPING)
