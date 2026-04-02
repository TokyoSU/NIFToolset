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
#include "NiTerrainConditionVariable.h"

//--------------------------------------------------------------------------------------------------
efd::CriticalSection NiTerrainConditionVariable::ms_kCVSemaphores;
NiTerrainConditionVariable::SemaphoreMap NiTerrainConditionVariable::ms_kThreadSemaphores;
//--------------------------------------------------------------------------------------------------
void NiTerrainConditionVariable::_SDMInit()
{
}

//--------------------------------------------------------------------------------------------------
void NiTerrainConditionVariable::_SDMShutdown()
{
    // Empty the ThreadSemaphore map.
    ms_kCVSemaphores.Lock();
    SemaphoreMap::iterator kIter;
    for (kIter = ms_kThreadSemaphores.begin(); kIter != ms_kThreadSemaphores.end(); ++kIter)
    {
        efd::Semaphore* pkSemaphore = kIter->second;
        delete pkSemaphore;
    }
    ms_kThreadSemaphores.clear();
    ms_kCVSemaphores.Unlock();
}

//--------------------------------------------------------------------------------------------------
efd::Semaphore* NiTerrainConditionVariable::GetThreadSemaphore()
{
    efd::Semaphore* pkSemaphore = NULL;

    ms_kCVSemaphores.Lock();
    // Find the semaphore for this thread
    SemaphoreMap::iterator kIter = ms_kThreadSemaphores.find(efd::GetCurrentThreadId());
    if (kIter != ms_kThreadSemaphores.end())
    {
        // Return this thread's semaphore
        pkSemaphore = kIter->second;
    }
    else
    {   
        // Create a semaphore for this thread
        pkSemaphore = new efd::Semaphore();
        ms_kThreadSemaphores.insert(std::make_pair(efd::GetCurrentThreadId(),
            pkSemaphore));
    }

    ms_kCVSemaphores.Unlock();
    return pkSemaphore;
}

//--------------------------------------------------------------------------------------------------
