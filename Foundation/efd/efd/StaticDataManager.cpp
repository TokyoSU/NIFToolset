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

#include <efd/StaticDataManager.h>
#include <efd/FoundationSDM.h>
#include <efd/InitOptions.h>
#include <efd/MemoryDefines.h>
#include <efd/Asserts.h>
#include <efd/Utilities.h>
#include <efd/ILogger.h>
#include <efd/MemManager.h>
#include <efd/MemTracker.h>

using namespace efd;

UInt8 StaticDataManager::ms_uiNumLibraries = 0;
UInt8 StaticDataManager::ms_uiNumInitializedLibraries = 0;
bool StaticDataManager::ms_initialized = false;
StaticDataManager::LibraryNode StaticDataManager::ms_akLibraries[EE_NUM_LIBRARIES];

UInt8 StaticDataManager::ms_auiProcessOrder[EE_NUM_LIBRARIES];

const InitOptions* StaticDataManager::ms_pInitOptions = NULL;
bool StaticDataManager::ms_autoCreatedInitOptions = false;

//--------------------------------------------------------------------------------------------------
void StaticDataManager::Init(const InitOptions* pOptions)
{
    // EFD is a required library, so its Init function is called directly
    // instead of being registered.
    if (pOptions == NULL)
    {
        pOptions = EE_EXTERNAL_NEW InitOptions();
        ms_autoCreatedInitOptions = true;
    }
    else
    {
        ms_autoCreatedInitOptions = false;
    }

    EE_ASSERT(pOptions);
    ms_pInitOptions = pOptions;

    if (!ms_initialized)
    {
        // Foundation is a required library, so it is initialized directly instead of
        // via ProcessAccumulatedLibraries
        FoundationSDM::Init();
        FoundationSDM::PerThreadInit();
    }

    ms_initialized = true;

    ProcessAccumulatedLibraries();

    EE_OUTPUT_DEBUG_STRING("efd::StaticDataManager Initialized\n");
}

//--------------------------------------------------------------------------------------------------
void StaticDataManager::Shutdown()
{
    if (ms_initialized)
    {
        PerThreadShutdown();

        // Shutdown in reverse order of initialization
        for (int i = ms_uiNumInitializedLibraries-1; i >= 0 ; i--)
        {
            ShutdownFunction pfnShutdown =
                *ms_akLibraries[ms_auiProcessOrder[i]].m_pfnShutdownFunction;
            if (pfnShutdown)
                pfnShutdown();
        }
        ms_uiNumInitializedLibraries = 0;

        // Called explicitly since the static data manager lives in this library.
        FoundationSDM::Shutdown();

        if (ms_autoCreatedInitOptions)
            EE_EXTERNAL_DELETE ms_pInitOptions;

        ms_initialized = false;
        EE_OUTPUT_DEBUG_STRING("efd::StaticDataManager Shutdown\n");

        // Notify the memory tracker that allocations are no longer allowed ('outside of main')
        MemTracker* pkTracker = MemTracker::Get();
        if (pkTracker != NULL)
            pkTracker->SetAllocatorAvailableStatus(false);
    }
}

//--------------------------------------------------------------------------------------------------
void StaticDataManager::PerThreadInit()
{
    // Foundation is a required library, so its PerThreadInit function is called
    // directly instead of being registered.
    FoundationSDM::PerThreadInit();

    for (UInt32 ui = 0; ui < ms_uiNumLibraries; ui++)
    {
        InitFunction pfnInit =
            *ms_akLibraries[ms_auiProcessOrder[ui]].m_pfnPerThreadInitFunction;
        if (pfnInit)
            pfnInit();
    }
}

//--------------------------------------------------------------------------------------------------
void StaticDataManager::PerThreadShutdown()
{
    // Shutdown in reverse order of initialization
    for (int i = ms_uiNumLibraries-1; i >= 0 ; i--)
    {
        ShutdownFunction pfnShutdown =
            *ms_akLibraries[ms_auiProcessOrder[i]].m_pfnPerThreadShutdownFunction;
        if (pfnShutdown)
            pfnShutdown();
    }

    // Foundation is a required library, so its PerThreadShutdown functions is called
    // directly instead of being registered.
    FoundationSDM::PerThreadShutdown();
}

