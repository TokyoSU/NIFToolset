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

#include <efd/Utilities.h>
#include <efd/MemTracker.h>
#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>
#include <efd/IMemLogHandler.h>
#include <efd/PathUtils.h>
#include <efd/StackUtils.h>
#include <efd/DefaultInitializeMemoryLogHandler.h>
#include <efd/StaticDataManager.h>
#include <efd/Profiler.h>

using namespace efd;
using namespace efd::StackUtils;

bool efd::gs_bDisableMemLogging = false;

const FLF FLF::UNKNOWN("",0,"");

size_t MemTracker::ms_stBreakOnAllocID = static_cast<size_t>(-1);
void* MemTracker::ms_pvBreakOnAllocRangeStart = NULL;
void* MemTracker::ms_pvBreakOnAllocRangeEnd = NULL;
const char* MemTracker::ms_pcBreakOnFunctionName = "@*";
MemTracker* MemTracker::ms_pkTracker = NULL;

bool MemTracker::ms_bOutputLeaksToDebugStream = true;
size_t MemTracker::ms_stBreakOnSizeRequested = INT_MAX;

//#define MEM_LOG_COMPLETE    NIMESSAGE_MEMORY_0
//#define MEM_LOG_LEAK        NIMESSAGE_MEMORY_1
#define MEM_LOG_COMPLETE    0
#define MEM_LOG_LEAK        1
// Use this setting to determine whether or not to flush the log file
// every time a line is sent or wait until the internal buffer is
// full to flush the log file.
#define MEM_FLUSH_LOG_ON_WRITE false

#if defined(EE_MEMTRACKER_DETAILEDREPORTING)
    #define EE_MTDR_ONLY(x) x
    #define EE_MTDR_ONLY_ARG(x) , x
#else
    #define EE_MTDR_ONLY(x)
    #define EE_MTDR_ONLY_ARG(x)
#endif // defined(EE_MEMTRACKER_DETAILEDREPORTING)

#if defined(EE_MEMTRACKER_STACKTRACE)
    #define EE_MTST_ONLY(x) x
    #define EE_MTST_ONLY_ARG(x) , x
#else
    #define EE_MTST_ONLY(x)
    #define EE_MTST_ONLY_ARG(x)
#endif // defined(EE_MEMTRACKER_STACKTRACE)

//------------------------------------------------------------------------------------------------
EE_PROFILER_CONTEXT(Memory);
//------------------------------------------------------------------------------------------------
void MemTracker::Initialize()
{
    m_pkMemLogHandler = NULL;
    m_stCurrentAllocID = 0;
    ResetSummaryStats();

    if (m_pkActualAllocator)
        m_pkActualAllocator->Initialize();
}

//------------------------------------------------------------------------------------------------
void MemTracker::PerThreadInit()
{
    if (m_pkActualAllocator)
        m_pkActualAllocator->PerThreadInit();
}

//------------------------------------------------------------------------------------------------
void MemTracker::CreateMemoryLogHandler()
{
    if (!m_pkMemLogHandler)
    {
        // Suppress the pre-main warning assert during the creation of the memory log handler
        m_stIgnoreRangeEnd = (size_t)(-1);

        // Create the memory log handler, noting which allocations were part of it and should not
        // be counted as a leak
        m_stIgnoreRangeStart = GetCurrentAllocationID();
        m_pkMemLogHandler = InitializeMemoryLogHandler();

        if (m_pkMemLogHandler)
            m_pkMemLogHandler->OnInit();

        m_stIgnoreRangeEnd = GetCurrentAllocationID();
    }
}

//------------------------------------------------------------------------------------------------
void MemTracker::Shutdown()
{
    // Now delete the memory logger. At this point all memory allocations/leaks have been reported.
    // Those reports explicitly ignored all allocations performed by the m_pkMemLogHandler since
    // the it is not deleted until after the reports have and produced. Including m_pkMemLogHandler
    // allocations in the report would result in 'false negative' memory leak reports.
    if (m_pkMemLogHandler)
    {
        m_pkMemLogHandler->OnShutdown();

        // Here we need to ensure that the MemTracker does not attempt to log the deletion of
        // memory logger (recursive death). To avoid this we simply copy the pointer address and
        // set m_pkMemLogHandler to NULL early. This ensures the MemTraker will not attempt to log
        // the deletion.
        IMemLogHandler* pkTempPointer = m_pkMemLogHandler;
        m_pkMemLogHandler = NULL;
        EE_DELETE pkTempPointer;
    }

#if defined(EE_EFD_CONFIG_DEBUG)
    if (m_stActiveAllocationCount != 0)
    {
        char acString[256];
        efd::Sprintf(acString, 256, "         There are %d leaked allocations"
            ".\n", m_stActiveAllocationCount);
        EE_OUTPUT_DEBUG_STRING("\n\n//----------------------------------------"
            "-----------------------------------\n");
        EE_OUTPUT_DEBUG_STRING("//         MemTracker Report:\n");
        EE_OUTPUT_DEBUG_STRING("//-------------------------------------------"
            "--------------------------------\n");
        EE_OUTPUT_DEBUG_STRING(acString);
        EE_OUTPUT_DEBUG_STRING("//-------------------------------------------"
            "--------------------------------\n");

        OutputLeakedMemoryToDebugStream(false);

        EE_OUTPUT_DEBUG_STRING("//-------------------------------------------"
            "--------------------------------\n");
    }
    else
    {
        EE_OUTPUT_DEBUG_STRING("\n\n//----------------------------------------"
            "-----------------------------------\n");
        EE_OUTPUT_DEBUG_STRING("//         MemTracker Report: No leaked allocations detected.\n");
        EE_OUTPUT_DEBUG_STRING("//-------------------------------------------"
            "--------------------------------\n");
    }
#endif

    if (m_pkActualAllocator)
        m_pkActualAllocator->Shutdown();
}

//------------------------------------------------------------------------------------------------
void MemTracker::PerThreadShutdown()
{
    if (m_pkActualAllocator)
        m_pkActualAllocator->PerThreadShutdown();
}

//------------------------------------------------------------------------------------------------
void MemTracker::ResetSummaryStats()
{
    m_fPeakMemoryTime = 0.0f;
    m_fPeakAllocationCountTime = 0.0f;
    m_stActiveMemory = 0;
    m_stPeakMemory = 0;
    m_stAccumulatedMemory = 0;
    m_stUnusedButAllocatedMemory = 0;

    m_stActiveAllocationCount = 0;
    m_stPeakAllocationCount = 0;
    m_stAccumulatedAllocationCount = 0;

    m_stActiveExternalMemory = 0;
    m_stPeakExternalMemory = 0;
    m_stAccumulatedExternalMemory = 0;

    m_stActiveExternalAllocationCount = 0;
    m_stPeakExternalAllocationCount = 0;
    m_stAccumulatedExternalAllocationCount = 0;
}

//------------------------------------------------------------------------------------------------
MemTracker::MemTracker(IAllocator* pkActualAllocator,
    bool bWriteToLog,
    unsigned int uiInitialSize,
    unsigned int uiGrowBy,
    bool bAlwaysValidateAll,
    bool bCheckArrayOverruns,
    bool assertOutsideOfMain)
    : m_fPeakMemoryTime(0.0f)
    , m_fPeakAllocationCountTime(0.0f)
    , m_ppManagedAllocUnitAddresses(NULL)
    , m_pstManagedAllocUnitSizes(NULL)
    , m_uiManagedAllocUnitCount(0)
    , m_uiManagedAllocUnitMaxSize(0)
    , m_bAlwaysValidateAll(bAlwaysValidateAll)
    , m_stReservoirGrowBy(uiGrowBy)
    , m_pkMemLogHandler(NULL)
    , m_stIgnoreRangeStart(0)
    , m_stIgnoreRangeEnd(0)
    , m_bCheckArrayOverruns(bCheckArrayOverruns)
    , m_ucFillChar(0xbd)
    , m_bWriteToLog(bWriteToLog)
    , m_insideOfMain(false)
    , m_assertOutsideOfMain(assertOutsideOfMain)
#if defined(EE_MEMTRACKER_SNAPSHOT)
    , m_bSnapshotActive(false)
    , m_pSnapshotHead(NULL)
#endif
{
    ms_pkTracker = this;
    m_pkActualAllocator = pkActualAllocator;
    memset(m_pkActiveMem, 0, ms_uiHashSize*sizeof(AllocUnit*));
    if (uiInitialSize > 0)
    {
        // create initial set of tracking data structures
        m_stReservoirBufferSize = 1;
        m_pkReservoir = (AllocUnit*)EE_EXTERNAL_CALLOC(uiInitialSize,
            sizeof(AllocUnit));

        // If you hit this EE_MEMASSERT, then the memory manager failed to
        // allocate internal memory for tracking the allocations
        EE_MEMASSERT(m_pkReservoir != NULL);

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
        // throw an exception in this situation
        if (m_pkReservoir == NULL)
            throw "Unable to allocate RAM for internal memory tracking data";
#endif

        // initialize the allocation units
        for (unsigned int i = 0; i < uiInitialSize-1; i++)
        {
            m_pkReservoir[i].m_kFLF = FLF::UNKNOWN;
            m_pkReservoir[i].m_pkNext = &m_pkReservoir[i+1];
        }
        m_pkReservoir[uiInitialSize-1].m_kFLF = FLF::UNKNOWN;
        m_pkReservoir[uiInitialSize-1].m_pkNext = NULL;

        m_ppkReservoirBuffer = (AllocUnit**)EE_EXTERNAL_MALLOC(sizeof(AllocUnit*));

        // If you hit this EE_MEMASSERT, then the memory manager failed to
        // allocate internal memory for tracking the allocations
        EE_MEMASSERT(m_ppkReservoirBuffer != NULL);

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
        // throw an exception in this situation
        if (m_ppkReservoirBuffer == NULL)
            throw "Unable to allocate RAM for internal memory tracking data";
#endif

        m_ppkReservoirBuffer[0] = m_pkReservoir;

        unsigned int uiInitialBytes = uiInitialSize*sizeof(AllocUnit) +
            sizeof(AllocUnit*);
        m_stActiveTrackerOverhead = uiInitialBytes;
        m_stPeakTrackerOverhead = uiInitialBytes;
        m_stAccumulatedTrackerOverhead = uiInitialBytes;
    }
    else
    {
        m_stReservoirBufferSize = 0;
    }
}

