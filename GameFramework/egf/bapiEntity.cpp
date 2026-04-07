// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 2006-2009 Todd Berkebile.
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include "egfPCH.h"

#include <egf/BehaviorAPI.h>
#include <egf/bapiEntity.h>
#include <egf/bapiInternal.h>
#include <egf/EntityManager.h>
#include <egf/FlatModel.h>
#include <egf/FlatModelManager.h>
#include <efd/ServiceManager.h>
#include <egf/Scheduler.h>
#include <egf/Entity.h>
#include <egf/ScriptContext.h>
#include <egf/egfLogIDs.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
// A private helper function used by several methods to filter the result lists.  Any strings
// in the list that do not contain the filter string are removed.
// DT21978 Rather than a simple substring we could use real Regex matching here
void RemoveNonMatchingStrings(const efd::utf8string& i_filter, efd::list<efd::utf8string>& io_list)
{
    efd::list<efd::utf8string>::iterator iter = io_list.begin();
    while (iter != io_list.end())
    {
        const utf8string& item = *iter;
        if (efd::utf8string::npos == item.find(i_filter))
        {
            // find returned -1 so the substring was not found, remove this item:
            efd::list<efd::utf8string>::iterator iterToRemove = iter;
            ++iter;
            io_list.erase(iterToRemove);
        }
        else
        {
            ++iter;
        }
    }
}

//------------------------------------------------------------------------------------------------
egf::EntityID BehaviorAPI::GetExecutingEntityId()
{
    EE_ASSERT(g_bapiContext.GetScriptEntity());
    return g_bapiContext.GetScriptEntity()->GetEntityID();
}

//------------------------------------------------------------------------------------------------
egf::EventID BehaviorAPI::GetCurrentEventID()
{
    EE_ASSERT(g_bapiContext.GetCurrentBehavior());
    return g_bapiContext.GetCurrentBehavior()->GetEventID();
}