//--------------------------------------------------------------------------------------------------
void StaticDataManager::AddLibrary(const char* pcLibName,
    InitFunction pfnInit,
    ShutdownFunction pfnShutdown,
    InitFunction pfnPerThreadInit,
    ShutdownFunction pfnPerThreadShutdown,
    const char* pcDependencies)
{
    // You hit this assert if there are more libraries registering than the
    // amount of allocated space. To change the amount of allocated space,
    // modify EE_NUM_LIBRARIES in StaticManager.h.
    EE_ASSERT(ms_uiNumLibraries < EE_NUM_LIBRARIES);

    // Store all there is to know about the library
    ms_akLibraries[ms_uiNumLibraries].m_pcName = pcLibName;
    ms_akLibraries[ms_uiNumLibraries].m_uiIndex = ms_uiNumLibraries;
    ms_akLibraries[ms_uiNumLibraries].m_pcDependsOn = pcDependencies;
    ms_akLibraries[ms_uiNumLibraries].m_uiNumDependencies = 0;
    ms_akLibraries[ms_uiNumLibraries].m_uiMaxDependencies = 0;
    ms_akLibraries[ms_uiNumLibraries].m_puiDependencies = 0;
    ms_akLibraries[ms_uiNumLibraries].m_pfnInitFunction = pfnInit;
    ms_akLibraries[ms_uiNumLibraries].m_pfnShutdownFunction = pfnShutdown;
    ms_akLibraries[ms_uiNumLibraries].m_pfnPerThreadInitFunction = pfnPerThreadInit;
    ms_akLibraries[ms_uiNumLibraries].m_pfnPerThreadShutdownFunction = pfnPerThreadShutdown;

    // Initially the process order is the order it is added.
    ms_auiProcessOrder[ms_uiNumLibraries] = ms_uiNumLibraries;

    ms_uiNumLibraries++;

}

//--------------------------------------------------------------------------------------------------
void StaticDataManager::RemoveLibrary(const char* pcLibName)
{
    UInt8 uiRemove = 0;
    for (uiRemove = 0; uiRemove < ms_uiNumLibraries; uiRemove++)
    {
        if (!strcmp(pcLibName, ms_akLibraries[uiRemove].m_pcName))
        {
            break;
        }
    }

    // You hit this assert if you try to remove a library that has not been
    // added.
    EE_ASSERT(uiRemove != ms_uiNumLibraries);

    // You hit this assert if the library you are trying to remove still has
    // active dependents. Remove the dependents first.
    EE_ASSERT(!HasDependents(uiRemove));

    // This test is only valid because we do not re-arrange the order
    // of initialization unless we also update ms_uiNumInitializedLibraries.
    if (uiRemove < ms_uiNumInitializedLibraries)
    {
        ShutdownFunction pfnShutdown =
            ms_akLibraries[uiRemove].m_pfnShutdownFunction;
        if (pfnShutdown)
            pfnShutdown();

        ms_uiNumInitializedLibraries--;
    }

    for (UInt32 ui = uiRemove + 1; ui < ms_uiNumLibraries; ui++)
    {
        --ms_akLibraries[ui].m_uiIndex;
        ms_akLibraries[ui - 1] = ms_akLibraries[ui];
    }

    // Find it in the process order list, and modify the process order
    // list indices to reflect the shuffle done above.
    UInt8 uiOrderRemove = ms_uiNumLibraries;
    for (UInt32 ui = 0; ui < ms_uiNumLibraries; ui++)
    {
        if (ms_auiProcessOrder[ui] == uiRemove)
        {
            uiOrderRemove = (UInt8)ui;
        }
        else if (ms_auiProcessOrder[ui] > uiRemove)
        {
            ms_auiProcessOrder[ui]--;
        }
    }

    // You hit this assert if the library is not in the process order.
    // Failure indicates an internal problem in this class.
    EE_ASSERT(uiOrderRemove != ms_uiNumLibraries);

    for (UInt32 ui = uiOrderRemove + 1; ui < ms_uiNumLibraries; ui++)
    {
        ms_auiProcessOrder[ui - 1] = ms_auiProcessOrder[ui];
    }

    ms_uiNumLibraries--;
}

