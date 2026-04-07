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

#include "egfPCH.h"

#include <egf/EntityLoaderService.h>
#include <egf/SAXEntityParser.h>
#include <efd/AssetLocatorService.h>

#include <efd/ServiceManager.h>
#include <egf/Scheduler.h>
#include <efd/IConfigManager.h>
#include <egf/Entity.h>
#include <egf/EntityChangeMessage.h>
#include <egf/egfLogIDs.h>
#include <egf/WorldFactory.h>


//------------------------------------------------------------------------------------------------
using namespace efd;
using namespace egf;


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntityLoaderService);

// sent on the private channel
EE_HANDLER(EntityLoaderService, OnLoadRequest, EntityLoadRequest);

// sent on the private channel
EE_HANDLER_WRAP(
    EntityLoaderService,
    HandleAssetLocatorResponse,
    AssetLocatorResponse,
    kMSGID_AssetLocatorResponse);

// sent on the local public channel
EE_HANDLER_WRAP(
    EntityLoaderService,
    HandleSchedulerCleared,
    IMessage,
    kMSGID_SchedulerCleared);

// For entity change tracking to monitor block loading completion
EE_HANDLER_WRAP(
    EntityLoaderService,
    OnEntityAdded,
    EntityChangeMessage,
    kMSGID_OwnedEntityAdded);

// For entity change tracking to monitor block unloading completion
EE_HANDLER_WRAP(
    EntityLoaderService,
    OnEntityRemoved,
    EntityChangeMessage,
    kMSGID_OwnedEntityRemoved);

// For entity change tracking to monitor block loading completion
EE_HANDLER_WRAP(
    EntityLoaderService,
    OnEntityEnterWorld,
    EntityChangeMessage,
    kMSGID_OwnedEntityEnterWorld);

// Sent on a private local channel.
EE_HANDLER(
    EntityLoaderService,
    HandleResetUnloadComplete,
    EntityLoadResult);

//------------------------------------------------------------------------------------------------
EntityLoaderService::EntityLoaderService()
: efd::IMessageHelperSystemService(efd::kCLASSID_EntityLoaderService)
, m_maxEntityLoadThrottle(40)
, m_maxEntityUnloadThrottle(10)
, m_myPrivateChannel()
, m_pMessageService(NULL)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 2200;
}

//------------------------------------------------------------------------------------------------
EntityLoaderService::~EntityLoaderService()
{
}

//------------------------------------------------------------------------------------------------
const char* egf::EntityLoaderService::GetDisplayName() const
{
    return "EntityLoader";
}

//------------------------------------------------------------------------------------------------
SyncResult EntityLoaderService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    // Make sure that the EntityManager is running before we attempt to load any files. If it is
    // not running then we cannot create entity instances so our service would be useless.
    pDependencyRegistrar->AddDependency<EntityManager>();
    // Because we register an IAssetFactory we must depend on the AssetFactoryManager class.
    pDependencyRegistrar->AddDependency<AssetFactoryManager>();
    // And because our IAssetFactory depends on FlatModelManager's IAssetFactory we depend on that
    // service too to ensure we shut down before that factory is invalidated.
    pDependencyRegistrar->AddDependency<FlatModelManager>();

    return efd::IMessageHelperSystemService::OnPreInit(pDependencyRegistrar);
}

