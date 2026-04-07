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

#include <efd/Helpers.h>
#include <efd/ILogger.h>
#include <efd/Asserts.h>
#include <efd/StackUtils.h>

using namespace efd;

//--------------------------------------------------------------------------------------------------
// Assert Helper Class
//--------------------------------------------------------------------------------------------------
AssertHandler AssertHelper::ms_assertHandler = NULL; /*static*/
bool AssertHelper::ms_alreadyInDoAssert = false;

//--------------------------------------------------------------------------------------------------
efd::SInt8 AssertHelper::DoAssert(const char* pPredicate, const char* pFile, efd::SInt32 line,
    const char* pFunction, const char* pMessage)
{
    if (ms_alreadyInDoAssert)
    {
        _LogMemAssert("Recursive assert", NULL, __FILE__, __LINE__, __FUNCTION__);
        return kDebugAbort;
    }
    ms_alreadyInDoAssert = true;


    if (!pMessage)
    {
        pMessage = pPredicate;
    }

    char* pStack = NULL;
#if defined(EE_USE_ASSERT_STACKTRACE)
    char trace[1024];
    efd::StackUtils::StackTrace(12, trace, EE_ARRAYSIZEOF(trace), 0, "\t");
    if (trace[0])
    {
        pStack = trace;
    }
#endif

    // Send the message to the logging system
    efd::GetLogger()->AssertMessage(pFile, line, pFunction, pPredicate, pMessage, pStack);

    efd::SInt8 retVal = kDebugAbort;
    // If there is an assert handler then call it
    if (ms_assertHandler)
    {
        retVal = (*ms_assertHandler)(pFile, line, pFunction, pPredicate, pMessage, pStack, false);
    }

    // Return the value indicating if the assert should be ignored, ignored once, or debugged
    ms_alreadyInDoAssert = false;
    return retVal;
}

//--------------------------------------------------------------------------------------------------
efd::SInt8 AssertHelper::DoVerify(const char* pPredicate, const char* pFile, efd::SInt32 line,
    const char* pFunction, const char* pMessage)
{
    if (ms_alreadyInDoAssert)
    {
        _LogMemAssert("Recursive verify", NULL, __FILE__, __LINE__, __FUNCTION__);
        return kDebugAbort;
    }
    ms_alreadyInDoAssert = true;

    if (!pMessage)
    {
        pMessage = pPredicate;
    }

    char* pStack = NULL;
#if defined(EE_USE_ASSERT_STACKTRACE)
    char trace[1024];
    efd::StackUtils::StackTrace(12, trace, EE_ARRAYSIZEOF(trace), 1, "\t");
    if (trace[0])
    {
        pStack = trace;
    }
#endif

    // Send the message to the logging system
    efd::GetLogger()->AssertMessage(pFile, line, pFunction, pPredicate, pMessage, pStack);

    SInt8 retVal = kDebugAbort;
    // If there is an assert handler then call it
    if (ms_assertHandler)
    {
        retVal = (*ms_assertHandler)(pFile, line, pFunction, pPredicate, pMessage, pStack, true);
    }

    // If "friendly verify" is not supported then we need to break here, which means we enter
    // the debugger one call stack deeper than we really want.  When friendly verify is supported
    // then simply returning kDebugAbort will cause us to enter the debugger in the correct place.
#if !defined(EE_SUPPORTS_FRIENDLY_VERIFY)
    // If the user requested the program break, then so it
    if (retVal == kDebugAbort)
        EE_DEBUG_BREAK();
#endif

    ms_alreadyInDoAssert = false;
    return retVal;
}

//--------------------------------------------------------------------------------------------------
AssertHandler AssertHelper::GetAssertHandler()
{
    return ms_assertHandler;
}

//--------------------------------------------------------------------------------------------------
AssertHandler AssertHelper::SetAssertHandler(AssertHandler handler)
{
    // Save the current handler to be returned
    AssertHandler retVal = ms_assertHandler;
    // Save the new handler
    ms_assertHandler = handler;
    // return the old handler
    return retVal;
}

efd::SInt8 AssertHelper::IgnoringAssertHandler(
    const char* /*pFile*/,
    efd::SInt32 /*line*/,
    const char* /*pFunction*/,
    const char* /*pPredicate*/,
    const char* /*pMessage*/,
    const char* /*pStackTrace*/,
    efd::Bool /*isAVerify*/)
{
    return kDebugAbort;
}

efd::SInt8 AssertHelper::StandardAssertHandler(
    const char* /*pFile*/,
    efd::SInt32 /*line*/,
    const char* /*pFunction*/,
    const char* /*pPredicate*/,
    const char* /*pMessage*/,
    const char* /*pStackTrace*/,
    efd::Bool /*isAVerify*/)
{
    return kDebugAbort;
}


//--------------------------------------------------------------------------------------------------
// PrintfHelper Class
//--------------------------------------------------------------------------------------------------
PrintfHelper::PrintfHelper(const char* pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    efd::Vsnprintf(m_szBuffer, EE_ARRAYSIZEOF(m_szBuffer), EE_TRUNCATE, pszFormat, args);
    va_end(args);
}

//--------------------------------------------------------------------------------------------------
// Log Helper Class
//--------------------------------------------------------------------------------------------------
LogHelper::LogHelper(const char* pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);

    int bytesNeeded = efd::Vscprintf(pszFormat, args);
    if (bytesNeeded+1 <= (int)EE_ARRAYSIZEOF(m_szBuffer))
    {
        m_pszBuffer = m_szBuffer;
    }
    else
    {
        m_pszBuffer = EE_ALLOC(char, bytesNeeded+1);
    }
    int bytesUsed = efd::Vsnprintf(m_pszBuffer, bytesNeeded+1, EE_TRUNCATE, pszFormat, args);

    // Assert causes a warning on some other platforms in Release configs.
    EE_VERIFY(bytesUsed == bytesNeeded);

    EE_ASSERT(m_pszBuffer[bytesUsed] == '\0');

    va_end(args);
}

//--------------------------------------------------------------------------------------------------
LogHelper::~LogHelper()
{
    if (m_pszBuffer != m_szBuffer)
    {
        EE_FREE(m_pszBuffer);
    }
}

//--------------------------------------------------------------------------------------------------
