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

#include "ecrPCH.h"
#include "NiVersion.h"

#include "TextureRapidIterationService.h"
#include "RenderService.h"

#include <efd/efdMessageIDs.h>
#include <efd/ServiceManager.h>
#include <efd/ecrLogIDs.h>
#include <efd/AssetLocatorService.h>

#include <NiTexture.h>
#include <NiSourceTexture.h>

using namespace efd;
using namespace ecr;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(TextureRapidIterationService);

//------------------------------------------------------------------------------------------------
// Register for asset change related messages:
EE_HANDLER_WRAP(
    TextureRapidIterationService,
    HandleAssetLocatorResponse,
    efd::AssetLocatorResponse,
    kMSGID_AssetLocatorResponse);

//------------------------------------------------------------------------------------------------
TextureRapidIterationService::TextureRapidIterationService()
    : m_assetRefreshCategory(kCAT_INVALID)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 1;
}

//------------------------------------------------------------------------------------------------
TextureRapidIterationService::~TextureRapidIterationService()
{
    // This method intentionally left blank (all shutdown occurs in OnShutdown)
}

//------------------------------------------------------------------------------------------------
const char* TextureRapidIterationService::GetDisplayName() const
{
    return "TextureRapidIterationService";
}

//------------------------------------------------------------------------------------------------
SyncResult TextureRapidIterationService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    EE_UNUSED_ARG(pDependencyRegistrar);

    // Subscribe to messages
    m_spMessageService = m_pServiceManager->GetSystemServiceAs<efd::MessageService>();
    EE_ASSERT(m_spMessageService);

    m_spMessageService->Subscribe(this, kCAT_LocalMessage);

    m_assetRefreshCategory = GenerateAssetResponseCategory();
    m_spMessageService->Subscribe(this, m_assetRefreshCategory);

    // Grab a pointer to other services
    m_spSceneGraphService = m_pServiceManager->GetSystemServiceAs<SceneGraphService>();
    EE_ASSERT(m_spSceneGraphService);

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
AsyncResult TextureRapidIterationService::OnInit()
{
    // Register with the reload manager if it exists
    m_spReloadManager = m_pServiceManager->GetSystemServiceAs<ReloadManager>();
    AddSupportedMimeType("image");

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
AsyncResult TextureRapidIterationService::OnShutdown()
{
    if (m_spMessageService != NULL)
    {
        m_spMessageService->Unsubscribe(this, kCAT_LocalMessage);
        m_spMessageService->Unsubscribe(this, m_assetRefreshCategory);
        m_spMessageService = NULL;
    }

    m_spSceneGraphService = NULL;
    m_spReloadManager = NULL;

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void TextureRapidIterationService::HandleAssetLocatorResponse(
    const efd::AssetLocatorResponse* pMessage,
    efd::Category targetChannel)
{
    efd::utf8string uri = pMessage->GetResponseURI();

    const AssetLocatorResponse::AssetURLMap& assets = pMessage->GetAssetURLMap();

    if (assets.size() == 0)
    {
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR2,
                ("%s: Found 0 assets when looking for URI '%s'.",
                __FUNCTION__, uri.c_str()));
    }

    for (AssetLocatorResponse::AssetURLMap::const_iterator currAsset = assets.begin();
        currAsset != assets.end();
        ++currAsset)
    {
        const efd::utf8string& assetFilePath = currAsset->second.url;

        if (targetChannel == m_assetRefreshCategory)
        {
            RefreshTextureAsset(assetFilePath);
        }
    }
}

//------------------------------------------------------------------------------------------------
void TextureRapidIterationService::HandleAssetChange(
    const efd::utf8string& physicalID,
    const efd::utf8string& tags)
{
    // Check the tags against each valid texture MIME type; if a match is found, start a reload
    for (efd::vector<efd::utf8string>::iterator i = m_supportedMimeTags.begin();
        i != m_supportedMimeTags.end();
        ++i)
    {
        if (tags.find(*i) != efd::utf8string::npos)
        {
            // Do a locate on the asset to cause the new version to be fetched.
            AssetLocatorServicePtr spAssetService =
                m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
            EE_ASSERT(spAssetService);

            spAssetService->AssetLocate(physicalID, m_assetRefreshCategory);

            return;
        }
    }
}

//------------------------------------------------------------------------------------------------
void TextureRapidIterationService::AddSupportedMimeType(const char* pMimeString)
{
    if (m_spReloadManager)
    {
        m_supportedMimeTags.push_back(pMimeString);
        m_spReloadManager->RegisterAssetChangeHandler(pMimeString, this);
    }
}

//------------------------------------------------------------------------------------------------
void TextureRapidIterationService::RefreshTextureAsset(const efd::utf8string& filename)
{
    const char* pFilename = filename.c_str();

    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    // Find the texture in the texture list
    NiTexture::LockTextureList();

    size_t numMatches = 0;

    NiTexture* pkNode = NiTexture::GetListHead();
    while (pkNode != NULL)
    {
        NiSourceTexture* pkTexture = NiDynamicCast(NiSourceTexture, pkNode);
        if (pkTexture != NULL)
        {
            const FixedString& texFilename = pkTexture->GetPlatformSpecificFilename();

            if (texFilename.EqualsNoCase(pFilename))
            {
                // Matched source texture, replace it.
                numMatches++;

                pkRenderer->LockRenderer();
                {
                    // Delete the old renderer data and/or pixel data
                    pkRenderer->PurgeTexture(pkTexture);
                    pkTexture->DestroyAppPixelData();

                    // Force the renderer data to be recreated, causing a reload from the file
                    pkRenderer->CreateSourceTextureRendererData(pkTexture);
                }
                pkRenderer->UnlockRenderer();
            }
        }

        pkNode = pkNode->GetListNext();
    }

    NiTexture::UnlockTextureList();

    // If we found anything other than exactly 1 match, it might be important for the user to know
    if (numMatches == 0)
    {
        // This is a low level as it will occur anytime an image is touched in the asset web,
        // but isn't currently loaded in the title (e.g., editing an image for a different level
        // or creating new content)
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kLVL3,
                ("%s: Failed to find any matching NiSourceTexture objects for "
                "physical texture asset '%s'.",
                __FUNCTION__, pFilename));
    }
    else if (numMatches > 1)
    {
        // This warning occurs if there are multiple loaded NiSourceTexture objects with the same
        // physical filename, which most likely indicates a lack of sharing or some other issue
        // with the NiTexturePalette object used when loading NIFs.
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kLVL1,
            ("%s: Warning, found more than one NiSourceTexture instance for physical "
            "texture asset '%s'.",
                __FUNCTION__, pFilename));
    }

    // Notify the rendering service of a change if it exists (so WorldBuilder, etc... can know
    // to redraw itself)
    if (numMatches > 0)
    {
        RenderService* pRenderService = m_pServiceManager->GetSystemServiceAs<RenderService>();
        if (pRenderService != NULL)
        {
            pRenderService->InvalidateRenderContexts();
        }
    }
}

//------------------------------------------------------------------------------------------------
