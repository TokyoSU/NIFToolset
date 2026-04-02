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

#include <efd/ReloadManager.h>
#include <efd/IAssetLocatorService.h>
#include <efd/AssetLocatorRequest.h>
#include <efd/MessageHandlerBase.h>
#include <efd/ServiceManager.h>


using namespace efd;

EE_HANDLER_WRAP(
    ReloadManager,
    HandleAssetLocatorResponse,
    AssetLocatorResponse,
    kMSGID_AssetServerChangeNotify);

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ReloadManager);

//------------------------------------------------------------------------------------------------
ReloadManager::ReloadManager()
    : m_spMessageService(0)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 2030;
}

//------------------------------------------------------------------------------------------------
ReloadManager::~ReloadManager()
{
}

//------------------------------------------------------------------------------------------------
const char* ReloadManager::GetDisplayName() const
{
    return "ReloadManager";
}

//------------------------------------------------------------------------------------------------
efd::Category ReloadManager::GetReloadManagerCategory()
{
    // This category is used exclusively by the AssetLocatorService to forward interesting
    // messages to the ReloadManager -- use the p2p expected channel usage.
    return Category(
            UniversalID::ECU_EventChannel,
            kNetID_ISystemService,
            ReloadManager::CLASS_ID);
}

//------------------------------------------------------------------------------------------------
efd::SyncResult ReloadManager::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    // Best practice for message subscribers is to depend on the message service.
    pDependencyRegistrar->AddDependency<MessageService>();
    return efd::SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult ReloadManager::OnInit()
{
    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    EE_ASSERT(m_spMessageService);

    RegisterMessageWrapperFactory<AssetLocatorResponse, kMSGID_AssetServerChangeNotify>(
        m_spMessageService);

    // Don't subscribe to the asset client service public category directly. Messages coming
    // into the ReloadManager are pumped here via the AssetLocatorService. When the ALS receives
    // an asset update notification it forwards that notification to the ReloadManager on the
    // reload manager private category.
    m_spMessageService->Subscribe(this, GetReloadManagerCategory());

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult ReloadManager::OnTick()
{
    efd::AssetLocatorResponse::AssetURLMap::const_iterator it = m_changedAssets.begin();
    for (; it != m_changedAssets.end(); ++it)
    {
        InvokeHandlers(it->first, it->second);
    }
    m_changedAssets.clear();
    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult ReloadManager::OnShutdown()
{
    if (m_spMessageService)
    {
        m_spMessageService->Unsubscribe(this, GetReloadManagerCategory());
    }

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void ReloadManager::HandleAssetLocatorResponse(
    const efd::AssetLocatorResponse* pMessage,
    efd::Category targetChannel)
{
    // The asset locator service has detected an asset change and forwarded the message here.
    // Track the list of changed assets here so we can pass this information to all registered
    // asset change handlers during OnTick().
    const AssetLocatorResponse::AssetURLMap& assets = pMessage->GetAssetURLMap();
    AssetLocatorResponse::AssetURLMap::const_iterator it = assets.begin();
    for (; it != assets.end(); ++it)
    {
        // add the changed asset only if we haven't seen it already
        if (m_changedAssets.find(it->first) == m_changedAssets.end())
        {
            m_changedAssets[it->first] = it->second;

            EE_LOG(
                efd::kAssets,
                efd::ILogger::kLVL3,
                ("%s> %s (%s) -> %s", __FUNCTION__,
                it->first.c_str(), it->second.tagset.c_str(), it->second.url.c_str()));
        }
    }
}


//------------------------------------------------------------------------------------------------
void ReloadManager::RegisterAssetChangeHandler(
    const efd::utf8string& tag,
    efd::AssetChangeHandler* handler)
{
    AssetChangeHandlerList &list = m_changeHandlers[tag];
    AssetChangeHandlerList::const_iterator it = list.find(handler);
    if (it == list.end())
    {
        list.push_back(handler);
    }
}

//------------------------------------------------------------------------------------------------
void ReloadManager::UnregisterAssetChangeHandler(
    const efd::utf8string& tag,
    efd::AssetChangeHandler* handler)
{
    AssetChangeHandlerList &list = m_changeHandlers[tag];
    AssetChangeHandlerList::iterator i = list.find(handler);
    if (i != list.end())
    {
        list.erase(i);
    }
}

//------------------------------------------------------------------------------------------------
void ReloadManager::InvokeHandlers(
    const efd::utf8string &uuid,
    const efd::AssetLocatorResponse::AssetLoc& assetLoc)
{
    EE_LOG(
        efd::kAssets,
        efd::ILogger::kLVL3,
        ("%s> Invoke registered handlers for %s", __FUNCTION__,
        assetLoc.tagset.c_str()));

    // break the tagset into individual tags. The prefix is 'URN', which we skip.
    size_t start = assetLoc.tagset.find(":");
    size_t end = assetLoc.tagset.find(":", start + 1);
    while (start != utf8string::npos)
    {
        utf8string tag = assetLoc.tagset.substr(start + 1, end - start - 1);

        AssetChangeHandlerMap::iterator it = m_changeHandlers.find(tag);
        if (it != m_changeHandlers.end())
        {
            AssetChangeHandlerList &list = it->second;
            AssetChangeHandlerList::iterator handlerIt = list.begin();
            for (; handlerIt != list.end(); ++handlerIt)
            {
                (*handlerIt)->HandleAssetChange(uuid, tag);
            }
        }

        start = end;
        end = assetLoc.tagset.find(":", start + 1);
    }
}
