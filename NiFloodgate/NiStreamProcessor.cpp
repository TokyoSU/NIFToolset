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

//---------------------------------------------------------------------------
#include "NiFloodgatePCH.h"
#include "NiStreamProcessor.h"
#include "NiFloodgateSDM.h"
#include "NiSPWorkflow.h"
#include "NiSystemDesc.h"

//---------------------------------------------------------------------------
NiStreamProcessor* NiStreamProcessor::ms_pkInstance = NULL;
NiThread::Priority NiStreamProcessor::ms_eThreadPriority = 
    NiThread::HIGHEST;

#if !defined(_PS3) && !defined(_WII)
NiStreamProcessor::DefineWorkerThreadCount 
    NiStreamProcessor::ms_pfnDefineWorkerThreadCountFunc = 
    NiStreamProcessor::DefineWorkerThreadCountDefaultFunc;

NiStreamProcessor::AssignDispatcherThreadAffinity 
    NiStreamProcessor::ms_pfnAssignDispatcherThreadAffinityFunc = 
    NiStreamProcessor::AssignDispatcherThreadAffinityDefaultFunc;
#endif

//---------------------------------------------------------------------------
NiStreamProcessor::NiStreamProcessor(NiUInt32 uiMaxQueueSize) :
    m_eStatus(STOPPED),
    m_uiMaxQueueSize(uiMaxQueueSize),
#if !defined(_WII)
    m_pkThreadPool(NULL),
    m_pkSubmitThread(NULL),
    m_pkSubmitThreadProc(NULL),
    m_kSubmitSemaphore(0, 
        NiStreamProcessor::NUM_PRIORITIES * uiMaxQueueSize + 1),
    m_pkEventQueue(NULL),