//------------------------------------------------------------------------------------------------
MemTracker::~MemTracker()
{
    // Free all memory used by the hash table of alloc units
    if (m_ppkReservoirBuffer)
    {
        for (unsigned int i = 0; i < m_stReservoirBufferSize; i++)
        {
            EE_EXTERNAL_FREE(m_ppkReservoirBuffer[i]);
        }
        EE_EXTERNAL_FREE(m_ppkReservoirBuffer);
    }

    ms_pkTracker = NULL;

    EE_EXTERNAL_DELETE m_pkActualAllocator;
}

//------------------------------------------------------------------------------------------------
void MemTracker::GrowReservoir()
{
    EE_MEMASSERT(!m_pkReservoir);

    m_pkReservoir = EE_EXTERNAL_ALLOC(AllocUnit, m_stReservoirGrowBy);

    // If you hit this EE_MEMASSERT, then the memory manager failed to allocate
    // internal memory for tracking the allocations
    EE_MEMASSERT(m_pkReservoir != NULL);

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
    // throw an exception in this situation
    if (m_pkReservoir == NULL)
        throw "Unable to allocate RAM for internal memory tracking data";
#endif

        m_stActiveTrackerOverhead += sizeof(AllocUnit) *
            m_stReservoirGrowBy + sizeof(AllocUnit*);

        if (m_stActiveTrackerOverhead >
            m_stPeakTrackerOverhead)
        {
            m_stPeakTrackerOverhead =
                m_stActiveTrackerOverhead;
        }
        m_stAccumulatedTrackerOverhead += sizeof(AllocUnit) *
            m_stReservoirGrowBy + sizeof(AllocUnit*);

    // Build a linked-list of the elements in the reservoir
    // Initialize the allocation units
    for (unsigned int i = 0; i < m_stReservoirGrowBy-1; i++)
    {
        m_pkReservoir[i].m_kFLF = FLF::UNKNOWN;
        m_pkReservoir[i].m_pkNext = &m_pkReservoir[i+1];
    }
    m_pkReservoir[m_stReservoirGrowBy-1].m_kFLF = FLF::UNKNOWN;
    m_pkReservoir[m_stReservoirGrowBy-1].m_pkNext = NULL;

    // Add this address to the reservoir buffer so it can be freed later
    AllocUnit **pkTemp = (AllocUnit**) EE_EXTERNAL_REALLOC(
        m_ppkReservoirBuffer,
        (m_stReservoirBufferSize + 1) * sizeof(AllocUnit*));

    // If you hit this EE_MEMASSERT, then the memory manager failed to allocate
    // internal memory for tracking the allocations
    EE_MEMASSERT(pkTemp != NULL);
    if (pkTemp)
    {
        m_ppkReservoirBuffer = pkTemp;
        m_ppkReservoirBuffer[m_stReservoirBufferSize] = m_pkReservoir;
        m_stReservoirBufferSize++;
    }
#if defined(EE_MEMORY_ENABLE_EXCEPTIONS)
    else
    {
        // throw an exception in this situation
        throw "Unable to allocate RAM for internal memory tracking data";
    }
#endif
}

//------------------------------------------------------------------------------------------------
void* MemTracker::Allocate(
    size_t& stSizeInBytes,
    size_t& stAlignment,
    MemHint kHint,
    MemEventType eEventType,
    const char* pcFile,
    int iLine,
    const char* pcFunction)
{
    m_kCriticalSection.Lock();

    // If you hit this EE_MEMASSERT, you are allocating before EE_INIT was called, or after
    // EE_SHUTDOWN was called.  The ability to allocate through Gamebryo outside of pre-main
    // or post-main is deprecated and will be removed in a future release.
    EE_MEMASSERT("Warning: Using the Gamebryo allocator outside of EE_INIT/EE_SHUTDOWN "
        "is deprecated." && (CanAllocateNow() ||
         ((m_stCurrentAllocID >= m_stIgnoreRangeStart) &&
         (m_stCurrentAllocID < m_stIgnoreRangeEnd))));

    size_t stSizeOriginal = stSizeInBytes;
    double fTime = efd::GetCurrentTimeInSec();

    if (m_bCheckArrayOverruns)
    {
       stSizeInBytes = PadForArrayOverrun(stAlignment, stSizeInBytes);
    }

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific
    // function name.
    EE_MEMASSERT(strcmp(ms_pcBreakOnFunctionName, pcFunction) != 0);

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific
    // allocation ID.
    EE_MEMASSERT(ms_stBreakOnAllocID == static_cast<size_t>(-1) ||
        ms_stBreakOnAllocID != m_stCurrentAllocID);

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific
    // allocation request size.
    EE_MEMASSERT(ms_stBreakOnSizeRequested != stSizeOriginal);

    // Actually perform the allocation. Note that size and alignment
    // may be adjusted by the call.
    void* pvMem = m_pkActualAllocator->Allocate(stSizeInBytes, stAlignment,
        kHint, eEventType, pcFile, iLine, pcFunction);

    if (pvMem == NULL)
    {
        // If you hit this, your memory request was not satisfied
        m_kCriticalSection.Unlock();
        return NULL;
    }

    EE_MEMASSERT(!FindAllocUnit(pvMem));

    // update summary statistics
    m_stActiveAllocationCount++;
    m_stAccumulatedAllocationCount++;
    if (m_stActiveAllocationCount > m_stPeakAllocationCount)
    {
        m_stPeakAllocationCount = m_stActiveAllocationCount;
        m_fPeakAllocationCountTime = fTime;
    }

    m_stActiveMemory += stSizeInBytes;
    m_stAccumulatedMemory += stSizeInBytes;
    if (m_stActiveMemory > m_stPeakMemory)
    {
        m_stPeakMemory = m_stActiveMemory;
        m_fPeakMemoryTime = fTime;
    }

    // If you hit this EE_MEMASSERT, your memory request result was smaller
    // than the input.
    EE_MEMASSERT(stSizeInBytes >= stSizeOriginal);

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific
    // allocation address range.
    EE_MEMASSERT(!IsInsideBreakRange(pvMem, stSizeInBytes));

    // Pad the start and end of the allocation with the pad character
    // so that we can check for array under and overruns. Note that the
    // address is shifted to hide the padding before the allocation.
    if (m_bCheckArrayOverruns)
    {
        MemoryFillForArrayOverrun(pvMem, stAlignment, stSizeOriginal);
    }

    // Fill the originally requested memory size with the pad character.
    // This will allow us to see how much of the allocation was
    // actually used.
    MemoryFillWithPattern(pvMem, stSizeOriginal);

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
    try
    {
#endif

    // If you hit this EE_MEMASSERT, the somehow you have allocated a memory
    // unit to an address that already exists. This should never happen
    // and is an indicator that something has gone wrong in the sub-allocator.
    EE_MEMASSERT(FindAllocUnit(pvMem) == NULL);

    // Grow the tracking unit reservoir if necessary
    if (!m_pkReservoir)
        GrowReservoir();

    // If you hit this EE_MEMASSERT, the free store for allocation units
    // does not exist. This should only happen if the reservoir
    // needed to grow and was unable to satisfy the request. In other words,
    // you may be out of memory.
    EE_MEMASSERT (m_pkReservoir != NULL);

    // Get an allocation unit from the reservoir
    AllocUnit* pkUnit = m_pkReservoir;
    m_pkReservoir = pkUnit->m_pkNext;

    // fill in the known information
    pkUnit->Reset();
    pkUnit->m_stAllocationID = m_stCurrentAllocID;
    pkUnit->m_stAlignment = stAlignment;
    pkUnit->m_ulAllocThreadId = GetCurrentThreadId();
    pkUnit->m_eAllocType = eEventType;
    pkUnit->m_kAllocHint = kHint;
    pkUnit->m_fAllocTime = (float)fTime;
    pkUnit->m_pvMem = pvMem;
    pkUnit->m_stSizeRequested = stSizeOriginal;
    pkUnit->m_stSizeAllocated = stSizeInBytes;
    pkUnit->m_kFLF.Set(pcFile, iLine, pcFunction);
#if defined(EE_MEMTRACKER_STACKTRACE)
    // Grab a stack trace skipping the top two frames (ourself and our caller) which we know
    // are uninteresting allocation related methods.
    pkUnit->m_stackSize = FastStackTrace(pkUnit->m_stack, EE_ARRAYSIZEOF(pkUnit->m_stack), 2);
#endif

    EE_PROFILER_MEM_ALLOC(Memory, pkUnit->m_pvMem, pkUnit->m_stSizeRequested, 
        pcFunction, pcFile, iLine);

    if (m_bWriteToLog && m_pkMemLogHandler && m_pkMemLogHandler->IsInitialized())
    {
        // Log the allocation
        m_pkMemLogHandler->LogAllocUnit(pkUnit, "\t");
    }

    // If you hit this EE_MEMASSERT, then this allocation was made from a
    // source that isn't setup to use this memory tracking software, use the
    // stack frame to locate the source and include MemManager.h.
    EE_MEMASSERT_UNIT(eEventType != EE_MET_UNKNOWN, pkUnit);

    // Insert the new allocation into the hash table
    InsertAllocUnit(pkUnit);

    // Validate every single allocated unit in memory
    if (m_bAlwaysValidateAll)
    {
        bool bValidateAllAllocUnits = ValidateAllAllocUnits();
        EE_UNUSED_ARG(bValidateAllAllocUnits);
        EE_MEMASSERT_UNIT(bValidateAllAllocUnits, pkUnit);
    }

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
    }
    catch(const char *err)
    {
        // Deal with the errors
        // Deal with the errors
    }
#endif

    ++m_stCurrentAllocID;
    m_kCriticalSection.Unlock();

    return pvMem;
}

