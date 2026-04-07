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

#include "NiPSysResetOnLoopCtlr.h"

NiImplementRTTI(NiPSysResetOnLoopCtlr, NiTimeController);

//--------------------------------------------------------------------------------------------------
NiPSysResetOnLoopCtlr::NiPSysResetOnLoopCtlr()
{
}

//--------------------------------------------------------------------------------------------------
void NiPSysResetOnLoopCtlr::Update(float)
{
    EE_FAIL("This class is deprecated and should only be used for "
        "streaming!");
}

//--------------------------------------------------------------------------------------------------
bool NiPSysResetOnLoopCtlr::TargetIsRequiredType() const
{
     EE_FAIL("This class is deprecated and should only be used for "
        "streaming!");
    return false;
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSysResetOnLoopCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSysResetOnLoopCtlr::LoadBinary(NiStream& kStream)
{
    NiTimeController::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSysResetOnLoopCtlr::LinkObject(NiStream& kStream)
{
    NiTimeController::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSysResetOnLoopCtlr::RegisterStreamables(NiStream& kStream)
{
    return NiTimeController::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSysResetOnLoopCtlr::SaveBinary(NiStream& kStream)
{
    NiTimeController::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSysResetOnLoopCtlr::IsEqual(NiObject* pkObject)
{
    return NiTimeController::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