//--------------------------------------------------------------------------------------------------
void StaticDataManager::ProcessAccumulatedLibraries()
{
    if (!ms_initialized || !ms_uiNumLibraries)
        return;

    // Initialize all libraries
    if (ms_uiNumInitializedLibraries == 0)
    {
        EE_VERIFY(ComputeProcessOrder());
    }

    for (UInt32 ui = ms_uiNumInitializedLibraries; ui < ms_uiNumLibraries; ui++)
    {
        InitFunction pfnInit =
            *ms_akLibraries[ms_auiProcessOrder[ui]].m_pfnInitFunction;
        if (pfnInit)
            pfnInit();

        InitFunction pfnThreadInit =
            *ms_akLibraries[ms_auiProcessOrder[ui]].m_pfnPerThreadInitFunction;
        if (pfnThreadInit)
            pfnThreadInit();
    }

    ms_uiNumInitializedLibraries = ms_uiNumLibraries;
}

//--------------------------------------------------------------------------------------------------
bool StaticDataManager::HasDependents(const UInt8 uiLibIndex)
{
    const char* pcName = ms_akLibraries[uiLibIndex].m_pcName;
    for (UInt32 ui = 0; ui < ms_uiNumInitializedLibraries; ui++)
    {
        if (uiLibIndex == ui)
            continue;

        if (strstr(ms_akLibraries[ui].m_pcDependsOn, pcName))
           return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool StaticDataManager::DependenciesInitialized(const UInt8 uiLibIndex)
{
    size_t uiLength = strlen(ms_akLibraries[uiLibIndex].m_pcDependsOn) + 1;
    char* pcDependsOn = (char*)EE_EXTERNAL_MALLOC(uiLength);
    efd::Strncpy(pcDependsOn, uiLength, ms_akLibraries[uiLibIndex].m_pcDependsOn,
        uiLength - 1);
    char* pcContext = 0;
    char* pcDependency = 0;
    pcDependency = efd::Strtok(pcDependsOn, " ", &pcContext);
    while (pcDependency)
    {
        UInt32 uiDepIndex = 0;
        for (; uiDepIndex < ms_uiNumInitializedLibraries ; uiDepIndex++)
        {
            if (!strcmp(pcDependency, ms_akLibraries[uiDepIndex].m_pcName))
            {
                break;
            }
        }
        if (uiDepIndex == ms_uiNumInitializedLibraries)
            return false;
        pcDependency = efd::Strtok(0, " ", &pcContext);
    }

    EE_EXTERNAL_FREE(pcDependsOn);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool StaticDataManager::ComputeProcessOrder()
{
    if (ms_uiNumLibraries == 0)
        return true;

    UInt8 auiUnvisited[EE_NUM_LIBRARIES];
    UInt8 uiNumUnvisited = 0;

    UInt8 auiDependencyFree[EE_NUM_LIBRARIES];
    UInt8 uiNumDependencyFree = 0;

    // Set all the edges in the graph
    for (UInt8 ui = 0; ui < ms_uiNumLibraries; ui++)
    {
        if (!ConstructDependencyGraph(ui, auiUnvisited, uiNumUnvisited,
            auiDependencyFree, uiNumDependencyFree))
        {
            return false;
        }
    }

    // If you hit this assert, then there is no library without dependencies,
    // which implies a circular dependency structure.
    EE_ASSERT(uiNumDependencyFree > 0);

    UInt8 uiNumProcessed = 0;
    while (uiNumDependencyFree > 0)
    {
        UInt8 uiNextLib = auiDependencyFree[uiNumDependencyFree - 1];
        --uiNumDependencyFree;

        ms_auiProcessOrder[uiNumProcessed] = uiNextLib;

        RemoveDependencies(uiNextLib, auiUnvisited, uiNumUnvisited,
            auiDependencyFree, uiNumDependencyFree);

        ++uiNumProcessed;
    }

    // When done, there should be no unvisited nodes. If there are, there is
    // a circular dependency.
    EE_ASSERT(uiNumUnvisited == 0);
    EE_ASSERT(uiNumProcessed == ms_uiNumLibraries);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool StaticDataManager::ConstructDependencyGraph(const UInt8 uiLibIndex,
    UInt8* auiUnvisited,
    UInt8& uiNumUnvisited,
    UInt8* auiDependencyFree,
    UInt8& uiNumDependencyFree)
{
    size_t uiLength = strlen(ms_akLibraries[uiLibIndex].m_pcDependsOn) + 1;
    char* pcDependsOn = (char*)EE_EXTERNAL_MALLOC(uiLength);
    efd::Strncpy(pcDependsOn, uiLength, ms_akLibraries[uiLibIndex].m_pcDependsOn,
        uiLength - 1);
    char* pcContext = 0;
    char* pcDependency = 0;
    pcDependency = efd::Strtok(pcDependsOn, " ", &pcContext);

    if (!pcDependency)
    {
        auiDependencyFree[uiNumDependencyFree] = uiLibIndex;
        ++uiNumDependencyFree;
    }
    else
    {
        while (pcDependency)
        {
            UInt32 uiDepIndex = 0;
            for (; uiDepIndex < ms_uiNumLibraries ; uiDepIndex++)
            {
                if (!strcmp(pcDependency, ms_akLibraries[uiDepIndex].m_pcName))
                {
                    AddDependency(uiLibIndex, (UInt8)uiDepIndex);
                    break;
                }
            }
            if (uiDepIndex == ms_uiNumLibraries)
            {
                EE_OUTPUT_DEBUG_STRING("------- ERROR -------\n");
                char acMsg[1024];
                efd::Sprintf(acMsg, 1024, "StaticDataManager: Can't find "
                    "library %s that library %s depends on.\n",
                    pcDependency, ms_akLibraries[uiLibIndex].m_pcName);
                EE_OUTPUT_DEBUG_STRING(acMsg);
                efd::Sprintf(acMsg, 1024, "StaticDataManager: Did you include "
                    "the top-level header file for %s?\n", pcDependency);
                EE_OUTPUT_DEBUG_STRING(acMsg);

                EE_FAIL("Missing #include library. See debugger output text.");
                return false;
            }
            pcDependency = efd::Strtok(0, " ", &pcContext);
        }

        auiUnvisited[uiNumUnvisited] = uiLibIndex;
        ++uiNumUnvisited;
    }

    EE_EXTERNAL_FREE(pcDependsOn);

    return true;
}

//--------------------------------------------------------------------------------------------------
void StaticDataManager::AddDependency(const UInt8 uiLibIndex,
    const UInt8 uiDependentIndex)
{
    LibraryNode* pkLib = ms_akLibraries + uiLibIndex;
    if (pkLib->m_uiMaxDependencies == pkLib->m_uiNumDependencies)
    {
        pkLib->m_uiMaxDependencies += 5;
        UInt8* puiNewDependencies = (UInt8*)EE_EXTERNAL_MALLOC(pkLib->m_uiMaxDependencies);

        for (UInt32 ui = 0; ui < pkLib->m_uiNumDependencies; ui++)
        {
            puiNewDependencies[ui] = pkLib->m_puiDependencies[ui];
        }

        if (pkLib->m_puiDependencies)
        {
            EE_EXTERNAL_FREE(pkLib->m_puiDependencies);
        }
        pkLib->m_puiDependencies = puiNewDependencies;
    }

    pkLib->m_puiDependencies[pkLib->m_uiNumDependencies] = uiDependentIndex;
    ++pkLib->m_uiNumDependencies;
}

//--------------------------------------------------------------------------------------------------
void StaticDataManager::RemoveDependencies(const UInt8 uiLibIndex,
    UInt8* auiUnvisited,
    UInt8& uiNumUnvisited,
    UInt8* auiDependencyFree,
    UInt8& uiNumDependencyFree)
{
    UInt32 ui = 0;
    while (ui < uiNumUnvisited)
    {
        LibraryNode* pkLib = ms_akLibraries + auiUnvisited[ui];
        bool bRemoved = false;

        for (UInt32 uj = 0 ; uj < pkLib->m_uiNumDependencies; uj++)
        {
            if (pkLib->m_puiDependencies[uj] == uiLibIndex)
            {
                for (UInt32 uk = uj + 1; uk < pkLib->m_uiNumDependencies; uk++)
                {
                    pkLib->m_puiDependencies[uk - 1] =
                        pkLib->m_puiDependencies[uk];
                }
                --pkLib->m_uiNumDependencies;
                if (!pkLib->m_uiNumDependencies)
                {
                    EE_EXTERNAL_FREE(pkLib->m_puiDependencies);
                    pkLib->m_puiDependencies = 0;
                    pkLib->m_uiMaxDependencies = 0;

                    auiDependencyFree[uiNumDependencyFree] = auiUnvisited[ui];
                    ++uiNumDependencyFree;

                    for (UInt32 uk = ui + 1; uk < uiNumUnvisited; uk++)
                    {
                        auiUnvisited[uk - 1] = auiUnvisited[uk];
                    }
                    --uiNumUnvisited;
                    bRemoved = true;
                }
                break;
            }
        }

        if (!bRemoved)
        {
            ui++;
        }
    }
}

//--------------------------------------------------------------------------------------------------
