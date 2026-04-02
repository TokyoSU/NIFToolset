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
#include "NiSystemPCH.h"

#include "NiLog.h"
#include "NiStaticDataManager.h"
#include "NiSystemSDM.h"
#include "NiRTLib.h"
#include "NiInitOptions.h"
#include "NiMemoryDefines.h"
#include "NiDebug.h"
#include "NiSystem.h"

const NiInitOptions* NiStaticDataManager::ms_pkInitOptions = NULL;
bool NiStaticDataManager::ms_bAutoCreatedInitOptions = false;
bool NiStaticDataManager::ms_initialized = false;

//--------------------------------------------------------------------------------------------------
void NiStaticDataManager::Init(const NiInitOptions* pkOptions, bool kEEInit)
{
    if (pkOptions == NULL)
    {
        pkOptions = NiExternalNew NiInitOptions();
        ms_bAutoCreatedInitOptions = true;
    }
    else
    {
        if (!kEEInit)
            EE_ASSERT(pkOptions == efd::StaticDataManager::GetInitOptions());
            
        ms_bAutoCreatedInitOptions = false;
    }

    EE_ASSERT(pkOptions);
    ms_pkInitOptions = pkOptions;

    ms_initialized = true;

    if (kEEInit)
    {
        EE_INIT(pkOptions);
    }
    else 
    {
        ProcessAccumulatedLibraries();
    }

    NiOutputDebugString("NiStaticDataManager Initialized\n");
}

//--------------------------------------------------------------------------------------------------
void NiStaticDataManager::Shutdown(bool kEEShutdown)
{
    if (kEEShutdown)
        EE_SHUTDOWN();

    if (ms_bAutoCreatedInitOptions)
        NiExternalDelete ms_pkInitOptions;

    NiOutputDebugString("NiStaticDataManager Shutdown\n");
}

//--------------------------------------------------------------------------------------------------
const NiInitOptions* NiStaticDataManager::GetInitOptions()
{
    return ms_pkInitOptions;
}

//--------------------------------------------------------------------------------------------------
void NiStaticDataManager::AddLibrary(const char* libraryName,
    efd::StaticDataManager::InitFunction pInit,
    efd::StaticDataManager::ShutdownFunction pShutdown,
    const char* dependencies)
{
    efd::StaticDataManager::AddLibrary(libraryName, pInit, pShutdown,
        NULL, NULL, dependencies);
}

//--------------------------------------------------------------------------------------------------
void NiStaticDataManager::RemoveLibrary(const char* libraryName)
{
    efd::StaticDataManager::RemoveLibrary(libraryName);
}

//--------------------------------------------------------------------------------------------------
void NiStaticDataManager::ProcessAccumulatedLibraries()
{
    if (ms_initialized)
        efd::StaticDataManager::ProcessAccumulatedLibraries();
}

//--------------------------------------------------------------------------------------------------
bool NiStaticDataManager::IsInitialized()
{
    return ms_initialized;
}

//--------------------------------------------------------------------------------------------------