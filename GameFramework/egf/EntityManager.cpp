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

#include <egf/EntityManager.h>
#include <egf/FlatModelManager.h>
#include <egf/EntityChangeMessage.h>
#include <efd/Metrics.h>
#include <egf/egfPropertyIDs.h>
#include <efd/AssetLocatorService.h>
#include <egf/Scheduler.h>
#include <egf/egfLogIDs.h>
#include <efd/ServiceManager.h>
#include <efd/AssetFactoryManager.h>
#include <egf/EntityFactoryRequest.h>
#include <egf/EntityFactoryResponse.h>
#include <egf/EntityIDFactory.h>
#include <egf/FlatModelFactoryResponse.h>
#include <egf/EntityFactory.h>
#include <efd/AssetLoadRequest.h>
#include <efd/AssetLoadResponse.h>
#include <egf/EntityLoaderMessages.h>
#include <egf/SimDebugger.h>
#include <egf/StandardModelLibraryPropertyIDs.h>


using namespace efd;
using namespace egf;

using EE_STL_NAMESPACE::pair;
using EE_STL_NAMESPACE::multimap;
using EE_STL_NAMESPACE::make_pair;

//------------------------------------------------------------------------------------------------
// Scheduler class method implementations
//
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntityManager);


//------------------------------------------------------------------------------------------------
EE_HANDLER(EntityManager,
           HandleEntityFactoryResponse,
           EntityFactoryResponse);

//------------------------------------------------------------------------------------------------
EE_HANDLER(EntityManager,
           HandleEntityPreloadResponse,
           EntityPreloadResponse);


//------------------------------------------------------------------------------------------------
EntityManager::EntityManager()
    : m_entityLoadCategory(UniversalID::ECU_Point2Point,kNetID_ISystemService,kCLASSID_EntityManager)
    , m_spOwnedEntityAddedMsg(EE_NEW OwnedEntityAddedMessage)
    , m_spOwnedEntityEnterWorldMsg(EE_NEW OwnedEntityEnterWorldMessage)
    , m_spOwnedEntityUpdatedMsg(EE_NEW OwnedEntityUpdatedMessage)
    , m_spOwnedEntityExitWorldMsg(EE_NEW OwnedEntityExitWorldMessage)
    , m_spOwnedEntityRemovedMsg(EE_NEW OwnedEntityRemovedMessage)
    , m_defaultAutoEnterWorld(true)
    , m_entitiesRemovingList(0)
    , m_shuttingDown(false)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 5450;

    m_tickCount = 0;
    m_preloadWarningThreshold = 200;
}

//------------------------------------------------------------------------------------------------
EntityManager::~EntityManager()
{
    m_EntityCacheMap.clear();
}


//------------------------------------------------------------------------------------------------
EntityPtr EntityManager::EntityFactoryMethod(
    const FlatModel* i_pModel,
    egf::EntityID i_eid,
    bool i_master)
{
    return EE_NEW Entity(i_pModel, i_eid, i_master);
}

namespace efd
{
    template<>
    void ConvertingAssignment(efd::UInt64& o_out, const egf::EntityID& i_in)
    {
        o_out = i_in.GetValue();
    }
    template<>
    void ConvertingAssignment(efd::UInt64& o_out, const efd::Category& i_in)
    {
        o_out = i_in.GetValue();
    }

} // end namespace efd

