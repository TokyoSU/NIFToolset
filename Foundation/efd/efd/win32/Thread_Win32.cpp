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
//-------------------------------------------------------------------------------------------------
#include "efdPCH.h"

#include <efd/Thread.h>
#include <efd/ILogger.h>

using namespace efd;

//-------------------------------------------------------------------------------------------------
// The information on how to inform the debugger of a thread name on Windows comes from:
//    http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
// It works by throwing a structured exception (which gets immediately caught by the application if
// no debugger is present) with a payload consisting of a magic number and a pointer to the name.
//-------------------------------------------------------------------------------------------------
#define MS_VC_EXCEPTION 0x406D1388

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType;     // Must be 0x1000.
   LPCSTR szName;    // Pointer to name (in user address space)
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void InformDebuggerOfThreadName(DWORD dwThreadID, const char* threadName)
{
   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = threadName;
   info.dwThreadID = dwThreadID;
   info.dwFlags = 0;

   __try
   {
      RaiseException(MS_VC_EXCEPTION,
          0,
          sizeof(info)/sizeof(ULONG_PTR),
          reinterpret_cast<ULONG_PTR*>(&info));
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
   }
}
//-------------------------------------------------------------------------------------------------
bool Thread::SystemCreateThread()
{
    if (!m_pProcedure)
        return false;

    DWORD threadID;
    m_hThread = CreateThread(
        NULL,
        m_stackSize,
        ThreadProc,
        this,
        CREATE_SUSPENDED,
        &threadID);

    if (m_hThread == 0)
        return false;

    // Name the thread
    if (GetName())
        InformDebuggerOfThreadName(threadID, GetName());

    m_priority = NORMAL;
    m_status = SUSPENDED;

    // Initialize the thread affinity based on the process affinity
    DWORD_PTR processAffinityMask;
    DWORD_PTR systemAffinityMask;

    if (GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask,
        &systemAffinityMask))
    {
        m_affinity.SetAffinityMask(
            static_cast<unsigned int>(processAffinityMask));
    }

    return true;
}
//-------------------------------------------------------------------------------------------------
bool Thread::SystemSetPriority(Priority priority)
{
    if (m_priority != priority)
    {
        SInt32 threadPriority;

        switch (priority)
        {
        case ABOVE_NORMAL:
            threadPriority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case BELOW_NORMAL:
            threadPriority = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        case HIGHEST:
            threadPriority = THREAD_PRIORITY_HIGHEST;
            break;
        case IDLE:
            threadPriority = THREAD_PRIORITY_IDLE;
            break;
        case LOWEST:
            threadPriority = THREAD_PRIORITY_LOWEST;
            break;
        case NORMAL:
            threadPriority = THREAD_PRIORITY_NORMAL;
            break;
        case TIME_CRITICAL:
            threadPriority = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        default:
            return false;
        }
        if (!SetThreadPriority(m_hThread, threadPriority))
            return false;

        m_priority = priority;
    }

    return true;
}
//-------------------------------------------------------------------------------------------------
bool Thread::SystemSetAffinity(const ProcessorAffinity& affinity)
{
    DWORD_PTR mask = affinity.GetAffinityMask();

    // Try to get the process affinity and use it to mask the thread affinity
    DWORD_PTR processAffinityMask;
    DWORD_PTR systemAffinityMask;

    if (GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask,
        &systemAffinityMask))
    {
        mask &= processAffinityMask;
    }

    DWORD_PTR prevMask = SetThreadAffinityMask(m_hThread, mask);

#if defined(EE_EFD_CONFIG_DEBUG)
    if (prevMask == 0)
    {
        EE_OUTPUT_DEBUG_STRING("Thread::SystemSetAffinity failed.\n");
        DWORD_PTR systemMask = 0;
        DWORD_PTR processMask = 0;
        BOOL ret = GetProcessAffinityMask(GetCurrentProcess(), &processMask,
            &systemMask);

        if (ret)
        {
            char pOutput[64];
            Sprintf(pOutput, 64, "Affinity Mask: 0x%08X\n", mask);
            EE_OUTPUT_DEBUG_STRING(pOutput);
            Sprintf(pOutput, 64, "Process Affinity Mask: 0x%08X\n",
                processMask);
            EE_OUTPUT_DEBUG_STRING(pOutput);
            Sprintf(pOutput, 64, "System Affinity Mask: 0x%08X\n",
                systemMask);
            EE_OUTPUT_DEBUG_STRING(pOutput);

            if (mask & ~processMask)
            {
                EE_OUTPUT_DEBUG_STRING("Affinity mask must be a subset of "
                    "process affinity mask.\n");
            }
        }
    }
#endif

    return (prevMask != 0);
}
//-------------------------------------------------------------------------------------------------
SInt32 Thread::SystemSuspend()
{
    if (m_hThread == 0)
        return -1;

    SInt32 ret = SuspendThread(m_hThread);
    if (ret != -1)
        m_status = SUSPENDED;
    return ret;
}
//-------------------------------------------------------------------------------------------------
SInt32 Thread::SystemResume()
{
    if (m_hThread == 0)
        return -1;

    int previousSuspendCount = ResumeThread(m_hThread);
    switch (previousSuspendCount)
    {
    case -1:
        break;
    case 0: // fall through
    case 1:
        m_status = RUNNING;
        break;
    default:
        m_status = SUSPENDED;
    }
    return previousSuspendCount;
}
//-------------------------------------------------------------------------------------------------
bool Thread::SystemWaitForCompletion()
{
    if (m_status == RUNNING)
    {
        WaitForSingleObject(m_hThread, INFINITE);

        // Set the status to complete here because it fixes a race
        // condition between the ThreadProc setting COMPLETE and then
        // SystemResume setting RUNNING.  The value might be stale (i.e.
        // the thread is complete, but says it is running) between
        // Resume/WaitForCompletion, but it will be correct after a
        // WaitForCompletion().
        m_status = COMPLETE;

        return true;
    }
    else
    {
        return false;
    }
}
//-------------------------------------------------------------------------------------------------
