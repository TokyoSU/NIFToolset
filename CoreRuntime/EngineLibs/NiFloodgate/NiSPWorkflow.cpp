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

//--------------------------------------------------------------------------------------------------
#include "NiFloodgatePCH.h"
#include "NiSPWorkflow.h"
#include "NiStreamProcessor.h"
#include "NiSPAlgorithms.h"

//--------------------------------------------------------------------------------------------------
NiTObjectPool<NiSPWorkflow>* NiSPWorkflow::ms_pkWorkflowPool = NULL;
efd::CriticalSection NiSPWorkflow::ms_kWorkflowPoolLock;

//--------------------------------------------------------------------------------------------------
NiSPWorkflow::NiSPWorkflow() :
    m_uiId(0),
    m_iCurrentStage(-1),
    m_eStatus(IDLE),
    m_pkCompletionHandler(NULL),
    m_uiRefCount(0),
    m_iPriority(0)
{
}

//--------------------------------------------------------------------------------------------------
NiSPWorkflow::~NiSPWorkflow()
{
    Clear();
}

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::InitializePools(NiUInt32 uiWorkflowPoolSize)
{
    ms_kWorkflowPoolLock.Lock();
    EE_ASSERT(ms_pkWorkflowPool == NULL);
    ms_pkWorkflowPool = NiNew NiTObjectPool<NiSPWorkflow>(uiWorkflowPoolSize);
    ms_kWorkflowPoolLock.Unlock();
}

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::ShutdownPools()
{
    ms_kWorkflowPoolLock.Lock();
    EE_ASSERT(ms_pkWorkflowPool);
    NiDelete ms_pkWorkflowPool;
    ms_pkWorkflowPool = NULL;
    ms_kWorkflowPoolLock.Unlock();
}

//--------------------------------------------------------------------------------------------------
NiSPWorkflowPtr NiSPWorkflow::GetFreeWorkflow()
{
    EE_ASSERT(ms_pkWorkflowPool && "ms_pkWorkflowPool was NULL!");

    ms_kWorkflowPoolLock.Lock();
    NiSPWorkflowPtr spWorkflow = ms_pkWorkflowPool->GetFreeObject();
    ms_kWorkflowPoolLock.Unlock();

    EE_ASSERT(spWorkflow && "spWorkflow was NULL!");
    if (spWorkflow)
    {
        spWorkflow->SetStatus(NiSPWorkflow::IDLE);
    }
    return spWorkflow;
}

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::ReleaseWorkflow()
{
    Clear();
    ms_kWorkflowPoolLock.Lock();
    ms_pkWorkflowPool->ReleaseObject(this);
    ms_kWorkflowPoolLock.Unlock();
}

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::Add(NiSPTask* pkTask)
{
    EE_ASSERT(m_eStatus != RUNNING);
    pkTask->SetWorkflow(this);
    m_kTasks.Add(pkTask);
}

//--------------------------------------------------------------------------------------------------
NiSPTask* NiSPWorkflow::AddNewTask(const NiUInt16 uiNumInputs,
     const NiUInt16 uiNumOutputs, const bool bIsSignalTask)
{
    EE_ASSERT(m_eStatus != RUNNING);

    NiSPTaskPtr spTask = NiSPTask::GetNewTask(uiNumInputs, uiNumOutputs);

    EE_ASSERT(spTask);

    if (bIsSignalTask)
    {
        spTask->SetSignalType(NiSPWorkload::SIGNAL_COMPLETION);
        spTask->SetKernel(NiSPTaskImpl::GetSignalKernel());
        spTask->SetSyncData((NiUInt64)GetAtomicUpdateCompleteAddress());
    }

    NiSPWorkflow::Add(spTask);

    return spTask;
}

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::ReplaceAt(NiUInt32 uiTaskIdx, NiSPTask* pkTask)
{
    EE_ASSERT(m_eStatus != RUNNING);
    m_kTasks.ReplaceAt(uiTaskIdx, pkTask);
}

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::RemoveAt(NiUInt32 uiTaskIdx)
{
    EE_ASSERT(m_eStatus != RUNNING);
    m_kTasks.RemoveAt(uiTaskIdx);
}

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::SortTasksByStage()
{
    EE_ASSERT(m_eStatus != RUNNING);
    NiSPAlgorithms::QuickSortPointers< NiTObjectPtrSet<NiSPTaskPtr>,
        NiSPTaskPtr>(&m_kTasks, 0, m_kTasks.GetSize() - 1);
}

