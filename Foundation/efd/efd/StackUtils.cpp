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
#include <efd/StackUtils.h>
#include <efd/Asserts.h>

// This file provides "no-op" implementations of the StackUtil methods when stack tracing is
// disabled.  When enabled, the implemenation is always platform specific so check the platform
// specific file for details.

#if !defined(EE_ENABLE_STACKTRACE)

//------------------------------------------------------------------------------------------------
bool efd::StackUtils::EnableStackTracing(bool)
{
    return false;
}

//------------------------------------------------------------------------------------------------
void efd::StackUtils::TurnOffStackTracing()
{
}

//------------------------------------------------------------------------------------------------
efd::UInt32 efd::StackUtils::FastStackTrace(
    void** o_pResults,
    efd::UInt32 i_maxDepth,
    efd::UInt32 i_skipFrames)
{
    EE_UNUSED_ARG(o_pResults);
    EE_UNUSED_ARG(i_maxDepth);
    EE_UNUSED_ARG(i_skipFrames);
    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 efd::StackUtils::ExceptionStackTrace(
    void** o_pResults,
    efd::UInt32 i_maxDepth,
    void* i_pPlatform1,
    void* i_pPlatform2)
{
    EE_UNUSED_ARG(o_pResults);
    EE_UNUSED_ARG(i_maxDepth);
    EE_UNUSED_ARG(i_pPlatform1);
    EE_UNUSED_ARG(i_pPlatform2);
    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 efd::StackUtils::StackTrace(
    efd::UInt32 i_maxDepth,
    char* o_pszzResultBuffer,
    efd::UInt32 i_cchBufferSize,
    efd::UInt32 i_skipFrames,
    const char* i_pszPrefix)
{
    EE_UNUSED_ARG(i_maxDepth);
    EE_UNUSED_ARG(i_skipFrames);
    EE_UNUSED_ARG(i_cchBufferSize);
    EE_UNUSED_ARG(i_pszPrefix);
    EE_ASSERT(i_cchBufferSize > 0);
    o_pszzResultBuffer[0] = '\0';
    return 0;
}

//------------------------------------------------------------------------------------------------
bool efd::StackUtils::ResolveSymbolNames(
    const void* const * i_pSymbols,
    efd::UInt32 i_cSymbols,
    char* o_pszzResultBuffer,
    efd::UInt32 i_cchBufferSize,
    const char* i_pszPrefix)
{
    EE_UNUSED_ARG(i_pSymbols);
    EE_UNUSED_ARG(i_cSymbols);
    EE_UNUSED_ARG(i_cchBufferSize);
    EE_UNUSED_ARG(i_pszPrefix);
    EE_ASSERT(i_cchBufferSize > 0);
    o_pszzResultBuffer[0] = '\0';
    return false;
}

//------------------------------------------------------------------------------------------------
void efd::StackUtils::LogOnUnhandledException(bool i_turnOn)
{
    EE_UNUSED_ARG(i_turnOn);
}
#endif // !defined(EE_ENABLE_STACKTRACE)

