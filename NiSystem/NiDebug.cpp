// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

// Precompiled Header
#include "NiSystemPCH.h"
#include "NiDebug.h"
#include "NiSystem.h"

NISYSTEM_ENTRY NiAssertFailProcFuncPtr NiAssertFail::ms_pfnNiAssertFailProc = NiAssertFail::DefaultAssertFail;

bool NiAssertFail::SimpleAssertFail(
        const char* pcExpression,
        const char* pcFile,
        const char*,
        const int iLine)
{
    char acString[1024];
    NiSprintf(acString, 1024, "*** Assertion Failure***\n"
        "%s(%d) : Expression \"%s\"\n", pcFile, iLine, pcExpression);
    NiOutputDebugString(acString);
    return false;
}

bool NiAssertFail::DefaultAssertFail(
    const char* pcExpression,
    const char* pcFile,
    const char* pcFunction,
    const int iLine)
{
    static const char* pcCaption = "Assert Failed";
    char acString[512];
    NiSprintf(
        acString,
        sizeof(acString),
        "\n"
        "Assert Failed: %s\n\n"
        "File: %s\n"
        "Line: %d\n"
        "Function: %s\n",
        pcExpression,
        pcFile,
        iLine,
        pcFunction);

    fprintf(stderr, acString);

    int iButtonResult = MessageBox(
        NULL,
        acString,
        pcCaption,
        MB_ABORTRETRYIGNORE);

    switch (iButtonResult)
    {
    case IDABORT:
        exit(EXIT_FAILURE);
    default:
    case IDRETRY:
        NIDEBUGBREAK();
        return false;

    case IDIGNORE:
        return false;
    }
}
