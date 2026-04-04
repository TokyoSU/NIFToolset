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

#include <efd/ILogger.h>
#include <efd/NetMessage.h>
#include <efd/BitUtils.h>
#include <efd/Metrics.h>
#include <efd/MemTracker.h>
#include <efd/ServiceManager.h>

#include <egf/Entity.h>
#include <egf/EntityManager.h>
#include <egf/Scheduler.h>
#include <egf/EntityIDFactory.h>
#include <egf/FlatModelManager.h>
#include <egf/IBuiltinModel.h>
#include <egf/ScriptContext.h>
#include <egf/egfLogIDs.h>
#include <egf/EntityFactoryResponse.h>
#include <egf/SimDebugger.h>

#include <egf/egfSDM.h>

using namespace efd;
using namespace egf;

/// Note: This inserts egf into the static data manager chain.  It must be placed in a compilation
/// unit that is always used when the library is used (if it is only placed in egfSDM.cpp, it will
/// be removed by the linker)
static egfSDM egfSDMObject;


/// This is the maximum number of behaviors we ever expect to be running on a single entity.
/// It is used as a debugging aid, if we exceed this maximum its most likely because we are
/// failing to call FinishEvent or leaking PendingBehavior instances someplace.
static const efd::SInt32 kMaxExpectedBehaviors = 200;

//------------------------------------------------------------------------------------------------
// Entity Model class method implementations
//
EE_IMPLEMENT_CONCRETE_CLASS_INFO(Entity);

EE_HANDLER_WRAP(Entity, HandleMessage, egf::EventMessage, kMSGID_Event);
EE_HANDLER_WRAP(Entity, HandleMessage, egf::EventMessage, kMSGID_EventCancel);
EE_HANDLER_WRAP(Entity, HandleMessage, egf::EventMessage, kMSGID_EventReturn);

EE_HANDLER(Entity, HandleEntityFactoryResponse, egf::EntityFactoryResponse);

//------------------------------------------------------------------------------------------------
//
// Constructors for the Entity class
//
//  Since we use framework to create entity instances, don't get to pass params,
//  so, we call default constructor and then private Setter methods from static
//  create methods to fill in data...
//
Entity::Entity(const FlatModel* i_pTemplate, egf::EntityID i_eid, bool i_bCreateAsMasterCopy)
    : m_entityID(i_eid)
    , m_pFlatModel(i_pTemplate)
    , m_pEntityManager(NULL)
    , m_pScheduler(NULL)
    , m_pendingBehaviorCount(0)
    , m_executingBehaviorCount(0)
    , m_flagBits(0)
    , m_iterationCount(0)
{
    EE_MEM_SETDETAILEDREPORT(this, Entity::LeakDump);

    EE_LOG_METRIC_INCREMENT(kEntity, "TALLY");

    // "Master Copy" means its properties can be modified and its BuiltinModel objects will be
    // allocated.
    // Non-master copies are intended for replicated copies of entities.
    if (i_bCreateAsMasterCopy)
    {
        SetOwned(true);
    }

    // If needed, get the next id from the entityID factory
    if (!m_entityID.IsValid())
    {
        EntityID nextID = EntityIDFactory::GetNextID();
        EE_ASSERT(nextID.IsValid());
        SetID(nextID);
    }
}

//------------------------------------------------------------------------------------------------
Entity::~Entity()
{
    // decrement another destroyed entity (these are the running "in scheduler" counts)
    EE_LOG_METRIC_DECREMENT(kEntity, "TALLY");

    m_dirtyPropertySet.clear();
    for (PropertyMap::iterator itr = m_propertyMap.begin();
          itr != m_propertyMap.end();
          ++itr)
    {
        FreeProperty(itr->first, itr->second);
    }

    // It should be impossible to get here unless this queue is empty since enties in this queue
    // hold references the the entity itself, thus preventing the ref count from hitting zero.
    EE_ASSERT(m_pendingBehaviorQueue.empty());
}

//------------------------------------------------------------------------------------------------
//
// Property access methods
//
//------------------------------------------------------------------------------------------------
PropertyResult Entity::_GetPropDesc(PropertyID i_propID, const PropertyDescriptor*& o_ppd) const
{
    // make sure there's a flat model
    if (!m_pFlatModel)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Tried to check property type on Entity ID: %s, but entity has no flat model.",
            m_entityID.ToString().c_str()));

        return PropertyResult_ModelNotFound;
    }

    // make sure the flat model has a descriptor for the property id
    o_ppd = m_pFlatModel->GetPropertyDescriptor(i_propID);
    if (!o_ppd)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR3,
            ("Tried to check property type on Entity ID: %s, but could not find a property "
            "descriptor for Property ID: %d in Flat Model: %d (%s)",
            m_entityID.ToString().c_str(),
            i_propID,
            m_pFlatModel->GetID(),
            m_pFlatModel->GetName().c_str()));

        return PropertyResult_PropertyNotFound;
    }

    return PropertyResult_OK;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::_GetPropDesc(
    const efd::utf8string& i_strPropName,
    const PropertyDescriptor*& o_ppd) const
{
    // make sure there's a flat model
    if (!m_pFlatModel)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Tried to check property type on Entity ID: %s, but entity has no flat model.",
            m_entityID.ToString().c_str()));

        return PropertyResult_ModelNotFound;
    }

    // make sure the flat model has a descriptor for the property id
    o_ppd = m_pFlatModel->GetPropertyDescriptor(i_strPropName);
    if (!o_ppd)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR3,
            ("Tried to check property type on Entity ID: %s, but could not find a property "
            "descriptor for Property named: \"%s\" in Flat Model: %d (%s)",
            m_entityID.ToString().c_str(),
            i_strPropName.c_str(),
            m_pFlatModel->GetID(),
            m_pFlatModel->GetName().c_str()));

        return PropertyResult_PropertyNotFound;
    }

    return PropertyResult_OK;
}

