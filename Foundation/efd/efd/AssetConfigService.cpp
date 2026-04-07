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

#include "efdPCH.h"

#include <efd/ConfigManager.h>
#include <efd/AssetConfigService.h>
#include <efd/IAssetServer.h>
#include <efd/efdLogIDs.h>
#include <efd/MessageHandlerBase.h>
#include <efd/ServiceManager.h>


using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(AssetConfigService);

EE_HANDLER_WRAP(
    AssetConfigService,
    HandleConfigResponse,
    AssetConfigurationMessage,
    kMSGID_AssetConfigResponse);

//------------------------------------------------------------------------------------------------
const /*static*/ char* AssetConfigService::kConfigAssetWebSection = "AssetWeb";
const /*static*/ char* AssetConfigService::kConfigAssetLoaderSection = "AssetLoader";

//------------------------------------------------------------------------------------------------
AssetConfigService::AssetConfigService()
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 2140;
}

//------------------------------------------------------------------------------------------------
AssetConfigService::~AssetConfigService()
{
}

//------------------------------------------------------------------------------------------------
const char* AssetConfigService::GetDisplayName() const
{
    return "AssetConfigService";
}

//------------------------------------------------------------------------------------------------
void AssetConfigService::ReadConfiguration()
{
    // Find the config manager service
    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    EE_ASSERT(pConfigManager);

    // Get the root config section
    const ISection* pRoot = pConfigManager->GetConfiguration();

    if (!pRoot)
    {
        EE_LOG(efd::kAssets, efd::ILogger::kERR0,
            ("AssetConfigService::OnInit could not find root section in config."));
        return;
    }

    // Get the "AssetWeb" config section
    const ISection* pWebSection = pRoot->FindSection(kConfigAssetWebSection);
    if (pWebSection)
    {
        // Check the flag to preload graphs
        m_preloadGraphs = pWebSection->IsTrue("PreloadGraphs", true);

        // Store the asset-web path
        m_assetWebRoot = pWebSection->FindValue("Path");
        if (!m_assetWebRoot.empty())
        {
            EE_LOG(efd::kAssets, efd::ILogger::kLVL2,
                ("AssetConfigService::OnInit configured for Asset Web location: %s",
                m_assetWebRoot.c_str()));
        }
        else
        {
            EE_LOG(efd::kAssets, efd::ILogger::kERR3,
                ("AssetConfigService::OnInit no AssetWeb.Path set. Assets may fail."));
        }

        // Load any tests specified in the configuration
        for (UInt32 index = 0; ; ++index)
        {
            utf8string strValueName(Formatted, "QueryTest%d", index);
            utf8string strValue = pWebSection->FindValue(strValueName);
            if (strValue.empty())
            {
                // Empty value means non-existent, but numbering is supposed to start at one so
                // we keep going if item 0 is missing
                if (index > 0)
                    break;
            }
            else
            {
                m_queryTests.push_back(strValue);
            }
        }
        for (UInt32 index = 0; ; ++index)
        {
            utf8string strValueName(Formatted, "QTestResult%d", index);
            utf8string strValue = pWebSection->FindValue(strValueName);
            if (strValue.empty())
            {
                // Empty value means non-existent, but numbering is supposed to start at one so
                // we keep going if item 0 is missing
                if (index > 0)
                    break;
            }
            else
            {
                m_queryTestResults.push_back(strValue);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
efd::SyncResult AssetConfigService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    pDependencyRegistrar->AddDependency<IConfigManager>();
    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult AssetConfigService::OnInit()
{
    // Find the local message service
    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    EE_ASSERT(m_spMessageService);

    // Find the Tools message service
    m_spToolsMsgService =
        m_pServiceManager->GetSystemServiceAs<MessageService>(kCLASSID_ToolsMessageService);
    if (!m_spToolsMsgService)
    {
        m_spToolsMsgService = m_spMessageService;
    }

    // Find other local asset services
    m_pAssetLocator = m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();

    // Initialize values from Config
    ReadConfiguration();

    if (m_pAssetLocator)
    {
        m_spToolsMsgService->Subscribe(this, GetAssetClientServiceCategory(), QOS_RELIABLE);
    }

    // If there is a local asset service, pass configuration information to it now
    OpenAssetWebLocal(m_assetWebRoot);

    // Let other local service subscribers know about the change
    AssetConfigurationMessagePtr notify =
        EE_NEW MessageWrapper<AssetConfigurationMessage, kMSGID_AssetConfigNotify>();
    notify->SetAssetWebPath(m_assetWebRoot);
    m_spMessageService->SendLocal(notify);

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult AssetConfigService::OnTick()
{
    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult AssetConfigService::OnShutdown()
{
    if (m_pAssetLocator)
    {
        m_spToolsMsgService->Unsubscribe(this, GetAssetClientServiceCategory());
    }
    m_spToolsMsgService = NULL;
    m_spMessageService = NULL;

    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void AssetConfigService::RescanAssetWebRoot()
{
    // Must be a client to do this
    EE_ASSERT(m_pAssetLocator);

    AssetConfigurationMessagePtr req = EE_NEW
        MessageWrapper< AssetConfigurationMessage, kMSGID_AssetConfigRequest >();
    req->SetAssetWebPath(m_assetWebRoot);
    req->SetForceRescan(true);
    m_spMessageService->Send(req, GetAssetServerServiceCategory());
}

//------------------------------------------------------------------------------------------------
efd::Bool AssetConfigService::OpenAssetWebLocal(const utf8string& web_root)
{
    m_assetWebRoot = web_root;

    if (m_pAssetLocator && !m_assetWebRoot.empty())
    {
        if (!EE_VERIFY_MESSAGE(m_pAssetLocator->AssetWebOpen(m_assetWebRoot),
            ("Failure: Local asset web metadata invalid. Regenerate with Asset Controller.")))
        {
            EE_LOG(
                efd::kAssets,
                efd::ILogger::kERR0,
                ("Local asset web metadata invalid. Regenerate with Asset Controller."));

            m_pServiceManager->Shutdown();

            return false;
        }

        SendLocalResponse();
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool AssetConfigService::OpenAssetWebRemote(const utf8string& web_root)
{
    // Must be a client to do this
    EE_ASSERT(m_pAssetLocator);

    AssetConfigurationMessagePtr req = EE_NEW
        MessageWrapper<AssetConfigurationMessage, kMSGID_AssetConfigRequest>();
    req->SetAssetWebPath(web_root);
    m_spToolsMsgService->Send(req, GetAssetServerServiceCategory());

    return true;
}

//------------------------------------------------------------------------------------------------
void AssetConfigService::GetQueryTests(efd::vector<utf8string>& queries) const
{
    queries = m_queryTests;
}

//------------------------------------------------------------------------------------------------
void AssetConfigService::GetQueryTestResults(efd::vector<utf8string>& results) const
{
    results = m_queryTestResults;
}

//------------------------------------------------------------------------------------------------
void AssetConfigService::GetConfigValue(const efd::utf8string& key, efd::utf8string& value)
{
    // Find the config manager service
    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    EE_ASSERT(pConfigManager);

    // Get the root config section
    const ISection* pRoot = pConfigManager->GetConfiguration();

    // Get the key-value
    const ISection* pWebSection = pRoot->FindSection(kConfigAssetWebSection);
    if (pWebSection)
        value = pWebSection->FindValue(key);
}

//------------------------------------------------------------------------------------------------
void AssetConfigService::HandleConfigResponse(
    const efd::AssetConfigurationMessage* pMessage,
    efd::Category targetChannel)
{
    // Only a client can send the request and receive this response
    EE_ASSERT(m_pAssetLocator);

    EE_LOG(efd::kAssets, efd::ILogger::kLVL2,
        ("%s> New Remote AssetWeb Root set: %s",
        __FUNCTION__,
         pMessage->GetAssetWebPath().c_str()));
    SendLocalResponse(pMessage);

    EE_UNUSED_ARG(pMessage);
    EE_UNUSED_ARG(targetChannel);
}

//------------------------------------------------------------------------------------------------
void AssetConfigService::SendLocalResponse(const efd::AssetConfigurationMessage* pMessage)
{
    AssetConfigurationMessage* pLocalResp =
        EE_NEW MessageWrapper<AssetConfigurationMessage, kMSGID_AssetConfigResponse>();
    if (pMessage == NULL)
    {
        pLocalResp->SetAssetWebPath(m_assetWebRoot);
        pLocalResp->SetRemoteConfigFlag(false);
    }
    else
    {
        pLocalResp->SetAssetWebPath(pMessage->GetAssetWebPath());
        pLocalResp->SetRemoteConfigFlag(true);
    }
    m_spMessageService->SendLocal(pLocalResp);
}
