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

// Pre-compiled header
#include "ecrPCH.h"

#include <ecr/IAttachedObjectsProperty.h>
#include <egf/StandardModelLibraryPropertyIDs.h>


using namespace efd;
using namespace egf;
using namespace ecr;

//--------------------------------------------------------------------------------------------------
PropertyResult IAttachedObjectsProperty::GetAttachedObject(
    const utf8string& key,
    AttachNifData& val) const
{
    if (m_attachNifData.find(key, val))
        return PropertyResult_OK;
        
    return PropertyResult_KeyNotFound;
}
    
//--------------------------------------------------------------------------------------------------
void IAttachedObjectsProperty::SetAttachedObject(const utf8string& key, const AttachNifData& val)
{
    m_attachNifData[key] = val;
}
    
//--------------------------------------------------------------------------------------------------
UInt32 IAttachedObjectsProperty::GetKeyCount() const
{
    return (UInt32)m_attachNifData.size();
}
    
//--------------------------------------------------------------------------------------------------
void IAttachedObjectsProperty::ClearAllAttachedObjects()
{
    m_attachNifData.clear();
}

//--------------------------------------------------------------------------------------------------
void IAttachedObjectsProperty::GetNextKey(const utf8string& prevKey, utf8string& nextKey) const
{
    if (prevKey.empty())
    {
        efd::map<efd::utf8string, AttachNifData>::const_iterator iter = m_attachNifData.begin();
        if (iter == m_attachNifData.end())
            nextKey.clear();
        else
            nextKey = iter->first;
        return;
    }

    efd::map<efd::utf8string, AttachNifData>::const_iterator iter = m_attachNifData.find(prevKey);
    if (iter == m_attachNifData.end())
    {
        nextKey.clear();
        return;
    }
    
    ++iter;
    if (iter == m_attachNifData.end())
    {
        nextKey.clear();
    }
    else
    {
        nextKey = iter->first;
    }
}
    
//--------------------------------------------------------------------------------------------------
PropertyResult IAttachedObjectsProperty::RemoveAttachedObject(const efd::utf8string& key)
{
    efd::map<efd::utf8string, AttachNifData>::iterator iter = m_attachNifData.find(key);
    if (iter == m_attachNifData.end())
        return PropertyResult_KeyNotFound;
    
    m_attachNifData.erase(iter);
    
    return PropertyResult_OK;
}

//--------------------------------------------------------------------------------------------------
bool IAttachedObjectsProperty::SetAttachedObjectsProperty(const egf::PropertyDescriptor* pDefault)
{
    EE_ASSERT(pDefault->GetPropertyID() == kPropertyID_StandardModelLibrary_AttachedObjects);

    m_attachNifData.clear();

    efd::list<efd::utf8string> keys;
    pDefault->GetDefaultProperty()->GetPropertyKeys(
        kPropertyID_StandardModelLibrary_AttachedObjects,
        keys);
    efd::list<efd::utf8string>::iterator iter = keys.begin();
    while (iter != keys.end())
    {
        AttachNifData data;
        pDefault->GetDefaultProperty()->GetValue(
            kPropertyID_StandardModelLibrary_AttachedObjects,
            (*iter),
            &data);
        m_attachNifData[*iter] = data;
        ++iter;
    }
    
    return true;
}

//--------------------------------------------------------------------------------------------------
bool IAttachedObjectsProperty::operator==(const IAttachedObjectsProperty& other) const
{
    efd::map<utf8string, AttachNifData>::const_iterator thisIter = m_attachNifData.begin();
    efd::map<utf8string, AttachNifData>::const_iterator otherIter = other.m_attachNifData.begin();
    while (thisIter != m_attachNifData.end())
    {
        if (otherIter == other.m_attachNifData.end())
            return false;
        
        if (thisIter->first != otherIter->first ||
            thisIter->second != otherIter->second)
        {
            return false;
        }

        ++thisIter;
        ++otherIter;
    }
    
    if (otherIter != other.m_attachNifData.end())
        return false;
    
    return true;
}

//--------------------------------------------------------------------------------------------------
