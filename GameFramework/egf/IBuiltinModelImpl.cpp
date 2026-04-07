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

#include <egf/IBuiltinModelImpl.h>
#include "egfLogIDs.h"


using namespace egf;


//------------------------------------------------------------------------------------------------
IBuiltinModelImpl::IBuiltinModelImpl()
{
}

//------------------------------------------------------------------------------------------------
IBuiltinModelImpl::~IBuiltinModelImpl()
{
}

//------------------------------------------------------------------------------------------------
egf::PropertyResult IBuiltinModelImpl::GetValue(egf::PropertyID i_propID, void* o_pData) const
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }
    egf::BuiltinHelper::PropertyData* data = iter->second;
    return data->Get(this, o_pData);
}

//------------------------------------------------------------------------------------------------
egf::PropertyResult IBuiltinModelImpl::GetValue(
    PropertyID i_propID,
    const efd::utf8string& key,
    void* o_pData) const
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }
    egf::BuiltinHelper::PropertyData* data = iter->second;
    return data->Get(this, key, o_pData);
}

//------------------------------------------------------------------------------------------------
PropertyResult IBuiltinModelImpl::SetValue(
    PropertyID i_propID,
    const efd::utf8string& key,
    const void* i_pData)
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }
    egf::BuiltinHelper::PropertyData* data = iter->second;
    egf::PropertyResult result = data->Set(this, key, i_pData);
    return result;

}

//------------------------------------------------------------------------------------------------
egf::PropertyResult IBuiltinModelImpl::SetValue(egf::PropertyID i_propID, const void* i_pData)
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }
    egf::BuiltinHelper::PropertyData* data = iter->second;
    egf::PropertyResult result = data->Set(this, i_pData);
    return result;
}

//------------------------------------------------------------------------------------------------
PropertyResult IBuiltinModelImpl::GetValueAsString(PropertyID propID, efd::utf8string& data) const
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }
    const egf::BuiltinHelper::PropertyData* pPropData = iter->second;
    return pPropData->GetAsString(this, data);
}

//------------------------------------------------------------------------------------------------
PropertyResult IBuiltinModelImpl::GetValueAsString(PropertyID propID, const efd::utf8string& key,
                                                   efd::utf8string& data) const
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }
    const egf::BuiltinHelper::PropertyData* pPropData = iter->second;
    return pPropData->GetAsString(this, key, data);
}

//------------------------------------------------------------------------------------------------
PropertyResult IBuiltinModelImpl::SetValueByString(
    PropertyID i_propID,
    const efd::utf8string& key,
    const efd::utf8string& i_data)
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }
    egf::BuiltinHelper::PropertyData* data = iter->second;
    egf::PropertyResult result = data->SetByString(this, key, i_data);
    return result;
}

//------------------------------------------------------------------------------------------------
PropertyResult IBuiltinModelImpl::SetValueByString(
    PropertyID i_propID,
    const efd::utf8string& i_data)
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }
    egf::BuiltinHelper::PropertyData* data = iter->second;
    egf::PropertyResult result = data->SetByString(this, i_data);
    return result;
}

//------------------------------------------------------------------------------------------------
PropertyResult IBuiltinModelImpl::GetPropertyCount(
    PropertyID i_propID,
    efd::UInt32& o_count) const
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }

    egf::BuiltinHelper::PropertyData* data = iter->second;
    return data->GetKeyCount(this, o_count);
}

//------------------------------------------------------------------------------------------------
PropertyResult IBuiltinModelImpl::GetPropertyKeys(
    PropertyID i_propID,
    efd::list<efd::utf8string>& o_keys) const
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }

    egf::BuiltinHelper::PropertyData* data = iter->second;
    return data->GetKeys(this, o_keys);
}

//------------------------------------------------------------------------------------------------
PropertyResult IBuiltinModelImpl::GetNextPropertyKey(
    PropertyID i_propID,
    const efd::utf8string& i_previousKey,
    efd::utf8string& o_nextKey) const
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }

    egf::BuiltinHelper::PropertyData* data = iter->second;
    return data->GetNextPropertyKey(this, i_previousKey, o_nextKey);
}

//------------------------------------------------------------------------------------------------
PropertyResult IBuiltinModelImpl::HasValue(PropertyID propID, const efd::utf8string& key) const
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }

    egf::BuiltinHelper::PropertyData* data = iter->second;
    return data->HasValue(this, key);
}