//------------------------------------------------------------------------------------------------
efd::SyncResult EntityManager::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    pDependencyRegistrar->AddDependency<FlatModelManager>();
    // Because we register an IAssetFactory whenever there in an AssetFactoryManager we must depend
    // on the AssetFactoryManager service if it exists.
    pDependencyRegistrar->AddDependency<AssetFactoryManager>(sdf_Optional);
    pDependencyRegistrar->AddDependency<Scheduler>(sdf_Optional);

    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    if (!m_spMessageService)
    {
        EE_LOG(efd::kEntityManager, efd::ILogger::kERR0,
            ("Init: Failed to find local message service to register callback handlers!"));
        return efd::SyncResult_Failure;
    }

    m_spMessageService->Subscribe(this, kCAT_LocalMessage);
    m_spMessageService->Subscribe(this, m_entityLoadCategory);

    FlatModelManager* pfmm = m_pServiceManager->GetSystemServiceAs<FlatModelManager>();
    if (pfmm)
    {
        pfmm->RegisterEntityFactory(&EntityManager::EntityFactoryMethod, 0);
    }

    ParameterConverterManager::MakeDefaultBidirectionalConverters<egf::EntityID, efd::UInt64>();
    ParameterConverterManager::MakeDefaultBidirectionalConverters<efd::Category, efd::UInt64>();

    // Allow signed to unsigned conversion without error:
    ParameterConverterManager::MakeDefaultBidirectionalConverters<efd::SInt8, efd::UInt8>();
    ParameterConverterManager::MakeDefaultBidirectionalConverters<efd::SInt16, efd::UInt16>();
    ParameterConverterManager::MakeDefaultBidirectionalConverters<efd::SInt32, efd::UInt32>();
    ParameterConverterManager::MakeDefaultBidirectionalConverters<efd::SInt64, efd::UInt64>();

    // Allow integer up-casts to larger sizes (it is common to optimize something like an enum to
    // serialize as a smaller data size and this allows existing script that reads the larger
    // size to continue to function.
    ParameterConverterManager::MakeDefaultConverter<efd::SInt8, efd::SInt16>();
    ParameterConverterManager::MakeDefaultConverter<efd::SInt8, efd::SInt32>();
    ParameterConverterManager::MakeDefaultConverter<efd::SInt8, efd::UInt16>();
    ParameterConverterManager::MakeDefaultConverter<efd::SInt8, efd::UInt32>();
    ParameterConverterManager::MakeDefaultConverter<efd::UInt8, efd::SInt16>();
    ParameterConverterManager::MakeDefaultConverter<efd::UInt8, efd::SInt32>();
    ParameterConverterManager::MakeDefaultConverter<efd::UInt8, efd::UInt16>();
    ParameterConverterManager::MakeDefaultConverter<efd::UInt8, efd::UInt32>();

    ParameterConverterManager::MakeDefaultConverter<efd::SInt16, efd::SInt32>();
    ParameterConverterManager::MakeDefaultConverter<efd::SInt16, efd::UInt32>();
    ParameterConverterManager::MakeDefaultConverter<efd::UInt16, efd::SInt32>();
    ParameterConverterManager::MakeDefaultConverter<efd::UInt16, efd::UInt32>();
    
    // Allow conversion of AssetID to and from utf8string.
    ParameterConverterManager::MakeDefaultBidirectionalConverters<efd::utf8string, efd::AssetID>();

    SetDefaultAutoEnterWorld(true);

    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult EntityManager::OnInit()
{
    // If we have an AssetFactoryManager, we support background Entity loading.
    m_spAFM = m_pServiceManager->GetSystemServiceAs<AssetFactoryManager>();
    if (m_spAFM)
    {
        IScriptFactory* pScriptFactory = 0;

        Scheduler* pSim = m_pServiceManager->GetSystemServiceAs<Scheduler>();
        if (pSim)
        {
            ISchedulerScripting* pEngine = pSim->GetScriptingRuntime(BehaviorType_Lua);
            if (pEngine)
            {
                pScriptFactory = EE_DYNAMIC_CAST(IScriptFactory,
                    m_spAFM->GetAssetFactory(pEngine->GetAssetLoadCategory()));
            }
        }

        FlatModelFactory* pFlatModelFactory = m_pServiceManager->GetSystemServiceAs
            <FlatModelManager>()->GetFlatModelFactory();
        m_pEntityFactory = EE_NEW egf::EntityFactory(pFlatModelFactory);

        m_spAFM->RegisterAssetFactory(m_entityLoadCategory, m_pEntityFactory);
    }

    return efd::AsyncResult_Complete;
}


