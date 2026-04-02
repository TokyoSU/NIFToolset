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
#include <utility>
#include "NiTerrainSector.h"
#include "NiTerrain.h"
#include "NiTerrainStreamingManager.h"
#include "NiTerrainStreamingTask.h"

//--------------------------------------------------------------------------------------------------
NiTerrainStreamingManager::NiTerrainStreamingManager():
    m_kAllThreadedTasksCompleted(m_kMutex, true)
{
    // Take this thread to be the update/renderer thread - record it's ID
    m_uiUpdateThreadID = efd::GetCurrentThreadId();

    // Initialize the task processor:
    m_kTaskProcessor.SetAquireTaskQueueCallback(this, &Callback_AquireTaskQueue);
    m_kTaskProcessor.SetNumWorkers(8);

    // Initialize the task queues:
    m_kTaskQueues[TaskQueueType::PRELOADING].SetTaskProcessingCallback(
        &Callback_ProcessPreloadTask, this);
    m_kTaskQueues[TaskQueueType::LOADING].SetTaskProcessingCallback(
        &Callback_ProcessLoadingTask, this);
    m_kTaskQueues[TaskQueueType::BUILDING_HEIGHTS].SetTaskProcessingCallback(
        &Callback_ProcessBuildingHeightsTask, this);
    m_kTaskQueues[TaskQueueType::BUILDING_LIGHTING].SetTaskProcessingCallback(
        &Callback_ProcessBuildingLightingTask, this);
    m_kTaskQueues[TaskQueueType::BUILDING_MAPS].SetTaskProcessingCallback(
        &Callback_ProcessBuildingMapsTask, this);
    m_kTaskQueues[TaskQueueType::POPULATING].SetTaskProcessingCallback(
        &Callback_ProcessPopulatingTask, this);
    m_kTaskQueues[TaskQueueType::UNLOADING].SetTaskProcessingCallback(
        &Callback_ProcessUnloadingTask, this);
    m_kTaskQueues[TaskQueueType::REMOVAL].SetTaskProcessingCallback(
        &Callback_ProcessRemovalTask, this);
    m_kTaskQueues[TaskQueueType::LOCK].SetTaskProcessingCallback(
        &Callback_ProcessLockTask, this);
    m_kTaskQueues[TaskQueueType::UNLOCK].SetTaskProcessingCallback(
        &Callback_ProcessUnlockTask, this);
}

