// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 2006-2009 Todd Berkebile.
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include "egfPCH.h"

#include <egf/Entity.h>
#include <egf/FlatModelManager.h>
#include <egf/Scheduler.h>
#include <egf/ScriptContext.h>
#include <efd/Metrics.h>
#include <egf/egfLogIDs.h>
#include <efd/MemTracker.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
/// This is the maximum number of Milliseconds that we expect a single behavior call to use.
/// If a behavior takes longer then we will generate a warning and log timing data.
const TimeType kMaxAcceptableBehaviorTime = 5.0;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(egf::PendingBehavior);

//------------------------------------------------------------------------------------------------
PendingBehaviorPtr PendingBehavior::Create(const BehaviorDescriptor* pBehavior,
                                            egf::Entity* pEntity,
                                            efd::ParameterListPtr spArgs,
                                            efd::TimeType executeTime,
                                            EventID eventID,
                                            EntityID returnID,
                                            bool immediate)
{
    if (eventID == 0)
    {
        eventID = EventID::CreateEventID();
    }

    PendingBehaviorPtr spResult = EE_NEW PendingBehavior(pEntity,
        spArgs,
        executeTime,
        eventID,
        returnID,
        immediate);

    if (!spResult->GetBehaviorCallList(pBehavior))
    {
        EE_LOG(efd::kBehavior, efd::ILogger::kERR1, ("No behaviors found for event %s:%s",
            pBehavior->GetModelName().c_str(),
            pBehavior->GetName().c_str()));
        spResult = NULL;
    }
    return spResult;
}

//------------------------------------------------------------------------------------------------
PendingBehavior::PendingBehavior(egf::Entity* pEntity,
                                  efd::ParameterListPtr spArgs,
                                  efd::TimeType executeTime,
                                  EventID eventID,
                                  EntityID returnID,
                                  bool immediate)
: ScheduledTask(eventID, executeTime)
, m_spExecutingEntity(pEntity)
, m_spArgs(spArgs)
, m_returnID(returnID)
, m_lifeCycleAdvanceOnCompletion(0)
, m_started(false)
, m_canBlock(false)
, m_immediate(immediate)
, m_blockedSleep(false)
, m_blockedReply(false)
, m_blockedDebug(false)
, m_blockedEnityCreation(false)

#ifdef EE_USE_BEHAVIOR_TIMING_METRICS
, m_accumulatedTime(0.0)
, m_startTime(0.0)
, m_eventTime(0.0)
#endif
{
    EE_MEM_SETDETAILEDREPORT(this, PendingBehavior::LeakDump);
}

//------------------------------------------------------------------------------------------------
PendingBehavior::~PendingBehavior()
{
}

//------------------------------------------------------------------------------------------------
void PendingBehavior::DoTask(Scheduler* pScheduler)
{
    EE_ASSERT(m_spExecutingEntity);
    EE_UNUSED_ARG(pScheduler);

    if (m_spExecutingEntity->StartBehavior(this))
    {
        // If StartBehavior succeeds that should mean the scheduler has already been set into
        // this entity.  We rely on that fact in RunBehaviors so lets verify:
        EE_ASSERT(m_spExecutingEntity->GetExecutor() == pScheduler);

        m_started = true;

        bool result = RunBehaviors();

        // If I failed or finished call FinishBehavior, otherwise I should be blocked and the
        // FinishBehavior method will be called after I unblock.
        if (!result || m_behaviors.empty())
        {
            FinishBehavior(result);
        }
        else
        {
            EE_ASSERT(IsBlocked());
        }
    }
    else
    {
        // We failed to start this behavior for whatever reason.
    }
}

//------------------------------------------------------------------------------------------------
void PendingBehavior::AbortTask(Scheduler*)
{
    FinishBehavior(false);
}

