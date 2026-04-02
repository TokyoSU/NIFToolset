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
#include <NiParticlePCH.h>

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING

#include "NiPSysFieldMaxDistanceCtlr.h"

NiImplementRTTI(NiPSysFieldMaxDistanceCtlr, NiPSysModifierFloatCtlr);

//--------------------------------------------------------------------------------------------------
NiPSysFieldMaxDistanceCtlr::NiPSysFieldMaxDistanceCtlr()
{
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSysFieldMaxDistanceCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSysFieldMaxDistanceCtlr::LoadBinary(NiStream& kStream)
{
    NiPSysModifierFloatCtlr::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSysFieldMaxDistanceCtlr::LinkObject(NiStream& kStream)
{
    NiPSysModifierFloatCtlr::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSysFieldMaxDistanceCtlr::RegisterStreamables(NiStream& kStream)
{
    return NiPSysModifierFloatCtlr::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSysFieldMaxDistanceCtlr::SaveBinary(NiStream& kStream)
{
    NiPSysModifierFloatCtlr::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSysFieldMaxDistanceCtlr::IsEqual(NiObject* pkObject)
{
    return NiPSysModifierFloatCtlr::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
