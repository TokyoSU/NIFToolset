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
#include <egf/WorldFactory.h>
#include <egf/SAXBlockParser.h>
#include <efd/MessageService.h>
#include <egf/egfBaseIDs.h>
#include <egf/WorldFactoryRequest.h>
#include <efd/File.h>
#include <egf/FlatModelFactoryResponse.h>
#include <egf/FlatModelFactory.h>
#include <egf/egfLogIDs.h>
#include <efd/ILogger.h>
#include <efd/AssetFactoryManager.h>
#include <efd/AssetLocatorResponse.h>
#include <efd/SmartCriticalSection.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(WorldFactory);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(WorldFactory::WorldFactoryData);

//------------------------------------------------------------------------------------------------
// Register the AssetFactoryManager as a handler for WorldFactoryResponse messages.
EE_HANDLER_SUBCLASS(
    AssetFactoryManager,
    AssetLoadRequestHandler,
    AssetLoadRequest,
    WorldFactoryRequest);

//------------------------------------------------------------------------------------------------
WorldFactory::WorldFactory(AssetFactoryManager* pFactoryManager, FlatModelFactory* pFmLoader)
    : m_spFMLoader(pFmLoader)
    , m_privateCategory(Category(
        efd::UniversalID::ECU_PrivateChannel,
        kNetID_Any,
        efd::kBASEID_WorldFactoryCategory))
{
    pFactoryManager->RegisterAssetLocateHandler(m_privateCategory, this);
}

//------------------------------------------------------------------------------------------------
WorldFactory::~WorldFactory()
{
}