//------------------------------------------------------------------------------------------------
void MemTracker::InsertAllocUnit(AllocUnit* pkUnit)
{
    EE_MEMASSERT(pkUnit != NULL && pkUnit->m_pvMem != NULL);
    unsigned int uiHashIndex = AddressToHashIndex(pkUnit->m_pvMem);

    // Remap the new allocation unit to the head of the hash entry
    if (m_pkActiveMem[uiHashIndex])
    {
        EE_MEMASSERT_UNIT(m_pkActiveMem[uiHashIndex]->m_pkPrev == NULL, pkUnit);
        m_pkActiveMem[uiHashIndex]->m_pkPrev = pkUnit;
    }

    EE_MEMASSERT_UNIT(pkUnit->m_pkNext == NULL, pkUnit);
    pkUnit->m_pkNext = m_pkActiveMem[uiHashIndex];
    pkUnit->m_pkPrev = NULL;
    m_pkActiveMem[uiHashIndex] = pkUnit;

#if defined(EE_MEMTRACKER_SNAPSHOT)
    if (m_bSnapshotActive)
    {
        if (m_pSnapshotHead)
        {
            EE_MEMASSERT_UNIT(m_pSnapshotHead->m_pShapshotPrev == NULL, pkUnit);
            m_pSnapshotHead->m_pShapshotPrev = pkUnit;
        }
        EE_MEMASSERT_UNIT(pkUnit->m_pShapshotNext == NULL, pkUnit);
        pkUnit->m_pShapshotNext = m_pSnapshotHead;
        pkUnit->m_pShapshotPrev = NULL;
        m_pSnapshotHead = pkUnit;
    }
#endif
}

//------------------------------------------------------------------------------------------------
void MemTracker::RemoveAllocUnit(AllocUnit* pkUnit)
{
    EE_MEMASSERT(pkUnit != NULL && pkUnit->m_pvMem != NULL);
    unsigned int uiHashIndex = AddressToHashIndex(pkUnit->m_pvMem);

    // If you hit this EE_MEMASSERT, somehow we have emptied the
    // hash table for this bucket. This should not happen
    // and is indicative of a serious error in the memory
    // tracking infrastructure.
    EE_MEMASSERT_UNIT(m_pkActiveMem[uiHashIndex] != NULL, pkUnit);

    if (m_pkActiveMem[uiHashIndex] == pkUnit)
    {
        EE_MEMASSERT_UNIT(pkUnit->m_pkPrev == NULL, pkUnit);
        m_pkActiveMem[uiHashIndex] = pkUnit->m_pkNext;

        if (m_pkActiveMem[uiHashIndex])
        {
            m_pkActiveMem[uiHashIndex]->m_pkPrev = NULL;
        }
    }
    else
    {
        if (pkUnit->m_pkPrev)
        {
            EE_MEMASSERT_UNIT(pkUnit->m_pkPrev->m_pkNext == pkUnit, pkUnit);
            pkUnit->m_pkPrev->m_pkNext = pkUnit->m_pkNext;
        }
        if (pkUnit->m_pkNext)
        {
            EE_MEMASSERT_UNIT(pkUnit->m_pkNext->m_pkPrev == pkUnit, pkUnit);
            pkUnit->m_pkNext->m_pkPrev = pkUnit->m_pkPrev;
        }
    }

#if defined(EE_MEMTRACKER_SNAPSHOT)
    // We always remove deleted allocations from the snapshot even when the snapshot isn't active.
    // The idea is that you can do a snapshot of all allocations between Point A and Point B and
    // then at some later time (perhaps after a garbage collection pass, for example) you can ask
    // "what allocations from that snapshot are still outstanding?". This is used to detect cases
    // where memory use might be growing unexpectedly even though that memory isn't truly leaked
    // and would eventually be cleaned up, perhaps not until final shutdown.
    if (m_pSnapshotHead == pkUnit)
    {
        EE_MEMASSERT_UNIT(pkUnit->m_pShapshotPrev == NULL, pkUnit);
        m_pSnapshotHead = pkUnit->m_pShapshotNext;
        if (m_pSnapshotHead)
        {
            m_pSnapshotHead->m_pShapshotPrev = NULL;
        }
    }
    else
    {
        if (pkUnit->m_pShapshotPrev)
        {
            EE_MEMASSERT_UNIT(pkUnit->m_pShapshotPrev->m_pShapshotNext == pkUnit, pkUnit);
            pkUnit->m_pShapshotPrev->m_pShapshotNext = pkUnit->m_pShapshotNext;
        }
        if (pkUnit->m_pShapshotNext)
        {
            EE_MEMASSERT_UNIT(pkUnit->m_pShapshotNext->m_pShapshotPrev == pkUnit, pkUnit);
            pkUnit->m_pShapshotNext->m_pShapshotPrev = pkUnit->m_pShapshotPrev;
        }
    }
#endif
}

