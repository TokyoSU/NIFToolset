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

#include <egf/IBuiltinModel.h>
#include <egf/Entity.h>

namespace egf
{

//------------------------------------------------------------------------------------------------
IBuiltinModel::IBuiltinModel()
    : m_pOwningEntity(NULL)
{
}

//------------------------------------------------------------------------------------------------
bool IBuiltinModel::operator==(const IProperty& other) const
{
    if (this == &other)
    {
        return true;
    }
    if (other.GetPropertyType() != GetPropertyType())
    {
        return false;
    }

    const IBuiltinModel* otherClass = static_cast<const IBuiltinModel *>(&other);
    return otherClass->m_pOwningEntity == m_pOwningEntity;
}

//------------------------------------------------------------------------------------------------
void IBuiltinModel::SetDirty(PropertyID propID)
{
    if (m_pOwningEntity)
    {
        m_pOwningEntity->BuiltinPropertyChanged(propID, this);
    }
}

//------------------------------------------------------------------------------------------------
void IBuiltinModel::OnRemoved()
{
    m_pOwningEntity = NULL;
}

//------------------------------------------------------------------------------------------------
bool IBuiltinModel::Dispatch(const BehaviorDescriptor*, efd::ParameterList*)
{
    return false;
}

//------------------------------------------------------------------------------------------------
bool IBuiltinModel::Initialize(Entity* i_pOwner, const PropertyDescriptorList& i_Defaults)
{
    m_pOwningEntity = i_pOwner;

    // This default implementation is only valid if there are no built-in model properties.
    return i_Defaults.empty();
}

//------------------------------------------------------------------------------------------------
void IBuiltinModel::OnAdded()
{
}

//------------------------------------------------------------------------------------------------
void IBuiltinModel::OnEndLifecycle(efd::UInt32)
{
}

//------------------------------------------------------------------------------------------------
IProperty* IBuiltinModel::Clone() const
{
    return NULL;
}

//------------------------------------------------------------------------------------------------
void IBuiltinModel::OnOwningEntityReinitialized(Entity* i_pOwner)
{
    m_pOwningEntity = i_pOwner;
}

//------------------------------------------------------------------------------------------------
bool IBuiltinModel::ResetProperty(const egf::PropertyDescriptor*)
{
    return false;
}

//------------------------------------------------------------------------------------------------
efd::ClassID IBuiltinModel::GetPropertyType() const
{
    return 0;
}

//------------------------------------------------------------------------------------------------
const efd::ServiceManager* IBuiltinModel::GetServiceManager() const
{
    if (m_pOwningEntity)
    {
        return m_pOwningEntity->GetServiceManager();
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
} // end namespace egf

