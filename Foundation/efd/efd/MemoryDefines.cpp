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

// Precompiled Header
#include "efdPCH.h"

#include <efd/MemoryDefines.h>
#include <efd/MemManager.h>
#include <efd/ILogger.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
// _Malloc (see notes above)
//------------------------------------------------------------------------------------------------
#ifdef EE_MEMORY_DEBUGGER
void* efd::_Malloc(size_t stSizeInBytes, MemHint kHint,
    const char* pcSourceFile, int iSourceLine, const char* pcFunction)
#else
void* efd::_Malloc(size_t stSizeInBytes, MemHint kHint)
#endif
{
    EE_ASSERT(MemManager::IsInitialized());

    if (stSizeInBytes == 0)
        stSizeInBytes = 1;

    // Actually allocate the memory
    void* pvMem = MemManager::Get().Allocate(stSizeInBytes,
        EE_MEM_ALIGNMENT_DEFAULT, kHint, EE_MET_MALLOC
#ifdef EE_MEMORY_DEBUGGER
        , pcSourceFile, iSourceLine, pcFunction
#endif
   );

    EE_ASSERT(pvMem != NULL);

    // Return the allocated memory advanced by the size of the header
    return pvMem;
}

//------------------------------------------------------------------------------------------------
// _AlignedMalloc (see notes above)
//------------------------------------------------------------------------------------------------
#ifdef EE_MEMORY_DEBUGGER
void* efd::_AlignedMalloc(
    size_t stSizeInBytes,
    size_t stAlignment,
    MemHint kHint,
    const char* pcSourceFile,
    int iSourceLine,
    const char* pcFunction)
#else
void* efd::_AlignedMalloc(size_t stSizeInBytes, size_t stAlignment, MemHint kHint)
#endif
{
    EE_ASSERT(MemManager::IsInitialized());

    if (stSizeInBytes == 0)
        stSizeInBytes = 1;

    // Actually allocate the memory
    return MemManager::Get().Allocate(stSizeInBytes, stAlignment, kHint,
        EE_MET_ALIGNEDMALLOC
#ifdef EE_MEMORY_DEBUGGER
        , pcSourceFile, iSourceLine, pcFunction
#endif
   );
}

//------------------------------------------------------------------------------------------------
// _Realloc (see notes above)
//------------------------------------------------------------------------------------------------
#ifdef EE_MEMORY_DEBUGGER
void* efd::_Realloc(
    void *pvMem,
    size_t stSizeInBytes,
    MemHint kHint,
    const char* pcSourceFile,
    int iSourceLine,
    const char* pcFunction)
#else
void* efd::_Realloc(void *pvMem, size_t stSizeInBytes, MemHint kHint)
#endif
{
    EE_ASSERT(MemManager::IsInitialized());

    // If the intention is to use EE_REALLOC like EE_FREE, just use
    // EE_FREE.
    if (stSizeInBytes == 0 && pvMem != 0)
    {
        EE_FREE(pvMem);
        return NULL;
    }
    else if (pvMem == 0)
    {
        return _Malloc(stSizeInBytes, kHint
#ifdef EE_MEMORY_DEBUGGER
            , pcSourceFile, iSourceLine, pcFunction
#endif
       );
    }

    // Actually reallocate the memory
    pvMem = MemManager::Get().Reallocate(pvMem, stSizeInBytes,
        EE_MEM_ALIGNMENT_DEFAULT, kHint, EE_MET_REALLOC,
        EE_MEM_DEALLOC_SIZE_DEFAULT
#ifdef EE_MEMORY_DEBUGGER
        , pcSourceFile, iSourceLine, pcFunction
#endif
   );

    return pvMem;
}

//------------------------------------------------------------------------------------------------
// _AlignedRealloc (see notes above)
//------------------------------------------------------------------------------------------------
#ifdef EE_MEMORY_DEBUGGER
void* efd::_AlignedRealloc(
    void *pvMem,
    size_t stSizeInBytes,
    size_t stAlignment,
    MemHint kHint,
    const char* pcSourceFile,
    int iSourceLine,
    const char* pcFunction)
#else
void* efd::_AlignedRealloc(
    void *pvMem,
    size_t stSizeInBytes,
    size_t stAlignment,
    MemHint kHint)
