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

//  **  Deprecated class - no longer load/save the weight info.  **

// Precompiled Header
#include "NiMainPCH.h"

#include "NiVertWeightsExtraData.h"

NiImplementRTTI(NiVertWeightsExtraData,NiExtraData);

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiVertWeightsExtraData);

//--------------------------------------------------------------------------------------------------
void NiVertWeightsExtraData::CopyMembers(NiVertWeightsExtraData* pDest,
    NiCloningProcess& kCloning)
{
    NiExtraData::CopyMembers(pDest, kCloning);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiVertWeightsExtraData);

//--------------------------------------------------------------------------------------------------

// Deprecated class - no longer load the weight info into member variables.

void NiVertWeightsExtraData::LoadBinary(NiStream&)
{
}

//--------------------------------------------------------------------------------------------------
void NiVertWeightsExtraData::LinkObject(NiStream& kStream)
{
    NiExtraData::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiVertWeightsExtraData::RegisterStreamables(NiStream& kStream)
{
    return NiExtraData::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------

// Deprecated class - no longer save the weight info.

void NiVertWeightsExtraData::SaveBinary(NiStream&)
{
}

//--------------------------------------------------------------------------------------------------
bool NiVertWeightsExtraData::IsEqual(NiObject* pObject)
{
    if (!NiExtraData::IsEqual(pObject))
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiVertWeightsExtraData::GetViewerStrings(NiViewerStringsArray* pStrings)
{
    NiExtraData::GetViewerStrings(pStrings);

    pStrings->Add(NiGetViewerString(
        NiVertWeightsExtraData::ms_RTTI.GetName()));
}

//--------------------------------------------------------------------------------------------------