//------------------------------------------------------------------------------------------------
inline void Entity::FreeProperty(PropertyID propID, IProperty* prop)
{
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = _GetPropDesc(propID, pDesc);
    EE_UNUSED_ARG(result);

    // To be in our property map these look-ups must have worked before, we rely on them
    // continuing to work in order to properly release our memory:
    EE_ASSERT(PropertyResult_OK == result);
    EE_ASSERT(NULL != pDesc);

    // If the entry is a BuiltinModel we don't want to delete it, it will get released when
    // the BuiltinModel map is cleared.  So only delete the basic IProperty properties.
    if (!ShouldPropertyUseBuiltinModel(pDesc))
    {
        EE_DELETE prop;
    }
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetPropertyType(PropertyID i_propID, efd::ClassID& o_result) const
{
    o_result = 0;

    const PropertyDescriptor* pDescriptor;
    PropertyResult retval = _GetPropDesc(i_propID, pDescriptor);
    if (PropertyResult_OK == retval)
    {
        o_result = pDescriptor->GetPropertyClassID();
    }

    return retval;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetPropertyType(
    const efd::utf8string& i_strPropName,
    efd::ClassID& o_result) const
{
    o_result = 0;

    const PropertyDescriptor* pDescriptor;
    PropertyResult retval = _GetPropDesc(i_strPropName, pDescriptor);
    if (PropertyResult_OK == retval)
    {
        o_result = pDescriptor->GetPropertyClassID();
    }

    return retval;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetDataStorageType(PropertyID i_propID, efd::ClassID& o_result) const
{
    o_result = 0;

    const PropertyDescriptor* pDescriptor;
    PropertyResult retval = _GetPropDesc(i_propID, pDescriptor);
    if (PropertyResult_OK == retval)
    {
        o_result = pDescriptor->GetDataClassID();
    }

    return retval;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetDataStorageType(
    const efd::utf8string& i_strPropName,
    efd::ClassID& o_result) const
{
    o_result = 0;

    const PropertyDescriptor* pDescriptor;
    PropertyResult retval = _GetPropDesc(i_strPropName, pDescriptor);
    if (PropertyResult_OK == retval)
    {
        o_result = pDescriptor->GetDataClassID();
    }

    return retval;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetValueAsString(PropertyID propID, efd::utf8string& data) const
{
    const PropertyDescriptor* pDescriptor;
    PropertyResult result = _GetPropDesc(propID, pDescriptor);
    if (result != PropertyResult_OK)
        return result;
    const IProperty* pProp = GetPropertyForReading(pDescriptor);
    return pProp->GetValueAsString(propID, data);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetValueAsString(
    PropertyID propID,
    const efd::utf8string& key,
    efd::utf8string& data) const
{
    const PropertyDescriptor* pDescriptor;
    PropertyResult result = _GetPropDesc(propID, pDescriptor);
    if (result != PropertyResult_OK)
        return result;
    const IProperty* pProp = GetPropertyForReading(pDescriptor);
    return pProp->GetValueAsString(propID, key, data);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetPropertyCount(PropertyID i_propID, efd::UInt32& o_count) const
{
    o_count = 0;

    PropertyMap::const_iterator iter = m_propertyMap.find(i_propID);
    if (iter != m_propertyMap.end())
    {
        const IProperty* pProp = iter->second;
        return pProp->GetPropertyCount(i_propID, o_count);
    }

    if (!m_pFlatModel)
    {
        return PropertyResult_ModelNotFound;
    }

    const PropertyDescriptor* pDescriptor = m_pFlatModel->GetPropertyDescriptor(i_propID);
    if (!pDescriptor)
    {
        return PropertyResult_PropertyNotFound;
    }

    const IProperty* pProp = GetPropertyForReading(pDescriptor);
    return pProp->GetPropertyCount(i_propID, o_count);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetPropertyCount(
    const efd::utf8string& i_strName,
    efd::UInt32& o_count) const
{
    egf::PropertyID propID = EE_NAME2ID(i_strName);
    if (propID == 0)
    {
        o_count = 0;
        return PropertyResult_PropertyNotFound;
    }
    return GetPropertyCount(propID, o_count);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetPropertyKeys(
    PropertyID i_propID,
    efd::list<efd::utf8string>& o_keys) const
{
    if (!m_pFlatModel)
    {
        return PropertyResult_ModelNotFound;
    }

    const PropertyDescriptor* pDescriptor = m_pFlatModel->GetPropertyDescriptor(i_propID);
    if (!pDescriptor)
    {
        return PropertyResult_PropertyNotFound;
    }

    const IProperty* pProp = GetPropertyForReading(pDescriptor);
    return pProp->GetPropertyKeys(i_propID, o_keys);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetPropertyKeys(
    const efd::utf8string& i_strName,
    efd::list<efd::utf8string>& o_keys) const
{
    egf::PropertyID propID = EE_NAME2ID(i_strName);
    if (propID == 0)
    {
        o_keys.clear();
        return PropertyResult_PropertyNotFound;
    }
    return GetPropertyKeys(propID, o_keys);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetNextPropertyKey(
    PropertyID i_propID,
    const efd::utf8string& i_previousKey,
    efd::utf8string& o_nextKey) const
{
    if (!m_pFlatModel)
    {
        return PropertyResult_ModelNotFound;
    }

    const PropertyDescriptor* pDescriptor = m_pFlatModel->GetPropertyDescriptor(i_propID);
    if (!pDescriptor)
    {
        return PropertyResult_PropertyNotFound;
    }

    const IProperty* pProp = GetPropertyForReading(pDescriptor);
    return pProp->GetNextPropertyKey(i_propID, i_previousKey, o_nextKey);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetNextPropertyKey(
    const efd::utf8string& i_strName,
    const efd::utf8string& i_previousKey,
    efd::utf8string& o_nextKey) const
{
    egf::PropertyID propID = EE_NAME2ID(i_strName);
    if (propID == 0)
    {
        o_nextKey.clear();
        return PropertyResult_PropertyNotFound;
    }
    return GetNextPropertyKey(propID, i_previousKey, o_nextKey);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::ValidateTypeSafety(
    PropertyID propID,
    efd::ClassID classID,
    const PropertyDescriptor*& rpDescriptor) const
{
    // make sure there's a flat model
    if (!m_pFlatModel)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Tried to validate type safety on Entity ID: %s, but entity has no flat model.",
            m_entityID.ToString().c_str()));

        return PropertyResult_ModelNotFound;
    }

    // make sure the flat model has a descriptor for the property id
    rpDescriptor = m_pFlatModel->GetPropertyDescriptor(propID);
    if (!rpDescriptor)
    {
        efd::utf8string propName(efd::Formatted, "0x%08X", propID);
        if (GetServiceManager()) //< Can be NULL if entity is not yet added to EntityManager
        {
            FlatModelManager* pfmm = GetServiceManager()->GetSystemServiceAs<FlatModelManager>();
            if (pfmm)
            {
                pfmm->GetPropertyNameByID(propID, propName);
            }
        }

        EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
            ("Tried to validate type safety on Entity ID: %s, but could not find a property "
            "descriptor for Property '%s' in Flat Model: %d (%s)",
            m_entityID.ToString().c_str(),
            propName.c_str(),
            m_pFlatModel->GetID(),
            m_pFlatModel->GetName().c_str()));

        return PropertyResult_PropertyNotFound;
    }

    // check the data class id in the descriptor against the parameter
    if (rpDescriptor->GetDataClassID() != classID)
        return PropertyResult_TypeMismatch;

    return PropertyResult_OK;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetPropertyValueUnsafe(
    PropertyID propID,
    void* data,
    efd::ClassID classID) const
{
    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = ValidateTypeSafety(propID, classID, pDesc);
    if (result != PropertyResult_OK)
        return result;

    const IProperty* pProp = GetPropertyForReading(pDesc);

    // assign the value from the property to data
    return pProp->GetValue(propID, data);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetPropertyValueUnsafe(
    PropertyID propID,
    const efd::utf8string& key,
    void* data,
    efd::ClassID classID) const
{
    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = ValidateTypeSafety(propID, classID, pDesc);
    if (result != PropertyResult_OK)
        return result;

    const IProperty* pProp = GetPropertyForReading(pDesc);

    // assign the value from the property to data
    return pProp->GetValue(propID, key, data);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetDefaultPropertyValueUnsafe(
    PropertyID propID,
    void* data,
    efd::ClassID classID) const
{
    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = ValidateTypeSafety(propID, classID, pDesc);
    if (result != PropertyResult_OK)
        return result;

    // use the descriptor to get the default property and assign that value to data
    return pDesc->GetDefaultProperty()->GetValue(propID, data);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::GetDefaultPropertyValueUnsafe(
    PropertyID propID,
    const efd::utf8string& key,
    void* data,
    efd::ClassID classID) const
{
    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = ValidateTypeSafety(propID, classID, pDesc);
    if (result != PropertyResult_OK)
        return result;

    // use the descriptor to get the default property and assign that value to data
    return pDesc->GetDefaultProperty()->GetValue(propID, key, data);
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::ResetProperty(const efd::utf8string& propName)
{
    const PropertyDescriptor* pDesc = m_pFlatModel->GetPropertyDescriptor(propName);

    if (pDesc)
    {
        return ResetProperty(pDesc->GetPropertyID());
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("ResetProperty failed on property: %s for EntityID: %s. "
            "Could not find PropertyDescriptor in flatmodel: %s.",
            propName.c_str(),
            m_entityID.ToString().c_str(),
            m_pFlatModel->GetName().c_str()));
        return PropertyResult_PropertyNotFound;
    }
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::ResetProperty(PropertyID propID)
{
    const PropertyDescriptor* pDesc = GetModel()->GetPropertyDescriptor(propID);
    if (pDesc)
    {
        if (ShouldPropertyUseBuiltinModel(pDesc))
        {
            // In order to "reset" a BuiltinModel property we actually need to manually set the
            // property to the default value.  Simply removing the IProperty from our property
            // map isn't good enough since the code never used the default value for BuiltinModel
            // properties and will always directly access the BuiltinModel itself.

            IBuiltinModel* pComp = FindBuiltinModel(pDesc->GetSource());
            EE_VERIFY(pComp->ResetProperty(pDesc));
            SetDirty(propID, pComp);
        }
        else
        {
            PropertyMap::iterator iterValues = m_propertyMap.find(propID);
            if (iterValues != m_propertyMap.end())
            {
                // For a regular property, we simply discard the override from the property map and
                // it will fall back to the default value:
                IProperty* pProp = iterValues->second;
                m_propertyMap.erase(propID);

                FreeProperty(propID, pProp);

                const IProperty* pDefaultProp = pDesc->GetDefaultProperty();
                SetDirty(propID, pDefaultProp);
            }
        }
        return PropertyResult_OK;
    }

    return PropertyResult_PropertyNotFound;
}

//------------------------------------------------------------------------------------------------
void Entity::RapidIterateResetProperty(
    PropertyID changedPropID,
    const PropertyDescriptor* newDesc)
{
    // Always try to remove the override value, if any. This will potentially free the old property
    // value using the old PropertyDescriptor so everything should be okay.
    RemoveProperty(changedPropID);

    if (ShouldPropertyUseBuiltinModel(newDesc))
    {
        // To result a built-in model property we have to push the new default value into the
        // built-in model in order for it to take effect. We need to use the correct new default
        // default value so we can't just call ResetProperty.
        IBuiltinModel* pComp = FindBuiltinModel(newDesc->GetSource());
        if (pComp)
        {
            EE_VERIFY(pComp->ResetProperty(newDesc));
            SetDirty(changedPropID, pComp);
        }
        else
        {
            // You tried to rapidly iterated a new component into existence, this is NOT going to
            // work! Once we update the FlatModel for this entity any access of the property in
            // question would fail (and most likely crash the process)
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Missing Builtin: property '%s' in model '%s' was rapidly iterated to use built-in"
                " model '%s' which is missing. This will result in a crash if this property is "
                "accessed!",
                newDesc->GetName().c_str(),
                GetModelName().c_str(),
                newDesc->GetSource().c_str()));
        }
    }
    else
    {
        // For normal properties we already removed the override, if present, so we just need to
        // mark it as dirty using the new property value:
        SetDirty(changedPropID, newDesc->GetDefaultProperty());
    }
}

//------------------------------------------------------------------------------------------------
void Entity::RapidIterateDefaultPropertyValue(
    PropertyID changedPropID,
    const PropertyDescriptor* newDesc)
{
    const PropertyDescriptor* oldDesc = GetModel()->GetPropertyDescriptor(changedPropID);

    // When the default value has changed we need to update and/or dirty the current value if:
    // 1) The property is not overridden and thus the change in default is a change in current
    //      value so it needs to be marked as dirty (or reset if it is a built-in model property)
    // 2) The property implementation class changed so any old value must be discarded
    // 3) The "built-in"-ness of the property changed so any old value must be discarded
    // 4) The property data type changed so any old value must be discarded. Technically this
    //      check should be redundant because any data type change also changes property type.
    if ((!IsPropertyOverridden(changedPropID)) ||
        (oldDesc->GetPropertyClassID() != newDesc->GetPropertyClassID()) ||
        (ShouldPropertyUseBuiltinModel(oldDesc) != ShouldPropertyUseBuiltinModel(newDesc)) ||
        (oldDesc->GetDataClassID() != newDesc->GetDataClassID()))
    {
        RapidIterateResetProperty(changedPropID, newDesc);
    }
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::RemoveProperty(PropertyID propID)
{
    PropertyMap::iterator iterValues = m_propertyMap.find(propID);

    if (iterValues != m_propertyMap.end())
    {
        IProperty* pProp = iterValues->second;
        FreeProperty(propID, pProp);
        m_propertyMap.erase(propID);
        return PropertyResult_OK;
    }

    return PropertyResult_KeyNotFound;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::ResetAllProperties()
{
    PropertyResult result = PropertyResult_OK;

    // Calling ResetProperty will remove items from the m_propertyMap, but only if they are non
    // Built-in Model properties.  This makes iterating m_propertyMap directly tricky, so we are
    // making a copy of the map and iterating that instead.  This is only called during rapid
    // iteration scenarios so the cost of this copy should be acceptable.
    PropertyMap originalMap = m_propertyMap;
    for (PropertyMap::iterator iter = originalMap.begin();
         iter != originalMap.end();
         ++iter)
    {
        PropertyResult check = ResetProperty(iter->first);
        if (check != PropertyResult_OK)
        {
            // If we hit any errors we'll return just the last error returned.  But we still
            // continue on trying to reset all the remaining properties.
            result = check;
        }
    }
    return result;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::SetPropertyValueUnsafe(
    PropertyID propID,
    const void* data,
    efd::ClassID classID)
{
    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = ValidateTypeSafety(propID, classID, pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    result = CheckWritability(pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    IProperty* pProp = GetPropertyForWriting(pDesc);

    // assign the value to the property
    result = pProp->SetValue(propID, data);

    // set the property dirty
    if (result == PropertyResult_OK)
    {
        SetDirty(propID, pProp);
    }

    return result;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::SetPropertyValueUnsafe(
    PropertyID propID,
    const efd::utf8string& key,
    const void* data,
    efd::ClassID classID)
{
    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = ValidateTypeSafety(propID, classID, pDesc);
    if (result != PropertyResult_OK)
        return result;

    result = CheckWritability(pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    IProperty* pProp = GetPropertyForWriting(pDesc);

    // assign the value to the property
    result = pProp->SetValue(propID, key, data);

    // set the property dirty
    if (result == PropertyResult_OK)
    {
        SetDirty(propID, pProp);
    }

    return result;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::SetPropertyValueByString(
    PropertyID propID,
    const efd::utf8string& i_strValue)
{
    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = _GetPropDesc(propID, pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    result = CheckWritability(pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    IProperty* pProp = GetPropertyForWriting(pDesc);

    // assign the value to the property
    result = pProp->SetValueByString(propID, i_strValue);

    // set the property dirty
    if (result == PropertyResult_OK)
    {
        SetDirty(propID, pProp);
    }

    return result;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::SetPropertyValueByString(
    PropertyID propID,
    const efd::utf8string& i_strKey,
    const efd::utf8string& i_strValue)
{
    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = _GetPropDesc(propID, pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    result = CheckWritability(pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    IProperty* pProp = GetPropertyForWriting(pDesc);

    // assign the value to the property
    result = pProp->SetValueByString(propID, i_strKey, i_strValue);

    // set the property dirty
    if (result == PropertyResult_OK)
    {
        SetDirty(propID, pProp);
    }

    return result;
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::HasPropertyValue(PropertyID propID, const efd::utf8string& key) const
{
    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = _GetPropDesc(propID, pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    const IProperty* pProp = GetPropertyForReading(pDesc);
    return pProp->HasValue(propID, key);
}

//------------------------------------------------------------------------------------------------
inline bool Entity::ShouldPropertyUseBuiltinModel(const PropertyDescriptor* pDesc) const
{
    // NOTE: A property might have both the FromBuiltinModel and FromReplicaBuiltinModel traits set.

    if (IsOwned())
    {
        // BuiltinModel properties only come from the BuiltinModel on the owned copy of the entity,
        // otherwise we keep a normal IProperty version just like any other property.
        return pDesc->GetTrait(PropertyTrait_FromBuiltinModel);
    }

    // Conversely ReplicaBuiltinModel properties only come from the BuiltinModel on the replicated
    // copy of the entity, otherwise we keep a normal IProperty version just like any other
    // property.
    return pDesc->GetTrait(PropertyTrait_FromReplicaBuiltinModel);
}

//------------------------------------------------------------------------------------------------
const IProperty* Entity::GetPropertyForReading(const PropertyDescriptor* pDesc) const
{
    const IProperty* pProp = NULL;

    // find the property locally
    PropertyMap::const_iterator iterValues = m_propertyMap.find(pDesc->GetPropertyID());
    if (iterValues != m_propertyMap.end())
    {
        pProp = iterValues->second;
    }
    else
    {
        // If the property couldn't be found locally, then use the default property.
        // For BuiltinModel properties the BuiltinModel always has the correct current value.
        if (ShouldPropertyUseBuiltinModel(pDesc))
        {
            pProp = FindBuiltinModel(pDesc->GetSource());
        }
        else
        {
            pProp = pDesc->GetDefaultProperty();
        }
    }

    return pProp;
}

//------------------------------------------------------------------------------------------------
inline PropertyResult Entity::CheckWritability(const PropertyDescriptor* pDesc) const
{
    if (pDesc->GetTrait(PropertyTrait_ReadOnly))
    {
        return PropertyResult_ReadOnlyError;
    }

    // Only an "owned" (i.e. in the scheduler) entity can have properties set on it.  Replicated
    // entities are effectively read-only copies.  The Mutable trait overrides this default,
    // but keep in mind mutable writes will not get replicated to anyone else
    if (!IsOwned() && !pDesc->GetTrait(PropertyTrait_Mutable))
    {
        return PropertyResult_EntityNotOwned;
    }

    return PropertyResult_OK;
}

//------------------------------------------------------------------------------------------------
IProperty* Entity::GetPropertyForWriting(const PropertyDescriptor* pDesc)
{
    IProperty* pProp = NULL;

    // First we look at our local list of properties we've modifed before
    PropertyID propID = pDesc->GetPropertyID();
    PropertyMap::iterator iterValues = m_propertyMap.find(propID);
    if (iterValues != m_propertyMap.end())
    {
        pProp = iterValues->second;
    }
    else
    {
        // Don't have an overridden copy of this property yet, so we need to find its source.
        if (ShouldPropertyUseBuiltinModel(pDesc))
        {
            pProp = FindBuiltinModel(pDesc->GetSource());
        }
        else
        {
            // Clone it from the model and stuff it in the local map
            pProp = pDesc->GetDefaultProperty()->Clone();
        }

        m_propertyMap[propID] = pProp;
    }
    return pProp;
}

//------------------------------------------------------------------------------------------------
void Entity::BuiltinPropertyChanged(PropertyID propID, IBuiltinModel* pProp)
{
    // The built-in model may or may not already be in the property map. If the value has never
    // been changed through a Entity::SetPropertyValue call or through a block file override then
    // it will not be in the property map. We need to add it to the property map in order for
    // certain simulation debugging commands and online entity replication features to work. This
    // also optimizes any future property reads or writes to one map lookup rather than three. If
    // the property is already in the map it should have the exact same pointer as pProp.
    EE_ASSERT(m_propertyMap.count(propID) == 0 || m_propertyMap[propID] == pProp);
    m_propertyMap[propID] = pProp;

    SetDirty(propID, pProp);
}

//------------------------------------------------------------------------------------------------
void Entity::SetDirty(PropertyID propID, const IProperty* prop)
{
    // add this property to this entities dirty map (list of dirty properties)
    //  Note: we use a map so multiple "sets" are ignored (only one entry per ID)
    m_dirtyPropertySet.insert(propID);

    // check that we have a scheduler owner
    if (m_pEntityManager)
    {
        // if here, we are owned, we can add this entity to scheduler's dirty list
        m_pEntityManager->AddDirty(this);
    }

#ifndef EE_CONFIG_SHIPPING
    if (SimDebugger::Instance())
        SimDebugger::Instance()->SendPropertyUpdate(this, propID, prop);
#else
    EE_UNUSED_ARG(prop);
#endif
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::RemovePropertyValue(
    PropertyID propID,
    const efd::utf8string& key)
{
    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    pDesc = m_pFlatModel->GetPropertyDescriptor(propID);
    if (!pDesc)
    {
        efd::utf8string propName(efd::Formatted, "0x%08X", propID);
        if (GetServiceManager()) //< Can be NULL if entity is not yet added to EntityManager
        {
            FlatModelManager* pfmm = GetServiceManager()->GetSystemServiceAs<FlatModelManager>();
            if (pfmm)
            {
                pfmm->GetPropertyNameByID(propID, propName);
            }
        }

        EE_LOG(efd::kEntity, efd::ILogger::kLVL2,
            ("Tried to validate type safety on Entity ID: %s, but could not find a property "
            "descriptor for Property ID: %d in Flat Model: %d (%s)",
            m_entityID.ToString().c_str(),
            propName.c_str(),
            m_pFlatModel->GetID(),
            m_pFlatModel->GetName().c_str()));

        return PropertyResult_PropertyNotFound;
    }

    PropertyResult result = CheckWritability(pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    IProperty* pProp = GetPropertyForWriting(pDesc);

    // assign the value to the property
    result = pProp->RemoveValue(propID, key);

    // set the property dirty
    if (result == PropertyResult_OK)
    {
        SetDirty(propID, pProp);
    }

    return result;
}


//------------------------------------------------------------------------------------------------
const efd::ServiceManager* Entity::GetServiceManager() const
{
    if (m_pEntityManager)
    {
        return m_pEntityManager->GetServiceManager();
    }
    return NULL;
}


//------------------------------------------------------------------------------------------------
PropertyID Entity::_Name2ID(const efd::utf8string& propName, const char* pszMethod) const
{
    const PropertyDescriptor* ppd = m_pFlatModel->GetPropertyDescriptor(propName);
    if (ppd)
    {
        return ppd->GetPropertyID();
    }

    // failed to find property descriptor
    EE_LOG(efd::kEntity, efd::ILogger::kERR3,
        ("%s, %s, could not find a property (%s) in Flat Model: 0x%08X (%s)",
        pszMethod,
        m_entityID.ToString().c_str(),
        propName.c_str(),
        m_pFlatModel->GetID(),
        m_pFlatModel->GetName().c_str()));
    EE_UNUSED_ARG(pszMethod);

    return 0;
}


//------------------------------------------------------------------------------------------------
efd::Bool Entity::CallImmediateBehavior(
    BehaviorID i_behaviorID,
    FlatModelID i_mixinModelID,
    efd::ParameterList* pArgs,
    EventID eventID,
    EntityID returnID)
{
    if (!m_pScheduler)
    {
        // if here, error: The entity doesn't have a scheduler set.
         EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: Behavior '0x%08X' cannot execute in flat model '%s' because there is no"
            " scheduler.", __FUNCTION__, i_behaviorID, GetModelName().c_str()));
        return false;
    }

    if (i_mixinModelID && !GetModel()->ContainsModel(i_mixinModelID))
    {
        // if here, error: The entity does not mix-in the proper model to be allowed to execute
        // this behavior
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Entity type '%s'(%s) does not mixin model '0x%08X'.  Cannot CallImmediateBehavior "
            "on '0x%08X'",
            GetModelName().c_str(), GetEntityID().ToString().c_str(),
            i_mixinModelID, i_behaviorID));
    }

    const BehaviorDescriptor* pBehavior = GetModel()->GetBehaviorDescriptor(i_behaviorID);
    if (pBehavior == NULL)
    {
        // if here, error: didn't find that behavior in this entity's model's list of behaviors
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: Behavior '0x%08X' not contained in flat model '%s'.",
            __FUNCTION__, i_behaviorID, GetModelName().c_str()));
        return false;
    }

    if (!pBehavior->GetTrait(BehaviorTrait_Immediate))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: Behavior '0x%08X' in flat model '%s' is not immedate.",
            __FUNCTION__, i_behaviorID, GetModelName().c_str()));
        return false;
    }

    bool result = true;

    if (eventID == 0)
    {
        eventID = EventID::CreateEventID();
    }

    PendingBehaviorPtr spBehaviorToRun =
        PendingBehavior::Create(pBehavior, this, pArgs, time_Now, eventID, returnID, true);
    if (EE_VERIFY(spBehaviorToRun))
    {
        // Immediate behaviors bypass InsertBehavior but StartBehavior still assumes the
        // behavior was pending, so up the count one here:
        ++m_pendingBehaviorCount;

        spBehaviorToRun->DoTask(m_pScheduler);

        EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
            ("Immediate behavior '%s:%s' %s executed for model '%s' %s by %s",
            pBehavior->GetModelName().c_str(),
            pBehavior->GetName().c_str(),
            eventID.ToString().c_str(),
            GetModelName().c_str(),
            GetEntityID().ToString().c_str(),
            returnID.ToString().c_str()));
    }

    return result;
}

//------------------------------------------------------------------------------------------------
efd::Bool Entity::CallImmediateBehavior(
    const efd::utf8string& i_strBehaviorName,
    const efd::utf8string& i_strMixinModelName,
    efd::ParameterList* pArgs,
    EventID eventID,
    EntityID returnID)
{
    if (!m_pScheduler)
    {
        // if here, error: The entity doesn't have a scheduler set.
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: Behavior '%s' cannot execute in flat model '%s' because there is no"
            " scheduler.", __FUNCTION__, i_strBehaviorName.c_str(), GetModelName().c_str()));
        return false;
    }

    if (!i_strMixinModelName.empty() && !GetModel()->ContainsModel(i_strMixinModelName))
    {
        // if here, error: The entity does not mix-in the proper model to be allowed to execute
        // this behavior
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Entity type '%s'(%s) does not mixin model '%s'.  Cannot CallImmediateBehavior "
            "on '%s'",
            GetModelName().c_str(), GetEntityID().ToString().c_str(),
            i_strMixinModelName.c_str(), i_strBehaviorName.c_str()));
        return false;
    }

    const BehaviorDescriptor* pBehavior = GetModel()->GetBehaviorDescriptor(i_strBehaviorName);
    if (pBehavior == NULL)
    {
        // if here, error: didn't find that behavior in this entity's model's list of behaviors
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: Behavior '%s' not contained in flat model '%s'.",
            __FUNCTION__, i_strBehaviorName.c_str(), GetModelName().c_str()));
        return false;
    }

    if (!pBehavior->GetTrait(BehaviorTrait_Immediate))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: Behavior '%s' in flat model '%s' is not immediate.",
            __FUNCTION__, i_strBehaviorName.c_str(), GetModelName().c_str()));
        return false;
    }

    bool result = true;
    PendingBehaviorPtr spBehaviorToRun =
        PendingBehavior::Create(pBehavior, this, pArgs, time_Now, eventID, returnID, true);
    if (EE_VERIFY(spBehaviorToRun))
    {
        // Immediate behaviors bypass InsertBehavior but StartBehavior still assumes the
        // behavior was pending, so up the count one here:
        ++m_pendingBehaviorCount;

        spBehaviorToRun->DoTask(m_pScheduler);

        EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
            ("Immediate behavior '%s:%s' (eventID=0x%016llX) executed for model '%s' %s by %s",
            pBehavior->GetModelName().c_str(),
            pBehavior->GetName().c_str(),
            eventID.GetValue(),
            GetModelName().c_str(),
            GetEntityID().ToString().c_str(),
            returnID.ToString().c_str()));
    }

    return result;
}

//------------------------------------------------------------------------------------------------
bool Entity::AddPendingBehavior(const EventMessage* pMessage)
{
    if (!EE_VERIFY_MESSAGE(GetServiceManager() != 0,
        ("An attempt was made to add a pending behavior when the Service Manager was "
        "unavailable. Are trying to add pending behaviors during shutdown?")))
    {
        return false;
    }

    if (!EE_VERIFY_MESSAGE(pMessage, ("Null message passed to AddPendingBehavior")))
    {
        return false;
    }

    if (!m_pScheduler)
    {
        // if here, error: The entity doesn't have a scheduler set.
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Error: Behavior id=%d cannot execute in flat model '%s' for this entity"
            " because there is no scheduler.",
            pMessage->GetBehaviorID(), GetModelName().c_str()));
        return false;
    }

    const BehaviorDescriptor* pBehavior =
        GetModel()->GetBehaviorDescriptor(pMessage->GetBehaviorID());
    if (pBehavior == NULL)
    {
        // if here, error: didn't find that behavior in this entity's model's list of behaviors
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Error: Behavior id=%d not contained in flat model '%s' for this entity",
            pMessage->GetBehaviorID(), GetModelName().c_str()));
        return false;
    }

    // Calculate the time that this behavior event should be executed
    efd::TimeType executionTime = GetExecutor()->GetGameTime() + pMessage->GetDelay();

    // DT32337 We should transfer "needs response" info from the message into the pending behavior
    // and use that to provide improved error feedback.

    return AddPendingBehavior(
        pBehavior,
        pMessage->GetMixinModelID(),
        pMessage->GetParameters(),
        executionTime,
        pMessage->GetEventID(),
        pMessage->GetSenderID());
}


//------------------------------------------------------------------------------------------------
bool Entity::AddPendingBehavior(BehaviorID id,
    FlatModelID mixinModelID,
    ParameterList* spArgs,
    efd::TimeType executionTime /*= 0.0*/,
    EventID eventID /*= 0*/,
    EntityID returnID /*= 0*/)
{
    const BehaviorDescriptor* pBehavior = GetModel()->GetBehaviorDescriptor(id);
    if (pBehavior == NULL)
    {
        // if here, error: didn't find that behavior in
        //  this entity's model's list of behaviors
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Error: Behavior id=%d not contained in flat model '%s' for this entity",
            id, GetModelName().c_str()));
        return false;
    }

    if (!m_pScheduler)
    {
        // if here, error: The entity doesn't have a scheduler set.
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Error: Behavior id=%d cannot execute in flat model '%s' for this entity"
            " because there is no scheduler.",
            id, GetModelName().c_str()));
        return false;
    }

    return AddPendingBehavior(pBehavior, mixinModelID, spArgs, executionTime, eventID, returnID);
}


//------------------------------------------------------------------------------------------------
bool Entity::AddPendingBehavior(const efd::utf8string& i_strBehaviorName,
    const efd::utf8string& i_strMixinModelName,
    ParameterList* spArgs,
    efd::TimeType executionTime /*= 0.0*/,
    EventID eventID /*= 0*/,
    EntityID returnID /*= 0*/)
{
    if (!m_pScheduler)
    {
        // if here, error: The entity doesn't have a scheduler set.
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Error: Behavior '%s' cannot execute in flat model '%s' for this entity"
            " because there is no scheduler.",
            i_strBehaviorName.c_str(),
            GetModelName().c_str()));
        return false;
    }

    const BehaviorDescriptor* pBehavior = GetModel()->GetBehaviorDescriptor(i_strBehaviorName);
    if (pBehavior == NULL)
    {
        // if here, error: didn't find that behavior in
        //  this entity's model's list of behaviors
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("%s: Error: Behavior '%s' not contained in flat model '%s' for this entity",
            __FUNCTION__,
            i_strBehaviorName.c_str(),
            GetModelName().c_str()));
        return false;
    }

    // If the model name is not NULL then make sure the entity contains specified model
    if ((i_strMixinModelName != efd::utf8string::NullString()) &&
        (!GetModel()->ContainsModel(i_strMixinModelName)))
    {
        // If here, the entity does not mix-in the proper model to be allowed to execute this
        // behavior. This isn't really an error, the model mix-in restriction is meant to be used
        // when you are sending a behavior to a category that many entities might be listening to
        // but you want to restrict which types of entities run the behavior.
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Error: The entity (%s) does not mix-in the proper model (%s) to be allowed "
            "to execute the requested behavior (%s).  Entity model id is (%s).",
            GetEntityID().ToString().c_str(),
            i_strMixinModelName.c_str(),
            i_strBehaviorName.c_str(),
            GetModelName().c_str()));
        return false;
    }

    if (IsBehaviorInvokeValid(pBehavior, m_pScheduler, true, false))
    {
        QueuePendingBehavior(pBehavior, spArgs, executionTime, eventID, returnID);
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool Entity::AddPendingBehavior(const BehaviorDescriptor* pBehavior,
    FlatModelID mixinModelID,
    ParameterList* spArgs,
    efd::TimeType executionTime /*= 0.0*/,
    EventID eventID /*= 0*/,
    EntityID returnID /*= 0*/)
{
    bool result = false;

    if (pBehavior == NULL)
    {
        // if here, error: didn't find that behavior in
        //  this entity's model's list of behaviors
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Error: NULL Behavior passed to AddPendingBehavior"));
    }
    else if ((mixinModelID != 0) && (!GetModel()->ContainsModel(mixinModelID)))
    {
        // if here, error: A mixin model ID was specified but the entity does not mix-in the
        // proper model to be allowed to execute this behavior
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Error: The entity (%s) does not mix-in the proper model (%d) to be allowed "
            "to execute the requested behavior (%d).  Entity model id is (%lu).",
            GetEntityID().ToString().c_str(),
            mixinModelID,
            pBehavior->GetID(),
            GetModel()->GetID()));
    }
    else if (IsBehaviorInvokeValid(pBehavior, m_pScheduler, true, false))
    {
        QueuePendingBehavior(pBehavior, spArgs, executionTime, eventID, returnID);
        result = true;
    }

    return result;
}

