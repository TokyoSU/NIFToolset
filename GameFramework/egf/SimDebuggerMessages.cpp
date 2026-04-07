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
// Emergent Game Technologies, Calabasas, California 91302
// http://www.emergent.net

#include "egfPCH.h"

#include <egf/SimDebuggerMessages.h>
#include <efd/SerializeRoutines.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(DebugNoteBase);

//------------------------------------------------------------------------------------------------
void DebugNoteBase::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);

    Serializer::SerializeObject(m_timestamp, ar);
    Serializer::SerializeObject(m_gameID, ar);
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(DebugNoteEvent);

//------------------------------------------------------------------------------------------------
void DebugNoteEvent::Serialize(efd::Archive& ar)
{
    DebugNoteBase::Serialize(ar);

    Serializer::SerializeObject(m_eventName, ar);
    Serializer::SerializeObject(m_params, ar);
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(DebugNoteSessionStarted);


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(DebugNoteFlatModelDefined);

//------------------------------------------------------------------------------------------------
void DebugNoteFlatModelDefined::Serialize(efd::Archive& ar)
{
    DebugNoteBase::Serialize(ar);

    Serializer::SerializeObject(m_modelName, ar);
    Serializer::SerializeObject(m_isMesh, ar);
    Serializer::SerializeObject(m_isActor, ar);

    // Write properties
    SR_StdVector<SR_StdPair<> >::Serialize(m_properties, ar);
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(DebugNoteEntityCreated);

//------------------------------------------------------------------------------------------------
void DebugNoteEntityCreated::Serialize(efd::Archive& ar)
{
    DebugNoteBase::Serialize(ar);

    Serializer::SerializeObject(m_toolID, ar);
    Serializer::SerializeObject(m_modelName, ar);
    Serializer::SerializeObject(m_position, ar);
    Serializer::SerializeObject(m_facing, ar);

    // Write properties
    SR_StdVector<SR_StdPair<> >::Serialize(m_properties, ar);
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(DebugNoteEntityDestroyed);


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(DebugNotePropertyChanged);

//------------------------------------------------------------------------------------------------
void DebugNotePropertyChanged::Serialize(efd::Archive& ar)
{
    DebugNoteBase::Serialize(ar);

    Serializer::SerializeObject(m_propertyName, ar);
    Serializer::SerializeObject(m_propertyValue, ar);
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(DebugNoteCollectionPropertyChanged);

//------------------------------------------------------------------------------------------------
void DebugNoteCollectionPropertyChanged::Serialize(efd::Archive& ar)
{
    DebugNoteBase::Serialize(ar);

    Serializer::SerializeObject(m_propertyName, ar);

    // Write items
    SR_StdVector<SR_StdPair<> >::Serialize(m_items, ar);
}

//------------------------------------------------------------------------------------------------
