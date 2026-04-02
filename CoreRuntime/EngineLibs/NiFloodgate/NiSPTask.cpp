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
#include "NiSPTask.h"
#include "NiSPStream.h"
#include "NiSPKernel.h"
#include "NiSPWorkload.h"

//--------------------------------------------------------------------------------------------------
const NiUInt32 NiSPTask::AUTO_BLOCK_COUNT = 0xFFFFFFFF;
NiTObjectPool<NiSPTask>* NiSPTask::ms_pkTaskPool = NULL;
efd::CriticalSection NiSPTask::ms_kTaskPoolLock;

//--------------------------------------------------------------------------------------------------
void NiSPTask::InitializePools(NiUInt32 uiTaskPoolSize)
{
    ms_kTaskPoolLock.Lock();
    EE_ASSERT(ms_pkTaskPool == NULL);
    ms_pkTaskPool = NiNew NiTObjectPool<NiSPTask>(uiTaskPoolSize);
    ms_kTaskPoolLock.Unlock();
}

//--------------------------------------------------------------------------------------------------
void NiSPTask::ShutdownPools()
{
    ms_kTaskPoolLock.Lock();
    EE_ASSERT(ms_pkTaskPool != NULL);
    ms_pkTaskPool->PurgeAllObjects();
    NiDelete ms_pkTaskPool;
    ms_pkTaskPool = NULL;
    ms_kTaskPoolLock.Unlock();
}

//--------------------------------------------------------------------------------------------------
NiSPTaskPtr NiSPTask::GetNewTask(const NiUInt16 uiNumInputs,
    const NiUInt16 uiNumOutputs)
{
    ms_kTaskPoolLock.Lock();
    NiSPTask* pkTask = ms_pkTaskPool->GetFreeObject();
    ms_kTaskPoolLock.Unlock();

    EE_ASSERT(pkTask->m_pkWorkflow == NULL);
    EE_ASSERT(pkTask->m_uiSignalId == 0);
    EE_ASSERT(pkTask->m_pkKernel == NULL);
    EE_ASSERT(pkTask->m_uiSliceSize == 0);
    EE_ASSERT(pkTask->m_eStatus == IDLE);
    EE_ASSERT(pkTask->m_uiSyncData == 0);
    EE_ASSERT(pkTask->m_uiOptimalBlockCount == AUTO_BLOCK_COUNT);
    EE_ASSERT(pkTask->m_usStage == 0);
    EE_ASSERT(pkTask->m_uFlags ==
        (FLAG_IS_DATA_DECOMP_ENABLED | FLAG_IS_ENABLED | FLAG_IS_RESET));
    EE_ASSERT(pkTask->m_eSignalType == NiSPWorkload::SIGNAL_NONE);
    EE_ASSERT(pkTask->m_uiTaskId == (
        reinterpret_cast<NiUInt64>(pkTask) & 0xFFFFFF));
    EE_ASSERT(pkTask->GetInputCount() == 0);
    EE_ASSERT(pkTask->GetOutputCount() == 0);

    pkTask->m_kInputStreams.ReallocNoShrink(uiNumInputs);
    pkTask->m_kOutputStreams.ReallocNoShrink(uiNumOutputs);

    return pkTask; //NiUInt8
}

//--------------------------------------------------------------------------------------------------
void NiSPTask::DeleteThis()
{
    ms_kTaskPoolLock.Lock();

    // Only one task can be clearing at any given time.
    Clear(true);

    ms_pkTaskPool->ReleaseObject(this);
    ms_kTaskPoolLock.Unlock();
}

//--------------------------------------------------------------------------------------------------
NiSPTask::NiSPTask() :
    m_kStreamsLock("SPTaskL"),
    m_pkWorkflow(NULL),
    m_uiTaskId(0),
    m_uiSignalId(0),
    m_pkKernel(NULL),
    m_uiSliceSize(0),
    m_eStatus(IDLE),
    m_uiSyncData(0),
    m_uiOptimalBlockCount(AUTO_BLOCK_COUNT),
    m_usStage(0),
    m_uFlags(FLAG_IS_DATA_DECOMP_ENABLED | FLAG_IS_ENABLED | FLAG_IS_RESET),
    m_eSignalType(NiSPWorkload::SIGNAL_NONE)
{
    SetId(static_cast<NiUInt32>(
        reinterpret_cast<NiUInt64>(this) & 0xFFFFFF));
}

//--------------------------------------------------------------------------------------------------
NiSPTask::~NiSPTask()
{
}