//------------------------------------------------------------------------------------------------
bool Entity::IsBehaviorInvokeValid(BehaviorID i_bid)
{
    const BehaviorDescriptor* pBehavior = GetModel()->GetBehaviorDescriptor(i_bid);
    if (pBehavior)
    {
        return IsBehaviorInvokeValid(pBehavior, m_pScheduler, false, false);
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool Entity::IsBehaviorInvokeValid(
    BehaviorID i_bid,
    Scheduler* psim,
    bool expectSuccess,
    bool isViewEvent)
{
    const BehaviorDescriptor* pBehavior = GetModel()->GetBehaviorDescriptor(i_bid);
    if (pBehavior)
    {
        return IsBehaviorInvokeValid(pBehavior, psim, expectSuccess, isViewEvent);
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool Entity::IsBehaviorInvokeValid(
    const BehaviorDescriptor* pBehavior,
    Scheduler* psim,
    bool expectSuccess,
    bool isViewEvent)
{
    // These conditions should be impossible and are repaired by SAXModelParser if they are found.
    //@{
    EE_ASSERT(BehaviorType_Invalid != pBehavior->GetType());
    EE_ASSERT(BehaviorType_Abstract != pBehavior->GetType());
    // If I'm virtual I must have a non-empty invocation list:
    EE_ASSERT(BehaviorType_Virtual != pBehavior->GetType() ||
        !pBehavior->GetInvocationOrderedModelNames().empty());
    //@}

    if (!psim)
    {
        // If I don't have a scheduler pointer that implies this entity has already been destroyed.
        if (expectSuccess)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR2,
                ("Behavior '%s' can not run on %s entity %s: no scheduler.",
                pBehavior->GetName().c_str(),
                GetModelName().c_str(),
                m_entityID.ToString().c_str()));
        }
        return false;
    }

    // now check behavior traits against the kind of executing/owned entity that we are...
    // NOTE: If a behavior is neither Private nor ViewOnly then it can run on either an owned
    // entity or a replication.  Previously Private trait did nothing and the default behavior
    // was to only allow running on the owned entity.
    if (pBehavior->GetTrait(BehaviorTrait_Private) && !IsOwned())
    {
        // Private behaviors can only run on the owned entity. We only expect private behaviors to
        // be sent using SendEvent, never as view behaviors, so we always expect this condition to
        // be true.
        if (expectSuccess)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR2,
                ("Behavior '%s' can not run on %s entity %s: private behavior invoked on replica.",
                pBehavior->GetName().c_str(),
                GetModelName().c_str(),
                m_entityID.ToString().c_str()));
        }
        return false;
    }
    if (pBehavior->GetTrait(BehaviorTrait_ViewOnly) && IsOwned())
    {
        // ViewOnly behaviors can only be executed on a replica. For view events this trait can be
        // used to limit when the behavior is invoked so only log this as an error for non-view
        // events.
        if (expectSuccess && !isViewEvent)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR2,
                ("Behavior '%s' can not run on %s entity %s: view-only behavior invoked on master.",
                pBehavior->GetName().c_str(),
                GetModelName().c_str(),
                m_entityID.ToString().c_str()));
        }
        return false;
    }
    if (pBehavior->GetTrait(BehaviorTrait_InWorldOnly) && !IsInWorld())
    {
        // Entity is not currently in the world and so is not allowed to execute this behavior.
        return false;
    }

    switch (pBehavior->GetType())
    {
    case BehaviorType_Remote:
        // Can't run a behavior that is of an invalid or non-executing type
        if (expectSuccess)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR2,
                ("Behavior '%s' can not run on %s entity %s: behavior is remote.",
                pBehavior->GetName().c_str(),
                GetModelName().c_str(),
                m_entityID.ToString().c_str()));
        }
        return false;

    case BehaviorType_Builtin:
        if (!FindBuiltinModel(pBehavior->GetModelName()))
        {
            // Trying to run a BuiltinModel behavior when we don't have that BuiltinModel.  Note
            // that we DO mixin the correct model, we checked that above. The model just isn't a
            // BuiltinModel for us. It might be a BuiltinModel on the remote side.
            if (expectSuccess)
            {
                EE_LOG(efd::kEntity, efd::ILogger::kERR2,
                    ("Behavior '%s' can not run on %s entity %s: built-in model '%s' not found.",
                    pBehavior->GetName().c_str(),
                    GetModelName().c_str(),
                    m_entityID.ToString().c_str(),
                    pBehavior->GetModelName().c_str()));
            }
            return false;
        }
        break;

    default:
        break;
    }

    bool result = true;
    if (Scheduler::IsScriptedBehaviorType(pBehavior->GetType()))
    {
#if defined(DISABLE_SCRIPTING)
        // scripted behaviors are not valid
        result = false;
#else
        result =
            psim->GetRuntimeStatus(pBehavior->GetType()) == ISchedulerScripting::rtstat_Ready;
#endif
        if (!result && expectSuccess)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR2,
                ("Behavior '%s' can not run on %s entity %s: script type '%s' not available.",
                pBehavior->GetName().c_str(),
                GetModelName().c_str(),
                m_entityID.ToString().c_str(),
                pBehavior->GetTypeName()));
        }
    }

    return result;
}

