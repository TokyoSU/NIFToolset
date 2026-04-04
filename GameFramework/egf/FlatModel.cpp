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

#include <egf/FlatModel.h>
#include <egf/FlatModelManager.h>
#include <efd/ServiceManager.h>
#include <efd/ILogger.h>
#include <efd/BitUtils.h>
#include <egf/egfLogIDs.h>

using namespace efd;
using namespace egf;

//--------------------------------------------------------------------------------------------------
FlatModel::FlatModel(FlatModelManager* pFlatModelManager)
: m_flatModelID(0)
, m_traits(0)
, m_activeReplicationGroups(0)
,m_pFlatModelManager(pFlatModelManager)
{
}


//--------------------------------------------------------------------------------------------------
FlatModel::~FlatModel()
{
}


//--------------------------------------------------------------------------------------------------
const efd::utf8string& FlatModel::GetName() const
{
    return m_name;
}


//--------------------------------------------------------------------------------------------------
void FlatModel::SetName(const efd::utf8string& name)
{
    // You should only ever set the name once
    EE_ASSERT(m_name.empty());

    m_name = name;
}

//--------------------------------------------------------------------------------------------------
FlatModelID FlatModel::GetID() const
{
    return m_flatModelID;
}


//--------------------------------------------------------------------------------------------------
void FlatModel::SetID(FlatModelID id)
{
    // You should only ever set the ID once
    EE_ASSERT(0 == m_flatModelID);

    m_flatModelID = id;

    // We should always contain our own model id so we add it to the mixin's list
    AddMixinModel(m_flatModelID);
}


//--------------------------------------------------------------------------------------------------
efd::Bool FlatModel::GetTrait(ModelTraits trait) const
{
    return BitUtils::AllBitsAreSet(m_traits, (UInt32)trait);
}


//--------------------------------------------------------------------------------------------------
efd::UInt32 FlatModel::GetTraits() const
{
    return m_traits;
}


//--------------------------------------------------------------------------------------------------
void FlatModel::SetTrait(ModelTraits trait, efd::Bool val)
{
    m_traits = BitUtils::SetBitsOnOrOff(m_traits, (UInt32)trait, val);
}