//------------------------------------------------------------------------------------------------
efd::AsyncResult EntityManager::OnTick()
{
    m_tickCount++;

    UpdateDirtyEntities();
    SendPreloadWarnings();

    return efd::AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult EntityManager::OnShutdown()
{
    while (!m_EntityCacheMap.empty())
    {
        EntityMap::iterator iter = m_EntityCacheMap.begin();
        EntityPtr spEntity = iter->second;
        if (!RemoveEntity(spEntity))
        {
            // Handle failure just in case their are bogus entires in the m_EntityCacheMap
            m_EntityCacheMap.erase(iter);
        }
        EE_ASSERT(m_EntityCacheMap.find(spEntity->GetEntityID()) == m_EntityCacheMap.end());
    }
    EE_ASSERT(m_entitiesRemovingList.empty());

    if (m_spAFM)
    {
        m_spAFM->UnregisterAssetFactory(m_entityLoadCategory);
        m_spAFM = NULL;
    }

    if (m_spMessageService)
    {
        set<Category>::const_iterator i;
        for (i = m_preloadCategories.begin(); i != m_preloadCategories.end(); i++)
        {
            m_spMessageService->Unsubscribe(this, *i);
        }

        m_spMessageService->Unsubscribe(this, kCAT_LocalMessage);
        m_spMessageService->Unsubscribe(this, m_entityLoadCategory);
        m_spMessageService = NULL;
    }

    m_preloadCategories.clear();
    return efd::AsyncResult_Complete;
}


//------------------------------------------------------------------------------------------------
bool EntityManager::AddEntity(Entity* pEntity, ParameterList* pParameterList)
{
    if (pEntity == NULL)
    {
        EE_LOG(efd::kEntityManager, efd::ILogger::kERR2,
            ("EntityManager: Attempt to add Null entity. Attempt ignored."));
        return false;
    }
    if (m_shuttingDown)
    {
        EE_LOG(efd::kEntityManager, efd::ILogger::kERR2,
            ("EntityManager shutdown in progress, not accepting new entities."));
        return false;
    }
    if (pEntity->IsDestroyInProgress())
    {
        EE_FAIL("Destroyed entities cannot be added back to the"
            "EntityManager. Consider using Enter/ExitWorld instead.");
        EE_LOG(efd::kEntityManager, efd::ILogger::kERR0,
            ("Destroyed entity %s cannot be added back to the "
            "EntityManager. Consider using Enter/ExitWorld instead.",
            pEntity->GetEntityID().ToString().c_str()));

        return false;
    }

    // first, validate don't already have this entity as one in local cache
    const EntityID& entityID = pEntity->GetEntityID();
    EntityPtr spFoundEntity;
    if (m_EntityCacheMap.find(entityID, spFoundEntity) == true)
    {
        EE_LOG(efd::kEntityManager, efd::ILogger::kERR2,
            ("EntityManager: Specified Entity (%s) already exists.  Attempt to add ignored.",
            entityID.ToString().c_str()));
        return false;
    }

    // Store entity into local cache map.  We need to do this right away even though the entity
    // isn't fully created yet.  The entity's OnCreate behavior may call built-ins that require
    // being able to lookup the entity by ID.
    m_EntityCacheMap[ entityID ] = pEntity;

    // Entities that are added but not yet fully created are also tracked in the new entity map.
    // We use this to avoid sending out update messages for partially constructed entities.
    m_NewEntityMap[ entityID ] = pEntity;

    EE_LOG(efd::kEntityManager, efd::ILogger::kLVL1,
        ("EntityManager: Successfully added %s (%s) owned=%d",
        entityID.ToString().c_str(),
        pEntity->GetModelName().c_str(),
        (int)pEntity->IsOwned()));

    // If we have a dataID (i.e. if we were created by loading a block file) then store a
    // mapping from our dataID to the created entity.
    const ID128& dataID = pEntity->GetDataFileID();
    if (dataID.IsValid())
    {
        m_entityDataIDMap.insert(DataIdToEntityMapEntry(dataID, pEntity));
    }

#ifndef EE_CONFIG_SHIPPING
    // Report new entity to Sim Debugger
    if (SimDebugger::Instance())
        SimDebugger::Instance()->StartTrackingEntity(pEntity);
#endif

    bool autoEnterWorld = m_defaultAutoEnterWorld;
    if (pParameterList)
    {
        // Check to see if we should automatically add the entity to the world. If the key is not
        // found the 'autoEnterWorld' variable will not be modified.
        if (pr_OK != pParameterList->GetParameter("AutoEnterWorld", autoEnterWorld))
        {
            // If this parameter wasn't provided but other parameters were then we try to add the
            // parameter set to the default value. This allows OnCreate implementations to always
            // know the value of this setting.
            pParameterList->AddParameter("AutoEnterWorld", autoEnterWorld);
        }
    }
    else
    {
        // Use the default list that's already pre-populated
        pParameterList = m_spDefaultOnCreateParams;
    }

    // The actual entry into the world won't occur until OnCreate finishes, this just effectively
    // queues the event to happen later.
    if (autoEnterWorld)
    {
        pEntity->EnterWorld();
    }

    pEntity->SetEntityManager(this);
    pEntity->OnAdded(pParameterList);

    // Record the total extant entities
    EE_LOG_METRIC(kEntity, "TOTAL", (efd::UInt32)m_EntityCacheMap.size());

    if (pEntity->IsOwned())
    {
        // tick off an owned entity created by model ID
        EE_LOG_METRIC_COUNT_FMT(kEntity, ("CREATE.OWNED.%s",
            pEntity->GetModelName().c_str()));
    }
    else
    {
        // tick off another replica, plus by model ID
        EE_LOG_METRIC_COUNT_FMT(kEntity, ("CREATE.REPLICA.%s",
            pEntity->GetModelName().c_str()));
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool EntityManager::DestroyEntity(const EntityID& id)
{
    return DestroyEntity(LookupEntity(id));
}

//------------------------------------------------------------------------------------------------
bool EntityManager::DestroyEntity(Entity* pEntity)
{
    if (pEntity == NULL)
    {
        EE_LOG(efd::kEntityManager, efd::ILogger::kERR2,
            ("EntityManager: Attempt to remove Null entity.  Attempt ignored."));
        return false;
    }

    EntityID entityID = pEntity->GetEntityID();

    EntityMap::iterator iter = m_EntityCacheMap.find(entityID);

    if (iter == m_EntityCacheMap.end())
    {
        EE_LOG(efd::kEntityManager, efd::ILogger::kERR2,
            ("EntityManager: Specified Entity (%s) not found.  Attempt to remove ignored.",
            entityID.ToString().c_str()));
        return false;
    }
    // Request of the Entity that it destroy itself. RemoveEntity will be called after appropriate
    // lifecycles complete.
    pEntity->Destroy();
    return true;
}

//------------------------------------------------------------------------------------------------
bool EntityManager::RemoveEntity(Entity* pEntity)
{
    if (pEntity == NULL)
    {
        EE_LOG(efd::kEntityManager, efd::ILogger::kERR2,
            ("EntityManager: Attempt to remove Null entity.  Attempt ignored."));
         return false;
    }

    EntityID entityID = pEntity->GetEntityID();

    EntityMap::iterator iter = m_EntityCacheMap.find(entityID);

    if (iter == m_EntityCacheMap.end())
    {
        EE_LOG(efd::kEntityManager, efd::ILogger::kERR2,
            ("EntityManager: Specified Entity (%s) not found.  Attempt to remove ignored.",
            entityID.ToString().c_str()));
        return false;
    }

    if (m_entitiesRemovingList.find(pEntity) != m_entitiesRemovingList.end())
    {
        EE_LOG(efd::kEntityManager, efd::ILogger::kERR2,
            ("EntityManager: Attempted to remove entity (%s) twice, second attempt ignored.",
            entityID.ToString().c_str()));
        return false;
    }

    // remember which entities we are in the process of removing
    m_entitiesRemovingList.push_front(pEntity);

    //use the local message service to send the LocalEntityRemoved message
    SendEntityRemovalNotification(pEntity);

    pEntity->OnRemoved();
    pEntity->SetEntityManager(NULL);

#ifndef EE_CONFIG_SHIPPING
    // Report entity removal to Sim Debugger
    if (SimDebugger::Instance())
        SimDebugger::Instance()->StopTrackingEntity(pEntity);
#endif

    const ID128& dataID = pEntity->GetDataFileID();
    if (dataID.IsValid())
    {
        DataIdToEntityMap::iterator itCurr = m_entityDataIDMap.lower_bound(dataID);
        DataIdToEntityMap::iterator itLast = m_entityDataIDMap.upper_bound(dataID);
        while (itCurr != itLast)
        {
            if (itCurr->second == pEntity)
            {
                m_entityDataIDMap.erase(itCurr);
                break;
            }
            ++itCurr;
        }
    }

    // Just in case I'm deleted shortly after creation and still have pending asset lookups:
    m_preloadingEntities.erase(pEntity);
    m_preloadWarnings.erase(pEntity);

    // If I'm dirty, remove myself so I don't send any updates after deletion
    ClearDirty(pEntity);

    // remove from local cache.
    // NOTE: This will delete the memory pointed to by pEntity.
    m_EntityCacheMap.erase(iter);
    // Just in case, make sure we aren't in the new map still.  We might get removed before we
    // finish getting added, for example an entity might destroy itself during OnCreate.
    m_NewEntityMap.erase(entityID);
    // We are done removing this entity, no need to remember it
    m_entitiesRemovingList.remove(pEntity);

    EE_LOG(efd::kEntityManager, efd::ILogger::kLVL1,
        ("EntityManager: Successfully removed %s",
        entityID.ToString().c_str()));

    // Record the total extant entities
    EE_LOG_METRIC(kEntity, "TOTAL", (efd::UInt32)m_EntityCacheMap.size());

    return true;
}


//------------------------------------------------------------------------------------------------
Entity* EntityManager::LookupEntity(const EntityID &id) const
{
    EntityPtr spFoundEntity = NULL;
    m_EntityCacheMap.find(id, spFoundEntity);
    return spFoundEntity;
}

//------------------------------------------------------------------------------------------------
Entity* EntityManager::LookupEntityByDataFileID(const efd::ID128 &id) const
{
    EntityPtr spFoundEntity = NULL;
    DataIdToEntityMap::const_iterator it = m_entityDataIDMap.find(id);
    if (it != m_entityDataIDMap.end())
    {
        spFoundEntity = it->second;
    }
    return spFoundEntity;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 EntityManager::LookupEntityByDataFileID(
    const efd::ID128 &id,
    efd::set<Entity*>& o_results) const
{
    efd::UInt32 result = 0;
    DataIdToEntityMap::const_iterator first = m_entityDataIDMap.lower_bound(id);
    DataIdToEntityMap::const_iterator last = m_entityDataIDMap.upper_bound(id);
    while (first != last)
    {
        o_results.insert(first->second);
        ++result;
        ++first;
    }
    return result;
}

//------------------------------------------------------------------------------------------------
EntityManager::EntityMap::const_iterator EntityManager::GetFirstEntityPos() const
{
    return m_EntityCacheMap.begin();
}

//------------------------------------------------------------------------------------------------
bool EntityManager::GetNextEntity(
    EntityManager::EntityMap::const_iterator& io_pos,
    Entity*& o_pEntity) const
{
    if (m_EntityCacheMap.end() != io_pos)
    {
        o_pEntity = io_pos->second;
        ++io_pos;
        return true;
    }

    o_pEntity = NULL;
    return false;
}


//------------------------------------------------------------------------------------------------
EntityManager::FilteredIterator EntityManager::GetFilteredIterator(
    EntityManager::FilterFunction i_pfn,
    void* i_pParam) const
{
    return FilteredIterator(m_EntityCacheMap.begin(), i_pfn, i_pParam);
}


//------------------------------------------------------------------------------------------------
bool EntityManager::GetNextEntity(
    EntityManager::FilteredIterator& io_pos,
    Entity* &o_pEntity) const
{
    while (io_pos.m_iter != m_EntityCacheMap.end())
    {
        Entity* pEntity = io_pos.m_iter->second;
        ++io_pos.m_iter;
        if (!io_pos.m_pfnFilter || io_pos.m_pfnFilter(pEntity, io_pos.m_pParam))
        {
            o_pEntity = pEntity;
            return true;
        }
    }
    o_pEntity = NULL;
    return false;
}


//------------------------------------------------------------------------------------------------
void EntityManager::AddDirty(Entity* pEntity)
{
    const EntityID& eid = pEntity->GetEntityID();

    // store entity into dirty map (will just overwrite same if already there)...
    if (LookupEntity(eid) == NULL)
    {
        // if here, attempting to set "dirty" an entity we don't own!
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Error: Attempt to set 'dirty' on an unmanaged entity (%s)",
            eid.ToString().c_str()));
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
            ("Flagging %s entity %s dirty",
            pEntity->GetModel()->GetName().c_str(),
            eid.ToString().c_str()));
        m_entityDirtyMap[ eid ] = pEntity;
    }
}


//------------------------------------------------------------------------------------------------
void EntityManager::ClearDirty(Entity* pEntity)
{
    EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
        ("Clearing %s entity %s dirty flag",
        pEntity->GetModel()->GetName().c_str(),
        pEntity->GetEntityID().ToString().c_str()));
    m_entityDirtyMap.erase(pEntity->GetEntityID());
}