//------------------------------------------------------------------------------------------------
egf::EntityID BehaviorAPI::GetReturnEntityId()
{
    EE_ASSERT(g_bapiContext.GetCurrentBehavior());
    return g_bapiContext.GetCurrentBehavior()->GetReturnEntityID();
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::IsKindOf(
    egf::EntityID entity,
    const efd::utf8string& modelName)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!modelName.empty());

    EntityManager* pScheduler = g_bapiContext.GetSystemServiceAs<EntityManager>();
    Entity* pEntity = pScheduler->LookupEntity(entity);
    if (!pEntity)
    {
        return ec_EntityNotFound;
    }

    if (pEntity->GetModel()->ContainsModel(modelName))
    {
        return ec_Yes;
    }
    return ec_No;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::HasMixin(
    egf::EntityID entity,
    const efd::utf8string& mixinModelName)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!mixinModelName.empty());

    EntityManager* pScheduler = g_bapiContext.GetSystemServiceAs<EntityManager>();
    Entity* pEntity = pScheduler->LookupEntity(entity);
    if (!pEntity)
    {
        return ec_EntityNotFound;
    }

    // If its "me" I am an IsKindOf "me" but I do not "mixin" me:
    if (mixinModelName != pEntity->GetModel()->GetName())
    {
        if (pEntity->GetModel()->ContainsModel(mixinModelName))
        {
            return ec_Yes;
        }
    }
    return ec_No;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::HasBehavior(
    egf::EntityID entity,
    const efd::utf8string& behaviorName)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!behaviorName.empty());

    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(entity);
    if (!pEntity)
    {
        return ec_EntityNotFound;
    }

    if (NULL != pEntity->GetModel()->GetBehaviorDescriptor(behaviorName))
    {
        return ec_Yes;
    }
    return ec_No;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::HasProperty(
    egf::EntityID entity,
    const efd::utf8string& propertyName)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!propertyName.empty());

    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(entity);
    if (!pEntity)
    {
        return ec_EntityNotFound;
    }

    if (PropertyResult_OK == pEntity->HasPropertyValue(propertyName))
    {
        return ec_Yes;
    }
    return ec_No;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::HasPropertyKey(
    egf::EntityID entity,
    const efd::utf8string& propertyName,
    const efd::utf8string& propertyKey)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!propertyName.empty());

    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(entity);
    if (!pEntity)
    {
        return ec_EntityNotFound;
    }

    if (PropertyResult_OK == pEntity->HasPropertyValue(propertyName, propertyKey))
        return ec_Yes;
    return ec_No;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::FindMixins(
    egf::EntityID entity,
    efd::list<efd::utf8string>* OutValue)
{
    EE_ASSERT(entity.IsValid());

    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(entity);
    if (!pEntity)
    {
        return ec_EntityNotFound;
    }

    pEntity->GetModel()->GetMixinNames(*OutValue);
    return ec_Yes;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::FindMatchingMixins(
    egf::EntityID entity,
    const efd::utf8string &filter,
    efd::list<efd::utf8string>* OutValue)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!filter.empty());

    EntityCheck result = FindMixins(entity, OutValue);
    if (ec_Yes == result)
    {
        RemoveNonMatchingStrings(filter, *OutValue);
    }
    return result;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::FindBehaviors(
    egf::EntityID entity,
    efd::list<efd::utf8string>* OutValue)
{
    EE_ASSERT(entity.IsValid());

    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(entity);
    if (!pEntity)
    {
        return ec_EntityNotFound;
    }

    pEntity->GetModel()->GetBehaviorNames(*OutValue);
    return ec_Yes;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::FindMatchingBehaviors(
    egf::EntityID entity,
    const efd::utf8string& filter,
    efd::list<efd::utf8string>* OutValue)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!filter.empty());

    EntityCheck result = FindBehaviors(entity, OutValue);
    if (ec_Yes == result)
    {
        RemoveNonMatchingStrings(filter, *OutValue);
    }
    return result;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::FindProperties(
    egf::EntityID entity,
    efd::list<efd::utf8string>* OutValue)
{
    EE_ASSERT(entity.IsValid());

    EntityManager* pem = g_bapiContext.GetSystemServiceAs<EntityManager>();
    Entity* pEntity = pem->LookupEntity(entity);
    if (!pEntity)
    {
        return ec_EntityNotFound;
    }

    pEntity->GetModel()->GetPropertyNames(*OutValue);
    return ec_Yes;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::FindMatchingProperties(
    egf::EntityID entity,
    const efd::utf8string& filter,
    efd::list<efd::utf8string>* OutValue)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!filter.empty());

    EntityCheck result = FindProperties(entity, OutValue);
    if (ec_Yes == result)
    {
        RemoveNonMatchingStrings(filter, *OutValue);
    }
    return result;
}

//------------------------------------------------------------------------------------------------
BehaviorAPI::EntityCheck BehaviorAPI::FindPropertyKeys(
    egf::EntityID entity,
    const efd::utf8string& propertyName,
    efd::list<efd::utf8string>* OutValue)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!propertyName.empty());

    EntityManager* pem = g_bapiContext.GetSystemServiceAs<EntityManager>();
    Entity* pEntity = pem->LookupEntity(entity);
    if (!pEntity)
    {
        return ec_EntityNotFound;
    }

    if (PropertyResult_OK == pEntity->GetPropertyKeys(propertyName, *OutValue))
    {
        return ec_Yes;
    }
    return ec_No;
}

