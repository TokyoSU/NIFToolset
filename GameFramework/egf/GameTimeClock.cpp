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

#include "egfPCH.h"

#include <egf/GameTimeClock.h>
#include <efd/ServiceManager.h>
#include <efd/Asserts.h>

using namespace efd;
using namespace egf;

/// The maximum number of times we expect pause to be called without calling resume.  Used for
/// debugging.  If this gets hit most likely someone is forgetting to call resume or is calling
/// pause more often then expected.
static const UInt32 kMaxExpectedPauses = 200;

//------------------------------------------------------------------------------------------------
GameTimeClock::GameTimeClock(efd::TimeType currentRealTime)
: efd::IClock()
, m_currentTime(0.0)
, m_realTime(currentRealTime)
, m_lastDelta(0.0)
, m_maxDelta(0.25)
, m_pauseCount(0)
, m_useFixedDelta(false)
, m_fixedDelta(0.0)
, m_realTimeScale(1.0)
{
    ComputeDelta();
}

//------------------------------------------------------------------------------------------------
void GameTimeClock::ComputeDelta()
{
    // When this method is called, we know that m_currentTime has already been set to the correct
    // value so we simply want to compute a delta that would yeild that result.  We know that:
    //      CurrentTime = (RealTime - Delta) * Scale
    // so therefore:
    //      Delta = RealTime - CurrentTime / Scale
    // Keep in mind that floating point math on computers always intales the addition of some
    // amount of error.  The computed delta is as good as we can get, but we need to take care
    // to ensure that time never moves backwards.
    //
    // Of course, if we happen to be paused we can skip this computation since a new delta will
    // be computed when we resume.
    if (0 == m_pauseCount)
    {
        TimeType realTime = GetRealTime();
        m_realTimeDelta = realTime - m_currentTime / m_realTimeScale;
    }
}

//------------------------------------------------------------------------------------------------
efd::TimeType GameTimeClock::GetCurrentTime() const
{
    return m_currentTime;
}

//------------------------------------------------------------------------------------------------
efd::TimeType GameTimeClock::GetRealTime()
{
    // Every tick the service manager updates us with the "real" time.
    return m_realTime;
}

//------------------------------------------------------------------------------------------------
efd::TimeType GameTimeClock::GetLastDelta() const
{
    return m_lastDelta;
}

//------------------------------------------------------------------------------------------------
void GameTimeClock::Update(efd::TimeType serviceFrameTime)
{
    m_realTime = serviceFrameTime;

    if (0 == m_pauseCount)
    {
        if (m_useFixedDelta)
        {
            efd::TimeType newTime = m_currentTime + m_fixedDelta;
            // If you hit this assert then either your delta time is too small or your current
            // time is too large.  The goal of this clock class is to provide millisecond level
            // accuracy for periods of up to ten years, so if this is hit then hopefully either
            // m_fixedDelta is less than 0.001 or m_currentTime is greater than ~315,360,000.
            EE_ASSERT(newTime > m_currentTime);
            m_lastDelta = m_fixedDelta;
            m_currentTime = newTime;
        }
        else
        {
            efd::TimeType realTime = GetRealTime();
            efd::TimeType adjustedTime = realTime - m_realTimeDelta;
            efd::TimeType scaledTime = adjustedTime * m_realTimeScale;

            // Cap update to an upper limit.  This prevents huge time jumps when debugging
            efd::TimeType maxTime = m_currentTime + m_maxDelta;
            bool fNeedToAdjustOffset = false;
            if (scaledTime > maxTime)
            {
                scaledTime = maxTime;
                fNeedToAdjustOffset = true;
            }

            // We must ensure time never moves backwards.  Floating pointing rounding combined
            // with operations like pause/resume and changing scale could lead to enough error
            // to move time backwards, so we protect against this.
            if (scaledTime > m_currentTime)
            {
                m_lastDelta = scaledTime - m_currentTime;
                m_currentTime = scaledTime;

                if (fNeedToAdjustOffset)
                {
                    // Since we capped time to Max Delta we need to compute a new offset
                    ComputeDelta();
                }
            }
        }
    }
    else
    {
        m_lastDelta = 0.0;
    }
}

//------------------------------------------------------------------------------------------------
bool GameTimeClock::SetCurrentTime(efd::TimeType currentTime)
{
    // Ensure time is moving forward
    if (currentTime > m_currentTime)
    {
        m_currentTime = currentTime;
        // We need to compute a new delta so that our next update call doesn't reverse the skip
        // we just applied:
        ComputeDelta();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void GameTimeClock::Pause()
{
    EE_ASSERT(m_pauseCount < kMaxExpectedPauses);
    ++m_pauseCount;
}

//------------------------------------------------------------------------------------------------
bool GameTimeClock::IsPaused() const
{
    return m_pauseCount > 0;
}

//------------------------------------------------------------------------------------------------
void GameTimeClock::Resume()
{
    EE_ASSERT(m_pauseCount > 0);
    --m_pauseCount;
    if (0 == m_pauseCount)
    {
        // resuming, compute a new delta
        ComputeDelta();
    }
}

//------------------------------------------------------------------------------------------------
void GameTimeClock::UseFixedDeltaMode(efd::TimeType deltaPerTick)
{
    m_useFixedDelta = true;
    m_fixedDelta = deltaPerTick;
}

//------------------------------------------------------------------------------------------------
void GameTimeClock::UseRealTimeMode()
{
    if (m_useFixedDelta)
    {
        m_useFixedDelta = false;
        // When turning off fixed delta mode we need to recompute the delta:
        ComputeDelta();
    }
}

//------------------------------------------------------------------------------------------------
bool GameTimeClock::SetTimeScale(efd::Float64 scale)
{
    if (scale > 0.0)
    {
        m_realTimeScale = scale;
        ComputeDelta();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool GameTimeClock::SetMaxDelta(efd::Float64 delta)
{
    if (delta > 0.0)
    {
        m_maxDelta = delta;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
efd::ClassID GameTimeClock::GetClockClassID() const
{
    return efd::kCLASSID_GameTimeClock;
}

//------------------------------------------------------------------------------------------------