//--------------------------------------------------------------------------------------------------
const PropertyDescriptor* FlatModel::GetPropertyDescriptor(PropertyID propID) const
{
    PropertyDescriptorMapByID::const_iterator iterValues = m_propertyDescriptorsByID.find(propID);
    if (iterValues != m_propertyDescriptorsByID.end())
    {
        return iterValues->second;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
const PropertyDescriptor* FlatModel::GetPropertyDescriptor(const efd::utf8string& name) const
{
    PropertyDescriptorMapByName::const_iterator iterValues = m_propertyDescriptorsByName.find(name);
    if (iterValues != m_propertyDescriptorsByName.end())
    {
        return iterValues->second;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
bool FlatModel::ContainsProperty(PropertyID propID) const
{
    return NULL != GetPropertyDescriptor(propID);
}


//--------------------------------------------------------------------------------------------------
bool FlatModel::ContainsProperty(const efd::utf8string& name) const
{
    return NULL != GetPropertyDescriptor(name);
}


//--------------------------------------------------------------------------------------------------
bool FlatModel::AddPropertyDescriptor(PropertyDescriptor* i_pDescriptor)
{
    // make sure the source descriptor has a default property.  not having a
    // default property is an error
    if (i_pDescriptor->GetDefaultProperty() == NULL)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Tried to add a property descriptor: %d (%s) to flat model %d (%s), "
            "but the descriptor's default value was null",
            i_pDescriptor->GetPropertyID(),
            i_pDescriptor->GetName().c_str(),
            m_flatModelID,
            m_name.c_str()));

        return false;
    }

    // Override any existing value.  This is needed to update default values when doing run-time
    // model flattening.
    m_propertyDescriptorsByID[i_pDescriptor->GetPropertyID()] = i_pDescriptor;
    m_propertyDescriptorsByName[i_pDescriptor->GetName()] = i_pDescriptor;

    m_activeReplicationGroups |= i_pDescriptor->GetReplicationGroups();

    return true;
}


//--------------------------------------------------------------------------------------------------
void FlatModel::GetPropertyNames(efd::list<efd::utf8string>& o_listOfNames) const
{
    o_listOfNames.clear();

    for (PropertyDescriptorMapByName::const_iterator iterValues = m_propertyDescriptorsByName.begin();
          iterValues != m_propertyDescriptorsByName.end();
          ++iterValues)
    {
        o_listOfNames.push_back(iterValues->first);
    }
}

//--------------------------------------------------------------------------------------------------
void FlatModel::DiffProperties(
    const egf::FlatModel* otherModel,
    efd::set<PropertyDescriptorPtr>& unchangedProperties,
    efd::set<PropertyDescriptorPtr>& newProperties,
    efd::set<PropertyDescriptorPtr>& updatedProperties,
    efd::set<PropertyDescriptorPtr>& deletedProperties) const
{
    PropertyDescriptorMapByID::const_iterator it = otherModel->m_propertyDescriptorsByID.begin();
    for (; it != otherModel->m_propertyDescriptorsByID.end(); ++it)
    {
        const PropertyDescriptorPtr newDesc = it->second;
        PropertyDescriptorMapByID::const_iterator oldDescIt =
            m_propertyDescriptorsByID.find(it->first);

        if (oldDescIt == m_propertyDescriptorsByID.end())
        {
            // brand new property
            newProperties.insert(newDesc);
        }
        else if ((*(oldDescIt->second)) == (*newDesc))
        {
            // exact same property
            unchangedProperties.insert(newDesc);
        }
        else
        {
            // updated property
            updatedProperties.insert(newDesc);
        }
    }
    // calculate deleted properties
    it = m_propertyDescriptorsByID.begin();
    for (; it != m_propertyDescriptorsByID.end(); ++it)
    {
        if (otherModel->m_propertyDescriptorsByID.find(it->first) ==
            otherModel->m_propertyDescriptorsByID.end())
        {
            // set the property descriptor to the old property for deletes.
            deletedProperties.insert(it->second);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void FlatModel::DiffMixins(const egf::FlatModel* pOtherModel,
                           efd::set<efd::utf8string>& addedMixins,
                           efd::set<efd::utf8string>& deletedMixins) const
{
    list<utf8string> my_mixins, other_mixins;
    list<utf8string>::const_iterator iter;

    // Get mixins used in each model
    GetMixinNames(my_mixins);
    pOtherModel->GetMixinNames(other_mixins);

    // Find any mixins that differ from the other model
    for (iter=other_mixins.begin(); iter!=other_mixins.end(); ++iter)
    {
        if (my_mixins.find (*iter) == my_mixins.end())
        {
            addedMixins.insert (*iter);
        }
    }

    for (iter=my_mixins.begin(); iter!=my_mixins.end(); ++iter)
    {
        if (other_mixins.find (*iter) == other_mixins.end())
        {
            deletedMixins.insert (*iter);
        }
    }
}

//--------------------------------------------------------------------------------------------------
efd::Bool FlatModel::GetReplicationGroup(efd::UInt32 index) const
{
    return efd::BitUtils::TestBitByIndex(m_activeReplicationGroups, index);
}


//--------------------------------------------------------------------------------------------------
efd::UInt32 FlatModel::GetReplicationGroups() const
{
    return m_activeReplicationGroups;
}


//--------------------------------------------------------------------------------------------------
const BuiltinModelDescriptor* FlatModel::GetBuiltinModelDescriptor(PropertyID propID) const
{
    BuiltinModelDescriptorMapByID::const_iterator iterValues = m_BuiltinModelDescriptorsByID.find(propID);
    if (iterValues != m_BuiltinModelDescriptorsByID.end())
    {
        return iterValues->second;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
const BuiltinModelDescriptor* FlatModel::GetBuiltinModelDescriptor(const efd::utf8string& name) const
{
    BuiltinModelDescriptorMapByName::const_iterator iterValues = m_BuiltinModelDescriptorsByName.find(name);
    if (iterValues != m_BuiltinModelDescriptorsByName.end())
    {
        return iterValues->second;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
bool FlatModel::ContainsBuiltinModel(FlatModelID i_BuiltinModelID) const
{
    return NULL != GetBuiltinModelDescriptor(i_BuiltinModelID);
}


//--------------------------------------------------------------------------------------------------
bool FlatModel::ContainsBuiltinModel(const efd::utf8string& i_strBuiltinModelName) const
{
    return NULL != GetBuiltinModelDescriptor(i_strBuiltinModelName);
}


//--------------------------------------------------------------------------------------------------
bool FlatModel::AddBuiltinModelDescriptor(BuiltinModelDescriptor* i_pDescriptor)
{
    // Note: its valid to add the same descriptor twice
    m_BuiltinModelDescriptorsByID[i_pDescriptor->GetID()] = i_pDescriptor;
    m_BuiltinModelDescriptorsByName[i_pDescriptor->GetName()] = i_pDescriptor;
    return true;
}


//--------------------------------------------------------------------------------------------------
void FlatModel::GetRequiredBuiltinModelsList(BuiltinModelList& o_BuiltinModelList) const
{
    o_BuiltinModelList.clear();

    for (BuiltinModelDescriptorMapByID::const_iterator iter = m_BuiltinModelDescriptorsByID.begin();
          iter != m_BuiltinModelDescriptorsByID.end();
          ++iter)
    {
        o_BuiltinModelList.push_back(iter->second);
    }
}

//--------------------------------------------------------------------------------------------------
void FlatModel::DiffBuiltinModels(
    const FlatModel* pOtherModel,
    efd::set<BuiltinModelDescriptorPtr>& addedBuiltinModels,
    efd::set<BuiltinModelDescriptorPtr>& deletedBuiltinModels) const
{
    FlatModel::BuiltinModelDescriptorMapByID::const_iterator compIter =
        m_BuiltinModelDescriptorsByID.begin();
    FlatModel::BuiltinModelDescriptorMapByID::const_iterator compEnd =
        m_BuiltinModelDescriptorsByID.end();

    while (compIter != compEnd)
    {
        FlatModelID compID = compIter->first;
        if (!pOtherModel->ContainsBuiltinModel(compID))
        {
            deletedBuiltinModels.insert(compIter->second);
        }
        ++compIter;
    }

    compIter = pOtherModel->m_BuiltinModelDescriptorsByID.begin();
    compEnd = pOtherModel->m_BuiltinModelDescriptorsByID.end();

    while (compIter != compEnd)
    {
        FlatModelID compID = compIter->first;
        if (!ContainsBuiltinModel(compID))
        {
            addedBuiltinModels.insert(compIter->second);
        }
        ++compIter;
    }
}

//--------------------------------------------------------------------------------------------------
const BehaviorDescriptor* FlatModel::GetBehaviorDescriptor(BehaviorID i_bid) const
{
    BehaviorDescriptorMapByID::const_iterator iterValues = m_BehaviorDescriptorsByID.find(i_bid);
    if (iterValues != m_BehaviorDescriptorsByID.end())
    {
        return iterValues->second;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
bool FlatModel::SplitModelAndBehavior(
    const efd::utf8string& i_name,
    efd::utf8string& o_model,
    efd::utf8string& o_behavior)
{
    size_t pos = i_name.find(":");

    if (utf8string::npos == pos)
    {
        o_behavior = i_name;
        return false;
    }

    o_model = i_name.substr(0, pos);
    o_behavior = i_name.substr(pos+1);

    return true;
}


//--------------------------------------------------------------------------------------------------
const BehaviorDescriptor* FlatModel::GetBehaviorDescriptor(const efd::utf8string& name) const
{
    utf8string model;
    utf8string behavior;
    if (SplitModelAndBehavior(name, model, behavior))
    {
        return GetMixinBehaviorDescriptor(model, behavior);
    }

    BehaviorDescriptorMapByName::const_iterator iterValues = m_BehaviorDescriptorsByName.find(name);
    if (iterValues != m_BehaviorDescriptorsByName.end())
    {
        return iterValues->second;
    }
    return NULL;
}


//--------------------------------------------------------------------------------------------------
const BehaviorDescriptor* FlatModel::GetMixinBehaviorDescriptor(
    const efd::utf8string& i_strModelName,
    const efd::utf8string& i_strBehaviorName) const
{
    const BehaviorDescriptor* pResult = NULL;

    if (m_pFlatModelManager)
    {
        const FlatModel* pMixin = m_pFlatModelManager->FindOrLoadModel(i_strModelName);
        if (pMixin)
        {
            pResult = pMixin->GetBehaviorDescriptor(i_strBehaviorName);
            if (pResult)
            {
                // make sure model name is set.  Why is this required?
                EE_ASSERT(!pResult->GetModelName().empty());
            }
        }
    }

    return pResult;
}


//--------------------------------------------------------------------------------------------------
const BehaviorDescriptor* FlatModel::GetMixinBehaviorDescriptor(
    FlatModelID i_modelId,
    BehaviorID i_behaviorId) const
{
    const BehaviorDescriptor* pResult = NULL;

    if (m_pFlatModelManager)
    {
        const FlatModel* pMixin = m_pFlatModelManager->FindModel(i_modelId);
        if (pMixin)
        {
            pResult = pMixin->GetBehaviorDescriptor(i_behaviorId);
            if (pResult)
            {
                // make sure model name is set.  Why is this required?
                EE_ASSERT(!pResult->GetModelName().empty());
            }
        }
    }

    return pResult;
}

//--------------------------------------------------------------------------------------------------
bool FlatModel::AddBehaviorDescriptor(BehaviorDescriptor* i_pBehavior)
{
    if (NULL == i_pBehavior)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Tried to add a NULL behavior descriptor to flat model %d (%s)",
            m_flatModelID, m_name.c_str()));

        return false;
    }

    m_BehaviorDescriptorsByID[i_pBehavior->GetID()] = i_pBehavior;
    m_BehaviorDescriptorsByName[i_pBehavior->GetName()] = i_pBehavior;
    return true;
}

//--------------------------------------------------------------------------------------------------
void FlatModel::GetBehaviorNames(efd::list<efd::utf8string>& o_listOfNames) const
{
    o_listOfNames.clear();

    for (BehaviorDescriptorMapByName::const_iterator iterValues = m_BehaviorDescriptorsByName.begin();
          iterValues != m_BehaviorDescriptorsByName.end();
          ++iterValues)
    {
        efd::utf8string behavior = iterValues->second->GetModelName();
        behavior += ":";
        behavior += iterValues->first;
        o_listOfNames.push_back(behavior);
    }
}


//--------------------------------------------------------------------------------------------------
bool FlatModel::ContainsModel(FlatModelID id) const
{
    return m_superModels.find(id) != m_superModels.end();
}


//--------------------------------------------------------------------------------------------------
bool FlatModel::ContainsModel(const efd::utf8string& i_strName) const
{
    if (!m_pFlatModelManager)
    {
        return false;
    }

    FlatModelID id = m_pFlatModelManager->GetModelIDByName(i_strName);
    return ContainsModel(id);
}


//--------------------------------------------------------------------------------------------------
void FlatModel::AddMixinModel(FlatModelID i_id)
{
    if (i_id)
    {
        m_superModels.push_back(i_id);
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("Tried to add an invalid mixin model ID to flat model %d (%s)",
            m_flatModelID, m_name.c_str()));
    }
}


//--------------------------------------------------------------------------------------------------
void FlatModel::GetMixinNames(efd::list<efd::utf8string>& o_listOfNames) const
{
    o_listOfNames.clear();

    if (m_pFlatModelManager)
    {
        for (SuperModelSet::const_iterator iter = m_superModels.begin();
              iter != m_superModels.end();
              ++iter)
        {
            FlatModelID id = *iter;

            // We are included in out own super models list but we don't want/need to include
            // our name is this result set.
            if (id != GetID())
            {
                const efd::utf8string& modelName = m_pFlatModelManager->GetModelNameByID(id);
                o_listOfNames.push_back(modelName);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
void FlatModel::GetBehaviorInvocationOrder(
    const efd::utf8string& i_behaviorName,
    const efd::list<efd::utf8string>*& o_invocationOrder) const
{
    const BehaviorDescriptor* behaviorDescriptor = GetBehaviorDescriptor(i_behaviorName);
    if (behaviorDescriptor == NULL)
    {
        o_invocationOrder = NULL;
    }
    else
    {
        const BehaviorDescriptor::InvocationOrderedModelNamesList& modelNames =
            behaviorDescriptor->GetInvocationOrderedModelNames();
        o_invocationOrder = &modelNames;
    }
}


//--------------------------------------------------------------------------------------------------
void FlatModel::PrepareDefaultPropertyList(const BuiltinModelDescriptor* i_pcd,
                                            PropertyDescriptorList& o_defaults) const
{
    o_defaults.clear();

    // iterate over all properties in the flat model, if they are sourced from the given
    // BuiltinModel then add them to the list
    for (PropertyDescriptorMapByID::const_iterator iter = m_propertyDescriptorsByID.begin();
          iter != m_propertyDescriptorsByID.end();
          ++iter)
    {
        PropertyDescriptor* pDesc = iter->second;
        if (pDesc->GetSource() == i_pcd->GetName())
        {
            o_defaults.push_back(pDesc);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void FlatModel::Mutate(const FlatModel* pNewFlatModel)
{
    // set the basic information to match the new model
    m_name = pNewFlatModel->m_name;
    m_traits = pNewFlatModel->m_traits;
    m_activeReplicationGroups = pNewFlatModel->m_activeReplicationGroups;

    efd::set<PropertyDescriptorPtr> unchangedProperties;
    efd::set<PropertyDescriptorPtr> newProperties;
    efd::set<PropertyDescriptorPtr> updatedProperties;
    efd::set<PropertyDescriptorPtr> deletedProperties;

    DiffProperties(
        pNewFlatModel,
        unchangedProperties,
        newProperties,
        updatedProperties,
        deletedProperties);

    // remove deleted properties.
    for (efd::set<PropertyDescriptorPtr>::const_iterator it = deletedProperties.begin();
        it != deletedProperties.end(); ++it)
    {
        m_propertyDescriptorsByID.erase((*it)->GetPropertyID());
        m_propertyDescriptorsByName.erase((*it)->GetName());
    }

    // add new properties.
    for (efd::set<PropertyDescriptorPtr>::const_iterator it = newProperties.begin();
         it != newProperties.end(); ++it)
    {
        m_propertyDescriptorsByID[(*it)->GetPropertyID()] = *it;
        m_propertyDescriptorsByName[(*it)->GetName()] = *it;
    }

    // replace existing descriptors with updated ones.
    for (efd::set<PropertyDescriptorPtr>::const_iterator it = updatedProperties.begin();
        it != updatedProperties.end(); ++it)
    {
        m_propertyDescriptorsByID[(*it)->GetPropertyID()] = *it;
        m_propertyDescriptorsByName[(*it)->GetName()] = *it;
    }

    // Update all the other maps:
    m_BehaviorDescriptorsByID = pNewFlatModel->m_BehaviorDescriptorsByID;
    m_BehaviorDescriptorsByName = pNewFlatModel->m_BehaviorDescriptorsByName;
    m_BuiltinModelDescriptorsByID = pNewFlatModel->m_BuiltinModelDescriptorsByID;
    m_BuiltinModelDescriptorsByName = pNewFlatModel->m_BuiltinModelDescriptorsByName;
    m_extraData = pNewFlatModel->m_extraData;
    m_superModels = pNewFlatModel->m_superModels;
}

//--------------------------------------------------------------------------------------------------
void FlatModel::AddExtraData(egf::ExtraData* extraData)
{
    EE_ASSERT(extraData);
    EE_ASSERT(extraData->GetType().length() > 0);

    m_extraData[extraData->GetType()].push_back(extraData);
}

//--------------------------------------------------------------------------------------------------
void FlatModel::GetExtraData(const efd::utf8string& type,
                             efd::list<ExtraDataPtr>& o_extraData) const
{
    m_extraData.find(type, o_extraData);
}
