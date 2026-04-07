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
#include <efd/File.h>
#include <egf/EntityFactoryRequest.h>
#include <egf/EntityFactoryResponse.h>
#include <egf/EntityFactory.h>
#include <egf/FlatModelManager.h>
#include <egf/FlatModelFactory.h>
#include <egf/egfLogIDs.h>
#include <efd/ILogger.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntityFactory);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntityFactory::EntityFactoryData);

//------------------------------------------------------------------------------------------------
// Register the AssetFactoryManager as a handler for EntityFactoryResponse messages.
EE_HANDLER_SUBCLASS(
    AssetFactoryManager,
    AssetLoadRequestHandler,
    AssetLoadRequest,
    EntityFactoryRequest);

//------------------------------------------------------------------------------------------------
EntityFactory::EntityFactory(FlatModelFactory* fmLoader)
    : m_spFMLoader(fmLoader)
{
}

//------------------------------------------------------------------------------------------------
EntityFactory::~EntityFactory()
{
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus EntityFactory::LoadAsset(
    AssetFactoryManager* pFactoryManager,
    const AssetLoadRequest* pRequest,
    AssetLoadResponsePtr& ppResponse)
{
    IAssetFactory::LoadStatus result = IAssetFactory::LOAD_BLOCKED;

    const EntityFactoryRequest* pCurrentRequest = EE_DYNAMIC_CAST(EntityFactoryRequest, pRequest);
    EE_ASSERT(pCurrentRequest);
    EE_ASSERT(pCurrentRequest->GetAssetPath().length() > 0);

    EntityFactoryData* pData = 0;
    result = GetResponseDataAs<EntityFactoryData>(pRequest, pData);
    if (result != IAssetFactory::LOAD_RUNNING)
    {
        return result;
    }

    // New request
    if (!pData)
    {
        // load our flat model
        EntityFactoryDataPtr newData = EE_NEW EntityFactoryData();
        newData->m_spFlatModelRequest = EE_NEW FlatModelFactoryRequest(
            pCurrentRequest->GetURN(),
            pCurrentRequest->GetResponseCategory(),
            pCurrentRequest->GetAssetPath(),
            pCurrentRequest->GetIsBackground(),
            pCurrentRequest->GetIsPreemptive(),
            pCurrentRequest->GetIsReload());

        AssetLoadResponsePtr pAssetLoadResponse;
        result = m_spFMLoader->LoadAsset(
            pFactoryManager,
            newData->m_spFlatModelRequest,
            pAssetLoadResponse);
        FlatModelFactoryResponsePtr spFlatModelFactoryResponse(
            EE_DYNAMIC_CAST(FlatModelFactoryResponse, pAssetLoadResponse));

        m_pendingRequestMap[pCurrentRequest] = newData;

        // The result is available immediately.
        if (result == IAssetFactory::LOAD_COMPLETE)
        {
            result = LoadEntity(
                pCurrentRequest,
                spFlatModelFactoryResponse,
                ppResponse);
        }
    }
    // We already know about this request.
    else
    {
        // check the load status
        AssetLoadResponsePtr spAssetLoadResponse;
        result = m_spFMLoader->LoadAsset(
            pFactoryManager,
            pData->m_spFlatModelRequest,
            spAssetLoadResponse);

        FlatModelFactoryResponsePtr spFlatModelFactoryResponse(
            EE_DYNAMIC_CAST(FlatModelFactoryResponse, spAssetLoadResponse));

        // The result is available immediately.
        if (result == IAssetFactory::LOAD_COMPLETE)
        {
            result = LoadEntity(pCurrentRequest, spFlatModelFactoryResponse, ppResponse);
        }
    }

    return result;
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus EntityFactory::LoadEntity(
    const EntityFactoryRequest* pCurrentRequest,
    FlatModelFactoryResponse* pFlatModelFactoryResponse,
    AssetLoadResponsePtr& ppResponse)
{
    IAssetFactory::LoadStatus result = IAssetFactory::LOAD_COMPLETE;

    // If the FlatModel load failed, we can't create the entity
    if (pFlatModelFactoryResponse->GetResult() != AssetLoadResponse::ALR_Success &&
        pFlatModelFactoryResponse->GetResult() != AssetLoadResponse::ALR_PartialSuccess)
    {
        EE_LOG(kEntityFactory, ILogger::kERR1,
           ("EntityFactory::LoadEntity failed. A call to the FlatModelFactory reported an error."));

        ppResponse = EE_NEW EntityFactoryResponse(
            pCurrentRequest->GetURN(),
            pCurrentRequest->GetResponseCategory(),
            pFlatModelFactoryResponse->GetResult(),
            pCurrentRequest->GetEntityID(),
            pCurrentRequest->GetAssetPath(),
            pCurrentRequest->GetIsReload());
    }
    // Otherwise attempt to factory a new entity instance.
    else
    {
        // create the new entity using the FlatModelManager
        EntityPtr spEntity = m_spFMLoader->m_spFMM->FactoryEntity(
            pFlatModelFactoryResponse->GetFlatModel(),
            pCurrentRequest->GetEntityID(),
            pCurrentRequest->IsMasterEntity());

        if (spEntity)
        {
            // Success.
            ppResponse = EE_NEW EntityFactoryResponse(
                pCurrentRequest->GetURN(),
                pCurrentRequest->GetResponseCategory(),
                AssetLoadResponse::ALR_Success,
                pCurrentRequest->GetEntityID(),
                pCurrentRequest->GetAssetPath(),
                pCurrentRequest->GetIsReload(),
                spEntity,
                pFlatModelFactoryResponse->GetModelName(),
                pFlatModelFactoryResponse->GetFlatModelMap(),
                pFlatModelFactoryResponse->GetDependentScripts());
        }
        else
        {
            // The FlatModelManager doesn't provide error information; mark this failure
            // as an ALR_ParseError for now.
            EE_LOG(kEntityFactory, ILogger::kERR1,
                ("EntityFactory::LoadEntity failed. "
                 "The FlatModelManager failed to factory the entity."));

            ppResponse = EE_NEW EntityFactoryResponse(
                pCurrentRequest->GetURN(),
                pCurrentRequest->GetResponseCategory(),
                AssetLoadResponse::ALR_ParseError,
                pCurrentRequest->GetEntityID(),
                pCurrentRequest->GetAssetPath(),
                pCurrentRequest->GetIsReload());
        }
    }

    RemoveResponseData(pCurrentRequest);

    return result;
}

//------------------------------------------------------------------------------------------------
IAssetFactory::ThreadSafetyDescription EntityFactory::GetThreadSafety() const
{
    return IAssetFactory::LOADER_BG_ONLY;
}