//------------------------------------------------------------------------------------------------
AsyncResult EntityLoaderService::OnInit()
{
    EE_ASSERT(m_pServiceManager);

    m_pMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    if (!m_pMessageService)
    {
        return AsyncResult_Failure;
    }

    AssetFactoryManager* pAFM = m_pServiceManager->GetSystemServiceAs<AssetFactoryManager>();
    EE_ASSERT(pAFM); // AddDependency call in OnPreInit above ensures this

    FlatModelManager* pFMM = m_pServiceManager->GetSystemServiceAs<FlatModelManager>();
    EE_ASSERT(pFMM); // AddDependency call in OnPreInit above ensures this

    m_pFlatModelFactory = pFMM->GetFlatModelFactory();

    // We set everything but the net id in our constructor.
    m_myPrivateChannel = efd::Category(
        efd::UniversalID::ECU_PrivateChannel,
        m_pMessageService->GetNetID(),
        EntityLoaderService::CLASS_ID);

    // Set up a WorldFactory for use by EntitySetLoadState.
    WorldFactory* pWorldFactory = EE_NEW WorldFactory(pAFM, m_pFlatModelFactory);
    pAFM->RegisterAssetFactory(EntitySetLoadState::GetCategory(), pWorldFactory);

    m_pMessageService->Subscribe(this, GetPrivateChannel());
    m_pMessageService->Subscribe(this, kCAT_LocalMessage);

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
AsyncResult EntityLoaderService::OnTick()
{
    m_entitiesLoadedThisTick = 0;
    m_entitiesUnloadedThisTick = 0;

    ProcessUnloadRequests();
    ProcessLoadRequests();

    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 EntityLoaderService::ProcessUnloadRequests()
{
    for (LoadRequestMap::iterator it = m_unloadRequests.begin();
        it != m_unloadRequests.end();
        /*do nothing*/)
    {
        LoadRequestMap::NodeType* pNode = *it;
        AsyncResult result = ProcessUnloadRequest(pNode->first, pNode->second);
        if (result != AsyncResult_Pending)
        {
            // On failure or completion we remove this entry from the unloading queue:
            it = m_unloadRequests.erase(it);
        }
        else
        {
            ++it;
        }

        if (m_entitiesUnloadedThisTick >= m_maxEntityUnloadThrottle)
        {
            // we reached our limit, so stop processing unloads for this tick
            break;
        }
    }
    return m_unloadRequests.size();
}

//------------------------------------------------------------------------------------------------
efd::UInt32 EntityLoaderService::ProcessLoadRequests()
{
    for (LoadRequestMap::iterator it = m_loadRequests.begin();
        it != m_loadRequests.end();
        /*do nothing*/)
    {
        LoadRequestMap::NodeType* pNode = *it;
        AsyncResult result = ProcessLoadRequest(pNode->first, pNode->second);
        if (result != AsyncResult_Pending)
        {
            // On failure or completion we remove this entry from the unloading queue:
            it = m_loadRequests.erase(it);
        }
        else
        {
            ++it;
        }

        if (m_entitiesLoadedThisTick >= m_maxEntityLoadThrottle)
        {
            // we reached our limit, so stop processing unloads for this tick
            break;
        }
    }
    return m_loadRequests.size();
}

//------------------------------------------------------------------------------------------------
AsyncResult EntityLoaderService::OnShutdown()
{
    m_entitiesUnloadedThisTick = 0;

    // Cancel any pending load requests:
    for (LoadRequestMap::iterator it = m_loadRequests.begin();
        it != m_loadRequests.end();
        it = m_loadRequests.begin())
    {
        LoadRequestMap::NodeType* pNode = *it;
        CancelLoadRequest(pNode->first);
    }

    // For any blocks that are loaded, request that they be unloaded
    CleanupAllLoadedWorlds();

    // Unload all entity sets.
    if (0 != ProcessUnloadRequests())
    {
        return AsyncResult_Pending;
    }

    if (m_pMessageService)
    {
        m_pMessageService->Unsubscribe(this, GetPrivateChannel());
        m_pMessageService->Unsubscribe(this, kCAT_LocalMessage);
        m_pMessageService = NULL;
    }

    // Unregister the WorldFactory
    AssetFactoryManager* pAFM = m_pServiceManager->GetSystemServiceAs<AssetFactoryManager>();
    pAFM->UnregisterAssetFactory(EntitySetLoadState::GetCategory());

    return efd::IMessageHelperSystemService::OnShutdown();
}

//------------------------------------------------------------------------------------------------
void EntityLoaderService::OnLoadRequest(const EntityLoadRequest* i_pMsg, efd::Category)
{
    if (i_pMsg)
    {
        switch (i_pMsg->m_op)
        {
        case EntityLoadRequest::elo_Load: // fall through
        case EntityLoadRequest::elo_Reload:
            RequestEntitySetLoad(i_pMsg);
            break;

        case EntityLoadRequest::elo_Unload:
            RequestEntitySetUnload(i_pMsg);
            break;

        case EntityLoadRequest::elo_CancelLoad:
            CancelLoadRequest(
                i_pMsg->m_blockID,
                i_pMsg->m_settings.GetNotificationCategory(),
                i_pMsg->m_settings.GetNotificationContext(),
                i_pMsg->m_settings.GetNotificationBehavior());
            break;

        case EntityLoadRequest::elo_Invalid:
        default:
           // noop
           break;
        }
    }
}

//------------------------------------------------------------------------------------------------
AsyncResult EntityLoaderService::RequestEntitySetLoad(
    const BlockIdentification& i_assetID,
    const BlockLoadParameters* i_pParameters)
{
    EntityLoadRequest request(i_assetID, i_pParameters);
    return RequestEntitySetLoad(&request);
}

//------------------------------------------------------------------------------------------------
AsyncResult EntityLoaderService::RequestEntitySetLoad(const EntityLoadRequest* i_pMsg)
{
    const BlockIdentification& requestID = i_pMsg->m_blockID;

    // If there is a request for the Asset just add the new callback info to the existing data.
    LoadRequestMap::NodeType* pExisting = m_loadRequests.find(requestID);
    if (pExisting)
    {
        // If AddCallback fails that means the exact same callback already tried to load this
        // block. That is not expected, different people might depend on the same block being
        // loaded but they should each have unique callbacks.
        if (!pExisting->second.m_spEntitySetLoadState->AddCallback(i_pMsg->m_settings))
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR3,
                ("Duplicate load request for '%s(#%d)', second attempt ignored.",
                requestID.m_blockAsset.c_str(),
                requestID.m_instance));

            // We treat this as failure because the settings specified in the second request will
            // not be used. This means things like the transform and which result messages to send
            // are ignored.
            return AsyncResult_Failure;
        }
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
            ("ELS: Queuing entity set file load for BlockID = %s(#%d) ",
            requestID.m_blockAsset.c_str(),
            requestID.m_instance));

        LoadData data(EE_NEW EntitySetLoadState(i_pMsg, m_pServiceManager));
        m_loadRequests.push_back(requestID, data);

        if (m_requestToUUIDMap.find(requestID) == m_requestToUUIDMap.end())
        {
            m_requestToUUIDMap[requestID] = AssetID::INVALID_ASSET_ID;

            // do a locate so we have the UUID available.
            AssetLocatorService* pALS =
                m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
            EE_ASSERT(pALS);

            pALS->AssetLocate(requestID.m_blockAsset, m_myPrivateChannel);
        }
    }

    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult EntityLoaderService::RequestEntitySetEnterWorld(
    const BlockIdentification& i_assetID)
{
    efd::AsyncResult rval = AsyncResult_Failure;

    EntityManager* pEntityManager = m_pServiceManager->GetSystemServiceAs<EntityManager>();
    if (!pEntityManager)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("ELS:PlaceEntitySetInWorld failed to find Scheduler!"));
        return AsyncResult_Failure;
    }

    BlockIdentification requestID = GetRequestID(i_assetID);
    LoadedWorldMap::iterator foundIt = m_loaded.find(requestID);
    if (foundIt != m_loaded.end())
    {
        WorldLoadedInfo& wli = *foundIt->second;
        rval = DoEntitySetEnterWorld(pEntityManager, wli.m_entities);
    }

    return rval;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult EntityLoaderService::DoEntitySetEnterWorld(
    EntityManager* pEntityManager,
    EntityIdSet& entities)
{
    for (EntityIdSet::const_iterator entityIt = entities.begin();
        entityIt != entities.end();
        ++entityIt)
    {
        Entity* pEntity = pEntityManager->LookupEntity(*entityIt);
        if (pEntity)
        {
            pEntity->EnterWorld();
        }
    }
    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult EntityLoaderService::RequestEntitySetExitWorld(const BlockIdentification& blockID)
{
    EntityManager* pEntityManager = m_pServiceManager->GetSystemServiceAs<EntityManager>();
    if (!pEntityManager)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("ELS:RequestEntitySetEnterWorld failed to find Scheduler!"));
        return AsyncResult_Failure;
    }

    BlockIdentification requestID = GetRequestID(blockID);
    LoadedWorldMap::iterator foundIt = m_loaded.find(requestID);
    if (foundIt != m_loaded.end())
    {
        WorldLoadedInfo& wli = *foundIt->second;
        DoEntitySetExitWorld(pEntityManager, wli.m_entities);
        return AsyncResult_Pending;
    }

    return AsyncResult_Failure;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult EntityLoaderService::DoEntitySetExitWorld(
    EntityManager* pEntityManager,
    EntityIdSet& entities)
{
    for (EntityIdSet::iterator entityIt = entities.begin();
        entityIt != entities.end();
        ++entityIt)
    {
        Entity* pEntity = pEntityManager->LookupEntity(*entityIt);
        if (pEntity)
        {
            pEntity->ExitWorld();
        }
    }

    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult EntityLoaderService::RequestEntitySetUnload(
    const BlockIdentification& i_blockID,
    efd::Category i_entity,
    efd::UInt32 i_context,
    egf::BehaviorID i_callback)
{
    EntityLoadRequest request(
        EntityLoadRequest::elo_Unload,
        i_blockID,
        i_entity,
        i_callback,
        i_context);
    return RequestEntitySetUnload(&request);
}

//------------------------------------------------------------------------------------------------
AsyncResult EntityLoaderService::RequestEntitySetUnload(const EntityLoadRequest* i_pMsg)
{
    BlockIdentification requestID = GetRequestID(i_pMsg->m_blockID);

    LoadRequestMap::NodeType* it = m_unloadRequests.find(requestID);
    if (it)
    {
        it->second.m_spEntitySetLoadState->AddCallback(i_pMsg->m_settings);
    }
    else
    {
        LoadData data(EE_NEW EntitySetLoadState(i_pMsg, m_pServiceManager));
        m_unloadRequests.push_back(requestID, data);
    }

    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
bool EntityLoaderService::CancelLoadRequest(
    const BlockIdentification& i_blockID,
    efd::Category i_cb,
    Context i_ctx,
    egf::BehaviorID i_callback)
{
    // find the underlying request ID.
    BlockIdentification requestId = GetRequestID(i_blockID);

    LoadRequestMap::NodeType* pendingLoad = m_loadRequests.find(requestId);
    if (pendingLoad)
    {
        SendResult(
            pendingLoad->second.m_spEntitySetLoadState->m_callbacks,
            EntityLoadResult::elr_LoadCancelSuccess,
            i_blockID);

        m_loadRequests.erase(pendingLoad);
    }

    SendResult(
        i_cb,
        i_callback,
        i_ctx,
        EntityLoadResult::elr_LoadCancelSuccess,
        i_blockID);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult EntityLoaderService::ProcessLoadRequest(
    BlockIdentification& requestId,
    LoadData& loadData)
{
    // If we have an unload pending for this requestID, wait until it completes.
    if (m_unloadRequests.find(requestId))
    {
        return AsyncResult_Pending;
    }

    EntitySetLoadStatePtr els = loadData.m_spEntitySetLoadState;

    switch (els->m_asset_status)
    {
    case EntitySetLoadState::LOAD_STARTED:
        {
            // Still waiting on a response to resolve the Asset ID
            EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
                ("ELS: Load started; waiting for response for BlockID = %s(#%d)",
                requestId.m_blockAsset.c_str(),
                requestId.m_instance));
        }
        break;

    case EntitySetLoadState::LOAD_NOT_STARTED:
        {
            els->BeginAssetLoad();
            EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
                ("ELS: Starting load for BlockID = %s(#%d)",
                requestId.m_blockAsset.c_str(),
                requestId.m_instance));
        }
        break;

    case EntitySetLoadState::LOAD_COMPLETE:
        {
            EntityLoadResult::Result result = EntityLoadResult::elr_Unknown;
            efd::AsyncResult ar = LoadEntitySet(requestId, loadData, result);
            if (result != EntityLoadResult::elr_Unknown)
            {
                SendResult(els->m_callbacks, result, requestId);
            }
            return ar;
        }

    case EntitySetLoadState::LOAD_FAILED:
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                ("Failed to load entity set for BlockID = %s(#%d)",
                requestId.m_blockAsset.c_str(),
                requestId.m_instance));

            SendResult(els->m_callbacks, EntityLoadResult::elr_AssetIDNotFound, requestId);
        }
        return AsyncResult_Failure;
    }

    return AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult EntityLoaderService::ProcessUnloadRequest(
    BlockIdentification& requestId,
    LoadData& ld)
{
    // If we have an EntityManager and we aren't in shutdown already then grab a pointer. Otherwise
    // we will short-circuit things as we can no longer rely on get callback notifications.
    EntityManager* pEntityManager = NULL;
    if (m_pServiceManager->GetCurrentState() != ServiceManager::kSysServState_ShuttingDown)
    {
        pEntityManager = m_pServiceManager->GetSystemServiceAs<EntityManager>();
    }

    EntitySetLoadStatePtr els = ld.m_spEntitySetLoadState;

    LoadedWorldMap::iterator iterLoaded = m_loaded.find(requestId);

    switch (ld.m_step)
    {
    default:
        EE_FAIL("Invalid state");
        break;

    case LoadData::lds_Initial:
        {
            LoadRequestMap::NodeType* pendingLoad = m_loadRequests.find(requestId);
            if (pendingLoad)
            {
                // If we have a pending load, cancel it.
                CancelLoadRequest(requestId);
            }

            if (iterLoaded == m_loaded.end())
            {
                // request to unload a block that isn't loaded
                if (!pendingLoad)
                {
                    SendResult(els->m_callbacks, EntityLoadResult::elr_AssetIDNotFound, requestId);
                    return AsyncResult_Failure;
                }

                // we canceled a block load before it finished, consider this a success
                SendResult(els->m_callbacks, EntityLoadResult::elr_LoadCancelSuccess, requestId);
                ld.m_step = LoadData::lds_Done;
                return AsyncResult_Complete;
            }

            // otherwise we have some work to do:
            ld.m_step = LoadData::lds_UnloadStarted;
        }
        /// ... fall through ...

    case LoadData::lds_UnloadStarted:
        {
            // This should be impossible as we should have returned in the previous case statement
            // under this condition.
            EE_ASSERT(iterLoaded != m_loaded.end());

            if (pEntityManager)
            {
                DoEntitySetExitWorld(pEntityManager, iterLoaded->second->m_entities);
            }
        }
        ld.m_step = LoadData::lds_DestroyEntities;
        /// ... fall through ...

    case LoadData::lds_DestroyEntities:
        if (iterLoaded != m_loaded.end())
        {
            // We hold a reference to the WorldLoadedInfo because it could be removed during one
            // of the Scheduler::OnDestroy calls below.
            WorldLoadedInfoPtr spWLI = iterLoaded->second;
            WorldLoadedInfo& wli = *spWLI;

            UInt32 throttle = GetMaxEntityUnloadThrottle();
            efd::UInt32 blockThrottle =
                wli.m_spEntitySetLoadState->m_settings.GetUnloadThresholdOverride();
            if (blockThrottle != BlockLoadParameters::k_UseDefaultLimit)
            {
                throttle = blockThrottle;
            }

            if (pEntityManager)
            {
                // Destroy all entities
                for (EntityIdSet::iterator iter = wli.m_entities.begin();
                    iter != wli.m_entities.end();
                    /* do nothing--see below */)
                {
                    // Check the count here after we compare iter against the end so we don't return
                    // false if we happen to have already finished. Also we could exceed this limit
                    // right from the start if we override the threshold and another block unloaded
                    // already this tick.
                    if (m_entitiesUnloadedThisTick >= throttle)
                    {
                        return AsyncResult_Pending;
                    }

                    EntityID eid = *iter;

                    // We MUST increment the iterator here because the DestoyEntity call will cause
                    // OnEntityRemoved to be called before returning if the entity has no shutdown
                    // behaviors, which modifies the map that we are iterating over, thus
                    // invalidating our iterator.
                    ++iter;

                    if (pEntityManager->DestroyEntity(eid))
                    {
                        // This will return false if the entity isn't found or if it's already being
                        // destroyed. In those cases, don't count it against entities destroyed this
                        // frame.
                        ++m_entitiesUnloadedThisTick;
                    }
                }
            }
            else
            {
                // no scheduler, just assume all entities are destroyed already
                wli.m_entities.clear();
            }

            SendResult(
                wli.m_spEntitySetLoadState->m_callbacks,
                EntityLoadResult::elr_Unloaded,
                requestId);
            SendResult(els->m_callbacks, EntityLoadResult::elr_Unloaded, requestId);

            // See if the block is still loaded, it might have completely finished unloading before
            // Scheduler::OnDestroy even returned.
            iterLoaded = m_loaded.find(requestId);
        }
        ld.m_step = LoadData::lds_WaitForDestruction;
        /// ... fall through ...

    case LoadData::lds_WaitForDestruction:
        if (iterLoaded == m_loaded.end())
        {
            // Yeah, this is what we wanted, the unload has completed
            SendResult(
                els->m_callbacks,
                EntityLoadResult::elr_EntityDestructionCompleted,
                requestId);
            ld.m_step = LoadData::lds_Done;
            return AsyncResult_Complete;
        }
        else
        {
            WorldLoadedInfoPtr spWLI = iterLoaded->second;
            WorldLoadedInfo& wli = *spWLI;

            // There is a bug someplace that can result in failure to receive all the entity
            // destruction messages. We need to remove any bogus entries from m_entities, if the
            // result is an empty set then we treat this as completion. In that case the normal
            // completion message will not get sent to the original callbacks.
            if (pEntityManager)
            {
                for (EntityIdSet::iterator entityIt = wli.m_entities.begin();
                    entityIt != wli.m_entities.end();
                    /*do nothing*/)
                {
                    EntityPtr spEntity = pEntityManager->LookupEntity(*entityIt);
                    if (spEntity)
                    {
                        ++entityIt;
                    }
                    else
                    {
                        // Unexpected, all entities in the set should exist!
                        wli.m_entities.erase(entityIt++);
                    }
                }
            }
            else
            {
                wli.m_entities.clear();
            }

            bool actuallyFinished = wli.m_entities.empty();
            if (actuallyFinished)
            {
                SendResult(
                    wli.m_spEntitySetLoadState->m_callbacks,
                    EntityLoadResult::elr_EntityDestructionCompleted,
                    requestId);
                SendResult(
                    els->m_callbacks,
                    EntityLoadResult::elr_EntityDestructionCompleted,
                    requestId);
                ld.m_step = LoadData::lds_Done;
                return AsyncResult_Complete;
            }
        }
        return AsyncResult_Pending;
    }

    EE_FAIL("Should be unreachable");
    return AsyncResult_Failure;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 EntityLoaderService::SendResult(
    const EntitySetLoadState::CallbackMap& i_callbacks,
    EntityLoadResult::Result i_elr,
    const BlockIdentification& i_assetID)
{
    efd::UInt32 cSuccess = 0;
    if (m_pMessageService)
    {
        for (EntitySetLoadState::CallbackMap::const_iterator iter = i_callbacks.begin();
             iter != i_callbacks.end();
             ++iter)
        {
            const EntitySetLoadState::CallbackIdentity& cbi = iter->first;
            const EntitySetLoadState::CallbackData& cbd = iter->second;

            if (ShouldSendResult(i_elr, cbd.second))
            {
                SendResult(cbi.first, cbi.second, cbd.first, i_elr, i_assetID);
                ++cSuccess;
            }
        }
    }
    return cSuccess;
}

//------------------------------------------------------------------------------------------------
bool EntityLoaderService::SendResult(
    efd::Category i_cb,
    egf::BehaviorID i_behavior,
    Context i_ctx,
    EntityLoadResult::Result i_elr,
    const BlockIdentification& i_assetID)
{
    if (m_pMessageService)
    {
        if (i_cb.IsValid())
        {
            IMessagePtr pResult;
            if (i_behavior)
            {
                // behavior callback
                ParameterListPtr args = EE_NEW ParameterList();
                args->AddParameter("BlockAsset", i_assetID.m_blockAsset);
                args->AddParameter("BlockInstance", i_assetID.m_instance);
                args->AddParameter("Result", i_elr);
                args->AddParameter("Context", i_ctx);

                pResult = EventMessage::CreateEvent(i_cb, 0, 0, i_behavior, 0.0, args);
            }
            else
            {
                pResult = EE_NEW EntityLoadResult(i_assetID, i_ctx, i_elr);
            }

            m_pMessageService->Send(pResult, i_cb);
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------------------------
AsyncResult EntityLoaderService::LoadEntitySet(
    const BlockIdentification& i_requestId,
    EntityLoaderService::LoadData& io_data,
    EntityLoadResult::Result& o_loadResult)
{
    EE_ASSERT(io_data.m_spEntitySetLoadState);

    EntitySetLoadState* els = io_data.m_spEntitySetLoadState;

    efd::UInt32 entityLoadLimit = GetMaxEntityLoadThrottle();
    efd::UInt32 blockThrottle = els->m_settings.GetLoadThresholdOverride();
    if (blockThrottle != BlockLoadParameters::k_UseDefaultLimit)
    {
        entityLoadLimit = blockThrottle;
    }

    // We might already be over our limit because our limit could be less than the default limit
    // and another block might have already loaded this tick.
    if (m_entitiesLoadedThisTick >= entityLoadLimit)
    {
        return AsyncResult_Pending;
    }

    UInt32 errors;
    efd::list<EntityPtr>* newEntityList = NULL;

    switch (io_data.m_step)
    {
    default:
        EE_FAIL("Invalid state");
        break;

    case LoadData::lds_Initial:
        {
            EE_ASSERT(io_data.m_pParser == 0);
            EE_ASSERT(io_data.m_spEntitySetLoadState->m_spWorldFactoryResponse);

            // If already loaded and we're not reloading, don't load it again.
            LoadedWorldMap::iterator itPrevious = m_loaded.find(i_requestId);
            if (!els->IsReload() && itPrevious != m_loaded.end())
            {
                o_loadResult = EntityLoadResult::elr_AlreadyLoaded;
                return AsyncResult_Complete;
            }

            // Make sure that flat model manager is in the state where it can accept requests
            FlatModelManager* pfmm = m_pServiceManager->GetSystemServiceAs<FlatModelManager>();
            if (!pfmm)
            {
                o_loadResult = EntityLoadResult::elr_Failed;
                return AsyncResult_Failure;
            }
            if (!pfmm->IsAvailable())
            {
                EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
                    ("ELS: FlatModelManager not ready, waiting for it to finish asset lookup..."));
                return AsyncResult_Pending;
            }

            typedef WorldFactoryResponse::FlatModelResponseSet FlatModelMsgs;
            const FlatModelMsgs& flatModelMsgs = els->m_spWorldFactoryResponse->GetFlatModels();

            for (FlatModelMsgs::const_iterator it2 = flatModelMsgs.begin();
                it2 != flatModelMsgs.end();
                ++it2)
            {
                pfmm->HandleAssetLoadMsg(*it2, kCAT_INVALID);
            }
            els->m_spWorldFactoryResponse = NULL;

            EntityManager* pEntityMgr = m_pServiceManager->GetSystemServiceAs<EntityManager>();

            efd::set<EntityID>* pPrevious = NULL;
            if (itPrevious != m_loaded.end())
            {
                pPrevious = &itPrevious->second->m_entities;
            }

            io_data.m_pParser = EE_NEW SAXEntityParser(
                els->m_entity_buffer.c_str(),
                els->m_entity_file,
                i_requestId.m_instance,
                pfmm,
                pEntityMgr,
                pPrevious);
            if (els->m_settings.UseRotation())
                io_data.m_pParser->ApplyRotation(els->m_settings.GetRotation());
            if (els->m_settings.UseOffset())
                io_data.m_pParser->ApplyOffset(els->m_settings.GetOffset());

            if (SAXEntityParser::sep_Failed == io_data.m_pParser->BeginParse(errors))
            {
                o_loadResult = EntityLoadResult::elr_ParseFailed;
                return AsyncResult_Failure;
            }
            else
            {
                o_loadResult = EntityLoadResult::elr_Loading;
            }
            io_data.m_step = LoadData::lds_Parse;
        }
        // ... fall through ...

    case LoadData::lds_Parse:
        {
            EE_ASSERT(io_data.m_pParser != 0);

            // Set this each time to the remaining portion of the parse limit
            io_data.m_pParser->SetMaxEntityLoadsPerParsing(entityLoadLimit - m_entitiesLoadedThisTick);

            SAXEntityParser::Result result = io_data.m_pParser->Parse(newEntityList, errors);

            // @todo: we should take the already created entities and start placing them into the
            // world immediately. Currently we don't do this because we need to know the total
            // number of entities before we load any to avoid accidentally sending "block finished"
            // messages too soon, but if we remove the various message sending calls from the
            // entity Create/EnterWorld/Destroy methods and instead rely on the Process(Unl|L)oad
            // methods than that would no longer be an issue.

            efd::UInt32 additionalEntitiesParsed = io_data.m_pParser->GetEntitiesParsed();
            m_entitiesLoadedThisTick += additionalEntitiesParsed;

            switch (result)
            {
            default:
            case SAXEntityParser::sep_Failed:
                o_loadResult = EntityLoadResult::elr_ParseFailed;
                return AsyncResult_Failure;

            case SAXEntityParser::sep_Loading:
                EE_ASSERT(m_entitiesLoadedThisTick >= entityLoadLimit);
                return AsyncResult_Pending;

            case SAXEntityParser::sep_Loaded:
                // SAXEntityParser::Parse should have returned sep_Loading if we hit the limit:
                EE_ASSERT(m_entitiesLoadedThisTick < entityLoadLimit);
                // release the memory for the parsed block file
                els->m_entity_buffer.clear();
                break;
            }
            // Delete any entities from the scheduler that are no longer in the world.  With Rapid
            // Iteration we might actually be re-loading a block rather than loading it so that means
            // already created entities from the previous load might need to be deleted. NOTE: there is
            // a rare case when all previously loaded entities are no longer in the block in which case
            // DeleteRemovedEntities can remove 'bid' from 'm_loaded'.
            DeleteRemovedEntities(i_requestId, newEntityList);
            // Since this step only happens during rapid iteration, we consider it zero cost and do not
            // modify m_entitiesLoadedThisTick
            // Loaded Worlds info
            WorldLoadedInfoPtr pWLI = EE_NEW WorldLoadedInfo;
            pWLI->m_originalRequestID = i_requestId;
            pWLI->m_spEntitySetLoadState = els;

            // Add all the new entities to the EntitySet. Some of the entities might not be new
            // because we might be reloading the block due to Rapid Iteration.
            for (EntityList::iterator iter = newEntityList->begin();
                iter != newEntityList->end();
                ++iter)
            {
                Entity* pEntity = *iter;

                // Add all entities to the list of entities loaded from this block file asset.
                pWLI->m_entities.insert(pEntity->GetEntityID());

                // For every newly added entity we expect to receive an OnEntityAdded message, so
                // add it to the new set.  Previously existing entities in rapid iteration
                // scenarios will have already sent this message so we won't receive a new one.
                if (NULL == pEntity->GetEntityManager())
                {
                    pWLI->m_newEntities.insert(pEntity->GetEntityID());
                }
            }
            m_loaded[i_requestId] = pWLI;
            io_data.m_step = LoadData::lds_AddEntites;
        }
        // ... fall through ...

    case LoadData::lds_AddEntites:
        {
            EntityManager* pEntityMgr = m_pServiceManager->GetSystemServiceAs<EntityManager>();
            if (!pEntityMgr)
            {
                o_loadResult = EntityLoadResult::elr_Failed;
                return AsyncResult_Failure;
            }

            newEntityList = io_data.m_pParser->GetParserResults();

            ParameterListPtr params = EE_NEW ParameterList();
            params->AddParameter("AutoEnterWorld", els->m_settings.GetAutoEnterWorld());

            // Then add the entities to the scheduler after we have cached all the entities to load
            // in the world. We do this to sync the entity count so that a 'world loaded' message
            // is sent only when all entities have been created.
            for (EntityList::iterator iter = newEntityList->begin();
                iter != newEntityList->end();
                ++iter)
            {
                Entity* pEntity = *iter;
                // Add the entity to the simulator only if it has not already been added.  If we are
                // rapidly iterating this entity might be a previously existing entity that was added
                // when the block was previously loaded or changed.
                if (NULL == pEntity->GetEntityManager())
                {
                    pEntityMgr->AddEntity(pEntity, params);
                    // Adding an entity kicks off creation lifecycles which runs behaviors. We
                    // don't want to flood too many of those at once.
                    if (++m_entitiesLoadedThisTick >= entityLoadLimit)
                    {
                        return AsyncResult_Pending;
                    }
                }
            }
            // Now that we've finished with the contents of newEntityList we can free the parser
            // memory:
            EE_DELETE io_data.m_pParser;
            io_data.m_pParser = NULL;
            io_data.m_step = LoadData::lds_Done;
        }
        // ... fall through ...

    case LoadData::lds_Done:
        o_loadResult = EntityLoadResult::elr_Loaded;
        return AsyncResult_Complete;
    }

    EE_FAIL("Should not be reachable.");
    return AsyncResult_Failure;
}

//------------------------------------------------------------------------------------------------
void EntityLoaderService::DeleteRemovedEntities(
    const BlockIdentification& bid,
    const EntityList* newEntityList)
{
    EE_ASSERT(newEntityList);

    Scheduler* pScheduler = m_pServiceManager->GetSystemServiceAs<Scheduler>();
    if (!pScheduler)
    {
        return;
    }

    LoadedWorldMap::iterator it = m_loaded.find(bid);

    if (it == m_loaded.end())
    {
        // we haven't loaded this asset yet so we can't have any removed entities.
        return;
    }

    // Walk our list of previously loaded entities. If we find one that doesn't exist in
    // the new entity list remove it from the simulator.

    // Hold a reference to the WLI because in the rare case that all entities in the block are
    // removed we could end up removing the WLI from m_loaded during the Scheduler::DestroyEntity
    // call below.
    WorldLoadedInfoPtr spWLI = it->second;
    EntityIdSet& entities = spWLI->m_entities;
    for (EntityIdSet::const_iterator entityIt = entities.begin();
        entityIt != entities.end();
        /*do nothing, iter is advanced below*/)
    {
        bool found = false;
        EntityPtr spEntity = pScheduler->FindEntity(*entityIt);

        // Calling DestroyEntity below can result in immediate removal of the entity from the
        // set (when the entity has no OnDestroy behavior), so we advance the iterator before
        // calling destroy to be safe.
        ++entityIt;

        if (spEntity)
        {
            // If we find an existing entity not in the new entity list, remove it from the
            // simulator.
            for (EntityList::const_iterator newEntityIt = newEntityList->begin();
                 newEntityIt != newEntityList->end();
                 ++newEntityIt)
            {
                const efd::ID128& newId = (*newEntityIt)->GetDataFileID();
                const efd::ID128& oldId = spEntity->GetDataFileID();

                if (oldId == newId)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                pScheduler->DestroyEntity(spEntity);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void EntityLoaderService::OnWorldResetRequest()
{
    // Broadcast the news about the world being reset
    if (m_pMessageService)
    {
        IMessagePtr spMsgPtr = EE_NEW MessageWrapper< IMessage, kMSGID_WorldResetBegin >;
        m_pMessageService->SendImmediate(
            spMsgPtr,
            m_pMessageService->GetServicePublicCategory(efd::kCLASSID_EntityLoaderService));
    }

    // Unload the existing block files.
    for (LoadedWorldMap::const_iterator it = m_loaded.begin(); it != m_loaded.end(); ++it)
    {
        // Track the world to load it later.
        m_worldsToLoadAfterReset[it->first] = it->second->m_spEntitySetLoadState;

        // Unload the block.
        RequestEntitySetUnload(it->first, m_myPrivateChannel);
    }

    // Continue processing in the HandleResetUnloadComplete handler.
}

//------------------------------------------------------------------------------------------------
void EntityLoaderService::HandleResetUnloadComplete(
    const EntityLoadResult* i_pMsg,
    efd::Category targetChannel)
{
    if (i_pMsg->m_result == EntityLoadResult::elr_EntityDestructionCompleted && m_loaded.empty())
    {
        Scheduler* pScheduler = m_pServiceManager->GetSystemServiceAs<Scheduler>();
        EE_ASSERT(pScheduler);

        // clear all the remaining (dynamically created) entities.
        pScheduler->DeleteAllEntities();

        // We'll request the load in the HandleSchedulerCleared handler.
    }
}


//------------------------------------------------------------------------------------------------
void EntityLoaderService::HandleSchedulerCleared(const IMessage *, efd::Category)
{
    // Broadcast the news about the world that finished resetting
    if (m_pMessageService)
    {
        IMessagePtr spMsgPtr = EE_NEW MessageWrapper< IMessage, kMSGID_WorldResetEnd >;
        m_pMessageService->SendImmediate(
            spMsgPtr,
            m_pMessageService->GetServicePublicCategory(efd::kCLASSID_EntityLoaderService));
    }

    // Now load all the worlds.
    for (ResetWorldsToLoadMap::const_iterator worldIt = m_worldsToLoadAfterReset.begin();
        worldIt != m_worldsToLoadAfterReset.end();
        ++worldIt)
    {
        const BlockIdentification& worldAssetId = worldIt->first;
        BlockLoadParameters blp(worldIt->second->m_settings);

        // We need to request this for each callback in the original load state. Only the first
        // request will actually load the block; the remaining requests simply add the callbacks
        // to the initial request.
        EntitySetLoadState::CallbackMap::const_iterator cbIt = worldIt->second->m_callbacks.begin();
        if (cbIt == worldIt->second->m_callbacks.end())
        {
            // Even if there are no callbacks, we still need to reload the block
            RequestEntitySetLoad(worldAssetId, &blp);
        }
        else
        {
            for (; cbIt != worldIt->second->m_callbacks.end(); ++cbIt)
            {
                // It just happens that even if the BehaviorID is zero this will do the right thing
                // resulting in a message-style callback.
                blp.SetBehaviorCallback(cbIt->first.first, cbIt->first.second, cbIt->second.first);
                blp.SetActiveCallbacks(cbIt->second.second);

                RequestEntitySetLoad(worldAssetId, &blp);
            }
        }
    }

    m_worldsToLoadAfterReset.clear();
}

//------------------------------------------------------------------------------------------------
void EntityLoaderService::OnEntityAdded(const EntityChangeMessage* i_pMsg, efd::Category)
{
    Entity* pEntity = i_pMsg->GetEntity();
    EntityID eid = pEntity->GetEntityID();

    for (LoadedWorldMap::iterator iter = m_loaded.begin(); iter != m_loaded.end(); ++iter)
    {
        WorldLoadedInfo& wli = *iter->second;
        if (!wli.m_spEntitySetLoadState->m_settings.GetAutoEnterWorld())
        {
            if (wli.m_newEntities.count(eid))
            {
                wli.m_createdCount++;
                if (wli.m_createdCount >= wli.m_newEntities.size())
                {
                    // Finished creating the entities in this block and they aren't going to
                    // auto-enter the world so send the completed event now.
                    SendResult(
                        wli.m_spEntitySetLoadState->m_callbacks,
                        EntityLoadResult::elr_EntityCreationCompleted,
                        wli.m_originalRequestID);

                    // Kick off the OnEntitySetFinished lifecycle event for all the new entities in
                    // this block.
                    InvokeBlockLoadedBehaviors(wli.m_newEntities);

                    // Done loading, no more need to keep these new guys lying around
                    wli.m_newEntities.clear();
                    wli.m_createdCount = 0;
                }
                break;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void EntityLoaderService::OnEntityEnterWorld(const EntityChangeMessage* i_pMsg, efd::Category)
{
    Entity* pEntity = i_pMsg->GetEntity();
    EntityID eid = pEntity->GetEntityID();

    for (LoadedWorldMap::iterator iter = m_loaded.begin(); iter != m_loaded.end(); ++iter)
    {
        WorldLoadedInfo& wli = *iter->second;
        // we only care about entity sets that are set to automatically enter the world, for other
        // sets we send elr_EntityCreationCompleted when the creation events finish.
        if (wli.m_spEntitySetLoadState->m_settings.GetAutoEnterWorld())
        {
            if (wli.m_newEntities.count(eid))
            {
                wli.m_enteredWorldCount++;
                if (wli.m_enteredWorldCount >= wli.m_newEntities.size())
                {
                    // finished creating the entities in this block!
                    SendResult(
                        wli.m_spEntitySetLoadState->m_callbacks,
                        EntityLoadResult::elr_EntityCreationCompleted,
                        wli.m_originalRequestID);

                    // Kick off the OnEntitySetFinished lifecycle event for all the entities in
                    // this block.
                    InvokeBlockLoadedBehaviors(wli.m_newEntities);

                    // Done loading, no more need to keep these new guys lying around
                    wli.m_newEntities.clear();
                    wli.m_enteredWorldCount = 0;
                }
                break;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void EntityLoaderService::OnEntityRemoved(const EntityChangeMessage* i_pMsg, efd::Category)
{
    Entity* pEntity = i_pMsg->GetEntity();
    EntityID eid = pEntity->GetEntityID();

    for (LoadedWorldMap::iterator iter = m_loaded.begin(); iter != m_loaded.end(); ++iter)
    {
        WorldLoadedInfo& wli = *iter->second;
        EntityIdSet& entitySet = wli.m_entities;
        if (entitySet.erase(eid))
        {
            if (entitySet.empty())
            {
                SendResult(
                    wli.m_spEntitySetLoadState->m_callbacks,
                    EntityLoadResult::elr_EntityDestructionCompleted,
                    wli.m_originalRequestID);

                // remove from the loaded list
                m_loaded.erase(iter);
            }
            break;
        }
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 EntityLoaderService::ReloadWorld(const efd::AssetID& assetId)
{
    efd::UInt32 result = 0;
#if defined(EE_CONFIG_SHIPPING)
    EE_UNUSED_ARG(assetId);
#else
    // Get just the AssetID portion from the map
    AssetID requestId = GetRequestID(assetId);

    // search for any BlockIdentification using that AssetID:
    for (LoadedWorldMap::iterator it = m_loaded.begin();
        it != m_loaded.end();
        ++it)
    {
        if (it->first.m_blockAsset == requestId)
        {
            // Copy the original block load parameters but change the callback
            BlockLoadParameters& blp = it->second->m_spEntitySetLoadState->m_settings;

            // Reload using the exact original BlockIdentification to preserve the instance number
            EntityLoadRequest request(it->first, &blp, true);
            RequestEntitySetLoad(&request);
        }
    }
#endif
    return result;
}

//------------------------------------------------------------------------------------------------
void EntityLoaderService::CleanupAllLoadedWorlds()
{
    for (LoadedWorldMap::const_iterator it = m_loaded.begin(); it != m_loaded.end(); ++it)
    {
        RequestEntitySetUnload(it->first);
    }
}

//------------------------------------------------------------------------------------------------
void EntityLoaderService::InvokeBlockLoadedBehaviors(
    const EntityLoaderService::EntityIdSet& entities)
{
    EntityManager* pEntityManager = m_pServiceManager->GetSystemServiceAs<EntityManager>();

    for (EntityIdSet::const_iterator iter = entities.begin();
         iter != entities.end();
         ++iter)
    {
        Entity* pEntity = pEntityManager->LookupEntity(*iter);
        if (pEntity)
        {
            pEntity->OnEntitySetFinished();
        }
    }
}

//------------------------------------------------------------------------------------------------
void EntityLoaderService::HandleAssetLocatorResponse(
    const efd::AssetLocatorResponse* pMessage,
    efd::Category targetCategory)
{
    if (pMessage->GetAssetURLMap().size() == 0)
    {
        // Asset not found...
        m_requestToUUIDMap.erase(pMessage->GetResponseURI());
    }
    else
    {
        // We expect each request to resolve to one and only one asset.
        AssetLocatorResponse::AssetURLMap::const_iterator urlIt =
            pMessage->GetAssetURLMap().begin();

        if (urlIt != pMessage->GetAssetURLMap().end())
        {
            m_requestToUUIDMap[pMessage->GetResponseURI()] = urlIt->first;
        }
    }
}

//------------------------------------------------------------------------------------------------
bool EntityLoaderService::IsLoaded(const BlockIdentification& blockId)
{
    BlockIdentification requestId = GetRequestID(blockId);
    return m_loaded.find(requestId) != m_loaded.end();
}

//------------------------------------------------------------------------------------------------
BlockIdentification EntityLoaderService::GetRequestID(const BlockIdentification& blockID) const
{
    if (blockID.IsUUID())
    {
        for (RequestToUUIDMap::const_iterator it = m_requestToUUIDMap.begin();
            it != m_requestToUUIDMap.end();
            ++it)
        {
            if (it->second == blockID.m_blockAsset)
            {
                return BlockIdentification(it->first, blockID.m_instance);
            }
        }
    }
    return blockID;
}

//------------------------------------------------------------------------------------------------
bool EntityLoaderService::ShouldSendResult(EntityLoadResult::Result result, efd::UInt32 flags)
{
    efd::UInt32 flagToCheck = 0;

#define EE_CHECKRESULT(name) case EntityLoadResult::elr_##name: \
    flagToCheck = (efd::UInt32)BlockLoadParameters::blc_##name; break

    switch (result)
    {
        EE_CHECKRESULT(Loading);
        EE_CHECKRESULT(Loaded);
        EE_CHECKRESULT(AlreadyLoaded);
        EE_CHECKRESULT(EntityCreationCompleted);
        EE_CHECKRESULT(LoadCancelSuccess);
        EE_CHECKRESULT(Unloaded);
        EE_CHECKRESULT(EntityDestructionCompleted);
        EE_CHECKRESULT(Failed);
        EE_CHECKRESULT(RequestFailure);
        EE_CHECKRESULT(EntitySetNotFound);
        EE_CHECKRESULT(AssetIDNotFound);
        EE_CHECKRESULT(ParseFailed);

    default:
        EE_FAIL("Invalid state");
        break;
    }
#undef EE_CHECKRESULT

    // If this asserts it most likely means a new EntityLoadResult::Result value was added but the
    // BlockLoadParameters::BlockLoadCallback was not updated.
    EE_ASSERT(flagToCheck);
    return BitUtils::AnyBitsAreSet(flags, flagToCheck);
}


//------------------------------------------------------------------------------------------------
EntityLoaderService::LoadData::LoadData()
    : m_spEntitySetLoadState()
    , m_pParser(NULL)
    , m_step(EntityLoaderService::LoadData::lds_Initial)
{
}

//------------------------------------------------------------------------------------------------
EntityLoaderService::LoadData::LoadData(EntitySetLoadState* pESLS)
    : m_spEntitySetLoadState(pESLS)
    , m_pParser(NULL)
    , m_step(EntityLoaderService::LoadData::lds_Initial)
{
}

//------------------------------------------------------------------------------------------------
EntityLoaderService::LoadData::~LoadData()
{
    EE_DELETE m_pParser;
}

//------------------------------------------------------------------------------------------------
