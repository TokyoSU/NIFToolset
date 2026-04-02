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
#include "NiSystemPCH.h"
#include "NiDebug.h"
#include "NiSystem.h"

// This function is designed to only log and not trigger a breakpoint
efd::SInt8 NiAssertFail::SimpleAssertFail(
    const char* pcFile,
    efd::SInt32 iLine,
    const char* /*pcFunc*/,
    const char* pcPred,
    const char* /*pcMsg*/,
    const char* /*pcStack*/,
    efd::Bool /*isAVerify*/)
{
    char acString[1024];
    NiSprintf(acString, 1024, "*** Assertion Failure***\n"
        "%s(%d) : Expression \"%s\"\n", pcFile, iLine, pcPred);
    NiOutputDebugString(acString);

    return efd::AssertHelper::kIgnore;
}
