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

#include <efd/LoopedThread.h>

using namespace efd;
//-------------------------------------------------------------------------------------------------
LoopedThread::~LoopedThread()
{
    Shutdown();
    WaitForCompletion();
}
//-------------------------------------------------------------------------------------------------
LoopedThread* LoopedThread::Create(
    LoopedThreadProcedure* pProcedure,
    UInt32 stackSize,
    const char* pcThreadName)
{
    LoopedThread* pThread = EE_NEW LoopedThread(pProcedure, stackSize, pcThreadName);

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
LoopedThread::LoopedThread(LoopedThreadProcedure* pProcedure,
    UInt32 stackSize,
    const char* pcThreadName) :
        Thread(pProcedure, stackSize, pcThreadName),
        m_lastLoop(false),
        m_complete(0),
        m_start(0)
{
    // Initialize semaphores correctly
    SignalComplete();

    // m_kStart now has value 0 (max 1)
    // m_kComplete now has value 1 (max 1)
}
//-------------------------------------------------------------------------------------------------
void LoopedThread::DoLoop()
{
    // Set complete outside of the thread proc to avoid race condition
    // with WaitForLoopCondition()'s WaitComplete()/SignalComplete().
    WaitComplete();
    SignalStart();
}
//-------------------------------------------------------------------------------------------------
bool LoopedThread::WaitForLoopCompletion()
{
    // If in the middle of a loop, wait will block.  If not, it will
    // blow right through the wait/signal.
    WaitComplete();
    SignalComplete();
    return true;
}
//-------------------------------------------------------------------------------------------------
