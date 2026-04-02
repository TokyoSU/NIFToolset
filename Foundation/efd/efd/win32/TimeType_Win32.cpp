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

#include <efd/TimeType.h>
#include <Time.h>

using namespace efd;

// File scope globals used by both GetCurrentTimeInSec() and SetInitialTimeInSec()
bool gs_appTimeInitialized = false;
LARGE_INTEGER gs_freq;
LARGE_INTEGER gs_appTimeOffset;

//------------------------------------------------------------------------------------------------
void efd::SetInitialTimeInSec(float offsetInSeconds /* = 0.0f */)
{
    QueryPerformanceFrequency(&gs_freq);
    QueryPerformanceCounter(&gs_appTimeOffset);

    gs_appTimeOffset.QuadPart -=
        (LONGLONG)((long double)offsetInSeconds * (long double)gs_freq.QuadPart);

    gs_appTimeInitialized = true;
}

//------------------------------------------------------------------------------------------------
efd::TimeType efd::GetCurrentTimeInSec()
{
    if (!gs_appTimeInitialized)
    {
        SetInitialTimeInSec();
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return efd::TimeType((long double)(counter.QuadPart - gs_appTimeOffset.QuadPart) /
        (long double) gs_freq.QuadPart);
}

//------------------------------------------------------------------------------------------------
// Implementation of efd::RealTimeClock
//------------------------------------------------------------------------------------------------
TimeType RealTimeClock::GetCurrentTime()
{
    // DT32308 This is only has 1 sec accuracy, we need to use a better method
    __time64_t long_time;
    _time64(&long_time);
    return (efd::Float64)long_time;
}

//------------------------------------------------------------------------------------------------
// Implementation of efd::HighPrecisionClock
//------------------------------------------------------------------------------------------------
TimeType HighPrecisionClock::GetCurrentTime() const
{
    // This method is very similar to GetCurrentTimeInSec(), but it always returns time since app
    // start instead of time since the last SetInitialTimeInSec() call.
    static bool bFirst = true;
    static LARGE_INTEGER freq;
    static LARGE_INTEGER initial;

    if (bFirst)
    {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&initial);
        bFirst = false;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    efd::Float64 current = (efd::Float64)((efd::Float64)(counter.QuadPart - initial.QuadPart) /
        (efd::Float64) freq.QuadPart);

    current += (efd::Float64)m_syncOffset;

    return current;
}

//------------------------------------------------------------------------------------------------
void HighPrecisionClock::SetSynchronizationOffset(TimeType i_offset)
{
    m_syncOffset = i_offset;
}

//------------------------------------------------------------------------------------------------
