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

#include <efd/LoopedThreadProcedure.h>
#include <efd/LoopedThread.h>

using namespace efd;
//-------------------------------------------------------------------------------------------------
bool LoopedThreadProcedure::LoopedProcedure(efd::Thread*)
{
    EE_FAIL("LoopedThreadProcedure::LoopedProcedure() should never run. There "
        "may be a signature mismatch in the implementation of LoopedProcedure() "
        "in a class derived from LoopedThreadProcedure.");
    return false;
}
//-------------------------------------------------------------------------------------------------
UInt32 LoopedThreadProcedure::Execute(efd::Thread* pArg)
{
    LoopedThread* pThread = static_cast<LoopedThread*>(pArg);

    pThread->WaitStart();
    while (!pThread->GetLastLoop())
    {
        if (!LoopedProcedure(pThread))
        {
            // If return value is false, then the thread explicitly
            // terminates itself.
            pThread->SignalComplete();
            return PROC_TERMINATED;
        }

        pThread->SignalComplete();
        pThread->WaitStart();
    }

    // If last loop was set, then thread is terminated by some controlling
    // thread.
    pThread->SignalComplete();
    return THREAD_TERMINATED;
}
//-------------------------------------------------------------------------------------------------