//------------------------------------------------------------------------------------------------
efd::utf8string BehaviorAPI::GetNextPropertyKey(
    egf::EntityID entity,
    const efd::utf8string& propertyName,
    const efd::utf8string& previousKey)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!propertyName.empty());

    efd::utf8string result;

    EntityManager* pem = g_bapiContext.GetSystemServiceAs<EntityManager>();
    Entity* pEntity = pem->LookupEntity(entity);
    if (pEntity)
    {
        pEntity->GetNextPropertyKey(propertyName, previousKey, result);
    }

    return result;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& BehaviorAPI::GetModelName(egf::EntityID entity)
{
    EE_ASSERT(entity.IsValid());

    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(entity);
    if (pEntity)
    {
        return pEntity->GetModelName();
    }
    return utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& BehaviorAPI::GetBehaviorSource(
    egf::EntityID entity,
    const efd::utf8string& behaviorName)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!behaviorName.empty());

    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(entity);
    if (pEntity)
    {
        const BehaviorDescriptor* pDesc = pEntity->GetModel()->GetBehaviorDescriptor(behaviorName);
        if (NULL != pDesc)
        {
            return pDesc->GetModelName();
        }
    }
    return utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& BehaviorAPI::GetPropertySource(
    egf::EntityID entity,
    const efd::utf8string& propertyName)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!propertyName.empty());

    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(entity);
    if (pEntity)
    {
        const PropertyDescriptor* pDesc = pEntity->GetModel()->GetPropertyDescriptor(propertyName);
        if (NULL != pDesc)
        {
            return pDesc->GetSource();
        }
    }
    return utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::ResetProperty(egf::EntityID entity, const efd::utf8string& propertyName)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!propertyName.empty());

    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(entity);
    if (pEntity)
    {
        if (pEntity->ResetProperty(propertyName) == PropertyResult_OK)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::RemovePropertyValue(
    egf::EntityID entity,
    const efd::utf8string& propertyName,
    const efd::utf8string& key)
{
    EE_ASSERT(entity.IsValid());
    EE_ASSERT(!propertyName.empty());
    EE_ASSERT(!key.empty());

    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(entity);
    if (pEntity)
    {
        if (pEntity->RemovePropertyValue(propertyName, key) == PropertyResult_OK)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
// Entity discovery

//------------------------------------------------------------------------------------------------
bool bapiInternal::FF_ExcludeSingleID(const Entity* i_pEntity, void* pParam)
{
    egf::EntityID* pid = static_cast<egf::EntityID*>(pParam);
    return *pid != i_pEntity->GetEntityID();
}

//------------------------------------------------------------------------------------------------
efd::UInt32 bapiInternal::FindAllEntities(
    EntityManager::FilterFunction i_pfnFilter,
    void* pParam,
    efd::list<EntityID>& o_results)
{
    EE_ASSERT(i_pfnFilter != NULL);
    EE_ASSERT(pParam != NULL);

    efd::UInt32 count = 0;
    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs<EntityManager>();
    EntityManager::FilteredIterator iter = pEntityManager->GetFilteredIterator(i_pfnFilter, pParam);
    Entity* pEntity;
    while (pEntityManager->GetNextEntity(iter, pEntity))
    {
        count++;
        o_results.push_back(pEntity->GetEntityID());
    }

    return count;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 BehaviorAPI::FindAllEntities(efd::list<EntityID>* OutValue)
{
    egf::EntityID id = GetExecutingEntityId();
    return bapiInternal::FindAllEntities(bapiInternal::FF_ExcludeSingleID, &id, *OutValue);
}

//------------------------------------------------------------------------------------------------
bool FF_IncludeModel(const Entity* pEntity, void* pParam)
{
    const efd::utf8string* pModelName = static_cast<const efd::utf8string*>(pParam);
    return (pEntity->GetModel()->GetName() == *pModelName);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 BehaviorAPI::FindEntitiesByModel(
    const efd::utf8string& modelName,
    efd::list<egf::EntityID>* OutValue)
{
    EE_ASSERT(!modelName.empty());

    OutValue->clear();
    return bapiInternal::FindAllEntities(FF_IncludeModel, (void*)&modelName, *OutValue);
}

//------------------------------------------------------------------------------------------------
bool FF_IncludeBaseModel(const Entity* pEntity, void* pParam)
{
    const efd::utf8string* pModelName = static_cast<const efd::utf8string*>(pParam);
    return (pEntity->GetModel()->ContainsModel(*pModelName));
}

//------------------------------------------------------------------------------------------------
efd::UInt32 BehaviorAPI::FindEntitiesByBaseModel(
    const efd::utf8string& modelName,
    efd::list<egf::EntityID>* OutValue)
{
    EE_ASSERT(!modelName.empty());

    OutValue->clear();
    return bapiInternal::FindAllEntities(FF_IncludeBaseModel, (void*)&modelName, *OutValue);
}

//------------------------------------------------------------------------------------------------
egf::EntityID BehaviorAPI::FindEntityByDataID(const efd::ID128& dataId)
{
    EE_ASSERT(dataId.IsValid());

    EntityManager* pem = g_bapiContext.GetSystemServiceAs<EntityManager>();
    if (pem)
    {
        Entity* pEntity = pem->LookupEntityByDataFileID(dataId);
        if (pEntity)
        {
            return pEntity->GetEntityID();
        }
    }
    return egf::kENTITY_INVALID;
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::FindAllBehaviors(
    const efd::utf8string& modelName,
    efd::list<efd::utf8string>* OutValue)
{
    EE_ASSERT(!modelName.empty());

    FlatModelManager* pfmm = g_bapiContext.GetSystemServiceAs<FlatModelManager>();
    EE_ASSERT(pfmm);
    const FlatModel* pModel = pfmm->FindOrLoadModel(modelName);
    if (pModel)
    {
        pModel->GetBehaviorNames(*OutValue);
        return true;
    }

    EE_LOG(efd::kEntity, efd::ILogger::kERR2,
        ("%s: Error: Model '%s' does not exist.", __FUNCTION__, modelName.c_str()));
    return false;
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::SubscribeToCategory(egf::EntityID entityID, efd::Category catID)
{
    EE_ASSERT(entityID.IsValid());
    EE_ASSERT(catID.IsValid());

    // subscribe this entity to the specified category
    // returns false if the entity is unknown to the scheduler

    Scheduler* pSched = g_bapiContext.GetSystemServiceAs<Scheduler>();
    EE_ASSERT(pSched); // should never happen, of course ...

    Entity* pEntity = pSched->LookupEntity(entityID);
    if (!pEntity)
    {
        EE_LOG(efd::kPython, efd::ILogger::kERR3,
            ("SubscribeToCategory() bapiEntity subscription failed: "
            "entity unknown to scheduler: %s", entityID.ToString().c_str()));
        return false;
    }

    pSched->SubscribeEntity(pEntity, catID);

    EE_LOG(efd::kPython, efd::ILogger::kLVL3,
            ("SubscribeToCategory() bapiEntity subscription success: %s -- %s",
            entityID.ToString().c_str(), catID.ToString().c_str()));

    return true;
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::UnsubscribeToCategory(egf::EntityID entityID, efd::Category catID)
{
    EE_ASSERT(entityID.IsValid());
    EE_ASSERT(catID.IsValid());

    // unsubscribe this entity to the specified category
    // returns false if the entity is unknown to the scheduler
    Scheduler* pSched = g_bapiContext.GetSystemServiceAs<Scheduler>();
    EE_ASSERT(pSched); // again, should never happen, of course ...

    Entity* pEntity = pSched->LookupEntity(entityID);
    if (!pEntity)
    {
        EE_LOG(efd::kPython, efd::ILogger::kERR3,
            ("UnsubscribeToCategory() bapiEntity unsubscription failed: "
            "entity unknown to scheduler: %s", entityID.ToString().c_str()));
        return false;
    }

    pSched->UnsubscribeEntity(pEntity, catID);

    EE_LOG(efd::kPython, efd::ILogger::kLVL3,
            ("UnsubscribeToCategory() bapiEntity unsubscription success: %s -- %s",
            entityID.ToString().c_str(), catID.ToString().c_str()));

    return true;
}

//------------------------------------------------------------------------------------------------
egf::EntityID BehaviorAPI::CreateEntity(
    const utf8string& strModelName,
    ParameterList* pCreationParams,
    const utf8string& callbackBehavior,
    ParameterList* pInitialProperties)
{
    EE_ASSERT(!strModelName.empty());

    Entity* waitingEntity = g_bapiContext.GetScriptEntity();
    EE_ASSERT(waitingEntity);

    EntityManager* pEntityMgr = g_bapiContext.GetSystemServiceAs< EntityManager >();
    EE_ASSERT(pEntityMgr);

    EntityID id = pEntityMgr->CreateEntity(
        strModelName,
        waitingEntity->GetPrivateCatID(),
        pCreationParams,
        pInitialProperties);

    if (id != kENTITY_INVALID)
    {
        if (!callbackBehavior.empty())
        {
            waitingEntity->SetEntityCreateCallbackBehavior(id, callbackBehavior);
        }
    }

    return id;
}

//------------------------------------------------------------------------------------------------
egf::EntityID BehaviorAPI::SpawnEntity(
    const efd::utf8string& modelName,
    efd::ParameterList* pCreationParams)
{
    if (modelName.empty())
    {
        return egf::kENTITY_INVALID;
    }

    egf::FlatModelManager* pFMM = g_bapiContext.GetSystemServiceAs<egf::FlatModelManager>();
    EE_ASSERT(pFMM);

    egf::EntityPtr spEntity = pFMM->FactoryEntity(modelName);
    if (spEntity)
    {
        egf::EntityManager* pEM = g_bapiContext.GetSystemServiceAs<egf::EntityManager>();
        EE_ASSERT(pEM);
        pEM->AddEntity(spEntity, pCreationParams);

        return spEntity->GetEntityID();
    }

    // Attempted to spawn a flat model that isn't preloaded! We log an error and then try to
    // preload the model in the hopes that by the next time this is called it will be loaded.
    EE_LOG(efd::kApp, efd::ILogger::kERR1,
        ("BehaviorAPI.SpawnEntity failed for flat model '%s'. This model must be preloaded.",
        modelName.c_str()));
    pFMM->PreloadModel(modelName);

    return egf::kENTITY_INVALID;
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::DestroyEntity(egf::EntityID id)
{
    EE_ASSERT(id.IsValid());

    Scheduler* pSim = g_bapiContext.GetSystemServiceAs< Scheduler >();
    if (!pSim)
    {
        return false;
    }
    return pSim->DestroyEntity(id);
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::EntityEnterWorld(egf::EntityID id)
{
    EE_ASSERT(id.IsValid());

    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs< EntityManager >();
    if (!pEntityManager)
    {
        return false;
    }
    Entity* pEntity = pEntityManager->LookupEntity(id);
    if (!pEntity)
    {
        return false;
    }
    pEntity->EnterWorld();
    return true;
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::EntityExitWorld(egf::EntityID id)
{
    EE_ASSERT(id.IsValid());

    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs< EntityManager >();
    if (!pEntityManager)
    {
        return false;
    }
    Entity* pEntity = pEntityManager->LookupEntity(id);
    if (!pEntity)
    {
        return false;
    }
    pEntity->ExitWorld();
    return true;
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::IsEntityInWorld(egf::EntityID id)
{
    EE_ASSERT(id.IsValid());

    EntityManager* pEntityManager = g_bapiContext.GetSystemServiceAs< EntityManager >();
    if (!pEntityManager)
    {
        return false;
    }
    Entity* pEntity = pEntityManager->LookupEntity(id);
    if (!pEntity)
    {
        return false;
    }
    return pEntity->IsInWorld();
}

//------------------------------------------------------------------------------------------------
efd::UInt32 BehaviorAPI::IncrementProperty(egf::EntityID id, const efd::utf8string& propName)
{
    EE_ASSERT(id.IsValid());
    EE_ASSERT(!propName.empty());

    efd::UInt32 value = 0;
    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(id);

    if (pEntity)
    {
        if (pEntity->IncrementPropertyValue(propName, value) == egf::PropertyResult_OK)
        {
            return value;
        }
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 BehaviorAPI::DecrementProperty(egf::EntityID id, const efd::utf8string& propName)
{
    EE_ASSERT(id.IsValid());
    EE_ASSERT(!propName.empty());

    efd::UInt32 value = 0;
    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(id);
    if (pEntity)
    {
        if (pEntity->DecrementPropertyValue(propName, value) == egf::PropertyResult_OK)
        {
            return value;
        }
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 BehaviorAPI::SwapProperty(
    egf::EntityID id,
    const efd::utf8string& propName,
    efd::UInt32 newValue)
{
    EE_ASSERT(id.IsValid());
    EE_ASSERT(!propName.empty());

    efd::UInt32 value = 0;
    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    Entity* pEntity = pScheduler->FindEntity(id);
    if (pEntity)
    {
        if (pEntity->GetPropertyValue(propName, value) == egf::PropertyResult_OK)
        {
            if (pEntity->SetPropertyValue(propName, newValue) == egf::PropertyResult_OK)
            {
                return value;
            }
        }
    }

    return 0;
}