//------------------------------------------------------------------------------------------------
void EntityManager::UpdateDirtyEntities()
{
    for (EntityManager::EntityMap::iterator edPos = m_entityDirtyMap.begin();
          edPos != m_entityDirtyMap.end();
          ++edPos)
    {
        // if here, have another entity to send an update message for...
        Entity* pEntity = edPos->second;
        if (pEntity)
        {
            // Entities that are pending final creation do not send out update notifications.
            // When their OnCreate behavior completes they will be removed from the new entity
            // map, all their dirty state will be cleared, and an initial creation notification
            // will be sent out.  Only after that completes do we start sending updates.
            // Entities that have not yet entered the world also do not clear dirty as
            // dirty will be cleared after they enter the world.
            if (pEntity->IsInWorld())
            {
                SendEntityUpdateNotification(pEntity);

                // Once the update is sent we clear the dirtiness. Note that this only clears
                // local dirtiness and is unrelated to replication dirty state.
                pEntity->ClearDirty();

                // if here, a valid pointer, add the metric
                EE_LOG_METRIC_COUNT_FMT(kEntity, ("DIRTY.MODEL.%s",
                    pEntity->GetModelName().c_str()));
            }
        }
    }

    m_entityDirtyMap.clear();    // insure dirty list empty at end of tick
}