//--------------------------------------------------------------------------------------------------
NiTerrainStreamingManager::~NiTerrainStreamingManager()
{
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingManager::RequestRebuildSector( NiTerrainSector* pkSector, 
    NiRect<NiInt32> kChangedRegion)
{
    EE_ASSERT(pkSector);

    NiTerrainStreamingTask* pkNewTask = EE_NEW NiTerrainStreamingTask();
    pkNewTask->SetSector(pkSector);
    pkNewTask->SetBuildRegion(kChangedRegion);

    // Submit the task for processing
    return StartTask(pkNewTask, true);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingManager::RequestPageSector(NiTerrainSector* pkSector, NiInt32 iTargetLOD,
    bool bWaitForCompletion)
{
    EE_ASSERT(pkSector);

    NiTerrainStreamingTask* pkNewTask = EE_NEW NiTerrainStreamingTask();
    pkNewTask->SetSector(pkSector);
    pkNewTask->SetTargetLOD(iTargetLOD);
    
    // Submit the task for processing
    return StartTask(pkNewTask, bWaitForCompletion);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingManager::RequestCreateBlankSector(NiTerrainSector* pkSector, 
    bool bWaitForCompletion)
{
    EE_ASSERT(pkSector);

    NiTerrainStreamingTask* pkNewTask = EE_NEW NiTerrainStreamingTask();

    // When creating a blank terrain, always create to maximum detail level
    pkNewTask->SetSector(pkSector);
    pkNewTask->SetTargetLOD(pkSector->GetTerrain()->GetNumLOD());
    pkNewTask->SetCreateBlankHeightData(true);

    // Submit the task for processing
    return StartTask(pkNewTask, bWaitForCompletion);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingManager::StartTask(NiTerrainStreamingTask* pkTask, bool bWaitForCompletion)
{
    EE_ASSERT(m_uiUpdateThreadID == efd::GetCurrentThreadId());
    EE_ASSERT(pkTask);
    m_kMutex.Lock();

    // Begin using a smart pointer 
    // (forces the life of the task until after waiting has completed)
    NiTerrainStreamingTaskPtr spTask = pkTask;

    // Figure out if this task has already been submitted?
    NiTerrainStreamingTask* pkCurrentTask = NULL;
    if (m_kTaskedSectors.find(pkTask->GetSector(), pkCurrentTask))
    {
        EE_ASSERT(pkCurrentTask != spTask);
        if (!CancelTask(pkCurrentTask))
        {
            m_kMutex.Unlock();
            spTask->Cancel();
            spTask->Close();
            return false;
        }
    }

    NiUInt32 uiNumWorkers = m_kTaskProcessor.GetNumWorkers();
    spTask->SetMaxWorkUnits(uiNumWorkers > 0 ? uiNumWorkers : 1);

    // Analyze the task, see what needs to be loaded
    spTask->Initialize();

    // Assign this task as the next task to be executed for this sector
    m_kTaskedSectors.insert(std::make_pair(spTask->GetSector(), spTask));

    m_kMutex.Unlock();

    // Is it a loading, or unloading task?
    if (spTask->GetTaskType() == NiTerrainStreamingTask::TaskType::UNLOADING)
    {
        SubmitTask(spTask, NiTerrainStreamingTask::Status::LOCKING);
    }
    else
    {
        SubmitTask(spTask, NiTerrainStreamingTask::Status::PRELOADING);
    }

    // Notify listener
    NotifyStartTask(pkTask);

    if (bWaitForCompletion)
    {
        WaitForTaskToComplete(spTask);
    }
    
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingManager::SubmitTask(NiTerrainStreamingTask* pkTask,
    NiTerrainStreamingTask::Status::VALUE eStatus)
{
    EE_ASSERT(pkTask);

    // Has this task completed?
    if (pkTask->GetComplete())
    {
        CompleteTask(pkTask);
        return true;
    }
    
    // What stage is it up to:
    pkTask->SetStatus(eStatus);
    // Notify listener
    NotifyTaskStatus(pkTask);
    
    m_kMutex.Lock();
    NiTerrainTaskQueue* pkQueue = NULL;
    switch (eStatus)
    {
        case NiTerrainStreamingTask::Status::PRELOADING:
            pkQueue = &m_kTaskQueues[TaskQueueType::PRELOADING];
            break;
        case NiTerrainStreamingTask::Status::LOADING:
            pkQueue = &m_kTaskQueues[TaskQueueType::LOADING];
            break;
        case NiTerrainStreamingTask::Status::BUILDING_HEIGHTS:
            pkQueue = &m_kTaskQueues[TaskQueueType::BUILDING_HEIGHTS];
            break;
        case NiTerrainStreamingTask::Status::BUILDING_LIGHTING:
            pkQueue = &m_kTaskQueues[TaskQueueType::BUILDING_LIGHTING];
            break;
        case NiTerrainStreamingTask::Status::BUILDING_MAPS:
            pkQueue = &m_kTaskQueues[TaskQueueType::BUILDING_MAPS];
            break;
        case NiTerrainStreamingTask::Status::POPULATING:
            pkQueue = &m_kTaskQueues[TaskQueueType::POPULATING];
            break;
        case NiTerrainStreamingTask::Status::UNLOADING:
            pkQueue = &m_kTaskQueues[TaskQueueType::UNLOADING];
            break;
        case NiTerrainStreamingTask::Status::REMOVING:
            pkQueue = &m_kTaskQueues[TaskQueueType::REMOVAL];
            break;
        case NiTerrainStreamingTask::Status::LOCKING:
            pkQueue = &m_kTaskQueues[TaskQueueType::LOCK];
            break;
        case NiTerrainStreamingTask::Status::UNLOCKING:
            pkQueue = &m_kTaskQueues[TaskQueueType::UNLOCK];
            break;
        default:
            EE_FAIL("Invalid task submission state");
    }

    // Not all tasks are complete, so we need to notify the condition variable. 
    m_kAllThreadedTasksCompleted.SetValue(m_kTaskedSectors.size() == 0);

    pkQueue->SubmitTask(pkTask);
    m_kMutex.Unlock();

    // Notify threads that a new task is available
    bool bUpdateThreadTask = pkTask->GetStatus() == NiTerrainStreamingTask::Status::REMOVING ||
        pkTask->GetStatus() == NiTerrainStreamingTask::Status::LOCKING ||
        pkTask->GetStatus() == NiTerrainStreamingTask::Status::UNLOCKING;
    if (!bUpdateThreadTask)
    {
        // Notify once for each work unit added
        NiUInt32 uiNumWorkUnits = pkTask->GetCurrentTotalWorkUnits();
        for (NiUInt32 ui = 0; ui < uiNumWorkUnits; ++ui)
            m_kTaskProcessor.NotifyTaskAdded();
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::CompleteTask(NiTerrainStreamingTask* pkTask)
{
    m_kMutex.Lock();

    // Remove the entry for this sector from the map
    if (pkTask->GetStatus() != NiTerrainStreamingTask::Status::CANCELLED)
    {
        NiTerrainStreamingTask* pkCurrentTask = NULL;
        if (m_kTaskedSectors.find(pkTask->GetSector(), pkCurrentTask))
        {
            EE_ASSERT(pkCurrentTask == pkTask);
            m_kTaskedSectors.erase(pkTask->GetSector());
        }
    }

    // Detect if we have completed all the tasks
    m_kAllThreadedTasksCompleted.SetValue(m_kTaskedSectors.size() == 0);

    // Notify listener
    NotifyFinishTask(pkTask);

    // Signal any threads waiting on this task for completion and remove any waiting tasks
    pkTask->Close();

    m_kMutex.Unlock();
}

//--------------------------------------------------------------------------------------------------
NiTerrainTaskQueue* NiTerrainStreamingManager::AquireTaskQueue()
{
    NiTerrainTaskQueue* pkQueue = NULL;
    
    // Select which queue to execute
    m_kMutex.Lock();

    // Always choose a preload task, if able
    // Only one thread is ever allowed to access the disk
    if (m_kTaskQueues[TaskQueueType::PRELOADING].GetActiveWorkUnitCount() == 0 && 
        m_kTaskQueues[TaskQueueType::PRELOADING].ReserveWorkUnit())
    {
        pkQueue = &m_kTaskQueues[TaskQueueType::PRELOADING];
    }
    else
    {

        // Find the queue that has the most tasks submitted
        NiInt32 iBestQueue = TaskQueueType::NUM_TASK_QUEUES;
        for (NiInt32 iQueue = TaskQueueType::NUM_TASK_QUEUES - 1; 
            iQueue >= 0; 
            --iQueue)
        {
            // Don't distribute removal tasks to threads
            if (iQueue == TaskQueueType::PRELOADING ||
                iQueue == TaskQueueType::REMOVAL ||
                iQueue == TaskQueueType::LOCK ||
                iQueue == TaskQueueType::UNLOCK)
                continue;

            NiUInt32 uiNumTasks = m_kTaskQueues[iQueue].GetNumUnreservedWorkUnits();
            if (uiNumTasks)
            {
                iBestQueue = iQueue;
                break;
            }
        }

        // Attempt to reserve a task on the largest task list
        if (iBestQueue != TaskQueueType::NUM_TASK_QUEUES &&
            m_kTaskQueues[iBestQueue].ReserveWorkUnit())
        {
            pkQueue = &m_kTaskQueues[iBestQueue];
        }
    }

    m_kMutex.Unlock();
    return pkQueue;
}

//--------------------------------------------------------------------------------------------------
NiTerrainTaskQueue* NiTerrainStreamingManager::Callback_AquireTaskQueue(void* pvArg)
{
    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);
    return pkStreamingManager->AquireTaskQueue();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Callback_ProcessPreloadTask(void* pvArg, 
    NiTerrainStreamingTask* pkTask, NiUInt32 uiWorkUnit)
{
    NI_UNUSED_ARG(uiWorkUnit);

    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);

    // Preload data from file:
    pkTask->StartWorkUnit();
    pkTask->Preload();

    // Submit it for building
    if (pkTask->CompleteWorkUnit())
        pkStreamingManager->SubmitTask(pkTask, NiTerrainStreamingTask::Status::LOADING);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Callback_ProcessLoadingTask(void* pvArg, 
    NiTerrainStreamingTask* pkTask, NiUInt32 uiWorkUnit)
{
    NI_UNUSED_ARG(uiWorkUnit);

    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);
    
    // Process data loaded from file into useful data (decompression etc)
    pkTask->StartWorkUnit();
    pkTask->LoadBuffers();

    // Submit it for building
    if (pkTask->CompleteWorkUnit())
        pkStreamingManager->SubmitTask(pkTask, NiTerrainStreamingTask::Status::BUILDING_HEIGHTS);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Callback_ProcessBuildingHeightsTask(void* pvArg, 
    NiTerrainStreamingTask* pkTask, NiUInt32 uiWorkUnit)
{
    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);
    
    // Build the buffers
    pkTask->StartWorkUnit();
    pkTask->BuildHeights(uiWorkUnit);

    // Submit it for population
    if (pkTask->CompleteWorkUnit())
        pkStreamingManager->SubmitTask(pkTask, NiTerrainStreamingTask::Status::BUILDING_LIGHTING);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Callback_ProcessBuildingLightingTask(void* pvArg, 
    NiTerrainStreamingTask* pkTask, NiUInt32 uiWorkUnit)
{
    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);

    // Build the buffers
    pkTask->StartWorkUnit();
    pkTask->BuildLighting(uiWorkUnit);

    // Submit it for population
    if (pkTask->CompleteWorkUnit())
        pkStreamingManager->SubmitTask(pkTask, NiTerrainStreamingTask::Status::BUILDING_MAPS);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Callback_ProcessBuildingMapsTask(void* pvArg, 
    NiTerrainStreamingTask* pkTask, NiUInt32 uiWorkUnit)
{
    NI_UNUSED_ARG(uiWorkUnit);

    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);

    // Build the buffers
    pkTask->StartWorkUnit();
    pkTask->BuildLowDetailNormalMap();

    // Submit it for population
    if (pkTask->CompleteWorkUnit())
        pkStreamingManager->SubmitTask(pkTask, NiTerrainStreamingTask::Status::LOCKING);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Callback_ProcessPopulatingTask(void* pvArg, 
    NiTerrainStreamingTask* pkTask, NiUInt32 uiWorkUnit)
{
    NI_UNUSED_ARG(uiWorkUnit);

    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);

    // Populate the cells
    pkTask->StartWorkUnit();
    pkTask->PopulateStreams();

    // Submit it for population
    if (pkTask->CompleteWorkUnit())
        pkStreamingManager->SubmitTask(pkTask, NiTerrainStreamingTask::Status::UNLOCKING);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Callback_ProcessUnloadingTask(void* pvArg, 
    NiTerrainStreamingTask* pkTask, NiUInt32 uiWorkUnit)
{
    NI_UNUSED_ARG(uiWorkUnit);

    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);

    // Unload the streams
    pkTask->StartWorkUnit();
    pkTask->UnloadStreams();

    // The task may require removal by the terrain thread
    if (pkTask->CompleteWorkUnit())
        pkStreamingManager->SubmitTask(pkTask, NiTerrainStreamingTask::Status::UNLOCKING);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Callback_ProcessRemovalTask(void* pvArg, 
    NiTerrainStreamingTask* pkTask, NiUInt32 uiWorkUnit)
{
    NI_UNUSED_ARG(uiWorkUnit);

    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);

    // Remove the sector from terrain
    pkTask->StartWorkUnit();
    pkTask->RemoveFromTerrain();

    // Submit it for completion
    EE_ASSERT (pkTask->GetComplete());
    if (pkTask->CompleteWorkUnit())
        pkStreamingManager->SubmitTask(pkTask, pkTask->GetStatus());
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Callback_ProcessLockTask(void* pvArg, 
    NiTerrainStreamingTask* pkTask, NiUInt32 uiWorkUnit)
{
    NI_UNUSED_ARG(uiWorkUnit);

    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);
    EE_ASSERT(pkStreamingManager->m_uiUpdateThreadID == efd::GetCurrentThreadId());

    // Unload the streams
    pkTask->StartWorkUnit();
    pkTask->AcquireLocks();

    // The task may require removal by the terrain thread
    if (pkTask->CompleteWorkUnit())
    {
        if (pkTask->GetTaskType() == NiTerrainStreamingTask::TaskType::UNLOADING)
        {
            pkStreamingManager->SubmitTask(pkTask, NiTerrainStreamingTask::Status::UNLOADING);
        }
        else
        {
            pkStreamingManager->SubmitTask(pkTask, NiTerrainStreamingTask::Status::POPULATING);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Callback_ProcessUnlockTask(void* pvArg, 
    NiTerrainStreamingTask* pkTask, NiUInt32 uiWorkUnit)
{
    NI_UNUSED_ARG(uiWorkUnit);

    NiTerrainStreamingManager* pkStreamingManager = (NiTerrainStreamingManager*)(pvArg);
    EE_ASSERT(pkStreamingManager->m_uiUpdateThreadID == efd::GetCurrentThreadId());

    // Unload the streams
    pkTask->StartWorkUnit();
    pkTask->ReleaseLocks();

    // Submit it for completion
    if (pkTask->CompleteWorkUnit())
    {
        if (pkTask->GetTaskType() == NiTerrainStreamingTask::TaskType::UNLOADING)
        {
            pkStreamingManager->SubmitTask(pkTask, NiTerrainStreamingTask::Status::REMOVING);
        }
        else 
        {
            EE_ASSERT(pkTask->GetComplete());
            pkStreamingManager->SubmitTask(pkTask, pkTask->GetStatus());
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::PerformRendererThreadTasks(efd::UInt32 uiMaxNumTasksToProcess)
{
    EE_ASSERT(m_uiUpdateThreadID == efd::GetCurrentThreadId());

    // If the number of tasks to process is 0 then execute ALL pending tasks
    efd::UInt32 uiNumTasksLeft = uiMaxNumTasksToProcess;
    if (uiNumTasksLeft == 0)
        uiNumTasksLeft = efd::UInt32(-1);

    // Remove unloaded sectors
    ExecuteQueueTasks(&m_kTaskQueues[TaskQueueType::REMOVAL], uiNumTasksLeft);

    // Perform any unlock tasks that are waiting
    ExecuteQueueTasks(&m_kTaskQueues[TaskQueueType::UNLOCK], uiNumTasksLeft);

    // Perform any lock tasks that are waiting
    ExecuteQueueTasks(&m_kTaskQueues[TaskQueueType::LOCK], uiNumTasksLeft);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::ExecuteQueueTasks(NiTerrainTaskQueue* pkQueue, 
    efd::UInt32& uiMaxTasks)
{
    EE_ASSERT(pkQueue);

    // Execute the set number of tasks
    while (uiMaxTasks && pkQueue->ReserveWorkUnit())
    {
        pkQueue->ProcessWorkUnit();
        uiMaxTasks--;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingManager::CancelTask(NiTerrainStreamingTask* pkTask)
{
    m_kMutex.Lock();

    NiTerrainSector* pkSector = pkTask->GetSector();
    bool bSuccess = pkTask->Cancel();
    if (bSuccess)
    {
        // Remove the entry for this sector from the map
        NiTerrainStreamingTask* pkCurrentTask = NULL;
        if (m_kTaskedSectors.find(pkSector, pkCurrentTask))
        {
            EE_ASSERT(pkCurrentTask == pkTask);
            m_kTaskedSectors.erase(pkSector);
        }
    }

    m_kMutex.Unlock();
    return bSuccess;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::Abort()
{
    EE_ASSERT(m_uiUpdateThreadID == efd::GetCurrentThreadId());

    // First, try to cancel all loading tasks
    m_kMutex.Lock();
    {
        typedef efd::list<NiTerrainSector*> SectorList;
        SectorList kCancelledSectors;

        // Look through the tasked sectors map for any loading tasks that are not underway.
        for (SectorToTaskMap::iterator kIter = m_kTaskedSectors.begin();
            kIter != m_kTaskedSectors.end();
            kIter++)
        {
            NiTerrainStreamingTask* pkTask = (*kIter).second;

            if (pkTask->GetWaiting() == true && 
                pkTask->GetStatus() == NiTerrainStreamingTask::Status::PRELOADING)
            {
                if (pkTask->Cancel())
                {
                    kCancelledSectors.push_back((*kIter).first);
                }
            }
        }

        // Were any sectors cancelled?
        for (SectorList::iterator kIter = kCancelledSectors.begin(); 
            kIter != kCancelledSectors.end(); 
            kIter++)
        {
            m_kTaskedSectors.erase(*kIter);
        }
    }

    // Wait for all the remaining tasks to be completed appropriately
    WaitForAllTasksCompleted();
    m_kMutex.Unlock();

    // Force the task processor to end all the threads
    m_kTaskProcessor.Shutdown();
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::WaitForTaskToComplete(NiTerrainStreamingTask* pkTask)
{
    if (m_uiUpdateThreadID == efd::GetCurrentThreadId())
    {// This thread may need to do streaming work itself, spin-lock until all the work is done.
        while (!pkTask->IsClosed())
        {
            PerformRendererThreadTasks();
            efd::Sleep(10);
        }
    }
    else
    {// This thread does not have any work to do in streaming - let it wait
        pkTask->WaitForClose();
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::WaitForAllTasksCompleted()
{
    if (m_uiUpdateThreadID == efd::GetCurrentThreadId())
    {// This thread may need to do streaming work itself, spin-lock until all the work is done.
        while (!m_kAllThreadedTasksCompleted.GetValue())
        {
            PerformRendererThreadTasks();
            efd::Sleep(10);
        }
    }
    else
    {// This thread does not have any work to do in streaming - let it wait
        m_kMutex.Lock();
        m_kAllThreadedTasksCompleted.WaitEqual(true);
        m_kMutex.Unlock();
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingManager::PollSectorStreamingStatus(NiTerrainSector* pkSector)
{
    bool bStatus = false;
    m_kMutex.Lock();
    
    NiTerrainStreamingTask* pkCurrentTask = NULL;
    if (m_kTaskedSectors.find(pkSector, pkCurrentTask))
    {
        bStatus = pkCurrentTask != NULL;
    }

    m_kMutex.Unlock();
    return bStatus;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainStreamingManager::PollStreamingStatus()
{
    bool bStatus = false;
    m_kMutex.Lock();
    
    bStatus = m_kTaskedSectors.size() != 0;

    m_kMutex.Unlock();
    return bStatus;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::NotifyStartTask(NiTerrainStreamingTask* pkTask)
{
    Listener::TaskStartedEventArgs kEventArgs;
    kEventArgs.m_sectorID = pkTask->GetSectorID();
    kEventArgs.m_taskID = Listener::TaskID(pkTask->GetSector());
    kEventArgs.m_taskType = pkTask->GetTaskType();
    kEventArgs.m_targetLOD = pkTask->GetTargetLOD();

    m_kListener.m_taskStarted.Raise(pkTask->GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::NotifyTaskStatus(NiTerrainStreamingTask* pkTask)
{
    Listener::TaskStatusChangeEventArgs kEventArgs;
    kEventArgs.m_sectorID = pkTask->GetSectorID();
    kEventArgs.m_taskID = Listener::TaskID(pkTask->GetSector());
    kEventArgs.m_taskType = pkTask->GetTaskType();
    kEventArgs.m_status = pkTask->GetStatus();
    kEventArgs.m_targetLOD = pkTask->GetTargetLOD();

    m_kListener.m_taskStatusChanged.Raise(pkTask->GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::NotifyFinishTask(NiTerrainStreamingTask* pkTask)
{
    Listener::TaskFinishedEventArgs kEventArgs;
    kEventArgs.m_sectorID = pkTask->GetSectorID();
    kEventArgs.m_taskID = Listener::TaskID(pkTask->GetSector());
    kEventArgs.m_taskType = pkTask->GetTaskType();
    kEventArgs.m_errorCode = pkTask->GetErrorCode();
    kEventArgs.m_targetLOD = pkTask->GetTargetLOD();

    m_kListener.m_taskFinished.Raise(pkTask->GetTerrain(), kEventArgs);
}

//--------------------------------------------------------------------------------------------------
NiTerrainStreamingManager::Listener& NiTerrainStreamingManager::GetListener()
{
    return m_kListener;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainStreamingManager::SetNumWorkers(efd::UInt32 uiNumWorkers)
{
    m_kTaskProcessor.SetNumWorkers(uiNumWorkers);
}

//--------------------------------------------------------------------------------------------------
NiTerrainStreamingManager::Listener::Listener()
{
    // Do nothing
}

//--------------------------------------------------------------------------------------------------
NiTerrainStreamingManager::Listener::~Listener()
{
    // Do nothing
}

//--------------------------------------------------------------------------------------------------