#endif
    m_bIsActive(false)
{
#if !defined(_PS3) && !defined(_WII)
    m_bParallelExecution = true;
#endif
}
//---------------------------------------------------------------------------
NiStreamProcessor::~NiStreamProcessor()
{
}
//---------------------------------------------------------------------------
void NiStreamProcessor::InitializePools()
{
    NiSPTask::InitializePools(128);
    NiSPTaskImpl::InitializePools(128);
    NiSPWorkflow::InitializePools(128);
}
//---------------------------------------------------------------------------
void NiStreamProcessor::ShutdownPools()
{
    NiSPWorkflow::ShutdownPools();
    NiSPTaskImpl::ShutdownPools();
    NiSPTask::ShutdownPools();
}
//---------------------------------------------------------------------------
void NiStreamProcessor::Create(NiUInt32 uiMaxQueueSize)
{
    // Create instance
    ms_pkInstance = NiNew NiStreamProcessor(uiMaxQueueSize);

#if !defined(_PS3) && !defined(_WII)
    const NiInitOptions* pkInitOptions = NiStaticDataManager::GetInitOptions();
    ms_pkInstance->SetParallelExecution(
        ((NiInitOptions*)pkInitOptions)->GetParallelExecution());
#endif

    NIASSERT(ms_pkInstance);
}
//---------------------------------------------------------------------------
void NiStreamProcessor::Destroy()
{
    // Delete instance
    NIASSERT(ms_pkInstance);
    NiDelete ms_pkInstance;
    ms_pkInstance = NULL;
}
//---------------------------------------------------------------------------
bool NiStreamProcessor::Initialize()
{
    if (ms_pkInstance)
    {
        ms_pkInstance->InitializeInternal();
        ms_pkInstance->Start();
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------
bool NiStreamProcessor::Shutdown()
{
    // Stop the manager and wait for background thread to complete
    ms_pkInstance->Stop();

    // Perform internal shutdown
    ms_pkInstance->ShutdownInternal();

    return true;
}
//---------------------------------------------------------------------------
#if !defined(_PS3) && !defined(_WII)
//---------------------------------------------------------------------------
bool NiStreamProcessor::ExecuteOneWorkflowSerial(NiSPWorkflowPtr spWorkflow)
{
    // Ensure that we were not given a workflow that is currently being
    // executed. Since we are operating with a serial execution model 
    // this should always be the case.
    NIASSERT(spWorkflow->GetStatus() == NiSPWorkflow::PENDING);

    // Attempt to prepare the task
    if (spWorkflow->Prepare())
    {
        // Determine dependencies and 
        // execution order of contained tasks
        m_kAnalyzer.Analyze(spWorkflow);

        // Set workflow id on manager hash table
        NIASSERT(spWorkflow->GetId() != 0)

        // Update status and execute
        spWorkflow->SetStatus(NiSPWorkflow::RUNNING);

        spWorkflow->ExecuteSerial();

        // Ensure that the workflow completed. Since we are operating
        // with a serial execution model this should always be the case.
        NIASSERT(spWorkflow->GetStatus() == NiSPWorkflow::COMPLETED);

        return true;
    }

    return false;
}
#endif
//---------------------------------------------------------------------------
bool NiStreamProcessor::Poll(const NiSPWorkflow* pkWorkflow)
{
    NIASSERT(pkWorkflow);

    NiSPWorkflow::Status eStatus = pkWorkflow->GetStatus();
    if (eStatus == NiSPWorkflow::COMPLETED || eStatus == NiSPWorkflow::ABORTED)
    {
        // Memory synch on consoles.  Win32 handles it in the compiler via
        // the "volatile" keyword.  Wii doesn't need to sync since we are
        // always in serial execution mode.
#if defined(_XENON) || defined(_PS3)
        // We read a volatile flag that indicated the workflow was
        // complete. We need a memory barrier to insure that any reads
        // of the results are at least as current as that flag.
        __lwsync();
#endif
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------
bool NiStreamProcessor::Wait(const NiSPWorkflow* pkWorkflow, 
    NiUInt32 uiTimeout)
{
    // Poll the workflow.
    bool bDone = Poll(pkWorkflow);

    // If we're finished or the timeout is zero, return immediately without
    // executing any timeout logic or yielding the thread.
    if (bDone || !uiTimeout)
        return bDone;

    bool bInfinite = false;
    if (uiTimeout == 0xFFFFFFFF)
        bInfinite = true;

    NiUInt64 uiTimeoutTicks = 0;
    if (!bInfinite)
    {
        // Get an approximate number of ticks until the timeout occurs. Note
        // that we multiply by the frequency before dividing for increased 
        // precision since some platforms have performance counters in the 
        // 1-10 MHz range.
        NiUInt64 uiTicks = 
            ((NiUInt64)NiSystemDesc::GetSystemDesc().
            GetPerformanceCounterHz() * uiTimeout) / 1000000;

        // Determine the number of ticks returned from NiGetPerformanceCounter
        // when the timeout expires. Note that we scale the input argument in
        // microseconds to the performance counter's frequency.
        uiTimeoutTicks = NiGetPerformanceCounter() + uiTicks;
    }
    
    // Wait until our workflow is done or timeout has occurred
    while (bInfinite || (NiGetPerformanceCounter() < uiTimeoutTicks))
    {
        if (Poll(pkWorkflow))
            return true;
        NiYieldThread();
    }
    return false;
}
//---------------------------------------------------------------------------
NiStreamProcessor::Status NiStreamProcessor::Start()
{
    if (m_eStatus == NiStreamProcessor::RUNNING)
        return m_eStatus;

#if !defined(_WII)
    m_eStatus = NiStreamProcessor::RUNNING;

#if !defined(_PS3)
    if (!GetParallelExecution())
    {
        NIASSERT(m_pkSubmitThreadProc == NULL);
        NIASSERT(m_pkSubmitThread == NULL);

        m_eStatus = StartInternal();
        return m_eStatus;
    }
#endif

    m_pkSubmitThreadProc = NiNew NiStreamProcessor::ManagerProc(this);
    NIASSERT(m_pkSubmitThreadProc);

    m_pkSubmitThread = NiThread::Create(m_pkSubmitThreadProc);
    m_pkSubmitThread->SetPriority(ms_eThreadPriority);

#if !defined(_PS3)
    NiProcessorAffinity kAffinity;
    ms_pfnAssignDispatcherThreadAffinityFunc(kAffinity);
    m_pkSubmitThread->SetThreadAffinity(kAffinity);
#endif

    NIASSERT(m_pkSubmitThread);
    m_pkSubmitThread->SetName("Floodgate: Submit Thread");
    m_pkSubmitThread->Resume();
#endif
    
    m_eStatus = StartInternal();

    return m_eStatus;
}
//---------------------------------------------------------------------------
NiStreamProcessor::Status NiStreamProcessor::Stop()
{
    // Signal that we are stopping
    m_kManagerLock.Lock();
    m_eStatus = NiStreamProcessor::STOPPING;
    m_kManagerLock.Unlock();

#if defined(_WII)
    m_eStatus = StopInternal();
#else
    // Wake up the manager thread and allow it to come to a complete stop
#if defined(_PS3)
    // Submit a dummy workflow (NULL) to wake up the manager thread
    Submit(NULL, IMMEDIATE);
#else
    // Signal the update and poll semaphore to allow the thread to complete
    m_kSubmitSemaphore.Signal();
#endif

    if (m_pkSubmitThreadProc || m_pkSubmitThread)
    {
        m_pkSubmitThread->WaitForCompletion();

        // Stop the internal (platform-specific) systems
        m_kManagerLock.Lock();
        m_eStatus = StopInternal();
        
        NiDelete m_pkSubmitThread;
        NiDelete m_pkSubmitThreadProc;
        m_pkSubmitThread = 0;
        m_pkSubmitThreadProc = 0;
        m_kManagerLock.Unlock();
    }
#endif

    return m_eStatus;
}
//---------------------------------------------------------------------------
void NiStreamProcessor::Clear(NiSPWorkflow* pkWorkflow)
{
    m_kManagerLock.Lock();

    pkWorkflow->Reset();
    pkWorkflow->SetStatus(NiSPWorkflow::IDLE);

    m_kManagerLock.Unlock();
}
//---------------------------------------------------------------------------
const NiUInt32 NiStreamProcessor::GetActiveWorkflowCount() const
{
#if defined(_WII)
    return 0;
#elif defined(_PS3)
    return m_pkSubmitThreadProc->GetActiveWorkflowCount();
#else
    if (m_bParallelExecution)
        return m_pkSubmitThreadProc->GetActiveWorkflowCount();
    return 0;
#endif
}
//---------------------------------------------------------------------------
#if !defined(_WII)
//---------------------------------------------------------------------------
// ManagerProc functions.
//---------------------------------------------------------------------------
NiStreamProcessor::ManagerProc::ManagerProc(NiStreamProcessor* pkManager) : 
    m_pkManager(pkManager),
    m_uiActiveWorkflowCount(0)
{
}
//---------------------------------------------------------------------------
const NiUInt32 NiStreamProcessor::ManagerProc::GetActiveWorkflowCount() const
{
    return m_uiActiveWorkflowCount;
}
//---------------------------------------------------------------------------
const NiUInt32 NiStreamProcessor::ManagerProc::GetActiveWorkflowQueueCount() 
    const
{
    NiUInt32 uiActiveCount = 0;
    for (NiInt32 iPriority = IMMEDIATE; iPriority >= 0; iPriority--)
    {
        uiActiveCount += m_akWorkflowQueues[iPriority].GetSize();
    }

    return uiActiveCount;
}
//---------------------------------------------------------------------------
#endif

class NiFloodgateEventQueue : public NiTObjectQueue<NiSPWorkflowPtr>
{
public:
    NiFloodgateEventQueue(unsigned int uiInitialSize) :
        NiTObjectQueue<NiSPWorkflowPtr>(uiInitialSize)
    {
    }
};

//---------------------------------------------------------------------------
NiUInt32 NiStreamProcessor::GetWorkerThreadCount()
{
    if (!m_pkThreadPool)
        return 0;

    return m_pkThreadPool->GetWorkerThreadCount();
}
//---------------------------------------------------------------------------
bool NiStreamProcessor::InitializeInternal()
{
    const NiInitOptions* pkInitOptions = NiStaticDataManager::GetInitOptions();
    unsigned int uiTaskThreads = 0;

    if (!((NiInitOptions*)pkInitOptions)->GetParallelExecution())
    {
        SetParallelExecution(false);
    }
    else
    {
        // Determine the number of worker threads to create.
        uiTaskThreads = ms_pfnDefineWorkerThreadCountFunc();

        // If no worker threads are to be used then operate with a serial
        // execution model.
        if (uiTaskThreads == 0)
            SetParallelExecution(false);
    }

    // Initialize the entries even if we're not running in parallel. 
    // Otherwise, we may run into infinite loops on shutdown when we try to
    // pop the stack and encounter memory fill values.
    InitializeSListHead_GB(&m_kFreeFinalizationEntries);
    InitializeSListHead_GB(&m_kFinalizationStack);

    if (!GetParallelExecution())
        return false;

    m_pkThreadPool = NiNew NiSPThreadPool(uiTaskThreads);
    NIASSERT(m_pkThreadPool);
    m_pkThreadPool->Initialize();

    // Create the event queue from main thread(s) to manager thread
    m_pkEventQueue = NiNew NiFloodgateEventQueue(100);

    // Allocate entries for the finalization stack. It's set to be 1/16 of
    // the max queue size although it grows dynamically as needed.
    NiUInt32 uiNumEntries = NiMax(m_uiMaxQueueSize / 16, 1);
    for (NiUInt32 ui = 0; ui < uiNumEntries; ++ui)
    {
        FinalizationEntry* pkEntry = NiNew FinalizationEntry();
        InterlockedPushEntrySList_GB(&m_kFreeFinalizationEntries,
            &pkEntry->m_kListEntry);
    }

    return true;
}
//---------------------------------------------------------------------------
NiUInt32 NiStreamProcessor::DefineWorkerThreadCountDefaultFunc()
{
    const NiSystemDesc& kSystemDesc = NiSystemDesc::GetSystemDesc();
    NIASSERT(kSystemDesc.GetPlatformID() == NiSystemDesc::NI_WIN32);

    // If we only have a single physical core, return 0 as we want to run
    // in serial mode.
    NiUInt32 uiPhysicalCores = kSystemDesc.GetPhysicalCoreCount();
    if (uiPhysicalCores == 1)
        return 0;

    NiUInt32 uiLogicalProc = kSystemDesc.GetLogicalProcessorCount();

    // Assume that additional logical processors can be useful as half a 
    // physical core. Our worker thread count will be one less than the 
    // number of physical cores plus half the number of additionall logical
    // cores.

    return ((uiPhysicalCores - 1) + ((uiLogicalProc - uiPhysicalCores) / 2));
}
//---------------------------------------------------------------------------
void NiStreamProcessor::AssignDispatcherThreadAffinityDefaultFunc(
    NiProcessorAffinity& kProcessorAffinity)
{
    kProcessorAffinity.SetIdealProcessor(
        NiProcessorAffinity::PROCESSOR_0);
    kProcessorAffinity.SetAffinityMask(
        (unsigned int)NiProcessorAffinity::PROCESSOR_0);
}
//---------------------------------------------------------------------------
bool NiStreamProcessor::ShutdownInternal()
{
    NiDelete m_pkEventQueue;
    m_pkEventQueue = NULL;

    if (m_pkThreadPool)
        m_pkThreadPool->Shutdown();

    NiDelete m_pkThreadPool;
    m_pkThreadPool = NULL;

    // Delete all the entries from the finalization stack.
    FinalizationEntry* pkEntry =
        reinterpret_cast<FinalizationEntry*>(
            InterlockedPopEntrySList_GB(&m_kFreeFinalizationEntries));
    while (pkEntry)
    {
        NiDelete pkEntry;
        pkEntry = reinterpret_cast<FinalizationEntry*>(
            InterlockedPopEntrySList_GB(&m_kFreeFinalizationEntries));
    }

    // Verify that the finalization queue is empty.
    pkEntry = reinterpret_cast<FinalizationEntry*>(
        InterlockedPopEntrySList_GB(&m_kFinalizationStack));
    NIASSERT(pkEntry == NULL);

    return true;
}
//---------------------------------------------------------------------------
void NiStreamProcessor::Receive()
{
    // Noop for win32
}
//---------------------------------------------------------------------------
NiStreamProcessor::Status NiStreamProcessor::StartInternal()
{
    return NiStreamProcessor::RUNNING;
}
//---------------------------------------------------------------------------
NiStreamProcessor::Status NiStreamProcessor::StopInternal()
{
    return NiStreamProcessor::STOPPED;
}
//---------------------------------------------------------------------------
bool NiStreamProcessor::SetWorkerThreadCount(NiUInt32 uiWorkerThreadCount)
{
    NiUInt32 uiCurrentWorkerThreadCount = GetWorkerThreadCount();

    if (uiCurrentWorkerThreadCount == uiWorkerThreadCount)
        return true;

    // Check for a change in execution model.
    if (uiCurrentWorkerThreadCount == 0)
    {
        // Switching from serial execution to parallel execution

        // Initialize the entries lists.
        InitializeSListHead_GB(&m_kFreeFinalizationEntries);
        InitializeSListHead_GB(&m_kFinalizationStack);

        NIASSERT(m_pkThreadPool == NULL);
        m_pkThreadPool = NiNew NiSPThreadPool(uiWorkerThreadCount);
        NIASSERT(m_pkThreadPool);
        m_pkThreadPool->Initialize();

        // Create the event queue from main thread(s) to manager thread
        NIASSERT(m_pkEventQueue == NULL);
        m_pkEventQueue = NiNew NiFloodgateEventQueue(100);

        // Allocate entries for the finalization stack. It's set to be 1/16 of
        // the max queue size although it grows dynamically as needed.
        NiUInt32 uiNumEntries = NiMax(m_uiMaxQueueSize / 16, 1);
        for (NiUInt32 ui = 0; ui < uiNumEntries; ++ui)
        {
            FinalizationEntry* pkEntry = NiNew FinalizationEntry();
            InterlockedPushEntrySList_GB(&m_kFreeFinalizationEntries,
                &pkEntry->m_kListEntry);
        }

        m_eStatus = NiStreamProcessor::RUNNING;

        m_pkSubmitThreadProc = NiNew NiStreamProcessor::ManagerProc(this);
        NIASSERT(m_pkSubmitThreadProc);

        m_pkSubmitThread = NiThread::Create(m_pkSubmitThreadProc);
        m_pkSubmitThread->SetPriority(ms_eThreadPriority);

        NiProcessorAffinity kAffinity;
        ms_pfnAssignDispatcherThreadAffinityFunc(kAffinity);
        m_pkSubmitThread->SetThreadAffinity(kAffinity);

        NIASSERT(m_pkSubmitThread);
        m_pkSubmitThread->SetName("Floodgate: Submit Thread");
        m_pkSubmitThread->Resume();

        m_eStatus = StartInternal();

        m_kManagerLock.Lock();
        m_bParallelExecution = true;
        m_kManagerLock.Unlock();
        return true;
    }
    else if (uiCurrentWorkerThreadCount != 0 && uiWorkerThreadCount == 0)
    {
        // Switching form parallel execution to serial execution.

        // Go ahead and switch the execution model so no incoming task
        // will be processed in parallel.
        m_kManagerLock.Lock();
        m_bParallelExecution = false;
        m_kManagerLock.Unlock();

        // Need to wait for all current workflows to complete.
        NIASSERT(m_pkSubmitThreadProc);

        bool bDone = false;
        while (!bDone)
        {
            m_kManagerLock.Lock();

            if (m_pkSubmitThreadProc->GetActiveWorkflowCount() == 0 &&
                m_pkSubmitThreadProc->GetActiveWorkflowQueueCount() == 0 &&
                m_kSubmitSemaphore.GetCount() == 0)
            {
                bDone = true;
            }
            m_kManagerLock.Unlock();

            NiYieldThread();
        }

        m_pkThreadPool->CompleteActiveWork();
        Shutdown();

        // Flush out any un-needed signals they may have come about while
        // shutting down the ManagerProc.
        while (m_kSubmitSemaphore.GetCount() > 0)
            m_kSubmitSemaphore.Wait();

        return true;
    }
    else
    {
        // Continuing to use the parallel execution model. We're simply just
        // changing the number of available worker threads.
        m_pkThreadPool->SetWorkerThreadCount(uiWorkerThreadCount);
    }

    return false;
}
//---------------------------------------------------------------------------
bool NiStreamProcessor::SetParallelExecution(bool bParallelExecution)
{
    if (bParallelExecution == m_bParallelExecution)
        return true;

    if (m_eStatus == NiStreamProcessor::RUNNING)
    {
        if (!bParallelExecution)
            return SetWorkerThreadCount(0);

        return false;
    }
    else if (bParallelExecution)
    {
        NiUInt32 uiTaskThreads = ms_pfnDefineWorkerThreadCountFunc();
        return SetWorkerThreadCount(uiTaskThreads);
    }
    else
    {
        m_kManagerLock.Lock();
        m_bParallelExecution = bParallelExecution;
        m_kManagerLock.Unlock();
    }

    return true;
}
//---------------------------------------------------------------------------
bool NiStreamProcessor::ExecuteOneWorkflow(NiSPWorkflowPtr spWorkflow)
{
    if (spWorkflow->GetStatus() == NiSPWorkflow::PENDING)
    {
        // Attempt to prepare the task
        if (spWorkflow->Prepare())
        {
            // Determine dependencies and 
            // execution order of contained tasks
            m_kAnalyzer.Analyze(spWorkflow);

            // Set workflow id on manager hash table
            NIASSERT(spWorkflow->GetId() != 0)

                // Update status and execute
                spWorkflow->SetStatus(NiSPWorkflow::RUNNING);

            spWorkflow->Execute();

            // If the workflow is still running, add it to the back of 
            // the queue for polling later.
            if (spWorkflow->GetStatus() == NiSPWorkflow::RUNNING)
            {
                return false;
            }

            return true;
        }
    }
    else if (spWorkflow->GetStatus() == NiSPWorkflow::RUNNING)
    {
        spWorkflow->Execute();

        // If the workflow is still running, add it to the back of 
        // the queue for polling later.
        if (spWorkflow->GetStatus() == NiSPWorkflow::RUNNING)
        {
            return false;
        }

        return true;
    }

    // This assert will get hit if spWorkflow->Prepare returned false.
    NIASSERT(spWorkflow->GetStatus() == NiSPWorkflow::COMPLETED);

    // Must be done, so we don't need to add it back to the queue.
    return true;
}
//---------------------------------------------------------------------------
bool NiStreamProcessor::Submit(NiSPWorkflow* pkWorkflow, Priority ePriority)
{
    if (pkWorkflow)
    {
        pkWorkflow->SetPriority(ePriority);
        pkWorkflow->SetStatus(NiSPWorkflow::PENDING);

        m_kManagerLock.Lock();
        if (m_bParallelExecution)
        {
            m_pkEventQueue->Add(pkWorkflow);
            m_kSubmitSemaphore.Signal();
        }
        else
        {
            ExecuteOneWorkflowSerial(pkWorkflow);
        }
        m_kManagerLock.Unlock();

    }

    return true;
}
//---------------------------------------------------------------------------
void NiStreamProcessor::SubmitForFinalization(NiSPWorkflow* pkWorkflow)
{
    // Get an entry from the pool of free entries.
    FinalizationEntry* pkEntry = reinterpret_cast<FinalizationEntry*>(
        InterlockedPopEntrySList_GB(&m_kFreeFinalizationEntries));

    // Allocate a new entry if NULL.
    if (!pkEntry)
    {
        pkEntry = NiNew FinalizationEntry();
    }

    // Initialize the workflow member.
    pkEntry->m_pkWorkflow = pkWorkflow;

    // Push onto the finalization queue. We don't use release semantics
    // because the semaphore will insert a memory barrier.
    InterlockedPushEntrySList_GB(&m_kFinalizationStack,
        &pkEntry->m_kListEntry);

    // Signal the submit semaphore to wake the dispatch thread.
    m_kSubmitSemaphore.Signal();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// ManagerProc functions.
//---------------------------------------------------------------------------
NiUInt32 NiStreamProcessor::ManagerProc::ThreadProcedure(NiThread* pkThread)
{
    m_pkThread = pkThread;

    // Loop on the manager until an external source sets it to stop.
    while (m_pkManager->IsRunning())
    {
        // Block on the update semaphore until at least one workflow is added.
        // We'll immediately resignal so that the semaphore indicates the true
        // number of workflows.
        m_pkManager->m_kSubmitSemaphore.Wait();
        m_pkManager->m_kSubmitSemaphore.Signal();

        // Finalize anything in the finalization stack.
        FinalizationEntry* pkEntry =
            reinterpret_cast<FinalizationEntry*>(
                InterlockedPopEntrySList_GB(
                    &m_pkManager->m_kFinalizationStack));
        while (pkEntry)
        {
            pkEntry->m_pkWorkflow->ExecutionComplete();
            pkEntry->m_pkWorkflow->SetStatus(NiSPWorkflow::COMPLETED);
            pkEntry->m_pkWorkflow = NULL;
            InterlockedPushEntrySList_GB(
                &m_pkManager->m_kFreeFinalizationEntries,
                &pkEntry->m_kListEntry);

            // N.B. There is a chance that we could block on this semaphore
            // here if the thread pool had pushed and entry for finalization
            // but not signaled the semaphore. In that case, we would sleep
            // and be immediately reawakened when that thread hits the signal
            // in SubmitForFinalization.
            m_pkManager->m_kSubmitSemaphore.Wait();

            pkEntry = reinterpret_cast<FinalizationEntry*>(
                InterlockedPopEntrySList_GB(
                    &m_pkManager->m_kFinalizationStack));
        }

        if (!m_pkManager->m_pkEventQueue->IsEmpty())
        {
            // Move things from the external queue to the internal list of 
            // priority queues as quickly as possible, as we're in a critical 
            // section here. Up to 8 workflows are moved at once. This helps
            // to reduce the number of full loops required to add in all
            // submitted tasks to the priority queues.
            NiSPWorkflowPtr spWorkflows[8];
            NiUInt32 uiIdx = 0;

            m_pkManager->m_kManagerLock.Lock();

            // Note this adds the works flows to the array in the reverse
            // order they were submitted.
            while (uiIdx < 8 && !m_pkManager->m_pkEventQueue->IsEmpty())
                spWorkflows[uiIdx++] = m_pkManager->m_pkEventQueue->Remove();

            m_pkManager->m_kManagerLock.Unlock();


            // Prepare Workflows
            for (NiUInt32 ui = 0; ui < uiIdx; ui++)
            {
                // Access workflows in the array in the reverse order. This
                // ensures they are added to the priority queues in the same
                // order they were submitted.
                NiSPWorkflowPtr spWorkflow = spWorkflows[uiIdx - ui - 1];

                if (spWorkflow)
                {
                    // Get queue and check for available space
                    Priority ePriority = (Priority)spWorkflow->GetPriority();
                    NiSPWorkflowQueue& kQueue = m_akWorkflowQueues[ePriority];
                    if (kQueue.GetSize() >= m_pkManager->m_uiMaxQueueSize)
                    {
                        NILOG("NiStreamProcessor::ManagerProc::ThreadProcedure"
                            "- Too many outstanding workflows\n");
                    }

                    // Update the status of the workflow to pending
                    spWorkflow->Reset();
                    spWorkflow->SetStatus(NiSPWorkflow::PENDING);

                    // Add workflow to the proper queue
                    kQueue.Add(spWorkflow);
                    m_uiActiveWorkflowCount++;
                }
            }
        }

        // Iterate over all queues in order of decreasing priority
        for (NiInt32 iPriority = IMMEDIATE; iPriority >= 0; --iPriority)
        {
            Priority ePriority = static_cast<Priority>(iPriority);

            NiUInt32 uiSize = m_akWorkflowQueues[ePriority].GetSize();

            // Execute all workflows at this priority level
            for (NiUInt32 ui = 0; ui < uiSize; ui++)
            {
                NiSPWorkflowPtr spWorkflow =
                    m_akWorkflowQueues[ePriority].Head();

                // Push the entire workflow to the thread pool. The thread 
                // pool will split the the workflow's joblists between the
                // worker threads. The thread pool is responsible for ensuring
                // that a workflow's joblists execution respects their
                // dependencies i.e. a workflow's stages will be executed in 
                // monotonically increased order.
                m_pkManager->ExecuteOneWorkflow(spWorkflow);
                m_akWorkflowQueues[ePriority].Remove();

                NIASSERT(m_uiActiveWorkflowCount);
                m_uiActiveWorkflowCount--;

                // We've removed a workflow from the queue. We need to 
                // wait on the semaphore again to make sure we have
                // a task in the queues and keep the semaphore accurate.
                m_pkManager->m_kSubmitSemaphore.Wait();
            }
        }
    }

    // Finalize anything in the finalization stack.
    FinalizationEntry* pkEntry =
        reinterpret_cast<FinalizationEntry*>(
            InterlockedPopEntrySList_GB(
                &m_pkManager->m_kFinalizationStack));
    while (pkEntry)
    {
        pkEntry->m_pkWorkflow->ExecutionComplete();
        pkEntry->m_pkWorkflow->SetStatus(NiSPWorkflow::COMPLETED);
        pkEntry->m_pkWorkflow = NULL;
        InterlockedPushEntrySList_GB(&m_pkManager->m_kFreeFinalizationEntries,
            &pkEntry->m_kListEntry);
        pkEntry = reinterpret_cast<FinalizationEntry*>(
            InterlockedPopEntrySList_GB(
                &m_pkManager->m_kFinalizationStack));
    }


    // Abort the remaining elements
    for (NiInt32 iPriority = IMMEDIATE; iPriority >= 0; iPriority--)
    {
        NiSPWorkflowPtr spWorkflow = m_akWorkflowQueues[iPriority].Remove();
        while (spWorkflow)
        {
            spWorkflow->SetStatus(NiSPWorkflow::ABORTED);
            spWorkflow->Clear();
            spWorkflow = m_akWorkflowQueues[iPriority].Remove();
        }
    }

    // Stop signal has been received. We'll return an exit code of 0.
    return 0;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Lockless stack functions.
//---------------------------------------------------------------------------
void InitializeSListHead_GB(PSLIST_HEADER_GB pkListHead)
{
    NIASSERT(pkListHead != NULL);
    pkListHead->Next = NULL;
}
//---------------------------------------------------------------------------
PSLIST_ENTRY_GB InterlockedPopEntrySList_GB(
    PSLIST_HEADER_GB pkListHead)
{
    NIASSERT(pkListHead);

    bool bDone = false;
    PSLIST_ENTRY_GB pkCurrentHead = NULL;
    while (!bDone)
    {
        pkCurrentHead = pkListHead->Next;
        if (pkCurrentHead)
        {
            // pvTestedHead will contain the contents of pkListHead->Next
            // before the compare/exchange takes place
            void* pvTestedHead = InterlockedCompareExchangePointerAcquire(
                (void**)&(pkListHead->Next),
                pkCurrentHead->Next,
                pkCurrentHead);

            // Repeating the test that InterlockedCompareExchangePointer does
            // will indicate whether or not the function succeeded.
            if (pvTestedHead == pkCurrentHead)
            {
                bDone = true;
                pkCurrentHead->Next = NULL;
            }
        }
        else
        {
            // Nothing to pop, and still atomic
            bDone = true;
        }
    }
    return pkCurrentHead;
}
//---------------------------------------------------------------------------
PSLIST_ENTRY_GB InterlockedPushEntrySList_GB(
    PSLIST_HEADER_GB pkListHead,
    PSLIST_ENTRY_GB pkListEntry)
{
    NIASSERT(pkListHead);
    NIASSERT(pkListEntry);

    PSLIST_ENTRY_GB pkInitialHead = NULL;
    bool bDone = false;
    while (!bDone)
    {
        pkInitialHead = pkListHead->Next;
        pkListEntry->Next = pkInitialHead;
        // pvTestedHead will contain the contents of pkListHead->Next
        // before the compare/exchange takes place
        void* pvTestedHead = InterlockedCompareExchangePointerRelease(
            (void**)&(pkListHead->Next),
            pkListEntry,
            pkInitialHead);
        // Repeating the test that InterlockedCompareExchangePointer does
        // will indicate whether or not the function succeeded.
        bDone = (pvTestedHead == pkInitialHead);
    }

    return pkInitialHead;
}
//---------------------------------------------------------------------------