//------------------------------------------------------------------------------------------------
void EntityManager::OnEntityBeginLifecycle(Entity* pEntity, efd::UInt32 lifecycle)
{
    EE_UNUSED_ARG(pEntity);
    EE_UNUSED_ARG(lifecycle);
}

//------------------------------------------------------------------------------------------------
void EntityManager::OnEntityEndLifecycle(Entity* pEntity, efd::UInt32 lifecycle)
{
    switch (lifecycle)
    {
    case Entity::lifecycle_OnCreate:
        // If I was instructed to pre-load assets then I need to resolve which assets need to
        // be loaded and kick off those loads now.  Otherwise (normal case) I'm finished with
        // the setup of this entity and should send out the initial discovery.
        if (IsEntityPreloadRequested(pEntity))
        {
            PreloadEntityAssets(pEntity);
        }
        else
        {
            pEntity->OnAssetsLoaded();
        }
        break;

    case Entity::lifecycle_OnAssetsLoaded:
        // If I'm still a 'new entity' when this behavior finishes then I need to finish setup.
        if (m_NewEntityMap.count(pEntity->GetEntityID()))
        {
            FinishEntitySetup(pEntity);
        }
        break;

    case Entity::lifecycle_OnEnterWorld:
        {
            // Entities are never considered "dirty" until after they enter the world.
            // So clear out the state from any properties that might have been changed during the
            // OnCreate behavior execution.
            pEntity->ClearDirty();
            ClearDirty(pEntity);
            SendEntityEnterWorldNotification(pEntity);
        }
        break;

    case Entity::lifecycle_OnExitWorld:
        SendEntityExitWorldNotification(pEntity);
        break;

    case Entity::lifecycle_OnDestroy:
        RemoveEntity(pEntity);
        break;
    }
}

