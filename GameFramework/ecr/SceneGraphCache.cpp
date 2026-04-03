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

#include <ecr/SceneGraphCache.h>

#include <ecr/SceneGraphCacheResponse.h>
#include <efd/ecrLogIDs.h>
#include <efd/AssetLoadRequest.h>
#include <ecr/NIFFactory.h>
#include <ecr/NIFFactoryResponse.h>

using namespace efd;
using namespace ecr;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(SceneGraphCache);

// Register for asset locator response messages:
EE_HANDLER_WRAP(SceneGraphCache, HandleAssetLocatorResponse,
    AssetLocatorResponse, kMSGID_AssetLocatorResponse);

// Register for asset load responses.
EE_HANDLER(SceneGraphCache, HandleAssetLoadResponse, NIFFactoryResponse);

const SceneGraphCache::SceneGraphCacheHandle SceneGraphCache::k_invalidHandle = 0;

//------------------------------------------------------------------------------------------------
SceneGraphCache::SceneGraphCache(NiTexturePalette* pTexturePalette)
    : m_pMessageService(NULL)
    , m_pAssetLocatorService(NULL)
    , m_pAssetFactoryManager(NULL)
    , m_spPalette(pTexturePalette)
    , m_subscribed(false)
{
}

//------------------------------------------------------------------------------------------------
SceneGraphCache::~SceneGraphCache()
{
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::Init(
    efd::MessageService* pMessageService,
    efd::AssetLocatorService* pAssetLocator,
    efd::AssetFactoryManager* pAssetFactory)
{
    if (!m_spPalette)
        m_spPalette = EE_NEW NiDefaultTexturePalette();

    m_pMessageService = pMessageService;
    m_pAssetLocatorService = pAssetLocator;
    m_pAssetFactoryManager = pAssetFactory;

    m_assetLoadCategory = m_pMessageService->GetGloballyUniqueCategory();
    m_assetReloadCategory = m_pMessageService->GetGloballyUniqueCategory();
    m_assetLocateCategory = GenerateAssetResponseCategory();
    m_assetRefreshCategory = GenerateAssetResponseCategory();
    m_assetReloadResponseCategory = m_pMessageService->GetGloballyUniqueCategory();

    NIFFactory* pFactory = EE_NEW NIFFactory(m_spPalette);
    m_pAssetFactoryManager->RegisterAssetFactory(m_assetLoadCategory, pFactory);
    m_pAssetFactoryManager->RegisterAssetFactory(m_assetReloadCategory, pFactory);
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::Shutdown()
{
    m_pAssetFactoryManager->UnregisterAssetFactory(m_assetLoadCategory);
    m_pAssetFactoryManager->UnregisterAssetFactory(m_assetReloadCategory);

    // Unregister for messages
    if (m_subscribed)
    {
        m_pMessageService->Unsubscribe(this, m_assetLoadCategory);
        m_pMessageService->Unsubscribe(this, m_assetReloadCategory);
        m_pMessageService->Unsubscribe(this, m_assetLocateCategory);
        m_pMessageService->Unsubscribe(this, m_assetRefreshCategory);
        m_subscribed = false;
    }

    // Empty the cache
    efd::map<utf8string, SceneGraphCacheDataPtr>::iterator iter = m_cache.begin();
    while (iter != m_cache.end())
    {
        iter->second = 0;
        ++iter;
    }
    m_cache.clear();

    // Clear any outstanding requests
    efd::map<utf8string, PendingRequestDataPtr>::iterator pendingIter =  m_pendingRequests.begin();
    while (pendingIter != m_pendingRequests.end())
    {
        pendingIter->second = 0;
        ++pendingIter;
    }
    m_pendingRequests.clear();

    efd::map<utf8string, FilePathVector*>::iterator identIter = m_identifierCache.begin();
    while (identIter != m_identifierCache.end())
    {
        EE_DELETE identIter->second;
        ++identIter;
    }
    m_identifierCache.clear();

    m_spPalette = 0;
}

//------------------------------------------------------------------------------------------------
bool SceneGraphCache::CacheSceneGraph(
    const efd::utf8string& identifier,
    const efd::Category responseCategory,
    ISceneGraphCacheRequestData* pResponseData,
    vector<SceneGraphCacheHandle>& handles)
{
    FilePathVector* filePaths = 0;
    SceneGraphCacheDataPtr spData = 0;

    filePaths = FindFilePaths(identifier);

    if (filePaths)
    {
        bool foundAll = true;
        for (UInt32 ui = 0; ui < filePaths->size(); ++ui)
        {
            if (!m_cache.find((*filePaths)[ui], spData))
            {
                foundAll = false;
                handles.clear();

                /// We have seen this identifier before, but some of its files have been uncached.
                /// For simplicity, pretend we have never seen it before.
                break;
            }
            handles.push_back(spData);
        }
        if (foundAll)
        {
            return true;
        }
    }

    if (identifier.find("urn:") == utf8string::npos)
    {
        // See if we already have it cached
        efd::map<utf8string, SceneGraphCacheDataPtr>::iterator cacheIter =
            m_cache.find(identifier);
        if (cacheIter != m_cache.end())
        {
            handles.clear();
            handles.push_back(cacheIter->second);
            return true;
        }

        RequestFilePath(identifier, identifier, responseCategory, pResponseData);

        return false;
    }

    RequestURN(identifier, responseCategory, pResponseData);

    return false;
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::RequestFilePath(
    const efd::utf8string& identifier,
    const efd::utf8string& fileName,
    const efd::Category responseCategory,
    ISceneGraphCacheRequestData* pResponseData)
{
    // Create a request data object
    RequestData* pRequestData = EE_NEW RequestData;
    pRequestData->m_identifier = identifier;
    pRequestData->m_responseCategory = responseCategory;
    pRequestData->m_pResponseData = pResponseData;

    // Look for an existing pending request, and create it if there is not one
    PendingRequestDataPtr spPendingData = 0;
    if (!m_pendingRequests.find(identifier, spPendingData) || !spPendingData)
    {
        spPendingData = EE_NEW PendingRequestData;
        spPendingData->m_status = AS_Loading;
        spPendingData->m_identifier = identifier;
        m_pendingRequests[identifier] = spPendingData;
        spPendingData->m_filePaths.push_back(fileName);
        spPendingData->m_requestList.push_back(pRequestData);
    }
    else
    {
        EE_ASSERT(spPendingData->m_filePaths.size() == 1);

        // Add this request to the set for this identifier
        if (spPendingData->m_filePaths[0] == fileName)
        {
            // The name hasn't changed while processing
            spPendingData->m_requestList.push_back(pRequestData);
            return;
        }
        else
        {
            // The name for this identifier has changed while still loading.
            // Not sure how this can happen, but if it does we cancel the existing load
            // for the identifier and create a new one.

            // First, get the file data. We should have it but handle it if we don't.
            SceneGraphCacheDataPtr spPendingFile = 0;
            if (!m_pendingFiles.find(spPendingData->m_filePaths[0], spPendingFile) ||
                !spPendingFile)
            {
                // Change the stored file name
                spPendingData->m_filePaths.clear();
                spPendingData->m_filePaths.push_back(fileName);
                spPendingData->m_requestList.push_back(pRequestData);

                // Fall through to make the missing file pending data
            }
            else
            {
                // Find the request in the file data and remove it. This prevents us
                // from sending messages for the over-written file
                for (efd::list<PendingRequestDataPtr>::iterator requestIter =
                    spPendingFile->m_requestList.begin();
                    requestIter != spPendingFile->m_requestList.end();
                    requestIter++)
                {
                    if (spPendingData == *requestIter)
                    {
                        spPendingFile->m_requestList.erase(requestIter);
                        break;
                    }
                }

                // Send failed messages for all the existing requests for this identifier
                for (efd::list<RequestDataPtr>::iterator requestIter =
                    spPendingData->m_requestList.begin();
                    requestIter != spPendingData->m_requestList.end();
                    ++requestIter)
                {
                    SceneGraphCacheResponse* pCacheResp = EE_NEW SceneGraphCacheResponse(
                        (*requestIter)->m_identifier,
                        (*requestIter)->m_responseCategory,
                        (*requestIter)->m_pResponseData,
                        AssetCacheResponse::ACR_FileNotFound,
                        AssetCacheResponse::ACT_CacheAdd);

                    m_pMessageService->SendLocal(pCacheResp, (*requestIter)->m_responseCategory);

                    // release the RequestDataPtr
                    *requestIter = 0;
                }

                // Clear the old request
                m_pendingRequests[identifier] = 0;
                m_pendingRequests.erase(identifier);

                // Create a new one
                spPendingData = EE_NEW PendingRequestData;
                spPendingData->m_status = AS_Loading;
                spPendingData->m_identifier = identifier;
                m_pendingRequests[identifier] = spPendingData;
                spPendingData->m_filePaths.push_back(fileName);
                spPendingData->m_requestList.push_back(pRequestData);
            }
        }
    }

    // Look for an existing request for the file name, and create one if it does not exist
    SceneGraphCacheDataPtr spPendingFile = 0;
    if (!m_pendingFiles.find(fileName, spPendingFile))
    {
        spPendingFile = EE_NEW SceneGraphCacheData;
        spPendingFile->m_llid.clear();
        spPendingFile->m_physicalID = identifier;
        spPendingFile->m_filePath = fileName;
        spPendingFile->m_status = AS_Loading;
        m_pendingFiles[fileName] = spPendingFile;

        if (!m_subscribed)
            Subscribe();

        AssetLoadRequest* pLoadRequest =
            EE_NEW AssetLoadRequest(identifier, m_assetLoadCategory, fileName);
        m_pMessageService->SendLocal(pLoadRequest, AssetFactoryManager::MessageCategory());
    }

    // Add this request to the list for the file.
    spPendingFile->m_requestList.push_back(spPendingData);
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::RequestURN(
    const efd::utf8string& identifier,
    const efd::Category responseCategory,
    ISceneGraphCacheRequestData* pResponseData)
{
    // Create a request data object
    RequestData* pRequestData = EE_NEW RequestData;
    pRequestData->m_identifier = identifier;
    pRequestData->m_responseCategory = responseCategory;
    pRequestData->m_pResponseData = pResponseData;

    // Look for an existing pending request, and create it if there is not one
    PendingRequestDataPtr spPendingData = 0;
    if (!m_pendingRequests.find(identifier, spPendingData))
    {
        spPendingData = EE_NEW PendingRequestData;
        spPendingData->m_status = AS_Locating;
        spPendingData->m_identifier = identifier;
        m_pendingRequests[identifier] = spPendingData;

        if (!m_subscribed)
            Subscribe();

        // Locate the file names for this request
        m_pAssetLocatorService->AssetLocate(identifier, m_assetLocateCategory);
    }

    // Add this request to the set for this identifier
    spPendingData->m_requestList.push_back(pRequestData);
}

//------------------------------------------------------------------------------------------------
bool SceneGraphCache::UnCacheSceneGraph(SceneGraphCacheHandle handle)
{
    /// Check it's in the cache
    efd::map<utf8string, SceneGraphCacheDataPtr>::iterator cacheIter =
        m_cache.find(handle->m_filePath);
    if (cacheIter == m_cache.end())
        return false;

    // Check for ref count of three. One for the caller of this function, one for the
    // handle argument, and one for the cache.
    if (handle->GetRefCount() == 3)
    {
        m_cache.erase(cacheIter);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool SceneGraphCache::ReloadSceneGraph(
    SceneGraphCacheHandle handle,
    efd::Category responseCategory,
    ISceneGraphCacheRequestData* pResponseData)
{
    /// Check it's in the cache
    efd::map<utf8string, SceneGraphCacheDataPtr>::iterator cacheIter =
        m_cache.find(handle->m_filePath);
    if (cacheIter == m_cache.end())
        return false;

    // Create a request data object so we remember who to respond to
    RequestData* pData = EE_NEW RequestData;
    pData->m_identifier = handle->m_filePath;
    pData->m_responseCategory = responseCategory;
    pData->m_pResponseData = pResponseData;

    PendingRequestData* pPendingData = EE_NEW PendingRequestData;
    pPendingData->m_requestList.push_back(pData);

    handle->m_requestList.push_back(pPendingData);

    // The asset is being reloaded
    handle->m_status = AS_Reloading;

    if (!m_subscribed)
        Subscribe();

    AssetLoadRequest* pLoadRequest =
        EE_NEW AssetLoadRequest(handle->m_filePath, m_assetReloadCategory, handle->m_filePath);
    m_pMessageService->SendLocal(pLoadRequest, AssetFactoryManager::MessageCategory());

    return true;
}

//------------------------------------------------------------------------------------------------
bool SceneGraphCache::ReplaceSceneGraph(
    SceneGraphCacheHandle handle,
    const efd::utf8string& newFileName,
    efd::Category responseCategory,
    ISceneGraphCacheRequestData* pResponseData)
{
    /// Check it's in the cache
    efd::map<utf8string, SceneGraphCacheDataPtr>::iterator cacheIter =
        m_cache.find(handle->m_filePath);
    if (cacheIter == m_cache.end())
        return false;

    // Request a file name load.
    RequestFilePath(handle->m_physicalID, newFileName, responseCategory, pResponseData);

    return true;
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::HandleAssetChange(
    const efd::utf8string& physicalID,
    const efd::utf8string& tags)
{
    EE_UNUSED_ARG(tags);

    // Just do a locate on the asset to cause the new version to be fetched.
    // Even if we already know this lookup, we need to wait for the AssetLocatorService
    // to let us know that the asset is ready.
    if (tags.find("gamebryo-scenegraph") != efd::utf8string::npos)
    {
        m_pAssetLocatorService->AssetLocate(physicalID, m_assetRefreshCategory);
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::HandleReloadRequest(const efd::AssetLocatorResponse* pMessage)
{
    // Get the original request URI
    const AssetLocatorResponse::AssetURLMap& assets = pMessage->GetAssetURLMap();

    if (assets.size() == 0)
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR1,
            ("%s: Found 0 assets when trying to reload '%s'. Keeping the old asset.",
            __FUNCTION__,
            pMessage->GetResponseURI().c_str()));
        return;
    }

    if (assets.size() > 1)
    {
        EE_LOG(efd::kGamebryoGeneral0,
            efd::ILogger::kERR1,
            ("%s: Found %d assets when trying to reload '%s'. "
            "Ignoring all but the first.",
            __FUNCTION__,
            assets.size(),
            pMessage->GetResponseURI().c_str()));
    }

    AssetLocatorResponse::AssetURLMap::const_iterator assetIterator = assets.begin();
    const efd::utf8string& assetID = assetIterator->first;

    // Look through the cache for a matching physical ID or file name
    SceneGraphCacheDataPtr spCacheData = 0;
    FilePathVector* filePaths;
    if (m_identifierCache.find(assetID, filePaths))
    {
        if (filePaths->size() == 0)
        {
            EE_LOG(efd::kGamebryoGeneral0,
                efd::ILogger::kLVL3,
                ("%s: Found ID %s but no file paths. "
                "Assuming we have a pending load and not reloading.",
                __FUNCTION__,
                assetID.c_str()));
            return;
        }
        if (filePaths->size() > 1)
        {
            EE_LOG(efd::kGamebryoGeneral0,
                efd::ILogger::kERR1,
                ("%s: Found %d assets when trying to reload '%s'. "
                "Doing nothing, as this should never occur.",
                __FUNCTION__,
                filePaths->size(),
                pMessage->GetResponseURI().c_str()));
            return;
        }

        EE_VERIFY(m_cache.find((*filePaths)[0], spCacheData));
    }
    else
    {
        // We need to iterate looking for a physical ID or file path match, because the
        // physical ID might now be representing a different file path.
        efd::map<efd::utf8string, SceneGraphCacheDataPtr>::iterator cacheIter = m_cache.begin();
        while (cacheIter != m_cache.end())
        {
            spCacheData = cacheIter->second;
            if (spCacheData->m_physicalID == assetID ||
                spCacheData->m_filePath == assetIterator->second.url)
            {
                break;
            }

            ++cacheIter;
       }

        if (cacheIter == m_cache.end())
        {
            EE_LOG(efd::kGamebryoGeneral0,
                efd::ILogger::kLVL3,
                ("%s: Found no ID %s in the cache.",
                __FUNCTION__,
                assetID.c_str()));
            return;
        }
    }

    EE_ASSERT(spCacheData);

    if (assetIterator->second.url == spCacheData->m_filePath)
    {
        // Existing file, so treat it as a reload.
        // Create a request data object so we remember who to respond to
        RequestData* pData = EE_NEW RequestData;
        pData->m_identifier = spCacheData->m_filePath;
        pData->m_responseCategory = m_assetReloadResponseCategory;
        pData->m_pResponseData = NULL;

        PendingRequestData* pPendingData = EE_NEW PendingRequestData;
        pPendingData->m_requestList.push_back(pData);

        spCacheData->m_requestList.push_back(pPendingData);

        // The asset is being reloaded
        spCacheData->m_status = AS_Reloading;

        if (!m_subscribed)
            Subscribe();

        AssetLoadRequest* pLoadRequest = EE_NEW AssetLoadRequest(
            spCacheData->m_filePath,
            m_assetReloadCategory,
            spCacheData->m_filePath);
        m_pMessageService->SendLocal(pLoadRequest, AssetFactoryManager::MessageCategory());
    }
    else
    {
        // New file path, so start again from scratch
        RequestFilePath(
            spCacheData->m_physicalID,
            assetIterator->second.url,
            m_assetReloadResponseCategory,
            NULL);
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::HandleAssetLocatorResponse(
    const efd::AssetLocatorResponse* pMessage,
    efd::Category targetChannel)
{
    // Check for refrsh request
    if (targetChannel == m_assetRefreshCategory)
    {
        HandleReloadRequest(pMessage);
        return;
    }

    const utf8string& identifier = pMessage->GetResponseURI();
    const AssetLocatorResponse::AssetURLMap& assets = pMessage->GetAssetURLMap();

    // Find our request data
    PendingRequestDataPtr spPendingData = 0;
    if (!m_pendingRequests.find(identifier, spPendingData))
    {
        // The user must have cancelled the request
        return;
    }
    EE_ASSERT(spPendingData);
    EE_ASSERT(spPendingData->m_status == AS_Locating);

    // No found assets requires a response message for all requests
    if (assets.size() == 0)
    {
        for (efd::list<RequestDataPtr>::iterator requestIter =
            spPendingData->m_requestList.begin();
            requestIter != spPendingData->m_requestList.end();
            ++requestIter)
        {
            // Only send the response if this was not a reload.
            if ((*requestIter)->m_responseCategory != m_assetReloadResponseCategory)
            {
                SceneGraphCacheResponse* pCacheResp = EE_NEW SceneGraphCacheResponse(
                    (*requestIter)->m_identifier,
                    (*requestIter)->m_responseCategory,
                    (*requestIter)->m_pResponseData,
                    AssetCacheResponse::ACR_AssetNotFound,
                    AssetCacheResponse::ACT_CacheAdd);

                m_pMessageService->SendLocal(pCacheResp, (*requestIter)->m_responseCategory);
            }

            // release the RequestDataPtr
            *requestIter = 0;
        }

        m_pendingRequests[identifier] = 0;
        m_pendingRequests.erase(identifier);

        return;
    }

    // Work through the file paths found
    AssetLocatorResponse::AssetURLMap::const_iterator assetIter;
    for (assetIter = assets.begin(); assetIter != assets.end(); assetIter++)
    {
        // Add the file to the set we need for this request
        spPendingData->m_filePaths.push_back(assetIter->second.url);

        SceneGraphCacheDataPtr spFileData = 0;
        if (m_cache.find(assetIter->second.url, spFileData))
        {
            // This file is already cached, so we can increment the set of found files for
            // the requests.
            ++(spPendingData->m_cachedCount);
        }
        else
        {
            if (!m_pendingFiles.find(assetIter->second.url, spFileData))
            {
                // We're here if we know nothing of this file. So we need to request it.
                spFileData = EE_NEW SceneGraphCacheData;
                spFileData->m_status = AS_Loading;
                spFileData->m_llid = assetIter->second.llid;
                spFileData->m_physicalID = assetIter->first;
                spFileData->m_filePath = assetIter->second.url;
                m_pendingFiles[spFileData->m_filePath] = spFileData;

                AssetLoadRequest* pLoadRequest = EE_NEW AssetLoadRequest(
                    assetIter->second.url,
                    m_assetLoadCategory,
                    assetIter->second.url);
                m_pMessageService->SendLocal(pLoadRequest, AssetFactoryManager::MessageCategory());

            }
            spFileData->m_requestList.push_back(spPendingData);
        }
    }

    /// Check in case we have found all the files in the cache
    if (CheckCompleteRequest(spPendingData))
        return;

    /// Not done, so must be loading.
    spPendingData->m_status = AS_Loading;
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::HandleAssetLoadResponse(
    const NIFFactoryResponse* pResponse,
    efd::Category targetChannel)
{
    // Find the request
    SceneGraphCacheDataPtr spData = 0;
    if (targetChannel == m_assetLoadCategory)
    {
        if (!m_pendingFiles.find(pResponse->GetAssetPath(), spData))
        {
            // The request may have been cancelled. Just exit.
            return;
        }
    }
    else if (targetChannel == m_assetReloadCategory)
    {
        if (!m_cache.find(pResponse->GetAssetPath(), spData))
        {
            // The asset may have been removed from the cache. Just exit.
            return;
        }
    }
    else
    {
        // We hit this assert if a message comes to this handler on a category we don't
        // know about. For that to occur, we would have to somehow have been subscribed for
        // a different category.
        EE_FAIL("SceneGraphCache::HandleAssetLoadResponse: Load response on an unknown category");
        return;
    }

    // Now we should have valid data at this point
    EE_ASSERT(spData);
    EE_ASSERT(spData->m_filePath == pResponse->GetAssetPath());

    AssetLoadResponse::AssetLoadResult loadResult = pResponse->GetResult();
    AssetCacheResponse::AssetCacheResult cacheResult = AssetCacheResponse::ACR_Success;
    if (loadResult != AssetLoadResponse::ALR_Success &&
        loadResult != AssetLoadResponse::ALR_PartialSuccess)
    {
        // Some kind of error that prevents us from caching the asset. An error was logged by the
        // NIF Factory, so we just state what we will do about it.
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR2,
            ("Failed to load NIF asset %s. Cached copy will have no objects.",
            spData->m_filePath.c_str()));
        cacheResult = loadResult == AssetLoadResponse::ALR_FileAccessDenied ?
            AssetCacheResponse::ACR_FileAccessDenied :
            AssetCacheResponse::ACR_ParseError;
    }

    if (targetChannel == m_assetLoadCategory)
    {
        EE_ASSERT(spData->m_objects.size() == 0);
        UInt32 uiCount = pResponse->GetObjectCount();
        for (UInt32 ui = 0; ui < uiCount; ui++)
        {
            if (pResponse->GetObjectAt(ui) != 0)
                spData->m_objects.push_back(pResponse->GetObjectAt(ui));
        }
        m_cache[spData->m_filePath] = spData;
        m_pendingFiles.erase(spData->m_filePath);
        spData->m_status = AS_Ready;

        // Add the asset to the identifier cache if it has an llid
        if (!spData->m_llid.empty())
        {
            FilePathVector filePaths;
            filePaths.push_back(spData->m_filePath);
            AddFilePaths(spData->m_llid, filePaths);
        }

        // Go through all the requests and increment the number of cached files
        for (efd::list<PendingRequestDataPtr>::iterator requestIter =
            spData->m_requestList.begin();
            requestIter != spData->m_requestList.end();
            requestIter++)
        {
            PendingRequestDataPtr spPendingData = *requestIter;
            EE_ASSERT(spPendingData);
            ++(spPendingData->m_cachedCount);
            CheckCompleteRequest(spPendingData);
            *requestIter = 0;
        }
        spData->m_requestList.clear();
    }
    else if (targetChannel == m_assetReloadCategory)
    {
        for (UInt32 ui = 0; ui < spData->m_objects.size(); ++ui)
            spData->m_objects[ui] = 0;
        spData->m_objects.clear();

        UInt32 uiCount = pResponse->GetObjectCount();
        for (UInt32 ui = 0; ui < uiCount; ui++)
        {
            if (pResponse->GetObjectAt(ui) != 0)
                spData->m_objects.push_back(pResponse->GetObjectAt(ui));
        }
        spData->m_status = AS_Ready;

        // Send all the responses
        for (efd::list<PendingRequestDataPtr>::iterator pendingIter =
            spData->m_requestList.begin();
            pendingIter != spData->m_requestList.end();
            pendingIter++)
        {
            PendingRequestDataPtr spPendingData = *pendingIter;
            EE_ASSERT(spPendingData);

            for (efd::list<RequestDataPtr>::iterator requestIter =
                spPendingData->m_requestList.begin();
                requestIter != spPendingData->m_requestList.end();
                ++requestIter)
            {
               RequestData* pData = *requestIter;

               SceneGraphCacheResponse* pCacheResp = EE_NEW SceneGraphCacheResponse(
                    pData->m_identifier,
                    pData->m_responseCategory,
                    pData->m_pResponseData,
                    cacheResult,
                    AssetCacheResponse::ACT_CacheReload);
                pCacheResp->AddHandle(spData);

                m_pMessageService->SendLocal(pCacheResp, pData->m_responseCategory);
            }
        }
        spData->m_requestList.clear();
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::Subscribe()
{
    // You hit this assert if you did not call Init.
    EE_ASSERT(m_pMessageService);

    m_pMessageService->Subscribe(this, m_assetLoadCategory);
    m_pMessageService->Subscribe(this, m_assetReloadCategory);
    m_pMessageService->Subscribe(this, m_assetLocateCategory);
    m_pMessageService->Subscribe(this, m_assetRefreshCategory);

    m_subscribed = true;
}

//------------------------------------------------------------------------------------------------
SceneGraphCache::FilePathVector* SceneGraphCache::FindFilePaths(const efd::utf8string& identifier)
{
    map<utf8string, FilePathVector*>::iterator iter = m_identifierCache.find(identifier);
    if (iter == m_identifierCache.end())
        return 0;

    return iter->second;
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::AddFilePaths(const efd::utf8string& identifier, const FilePathVector& paths)
{
    map<utf8string, FilePathVector*>::iterator iter = m_identifierCache.find(identifier);
    if (iter != m_identifierCache.end())
    {
        if (paths == *iter->second)
        {
            // Existing entry matches new entry, avoid allocations by keeping existing entry.
            return;
        }
        // Existing entry is different, update by copying new vector into old vector. This should
        // minimize the allocations required to perform the update.
        *iter->second = paths;
    }
    else
    {
        // New entry, allocate a new FilePathVector by copying the passed in vector.
        FilePathVector* pPaths = EE_EXTERNAL_NEW FilePathVector(paths);
        m_identifierCache[identifier] = pPaths;
    }
}

//------------------------------------------------------------------------------------------------
void SceneGraphCache::RemoveFilePaths(const efd::utf8string& identifier)
{
    map<utf8string, FilePathVector*>::iterator iter = m_identifierCache.find(identifier);
    if (iter != m_identifierCache.end())
    {
        EE_EXTERNAL_DELETE iter->second;
        m_identifierCache.erase(iter);
    }
}

//------------------------------------------------------------------------------------------------
bool SceneGraphCache::CheckCompleteRequest(PendingRequestData* pPendingRequest)
{
    bool isComplete = (pPendingRequest->m_cachedCount == pPendingRequest->m_filePaths.size());

    if (!isComplete)
        return false;

    // Remove all traces from the pending list, we're done
    AddFilePaths(pPendingRequest->m_identifier, pPendingRequest->m_filePaths);

    // Send all the responses
    for (efd::list<RequestDataPtr>::iterator requestIter = pPendingRequest->m_requestList.begin();
        requestIter != pPendingRequest->m_requestList.end();
        ++requestIter)
    {
       RequestData* pData = *requestIter;

       SceneGraphCacheResponse* pCacheResp = EE_NEW SceneGraphCacheResponse(
            pData->m_identifier,
            pData->m_responseCategory,
            pData->m_pResponseData,
            AssetCacheResponse::ACR_Success,
            AssetCacheResponse::ACT_CacheAdd);

        for (UInt32 ui = 0; ui < pPendingRequest->m_filePaths.size(); ++ui)
        {
            // Find the cached file data. It must be present.
            SceneGraphCacheDataPtr spCacheData = 0;
            EE_VERIFY(m_cache.find(pPendingRequest->m_filePaths[ui], spCacheData));

            // Add the file to the list of handles to return to the requestor
            pCacheResp->AddHandle(spCacheData);
        }

        m_pMessageService->SendLocal(pCacheResp, pData->m_responseCategory);
    }

    m_pendingRequests[pPendingRequest->m_identifier] = 0;
    m_pendingRequests.erase(pPendingRequest->m_identifier);

    return true;
}

//------------------------------------------------------------------------------------------------
SceneGraphCache::SceneGraphCacheData::SceneGraphCacheData()
    : m_status(AS_Invalid)
{
}

//------------------------------------------------------------------------------------------------
SceneGraphCache::SceneGraphCacheData::~SceneGraphCacheData()
{
    efd::list<PendingRequestDataPtr>::iterator requestIter = m_requestList.begin();
    while (requestIter != m_requestList.end())
    {
        *requestIter = 0;
        ++requestIter;
    }

    for (UInt32 ui = 0; ui < m_objects.size(); ++ui)
        m_objects[ui] = 0;
    m_objects.clear();
}

//------------------------------------------------------------------------------------------------
SceneGraphCache::PendingRequestData::PendingRequestData()
    : m_status(AS_Invalid)
    , m_cachedCount(0)
{
}

//------------------------------------------------------------------------------------------------
SceneGraphCache::PendingRequestData::~PendingRequestData()
{
    efd::list<RequestDataPtr>::iterator requestIter = m_requestList.begin();
    while (requestIter != m_requestList.end())
    {
        *requestIter = 0;
        ++requestIter;
    }
}

//------------------------------------------------------------------------------------------------
SceneGraphCache::RequestData::RequestData()
    : m_responseCategory(efd::kCAT_INVALID)
    , m_pResponseData(0)
{
}

//------------------------------------------------------------------------------------------------
SceneGraphCache::RequestData::~RequestData()
{
}