//--------------------------------------------------------------------------------------------------
bool NiSPWorkflow::Prepare()
{
    if (!m_kWorkflowImpl.Prepare())
        return false;

    // Find completion signal task
    NiUInt32 uiTaskCount = GetSize();
    NiSPTask* pkSignalTask = NULL;
    bool bFoundSignalTask = false;
    for (NiUInt32 uiIndex = 0; uiIndex < uiTaskCount; ++uiIndex)
    {
        // If this is a sync task then set this workflow's id to
        // that of its signal task so that the manager can detect
        // its completion
        NiSPTask* pkTask = GetAt(uiIndex);
        if (pkTask->GetSignalType() == NiSPWorkload::SIGNAL_COMPLETION)
        {
            pkSignalTask = pkTask;
            bFoundSignalTask = true;
            break;
        }
    }

    // If there was no signal kernel then add one manually
    if (!bFoundSignalTask)
    {
        pkSignalTask = AddNewTask(0, 0, true);
    }
    m_uiId = pkSignalTask->GetId();

    // Prepare contained tasks
    uiTaskCount = GetSize();
    for (NiUInt32 uiIndex = 0; uiIndex < uiTaskCount; ++uiIndex)
    {
        NiSPTask* pkTask = GetAt(uiIndex);
        pkTask->SetSignalId(m_uiId);
        pkTask->Prepare();
    }

    // Ensure that the workflow has been given an id
    EE_ASSERT(m_uiId > 0 && "Workflow signal id is invalid!");

    return true;
}

//--------------------------------------------------------------------------------------------------
#if !defined(_PS3)
void NiSPWorkflow::ExecuteSerial()
{
    EE_ASSERT(GetStatus() == NiSPWorkflow::RUNNING);

    // Verify there are no jobs in the pending job list queue. This queue
    // should go unused while running with a serial execution model.
    EE_ASSERT(GetNumPendingJobLists() == 0);

    NiUInt32 uiTaskCount = GetSize();
    for (NiUInt32 uiIndex = 0; uiIndex < uiTaskCount; ++uiIndex)
    {
        NiSPTask* pkTask = GetAt(uiIndex);
        if (!pkTask->IsEnabled())
            continue;

        NiSPTaskImpl& kTaskImpl = pkTask->GetImpl();
        NiUInt32 uiListCount = kTaskImpl.GetSize();
        for (NiUInt32 i = 0; i < uiListCount; i++)
        {
            NiSPJobList* pkJobList = kTaskImpl.GetAt(i);
            NiUInt32 uiJobCount = pkJobList->GetJobCount();

            for (NiUInt32 j = 0; j < uiJobCount; j++)
            {
                NiSPWorkload* kWorkload = pkJobList->GetWorkload(j);
                kWorkload->Preload();

                NiSPKernel* pkKernel = kWorkload->GetKernel();
                pkKernel->Execute(*kWorkload);
            }
        }
    }

    ExecutionComplete();
    SetStatus(NiSPWorkflow::COMPLETED);
}
#endif

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::Reset()
{
    EE_ASSERT(m_eStatus != RUNNING);

    // Clear all tasks...
    NiInt32 uiIndex = GetSize();
    while (--uiIndex >= 0)
    {
        NiSPTask* pkTask = GetAt(uiIndex);
        pkTask->Reset();
    }

    // Reset status
    m_iCurrentStage = -1;
    ClearPendingJobLists();
    ResetAtomicUpdateComplete();
}

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::Clear()
{
    EE_ASSERT(m_eStatus != RUNNING);

    // Clear all tasks...
    NiUInt32 uiIndex = GetSize();
    while (uiIndex)
    {
        --uiIndex;
        RemoveAt(uiIndex);
    }

    m_iCurrentStage = -1;
    ClearPendingJobLists();
    m_uiId = 0;
}

//--------------------------------------------------------------------------------------------------
NiSPTask* NiSPWorkflow::FindTask(NiUInt32 uiTaskId)
{
    // Find completion signal task
    NiUInt32 uiTaskCount = GetSize();
    for (NiUInt32 uiIndex = 0; uiIndex < uiTaskCount; ++uiIndex)
    {
        NiSPTask* pkTask = GetAt(uiIndex);
        if (pkTask->GetId() == uiTaskId)
        {
            return pkTask;
        }
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
#if defined(_PS3)
    void NiSPWorkflow::HandleSignal(NiUInt32 uiSignal)
#else
    void NiSPWorkflow::HandleSignal(NiUInt32)
#endif
{
#if defined(_PS3)
    NiSPTask* pkTask = FindTask(uiSignal);
    if (pkTask)
    {
        m_kWorkflowImpl.Execute(pkTask);
    }
#endif
}

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::ExecutionComplete()
{
    // Release workflow implementation class
    m_kWorkflowImpl.Release();

    // Unlock any data streams
    NiUInt32 uiTaskCount = GetSize();
    for (NiUInt32 uiIndex = 0; uiIndex < uiTaskCount; ++uiIndex)
    {
        NiSPTask* pkTask = GetAt(uiIndex);
        pkTask->Finalize();
    }
}

//--------------------------------------------------------------------------------------------------
void NiSPWorkflow::SetPriority(int iPriority)
{
    m_iPriority = iPriority;
}

//--------------------------------------------------------------------------------------------------
int NiSPWorkflow::GetPriority() const
{
    return m_iPriority;
}

//--------------------------------------------------------------------------------------------------
