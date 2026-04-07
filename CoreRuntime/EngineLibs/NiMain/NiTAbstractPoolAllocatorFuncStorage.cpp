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

#include "NiMainPCH.h"
#include "NiTAbstractPoolAllocatorFuncStorage.h"
#include <NiDebug.h>
#include <NiMemoryDefines.h>

//--------------------------------------------------------------------------------------------------
#if EE_USE_PER_THREAD_ALLOCATOR_POOLS
//--------------------------------------------------------------------------------------------------

#define BLOCK_CACHE_SIZE 5

NiTAbstractPoolAllocatorFuncStorage::AllocNode*
    NiTAbstractPoolAllocatorFuncStorage::ms_pkBlockHeader = NULL;

efd::CriticalSection NiTAbstractPoolAllocatorFuncStorage::ms_kCriticalSection;

void NiTAbstractPoolAllocatorFuncStorage::PerThreadShutdown()
{
    ms_kCriticalSection.Lock();

    AllocNode* pkNode = ms_pkBlockHeader;
    for (; pkNode != NULL; pkNode = pkNode->m_pkNext)
    {
        PerThreadShutdownFunction pfnPerThreadShutdown = pkNode->m_pfPerThreadShutdown;
        EE_ASSERT(pfnPerThreadShutdown);
        pfnPerThreadShutdown();
    }

    ms_kCriticalSection.Unlock();
}
//--------------------------------------------------------------------------------------------------
void NiTAbstractPoolAllocatorFuncStorage::AddPerThreadShutdown(
    PerThreadShutdownFunction pfnPerThreadShutdown)
{
    EE_ASSERT(pfnPerThreadShutdown);
    ms_kCriticalSection.Lock();

    AllocNode* pkNode = NiNew AllocNode();
    pkNode->m_pfPerThreadShutdown = pfnPerThreadShutdown;
    pkNode->m_pkNext = ms_pkBlockHeader;
    ms_pkBlockHeader = pkNode;
    
    ms_kCriticalSection.Unlock();
}
//--------------------------------------------------------------------------------------------------
void NiTAbstractPoolAllocatorFuncStorage::RemovePerThreadShutdown(
    PerThreadShutdownFunction pfnPerThreadShutdown)
{
    ms_kCriticalSection.Lock();

    AllocNode* pkNodePrev = NULL;
    AllocNode* pkNode = ms_pkBlockHeader;

    for (; pkNode != NULL; pkNode = pkNode->m_pkNext)
    {
        if (pkNode->m_pfPerThreadShutdown == pfnPerThreadShutdown)
        {
            if (pkNodePrev)
            {
                pkNodePrev->m_pkNext = pkNode->m_pkNext;
            }
            else
            {
                ms_pkBlockHeader = pkNode->m_pkNext;
            }
            NiDelete pkNode;

            ms_kCriticalSection.Unlock();
            return;
        }
        pkNodePrev = pkNode;
    }

    // If this assert is hit, the passed in value was not found in the
    // registered list of per-thread shutdowns
    EE_ASSERT(false);
    
    ms_kCriticalSection.Unlock();
}
#endif // EE_USE_PER_THREAD_ALLOCATOR_POOLS