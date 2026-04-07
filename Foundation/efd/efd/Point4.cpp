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

#include <efd/Point4.h>
#include <efd/Serialize.h>

//------------------------------------------------------------------------------------------------
void efd::Point4::Serialize(efd::Archive& ar)
{
    efd::Serializer::SerializeObject(m_afPt[0], ar);
    efd::Serializer::SerializeObject(m_afPt[1], ar);
    efd::Serializer::SerializeObject(m_afPt[2], ar);
    efd::Serializer::SerializeObject(m_afPt[3], ar);
}

//------------------------------------------------------------------------------------------------
