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

#include <efd/MemManager.h>
#include <efd/StandardAllocator.h>
#include <efd/MemTracker.h>
#include <efd/StaticDataManager.h>
#include <efd/Logger.h>

#include <efd/DefaultInitializeMemoryManager.h>
#include <efd/efdLibType.h>

//------------------------------------------------------------------------------------------------

using namespace efd;

// Make use of the default memory allocator implementation if efd is being built
// as a DLL.
#if defined(EE_EFD_EXPORT)
EE_USE_DEFAULT_ALLOCATOR;
#endif

static bool g_MemManagerWasShutdown = false;
static bool g_MemManagerWasCreated = false;
static bool g_MemManagerIsPreMain = false;

namespace efd
{

MemManager* efd::MemManager::ms_pkMemManager = 0;

//------------------------------------------------------------------------------------------------
MemManager::MemManager()
    : m_pkAllocator(NULL)
    , m_bMemLogHandlerCreated(false)
{
    m_pkAllocator = CreateGlobalMemoryAllocator();
    EE_MEMASSERT(m_pkAllocator);
    m_pkAllocator->Initialize();

    g_MemManagerWasCreated = true;
    g_MemManagerWasShutdown = false;
    ms_pkMemManager = this;
    CreateMemoryLogHandler();
}

//------------------------------------------------------------------------------------------------
MemManager::~MemManager()
{
    m_pkAllocator->Shutdown();

    EE_EXTERNAL_DELETE(m_pkAllocator);
    m_pkAllocator = NULL;

    g_MemManagerWasShutdown = true;
}

//------------------------------------------------------------------------------------------------
void MemManager::CreateMemoryLogHandler()
{
    if (ms_pkMemManager->m_pkAllocator && !ms_pkMemManager->m_bMemLogHandlerCreated)
    {
        ms_pkMemManager->m_bMemLogHandlerCreated = true;
        ms_pkMemManager->m_pkAllocator->CreateMemoryLogHandler();
    }
}

//------------------------------------------------------------------------------------------------
MemManager& MemManager::Get()
{
    EE_MEMASSERT(g_MemManagerWasShutdown == false);

    if (g_MemManagerWasCreated)
    {
        return *ms_pkMemManager;
    }
    else
    {
        static MemManager s_kManager;
        g_MemManagerIsPreMain = true;

        return s_kManager;
    }
}

//------------------------------------------------------------------------------------------------
bool MemManager::VerifyAddress(const void* pvMemory)
{
    EE_MEMASSERT(ms_pkMemManager->m_pkAllocator);
    return ms_pkMemManager->m_pkAllocator->VerifyAddress(pvMemory);
}

//------------------------------------------------------------------------------------------------

void MemManager::_SDMInit()
{
    if (!g_MemManagerWasCreated)
    {
        // Create the allocator
        ms_pkMemManager = EE_EXTERNAL_NEW MemManager;
        g_MemManagerWasCreated = true;

        EE_MEMASSERT(g_MemManagerIsPreMain == false);
    }

    // Notify the memory tracker that allocations are now allowed ('inside of main')
    MemTracker* pkTracker = MemTracker::Get();
    if (pkTracker != NULL)
        pkTracker->SetAllocatorAvailableStatus(true);
}

//------------------------------------------------------------------------------------------------
void MemManager::_SDMShutdown()
{
    if (g_MemManagerWasCreated && !g_MemManagerIsPreMain)
    {
        EE_EXTERNAL_DELETE ms_pkMemManager;
        ms_pkMemManager = NULL;

        g_MemManagerWasCreated = false;
    }
}

//------------------------------------------------------------------------------------------------
void MemManager::_SDMPerThreadInit()
{
    MemManager::GetAllocator()->PerThreadInit();
}

//------------------------------------------------------------------------------------------------
void MemManager::_SDMPerThreadShutdown()
{
    MemManager::GetAllocator()->PerThreadShutdown();
}

//------------------------------------------------------------------------------------------------
} // end namespace efd

// Implement the static TLS inside StandardAllocator if it has one
EE_IMPLEMENT_SMALL_OBJECT_ALLOCATOR(efd::StandardAllocator);

//------------------------------------------------------------------------------------------------