//------------------------------------------------------------------------------------------------
bool Entity::QueuePendingBehavior(
    const BehaviorDescriptor* pBehavior,
    ParameterList* spArgs,
    efd::TimeType executionTime,
    EventID eventID,
    EntityID returnID,
    efd::UInt32 lifeCycleAdvanceOnCompletion)
{
    if (!m_pScheduler)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Behavior '%s:%s' %s for model '%s' %s by %s failed because there is no scheduler.",
            pBehavior->GetModelName().c_str(),
            pBehavior->GetName().c_str(),
            eventID.ToString().c_str(),
            GetModelName().c_str(),
            GetEntityID().ToString().c_str(),
            returnID.ToString().c_str()));
        return false;
    }

    // NOTE: when destroy is mearly in progress is when we queue the OnDestroy behavior, but once
    // that finished it's game over and no more behaviors are valid.
    if (BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)kDestroyComplete))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Behavior '%s:%s' %s for model '%s' %s by %s ignored because the entity is destroyed.",
            pBehavior->GetModelName().c_str(),
            pBehavior->GetName().c_str(),
            eventID.ToString().c_str(),
            GetModelName().c_str(),
            GetEntityID().ToString().c_str(),
            returnID.ToString().c_str()));
        return false;
    }


    // These conditions should be impossible and are repaired by SAXModelParser if they are found.
    //@{
    EE_ASSERT(BehaviorType_Invalid != pBehavior->GetType());
    EE_ASSERT(BehaviorType_Abstract != pBehavior->GetType());
    // If I'm virtual I must have a non-empty invocation list:
    EE_ASSERT(BehaviorType_Virtual != pBehavior->GetType() ||
        !pBehavior->GetInvocationOrderedModelNames().empty());
    //@}

    PendingBehaviorPtr spBehaviorToRun =
        PendingBehavior::Create(pBehavior, this, spArgs, executionTime, eventID, returnID);
    if (EE_VERIFY(spBehaviorToRun))
    {
        if (lifeCycleAdvanceOnCompletion)
        {
            spBehaviorToRun->SetLifecycleToken(lifeCycleAdvanceOnCompletion);
        }

        InsertBehavior(spBehaviorToRun);

        EE_LOG(efd::kBehavior, efd::ILogger::kLVL3,
            ("Behavior '%s:%s' %s added to pending queue for model '%s' %s by %s",
            pBehavior->GetModelName().c_str(),
            pBehavior->GetName().c_str(),
            eventID.ToString().c_str(),
            GetModelName().c_str(),
            GetEntityID().ToString().c_str(),
            returnID.ToString().c_str()));

        return true;
    }
    return false;
}