//------------------------------------------------------------------------------------------------
void* MemTracker::Reallocate(
    void* pvMemory,
    size_t& stSizeInBytes,
    size_t& stAlignment,
    MemHint kHint,
    MemEventType eEventType,
    size_t stSizeCurrent,
    const char* pcFile,
    int iLine,
    const char* pcFunction)
{
    // Store the original request size for later use.
    size_t stSizeOriginal = stSizeInBytes;

    // If the address is null, then this function should behave the same as an allocation routine.
    if (pvMemory == NULL)
        return Allocate(stSizeInBytes, stAlignment, kHint, eEventType, pcFile, iLine, pcFunction);

    // If the requested size is 0, then this function should behave the same as a deallocation
    // routine.
    if (stSizeInBytes == 0)
    {
        Deallocate(pvMemory, eEventType, EE_MEM_DEALLOC_SIZE_DEFAULT);
        return NULL;
    }

    m_kCriticalSection.Lock();

    double fTime = efd::GetCurrentTimeInSec();

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
    try
    {
#endif
    // A reallocation is tracked with two allocation units.  The first is the old unit.  The
    // second unit tracks the new allocation, which may or may not have the same address as the
    // old allocation.  Its allocation type is set to the event type passed into this function.

    AllocUnit* pkUnit = FindAllocUnit(pvMemory);

    if (pkUnit == NULL)
    {
        // If you hit this EE_MEMASSERT, you tried to reallocate RAM that wasn't allocated by this
        // memory manager.
        EE_MEMASSERT(pkUnit != NULL);

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
        throw "Request to reallocate RAM that was never allocated";
#endif

        m_kCriticalSection.Unlock();

        return NULL;
    }

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific function name.
    EE_MEMASSERT(strcmp(ms_pcBreakOnFunctionName, pcFunction) != 0);

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation ID.
    EE_MEMASSERT(ms_stBreakOnAllocID ==
        static_cast<size_t>(-1) || ms_stBreakOnAllocID != pkUnit->m_stAllocationID);

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation request
    // size.
    EE_MEMASSERT(ms_stBreakOnSizeRequested != pkUnit->m_stSizeRequested);

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation address
    // range.
    EE_MEMASSERT(!IsInsideBreakRange(pkUnit->m_pvMem, pkUnit->m_stSizeRequested));

    // If you hit this EE_MEMASSERT, then the allocation unit that is about to be deleted
    // requested an initial size that doesn't match what is currently the 'size' argument for
    // deallocation.  This is most commonly caused by the lack of a virtual destructor for a
    // class that is used polymorphically in an array.
    if (stSizeCurrent != EE_MEM_DEALLOC_SIZE_DEFAULT)
        EE_MEMASSERT(stSizeCurrent == pkUnit->m_stSizeRequested);

    if (m_bCheckArrayOverruns)
    {
        // If you hit this EE_MEMASSERT, you have code that overwrites either before or after the
        // range of an allocation.  Check the pkUnit for information about which allocation is
        // being overwritten.

        bool bCheckForArrayOverrun =
            CheckForArrayOverrun(pvMemory, pkUnit->m_stAlignment, pkUnit->m_stSizeRequested);
        EE_UNUSED_ARG(bCheckForArrayOverrun);
        EE_MEMASSERT_UNIT(!bCheckForArrayOverrun, pkUnit);

        stSizeInBytes = PadForArrayOverrun(stAlignment, stSizeInBytes);

        if (stSizeCurrent != EE_MEM_DEALLOC_SIZE_DEFAULT)
            stSizeCurrent = PadForArrayOverrun(pkUnit->m_stAlignment, stSizeCurrent);
    }

    // If you hit this EE_MEMASSERT, then the allocation unit that is about to be reallocated is
    // damaged.
    EE_MEMASSERT(ValidateAllocUnit(pkUnit));

    // Alignment should be the same between reallocations.
    EE_MEMASSERT(pkUnit->m_stAlignment == stAlignment);

    // Determine how much memory was actually set.
    size_t stSizeUnused = MemoryBytesWithPattern(pkUnit->m_pvMem, pkUnit->m_stSizeRequested);
    m_stUnusedButAllocatedMemory += stSizeUnused;

    // Save the thread id that freed the memory.
    unsigned long ulFreeThreadId = GetCurrentThreadId();

    // Perform the actual memory reallocation.
    void* pvNewMemory = m_pkActualAllocator->Reallocate(
        pvMemory,
        stSizeInBytes,
        stAlignment,
        kHint,
        eEventType,
        stSizeCurrent,
        pcFile,
        iLine,
        pcFunction);

    // If you hit this EE_MEMASSERT, then the reallocation was unable to be satisfied.
    EE_MEMASSERT(pvNewMemory != NULL);

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation address
    // range.
    EE_MEMASSERT(!IsInsideBreakRange(pvNewMemory, stSizeInBytes));

    // Update summary statistics.
    size_t stPreReallocSize = pkUnit->m_stSizeRequested;

    m_stAccumulatedAllocationCount++;

    // Determine whether the post-realloc size is larger or smaller than the original size, and
    // update the statistics accordingly.
    size_t stIncreaseInSize = 0;
    if (stSizeInBytes >= pkUnit->m_stSizeAllocated)
    {
        stIncreaseInSize = stSizeInBytes - pkUnit->m_stSizeAllocated;
        m_stActiveMemory += stIncreaseInSize;
        m_stAccumulatedMemory += stIncreaseInSize;
    }
    else
    {
        m_stActiveMemory -= pkUnit->m_stSizeAllocated - stSizeInBytes;
    }

    if (m_stActiveMemory > m_stPeakMemory)
    {
        m_stPeakMemory = m_stActiveMemory;
        m_fPeakMemoryTime = fTime;
    }

    // Pad the start and end of the allocation with the pad character so that we can check for
    // array under- and overruns.
    if (m_bCheckArrayOverruns)
    {
        MemoryFillForArrayOverrun(pvNewMemory, stAlignment, stSizeOriginal);
    }

    // Fill the originally requested memory size with the pad character.  This step will enable us
    // to see how much of the allocation was actually used.  For reallocation, we only want to
    // fill the unused section that was just allocated.
    if (stIncreaseInSize)
    {
        MemoryFillWithPattern((char*)pvNewMemory + stPreReallocSize, stIncreaseInSize);
    }

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation ID.
    EE_MEMASSERT(ms_stBreakOnAllocID ==
        static_cast<size_t>(-1) || m_stCurrentAllocID != ms_stBreakOnAllocID);

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation request
    // size.
    EE_MEMASSERT(ms_stBreakOnSizeRequested != stSizeOriginal);

    // If you hit this EE_MEMASSERT, then this reallocation was made from a that isn't setup to
    // use this memory tracking software, use the stack source frame to locate the source and
    // include MemManager.h.
    EE_MEMASSERT(eEventType != EE_MET_UNKNOWN);

    // If you hit this EE_MEMASSERT, you were trying to reallocate RAM that was not allocated in
    // a way that is compatible with realloc. In other words, you have a allocation/reallocation
    // mismatch.
    EE_MEMASSERT(pkUnit->m_eAllocType == EE_MET_MALLOC ||
        pkUnit->m_eAllocType == EE_MET_REALLOC ||
        pkUnit->m_eAllocType == EE_MET_ALIGNEDMALLOC ||
        pkUnit->m_eAllocType == EE_MET_ALIGNEDREALLOC);

    // If you hit this NIMEMASSERT, you performed a reallocation with a hint mismatch.  The hint
    // passed into reallocate should match the hint passed in when it was originally allocated.
    EE_MEMASSERT(pkUnit->m_kAllocHint.GetRaw() == kHint.GetRaw());

    // Update allocation unit.
    MemEventType eDeallocType = eEventType;
    double fDeallocTime = fTime;

    if (m_bWriteToLog && m_pkMemLogHandler && m_pkMemLogHandler->IsInitialized())
    {
        // Write out the freed memory to the memory log.
        m_kCriticalSection.Lock();
        m_pkMemLogHandler->LogAllocUnit(
            pkUnit,
            "\t",
            eDeallocType,
            fDeallocTime,
            ulFreeThreadId,
            stSizeUnused);
        m_kCriticalSection.Unlock();
    }

    // Remove this allocation unit from the hash table.
    RemoveAllocUnit(pkUnit);
    --m_stActiveAllocationCount;

    if (m_bAlwaysValidateAll)
        EE_MEMASSERT(ValidateAllAllocUnits());

    // Recycle the allocation unit.  Add it to the front of the reservoir.
    pkUnit->m_kFLF = FLF::UNKNOWN;
    pkUnit->m_pkNext = m_pkReservoir;
    m_pkReservoir = pkUnit;
    pkUnit = NULL;

    // Grow the tracking unit reservoir if necessary.
    if (!m_pkReservoir)
        GrowReservoir();

    // If you hit this EE_MEMASSERT, the free store for allocation units does not exist. This
    // circumstance should only happen if the reservoir needed to grow and was unable to satisfy
    // the request.  In other words, you may be out of memory.
    EE_MEMASSERT (m_pkReservoir != NULL);

    // Get an allocation unit from the reservoir.
    AllocUnit* pkNewUnit = m_pkReservoir;
    m_pkReservoir = pkNewUnit->m_pkNext;

    // Fill in the known information.
    pkNewUnit->Reset();
    pkNewUnit->m_stAllocationID = m_stCurrentAllocID;
    pkNewUnit->m_stAlignment = stAlignment;
    pkNewUnit->m_ulAllocThreadId = GetCurrentThreadId();
    pkNewUnit->m_eAllocType = eEventType;
    pkNewUnit->m_kAllocHint = kHint;
    pkNewUnit->m_fAllocTime = (float)fTime;
    pkNewUnit->m_pvMem = pvNewMemory;
    pkNewUnit->m_stSizeAllocated = stSizeInBytes;
    pkNewUnit->m_stSizeRequested = stSizeOriginal;
    pkNewUnit->m_kFLF.Set(pcFile, iLine, pcFunction);

#if defined(EE_MEMTRACKER_STACKTRACE)
    // Grab a stack trace skipping the top two frames (ourself and our caller) which we know are
    // uninteresting allocation related methods.
    pkNewUnit->m_stackSize =
        FastStackTrace(pkNewUnit->m_stack, EE_ARRAYSIZEOF(pkNewUnit->m_stack), 2);
#endif

    // If you hit this EE_MEMASSERT, then this allocation was made from a source that isn't set up
    // to use this memory tracking software; use the stack frame to locate the source and include
    // MemManager.h.
    EE_MEMASSERT(eEventType != EE_MET_UNKNOWN);

    // Insert the new allocation into the hash table.
    InsertAllocUnit(pkNewUnit);
    ++m_stActiveAllocationCount;
    ++m_stCurrentAllocID;

    if (m_bAlwaysValidateAll)
        EE_MEMASSERT(ValidateAllAllocUnits());

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
    }
    catch(const char *err)
    {
    }
#endif
    m_kCriticalSection.Unlock();

    return pvNewMemory;
}

//------------------------------------------------------------------------------------------------
void MemTracker::Deallocate(void* pvMemory, MemEventType eEventType, size_t stSizeInBytes)
{
    if (pvMemory)
    {
        m_kCriticalSection.Lock();

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
        try {
#endif
        double fTime = efd::GetCurrentTimeInSec();

        // Search the tracking unit hash table to find the address.
        AllocUnit* pkUnit = FindAllocUnit(pvMemory);

        if (pkUnit == NULL)
        {
            // If you hit this EE_MEMASSERT, you tried to deallocate RAM that wasn't allocated by
            // this memory manager.  This symptom may also be indicative of a double deletion.
            EE_MEMASSERT(pkUnit != NULL);
#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
            throw "Request to deallocate RAM that was never allocated";
#endif
            m_kCriticalSection.Unlock();

            return;
        }

        // If you hit this EE_MEMASSERT, you are allocating before EE_INIT was called, or after
        // EE_SHUTDOWN was called.  The ability to allocate through Gamebryo outside of pre-main
        // or post-main is deprecated and will be removed in a future release.
        EE_MEMASSERT("Warning: Using the Gamebryo allocator outside of EE_INIT/EE_SHUTDOWN "
            "is deprecated." && (CanAllocateNow() ||
             ((pkUnit->m_stAllocationID >= m_stIgnoreRangeStart) &&
             (pkUnit->m_stAllocationID < m_stIgnoreRangeEnd))));

        // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation ID.
        EE_MEMASSERT_UNIT(ms_stBreakOnAllocID == static_cast<size_t>(-1) ||
            ms_stBreakOnAllocID != pkUnit->m_stAllocationID, pkUnit);

        // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation
        // request size.
        EE_MEMASSERT_UNIT(ms_stBreakOnSizeRequested != pkUnit->m_stSizeRequested, pkUnit);

        // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation
        // address range.
        EE_MEMASSERT_UNIT(pkUnit->m_pvMem < ms_pvBreakOnAllocRangeStart ||
            pkUnit->m_pvMem > ms_pvBreakOnAllocRangeEnd, pkUnit);

        // If you hit this EE_MEMASSERT, then the allocation unit that is about to be deleted
        // requested an initial size that doesn't match what is currently the 'size' argument for
        // deallocation.  This circumstance is most commonly caused by the lack of a virtual
        // destructor for a class that is used polymorphically in an array.
        if (stSizeInBytes != EE_MEM_DEALLOC_SIZE_DEFAULT)
        {
            EE_MEMASSERT_UNIT(stSizeInBytes == pkUnit->m_stSizeRequested, pkUnit);
        }

        // If you hit this EE_MEMASSERT, then the allocation unit that is about to be deallocated
        // is damaged.
        bool bValidateAllocUnit = ValidateAllocUnit(pkUnit);
        EE_UNUSED_ARG(bValidateAllocUnit);
        EE_MEMASSERT_UNIT(bValidateAllocUnit, pkUnit);

        // If you hit this EE_MEMASSERT, then this deallocation was made from a source that isn't
        // set up to use this memory tracking software; use the stack frame to locate the source
        // and include MemManager.h.
        EE_MEMASSERT_UNIT(eEventType != EE_MET_UNKNOWN, pkUnit);

        // If you hit this EE_MEMASSERT, you were trying to deallocate RAM that was not allocated
        // in a way that is compatible with the deallocation method requested.  In other words,
        // you have an allocation/deallocation mismatch.
        EE_MEMASSERT_UNIT((eEventType == EE_MET_DELETE && pkUnit->m_eAllocType == EE_MET_NEW) ||
            (eEventType == EE_MET_DELETE_ARRAY && pkUnit->m_eAllocType == EE_MET_NEW_ARRAY) ||
            (eEventType == EE_MET_FREE && pkUnit->m_eAllocType == EE_MET_MALLOC) ||
            (eEventType == EE_MET_FREE && pkUnit->m_eAllocType == EE_MET_REALLOC) ||
            (eEventType == EE_MET_REALLOC && pkUnit->m_eAllocType == EE_MET_MALLOC) ||
            (eEventType == EE_MET_REALLOC && pkUnit->m_eAllocType == EE_MET_REALLOC) ||
            (eEventType == EE_MET_ALIGNEDFREE && pkUnit->m_eAllocType == EE_MET_ALIGNEDMALLOC) ||
            (eEventType == EE_MET_ALIGNEDFREE && pkUnit->m_eAllocType == EE_MET_ALIGNEDREALLOC) ||
            (eEventType == EE_MET_ALIGNEDREALLOC && pkUnit->m_eAllocType == EE_MET_ALIGNEDMALLOC) ||
            (eEventType == EE_MET_ALIGNEDREALLOC && pkUnit->m_eAllocType == EE_MET_ALIGNEDREALLOC)
            ||
            (eEventType == EE_MET_UNKNOWN), pkUnit);

        // Update allocation unit.
        MemEventType eDeallocType = eEventType;
        double fDeallocTime = fTime;

        // Determine how much memory was actually set.
        size_t stSizeUnused = MemoryBytesWithPattern(pvMemory, pkUnit->m_stSizeRequested);
        m_stUnusedButAllocatedMemory += stSizeUnused;

        // Save the thread id that freed the memory.
        unsigned long ulFreeThreadId = GetCurrentThreadId();

        if (m_bCheckArrayOverruns)
        {
            // If you hit this EE_MEMASSERT, you have code that overwrites either before or after
            // the range of an allocation.  Check the pkUnit for information about which
            // allocation is being overwritten.
            bool bCheckForArrayOverrun =
                CheckForArrayOverrun(pvMemory, pkUnit->m_stAlignment, pkUnit->m_stSizeRequested);
            EE_UNUSED_ARG(bCheckForArrayOverrun);
            EE_MEMASSERT_UNIT(!bCheckForArrayOverrun, pkUnit);

            if (stSizeInBytes!= EE_MEM_DEALLOC_SIZE_DEFAULT)
                stSizeInBytes = PadForArrayOverrun(pkUnit->m_stAlignment, stSizeInBytes);
        }

        // Perform the actual deallocation.
        m_pkActualAllocator->Deallocate(pvMemory, eEventType, stSizeInBytes);

        // Remove this allocation unit from the hash table.
        RemoveAllocUnit(pkUnit);

        // Update summary statistics.
        --m_stActiveAllocationCount;
        m_stActiveMemory -= pkUnit->m_stSizeAllocated;

        // Validate every single allocated unit in memory.
        if (m_bAlwaysValidateAll)
        {
            bool bValidateAllAllocUnits = ValidateAllAllocUnits();
            EE_UNUSED_ARG(bValidateAllAllocUnits);
            EE_MEMASSERT_UNIT(bValidateAllAllocUnits, pkUnit);
        }

        EE_UNUSED_ARG(fDeallocTime);
        EE_UNUSED_ARG(ulFreeThreadId);
        EE_UNUSED_ARG(eDeallocType);

        EE_PROFILER_MEM_FREE(Memory, pkUnit->m_pvMem);

        // TODO  mbailey  8/22/08  Temporarily disable memory logging for all stl allocations.
        // Currently, the xml memory log files produced by test apps can exceed 450 Mb, due to the
        // abundant amount of memory allocations performed by stl.  Disabling memory logging for
        // stl brings this size down to ~65 Mb.
        if (!efd::gs_bDisableMemLogging && m_bWriteToLog && m_pkMemLogHandler &&
            m_pkMemLogHandler->IsInitialized())
        {
            // Write out the freed memory to the memory log.
            m_kCriticalSection.Lock();
            m_pkMemLogHandler->LogAllocUnit(pkUnit, "\t", eDeallocType, fDeallocTime,
                ulFreeThreadId, stSizeUnused);
            m_kCriticalSection.Unlock();
        }

        // Recycle the allocation unit.  Add it to the front of the reservoir.
        pkUnit->m_kFLF = FLF::UNKNOWN;
        pkUnit->m_pkNext = m_pkReservoir;
        m_pkReservoir = pkUnit;

        // Validate every single allocated unit in memory.
        if (m_bAlwaysValidateAll)
        {
            bool bValidateAllAllocUnits = ValidateAllAllocUnits();
            EE_UNUSED_ARG(bValidateAllAllocUnits);
            EE_MEMASSERT_UNIT(bValidateAllAllocUnits, pkUnit);
        }
        m_kCriticalSection.Unlock();

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
    }
    catch(const char *err)
    {
        // Deal with the errors.
    }
#endif
    }
}