#endif
{
    EE_ASSERT(MemManager::IsInitialized());

    // If the intention is to use EE_REALLOC like EE_FREE, just use
    // EE_FREE.
    if (stSizeInBytes == 0 && pvMem != 0)
    {
        EE_ALIGNED_FREE(pvMem);
        return NULL;
    }
    else if (pvMem == 0)
    {
        return _AlignedMalloc(stSizeInBytes, stAlignment, kHint
#ifdef EE_MEMORY_DEBUGGER
            , pcSourceFile, iSourceLine, pcFunction
#endif
       );
    }

    // Actually reallocate the memory
    return MemManager::Get().Reallocate(pvMem, stSizeInBytes, stAlignment,
        kHint, EE_MET_ALIGNEDREALLOC, EE_MEM_DEALLOC_SIZE_DEFAULT
#ifdef EE_MEMORY_DEBUGGER
        , pcSourceFile, iSourceLine, pcFunction
#endif
   );

}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// EE_VERIFY_ADDRESS (see notes above)
//------------------------------------------------------------------------------------------------
#ifdef EE_MEMORY_DEBUGGER
// Check the memory management system to make sure that the address is
// currently active and was allocated by our system. For convenience,
// NULL is the only invalid address that is acceptable for this function.
bool  efd::_VerifyAddress(const void* pvMemory)
{
    if (pvMemory == NULL)
        return true;

    EE_ASSERT(MemManager::IsInitialized());
    return MemManager::Get().VerifyAddress(pvMemory);
}
#endif

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// EETrackAlloc / EETrackFree (see notes above)
//------------------------------------------------------------------------------------------------
#ifdef EE_MEMORY_DEBUGGER
bool efd::_TrackAlloc(const void* const pvMemory, size_t stSizeInBytes, MemHint kHint,
    const char* pcSourceFile, int iSourceLine, const char* pcFunction)
{
    if (pvMemory == NULL)
        return true;

    EE_ASSERT(MemManager::IsInitialized());
    return MemManager::Get().TrackAllocate(pvMemory, stSizeInBytes,
        kHint, EE_MET_EXTERNALALLOC, pcSourceFile, iSourceLine, pcFunction);
}

//------------------------------------------------------------------------------------------------
bool efd::_TrackFree(const void* const pvMemory)
{
    if (pvMemory == NULL)
        return true;

    EE_ASSERT(MemManager::IsInitialized());
    return MemManager::Get().TrackDeallocate(pvMemory, EE_MET_EXTERNALFREE);
}
#endif

//------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------
// _Free (see notes above)
//------------------------------------------------------------------------------------------------
void efd::_Free(void* pvMem, size_t stSizeInBytes)
{
    if (pvMem == NULL)
        return;

    if (stSizeInBytes == 0)
        stSizeInBytes = 1;

    MemManager::Get().Deallocate(pvMem, EE_MET_FREE, stSizeInBytes);
}

//------------------------------------------------------------------------------------------------
// _AlignedFree (see notes above)
//------------------------------------------------------------------------------------------------
void efd::_AlignedFree(void* pvMem, size_t stSizeInBytes)
{
    if (pvMem == NULL)
        return;

    if (stSizeInBytes == 0)
        stSizeInBytes = 1;

    MemManager::Get().Deallocate(pvMem, EE_MET_ALIGNEDFREE, stSizeInBytes);
}

//------------------------------------------------------------------------------------------------
void efd::_LogMemAssert(const char* pcCondition, const AllocUnit* pkUnit,
    const char* pcFile, int iLine, const char* pcFunction)
{
    efd::Char acBuff[1024];
    efd::Sprintf(acBuff, 1024, "%s:%d (%s) %s -- MemAssert failed.\n",
        pcFile, iLine, pcFunction, pcCondition);

    EE_UNUSED_ARG(pkUnit);

    EE_OUTPUT_DEBUG_STRING(acBuff);
    EE_DEBUG_BREAK();
}

//------------------------------------------------------------------------------------------------
#ifdef EE_MEMORY_DEBUGGER
bool efd::_MemMarker(const char* pcMarkerType, const char* pcClassifier,
    const char* pcString)
{
    MemManager::Get().SetMarker(pcMarkerType, pcClassifier, pcString);
    return true;
}

//------------------------------------------------------------------------------------------------
bool efd::_MemMarker(const char* pcMarkerType, const char* pcClassifier,
    unsigned int uiValue)
{
    char acBufferString[256];
    efd::Sprintf(acBufferString, 256, "%d", uiValue);
    MemManager::Get().SetMarker(pcMarkerType, pcClassifier, acBufferString);
    return true;
}

//------------------------------------------------------------------------------------------------
bool efd::_MemMarker(const char* pcMarkerType, const char* pcClassifier,
    const void* const pvValue)
{
    char acBufferString[256];
    efd::Sprintf(acBufferString, 256, "0x%p", pvValue);
    MemManager::Get().SetMarker(pcMarkerType, pcClassifier, acBufferString);
    return true;
}

//------------------------------------------------------------------------------------------------
#endif