//------------------------------------------------------------------------------------------------
bool PendingBehavior::BlockForReply()
{
    // Should be impossible to already be blocked:
    EE_ASSERT(m_blockedReply == false);
    EE_ASSERT(m_blockedSleep == false);
    EE_ASSERT(m_blockedDebug == false);
    EE_ASSERT(m_blockedEnityCreation == false);

    if (CanBlock())
    {
        // indicate that this behavior is now blocked for a reply.
        m_blockedReply = true;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool PendingBehavior::ResumeFromReply()
{
    // If we get here it should be because we're blocked for reply, verify:
    if (EE_VERIFY(m_blockedReply))
    {
        // If we are blocked for reply it should be impossible to be blocked for another reason:
        EE_ASSERT(m_blockedSleep == false);
        EE_ASSERT(m_blockedDebug == false);
        EE_ASSERT(m_blockedEnityCreation == false);

        // We're waking up so we are no longer blocked:
        m_blockedReply = false;

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool PendingBehavior::BlockForSleep()
{
    // Should be impossible to already be blocked:
    EE_ASSERT(m_blockedSleep == false);
    EE_ASSERT(m_blockedReply == false);
    EE_ASSERT(m_blockedDebug == false);
    EE_ASSERT(m_blockedEnityCreation == false);

    if (CanBlock())
    {
        // indicate that this behavior is now blocked for a sleep.
        m_blockedSleep = true;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool PendingBehavior::ResumeFromSleep()
{
    // If we get here it should be because we're blocked for sleep, verify:
    if (EE_VERIFY(m_blockedSleep))
    {
        // If we are blocked for sleep it should be impossible to be blocked for another reason:
        EE_ASSERT(m_blockedReply == false);
        EE_ASSERT(m_blockedDebug == false);
        EE_ASSERT(m_blockedEnityCreation == false);

        // We're waking up so we are no longer blocked:
        m_blockedSleep = false;

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool PendingBehavior::BlockForDebug()
{
    // Should be impossible to already be blocked:
    EE_ASSERT(m_blockedSleep == false);
    EE_ASSERT(m_blockedReply == false);
    EE_ASSERT(m_blockedDebug == false);
    EE_ASSERT(m_blockedEnityCreation == false);

    // Note: "NoBlock" behaviors are allowed to block for debugging unless they are immediate,
    // so instead of asking CanBlock we check the Immediate flag:
    if (!m_immediate)
    {
        // indicate that this behavior is now blocked for debugging.
        m_blockedDebug = true;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool PendingBehavior::ResumeFromDebug()
{
    // If we get here it should be because we're blocked for sleep, verify:
    if (EE_VERIFY(m_blockedDebug))
    {
        // If we are blocked for sleep it should be impossible to be blocked for another reason:
        EE_ASSERT(m_blockedReply == false);
        EE_ASSERT(m_blockedSleep == false);
        EE_ASSERT(m_blockedEnityCreation == false);

        // We're waking up so we are no longer blocked:
        m_blockedDebug = false;

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool PendingBehavior::BlockForEntityCreation()
{
    // Should be impossible to already be blocked:
    EE_ASSERT(m_blockedSleep == false);
    EE_ASSERT(m_blockedReply == false);
    EE_ASSERT(m_blockedDebug == false);
    EE_ASSERT(m_blockedEnityCreation == false);

    if (CanBlock())
    {
        // indicate that this behavior is now blocked for an entity creation.
        m_blockedEnityCreation = true;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool PendingBehavior::ResumeFromEntityCreation()
{
    // If we get here it should be because we're blocked for sleep, verify:
    if (EE_VERIFY(m_blockedEnityCreation))
    {
        // If we are blocked for sleep it should be impossible to be blocked for another reason:
        EE_ASSERT(m_blockedReply == false);
        EE_ASSERT(m_blockedDebug == false);
        EE_ASSERT(m_blockedSleep == false);

        // We're waking up so we are no longer blocked:
        m_blockedEnityCreation = false;

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void PendingBehavior::FinishBehavior(bool success)
{
    EE_ASSERT(m_spExecutingEntity);
    if (success)
    {
        bool result = true;

        // The current behavior finished, but there might be more behaviors as part of this event.
        // Pop off the completed behavior and call RunBehaviors again.
        if (!m_behaviors.empty())
        {
            m_behaviors.pop_front();
            result = RunBehaviors();
        }

        // This next run might have failed, blocked, or finished.  If I failed or finished then
        // I need to call FinishEvent, otherwise this method will get called again when the
        // now blocked behavior resumes (or is canceled).
        if (!result || m_behaviors.empty())
        {
            m_spExecutingEntity->FinishEvent(this, result);
        }
    }
    else
    {
        // Failing any behavior means we failed the event.
        m_spExecutingEntity->FinishEvent(this, success);
    }
}

//------------------------------------------------------------------------------------------------
bool PendingBehavior::GetBehaviorCallList(const BehaviorDescriptor* pBehavior)
{
    // Abstract behaviors should have never made it this far, they are blocked in
    // Entity::QueuePendingBehavior before we even generate a PendingBehavior object.
    EE_ASSERT(BehaviorType_Abstract != pBehavior->GetType());

    // behavior queuing rule:
    // IF the behavior descriptor list is empty THEN
    //   queue the descriptor's behavior
    // ELSE
    //   iterate the descriptor's behavior invocation order list, queuing the behaviors within
    //   the models found in list in the order they're found.
    const efd::list<efd::utf8string>& behaviorInvocationOrderList =
        pBehavior->GetInvocationOrderedModelNames();

    if (behaviorInvocationOrderList.empty())
    {
        // If I am virtual I must have an invocation order set so I shouldn't get here
        EE_ASSERT(BehaviorType_Virtual != pBehavior->GetType());

        // Not an extends behavior, just call the source behavior directly:
        BehaviorDescriptorPtr spBehavior(const_cast<BehaviorDescriptor*>(pBehavior));
        m_behaviors.push_back(spBehavior);
        if (false == pBehavior->GetTrait(egf::BehaviorTrait_NoBlock))
        {
            m_canBlock = true;
        }
    }
    else
    {
        const utf8string& eventName = pBehavior->GetName();

        // iterate list of model names in behavior's invocation order list
        for (efd::list<efd::utf8string>::const_iterator itr = behaviorInvocationOrderList.begin();
              itr != behaviorInvocationOrderList.end();
              ++itr)
        {
            // get behavior descriptor of iterated model
            efd::utf8string modelName = *itr;

            const BehaviorDescriptor* pBehaviorDescriptor =
                m_spExecutingEntity->GetModel()->GetMixinBehaviorDescriptor(modelName, eventName);
            if (pBehaviorDescriptor == 0)
            {
                // If you encounter this error, the behavior descriptor may have been removed
                // during parsing due to the "serverexeconly" or "clientexeconly" trait being
                // set, and being on the wrong prog type.
                EE_LOG(efd::kBehavior, efd::ILogger::kERR2,
                    ("Behavior descriptor not found for model '%s' in behavior invocation order "
                    "list of model '%s' for behavior '%s'",
                    modelName.c_str(),
                    m_spExecutingEntity->GetModelName().c_str(),
                    eventName.c_str()));
            }
            else
            {
                // We should only have real behaviors listed in the invocation list
                EE_ASSERT(BehaviorType_Virtual != pBehaviorDescriptor->GetType());
                EE_ASSERT(BehaviorType_Abstract != pBehaviorDescriptor->GetType());

                if (false == pBehaviorDescriptor->GetTrait(egf::BehaviorTrait_NoBlock))
                {
                    m_canBlock = true;
                }
                BehaviorDescriptorPtr spBehaviorDescriptor(
                    const_cast<BehaviorDescriptor*>(pBehaviorDescriptor));
                m_behaviors.push_back(spBehaviorDescriptor);
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool PendingBehavior::RunBehaviors()
{
    bool result = true;

    g_bapiContext.PushBehavior(this);

    while (!m_behaviors.empty())
    {
        // if here, have another pending behavior to start up...
        BehaviorDescriptor* pBehavior = m_behaviors.front();
        EE_ASSERT(pBehavior);
        if (!pBehavior->Initialize())
        {
            EE_LOG(efd::kBehavior, efd::ILogger::kERR1,
                ("Error: failed to prepare behavior '%s::%s' for execution, skipping.",
                pBehavior->GetModelName().c_str(), pBehavior->GetName().c_str()));
            result = false;
        }
        else
        {
            EE_LOG(efd::kBehavior, efd::ILogger::kLVL3,
                ("Behavior '%s::%s' (type=%s) invoked...",
                pBehavior->GetModelName().c_str(),
                pBehavior->GetName().c_str(),
                pBehavior->GetTypeName()));

            // Grab the current time
            efd::TimeType behaviorStartTime = efd::GetCurrentTimeInSec();
            StartBehaviorTimer(this);

            // now determine which kind of behavior we are executing...
            switch (pBehavior->GetType())
            {
            case BehaviorType_Abstract:
                // No-op for Abstract behaviors.
                break;
            case BehaviorType_C:
            case BehaviorType_Cpp:
                // if here, launching a "C" or C++ behavior...
                (pBehavior->GetCFunctionPtr())(m_spExecutingEntity, m_spArgs);
                break;

            case BehaviorType_Builtin:
                {
                    IBuiltinModel* pComp =
                        m_spExecutingEntity->FindBuiltinModel(pBehavior->GetModelName());
                    if (pComp)
                    {
                        if (!pComp->Dispatch(pBehavior, m_spArgs))
                        {
                            EE_LOG(efd::kBehavior, efd::ILogger::kERR1,
                                ("Behavior '%s' failed on built-in model '%s' (Dispatch error).",
                                pBehavior->GetName().c_str(),
                                pBehavior->GetModelName().c_str()));
                            result = false;
                        }
                    }
                    else
                    {
                        EE_LOG(efd::kBehavior, efd::ILogger::kERR1,
                            ("Builtin behavior '%s::%s' failed: Builtin '%s' does not exist on "
                            "entity %s of type '%s'.",
                            pBehavior->GetModelName().c_str(),
                            pBehavior->GetName().c_str(),
                            pBehavior->GetModelName().c_str(),
                            m_spExecutingEntity->GetEntityID().ToString().c_str(),
                            m_spExecutingEntity->GetModelName().c_str()));
                        result = false;
                    }
                }
                break;

            case BehaviorType_Python:
            case BehaviorType_Lua:
            default:
                // if here, launching a scripted behavior...
                if (!m_spExecutingEntity->GetExecutor()->DoScriptBehavior(this))
                {
                    EE_LOG(efd::kBehavior, efd::ILogger::kERR1,
                        ("%s Script '%s!%s' failed on entity %s of type '%s'.",
                        pBehavior->GetTypeName(),
                        pBehavior->GetModelName().c_str(),
                        pBehavior->GetName().c_str(),
                        m_spExecutingEntity->GetEntityID().ToString().c_str(),
                        m_spExecutingEntity->GetModelName().c_str()));
                    result = false;
                }
                break;

            case BehaviorType_Invalid:
            case BehaviorType_Remote:
            case BehaviorType_Virtual:
                EE_FAIL_MESSAGE(
                    ("Illegal behavior type (%d) when attempting to launch behavior '%s:%s'",
                    pBehavior->GetType(),
                    pBehavior->GetTypeName(),
                    pBehavior->GetModelName().c_str()));
                result = false;
                break;
            }

            efd::TimeType behaviorExecTime = 1000.0 * (efd::GetCurrentTimeInSec() - behaviorStartTime);

            if (behaviorExecTime > kMaxAcceptableBehaviorTime)
            {
                EE_LOG(efd::kBehavior, efd::ILogger::kLVL3,
                    ("Slow Behavior: @[%s, %s!%s, %.3fms@]",
                    pBehavior->GetTypeName(),
                    pBehavior->GetModelName().c_str(),
                    pBehavior->GetName().c_str(),
                    behaviorExecTime));
            }

            METRICS_ONLY(const char* pszBehaviorType = pBehavior->GetTypeName());

            // increment another behavior executed by code type and by behavior name
            EE_LOG_METRIC_COUNT_FMT(kBehavior, ("RUN.TYPE.%s", pszBehaviorType));
            EE_LOG_METRIC_COUNT_FMT(kBehavior, ("RUN.BEHAVIOR.%s.%s",
                pBehavior->GetModelName().c_str(),
                pBehavior->GetName().c_str()));

        }

        // If I failed there is no need to continue running this event:
        if (!result)
        {
            break;
        }

        // If I am blocked but not failed then I need to leave the behavior list alone
        else if (IsBlocked())
        {
            break;
        }

        // If I didn't fail and I'm not blocked then I finished the first behavior in the
        // list so pop it off and go on to the next one.
        else
        {
            m_behaviors.pop_front();
        }
    }

    g_bapiContext.PopBehavior(this);

    return result;
}

//------------------------------------------------------------------------------------------------
void PendingBehavior::LeakDump(void* pMem, char* o_buffer, unsigned int i_cchBuffer)
{
    PendingBehavior* ppd = reinterpret_cast<PendingBehavior*>(pMem);
    Entity* pEntity = ppd->GetScriptEntity();
    const BehaviorDescriptor* pbd = NULL;
    if (!ppd->m_behaviors.empty())
    {
        pbd = ppd->GetBehaviorDescriptor();
    }

    efd::Snprintf(o_buffer, i_cchBuffer, EE_TRUNCATE,
        "PendingBehavior<%s '%s' %s:%s>",
        pEntity ? pEntity->GetEntityID().ToString().c_str() : "NoEntity",
        pEntity ? pEntity->GetModelName().c_str() : "n/a",
        pbd ? pbd->GetModelName().c_str() : "unknown",
        pbd ? pbd->GetName().c_str() : "unknown");
}