//------------------------------------------------------------------------------------------------
bool MemTracker::TrackAllocate(
    const void* const pvMemory,
    size_t stSizeInBytes,
    MemHint kHint,
    MemEventType eEventType,
    const char* pcFile,
    int iLine,
    const char* pcFunction)
{
    m_kCriticalSection.Lock();

    size_t stSizeOriginal = stSizeInBytes;
    double fTime = efd::GetCurrentTimeInSec();

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific function name.
    EE_MEMASSERT(strcmp(ms_pcBreakOnFunctionName, pcFunction) != 0);

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation ID.
    EE_MEMASSERT(ms_stBreakOnAllocID == static_cast<size_t>(-1) ||
        ms_stBreakOnAllocID != m_stCurrentAllocID);

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation request
    // size.
    EE_MEMASSERT(ms_stBreakOnSizeRequested != stSizeOriginal);

    // Actually perform the allocation. Note that size and alignment may be adjusted by the call.
    m_pkActualAllocator->TrackAllocate(pvMemory, stSizeInBytes,
        kHint, eEventType, pcFile, iLine, pcFunction);

    // update overall summary statistics
    m_stActiveAllocationCount++;
    m_stAccumulatedAllocationCount++;
    if (m_stActiveAllocationCount > m_stPeakAllocationCount)
    {
        m_stPeakAllocationCount = m_stActiveAllocationCount;
        m_fPeakAllocationCountTime = fTime;
    }

    m_stActiveMemory += stSizeInBytes;
    m_stAccumulatedMemory += stSizeInBytes;
    if (m_stActiveMemory > m_stPeakMemory)
    {
        m_stPeakMemory = m_stActiveMemory;
        m_fPeakMemoryTime = fTime;
    }

    // update external-specific summary statistics
    m_stActiveExternalAllocationCount++;
    m_stAccumulatedExternalAllocationCount++;
    if (m_stActiveExternalAllocationCount > m_stPeakExternalAllocationCount)
    {
        m_stPeakExternalAllocationCount = m_stActiveExternalAllocationCount;
    }

    m_stActiveExternalMemory += stSizeInBytes;
    m_stAccumulatedExternalMemory += stSizeInBytes;
    if (m_stActiveExternalMemory > m_stPeakExternalMemory)
    {
        m_stPeakExternalMemory = m_stActiveExternalMemory;
    }

    if (pvMemory == NULL)
    {
        // If you hit this, your memory request was not satisfied
        m_kCriticalSection.Unlock();
        return true;
    }

    // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation address
    // range.
    EE_MEMASSERT(!IsInsideBreakRange(pvMemory, stSizeInBytes));

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
    try
    {
#endif

    // If you hit this EE_MEMASSERT, the somehow you have allocated a memory unit to an address
    // that already exists. This should never happen and is an indicator that something has gone
    // wrong in the sub-allocator.
    EE_MEMASSERT(FindAllocUnit(pvMemory) == NULL);

    // Grow the tracking unit reservoir if necessary
    if (!m_pkReservoir)
        GrowReservoir();

    // If you hit this EE_MEMASSERT, the free store for allocation units does not exist. This
    // should only happen if the reservoir needed to grow and was unable to satisfy the request.
    // In other words, you may be out of memory.
    EE_MEMASSERT (m_pkReservoir != NULL);

    // Get an allocation unit from the reservoir
    AllocUnit* pkUnit = m_pkReservoir;
    m_pkReservoir = pkUnit->m_pkNext;

    // fill in the known information
    pkUnit->Reset();
    pkUnit->m_stAllocationID = m_stCurrentAllocID;
    pkUnit->m_stAlignment = 0;
    pkUnit->m_ulAllocThreadId = GetCurrentThreadId();
    pkUnit->m_eAllocType = eEventType;
    pkUnit->m_kAllocHint = kHint;
    pkUnit->m_fAllocTime = (float)fTime;
    pkUnit->m_pvMem = (void*) pvMemory;
    pkUnit->m_stSizeRequested = stSizeOriginal;
    pkUnit->m_stSizeAllocated = stSizeInBytes;
    pkUnit->m_kFLF.Set(pcFile, iLine, pcFunction);
#if defined(EE_MEMTRACKER_STACKTRACE)
    // Grab a stack trace skipping the top two frames (ourself and our caller) which we know
    // are uninteresting allocation related methods.
    pkUnit->m_stackSize = FastStackTrace(pkUnit->m_stack, EE_ARRAYSIZEOF(pkUnit->m_stack), 2);
#endif

    // If you hit this EE_MEMASSERT, then this allocation was made from a source that isn't setup
    // to use this memory tracking software, use the stack frame to locate the source and include
    // MemManager.h.
    EE_MEMASSERT_UNIT(eEventType != EE_MET_UNKNOWN, pkUnit);

    // Insert the new allocation into the hash table
    InsertAllocUnit(pkUnit);

    // Validate every single allocated unit in memory
    if (m_bAlwaysValidateAll)
    {
        bool bValidateAllAllocUnits = ValidateAllAllocUnits();
        EE_UNUSED_ARG(bValidateAllAllocUnits);
        EE_MEMASSERT_UNIT(bValidateAllAllocUnits, pkUnit);
    }

#if defined(EE_MEMORY_ENABLE_EXCEPTIONS)
    }
    catch(const char *err)
    {
        // Deal with the errors
        // Deal with the errors
    }
#endif

    ++m_stCurrentAllocID;
    m_kCriticalSection.Unlock();

    return true;
}

