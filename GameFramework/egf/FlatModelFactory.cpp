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

#include <egf/FlatModelFactoryRequest.h>
#include <egf/FlatModelFactoryResponse.h>
#include <egf/FlatModelFactory.h>
#include <egf/FlatModelManager.h>
#include <egf/egfBaseIDs.h>
#include <egf/egfLogIDs.h>
#include <efd/SmartCriticalSection.h>
#include <egf/egfLogIDs.h>
#include <efd/ILogger.h>
#include <efd/AssetLocatorResponse.h>
#include <efd/AssetFactoryManager.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
// Register the AssetFactoryManager as the handler for FlatModelLoadRequest messages.
EE_HANDLER_SUBCLASS(
    AssetFactoryManager,
    AssetLoadRequestHandler,
    AssetLoadRequest,
    FlatModelFactoryRequest);

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(FlatModelFactory);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(FlatModelFactory::ResponseData);

//------------------------------------------------------------------------------------------------
FlatModelFactory::FlatModelFactory(
    AssetFactoryManager* pFactoryManager,
    FlatModelManager* pFmm,
    IScriptFactory* pScriptFactory)
    : m_spFMM(pFmm)
    , m_spScriptFactory(pScriptFactory)
    , m_privateCategory(Category(efd::UniversalID::ECU_PrivateChannel,
                                 kNetID_Any,
                                 efd::kBASEID_FlatModelFactoryCategory))
{
    EE_ASSERT(m_spFMM);

    pFactoryManager->RegisterAssetLocateHandler(m_privateCategory, this);
}

//------------------------------------------------------------------------------------------------
FlatModelFactory::~FlatModelFactory()
{
}

