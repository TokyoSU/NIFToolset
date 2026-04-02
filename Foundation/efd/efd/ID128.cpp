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
#include <efd/ID128.h>
#include <efd/Serialize.h>

//------------------------------------------------------------------------------------------------
void efd::ID128::Serialize(efd::Archive& ar)
{
    // value is a byte array so there are no endianness issues, just block copy it:
    efd::Serializer::SerializeRawBytes(m_value, EE_ARRAYSIZEOF(m_value), ar);
}

//------------------------------------------------------------------------------------------------