//------------------------------------------------------------------------------------------------
bool MemTracker::TrackDeallocate(const void* const pvMemory, MemEventType eEventType)
{
    if (pvMemory)
    {
        m_kCriticalSection.Lock();

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
        try {
#endif
        double fTime = efd::GetCurrentTimeInSec();

        // Search the tracking unit hash table to find the address.
        AllocUnit* pkUnit = FindAllocUnit(pvMemory);

        if (pkUnit == NULL)
        {
            // If you hit this EE_MEMASSERT, you tried to deallocate RAM that wasn't allocated by
            // this memory manager.  This symptom may also be indicative of a double deletion.
            // Please check the pkUnit FLF for information about the allocation.
            EE_MEMASSERT(pkUnit != NULL);
#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
            throw "Request to deallocate RAM that was never allocated";
#endif
            m_kCriticalSection.Unlock();

            return true;
        }

        // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation ID.
        EE_MEMASSERT_UNIT(ms_stBreakOnAllocID == static_cast<size_t>(-1) ||
            ms_stBreakOnAllocID != pkUnit->m_stAllocationID, pkUnit);

        // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation
        // request size.
        EE_MEMASSERT_UNIT(ms_stBreakOnSizeRequested != pkUnit->m_stSizeRequested, pkUnit);

        // If you hit this EE_MEMASSERT, you requested a breakpoint on a specific allocation
        // address range.
        EE_MEMASSERT_UNIT(!IsInsideBreakRange(pkUnit->m_pvMem, pkUnit->m_stSizeRequested),
            pkUnit);

        // If you hit this EE_MEMASSERT, then the allocation unit that is about to be deallocated
        // is damaged.
        bool bValidateAllocUnit = ValidateAllocUnit(pkUnit);
        EE_UNUSED_ARG(bValidateAllocUnit);
        EE_MEMASSERT_UNIT(bValidateAllocUnit, pkUnit);

        // If you hit this EE_MEMASSERT, then this deallocation was made from a source that isn't
        // set up to use this memory tracking software; use the stack frame to locate the source
        // and include MemManager.h
        EE_MEMASSERT_UNIT(eEventType == EE_MET_EXTERNALFREE, pkUnit);

        // If you hit this EE_MEMASSERT, you were trying to deallocate RAM that was not allocated
        // in a way that is compatible with the deallocation method requested.  In other words,
        // you have an allocation/deallocation mismatch.
        EE_MEMASSERT_UNIT(pkUnit->m_eAllocType == EE_MET_EXTERNALALLOC, pkUnit);

        // Update allocation unit.
        MemEventType eDeallocType = eEventType;
        double fDeallocTime = fTime;

        // Because we don't touch the tracked external address, simply state the unused amount is
        // zero.
        size_t stSizeUnused = 0;

        // Save the thread id that freed the memory.
        unsigned long ulFreeThreadId = GetCurrentThreadId();

        // Perform the actual deallocation.
        m_pkActualAllocator->TrackDeallocate(pvMemory, eEventType);

        // Remove this allocation unit from the hash table.
        RemoveAllocUnit(pkUnit);

        // Update overall summary statistics.
        --m_stActiveAllocationCount;
        m_stActiveMemory -= pkUnit->m_stSizeAllocated;

        // Update external-specific summary statistics.
        --m_stActiveExternalAllocationCount;
        m_stActiveExternalMemory -= pkUnit->m_stSizeAllocated;

        // Validate every single allocated unit in memory.
        if (m_bAlwaysValidateAll)
        {
            bool bValidateAllAllocUnits = ValidateAllAllocUnits();
            EE_UNUSED_ARG(bValidateAllAllocUnits);
            EE_MEMASSERT_UNIT(bValidateAllAllocUnits, pkUnit);
        }

        if (m_bWriteToLog && m_pkMemLogHandler && m_pkMemLogHandler->IsInitialized())
        {
            // Write out the freed memory to the memory log.
            m_kCriticalSection.Lock();
            m_pkMemLogHandler->LogAllocUnit(
                pkUnit,
                "\t",
                eDeallocType,
                fDeallocTime,
                ulFreeThreadId,
                stSizeUnused);
            m_kCriticalSection.Unlock();
        }

        // Recycle the allocation unit.  Add it to the front of the reservoir.
        pkUnit->m_kFLF = FLF::UNKNOWN;
        pkUnit->m_pkNext = m_pkReservoir;
        m_pkReservoir = pkUnit;

        // Validate every single allocated unit in memory.
        if (m_bAlwaysValidateAll)
        {
            bool bValidateAllAllocUnits = ValidateAllAllocUnits();
            EE_UNUSED_ARG(bValidateAllAllocUnits);
            EE_MEMASSERT_UNIT(bValidateAllAllocUnits, pkUnit);
        }
        m_kCriticalSection.Unlock();

#ifdef EE_MEMORY_ENABLE_EXCEPTIONS
    }
    catch(const char *err)
    {
        // Deal with the errors.
    }
#endif
    }
    return true;
}

//------------------------------------------------------------------------------------------------
bool MemTracker::SetMarker(const char* pcMarkerType, const char* pcClassifier, const char* pcString)
{
    if (!m_pkMemLogHandler || !m_pkMemLogHandler->IsInitialized())
        return false;

    // Lock the critical section to keep from inserting bogus markers into the stream
    m_kCriticalSection.Lock();
    bool bRet = m_pkMemLogHandler->SetMarker(pcMarkerType, pcClassifier, pcString);
    m_kCriticalSection.Unlock();

    return bRet;
}

//------------------------------------------------------------------------------------------------
AllocUnit* MemTracker::FindAllocUnit(const void* pvMem) const
{
    // Just in case...
    EE_MEMASSERT(pvMem != NULL);

    // Use the address to locate the hash index. Note that we shift off the
    // lower four bits. This is because most allocated addresses will be on
    // four-, eight- or even sixteen-byte boundaries. If we didn't do this,
    // the hash index would not have very good coverage.

    unsigned int uiHashIndex = AddressToHashIndex(pvMem);

    AllocUnit* pkUnit = m_pkActiveMem[uiHashIndex];
    while (pkUnit)
    {
        if (pkUnit->m_pvMem == pvMem)
            return pkUnit;

        pkUnit = pkUnit->m_pkNext;
    }

    return NULL;
}

//------------------------------------------------------------------------------------------------
#if defined(EE_MEMTRACKER_DETAILEDREPORTING)
void MemTracker::SetDetailReportForAllocation(const void* pvMem, AllocUnit::pfnDetailMethod pfn)
{
    AllocUnit* pUnit = FindAllocUnit(pvMem);
    if (pUnit)
    {
        pUnit->m_pfnDetails = pfn;
    }
}
#endif

//------------------------------------------------------------------------------------------------
void MemTracker::ReportActiveAllocations(efd::MemTracker::ReportActiveAllocUnit pfnReportCallBack)
{
    if (!m_pkMemLogHandler || !m_pkMemLogHandler->IsInitialized())
        return;

    m_kCriticalSection.Lock();

    for (unsigned int uiHashIndex = 0; uiHashIndex < ms_uiHashSize; uiHashIndex++)
    {
        AllocUnit* pkUnit = m_pkActiveMem[uiHashIndex];
        while (pkUnit)
        {
            // Ignore the alloc unit if it's ID is inside the ignore range. Allocs in the ignore
            // range were allocated by the IMemHandler and will be deleted after all reporting
            // is completed.
            if (pkUnit->m_stAllocationID < m_stIgnoreRangeStart ||
                pkUnit->m_stAllocationID >= m_stIgnoreRangeEnd)
            {
                pfnReportCallBack(pkUnit, m_pkMemLogHandler);
            }

            // continue to the next unit
            pkUnit = pkUnit->m_pkNext;
        }
    }

    m_kCriticalSection.Unlock();
}

//------------------------------------------------------------------------------------------------
bool MemTracker::VerifyAddress(const void* pvMemory)
{
    m_kCriticalSection.Lock();

    // Handle both regular pointers and array offset values.
    // from StandardAllocator.
    AllocUnit* pkUnit = FindAllocUnit(pvMemory);
    if (pkUnit)
    {
        if (m_bCheckArrayOverruns && (pkUnit->m_eAllocType != EE_MET_EXTERNALALLOC))
        {
            void* pvMemStore = pkUnit->m_pvMem;
            if (CheckForArrayOverrun(pvMemStore,
               pkUnit->m_stAlignment, pkUnit->m_stSizeRequested))
            {
                if (m_bAlwaysValidateAll == false)
                {
                    EE_LOG(efd::kMemTracker, ILogger::kERR0,
                        ("\nMemory Overrun Found At Allocation:"));
                    OutputAllocUnitToLog(pkUnit);
                }
                else
                {
                    // The logger has the potential to request additional memory, in which case
                    // if we have m_bAlwaysValidateAll enabled we'll end up in an endless loop
                    // and blow out the stack. Instead, write them out to the debug output.
                    EE_OUTPUT_DEBUG_STRING("\nMemory Overrun Found At Allocation:");
                    OutputAllocUnitToLog(pkUnit, true);
                }

                m_kCriticalSection.Unlock();
                return false;
            }
        }


        m_kCriticalSection.Unlock();
        return true;
    }

    // The Xbox 360 inserts an alignment-sized header into the allocation
    // when a destructor needs to be called. This value holds the count of
    // elements that were allocated. However, it will be unknown to this
    // function if such a header were actually added since all we have is
    // an address. Therefore, we must search backwards to some reasonable
    // degree of certainty to find the matching allocation unit.

    // 4 is the minimum offset/alignment for all supported compilers.
    size_t stOffset = 4;

    // The maximum offset that is supported by this function is 128.
    // Anything larger is probably unreasonable on modern systems.
    while (stOffset <= 128)
    {
        unsigned char* pcAdjustedMemory = ((unsigned char*)pvMemory) -
            stOffset;
        pkUnit = FindAllocUnit(pcAdjustedMemory);

        // We must be careful that the allocation unit exists and is big enough to actually
        // encompass the address and isn't actually right in front of it.
        if (pkUnit && (pcAdjustedMemory + pkUnit->m_stSizeRequested) >= (unsigned char*)pvMemory)
        {
            if (m_bCheckArrayOverruns && (pkUnit->m_eAllocType != EE_MET_EXTERNALALLOC))
            {
                void* pvMemStore = pkUnit->m_pvMem;
                if (CheckForArrayOverrun(pvMemStore,
                   pkUnit->m_stAlignment, pkUnit->m_stSizeRequested))
                {
                    if (m_bAlwaysValidateAll == false)
                    {
                        EE_LOG(efd::kMemTracker, ILogger::kERR0,
                            ("\nMemory Overrun Found At Allocation:"));
                        OutputAllocUnitToLog(pkUnit);
                    }
                    else
                    {
                        // The logger has the potential to request additional memory, in which case
                        // if we have m_bAlwaysValidateAll enabled we'll end up in an endless loop
                        // and blow out the stack. Instead, write them out to the debug output.
                        EE_OUTPUT_DEBUG_STRING("\nMemory Overrun Found At Allocation:");
                        OutputAllocUnitToLog(pkUnit, true);
                    }

                    m_kCriticalSection.Unlock();
                    return false;
                }

            }
            m_kCriticalSection.Unlock();
            return true;
        }
        // If we've found an allocation unit that exists before the
        // address we are looking up, there is no need to keep searching,
        // this address is unknown to us.
        else if (pkUnit)
        {
            m_kCriticalSection.Unlock();
            return false;
        }

        // Find the next alignment that is permissible.
        stOffset *= 2;
    }

    m_kCriticalSection.Unlock();
    return false;
}

