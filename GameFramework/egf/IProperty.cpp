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

#include <egf/IProperty.h>

using namespace egf;

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::GetValue(PropertyID propID, void* data) const
{
    EE_UNUSED_ARG(propID);
    EE_UNUSED_ARG(data);
    return PropertyResult_PropertyNotScalar;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::SetValue(PropertyID propID, const void* data)
{
    EE_UNUSED_ARG(propID);
    EE_UNUSED_ARG(data);
    return PropertyResult_PropertyNotScalar;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::GetValue(PropertyID propID, const efd::utf8string& key, void* data) const
{
    EE_UNUSED_ARG(propID);
    EE_UNUSED_ARG(key);
    EE_UNUSED_ARG(data);
    return PropertyResult_PropertyNotAssociativeArray;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::SetValue(PropertyID propID, const efd::utf8string& key, const void* data)
{
    EE_UNUSED_ARG(propID);
    EE_UNUSED_ARG(key);
    EE_UNUSED_ARG(data);
    return PropertyResult_PropertyNotAssociativeArray;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::GetValueAsString(PropertyID propID, efd::utf8string& data) const
{
    EE_UNUSED_ARG(propID);
    EE_UNUSED_ARG(data);
    return PropertyResult_PropertyNotScalar;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::SetValueByString(PropertyID propID, const efd::utf8string& data)
{
    EE_UNUSED_ARG(propID);
    EE_UNUSED_ARG(data);
    return PropertyResult_PropertyNotScalar;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::GetValueAsString(
    PropertyID propID,
    const efd::utf8string& key,
    efd::utf8string& data) const
{
    EE_UNUSED_ARG(propID);
    EE_UNUSED_ARG(key);
    EE_UNUSED_ARG(data);
    return PropertyResult_PropertyNotAssociativeArray;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::SetValueByString(
    PropertyID propID,
    const efd::utf8string& key,
    const efd::utf8string& data)
{
    EE_UNUSED_ARG(propID);
    EE_UNUSED_ARG(key);
    EE_UNUSED_ARG(data);
    return PropertyResult_PropertyNotAssociativeArray;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::GetPropertyCount(PropertyID i_propID, efd::UInt32& o_count) const
{
    EE_UNUSED_ARG(i_propID);
    o_count = 0;
    return PropertyResult_PropertyNotAssociativeArray;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::GetPropertyKeys(
    PropertyID i_propID,
    efd::list<efd::utf8string>& o_keys) const
{
    EE_UNUSED_ARG(i_propID);
    o_keys.clear();
    return PropertyResult_PropertyNotAssociativeArray;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::GetNextPropertyKey(
    PropertyID i_propID,
    const efd::utf8string& i_previousKey,
    efd::utf8string& o_nextKey) const
{
    EE_UNUSED_ARG(i_propID);
    EE_UNUSED_ARG(i_previousKey);
    o_nextKey.clear();
    return PropertyResult_PropertyNotAssociativeArray;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::HasValue(PropertyID propID, const efd::utf8string& key) const
{
    EE_UNUSED_ARG(propID);
    EE_UNUSED_ARG(key);
    return PropertyResult_PropertyNotAssociativeArray;
}

//------------------------------------------------------------------------------------------------
PropertyResult IProperty::RemoveValue(PropertyID propID, const efd::utf8string& key)
{
    EE_UNUSED_ARG(propID);
    EE_UNUSED_ARG(key);
    return PropertyResult_PropertyNotAssociativeArray;
}