//------------------------------------------------------------------------------------------------
void Entity::InsertBehavior(PendingBehavior* pPend)
{
    if (!m_pScheduler)
    {
        // If this entity is not already known by the scheduler then we generate an error message
        // and continue on. It is not valid to have a scheduler created after an entity.
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Attempt to queue a behavior on entity %s with no scheduler! Behavior ignored.",
            GetEntityID().ToString().c_str()));
        return;
    }

    ++m_pendingBehaviorCount;

    EE_ASSERT(m_pendingBehaviorCount >= 0);
    EE_ASSERT(m_pendingBehaviorCount >= (efd::SInt32)m_pendingBehaviorQueue.size());
    EE_ASSERT(m_pendingBehaviorCount < kMaxExpectedBehaviors);

    m_activeBehaviors.insert(pPend->GetEventID());

    if ((m_executingBehaviorCount < m_pScheduler->GetMaxThread()) || !pPend->CanBlock())
    {
        m_pScheduler->QueueTask(pPend);
        return;
    }

    // If this entity is not already known by the scheduler then we stick it into a local
    // pending queue.  This queue will get transfered to the scheduler when we are added.
    m_pendingBehaviorQueue.push(pPend);
}

//------------------------------------------------------------------------------------------------
bool Entity::IsBehaviorPending(EventID id) const
{
    bool behaviorPending = false;
    if (m_pScheduler)
    {
        behaviorPending = m_pScheduler->IsBehaviorPending(id);
    }
    if (!behaviorPending)
    {
        behaviorPending = (NULL != m_pendingBehaviorQueue.find(id));
    }
    return behaviorPending;
}

//------------------------------------------------------------------------------------------------
efd::Bool Entity::HasPendingBehaviors() const
{
    return m_pendingBehaviorCount > 0;
}

//------------------------------------------------------------------------------------------------
efd::Bool Entity::HasExecutingBehaviors() const
{
    return m_executingBehaviorCount > 0;
}

//------------------------------------------------------------------------------------------------
efd::Bool Entity::HasActiveBehaviors() const
{
    return HasPendingBehaviors() || HasExecutingBehaviors();
}

//------------------------------------------------------------------------------------------------
bool Entity::RemovePendingBehavior(const EventMessage* pMessage)
{
    return RemovePendingBehavior(pMessage->GetEventID());
}

//------------------------------------------------------------------------------------------------
bool Entity::RemovePendingBehavior(EventID eventID)
{
    bool eventRemoved = false;
    if (m_pScheduler)
    {
        // NOTE: Tasks removed from the Global Queue will have AbortTask called on them.
        eventRemoved = m_pScheduler->RemovePendingBehavior(eventID);
    }

    ScheduledTaskPtr spTask;
    if (m_pendingBehaviorQueue.erase(eventID, spTask))
    {
        spTask->AbortTask(m_pScheduler);
        eventRemoved = true;
    }

    if (!eventRemoved)
    {
        // if here, didn't find event on queue
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Attempt to remove %s from pending queue for %s model %s failed.  ID not found.",
            eventID.ToString().c_str(),
            GetEntityID().ToString().c_str(),
            GetModelName().c_str()));
    }
    return eventRemoved;
}

//------------------------------------------------------------------------------------------------
void Entity::SetOwned(bool ownerFlag)
{
    // set if this entity is owned by scheduler (allowed to set properties)
    m_flagBits = BitUtils::SetBitsOnOrOff(m_flagBits, (UInt32)kOwned, ownerFlag);
}

//------------------------------------------------------------------------------------------------
bool Entity::IsOwned() const
{
    // check that we have a scheduler owner OR are being initialized
    return BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)(kInit | kOwned));
}

//------------------------------------------------------------------------------------------------
void Entity::SetInit(bool initFlag)
{
    // set if this entity is being initialized (allowed to set read-only properties)
    m_flagBits = BitUtils::SetBitsOnOrOff(m_flagBits, (UInt32)kInit, initFlag);
}

//------------------------------------------------------------------------------------------------
efd::Bool Entity::IsCreated() const
{
    return BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)kCreated);
}

//------------------------------------------------------------------------------------------------
efd::Bool Entity::IsInWorld() const
{
    return BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)kInWorld);
}

//------------------------------------------------------------------------------------------------
bool Entity::IsDestroyInProgress() const
{
    // check to see if we are in the process of being destroyed
    return BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)(kRequestDestroy|kDestroyComplete));
}

//------------------------------------------------------------------------------------------------
#include "EventMessage.h"
/*static*/ Category Entity::MakePrivateCatID(const EntityID& entityID)
{
    return Category(entityID.GetValue());
}

//------------------------------------------------------------------------------------------------
Category Entity::GetPrivateCatID() const
{
    // EntityIDs are Categories, they implement a casting operator.
    return Category(GetEntityIDValue());
}

//------------------------------------------------------------------------------------------------
class PerformCallbackTask : public egf::ScheduledTask
{
public:
    PerformCallbackTask(Entity* pEntity, efd::utf8string strCallback)
        : egf::ScheduledTask(EventID::CreateEventID())
        , m_spEntity(pEntity)
        , m_strCallback(strCallback)
    {
    }

    virtual void DoTask(egf::Scheduler*)
    {
        EE_FAIL("Since we always abort ourself this should never get called.");
    }

    virtual bool SetResult(egf::Scheduler* pScheduler, const egf::EventMessage* pMessage)
    {
        if (!EE_VERIFY_MESSAGE(m_spEntity->GetServiceManager() != 0,
            ("An attempt was made to add an Entity behavior when the Service Manager was "
            "unavailable. Are trying to run behaviors during shutdown?")))
        {
            return false;
        }

        efd::TimeType execTime = pScheduler->GetGameTime() + pMessage->GetDelay();

        m_spEntity->AddPendingBehavior(
            m_strCallback,
            efd::utf8string::NullString(),
            pMessage->GetParameters(),
            execTime,
            0,
            pMessage->GetSenderID());

        // We don't actually need to Queue this task, inside AddPendingBehavior we will have
        // queued one or more other tasks to do the real work.  So return false here and we
        // will be aborted instead of queued.
        return false;
    }

protected:
    /// The entity on which to invoke the callback
    // DT32335 consider storing an EntityID instead, this entity might no longer be valid by the
    // time the response returns.
    EntityPtr m_spEntity;

    /// The callback behavior to invoke.  We store the callback as a string so that we can support
    /// "Model:Method" style callbacks.
    efd::utf8string m_strCallback;
};

//------------------------------------------------------------------------------------------------
EventID Entity::SendEvent(
    efd::Category categoryID,
    FlatModelID mixinModelID,
    FlatModelID invokeModelID,
    BehaviorID behaviorID,
    BehaviorID callbackBehaviorID /*= 0*/,
    efd::TimeType delay /*= 0.0f*/,
    ParameterList* pParams /*= NULL*/)
{
    if (!EE_VERIFY_MESSAGE(GetServiceManager() != 0,
        ("An attempt was made to send an Entity event when the Service Manager was "
        "unavailable. Are trying to send events during shutdown?")))
    {
        return false;
    }

    EventID retVal = 0;
    EventMessagePtr spEvent = EventMessage::CreateEvent(
        GetEntityID(),
        mixinModelID,
        invokeModelID,
        behaviorID,
        delay,
        pParams,
        (0 != callbackBehaviorID)); // If we have a callback then we need a reply

    if (spEvent)
    {
        // Grab the event ID so we can return it
        retVal = spEvent->GetEventID();

        // Grab a pointer to the net service so we can send the event
        MessageService* pMessageService = GetServiceManager()->GetSystemServiceAs<MessageService>();
        EE_ASSERT(pMessageService);

        pMessageService->Send(spEvent, categoryID, QOS_RELIABLE);

        FlatModelManager* pfmm = GetServiceManager()->GetSystemServiceAs<FlatModelManager>();
        utf8string behaviorName = pfmm->GetBehaviorNameByID(behaviorID);

        EE_LOG(efd::kBehavior, efd::ILogger::kLVL3,
            ("SendEvent() for entity %s, API sending to %s, mixinModelID='%u', "
            "behavior='%s', callback='%u', eventID=(0x%016llX)",
            GetEntityID().ToString().c_str(),
            categoryID.ToString().c_str(),
            mixinModelID,
            behaviorName.c_str(),
            callbackBehaviorID,
            retVal.GetValue()));

        // If we have a callback deal with it:
        if (callbackBehaviorID != 0)
        {
            const BehaviorDescriptor* pbd = GetModel()->GetBehaviorDescriptor(callbackBehaviorID);
            if (pbd)
            {
                // Create a task for the callback and stick it in the waiting queue
                ScheduledTaskPtr pTask = EE_NEW PerformCallbackTask(this, pbd->GetName());
                m_pScheduler->QueueTaskOnEvent(retVal, pTask);
            }
            else
            {
                // Failed to figure out the response behavior:
                EE_LOG(efd::kBehavior, efd::ILogger::kERR1,
                    ("SendEvent() failed to find callback '0x%08X' in model '%s'",
                    callbackBehaviorID, GetModelName().c_str()));
                return 0;
            }
        }
    }
    return retVal;
}