//------------------------------------------------------------------------------------------------
void* MemTracker::FindContainingAllocation(const void* pvMemory)
{
    m_kCriticalSection.Lock();

    for (unsigned int uiHash = 0; uiHash < ms_uiHashSize; uiHash++)
    {
        AllocUnit* pkUnit = m_pkActiveMem[uiHash];
        while (pkUnit)
        {
            void* pvBottom = pkUnit->m_pvMem;
            void* pvTop = (void*)(
                ((size_t)pvBottom) + pkUnit->m_stSizeAllocated);
            if (pvBottom <= pvMemory && pvMemory < pvTop)
            {
                void* pvResult = pkUnit->m_pvMem;
                m_kCriticalSection.Unlock();
                return pvResult;
            }

            pkUnit = pkUnit->m_pkNext;
        }
    }

    m_kCriticalSection.Unlock();
    return NULL;
}

//------------------------------------------------------------------------------------------------
void MemTracker::MemoryFillForArrayOverrun(void*& pvMemory,
    size_t stAlignment, size_t stSizeOriginal)
{
    char* pcMemArray = (char*) pvMemory;
    pvMemory = pcMemArray + stAlignment;
    MemoryFillWithPattern(pcMemArray, stAlignment);

    pcMemArray = pcMemArray + stAlignment + stSizeOriginal;
    MemoryFillWithPattern(pcMemArray, stAlignment);
}

//------------------------------------------------------------------------------------------------
void MemTracker::MemoryFillWithPattern(void* pvMemory,
    size_t stSizeInBytes)
{
    unsigned char* pcMemArray = (unsigned char*) pvMemory;
    for (unsigned int ui = 0; ui < stSizeInBytes; ui++)
    {
        pcMemArray[ui] = m_ucFillChar;
    }
}

//------------------------------------------------------------------------------------------------
size_t MemTracker::MemoryBytesWithPattern(void* pvMemory,
    size_t stSizeInBytes) const
{
    unsigned char* pcMemArray = (unsigned char*) pvMemory;
    size_t numBytes = 0;
    for (unsigned int ui = 0; ui < stSizeInBytes; ui++)
    {
        if (pcMemArray[ui] == m_ucFillChar)
        {
            numBytes++;
        }
    }

    return numBytes;
}

//------------------------------------------------------------------------------------------------
bool MemTracker::CheckForArrayOverrun(void*& pvMemory,
    size_t stAlignment, size_t stSizeOriginal) const
{
    EE_MEMASSERT(m_bCheckArrayOverruns);

    char* pcMemArray = (char*) pvMemory;
    pcMemArray -= stAlignment;
    pvMemory = pcMemArray;

    if (stAlignment != MemoryBytesWithPattern(pcMemArray, stAlignment))
        return true;

    pcMemArray = pcMemArray + stAlignment + stSizeOriginal;
    if (stAlignment != MemoryBytesWithPattern(pcMemArray, stAlignment))
        return true;

    return false;
}

//------------------------------------------------------------------------------------------------
size_t MemTracker::PadForArrayOverrun(
    size_t stAlignment,
    size_t stSizeOriginal)
{
    return stSizeOriginal + 2 * stAlignment;
}

//------------------------------------------------------------------------------------------------
bool MemTracker::ValidateAllocUnit(const AllocUnit* pkUnit) const
{
    if (pkUnit->m_stAllocationID > m_stCurrentAllocID)
        return false;

    if (pkUnit->m_stSizeAllocated < pkUnit->m_stSizeRequested)
        return false;

    if (pkUnit->m_stSizeAllocated == 0 ||  pkUnit->m_stSizeRequested == 0)
        return false;

    if (pkUnit->m_pvMem == NULL)
        return false;

    if (pkUnit->m_pkNext != NULL && pkUnit->m_pkNext->m_pkPrev != pkUnit)
        return false;

    return true;
}

//------------------------------------------------------------------------------------------------
bool MemTracker::ValidateAllAllocUnits() const
{
    unsigned int uiActiveCount = 0;
    for (unsigned int uiHashIndex = 0; uiHashIndex < ms_uiHashSize;
        uiHashIndex++)
    {
        AllocUnit* pkUnit = m_pkActiveMem[uiHashIndex];
        AllocUnit* pkPrev = NULL;

        while (pkUnit)
        {
            if (!ValidateAllocUnit(pkUnit))
                return false;

            if (pkUnit->m_pkPrev != pkPrev)
                return false;

            if (m_bCheckArrayOverruns)
            {
                void* pvMemStore = pkUnit->m_pvMem;
                if (CheckForArrayOverrun(pvMemStore, pkUnit->m_stAlignment,
                    pkUnit->m_stSizeRequested))
                {
                    if (m_bAlwaysValidateAll == false)
                    {
                        EE_LOG(efd::kMemTracker, ILogger::kERR0,
                            ("\nMemory Overrun Found At Allocation:"));

                        OutputAllocUnitToLog(pkUnit);
                    }
                    else
                    {
                        // The logger has the potential to request additional memory, in which case
                        // if we have m_bAlwaysValidateAll enabled we'll end up in an endless loop
                        // and blow out the stack. Instead, write them out to the debug output.
                        EE_OUTPUT_DEBUG_STRING("\nMemory Overrun Found At Allocation:");
                        OutputAllocUnitToLog(pkUnit, true);
                    }
                    return false;
                }
            }

            // continue to the next unit
            pkPrev = pkUnit;
            pkUnit = pkUnit->m_pkNext;
            uiActiveCount++;
        }
    }

    if (uiActiveCount != this->m_stActiveAllocationCount)
        return false;

    return true;
}

//------------------------------------------------------------------------------------------------
const char* MemTracker::FormatForXML(const char* pcInString)
{
    static char acOutString[1024];
    EE_MEMASSERT(pcInString != NULL);
    size_t stLen = strlen(pcInString);

    // If this assert is hit, then the length of the input string is longer
    // than the sanitization buffer (1024 characters including the NULL).
    EE_MEMASSERT(stLen < 1024);
    unsigned int uiIdx = 0;
    for (size_t stSrcIdx = 0; (stSrcIdx < stLen) && (uiIdx < 1023); stSrcIdx++)
    {
        acOutString[uiIdx] = pcInString[stSrcIdx];

        if (acOutString[uiIdx] == '<')
        {
            acOutString[uiIdx++] = '&';
            acOutString[uiIdx++] = 'l';
            acOutString[uiIdx++] = 't';
            acOutString[uiIdx++] = ';';
        }
        else if (acOutString[uiIdx] == '>')
        {
            acOutString[uiIdx++] = '&';
            acOutString[uiIdx++] = 'g';
            acOutString[uiIdx++] = 't';
            acOutString[uiIdx++] = ';';
        }
        else if (acOutString[uiIdx] == '&')
        {
            acOutString[uiIdx++] = '&';
            acOutString[uiIdx++] = 'a';
            acOutString[uiIdx++] = 'm';
            acOutString[uiIdx++] = 'p';
            acOutString[uiIdx++] = ';';
        }
        else if (acOutString[uiIdx] == '`')
        {
            acOutString[uiIdx++] = ' ';
        }
        else if (acOutString[uiIdx] == '\'')
        {
            acOutString[uiIdx++] = ' ';
        }
        else
        {
            uiIdx++;
        }
    }
    // If this assert is hit, then the XML-sanitized version of the input
    // was at least 1023 characters and was probably clipped at 1023.
    EE_MEMASSERT(uiIdx < 1023);

    acOutString[uiIdx] = '\0';
    return acOutString;
}

//------------------------------------------------------------------------------------------------
void MemTracker::OutputLeakedMemoryToDebugStream(bool bUseLog) const
{
    if (!ms_bOutputLeaksToDebugStream)
        return;

    if (bUseLog)
    {
        EE_LOG(efd::kMemTracker, ILogger::kERR0, ("\nLeaked Memory Report:"));
    }
#if defined(EE_EFD_CONFIG_DEBUG)
    else
    {
        EE_OUTPUT_DEBUG_STRING("\nLeaked Memory Report:\n");
    }
#endif

    for (unsigned int uiHashIndex = 0; uiHashIndex < ms_uiHashSize; uiHashIndex++)
    {
        AllocUnit* pkUnit = m_pkActiveMem[uiHashIndex];
        while (pkUnit)
        {
            if (bUseLog)
                OutputAllocUnitToLog(pkUnit);
            else
                OutputAllocUnitToDebugStream(pkUnit);

            // continue to the next unit
            pkUnit = pkUnit->m_pkNext;
        }
    }
}