//------------------------------------------------------------------------------------------------
void EntityManager::FinishEntitySetup(Entity* pEntity)
{
    // Now that setup has finished we are no longer considered a "new entity" so remove
    // ourselves from the new entities map.  This means future property changes for this
    // entity will result in update notifications.
    m_NewEntityMap.erase(pEntity->GetEntityID());

    // Get any pending data and send the EntityFactoryResponse message to the category.
    EntityIDPendingDataMap::iterator it = m_entityIdToPendingDataMap.find(pEntity->GetEntityID());
    if (it != m_entityIdToPendingDataMap.end())
    {
        if (it->second.m_spFactoryResponse)
        {
            // If the response does not have an entity set, be sure to set it before sending.
            if (!it->second.m_spFactoryResponse->GetEntity())
            {
                it->second.m_spFactoryResponse->SetEntity(pEntity);
            }

            // Forward the EntityFactoryResponse message to the provided category.
            if (m_spMessageService)
            {
                m_spMessageService->SendImmediate(
                    it->second.m_spFactoryResponse,
                    it->second.m_category);
            }
        }

        // This entity is complete, remove it from our pending map.
        m_entityIdToPendingDataMap.erase(it);
    }

    // Now we let the world know about this new entity.  Various system services that need
    // to operate on this entity will typically use this notification to trigger the start
    // of their work.
    SendEntityCreationNotification(pEntity);

}

//------------------------------------------------------------------------------------------------
void EntityManager::SendEntityCreationNotification(Entity* pEntity)
{
    if (m_spMessageService)
    {
        m_spOwnedEntityAddedMsg->SetEntity(pEntity);
        m_spMessageService->SendImmediate(m_spOwnedEntityAddedMsg);
        m_spOwnedEntityAddedMsg->SetEntity(NULL);
    }
}

//------------------------------------------------------------------------------------------------
void EntityManager::SendEntityEnterWorldNotification(Entity* pEntity)
{
    if (m_spMessageService)
    {
        m_spOwnedEntityEnterWorldMsg->SetEntity(pEntity);
        m_spMessageService->SendImmediate(m_spOwnedEntityEnterWorldMsg);
        m_spOwnedEntityEnterWorldMsg->SetEntity(NULL);
    }
}

//------------------------------------------------------------------------------------------------
void EntityManager::SendEntityUpdateNotification(Entity* pEntity)
{
    if (m_spMessageService)
    {
        m_spOwnedEntityUpdatedMsg->SetEntity(pEntity);
        m_spMessageService->SendImmediate(m_spOwnedEntityUpdatedMsg);
        m_spOwnedEntityUpdatedMsg->SetEntity(NULL);
    }
}

//------------------------------------------------------------------------------------------------
void EntityManager::SendEntityExitWorldNotification(Entity* pEntity)
{
    if (m_spMessageService)
    {
        m_spOwnedEntityExitWorldMsg->SetEntity(pEntity);
        m_spMessageService->SendImmediate(m_spOwnedEntityExitWorldMsg);
        m_spOwnedEntityExitWorldMsg->SetEntity(NULL);
    }
}

//------------------------------------------------------------------------------------------------
void EntityManager::SendEntityRemovalNotification(Entity* pEntity)
{
    if (m_spMessageService)
    {
        m_spOwnedEntityRemovedMsg->SetEntity(pEntity);
        m_spMessageService->SendImmediate(m_spOwnedEntityRemovedMsg);
        m_spOwnedEntityRemovedMsg->SetEntity(NULL);
    }
}

//------------------------------------------------------------------------------------------------
egf::EntityID EntityManager::CreateEntity(
    const efd::utf8string& modelName,
    const efd::Category& callback,
    efd::ParameterList* pCreationArgs,
    efd::ParameterList* pInitialProps,
    egf::EntityID entityId,
    bool isMaster)
{
    // If you get an assert here you've specified kCAT_LocalMessage as the callback category.
    // Entity creation callback messages cannot be sent on kCAT_LocalMessage.
    if (!EE_VERIFY(callback != kCAT_LocalMessage))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR3,
            ("Warning: Entity creation callback messages should not be set on kCAT_LocalMessage. "
             "CreateEntity for model %s ignored.", modelName.c_str()));
        return kENTITY_INVALID;
    }

    FlatModelManager* pFMM = m_pServiceManager->GetSystemServiceAs<FlatModelManager>();
    EE_ASSERT(pFMM);

    if (entityId == kENTITY_INVALID)
    {
        entityId = EntityIDFactory::GetNextID();
    }

    // If we don't have an AssetFactoryManager or the underlying flat model for this entity
    // is already loaded, factory the entity directly using the FlatModelManager.
    if (!m_spAFM || pFMM->FindModel(modelName))
    {
        static bool logOnce = false;
        if (!m_spAFM && !logOnce)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR3,
                ("EntityManager::CreateEntity - Background loading disabled. You must initialize "
                "an instance of the AssetFactoryManager and register it with the ServiceManager."));
            logOnce = true;
        }

        // The caller still expects this to be an asynchronous call and to receive an
        // EntityFactoryResponse. For example, if this call is made in response to a
        // BehaviorAPI.CreateWithCallback function call, the caller expects a behavior
        // callback to be invoked once the entity is created. We add an EntityFactoryResponse
        // to the pending map; this ensures a factory response is sent to the provided category.
        PendingEntityCreationData data;
        data.m_category = callback;
        data.m_pCreationArgs = pCreationArgs;
        data.m_pInitialPropertyValues = pInitialProps;
        data.m_spFactoryResponse = EE_NEW EntityFactoryResponse(
            utf8string(Formatted, "urn:emergent-flat-model:%s", modelName.c_str()),
            callback,
            AssetLoadResponse::ALR_Success,
            entityId);

        m_entityIdToPendingDataMap[entityId] = data;

        EntityPtr rval = pFMM->FactoryEntity(modelName, entityId, isMaster);
        EE_ASSERT(rval);
        if (pInitialProps)
        {
            rval->ApplyProperties(pInitialProps);
        }
        AddEntity(rval, pCreationArgs);

        return entityId;
    }

    // load the entity in the background if the flat model isn't found.
    PendingEntityCreationData data;
    data.m_category = callback;
    data.m_pCreationArgs = pCreationArgs;
    data.m_pInitialPropertyValues = pInitialProps;
    data.m_spFactoryResponse = 0;

    m_entityIdToPendingDataMap[entityId] = data;

    EntityFactoryRequest* pLoadRequest = EE_NEW EntityFactoryRequest(
        utf8string(Formatted, "urn:emergent-flat-model:%s", modelName.c_str()),
        m_entityLoadCategory,
        entityId,
        "",
        isMaster);

    m_spMessageService->SendLocal(pLoadRequest, AssetFactoryManager::MessageCategory());

    return entityId;
}

