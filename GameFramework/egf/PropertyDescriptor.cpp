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

#include "PropertyDescriptor.h"
#include <efd/ILogger.h>
#include <egf/egfLogIDs.h>

using namespace egf;
using namespace efd;


//-------------------------------------------------------------------------------------------------
PropertyResult PropertyDescriptor::SetDefaultProperty(const IProperty& prop)
{
    // make sure a default property doesn't current exist
    if (m_pDefaultProperty)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Tried to set the default value of property ID: %d (%s), but a value for that "
            "property has already been set.  Default properties should not be changed at "
            "runtime.",
            m_propertyID, m_name.c_str()));

        return PropertyResult_DefaultValueAlreadySet;
    }

    // ensure the property is the correct type!
    if (prop.GetPropertyType() != m_propertyClassID)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Tried to set default value of property ID: %d (%s) using an IProperty with an "
            "incompatible property type!  m_propertyClassID == 0x%08X, incoming type == 0x%08X",
            m_propertyID, m_name.c_str(), m_propertyClassID, prop.GetPropertyType()));

        return PropertyResult_TypeMismatch;
    }

    // ensure the property has the correct storage type!
    if (prop.GetDataType(m_propertyID) != m_dataClassID)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR2,
            ("Tried to set default value of property ID: %d (%s) using an IProperty with an "
            "incompatible storage type!  m_dataClassID == 0x%08X, incoming type == 0x%08X",
            m_propertyID, m_name.c_str(), m_dataClassID, prop.GetDataType(m_propertyID)));

        return PropertyResult_TypeMismatch;
    }

    // Clone the property and assign it to the member
    m_pDefaultProperty = prop.Clone();
    return PropertyResult_OK;
}


//-------------------------------------------------------------------------------------------------
bool PropertyDescriptor::IsValid() const
{
    bool result = true;

    if (0 == m_dataClassID ||
         0 == m_propertyClassID)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR3,
            ("PropertyDescriptor::IsValid failure: Invalid property class id(s)"));
        result = false;
    }
    if (0 == m_propertyID)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR3,
            ("PropertyDescriptor::IsValid failure: Invalid property id"));
        result = false;
    }

    if (m_name.empty())
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR3,
            ("PropertyDescriptor::IsValid failure: Invalid property name"));
        result = false;
    }

    if (NULL == m_pDefaultProperty)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR3,
            ("PropertyDescriptor::IsValid failure: No default property"));
        result = false;
    }
    else
    {
        if (m_pDefaultProperty->GetPropertyType() != m_propertyClassID)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR3,
                ("PropertyDescriptor::IsValid failure: default property property type mismatch"));
            result = false;
        }
        if (m_pDefaultProperty->GetDataType(m_propertyID) != m_dataClassID)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR3,
                ("PropertyDescriptor::IsValid failure: default property data type mismatch"));
            result = false;
        }
    }

    // If this property is from a built-in model then it must have a source specified.
    if (GetTrait(PropertyTrait_FromBuiltinModel) || GetTrait(PropertyTrait_FromReplicaBuiltinModel))
    {
        if (m_source.empty())
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR3,
                ("PropertyDescriptor::IsValid failure: built-in model property has no source."));
            result = false;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------------------------
void PropertyDescriptor::SetPropertyMetaType(PropertyDescriptor::PropertyMetaType t)
{
    m_metatype = t;
}

//-----------------------------------------------------------------------------------------------
PropertyDescriptor::PropertyMetaType PropertyDescriptor::GetPropertyMetaType() const
{
    return m_metatype;
}