//------------------------------------------------------------------------------------------------
void MemTracker::OutputActiveToDebugStream(const char* pcStringId,
    size_t stMinAllocID) const
{
    EE_UNUSED_ARG(pcStringId);
    efd::Char acBuff[1024];
    ms_pkTracker->m_kCriticalSection.Lock();

    efd::Sprintf(acBuff, 1024, "\nActive Memory Report: \"%s\"\n", pcStringId);
    EE_OUTPUT_DEBUG_STRING(acBuff);

    size_t stBaseAllocID = stMinAllocID;
    efd::Sprintf(acBuff, 1024, "Base Allocation ID: %d\n", stBaseAllocID);
    EE_OUTPUT_DEBUG_STRING(acBuff);

    size_t stNumAllocs = 0;
    size_t stTotalBytes = 0;

    // Find the min/max and byte count
    for (unsigned int uiHashIndex = 0; uiHashIndex < ms_uiHashSize; uiHashIndex++)
    {
        AllocUnit* pkUnit = m_pkActiveMem[uiHashIndex];
        while (pkUnit)
        {
            if (pkUnit->m_stAllocationID >= stMinAllocID)
            {
                stNumAllocs++;
                stTotalBytes += pkUnit->m_stSizeAllocated;
            }

            // continue to the next unit
            pkUnit = pkUnit->m_pkNext;
        }
    }

    // Iterate over all allocations in the order of their allocation
    for (size_t stCount = 0; stCount < stNumAllocs;)
    {
        AllocUnit* pkClosestUnit = NULL;

        // Find all the units we need to iterate through
        for (unsigned int uiHashIndex = 0; uiHashIndex < ms_uiHashSize;
            uiHashIndex++)
        {
            AllocUnit* pkUnit = m_pkActiveMem[uiHashIndex];
            while (pkUnit)
            {
                if (pkUnit->m_stAllocationID >= stMinAllocID)
                {
                    if (pkClosestUnit == NULL)
                    {
                        pkClosestUnit = pkUnit;
                    }
                    else if (pkUnit->m_stAllocationID <
                        pkClosestUnit->m_stAllocationID)
                    {
                        pkClosestUnit = pkUnit;
                    }
                }
                // continue to the next unit
                pkUnit = pkUnit->m_pkNext;
            }
        }

        if (pkClosestUnit)
        {
            OutputAllocUnitToDebugStream(pkClosestUnit);
            stMinAllocID = pkClosestUnit->m_stAllocationID + 1;
            stCount++;
        }
        else
        {
            break;
        }
    }

    EE_OUTPUT_DEBUG_STRING("\n=============================================================\n");
    efd::Sprintf(acBuff, 1024, "Active Allocation Count (within id range): %d\n", stNumAllocs);
    EE_OUTPUT_DEBUG_STRING(acBuff);
    efd::Sprintf(acBuff, 1024, "Total Bytes: %d\n", stTotalBytes);
    EE_OUTPUT_DEBUG_STRING(acBuff);

    ms_pkTracker->m_kCriticalSection.Unlock();
}

//------------------------------------------------------------------------------------------------
void MemTracker::OutputAllocUnitToDebugStream(const AllocUnit* pkUnit) const
{
    EE_MEMASSERT_UNIT(pkUnit->m_pvMem != 0, pkUnit);

#if defined(EE_EFD_CONFIG_DEBUG)

#if defined(EE_MEMTRACKER_DETAILEDREPORTING)
    char objectDetails[1024];
    pkUnit->GetDetailedReport(objectDetails, EE_ARRAYSIZEOF(objectDetails));
#endif // defined(EE_MEMTRACKER_DETAILEDREPORTING)

#if defined(EE_MEMTRACKER_STACKTRACE)
    char stack[1024];
    ResolveSymbolNames(pkUnit->m_stack, pkUnit->m_stackSize,
                        stack, EE_ARRAYSIZEOF(stack));
#endif // defined(EE_MEMTRACKER_STACKTRACE)

    char acString[1024];

    efd::Sprintf(acString, 1024,
        "%s(%d) : id = %d\tsize = %d\taddr='0x%p'" EE_MTDR_ONLY("\tdetail=%s") "\n"
        EE_MTST_ONLY("Call Stack:\n%s"),
        pkUnit->m_kFLF.m_pcFile,
        pkUnit->m_kFLF.m_uiLine,
        pkUnit->m_stAllocationID,
        pkUnit->m_stSizeAllocated,
        pkUnit->m_pvMem
        EE_MTDR_ONLY_ARG(objectDetails)
        EE_MTST_ONLY_ARG(stack));

    EE_OUTPUT_DEBUG_STRING(acString);
#else
    EE_UNUSED_ARG(pkUnit);
#endif
}

//------------------------------------------------------------------------------------------------
void MemTracker::OutputAllocUnitToLog(const AllocUnit* pkUnit, bool toDebugString) const
{
    EE_MEMASSERT(pkUnit->m_pvMem != 0);

#if !defined(EE_DISABLE_LOGGING)

#if defined(EE_MEMTRACKER_DETAILEDREPORTING)
    char objectDetails[1024];
    pkUnit->GetDetailedReport(objectDetails, EE_ARRAYSIZEOF(objectDetails));
#endif // defined(EE_MEMTRACKER_DETAILEDREPORTING)

#if defined(EE_MEMTRACKER_STACKTRACE)
    char stack[1024];
    ResolveSymbolNames(pkUnit->m_stack, pkUnit->m_stackSize, stack, EE_ARRAYSIZEOF(stack));
#endif // defined(EE_MEMTRACKER_STACKTRACE)

    if (toDebugString)
    {
        char buffer[1024];
        efd::Snprintf(
            buffer,
            1024,
            EE_TRUNCATE,
            "%s(%d) : id = %d\tsize = %d\taddr='0x%p'" EE_MTDR_ONLY("\tdetail=%s")
            EE_MTST_ONLY("\nCall Stack:\n%s"),
            pkUnit->m_kFLF.m_pcFile,
            pkUnit->m_kFLF.m_uiLine,
            pkUnit->m_stAllocationID,
            pkUnit->m_stSizeAllocated,
            pkUnit->m_pvMem
            EE_MTDR_ONLY_ARG(objectDetails)
            EE_MTST_ONLY_ARG(stack));

        EE_OUTPUT_DEBUG_STRING(buffer);
    }
    else
    {
        EE_LOG(efd::kMemTracker, ILogger::kERR0,
            ("%s(%d) : id = %d\tsize = %d\taddr='0x%p'" EE_MTDR_ONLY("\tdetail=%s")
            EE_MTST_ONLY("\nCall Stack:\n%s"),
            pkUnit->m_kFLF.m_pcFile,
            pkUnit->m_kFLF.m_uiLine,
            pkUnit->m_stAllocationID,
            pkUnit->m_stSizeAllocated,
            pkUnit->m_pvMem
            EE_MTDR_ONLY_ARG(objectDetails)
            EE_MTST_ONLY_ARG(stack)
      ));
    }

#else
    EE_UNUSED_ARG(pkUnit);
    EE_UNUSED_ARG(toDebugString);
#endif
}

//------------------------------------------------------------------------------------------------
void MemTracker::OutputMemorySummaryToDebugStream() const
{
    efd::Char acBuff[1024];
    efd::Sprintf(acBuff, 1024,
        "memory_summary timestamp='%f'\n"
        "TotalActiveSize='%d'\n"
        "PeakActiveSize='%d'\n"
        "AccumulatedSize='%d'\n"
        "AllocatedButUnusedSize='%d'\n"
        "ActiveAllocCount='%d'\n"
        "PeakActiveAllocCount='%d'\n"
        "TotalAllocCount='%d'\n"
        "TotalActiveExternalSize='%d'\n"
        "PeakActiveExternalSize='%d'\n"
        "AccumulatedExternalSize='%d'\n"
        "ActiveExternalAllocCount='%d'\n"
        "PeakExternalActiveAllocCount='%d'\n"
        "TotalExternalAllocCount='%d'\n"
        "TotalTrackerOverhead='%d'\n"
        "PeakTrackerOverhead='%d'\n"
        "AccumulatedTrackerOverhead='%d'\n",
        GetCurrentTimeInSec(),
        m_stActiveMemory,
        m_stPeakMemory,
        m_stAccumulatedMemory,
        m_stUnusedButAllocatedMemory,
        m_stActiveAllocationCount,
        m_stPeakAllocationCount,
        m_stAccumulatedAllocationCount,
        m_stActiveExternalMemory,
        m_stPeakExternalMemory,
        m_stAccumulatedExternalMemory,
        m_stActiveExternalAllocationCount,
        m_stPeakExternalAllocationCount,
        m_stAccumulatedExternalAllocationCount,
        m_stActiveTrackerOverhead,
        m_stPeakTrackerOverhead,
        m_stAccumulatedTrackerOverhead);

    EE_OUTPUT_DEBUG_STRING(acBuff);
}

//------------------------------------------------------------------------------------------------
void MemTracker::SetAllocatorAvailableStatus(bool bInsideOfMain)
{
    m_insideOfMain = bInsideOfMain;
}

//------------------------------------------------------------------------------------------------
#if defined(EE_MEMTRACKER_SNAPSHOT)
void MemTracker::BeginSnapshot()
{
    m_bSnapshotActive = true;
}

//------------------------------------------------------------------------------------------------
void MemTracker::EndSnapshot()
{
    m_bSnapshotActive = false;
}

//------------------------------------------------------------------------------------------------
void MemTracker::ClearSnapshot()
{
    for (AllocUnit* pUnit = m_pSnapshotHead; pUnit;)
    {
        AllocUnit* pCurrentUnit = pUnit;
        pUnit = pUnit->m_pShapshotNext;
        pCurrentUnit->m_pShapshotPrev = NULL;
        pCurrentUnit->m_pShapshotNext = NULL;
    }
    m_pSnapshotHead = NULL;
}

//------------------------------------------------------------------------------------------------
void MemTracker::ReportSnapshotAllocations(const char* pszSuffix)
{
    if (m_pkMemLogHandler)
    {
        m_kCriticalSection.Lock();
        m_pkMemLogHandler->BeginSnapshotLog(pszSuffix);

        for (AllocUnit* pUnit = m_pSnapshotHead; pUnit; pUnit = pUnit->m_pShapshotNext)
        {
            m_pkMemLogHandler->LogAllocUnitSnapshot(pUnit);
        }

        m_pkMemLogHandler->EndSnapshotLog();
        m_kCriticalSection.Unlock();
    }
}

//------------------------------------------------------------------------------------------------
#endif
