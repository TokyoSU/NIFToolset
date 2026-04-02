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

#include <efd/Color.h>
#include <efd/Serialize.h>

using namespace efd;

//------------------------------------------------------------------------------------------------

const Color Color::BLACK(0.0f,0.0f,0.0f);
const Color Color::WHITE(1.0f,1.0f,1.0f);

const ColorA ColorA::BLACK(0.0f,0.0f,0.0f,1.0f);
const ColorA ColorA::WHITE(1.0f,1.0f,1.0f,1.0f);

const ColorA_UInt8 ColorA_UInt8::BLACK(0,0,0,255);
const ColorA_UInt8 ColorA_UInt8::WHITE(255,255,255,255);

//------------------------------------------------------------------------------------------------
void Color::Serialize(efd::Archive& ar)
{
    Serializer::SerializeObject(r, ar);
    Serializer::SerializeObject(g, ar);
    Serializer::SerializeObject(b, ar);
}

//------------------------------------------------------------------------------------------------
void ColorA::Serialize(efd::Archive& ar)
{
    Serializer::SerializeObject(r, ar);
    Serializer::SerializeObject(g, ar);
    Serializer::SerializeObject(b, ar);
    Serializer::SerializeObject(a, ar);
}

//------------------------------------------------------------------------------------------------
void ColorA_UInt8::Serialize(efd::Archive& ar)
{
    // Serialize byte-wise to avoid endianness weirdness:
    Serializer::SerializeObject(m_kColor.r, ar);
    Serializer::SerializeObject(m_kColor.g, ar);
    Serializer::SerializeObject(m_kColor.b, ar);
    Serializer::SerializeObject(m_kColor.a, ar);
}