//------------------------------------------------------------------------------------------------
void FlatModelFactory::Shutdown()
{
    m_spFMM = 0;
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus FlatModelFactory::LoadAsset(
    AssetFactoryManager* pFactoryManager,
    const AssetLoadRequest* pRequest,
    AssetLoadResponsePtr& ppResponse)
{
    ppResponse = 0;
    IAssetFactory::LoadStatus result = IAssetFactory::LOAD_RUNNING;
    ResponseData* pResponseData = 0;

    result = GetResponseDataAs<ResponseData>(pRequest, pResponseData);
    if (result != IAssetFactory::LOAD_RUNNING)
    {
        return result;
    }

    // We have a new pRequest
    if (!pResponseData)
    {
        result = ProcessNewRequest(pFactoryManager, pRequest);

        // The result is available immediately.
        if (result == IAssetFactory::LOAD_COMPLETE)
        {
            GetResponseDataAs<ResponseData>(pRequest, pResponseData);
            EE_ASSERT(pResponseData);

            ppResponse = pResponseData->m_spResponse;
            EE_ASSERT(ppResponse);

            RemoveResponseData(pRequest);
        }
    }

    // We already know about this request.
    else
    {
        // If we have pending locates / loads indicate the load is still waiting for responses.
        UInt32 pendingCount = pResponseData->m_pendingLocateRequests.size();
        UInt32 pendingScriptCount = pResponseData->m_pendingScriptLoads.size();

        if (pendingCount || pendingScriptCount)
        {
            result = IAssetFactory::LOAD_RUNNING;
            CheckScriptLoads(pResponseData);
        }
        // If we don't have any remaining pending locates or script loads, we're done.
        else
        {
            ppResponse = pResponseData->m_spResponse;
            RemoveResponseData(pRequest);
            result = IAssetFactory::LOAD_COMPLETE;
        }
    }

    return result;
}

//------------------------------------------------------------------------------------------------
IAssetFactory::LoadStatus FlatModelFactory::ProcessNewRequest(
    AssetFactoryManager* pFactoryManager,
    const AssetLoadRequest* request)
{
    ResponseDataPtr spData = EE_NEW ResponseData();
    spData->m_pAFM = pFactoryManager;
    spData->m_isForeground = !request->GetIsBackground();

    m_pendingRequestMap[request] = spData;

    spData->m_spResponse = EE_NEW FlatModelFactoryResponse(
        request->GetURN(),
        request->GetResponseCategory(),
        AssetLoadResponse::ALR_Success,
        request->GetAssetPath(),
        request->GetIsReload());

    FlatModelPtr flatModel;
    FlatModelManager::LoadResult loadResult = m_spFMM->ParseModelFile(
        request->GetAssetPath(),
        flatModel);

    if (loadResult != FlatModelManager::OK)
    {
        EE_LOG(efd::kFlatModelFactory, efd::ILogger::kERR0,
            ("Failed to load a flat model for URN: %s, Path: %s",
             request->GetURN().c_str(),
             request->GetAssetPath().c_str()));

        switch (loadResult)
        {
        case FlatModelManager::ModelFileNotFound:
            EE_LOG(efd::kFlatModelFactory, ILogger::kERR0, ("File not found"));
            spData->m_spResponse->SetResult(AssetLoadResponse::ALR_FileNotFound);
            break;
        case FlatModelManager::ParseError:
            EE_LOG(efd::kFlatModelFactory, ILogger::kERR0, ("Parse Error"));
            spData->m_spResponse->SetResult(AssetLoadResponse::ALR_ParseError);
            break;
        default:
            EE_LOG(efd::kFlatModelFactory, ILogger::kERR0, ("Unknown Error"));
            spData->m_spResponse->SetResult(AssetLoadResponse::ALR_UnknownError);
            break;
        }

        return IAssetFactory::LOAD_COMPLETE;
    }
    else
    {
        FlatModelFactoryResponse* pTmp = spData->GetResponseAs<FlatModelFactoryResponse>();
        pTmp->m_modelName = flatModel->GetName();
        pTmp->m_flatModelMap[flatModel->GetName()] = flatModel;
    }

    FindBehaviors(spData, flatModel, true);

    // nothing else to lookup -- the load has completed.
    UInt32 pendingCount = spData->m_pendingLocateRequests.size();
    UInt32 pendingScriptLoads = spData->m_pendingScriptLoads.size();
    if (pendingCount == 0 && pendingScriptLoads == 0)
    {
        return IAssetFactory::LOAD_COMPLETE;
    }
    else
    {
        return IAssetFactory::LOAD_RUNNING;
    }
}

//------------------------------------------------------------------------------------------------
void FlatModelFactory::HandleAssetLocate(
    const efd::AssetLocatorResponse* pResponse,
    efd::Category targetChannel)
{
    // find the pRequest waiting for this asset.
    const utf8string& urn = pResponse->GetResponseURI();

    for (PendingRequestMap::iterator pendingIt = m_pendingRequestMap.begin();
        pendingIt != m_pendingRequestMap.end();
        ++pendingIt)
    {
        // If the pRequest doesn't have any ResponseData, it's not ready for processing yet.
        if (!pendingIt->second)
        {
            continue;
        }

        ResponseData* data = EE_DYNAMIC_CAST(ResponseData, pendingIt->second);

        EE_ASSERT(data);

        // See if we have a matching pending pRequest
        set<utf8string>::iterator nameIt = data->m_pendingLocateRequests.find(urn);

        // found a matching pRequest.
        if (nameIt != data->m_pendingLocateRequests.end())
        {
            if (pResponse->GetResponseURI().find("urn:emergent-flat-model") != utf8string::npos)
            {
                HandleFlatModelLocate(pResponse, data);
            }
            else if (pResponse->GetResponseURI().find("behavior:") != utf8string::npos)
            {
                HandleScriptLocate(pendingIt->first, pResponse, data);
            }

            // remove the pending locate pRequest from our set.
            data->m_pendingLocateRequests.erase(nameIt);
        }
    }
}

//------------------------------------------------------------------------------------------------
IAssetFactory::ThreadSafetyDescription FlatModelFactory::GetThreadSafety() const
{
    return IAssetFactory::LOADER_BG_ONLY;
}

//------------------------------------------------------------------------------------------------
void FlatModelFactory::HandleFlatModelLocate(
    const efd::AssetLocatorResponse* pResponse,
    ResponseDataPtr pendingResponse)
{
    const efd::AssetLocatorResponse::AssetURLMap& locations = pResponse->GetAssetURLMap();

    // iterate over all the location responses and look for matching model names
    efd::AssetLocatorResponse::AssetURLMap::const_iterator locIt = locations.begin();

    if (locIt == locations.end())
    {
        // Was not able to locate the asset.
        EE_LOG(efd::kFlatModelFactory, efd::ILogger::kERR1,
            ("FlatModelFactory: Failed to locate mix-in flat model, URN: %s",
            pResponse->GetResponseURI().c_str()));

        pendingResponse->m_spResponse->SetResult(AssetLoadResponse::ALR_PartialSuccess);
    }

    for (; locIt != locations.end(); ++locIt)
    {
        FlatModelPtr newFlatModel;
        FlatModelManager::LoadResult result = m_spFMM->ParseModelFile(
            locIt->second.url,
            newFlatModel);

        if (result != FlatModelManager::OK)
        {
            EE_LOG(efd::kFlatModelFactory, efd::ILogger::kERR1,
                ("Failed to load a flat model for mix-in Name: %s, Path: %s",
                locIt->second.name.c_str(),
                locIt->second.url.c_str()));

            // This can fail for a number of reasons:
            // - File not found.
            // - Sharing Violation; the file is already open.
            // - Access denied errors.
            // - Errors parsing the file.
            // Unfortunately, the FMM doesn't tell us what failed.
            // For now indicate a parse error.
            pendingResponse->m_spResponse->SetResult(AssetLoadResponse::ALR_ParseError);
        }
        else
        {
            FlatModelFactoryResponse* tmp =
                pendingResponse->GetResponseAs<FlatModelFactoryResponse>();
            EE_ASSERT(tmp);

            tmp->m_flatModelMap[newFlatModel->GetName()] = newFlatModel;

            FindBehaviors(pendingResponse, newFlatModel, false);
        }
    }
}

//------------------------------------------------------------------------------------------------
void FlatModelFactory::HandleScriptLocate(
    const efd::AssetLoadRequest* pRequest,
    const efd::AssetLocatorResponse* pResponse,
    ResponseDataPtr pendingResponse)
{
    const efd::AssetLocatorResponse::AssetURLMap& locations = pResponse->GetAssetURLMap();

    // iterate over all the location responses and look for matching model names
    efd::AssetLocatorResponse::AssetURLMap::const_iterator locIt = locations.begin();
    for (; locIt != locations.end(); ++locIt)
    {
        utf8string urn(pResponse->GetResponseURI());

        ScriptFactoryRequestPtr spScriptLoadRequest = EE_NEW ScriptFactoryRequest(
            urn,
            locIt->second.name,
            kCAT_INVALID,
            locIt->second.url,
            pRequest->GetIsBackground(),
            pRequest->GetIsPreemptive(),
            pRequest->GetIsReload());

        AssetLoadResponsePtr spLoadResponse;
        IAssetFactory::LoadStatus status = m_spScriptFactory->LoadAsset(
            pendingResponse->m_pAFM,
            spScriptLoadRequest,
            spLoadResponse);

        if (status == IAssetFactory::LOAD_COMPLETE)
        {
            ScriptFactoryResponse* r = EE_DYNAMIC_CAST(ScriptFactoryResponse, spLoadResponse);
            EE_ASSERT(r);
            EE_ASSERT(pendingResponse->m_spResponse);

            pendingResponse->GetResponseAs<FlatModelFactoryResponse>()->AddDependentScript(r);
        }
        else
        {
            pendingResponse->m_pendingScriptLoads[urn] = spScriptLoadRequest;
        }
    }
}

//------------------------------------------------------------------------------------------------
void FlatModelFactory::CheckScriptLoads(ResponseData* pResponseData)
{
    if (pResponseData->m_pendingScriptLoads.size() == 0)
    {
        return;
    }

    efd::map<efd::utf8string, egf::ScriptFactoryRequestPtr>::iterator it =
        pResponseData->m_pendingScriptLoads.begin();

    while (it != pResponseData->m_pendingScriptLoads.end())
    {
        AssetLoadResponsePtr spLoadResponse;
        IAssetFactory::LoadStatus status = m_spScriptFactory->LoadAsset(
            pResponseData->m_pAFM,
            it->second,
            spLoadResponse);

        if (status == IAssetFactory::LOAD_COMPLETE)
        {
            ScriptFactoryResponse* r = EE_DYNAMIC_CAST(ScriptFactoryResponse, spLoadResponse);
            EE_ASSERT(r);
            EE_ASSERT(pResponseData->m_spResponse);

            FlatModelFactoryResponse *tmp =
                pResponseData->GetResponseAs<FlatModelFactoryResponse>();
            EE_ASSERT(tmp);

            tmp->AddDependentScript(r);
            pResponseData->m_pendingScriptLoads.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

//------------------------------------------------------------------------------------------------
void FlatModelFactory::FindBehaviors(
    ResponseDataPtr pendingResponseData,
    const FlatModel* pFlatModel,
    bool isTopLevelRequest)
{
    // If we have a script factory, locate and load dependent scripts
    if (!m_spScriptFactory)
    {
        return;
    }

    utf8string scriptType("");
    if (m_spScriptFactory->GetScriptType() == BehaviorType_Lua)
    {
        scriptType = "lua-behavior";
    }
    else
    {
        EE_FAIL("No current support for this behavior type...");
        return;
    }

    // Name of the flat models needed to invoke behaviors at runtime.
    efd::set<efd::utf8string> behaviorModelNames;

    // Get the list of behavior names for this flat model.
    efd::list<efd::utf8string> behaviorNames;
    pFlatModel->GetBehaviorNames(behaviorNames);

    for (efd::list<efd::utf8string>::const_iterator it = behaviorNames.begin();
        it != behaviorNames.end();
        ++it)
    {
        utf8string modelName;
        utf8string behaviorName;
        FlatModel::SplitModelAndBehavior(*it, modelName, behaviorName);

        // Needed for virtual behaviors.
        if (modelName.size() == 0)
        {
            modelName = pFlatModel->GetName();
        }

        const BehaviorDescriptor* desc = pFlatModel->GetBehaviorDescriptor(behaviorName);
        if (desc)
        {
            const BehaviorDescriptor::InvocationOrderedModelNamesList& invokeOrder =
                desc->GetInvocationOrderedModelNames();

            if (invokeOrder.size())
            {
                behaviorModelNames.insert(invokeOrder.begin(), invokeOrder.end());
            }

            if (desc->GetType() == BehaviorType_Lua)
            {
                // Need the flat model if its different from the current flat model.
                if (modelName != pFlatModel->GetName())
                {
                    behaviorModelNames.insert(modelName);
                }
                else
                {
                    // We need the script.
                    SmartCriticalSection cs(m_pendingRequestLock);
                    cs.Lock();

                    utf8string urn = utf8string(
                        Formatted,
                        "urn:%s:Ese%s",
                        scriptType.c_str(),
                        pFlatModel->GetName().c_str());

                    if (pendingResponseData->m_pendingLocateRequests.find(urn) ==
                        pendingResponseData->m_pendingLocateRequests.end())
                    {
                        pendingResponseData->m_pendingLocateRequests.insert(urn);
                        pendingResponseData->m_pAFM->AssetLocate(urn, m_privateCategory);
                    }
                }
            }
        }
    }

    // Iterator over the model name list needed for behaviors and locate flat models needed
    // to invoke behaviors.
    if (isTopLevelRequest)
    {
        for (efd::set<efd::utf8string>::const_iterator it = behaviorModelNames.begin();
            it != behaviorModelNames.end();
            ++it)
        {
            m_pendingRequestLock.Lock();

            // We need to load the flat model for this model so we have a behavior descriptor
            // at runtime. Without this the invocation of the behavior will fail if a behavior
            // has the 'extends' or 'reverse extends' trait but we didn't load one or more
            // models in the invocation order list.
            if (*it != pFlatModel->GetName())
            {
                utf8string urn(Formatted, "urn:emergent-flat-model:%s", it->c_str());
                pendingResponseData->m_pendingLocateRequests.insert(urn);
                pendingResponseData->m_pAFM->AssetLocate(urn, m_privateCategory);
            }

            m_pendingRequestLock.Unlock();
        }
    }
}
