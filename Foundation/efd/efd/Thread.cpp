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

#include <efd/Thread.h>
#include <efd/StaticDataManager.h>
#include <efd/ThreadLocal.h>

using namespace efd;

EE_DEFINE_STATIC_TLS(efd::Thread*, gs_tlsCurrentThread);

//-------------------------------------------------------------------------------------------------
Thread::Thread(ThreadFunctor* pProcedure, UInt32 stackSize, const char* pcThreadName):
    m_status(SUSPENDED),
    m_returnValue(0xFFFFFFFF),
#if defined(EE_PLATFORM_WIN32) || defined(EE_PLATFORM_XBOX360)
    m_hThread(0),
#elif defined(EE_PLATFORM_PS3) || defined(EE_PLATFORM_LINUX)
    m_resumed(false),
#endif
    m_pName(0)
{
    SetStackSize(stackSize);
    EE_ASSERT(pProcedure);
    SetProcedure(pProcedure);

    if (pcThreadName)
        SetName(pcThreadName);
}
//-------------------------------------------------------------------------------------------------
Thread::~Thread()
{
    WaitForCompletion();

#if defined(EE_PLATFORM_WIN32) || defined(EE_PLATFORM_XBOX360)
    m_pProcedure = 0;
    if (m_hThread)
        CloseHandle(m_hThread);
    m_hThread = 0;
#elif defined(EE_PLATFORM_PS3)
    m_pProcedure = 0;
    pthread_detach(m_threadID);
    pthread_mutex_destroy(&m_mutexID);
#elif defined(EE_PLATFORM_LINUX)
    m_returnValue = pthread_mutex_lock(&m_mutexID);
    EE_ASSERT(0 == m_returnValue);
    EE_ASSERT(m_pProcedure);
    m_pProcedure = 0;
    pthread_mutex_unlock(&m_mutexID);

    pthread_detach(m_threadID);
    pthread_mutex_destroy(&m_mutexID);
#endif

    EE_FREE(m_pName);
}
//-------------------------------------------------------------------------------------------------
Thread* Thread::GetThread()
{
    return gs_tlsCurrentThread;
}
//-------------------------------------------------------------------------------------------------
Thread* Thread::Create(ThreadFunctor* pProcedure, UInt32 stackSize, const char* pcName)
{
    Thread* pThread = EE_NEW Thread(pProcedure, stackSize, pcName);
    if (pThread)
    {
        if (!pThread->SystemCreateThread())
        {
            EE_DELETE pThread;
            pThread = 0;
        }
    }

    return pThread;
}
//-------------------------------------------------------------------------------------------------
bool Thread::SetPriority(Thread::Priority priority)
{
    return SystemSetPriority(priority);
}
//-------------------------------------------------------------------------------------------------
int Thread::Suspend()
{
    return SystemSuspend();
}
//-------------------------------------------------------------------------------------------------
int Thread::Resume()
{
    return SystemResume();
}
//-------------------------------------------------------------------------------------------------
bool Thread::WaitForCompletion()
{
    return SystemWaitForCompletion();
}
//-------------------------------------------------------------------------------------------------
#if defined (EE_PLATFORM_WIN32) || defined (EE_PLATFORM_XBOX360)
DWORD WINAPI Thread::ThreadProc(void* pArg)
{
    Thread* pThread = (Thread*)pArg;
    gs_tlsCurrentThread = pThread;

    // Verify that the thread local storage is what we expect
    EE_ASSERT(gs_tlsCurrentThread == pThread);

    StaticDataManager::PerThreadInit();
    pThread->m_returnValue = pThread->GetProcedure()->Execute(pThread);
    StaticDataManager::PerThreadShutdown();

    // NOTE: the setting of m_eStatus here is not entirely thread-safe
    // with the checking of it in SystemWaitForCompletion().  However,
    // because the return value has already been set, the only side-effect
    // is a potentially stale return value in SystemWaitForCompletion().
    UInt32 returnValue = pThread->m_returnValue;
    pThread->m_status = COMPLETE;
    return returnValue;
}
//-------------------------------------------------------------------------------------------------
#elif defined (EE_PLATFORM_PS3)
void* Thread::ThreadProc(void* pArg)
{
    Thread* pThread = (Thread*)pArg;
    gs_tlsCurrentThread = pThread;

    // Verify that the thread local storage is what we expect
    EE_ASSERT(gs_tlsCurrentThread == pThread);

    EE_VERIFY(pthread_mutex_lock(&pThread->m_mutexID) == 0);

    if (pThread->m_resumed)
    {
        StaticDataManager::PerThreadInit();
        pThread->m_returnValue = pThread->GetProcedure()->Execute(pThread);
        StaticDataManager::PerThreadShutdown();
    }

    // NOTE: the setting of m_eStatus here is not entirely thread-safe
    // with the checking of it in SystemWaitForCompletion().  However,
    // because the return value has already been set, the only side-effect
    // is a potentially stale return value in SystemWaitForCompletion().
    pThread->m_status = COMPLETE;
    pthread_mutex_unlock(&pThread->m_mutexID);

    // Note that pthread_exit is implicitly called when this method returns.
    return NULL;
}
//-------------------------------------------------------------------------------------------------
#elif defined (EE_PLATFORM_LINUX)
void* Thread::ThreadProc(void* pArg)
{
    Thread* pThread = (Thread*)pArg;
    gs_tlsCurrentThread = pThread;

    // Verify that the thread local storage is what we expect
    EE_ASSERT(gs_tlsCurrentThread == pThread);

    pThread->m_returnValue = pthread_mutex_lock(&pThread->m_mutexID);
    EE_ASSERT(0 == pThread->m_returnValue);

    EE_ASSERT(pThread->GetProcedure());

    StaticDataManager::PerThreadInit();
    pThread->m_returnValue = pThread->GetProcedure()->Execute(pThread);
    StaticDataManager::PerThreadShutdown();

    // NOTE: the setting of m_eStatus here is not entirely thread-safe
    // with the checking of it in SystemWaitForCompletion().  However,
    // because the return value has already been set, the only side-effect
    // is a potentially stale return value in SystemWaitForCompletion().
    pThread->m_status = COMPLETE;
    pthread_mutex_unlock(&pThread->m_mutexID);

    // Note that pthread_exit is implicitly called when this method returns.
    return NULL;
}
#endif
//-------------------------------------------------------------------------------------------------
