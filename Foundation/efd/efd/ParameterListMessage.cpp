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

#include "efdPCH.h"

#include <efd/ParameterListMessage.h>
#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>

//------------------------------------------------------------------------------------------------
using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ParameterListMessage);


//------------------------------------------------------------------------------------------------
ParameterListMessage::ParameterListMessage()
: IMessage()
, m_params()
, m_nextIndex(0)
{
}

//------------------------------------------------------------------------------------------------
ParameterListMessage::~ParameterListMessage()
{
}

//------------------------------------------------------------------------------------------------
void ParameterListMessage::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_params, ar);
    if (ar.IsUnpacking())
    {
        m_nextIndex = 0;
    }
}

//------------------------------------------------------------------------------------------------
