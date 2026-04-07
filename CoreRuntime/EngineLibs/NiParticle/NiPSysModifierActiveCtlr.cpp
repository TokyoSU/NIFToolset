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

#include "NiPSysModifierActiveCtlr.h"

NiImplementRTTI(NiPSysModifierActiveCtlr, NiPSysModifierBoolCtlr);

//--------------------------------------------------------------------------------------------------
NiPSysModifierActiveCtlr::NiPSysModifierActiveCtlr()
{
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSysModifierActiveCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSysModifierActiveCtlr::LoadBinary(NiStream& kStream)
{
    NiPSysModifierBoolCtlr::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSysModifierActiveCtlr::LinkObject(NiStream& kStream)
{
    NiPSysModifierBoolCtlr::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSysModifierActiveCtlr::RegisterStreamables(NiStream& kStream)
{
    return NiPSysModifierBoolCtlr::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSysModifierActiveCtlr::SaveBinary(NiStream& kStream)
{
    NiPSysModifierBoolCtlr::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSysModifierActiveCtlr::IsEqual(NiObject* pkObject)
{
    return NiPSysModifierBoolCtlr::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
