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
#include "egfPCH.h"

#include <egf/PlaceableModel.h>
#include <egf/Entity.h>

using namespace efd;
using namespace egf;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(PlaceableModel);

EE_IMPLEMENT_BUILTINMODEL_PROPERTIES(PlaceableModel);

efd::vector<IPropertyCallback*>* PlaceableModel::ms_pCallbackList = 0;

//------------------------------------------------------------------------------------------------
void PlaceableModel::_SDMInit()
{
    ms_pCallbackList = EE_EXTERNAL_NEW efd::vector<IPropertyCallback*>;
}

//------------------------------------------------------------------------------------------------
void PlaceableModel::_SDMShutdown()
{
    EE_EXTERNAL_DELETE ms_pCallbackList;
}

//------------------------------------------------------------------------------------------------
IBuiltinModel* PlaceableModel::PlaceableModelFactory()
{
    return EE_NEW PlaceableModel();
}

//------------------------------------------------------------------------------------------------
PlaceableModel::PlaceableModel()
    : IPropertyCallbackInvoker(ms_pCallbackList)
    , m_position(0.0f, 0.0f, 0.0f)
    , m_rotation(0.0f, 0.0f, 0.0f)
    , m_scale(1.0f)
{
}

//------------------------------------------------------------------------------------------------
PlaceableModel::~PlaceableModel()
{
}

//------------------------------------------------------------------------------------------------
bool PlaceableModel::Initialize(egf::Entity* pOwner, const egf::PropertyDescriptorList& defaults)
{
    IBuiltinModel::Initialize(pOwner, defaults);

    for (PropertyDescriptorList::const_iterator iter = defaults.begin();
        iter != defaults.end();
        ++iter)
    {
        switch ((*iter)->GetPropertyID())
        {
        case kPropertyID_StandardModelLibrary_Position:
            (*iter)->GetDefaultProperty()->GetValue(
                kPropertyID_StandardModelLibrary_Position,
                &m_position);
            break;

        case kPropertyID_StandardModelLibrary_Rotation:
            (*iter)->GetDefaultProperty()->GetValue(
                kPropertyID_StandardModelLibrary_Rotation,
                &m_rotation);
            break;

        case kPropertyID_StandardModelLibrary_Scale:
            (*iter)->GetDefaultProperty()->GetValue(
                kPropertyID_StandardModelLibrary_Scale,
                &m_scale);
            break;

        default:
            // do nothing
            break;
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool PlaceableModel::ResetProperty(const egf::PropertyDescriptor* pDefault)
{
    switch (pDefault->GetPropertyID())
    {
    case kPropertyID_StandardModelLibrary_Position:
        {
            Point3 position;
            pDefault->GetDefaultProperty()->GetValue(
                kPropertyID_StandardModelLibrary_Position,
                &position);
            SetPosition(position);
        }
        break;

    case kPropertyID_StandardModelLibrary_Rotation:
        {
            Point3 rotation;
            pDefault->GetDefaultProperty()->GetValue(
                kPropertyID_StandardModelLibrary_Rotation,
                &rotation);
            SetRotation(rotation);
        }
        break;

    case kPropertyID_StandardModelLibrary_Scale:
        {
            Float32 scale;
            pDefault->GetDefaultProperty()->GetValue(
                kPropertyID_StandardModelLibrary_Scale,
                &scale);
            SetScale(scale);
        }
        break;

    default:
        // do nothing
        break;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool PlaceableModel::operator==(const IProperty& other) const
{
    if (!IBuiltinModel::operator ==(other))
        return false;

    const PlaceableModel* otherClass = static_cast<const PlaceableModel *>(&other);

    if (m_position != otherClass->m_position ||
        m_rotation != otherClass->m_rotation ||
        m_scale != otherClass->m_scale)
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void egf::PlaceableModel::SetInternalPosition(
    const efd::Point3& position,
    egf::IPropertyCallback* ignoreCallback)
{
    if (m_position != position)
    {
        m_position = position;

        m_pOwningEntity->BuiltinPropertyChanged(
            egf::kPropertyID_StandardModelLibrary_Position,
            this);

        InvokeCallbacks(
            egf::kFlatModelID_StandardModelLibrary_Placeable,
            m_pOwningEntity,
            kPropertyID_StandardModelLibrary_Position,
            this,
            0,
            ignoreCallback);
    }
}

//------------------------------------------------------------------------------------------------
void egf::PlaceableModel::SetInternalRotation(
    const efd::Point3& rotation,
    egf::IPropertyCallback* ignoreCallback)
{
    if (m_rotation != rotation)
    {
        m_rotation = rotation;

        m_pOwningEntity->BuiltinPropertyChanged(
            egf::kPropertyID_StandardModelLibrary_Rotation,
            this);

        InvokeCallbacks(
            egf::kFlatModelID_StandardModelLibrary_Placeable,
            m_pOwningEntity,
            kPropertyID_StandardModelLibrary_Rotation,
            this,
            0,
            ignoreCallback);
    }
}

//------------------------------------------------------------------------------------------------
void egf::PlaceableModel::SetInternalScale(
    const efd::Float32 scale,
    egf::IPropertyCallback* ignoreCallback)
{
    if (m_scale != scale)
    {
        m_scale = scale;

        m_pOwningEntity->BuiltinPropertyChanged(egf::kPropertyID_StandardModelLibrary_Scale, this);

        InvokeCallbacks(
            egf::kFlatModelID_StandardModelLibrary_Placeable,
            m_pOwningEntity,
            kPropertyID_StandardModelLibrary_Scale,
            this,
            0,
            ignoreCallback);
    }
}

//------------------------------------------------------------------------------------------------