//------------------------------------------------------------------------------------------------
bool Entity::PostEvent(const EventMessage* pEventMessage)
{
    switch (pEventMessage->GetClassID())
    {
    case kMSGID_Event:
        return AddPendingBehavior(pEventMessage);
    case kMSGID_EventCancel:
        return RemovePendingBehavior(pEventMessage);
    case kMSGID_EventReturn:
        // These messages should now be handled completely by the Scheduler:
        EE_FAIL("Entity::PostEvent received a kMSGID_EventReturn message");
        break;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool Entity::SendReturnValue(egf::PendingBehavior* pBehavior, ParameterList* pRetVals) const
{
    if (EE_VERIFY(GetServiceManager()))
    {
        MessageServicePtr spMessageService =
            GetServiceManager()->GetSystemServiceAs<MessageService>();
        EE_ASSERT(spMessageService);

        EntityID returnID = pBehavior->GetReturnEntityID();
        EventID eventID = pBehavior->GetEventID();

        EE_LOG(efd::kEntity, ILogger::kLVL3,
            ("Return Value sent to %s for event %s by %s",
            returnID.ToString().c_str(),
            eventID.ToString().c_str(),
            GetEntityID().ToString().c_str()));

        // Send the event
        EventMessagePtr spEvent = EventMessage::CreateReturn(GetEntityID(), eventID, pRetVals);

        // DT32337 It would be nice if the pending event tracked whether a response was expected so
        // that we could generate an error message if this response isn't expected.

        spMessageService->Send(spEvent, returnID, QOS_RELIABLE);

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
void Entity::SetExecutor(Scheduler* pSim)
{
    // We cannot really change our executor once set because we might have tasks remaining on
    // the previous Executor's global queue.
    EE_ASSERT(!m_pScheduler || !pSim || (m_pScheduler == pSim));

    // set scheduler who is executing our behaviors
    m_pScheduler = pSim;

    if (m_pScheduler)
    {
        // Push our pending queue into the global scheduler queue:
        while (!m_pendingBehaviorQueue.empty())
        {
            ScheduledTask* pPendBehavior = m_pendingBehaviorQueue.top();
            m_pScheduler->QueueTask(pPendBehavior);
            m_pendingBehaviorQueue.pop();
        }
    }
}

//------------------------------------------------------------------------------------------------
void Entity::Destroy()
{
    EE_LOG(efd::kEntity, efd::ILogger::kLVL3, ("Entity::OnDestroy()"));
    if (BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)(kRequestDestroy|kDestroyComplete)))
    {
        // The OnDestroy lifecycle can only be invoked once, ignore any additional calls
        return;
    }

    // tick off owned entity destroyed by model ID
    EE_LOG_METRIC_COUNT_FMT(kEntity, ("DESTROY.OWNED.%s", GetModelName().c_str()));

    // If we happen to be in the world, we should exit the world before kicking off OnDestroy.
    // This call might do nothing depending on our current state, or it might run the OnExitWorld
    // lifecycle to completion if there is no behavior connected to that lifecycle.
    ExitWorld();

    // note that a OnDestroy lifecycle has been requested:
    m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kRequestDestroy);

    // Now we check whether an OnExitWorld lifecycle is still in progress, if one is in progress
    // then we wait until it completes before kicking off OnDestroy, otherwise we start that
    // lifecycle immediately:
    if (!BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)kRequestExitWorld))
    {
        _OnLifecycleEvent(lifecycle_OnDestroy);
    }
}

//------------------------------------------------------------------------------------------------
void Entity::EnterWorld()
{
    m_flagBits = BitUtils::ClearBits(m_flagBits, (UInt32)kRequestExitWorld);

    // If this Entity is already entering the world or destroyed do nothing:
    if (BitUtils::AnyBitsAreSet(
        m_flagBits,
        (UInt32)(kRequestEnterWorld|kEnterWorldInProgress|kRequestDestroy|kDestroyComplete)))
    {
        return;
    }

    // If I am in the world and not in the middle of exiting the world do nothing:
    if (BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)kInWorld) &&
        BitUtils::NoBitsAreSet(m_flagBits, (UInt32)kExitWorldInProgress))
    {
        return;
    }

    m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kRequestEnterWorld);

    // If we are finished with creation we can now immediately start entering the world. Unless
    // kExitWorldInProgress is set which means the OnExitWorld lifecycle is in progress. In that
    // case we need to wait for it to finish before we re-enter the world.
    if (BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)(kCreated))
        && BitUtils::NoBitsAreSet(m_flagBits, (UInt32)kExitWorldInProgress))
    {
        _OnLifecycleEvent(lifecycle_OnEnterWorld);
    }
}

//------------------------------------------------------------------------------------------------
void Entity::ExitWorld()
{
    m_flagBits = BitUtils::ClearBits(m_flagBits, (UInt32)kRequestEnterWorld);
    // If I'm in the world or in progress entering the world, start the exit process:
    if (BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)kInWorld|kEnterWorldInProgress))
    {
        // Of course if I'm already leaving the world there is nothing to do:
        if (BitUtils::NoBitsAreSet(m_flagBits, (UInt32)kRequestExitWorld))
        {
            m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kRequestExitWorld);
            // If any lifecycle is in progress we must wait until it completes before
            // running the ExitWorld lifecycle.
            if (BitUtils::NoBitsAreSet(
                m_flagBits,
                (UInt32)(kEnterWorldInProgress|kExitWorldInProgress)))
            {
                _OnLifecycleEvent(lifecycle_OnExitWorld);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
EventID Entity::SendEvent(
    efd::Category categoryId,
    const char* pszMixinModel,
    const char* pszBehavior,
    efd::ParameterList* pParams,
    const char* pszCallback,
    efd::TimeType delay)
{
    EE_ASSERT_MESSAGE(GetServiceManager() != 0,
        ("An attempt was made to send an event when the Service Manager was "
        "unavailable. Are trying to send events during shutdown?"));
    if (GetServiceManager() == 0)
    {
        return false;
    }

    EventID retVal = 0;
    EventMessagePtr spEvent = CreateEventMessage(
        GetEntityID(),
        pszMixinModel,
        pszBehavior,
        pParams,
        pszCallback,
        delay,
        false);

    if (spEvent && GetServiceManager())
    {
        retVal = spEvent->GetEventID();

        // Grab a pointer to the net service so we can send the event
        MessageServicePtr spMessageService =
            GetServiceManager()->GetSystemServiceAs<MessageService>();
        EE_ASSERT(spMessageService);

        spMessageService->Send(spEvent, categoryId, QOS_RELIABLE);

        if (pszCallback && *pszCallback)
        {
            // Create a task for the callback and stick it in the waiting queue
            ScheduledTaskPtr pTask = EE_NEW PerformCallbackTask(this, pszCallback);
            m_pScheduler->QueueTaskOnEvent(retVal, pTask);
        }

        EE_LOG(efd::kBehavior, efd::ILogger::kLVL3,
            ("SendEvent() for entity %s, API sending to %s, mixinModelID='%u', "
            "behavior='%s', callback='%s', eventID=(0x%016llX)",
            GetEntityID().ToString().c_str(),
            categoryId.ToString().c_str(),
            pszMixinModel,
            pszBehavior,
            pszCallback ? pszCallback : "",
            retVal.GetValue()));
    }

    return retVal;
}

//------------------------------------------------------------------------------------------------
EventMessagePtr Entity::CreateEventMessage(
    egf::EntityID entityID,
    const char* pszMixinModel,
    const char* pszBehavior,
    efd::ParameterList* pParams,
    const char* strCallback, /* if empty, no callback */
    efd::TimeType delay,
    bool needResponse)
{
    if (!entityID || !pszBehavior || !*pszBehavior)
    {
        // Invalid parameters
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("CreateEventMessage() invalid parameters"));
        return NULL;
    }

    if (!EE_VERIFY_MESSAGE(GetServiceManager() != 0,
        ("An attempt was made to send a create an event when the Service Manager was "
        "unavailable. Are trying to send events during shutdown?")))
    {
        return NULL;
    }

    FlatModelManager* pfmm = GetServiceManager()->GetSystemServiceAs<FlatModelManager>();
    if (!pfmm)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("CreateEventMessage() failed to find FlatModelManager"));
        return NULL;
    }

    FlatModelID idMixinModel = 0;
    FlatModelID idInvokeModel = 0;

    if (pszMixinModel && *pszMixinModel)
    {
        idMixinModel = pfmm->GetModelIDByName(pszMixinModel);
        if (0 == idMixinModel)
        {
            // We might be asked to send an event to a model we've never heard of before. In that
            // case try to use the deprecated JIT load, but this will fail if background loading is
            // being used and the model isn't preloaded.
            const FlatModel* pModel = pfmm->FindOrLoadModel(pszMixinModel);
            if (!pModel)
            {
                EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
                    ("CreateEventMessage() failed to find mix-in model '%s'. If using background "
                    "loading this model (or any derived model) needs to be preloaded before "
                    "calling SendEvent. Note: mix-in model filters are normally not needed, "
                    "consider passing NULL or \"\" for the model name instead.",
                    pszMixinModel));
                return NULL;
            }
            idMixinModel = pModel->GetID();
        }
    }

    utf8string strInvokingModel;
    utf8string strBehaviorMethod;
    if (FlatModel::SplitModelAndBehavior(pszBehavior, strInvokingModel, strBehaviorMethod))
    {
        // For this case we need the target model loaded in order for the GetBehaviorIDByName to
        // have the maximum chance of working. If the model is not preloaded and background
        // loading is enabled this will always fail.
        const FlatModel* pModel = pfmm->FindOrLoadModel(strInvokingModel);
        if (!pModel)
        {
            // Might as well give this a try anyway, hopefully we know the behavior ID from the
            // models that have been loaded.
            idInvokeModel = pfmm->GetModelIDByName(strInvokingModel);
            if (0 == idInvokeModel)
            {
                EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
                    ("CreateEventMessage() failed to find invoke model '%s'. This model should "
                    "be preloaded prior to calling SendEvent.",
                    strInvokingModel.c_str()));
                return NULL;
            }
        }
        else
        {
            idInvokeModel = pModel->GetID();
        }
    }

    // Note: strBehaviorMethod is always set by SplitModelAndBehavior, even when it returns false
    BehaviorID idBehavior = pfmm->GetBehaviorIDByName(strBehaviorMethod);
    if (!idBehavior)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
            ("CreateEventMessage() failed to find behaviorID of '%s' in model %s",
            strBehaviorMethod.c_str(),
            strInvokingModel.c_str()));
        return NULL;
    }

    EventMessagePtr spEvent = EventMessage::CreateEvent(
        entityID,
        idMixinModel,
        idInvokeModel,
        idBehavior,
        delay,
        pParams,
        needResponse || (strCallback && *strCallback));

    return spEvent;
}