//------------------------------------------------------------------------------------------------
void WorldFactory::Shutdown()
{
    m_spFMLoader = 0;
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus WorldFactory::LoadAsset(
    AssetFactoryManager* pFactoryManager,
    const AssetLoadRequest* pRequest,
    AssetLoadResponsePtr& ppResponse)
{
    IAssetFactory::LoadStatus result = IAssetFactory::LOAD_RUNNING;

    WorldFactoryData* pResponseData = 0;
    result = GetResponseDataAs<WorldFactoryData>(pRequest, pResponseData);
    if (result != IAssetFactory::LOAD_RUNNING)
    {
        return result;
    }

    // This is a new request.
    if (!pResponseData)
    {
        const WorldFactoryRequest* pCurrentRequest = EE_DYNAMIC_CAST(WorldFactoryRequest, pRequest);
        result = LoadWorld(pFactoryManager, pCurrentRequest, ppResponse);
        if (result == IAssetFactory::LOAD_COMPLETE)
        {
            RemoveResponseData(pCurrentRequest);
        }
    }

    // A previous request that is still processing.
    else
    {
        switch (pResponseData->m_state)
        {
        case WorldFactoryData::LOAD_FLAT_MODELS:
            {
                LoadFlatModels(
                    pFactoryManager,
                    pResponseData,
                    pRequest->GetIsBackground(),
                    pRequest->GetIsPreemptive(),
                    pRequest->GetIsReload());
            }
            break;

        case WorldFactoryData::WAIT_FOR_FLAT_MODEL_LOADS:
            {
                CheckFlatModelLoads(pFactoryManager, pResponseData);
            }
            break;

        case WorldFactoryData::COMPLETE:
            {
                ppResponse = pResponseData->m_spResponse;
                RemoveResponseData(pRequest);
                result = IAssetFactory::LOAD_COMPLETE;
            }
            break;

        case WorldFactoryData::LOAD_WORLD_FILE:
        case WorldFactoryData::LOCATE_FLAT_MODELS:
            // do nothing
            break;
        }
    }

    return result;
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus WorldFactory::LoadWorld(
    AssetFactoryManager* pFactoryManager,
    const WorldFactoryRequest* pCurrentRequest,
    efd::AssetLoadResponsePtr& o_spResponse)
{
    EE_ASSERT(pCurrentRequest);
    EE_ASSERT(pCurrentRequest->GetAssetPath().length());

    // Load the world pFile into memory.
    File* pFile = File::GetFile(pCurrentRequest->GetAssetPath().c_str(), File::READ_ONLY);
    if (!pFile)
    {
        EE_LOG(efd::kWorldFactory, ILogger::kERR0,
            ("Could not open file \'%s\' for reading.", pCurrentRequest->GetAssetPath().c_str()));

        // couldn't open the world pFile.
        o_spResponse = EE_NEW WorldFactoryResponse(
            pCurrentRequest->GetURN(),
            pCurrentRequest->GetResponseCategory(),
            AssetLoadResponse::ALR_FileNotFound,
            pCurrentRequest->GetAssetPath(),
            pCurrentRequest->GetIsReload());

        return IAssetFactory::LOAD_COMPLETE;
    }

    UInt32 fileSize = pFile->GetFileSize();
    EE_ASSERT(fileSize);
    if (!fileSize)
    {
        EE_LOG(efd::kWorldFactory, ILogger::kERR0,
            ("File \'%s\' contains no data.", pCurrentRequest->GetAssetPath().c_str()));

        o_spResponse = EE_NEW WorldFactoryResponse(
            pCurrentRequest->GetURN(),
            pCurrentRequest->GetResponseCategory(),
            AssetLoadResponse::ALR_ParseError,
            pCurrentRequest->GetAssetPath(),
            pCurrentRequest->GetIsReload());

        return IAssetFactory::LOAD_COMPLETE;
    }

    char* buffer = EE_ALLOC(char, fileSize + 1);
    UInt32 readSize = pFile->Read(buffer, fileSize);
    buffer[fileSize] = '\0';

    EE_ASSERT(fileSize == readSize);
    EE_DELETE pFile;

    // parse the world to get our list of flat models.
    efd::set<utf8string> modelNames;

    UInt32 errors = SAXBlockParser::ParseBuffer(
        pCurrentRequest->GetAssetPath().c_str(),
        buffer,
        modelNames);

    if (errors)
    {
        EE_LOG(efd::kWorldFactory, ILogger::kERR0,
            ("Error parsing file \'%s\'.", pCurrentRequest->GetAssetPath().c_str()));

        // couldn't parse the world pFile.
        o_spResponse = EE_NEW WorldFactoryResponse(
            pCurrentRequest->GetURN(),
            pCurrentRequest->GetResponseCategory(),
            AssetLoadResponse::ALR_ParseError,
            pCurrentRequest->GetAssetPath(),
            pCurrentRequest->GetIsReload());

        return IAssetFactory::LOAD_COMPLETE;
    }

    // Prepare our response information.
    WorldFactoryResponseDataPtr spResponseData = EE_NEW WorldFactoryData();
    spResponseData->m_spResponse = EE_NEW WorldFactoryResponse(
        pCurrentRequest->GetURN(),
        pCurrentRequest->GetResponseCategory(),
        AssetLoadResponse::ALR_Success,
        pCurrentRequest->GetAssetPath(),
        pCurrentRequest->GetIsReload(),
        readSize,
        buffer);
    spResponseData->m_state = WorldFactoryData::LOCATE_FLAT_MODELS;

    m_pendingRequestMap[pCurrentRequest] = spResponseData;

    for (efd::set<utf8string>::const_iterator it = modelNames.begin(); it != modelNames.end(); ++it)
    {
        // Load any flat model files we don't know about.
        if (m_spFMLoader->m_spFMM->FindModel(*it) == 0)
        {
            utf8string urn(Formatted, "urn:emergent-flat-model:%s", it->c_str());

            spResponseData->m_pendingLocateRequests[urn] = it->c_str();
            spResponseData->m_locatedFlatModels[*it] = "";
            pFactoryManager->AssetLocate(urn, m_privateCategory);
        }
    }

    // If we don't have any pending locates, we're done.
    if (spResponseData->m_pendingLocateRequests.size() == 0)
    {
        spResponseData->m_state = WorldFactoryData::COMPLETE;
    }

    // We did some work, but are blocked. So return LOAD_RUNNING on this iteration.
    return IAssetFactory::LOAD_RUNNING;
}

//------------------------------------------------------------------------------------------------
void WorldFactory::HandleAssetLocate(
    const efd::AssetLocatorResponse* response,
    efd::Category targetChannel)
{
    const efd::AssetLocatorResponse::AssetURLMap& locations = response->GetAssetURLMap();

    // find all pending locate requests.
    for (PendingRequestMap::iterator pendingIt = m_pendingRequestMap.begin();
        pendingIt != m_pendingRequestMap.end();
        ++pendingIt)
    {
        // If the request doesn't have any ResponseData, it's not ready for processing yet.
        if (!pendingIt->second)
        {
            continue;
        }

        WorldFactoryData* pResponseData = EE_DYNAMIC_CAST(WorldFactoryData, pendingIt->second);
        EE_ASSERT(pResponseData);

        // See if we have a matching pending request
        efd::map<efd::utf8string, efd::utf8string>::iterator nameIt =
            pResponseData->m_pendingLocateRequests.find(response->GetResponseURI());

        // found a matching request.
        if (nameIt != pResponseData->m_pendingLocateRequests.end())
        {
            // iterate over all the location responses and look for matching model names
            efd::AssetLocatorResponse::AssetURLMap::const_iterator locIt = locations.begin();

            if (locIt == locations.end())
            {
                // Unable to find the asset
                EE_LOG(
                    efd::kWorldFactory,
                    efd::ILogger::kERR1,
                    ("Failed to locate flat model:\'%s\' needed by world:\'%s\'",
                    nameIt->second.c_str(),
                    pResponseData->m_spResponse->GetURN().c_str()));

                pResponseData->m_locatedFlatModels.erase(nameIt->second);
                pResponseData->m_spResponse->SetResult(AssetLoadResponse::ALR_PartialSuccess);
            }

            for (; locIt != locations.end(); ++locIt)
            {
                pResponseData->m_locatedFlatModels[locIt->second.name] = locIt->second.url;
            }

            pResponseData->m_pendingLocateRequests.erase(nameIt->first);

            // If we have no more pending locates, move on to loading flat model files.
            if (pResponseData->m_pendingLocateRequests.size() == 0)
            {
                pResponseData->m_state = WorldFactoryData::LOAD_FLAT_MODELS;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus WorldFactory::LoadFlatModels(
    AssetFactoryManager* pFactoryManager,
    WorldFactoryData* pResponseData,
    bool isBackground,
    bool isPreemptive,
    bool isReload)
{
    IAssetFactory::LoadStatus result = IAssetFactory::LOAD_RUNNING;

    LocatedFlatModelMap::iterator it = pResponseData->m_locatedFlatModels.begin();
    while (it != pResponseData->m_locatedFlatModels.end())
    {
        utf8string urn(Formatted, "urn:emergent-flat-model:%s", it->first.c_str());

        FlatModelFactoryRequestPtr flatModelRequest = EE_NEW FlatModelFactoryRequest(
            urn,
            kCAT_INVALID,
            it->second,
            isBackground,
            isPreemptive,
            isReload);

        FlatModelFactoryResponsePtr spFlatModelFactoryResponse;
        IAssetFactory::LoadStatus status = LoadFlatModel(
            pFactoryManager,
            pResponseData,
            flatModelRequest,
            spFlatModelFactoryResponse);

        // Remove the flat model location.
        pResponseData->m_locatedFlatModels.erase(it++);

        if (status == IAssetFactory::LOAD_COMPLETE)
        {
            // If we get COMPLETE here, it indicates there was an error loading the flat
            // model file. The world won't load in this state so we can exit early.
            if (pResponseData->m_state == WorldFactoryData::COMPLETE)
            {
                return status;
            }
        }
        else
        {
            pResponseData->m_pendingFlatModelLoads[urn] = flatModelRequest;
        }
    }

    pResponseData->m_state = WorldFactoryData::WAIT_FOR_FLAT_MODEL_LOADS;

    return result;
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus WorldFactory::LoadFlatModel(
    AssetFactoryManager* pFactoryManager,
    WorldFactoryData* pResponseData,
    FlatModelFactoryRequest* pFlatModelRequest,
    FlatModelFactoryResponsePtr& o_spFlatModelResponse)
{
    AssetLoadResponsePtr spResponse;
    IAssetFactory::LoadStatus result =
        m_spFMLoader->LoadAsset(pFactoryManager, pFlatModelRequest, spResponse);

    o_spFlatModelResponse = EE_DYNAMIC_CAST(FlatModelFactoryResponse, spResponse);

    WorldFactoryResponse* pWorldResponse = pResponseData->GetResponseAs<WorldFactoryResponse>();
    EE_ASSERT(pWorldResponse);

    if (result == IAssetFactory::LOAD_COMPLETE)
    {
        if (o_spFlatModelResponse->GetResult() == AssetLoadResponse::ALR_Success)
        {
            // add the flat model and all the mix in models.
            pWorldResponse->AddFlatModel(o_spFlatModelResponse);
        }
        // Loading the flat model failed.
        else
        {
            pResponseData->m_spResponse->SetResult(o_spFlatModelResponse->GetResult());
            pResponseData->m_state = WorldFactoryData::COMPLETE;
        }
    }

    return result;
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus WorldFactory::CheckFlatModelLoads(
    AssetFactoryManager* pFactoryManager,
    WorldFactoryData* pResponseData)
{
    IAssetFactory::LoadStatus result = IAssetFactory::LOAD_RUNNING;

    if (pResponseData->m_pendingFlatModelLoads.size())
    {
        FlatModelRequestMap& pendingLoads = pResponseData->m_pendingFlatModelLoads;

        for (FlatModelRequestMap::iterator pendingIt = pendingLoads.begin();
            pendingIt != pendingLoads.end();
            /*do nothing*/)
        {
            FlatModelFactoryResponsePtr spFlatModelFactoryResponse;
            IAssetFactory::LoadStatus status = LoadFlatModel(
                pFactoryManager,
                pResponseData,
                pendingIt->second,
                spFlatModelFactoryResponse);

            if (status == IAssetFactory::LOAD_COMPLETE)
            {
                // decrement the pending list.
                pResponseData->m_pendingFlatModelLoads.erase(pendingIt++);

                // Nothing left to do.
                if (pResponseData->m_state == WorldFactoryData::COMPLETE)
                {
                    return status;
                }
            }
            else
            {
                ++pendingIt;
            }
        }
    }
    else
    {
        pResponseData->m_state = WorldFactoryData::COMPLETE;
    }

    return result;
}

//------------------------------------------------------------------------------------------------
IAssetFactory::ThreadSafetyDescription WorldFactory::GetThreadSafety() const
{
    return IAssetFactory::LOADER_BG_ONLY;
}

//------------------------------------------------------------------------------------------------
WorldFactory::WorldFactoryData::WorldFactoryData()
    : m_state(WorldFactoryData::LOAD_WORLD_FILE)
    , m_pendingLocateRequests()
    , m_locatedFlatModels()
    , m_pendingFlatModelLoads()
{}