//--------------------------------------------------------------------------------------------------
void NiSPTask::Prepare()
{
    LockStreams();

    if (!IsEnabled())
        return;

    // Lock output streams
    NiUInt32 uiOutputStreamCount = m_kOutputStreams.GetSize();
    for (NiUInt32 uiIndex = 0; uiIndex < uiOutputStreamCount; uiIndex++)
    {
        NiSPStream* pkStream = m_kOutputStreams.GetAt(uiIndex);
        pkStream->Lock();
    }

    // Lock input streams
    NiUInt32 uiInputStreamCount = m_kInputStreams.GetSize();
    for (NiUInt32 uiIndex = 0; uiIndex < uiInputStreamCount; ++uiIndex)
    {
        NiSPStream* pkStream = m_kInputStreams.GetAt(uiIndex);
        pkStream->Lock();
    }

    if (IsCached())
    {
        return;
    }

    // Minimize memory use if told to do so.
    if (IsCompacted())
    {
        m_kInputStreams.Realloc();
        m_kOutputStreams.Realloc();
    }

    // If there are no streams then this is a sync task
    NiUInt32 uiCount = GetTotalCount();

    EE_ASSERT(m_pkKernel && "Error: NULL Kernel!");

#if defined(_PS3)
    // If a PPU Kernel was specified then turn this
    // into a PPU Notify signal task
    if (m_pkKernel->IsPPUKernel())
    {
        SetSignalType(NiSPWorkload::SIGNAL_PPU_TASK_NOTIFY);
        SetSyncData(GetId());
        SetIsDataDecompositionEnabled(false);
    }
#endif

    SetBit(GetSignalType() != NiSPWorkload::SIGNAL_NONE, FLAG_IS_SYNC);

    // Track how many streams are aligned
    NiUInt32 uiMisalignedStreamCount = uiCount;

    // Prepare output streams tracking alignment, and slice size
    // Output streams are prepare first so write locks on
    // data streams happen first. Subsequent read locks
    // will promote the locks to read / write locks
    NiUInt32 uiLeafStreamCount = uiOutputStreamCount;
    for (NiUInt32 uiIndex = 0; uiIndex < uiOutputStreamCount; uiIndex++)
    {
        NiSPStream* pStream = m_kOutputStreams.GetAt(uiIndex);
        pStream->Prepare();
        if (pStream->IsDataAligned())
        {
            uiMisalignedStreamCount--;
        }

        if (pStream->GetEffectiveInputSize() == 0)
        {
            uiLeafStreamCount--;
        }
        m_uiSliceSize += pStream->GetStride();
    }
    SetBit(!uiLeafStreamCount, FLAG_IS_LEAF);

    // Prepare input streams tracking alignment, and slice size
    // Lock input streams for read
    NiUInt32 uiRootStreamCount = uiInputStreamCount;
    for (NiUInt32 uiIndex = 0; uiIndex < uiInputStreamCount; ++uiIndex)
    {
        NiSPStream* pStream = m_kInputStreams.GetAt(uiIndex);
        pStream->Prepare();
        if (pStream->IsDataAligned())
        {
            uiMisalignedStreamCount--;
        }

        if (pStream->GetEffectiveOutputSize() == 0)
        {
            uiRootStreamCount--;
        }

        m_uiSliceSize += pStream->GetStride();
    }
    SetBit(!uiRootStreamCount, FLAG_IS_ROOT);

    // Save alignment
    SetBit(!uiMisalignedStreamCount, FLAG_IS_ALIGNED);

    // Initialize the task implementation
    m_kImpl.Initialize(this);

    // Set status to pending
    SetStatus(PENDING);

    // This task has been initialized now
    SetBit(false, FLAG_IS_RESET);
}

//--------------------------------------------------------------------------------------------------
void NiSPTask::Finalize()
{
    if (!IsEnabled())
    {
        UnlockStreams();
        return;
    }

    NiUInt32 uiInputStreamCount = m_kInputStreams.GetSize();
    for (NiUInt32 uiIndex = 0; uiIndex < uiInputStreamCount; ++uiIndex)
    {
        NiSPStream* pkStream = m_kInputStreams.GetAt(uiIndex);
        pkStream->Unlock();
    }

    // Prepare output streams tracking alignment, and slice size
    NiUInt32 uiOutputStreamCount = m_kOutputStreams.GetSize();
    for (NiUInt32 uiIndex = 0; uiIndex < uiOutputStreamCount; uiIndex++)
    {
        NiSPStream* pkStream = m_kOutputStreams.GetAt(uiIndex);
        pkStream->Unlock();
    }

    UnlockStreams();
}

//--------------------------------------------------------------------------------------------------
