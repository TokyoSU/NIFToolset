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

#include "NiMainPCH.h"

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING

#include "NiScreenElements.h"

NiImplementRTTI(NiScreenElements,NiTriShape);

//--------------------------------------------------------------------------------------------------
NiScreenElements::NiScreenElements()
{
}

//--------------------------------------------------------------------------------------------------
NiScreenElements::~NiScreenElements()
{
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiScreenElements);

//--------------------------------------------------------------------------------------------------
void NiScreenElements::CopyMembers(NiScreenElements* pkDest,
    NiCloningProcess& kCloning)
{
    NiTriShape::CopyMembers(pkDest, kCloning);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiScreenElements);

//--------------------------------------------------------------------------------------------------
void NiScreenElements::LoadBinary(NiStream& kStream)
{
    NiTriShape::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiScreenElements::LinkObject(NiStream& kStream)
{
    NiTriShape::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiScreenElements::RegisterStreamables(NiStream& kStream)
{
    return NiTriShape::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiScreenElements::SaveBinary(NiStream& kStream)
{
    NiTriShape::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiScreenElements::IsEqual(NiObject* pkObject)
{
    return NiTriShape::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