//------------------------------------------------------------------------------------------------
PropertyResult IBuiltinModelImpl::RemoveValue(PropertyID propID, const efd::utf8string& key)
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(propID);
    if (iter == map.end())
    {
        return egf::PropertyResult_PropertyNotFound;
    }

    egf::BuiltinHelper::PropertyData* data = iter->second;
    return data->RemoveValue(this, key);
}

//------------------------------------------------------------------------------------------------
efd::ClassID IBuiltinModelImpl::GetDataType(egf::PropertyID i_propID) const
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        return 0;
    }
    egf::BuiltinHelper::PropertyData* data = iter->second;
    return data->GetType();
}

//------------------------------------------------------------------------------------------------
bool IBuiltinModelImpl::Initialize(
    egf::Entity* i_pOwner,
    const egf::PropertyDescriptorList& i_Defaults)
{
    m_pOwningEntity = i_pOwner;
    efd::UInt32 error = 0;

    for (egf::PropertyDescriptorList::const_iterator iter = i_Defaults.begin();
          iter != i_Defaults.end();
          ++iter)
    {
        egf::PropertyDescriptor* pDesc = *iter;
        if (!ResetProperty(pDesc))
        {
            ++error;
        }
    }
    return 0 == error;
}

//------------------------------------------------------------------------------------------------
bool IBuiltinModelImpl::ResetProperty(const egf::PropertyDescriptor* pDesc)
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator prop = map.find(pDesc->GetPropertyID());
    if (prop != map.end())
    {
        egf::BuiltinHelper::PropertyData* data = prop->second;
        egf::PropertyResult result = data->SetDefault(this, pDesc);
        switch (result)
        {
        case egf::PropertyResult_OK:
            /* All properties have default values which we ignore for read-only bindings. */
        case egf::PropertyResult_ReadOnlyError:
            /* do nothing */
            return true;

        default:
            EE_LOG(efd::kEntity, efd::ILogger::kERR0,
                ("Unable to initialize property '%s', Error code: %d",
                pDesc->GetName().c_str(),
                result));
            return false;
        }
    }

    EE_LOG(efd::kEntity, efd::ILogger::kERR0,
        ("Unable to initialize property '%s', the property is not a member of this builtin model.",
         pDesc->GetName().c_str()));

    return false;
}

//------------------------------------------------------------------------------------------------
void IBuiltinModelImpl::SerializeProperty(egf::PropertyID i_propID, efd::Archive& io_ar)
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        io_ar.RaiseError();
        return;
    }
    egf::BuiltinHelper::PropertyData* data = iter->second;
    // The PropertyData helpers still use separate pack/unpack methods because they need to be
    // "const correct" as they often interact with interfaces built for external consumption.
    if (io_ar.IsPacking())
    {
        data->Pack(this, io_ar);
    }
    else
    {
        data->Unpack(this, io_ar);
    }
}

//------------------------------------------------------------------------------------------------
void IBuiltinModelImpl::AdvanceStream(egf::PropertyID i_propID, efd::Archive& i_ar) const
{
    const BuiltinModelPropertyMap& map = GetPropertyMap();
    BuiltinModelPropertyMap::const_iterator iter = map.find(i_propID);
    if (iter == map.end())
    {
        i_ar.RaiseError();
        return;
    }
    egf::BuiltinHelper::PropertyData* data = iter->second;
    data->Skip(this, i_ar);
}

//------------------------------------------------------------------------------------------------
efd::list<BuiltinModelPropertyMap*> BuiltinModelStaticMapCleaner::ms_mapsNeedingCleanup;

//------------------------------------------------------------------------------------------------
void BuiltinModelStaticMapCleaner::CleanMapAtSDMShutdown(BuiltinModelPropertyMap& map)
{
    ms_mapsNeedingCleanup.push_back(&map);
}

//------------------------------------------------------------------------------------------------
void BuiltinModelStaticMapCleaner::_SDMShutdown()
{
    for (efd::list<BuiltinModelPropertyMap*>::iterator iter = ms_mapsNeedingCleanup.begin();
        iter != ms_mapsNeedingCleanup.end();
        ++iter)
    {
        BuiltinModelPropertyMap* pmap = *iter;
        pmap->clear();
    }
    ms_mapsNeedingCleanup.clear();
}

//------------------------------------------------------------------------------------------------
