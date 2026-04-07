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

#include "NiTerrainPCH.h"
#include "NiTerrainTaskProcessor.h"

#include "NiTerrainStreamingTask.h"

//--------------------------------------------------------------------------------------------------
void NiTerrainTaskQueue::SubmitTask(TaskType* pkNewTask)
{
    m_kMutex.Lock();
    pkNewTask->SetWaiting(true);

#ifdef NITERRAIN_THREADING_DEBUG
    NiString kString;
    pkNewTask->ToString("Submitting", kString);
    NiOutputDebugString(kString);
    NiOutputDebugString("\n");
#endif

    // How many work units are required? We need to add one entry to the queue for each work unit.
    WorkUnit kWorkUnit;
    kWorkUnit.m_spTask = pkNewTask;
    NiUInt32 uiNumWorkUnits = pkNewTask->GetCurrentTotalWorkUnits();
    for (NiUInt32 ui = 0; ui < uiNumWorkUnits; ++ui)
    {
        kWorkUnit.m_uiWorkUnitID = ui;
        m_kWorkQueue.push(kWorkUnit);
    }

    m_kMutex.Unlock();
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainTaskQueue::ReserveWorkUnit()
{
    bool bAquired = false;
    m_kMutex.Lock();
    
    if (m_uiNumReservedWorkUnits < m_kWorkQueue.size())
    {
        ++m_uiNumReservedWorkUnits;
        ++m_uiNumActiveWorkUnits;
        bAquired = true;
    }

    m_kMutex.Unlock();
    return bAquired;
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiTerrainTaskQueue::GetActiveWorkUnitCount()
{
    NiUInt32 uiCount;
    m_kMutex.Lock();
    uiCount = m_uiNumActiveWorkUnits;
    m_kMutex.Unlock();

    return uiCount;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainTaskQueue::ProcessWorkUnit()
{
    // Find the next task to be executed
    m_kMutex.Lock();
    EE_ASSERT(m_kWorkQueue.size());
    WorkUnit kWorkUnit = m_kWorkQueue.front();

    m_kWorkQueue.pop();

    EE_ASSERT(m_uiNumReservedWorkUnits != 0);
    m_uiNumReservedWorkUnits--;
    m_kMutex.Unlock();

    kWorkUnit.m_spTask->SetWaiting(false);

#ifdef NITERRAIN_THREADING_DEBUG
    NiString kRunString, kFinString;

    kWorkUnit.ToString("Running", kRunString);
    kWorkUnit.ToString("Completed", kFinString);

    NiOutputDebugString(kRunString);
    NiOutputDebugString("\n");
#endif

    // Process that task
    if (m_kProcessCallback)
    {
        (*m_kProcessCallback)(m_pvCallbackArgument, kWorkUnit.m_spTask, kWorkUnit.m_uiWorkUnitID);
    }

    m_kMutex.Lock();
    EE_ASSERT(m_uiNumActiveWorkUnits != 0);
    --m_uiNumActiveWorkUnits;
    m_kMutex.Unlock();

#ifdef NITERRAIN_THREADING_DEBUG
    NiOutputDebugString(kFinString);
    NiOutputDebugString("\n");
#endif
}

//--------------------------------------------------------------------------------------------------
void NiTerrainTaskQueue::WorkUnit::ToString(const char* pcPrefix, NiString& kString)
{
    if (m_spTask != NULL)
    {
        NiString kWorkingString;
        m_spTask->ToString(pcPrefix, kWorkingString);
        kString.Format("%s current unit: %d", (const char*)kWorkingString, m_uiWorkUnitID);
    }
    else
    {
        kString.Format("%s invalid task, work unit: %d", pcPrefix, m_uiWorkUnitID);
    }
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
void NiTerrainTaskProcessor::ExecuteNextWorkUnit()
{
    EE_ASSERT(m_kAquireCallback);

    // Select the queue to execute from
    m_kLock.Lock();
    NiTerrainTaskQueue* pkQueue = (*m_kAquireCallback)(m_pvCallbackArgument);
    if (!pkQueue)
    {
        m_kTaskNotified.Wait();
        m_kLock.Unlock();
    }
    else
    {
        m_kLock.Unlock();
        pkQueue->ProcessWorkUnit();
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainTaskProcessor::Shutdown()
{
    // NOTE: This function will end the processing of tasks even if there are some leftover
    // Force all the threads to finish what they are working on
    m_kLock.Lock();
    SetNumWorkers(0);

    // Insert a signal to wake up all sleeping threads
    m_kTaskNotified.Broadcast();
    m_kLock.Unlock();

    // Loop through all the threads ever created and wait for them to finish
    ThreadList::iterator kIter;
    for (kIter = m_kThreadPool.begin(); kIter != m_kThreadPool.end(); ++kIter)
    {
        efd::Thread* pkThread = *kIter;
        EE_ASSERT(pkThread);
        pkThread->WaitForCompletion();
        EE_ASSERT(pkThread->GetStatus() == efd::Thread::COMPLETE);
    }
}
//--------------------------------------------------------------------------------------------------
