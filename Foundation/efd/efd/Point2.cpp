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

#include <efd/Point2.h>
#include <efd/Serialize.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
const Point2 Point2::ZERO(0.0f,0.0f);
const Point2 Point2::UNIT_X(1.0f,0.0f);
const Point2 Point2::UNIT_Y(0.0f,1.0f);

//------------------------------------------------------------------------------------------------
void Point2::Serialize(efd::Archive& ar)
{
    efd::Serializer::SerializeObject(x, ar);
    efd::Serializer::SerializeObject(y, ar);
}

//------------------------------------------------------------------------------------------------