//------------------------------------------------------------------------------------------------
IBuiltinModel* Entity::FindBuiltinModel(const efd::utf8string& i_strBuiltinModelName) const
{
    const BuiltinModelDescriptor* pComp =
        GetModel()->GetBuiltinModelDescriptor(i_strBuiltinModelName);
    if (pComp)
    {
        FlatModelID id = pComp->GetID();
        return FindBuiltinModel(id);
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
IBuiltinModel* Entity::FindBuiltinModelByClass(efd::ClassID i_classId) const
{
    BuiltModelMap::const_iterator iter = m_BuiltinModels.begin();

    while (iter != m_BuiltinModels.end())
    {
        egf::IBuiltinModelPtr spBuiltin = iter->second;
        if (spBuiltin->GetClassID() == i_classId)
            return spBuiltin;

        if (efd::ClassInfo::IsKindOf(spBuiltin->GetClassDesc(),i_classId))
            return spBuiltin;

        iter++;
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
bool Entity::AddBuiltinModel(FlatModelID i_BuiltinModelId, IBuiltinModel* i_pNewComp)
{
    if (i_BuiltinModelId && i_pNewComp)
    {
        BuiltModelMap::iterator iter = m_BuiltinModels.find(i_BuiltinModelId);
        if (iter == m_BuiltinModels.end())
        {
            // Good, we are not already in the list!
            m_BuiltinModels[i_BuiltinModelId] = i_pNewComp;
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool Entity::_OnLifecycleEvent(efd::UInt32 lifecycle, ParameterList* pParameterList)
{
    bool result = false;

    BeginLifecycle(lifecycle);

    // queue lifecycle event's behavior for immediate execution
    utf8string strEventName = GetLifecycleName(lifecycle);
    const BehaviorDescriptor* pBehavior = GetModel()->GetBehaviorDescriptor(strEventName);
    if (pBehavior != 0)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
            ("Entity::_OnLifecycleEvent(%s) - found behavior", strEventName.c_str()));

        result = QueuePendingBehavior(pBehavior, pParameterList, 0.0f, 0, 0, lifecycle);
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kLVL3,
            ("Entity::_OnLifecycleEvent(%s) - behavior not found", strEventName.c_str()));
    }

    if (!result)
    {
        // No behavior for this lifecycle, so immediate end it:
        EndLifecycle(lifecycle);
    }

    return result;
}

//------------------------------------------------------------------------------------------------
bool Entity::CreateBuiltinModels(FlatModelManager* i_pCompFactory)
{
    EE_ASSERT(NULL != GetModel());

    bool result = true;

    BuiltinModelList comps;
    GetModel()->GetRequiredBuiltinModelsList(comps);
    for (BuiltinModelList::iterator iter = comps.begin();
        iter != comps.end();
        ++iter)
    {
        BuiltinModelDescriptor* pcd = *iter;
        FlatModelID id = pcd->GetID();

        // If we're a master copy entity and the BuiltinModel is a "kBuiltinModel" add it or if
        // we're NOT a master copy (we're a reflected entity) and the BuiltinModel is a
        // "kReplicaBuiltinModel" then add the BuiltinModel
        if ((IsOwned() && pcd->GetTrait(ModelTrait_BuiltinModel) ||
            (!IsOwned() && pcd->GetTrait(ModelTrait_ReplicaBuiltinModel))))
        {
            IBuiltinModelPtr pComp = i_pCompFactory->FactoryBuiltinModel(id);
            if (pComp)
            {
                PropertyDescriptorList defaults;
                GetModel()->PrepareDefaultPropertyList(pcd, defaults);
                if (pComp->Initialize(this, defaults))
                {
                    AddBuiltinModel(id, pComp);
                }
                else
                {
                    EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                        ("Failed to initialize BuiltinModel '%s' "
                        "from model '%s' when creating entity %s",
                        pcd->GetName().c_str(), GetModelName().c_str(),
                        GetEntityID().ToString().c_str()));
                    result = false;
                }
            }
            else
            {
                EE_FAIL_MESSAGE(
                    ("Failed to create BuiltinModel '%s' from model '%s' when creating entity %s",
                    pcd->GetName().c_str(), GetModelName().c_str(),
                    GetEntityID().ToString().c_str()));
                EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                    ("Failed to create BuiltinModel '%s' from model '%s' when creating entity %s",
                    pcd->GetName().c_str(), GetModelName().c_str(),
                    GetEntityID().ToString().c_str()));
                result = false;
            }
        }
    }

    return result;
}

//------------------------------------------------------------------------------------------------
void Entity::OnAdded(ParameterList* pParameterList)
{
    EE_ASSERT_MESSAGE(!IsDestroyInProgress(),("Destroyed entities cannot be added back to the"
        "EntityManager. Consider using Enter/ExitWorld instead."));
    // We were just added to the EntityManager following our initial creation.

    // Notify all our BuiltinModel objects.
    for (BuiltModelMap::iterator iter = m_BuiltinModels.begin();
        iter != m_BuiltinModels.end();
        ++iter)
    {
        IBuiltinModel* pComp = iter->second;
        pComp->OnAdded();
    }

    // If there is a scheduler, set myself up to be able to run behaviors.
    Scheduler* pScheduler = m_pEntityManager->GetServiceManager()->GetSystemServiceAs<Scheduler>();
    if (pScheduler)
    {
        SetExecutor(pScheduler);
    }

    // We don't want replicated or non-master entities kicking off OnCreate events.
    if (IsOwned())
    {
        if (pScheduler)
        {
            // Only owned entities subscribe to their own private Category
            pScheduler->SubscribeEntity(this, GetPrivateCatID());
        }
        // NOTE: If I have no OnCreate behavior the following call will immediately trigger
        // EntityManager::OnEntityEndLifecycle which in turn will send out the local entity
        // creation notification.  So be sure to perform all important setup work prior to
        // triggering the OnCreate lifecycle event.
        _OnLifecycleEvent(lifecycle_OnCreate, pParameterList);
    }
}

//------------------------------------------------------------------------------------------------
void Entity::OnReinitialized()
{
  // Notify all our BuiltinModel objects.
    for (BuiltModelMap::iterator iter = m_BuiltinModels.begin();
        iter != m_BuiltinModels.end();
        ++iter)
    {
        IBuiltinModel* pComp = iter->second;
        pComp->OnOwningEntityReinitialized(this);
    }
}

//------------------------------------------------------------------------------------------------
void Entity::OnAssetsLoaded()
{
    _OnLifecycleEvent(lifecycle_OnAssetsLoaded);
}

//------------------------------------------------------------------------------------------------
void Entity::OnEntitySetFinished()
{
    if (IsOwned())
    {
        _OnLifecycleEvent(lifecycle_OnEntitySetFinished);
    }
    else
    {
        m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kEntitySetFishished);
    }
}

//------------------------------------------------------------------------------------------------
void Entity::OnRemoved()
{
    // We were just removed from the EntityManager for the purpose of being deleted, but we
    // haven't been deleted just yet.  We no longer have a pointer back to our container and we
    // are no longer registered for any sorts of message handlers or what-not.  We should no
    // longer have any replication groups set either.

    // Since we are being removed we should cancel any pending behaviors that were on us.
    // Otherwise we'll get a memory leak since these hold a reference back to the entity.
    while (!m_pendingBehaviorQueue.empty())
    {
        ScheduledTask* pPendBehavior = m_pendingBehaviorQueue.top();
        pPendBehavior->AbortTask(m_pScheduler);
        m_pendingBehaviorQueue.pop();
    }

    // Any events for us in the global queue
    if (m_pScheduler)
    {
        CancelActiveBehaviors();

        m_pScheduler->UnsubscribeEntity(this, GetPrivateCatID());
        SetExecutor(NULL);
    }

    // Clean up our BuiltinModel objects.  They all have pointers back to us and are likely
    // registered with other services.  Since we are about to be deleted that means they are about
    // to be deleted too so they should let any interested services know.
    for (BuiltModelMap::iterator iter = m_BuiltinModels.begin();
        iter != m_BuiltinModels.end();
        ++iter)
    {
        IBuiltinModel* pComp = iter->second;
        pComp->OnRemoved();
    }
}

//------------------------------------------------------------------------------------------------
void Entity::HandleMessage(const EventMessage *pEventMessage, efd::Category targetChannel)
{
    if (m_pScheduler)
    {
        m_pScheduler->ProcessEventMessage(this, pEventMessage, false);
    }
    else
    {
        EE_LOG(efd::kMessageTrace, efd::ILogger::kERR1,
            ("%s| Delivery failed, no scheduler in entity %s",
            pEventMessage->GetDescription().c_str(), GetEntityID().ToString().c_str()));
    }
}

//------------------------------------------------------------------------------------------------
bool Entity::StartBehavior(PendingBehavior* pBehavior)
{
    if (!m_pScheduler)
    {
        // This entity was removed from the scheduler between when this task was queued and when
        // it was started.  In this event we want to simply ingore this behavior.
        return false;
    }

    if ((m_executingBehaviorCount < m_pScheduler->GetMaxThread()) || !pBehavior->CanBlock())
    {
        // We are about to remove a pending behavior and run it, so update our count:
        EE_ASSERT(m_pendingBehaviorCount > 0);
        EE_ASSERT(m_pendingBehaviorCount > (efd::SInt32)m_pendingBehaviorQueue.size());
        EE_ASSERT(m_pendingBehaviorCount < kMaxExpectedBehaviors);
        --m_pendingBehaviorCount;

        // Of course this means the behavior now counts as executing:
        EE_ASSERT(m_executingBehaviorCount >= 0);
        EE_ASSERT(m_executingBehaviorCount < kMaxExpectedBehaviors);
        ++m_executingBehaviorCount;

        return true;
    }
    else
    {
        // Too many blocking behaviors, can't run more.  We push this event into a blocked
        // queue on the entity which will get pushed back into the global queue once the entity
        // finishes a behavior.
        InsertBehavior(pBehavior);
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void Entity::FinishEvent(PendingBehavior* pBehavior, bool success)
{
    StopBehaviorTimer(pBehavior);

    // this behavior is no longer active, so try to erase it, but don't worry if we have already
    // been removed from the m_activeBehaviors set since certain cases of aborting behaviors
    // will clear up that information before FinishEvent is called (Entity::CancelActiveBehaviors)
    m_activeBehaviors.erase(pBehavior->GetEventID());

    if (pBehavior->Started())
    {
        EE_ASSERT(m_executingBehaviorCount > 0);
        EE_ASSERT(m_executingBehaviorCount <= kMaxExpectedBehaviors);
        --m_executingBehaviorCount;
    }

    // m_pScheduler can be NULL if this event was aborted due to entity shutdown.
    if (m_pScheduler)
    {
        // If we have behaviors in our local pending queue when a behavior finishes, lets see
        // if this has freed enough room to push some of these to the global queue.
        for (int cBehaviorRoom = m_pScheduler->GetMaxThread() - m_executingBehaviorCount;
              !m_pendingBehaviorQueue.empty() && cBehaviorRoom > 0;
              --cBehaviorRoom)
        {
            // It looks like there's room, lets pop our top and push that to the global queue
            PendingBehavior* pBlockedBehavior = (PendingBehavior*)m_pendingBehaviorQueue.top();

            // When we use this because we have too many blocked behaviors only behaviors that
            // can block should be able to end up in this list.
            EE_ASSERT(pBlockedBehavior->CanBlock());

            m_pScheduler->QueueTask(pBlockedBehavior);

            m_pendingBehaviorQueue.pop();
        }
    }

    if (success)
    {
        // Tick off another behavior removed by name...
        EE_LOG_METRIC_COUNT_FMT(kBehavior, ("COMPLETE.%s", GetModelName().c_str()));

#ifdef EE_USE_BEHAVIOR_TIMING_METRICS
        // Log the total time (including waits for sleeps and reply blocks)
        METRICS_ONLY(TimeType now = efd::GetCurrentTimeInSec());
        EE_LOG_METRIC_FMT(kBehavior, ("RUN.SEC.%s", GetModelName().c_str()),
            (now - pBehavior->m_startTime));

        // Log the execution time (excluding blocking waits) accumulated for this behavior
        EE_LOG_METRIC_FMT(kBehavior, ("EXECUTE.SEC.%s", GetModelName().c_str()),
            pBehavior->m_accumulatedTime);
#endif
    }
    else
    {
        // increment another behavior removed before completion
        EE_LOG_METRIC_COUNT_FMT(kBehavior, ("REMOVE.%s", GetModelName().c_str()));
    }

    // DT32337 If this event expected a reply but one was not sent then we should indicate that
    // error here and generate an empty response to unblock the caller.

    if (pBehavior->GetLifecycleToken())
    {
        EndLifecycle(pBehavior->GetLifecycleToken());
    }
}

//------------------------------------------------------------------------------------------------
void Entity::BeginLifecycle(efd::UInt32 lifecycle)
{
    EE_ASSERT(m_pEntityManager);

    switch (lifecycle)
    {
    case lifecycle_OnEnterWorld:
        m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kEnterWorldInProgress);
        break;
    case lifecycle_OnExitWorld:
        m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kExitWorldInProgress);
        break;
    }
    m_pEntityManager->OnEntityBeginLifecycle(this, lifecycle);
}

//------------------------------------------------------------------------------------------------
Entity::LifeCycles Entity::ProcessEndLifecycle(efd::UInt32 lifecycle)
{
    LifeCycles lifecycleToRun = lifecycle_Invalid;
    switch (lifecycle)
    {
    case lifecycle_OnCreate:
        break;

    case lifecycle_OnAssetsLoaded:
        m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kAssetsLoaded);
        m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kCreated);
        // if destroy has been requested, don't attempt to run any other behaviors
        if (BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)(kRequestDestroy)))
        {
            lifecycleToRun = lifecycle_OnDestroy;
        }
        // If Enter world was requested before we were done being created, enter now.
        else if (BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)(kRequestEnterWorld)))
        {
            lifecycleToRun = lifecycle_OnEnterWorld;
        }
        else if (!GetDataFileID().IsValid())
        {
            // If not part of a set, call OnEntitySetFinished() after sending out the creation
            // notification.  If I am part of a set, the EntityLoaderService will call this
            // instead.
            lifecycleToRun = lifecycle_OnEntitySetFinished;
        }
        break;

    case lifecycle_OnEntitySetFinished:
        m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kEntitySetFishished);
        break;

    case lifecycle_OnEnterWorld:
        m_flagBits = BitUtils::ClearBits(m_flagBits, (UInt32)kRequestEnterWorld);
        m_flagBits = BitUtils::ClearBits(m_flagBits, (UInt32)kEnterWorldInProgress);
        m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kInWorld);

        // If exit was requested before we entered the world, exit now.
        if (BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)(kRequestExitWorld)))
        {
            lifecycleToRun = lifecycle_OnExitWorld;
        }
        else if (!GetDataFileID().IsValid() &&
            BitUtils::NoBitsAreSet(m_flagBits, (UInt32)(kEntitySetFishished)))
        {
            // If not part of a set, call OnEntitySetFinished() after sending out the creation
            // notification.  If I am part of a set, the EntityLoaderService will call this
            // instead.
            lifecycleToRun = lifecycle_OnEntitySetFinished;
        }

        break;

    case lifecycle_OnExitWorld:
        m_flagBits = BitUtils::ClearBits(m_flagBits, (UInt32)kRequestExitWorld);
        m_flagBits = BitUtils::ClearBits(m_flagBits, (UInt32)kExitWorldInProgress);
        m_flagBits = BitUtils::ClearBits(m_flagBits, (UInt32)kInWorld);

        // If Destroy was requested before we exited the world, destroy now.
        if (BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)(kRequestDestroy)))
        {
            lifecycleToRun = lifecycle_OnDestroy;
        }
        else if (BitUtils::AnyBitsAreSet(m_flagBits, (UInt32)(kRequestEnterWorld)))
        {
            lifecycleToRun = lifecycle_OnEnterWorld;
        }
        break;

    case lifecycle_OnDestroy:
        if (m_pScheduler)
        {
            // set m_pEntityManager to NULL so if EndLifecycle re-enters it does nothing
            EntityManager* pEntityManager = m_pEntityManager;
            m_pEntityManager = NULL;
            CancelActiveBehaviors();
            m_pEntityManager = pEntityManager;
        }
        m_flagBits = BitUtils::SetBits(m_flagBits, (UInt32)kDestroyComplete);
        break;

    default:
        // do nothing
        break;
    }
    return lifecycleToRun;
}

