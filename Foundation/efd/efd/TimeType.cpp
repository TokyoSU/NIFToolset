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

using namespace efd;

//-------------------------------------------------------------------------------------------------
IClock::~IClock()
{
}

//-------------------------------------------------------------------------------------------------
void HighPrecisionClock::Update(TimeType /*serviceFrameTime*/)
{
    // Nothing to do, every call to GetCurrentTime computes the current value
}

//-------------------------------------------------------------------------------------------------
efd::ClassID HighPrecisionClock::GetClockClassID() const
{
    return kCLASSID_HighPrecisionClock;
}

//-------------------------------------------------------------------------------------------------
TimeType StepHighPrecisionClock::GetCurrentTime() const
{
    return m_currentTime;
}

//-------------------------------------------------------------------------------------------------
void StepHighPrecisionClock::Update(TimeType /*serviceFrameTime*/)
{
    m_currentTime = ComputeCurrentTime();
}

//-------------------------------------------------------------------------------------------------
efd::ClassID StepHighPrecisionClock::GetClockClassID() const
{
    return kCLASSID_StepHighPrecisionClock;
}