//------------------------------------------------------------------------------------------------
void EntityManager::HandleEntityFactoryResponse(
    const egf::EntityFactoryResponse* pMessage,
    efd::Category targetChannel)
{
    if (targetChannel != m_entityLoadCategory)
    {
        // We only care about messages sent on the entity load category.
        return;
    }

    EntityID entityID = pMessage->GetEntityID();
    EntityPtr spEntity = pMessage->GetEntity();

    EntityIDPendingDataMap::iterator it = m_entityIdToPendingDataMap.find(entityID);

    // Make sure we were expecting this message
    EE_ASSERT(it != m_entityIdToPendingDataMap.end());
    if (it == m_entityIdToPendingDataMap.end())
    {
        // Received a message we were not expecting.
        return;
    }

    PendingEntityCreationData& pendingData = it->second;

    // Make sure the entity is valid; if not, exit early
    if (!spEntity)
    {
        m_spMessageService->SendLocal(pMessage, pendingData.m_category);
        m_entityIdToPendingDataMap.erase(entityID);
        return;
    }

    pendingData.m_spFactoryResponse = pMessage;

    // Add the flat model dependencies to the FlatModelManager
    FlatModelManager* pFlatManager = m_pServiceManager->GetSystemServiceAs<FlatModelManager>();
    pFlatManager->HandleAssetLoadMsg(pMessage, kCAT_INVALID);

    if (pendingData.m_pInitialPropertyValues)
    {
         spEntity->ApplyProperties(pendingData.m_pInitialPropertyValues);
    }
    // Add the new entity and call the relevant lifecycle behaviors
    // it is valid for the Entity to not be able to be added in some cases.
    AddEntity(spEntity, pendingData.m_pCreationArgs);
}

//------------------------------------------------------------------------------------------------
efd::Category EntityManager::GetEntityLoadCategory() const
{
    return m_entityLoadCategory;
}

//------------------------------------------------------------------------------------------------
EntityFactory* EntityManager::GetEntityFactory() const
{
    return m_pEntityFactory;
}

//------------------------------------------------------------------------------------------------
void EntityManager::RegisterPreloadService(Category completionCallback)
{
    EE_ASSERT(m_preloadCategories.find(completionCallback) == m_preloadCategories.end());

    // Try to get the message service if we don't already have it.
    if (!m_spMessageService)
        m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();

    if (m_spMessageService)
    {
        m_preloadCategories.insert(completionCallback);

        m_spMessageService->Subscribe(this, completionCallback);
    }
}

//------------------------------------------------------------------------------------------------
void EntityManager::UnregisterPreloadService(Category completionCallback)
{
    efd::set<efd::Category>::iterator iter = m_preloadCategories.find(completionCallback);
    if (iter != m_preloadCategories.end())
    {
        m_preloadCategories.erase(iter);

        if (m_spMessageService)
        {
            m_spMessageService->Unsubscribe(this, completionCallback);
        }
    }
}

//------------------------------------------------------------------------------------------------
void EntityManager::PreloadEntityAssets(egf::Entity *pEntity)
{
    if (m_preloadCategories.size() == 0)
    {
        // Skip preloading since no one is registered
        pEntity->OnAssetsLoaded();
        return;
    }

    EE_ASSERT(m_preloadingEntities.find(pEntity) == m_preloadingEntities.end());

    set<Category>::const_iterator i;
    for (i = m_preloadCategories.begin(); i != m_preloadCategories.end(); i++)
    {
        m_preloadingEntities.insert(make_pair(pEntity, *i));
    }

    EntityPreloadRequestPtr spMessage = EE_NEW EntityPreloadRequest();
    spMessage->m_pEntity = pEntity;
    m_spMessageService->SendLocal(spMessage, GetEntityPreloadCategory());

    PreloadWarningData warningData;
    warningData.m_tickStarted = m_tickCount;
    warningData.m_warningLogged = false;
    m_preloadWarnings[pEntity] = warningData;
}