//------------------------------------------------------------------------------------------------
void Entity::EndLifecycle(efd::UInt32 lifecycle)
{
    // CancelActiveBehaviors can cause EndLifecycle(lifecycle_OnDestroy) to be called if there
    // are multiple OnDestroy behaviors queued so we need to make sure if
    // EndLifecycle(lifecycle_OnDestroy) is called a second time it does nothing
    if (m_pEntityManager)
    {
        LifeCycles lifecycleToRun = ProcessEndLifecycle(lifecycle);

        // Notify all our BuiltinModel objects.
        for (BuiltModelMap::iterator iter = m_BuiltinModels.begin();
            iter != m_BuiltinModels.end();
            ++iter)
        {
            IBuiltinModel* pComp = iter->second;
            pComp->OnEndLifecycle(lifecycle);
        }
        m_pEntityManager->OnEntityEndLifecycle(this, lifecycle);
        if (lifecycleToRun != lifecycle_Invalid)
        {
            switch (lifecycleToRun)
            {
            case lifecycle_OnEntitySetFinished:
                OnEntitySetFinished();
                break;
            default:
                _OnLifecycleEvent(lifecycleToRun);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
const char* Entity::GetLifecycleName(efd::UInt32 lifecycle)
{
    switch (lifecycle)
    {
    case lifecycle_OnCreate:
        return "OnCreate";

    case lifecycle_OnAssetsLoaded:
        return "OnAssetsLoaded";

    case lifecycle_OnEntitySetFinished:
        return "OnEntitySetFinished";

    case lifecycle_OnEnterWorld:
        return "OnEnterWorld";

    case lifecycle_OnExitWorld:
        return "OnExitWorld";

    case lifecycle_OnDestroy:
        return "OnDestroy";
    }

    return "";
}

//------------------------------------------------------------------------------------------------
void Entity::CancelActiveBehaviors()
{
    if (m_pScheduler)
    {
        // Canceling an active behavior can sometimes result in queuing new behaviors, but since
        // we are trying to clear out all behaviors we want to avoid that.  Clearing the scheduler
        // pointer will have that effect, but we need to restore the pointer when we're done so
        // that other entity cleanup steps have access to it.
        Scheduler* pScheduler = m_pScheduler;
        m_pScheduler = NULL;

        // Calling Scheduler::RemovePendingBehavior will abort the pending behavior which will
        // eventually call into Entity::FinishBehavior which then attempts to remove the EventID
        // from the m_activeBehaviors table.  Its also possible that new behaviors might get added
        // to m_activeBehaviors during the removal process (although we try to prevent that by
        // setting m_pScheduler to NULL above). This means we need to be careful how we remove
        // all the items from this set.
        while (!m_activeBehaviors.empty())
        {
            ActiveBehaviorSet::iterator iter = m_activeBehaviors.begin();
            EventID nextID = *iter;
            m_activeBehaviors.erase(iter);
            pScheduler->RemovePendingBehavior(nextID);
        }

        m_pScheduler = pScheduler;
    }
}

//------------------------------------------------------------------------------------------------
void Entity::LeakDump(void* pMem, char* o_buffer, unsigned int i_cchBuffer)
{
    Entity* pEntity = reinterpret_cast<Entity*>(pMem);

    efd::Snprintf(o_buffer, i_cchBuffer, EE_TRUNCATE,
        "Entity<%s '%s'>",
        pEntity ? pEntity->GetEntityID().ToString().c_str() : "NoEntity",
        pEntity ? pEntity->GetModelName().c_str() : "n/a");
}

//------------------------------------------------------------------------------------------------
void Entity::SetEntityCreateCallbackBehavior(
    const egf::EntityID& entityId,
    const efd::utf8string& behavior)
{
    EE_ASSERT(entityId != kENTITY_INVALID);
    EE_ASSERT(!behavior.empty());

    m_pendingEntityCreateBehaviors[entityId] = behavior;
}

//------------------------------------------------------------------------------------------------
void Entity::HandleEntityFactoryResponse(const EntityFactoryResponse* pMsg, efd::Category)
{
    EE_ASSERT(pMsg);
    EntityID id = pMsg->GetEntityID();
    EE_ASSERT(id != kENTITY_INVALID);

    PendingEntityCreateBehaviorMap::iterator it = m_pendingEntityCreateBehaviors.find(id);
    if (it != m_pendingEntityCreateBehaviors.end())
    {
        EE_ASSERT(!it->second.empty());

        ParameterListPtr pDS = EE_NEW ParameterList();
        pDS->AddParameter("Entity", id);
        pDS->AddParameter("Result", (UInt32)pMsg->GetResult());
        SendEvent(GetEntityID(), GetModelName().c_str(), it->second.c_str(), pDS);
        m_pendingEntityCreateBehaviors.erase(it);
    }
}

//------------------------------------------------------------------------------------------------
PropertyResult Entity::SetPropertyValueFromArchive(
    PropertyID propID,
    efd::ClassID typeOfDataInBuffer,
    efd::Archive& io_ar)
{
    if (io_ar.GetError() || io_ar.IsPacking())
    {
        return PropertyResult_UnknownError;
    }

    // validate the type safety
    const PropertyDescriptor* pDesc = NULL;
    PropertyResult result = ValidateTypeSafety(propID, typeOfDataInBuffer, pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    result = CheckWritability(pDesc);
    if (result != PropertyResult_OK)
    {
        return result;
    }

    IProperty* pProp = GetPropertyForWriting(pDesc);

    // assign the value to the property
    pProp->SerializeProperty(propID, io_ar);

    // set the property dirty
    if (!io_ar.GetError())
    {
        SetDirty(propID, pProp);
    }
    else
    {
        result = PropertyResult_UnknownError;
    }

    return result;
}

//------------------------------------------------------------------------------------------------
bool Entity::ApplyProperties(ParameterList* pProperties)
{
    EE_ASSERT(pProperties);
    PropertyResult result = PropertyResult_UnknownError;

    for (UInt32 i = 0; i < pProperties->GetSize(); ++i)
    {
        const char* name = pProperties->GetParameterName(i);
        if (name)
        {
            const PropertyDescriptor* ppd = m_pFlatModel->GetPropertyDescriptor(name);
            if (ppd)
            {
                PropertyID propID = ppd->GetPropertyID();
                efd::ClassID type = pProperties->GetParameterDataType(i);
                efd::Archive ar(Archive::Unpacking, pProperties->GetParameterStorage(i));
                result = SetPropertyValueFromArchive(propID, type, ar);
                if (PropertyResult_OK != result)
                {
                    EE_LOG(efd::kEntity, efd::ILogger::kERR2,
                        ("SetPropertyValueFromArchive failed with result %d. Property '%s' on "
                        "entity %s of model '%s'",
                        result,
                        name,
                        m_entityID.ToString().c_str(),
                        GetModelName().c_str()));
                }
                else if (!ar.CheckForUnderflow())
                {
                    // Not all of the data was used, which is unexpected in this case since our
                    // storage window should have been the exact size of the data.
                    EE_LOG(efd::kEntity, efd::ILogger::kERR2,
                        ("ApplyProperties:SetPropertyValueFromArchive succeeded but did not "
                        "consume all data in the Archive! The unserialized data may be corrupt. "
                        "Property '%s' on entity %s of model '%s'",
                        name,
                        m_entityID.ToString().c_str(),
                        GetModelName().c_str()));
                    result = PropertyResult_SerializationError;
                }
            }
            else
            {
                // The property was not found
                EE_LOG(efd::kEntity, efd::ILogger::kERR2,
                    ("ApplyProperties: Property '%s' not found on entity %s of model '%s'",
                    name,
                    m_entityID.ToString().c_str(),
                    GetModelName().c_str()));
                result = PropertyResult_PropertyNotFound;
            }
        }
        else
        {
            // LOG: Must use named parameters with this method
            EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                ("ApplyProperties: ParameterList contains unnamed parameter %d. You must use "
                "named parameters where the names match property names."));
            result = PropertyResult_SerializationError;
        }
    }
    return (PropertyResult_OK == result);
}

//------------------------------------------------------------------------------------------------
