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

// Precompiled Header
#include "NiMainPCH.h"

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING

#include "NiParticles.h"

NiImplementRTTI(NiParticles, NiGeometry);

//--------------------------------------------------------------------------------------------------
NiParticles::NiParticles()
{
}

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiParticles);

//--------------------------------------------------------------------------------------------------
void NiParticles::LoadBinary(NiStream& stream)
{
    NiGeometry::LoadBinary(stream);
}

//--------------------------------------------------------------------------------------------------
void NiParticles::LinkObject(NiStream& stream)
{
    NiGeometry::LinkObject(stream);
}

//--------------------------------------------------------------------------------------------------
bool NiParticles::RegisterStreamables(NiStream& stream)
{
    return NiGeometry::RegisterStreamables(stream);
}

//--------------------------------------------------------------------------------------------------
void NiParticles::SaveBinary(NiStream& stream)
{
    NiGeometry::SaveBinary(stream);
}

//--------------------------------------------------------------------------------------------------
bool NiParticles::IsEqual(NiObject* pObject)
{
    return NiGeometry::IsEqual(pObject);
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