//------------------------------------------------------------------------------------------------
Category EntityManager::GetEntityPreloadCategory() const
{
    return Category(UniversalID::ECU_EventChannel, kNetID_Any, kBASEID_EntityPreload);
}

//------------------------------------------------------------------------------------------------
void EntityManager::HandleEntityPreloadResponse(const EntityPreloadResponse* pMessage,
                                                Category targetChannel)
{
    typedef pair<PreloadIterator, PreloadIterator> PreloadRange;

    Entity* pEntity = pMessage->GetEntity();

    PreloadRange entityRange = m_preloadingEntities.equal_range(pMessage->GetEntity());

    // Remove the pair<entity, targetChannel> from m_preloadingEntities
    bool entityAndChannelFound = false;
    for (PreloadIterator i = entityRange.first; i != entityRange.second; i++)
    {
        if (i->second == targetChannel)
        {
            entityAndChannelFound = true;
            m_preloadingEntities.erase(i);
            break;
        }
    }

    // If the entity is not even waiting for preloading (empty entityRange) or this message came
    // from an unexpected source (targetChannel not found in range) then we don't want to call
    // OnAssetsLoaded at all. This can happen if the entity is removed before preloading completes
    // (an expected case) or if extra messages are accidentally sent (not really expected).
    if (entityAndChannelFound)
    {
        // Check whether all services have completed preloading
        PreloadWarningData warningData;
        EE_VERIFY(m_preloadWarnings.find(pEntity, warningData));

        // If the range is empty after removing this response then we are done
        entityRange = m_preloadingEntities.equal_range(pEntity);
        if (entityRange.first == entityRange.second)
        {
            pEntity->OnAssetsLoaded();

            // Log a completion notice if we warned about the entity earlier
            if (warningData.m_warningLogged)
            {
                EE_LOG(efd::kEntityManager, efd::ILogger::kERR3,
                    ("%s (%s) eventually finished preloading after %lu ticks",
                    pEntity->GetEntityID().ToString().c_str(),
                    pEntity->GetModelName().c_str(),
                    m_tickCount - warningData.m_tickStarted));
            }

            m_preloadWarnings.erase(pEntity);
        }
        else
        {
            // Prepare to send another preloading warning if necessary
            warningData.m_warningLogged = false;
        }
    }
}

//------------------------------------------------------------------------------------------------
Bool EntityManager::IsEntityPreloadRequested(const Entity* pEntity) const
{
    Bool preloadRequested = false;

    // Check for Preloadable explicitly to reduce log output
    if (!pEntity->GetModel()->ContainsModel("Preloadable"))
        return false;

    if (pEntity->GetPropertyValue(kPropertyID_StandardModelLibrary_PreloadAssets, preloadRequested)
        == egf::PropertyResult_OK)
    {
        return preloadRequested;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------------------------
void EntityManager::SendPreloadWarnings()
{
    map<Entity*, PreloadWarningData>::iterator warningIter;

    for (warningIter = m_preloadWarnings.begin();
        warningIter != m_preloadWarnings.end();
        warningIter++)
    {
        Entity* pEntity = warningIter->first;
        PreloadWarningData& warningData = warningIter->second;

        if (m_tickCount > (warningData.m_tickStarted + m_preloadWarningThreshold))
        {
            if (warningData.m_warningLogged)
                continue;

            // Figure out how many services are still interfering with the preload
            pair<PreloadIterator, PreloadIterator> range =
                m_preloadingEntities.equal_range(pEntity);
            EE_ASSERT(range.first != m_preloadingEntities.end());
            UInt32 serviceCount = 0;
            for (PreloadIterator iter = range.first; iter != range.second; iter++)
                serviceCount++;

            EE_LOG(efd::kEntityManager, efd::ILogger::kERR3,
                ("%s (%s) has been preloading for %lu ticks--%u service%s still remain",
                pEntity->GetEntityID().ToString().c_str(),
                pEntity->GetModelName().c_str(),
                m_tickCount - warningData.m_tickStarted,
                serviceCount,
                (serviceCount != 1) ? "s" : ""));

            warningData.m_warningLogged = true;
        }
    }
}

//------------------------------------------------------------------------------------------------
const char* EntityManager::GetDisplayName() const
{
    return "EntityManager";
}

//------------------------------------------------------------------------------------------------
void EntityManager::SetDefaultAutoEnterWorld(bool newValue)
{
    if (!m_spDefaultOnCreateParams || newValue != m_defaultAutoEnterWorld)
    {
        m_defaultAutoEnterWorld = newValue;
        m_spDefaultOnCreateParams = EE_NEW ParameterList();
        m_spDefaultOnCreateParams->AddParameter("AutoEnterWorld", m_defaultAutoEnterWorld);
    }
}

//------------------------------------------------------------------------------------------------
