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

#include <egf/Scheduler.h>

#include <efd/IConfigManager.h>
#include <efd/ILogger.h>
#include <efd/Metrics.h>
#include <efd/NetMessage.h>
#include <egf/EntityIDFactory.h>
#include <egf/ScriptContext.h>
#include <egf/EntityChangeMessage.h>
#include <egf/egfLogIDs.h>
#include <egf/EntityFactoryResponse.h>
#include <egf/SimDebugger.h>

using namespace efd;
using namespace egf;


//------------------------------------------------------------------------------------------------
// config setting strings
const /*static*/ char* Scheduler::kSchedulerSection = EGF_SCHEDULER_SECTION;
const /*static*/ char* Scheduler::kMaxThread = "MaxThread";
const /*static*/ char* Scheduler::kShardID = "ShardID";
const /*static*/ char* kStatsDumpIntervalInSec = "StatsDumpIntervalInSec";

// This is how often we log a detailed statistics of the entities/behaviors execution by default.
// Can be overridden if specified in the config file as long as it fits within the allowed range
// defined by the other two consts.
const TimeType kDefaultStatsDumpIntervalInSec = 60.0;
const TimeType kMinStatsDumpIntervalInSec = 10.0;
const TimeType kMaxStatsDumpIntervalInSec = 3600.0;

//------------------------------------------------------------------------------------------------
// Scheduler class method implementations
EE_IMPLEMENT_CONCRETE_CLASS_INFO(Scheduler);

EE_HANDLER(Scheduler, HandleNetIDAssigned, AssignNetIDMessage);
EE_HANDLER(Scheduler, HandleEntityFactoryResponse, egf::EntityFactoryResponse);

//------------------------------------------------------------------------------------------------
Scheduler::Scheduler()
    : m_maxThread(256)
    , m_shardID(0)
    , m_schedulerID(0)
    , m_simOwnedCat()
    , m_bCleanEntitiesOnNextTick(false)
    , m_bIsTicking(false)
    , m_pGameClock(NULL)
    , m_startPaused(0)
    , m_destroyedEntitySet()
    , m_debuggerCallback(NULL)
    , m_scriptingRuntimes()
    , m_statsDumpIntervalInSec(kDefaultStatsDumpIntervalInSec)
    , m_numTicks(0)
    , m_numEntities(0)
    , m_numExecutingEntities(0)
    , m_numBehaviorsExecuted(0)
    , m_numTotalTicks(0)
    , m_numTotalEntities(0)
    , m_numTotalExecutingEntities(0)
    , m_numTotalBehaviorsExecuted(0)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 5500;
}

//------------------------------------------------------------------------------------------------
Scheduler::~Scheduler()
{
}

//------------------------------------------------------------------------------------------------
const char* Scheduler::GetDisplayName() const
{
    return "Scheduler";
}

//------------------------------------------------------------------------------------------------
egf::GameTimeClock* Scheduler::CreateGameTimeClock()
{
    return EE_NEW GameTimeClock(m_pServiceManager->GetServiceManagerTime());
}

//------------------------------------------------------------------------------------------------
efd::SyncResult Scheduler::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
#if !defined(EE_CONFIG_SHIPPING)
    // Review: Why exactly do we depend on RapidIterationSystemService? We don't seem to ever
    // directly use that service.
    pDependencyRegistrar->AddDependency(efd::kCLASSID_RapidIterationSystemService, sdf_Optional);
#endif

    // Because we register an IAssetFactory (via the SchedulerLua script engine that we manage) if
    // there is an AssetFactoryManager we must depend on the AssetFactoryManager service.
    pDependencyRegistrar->AddDependency<AssetFactoryManager>(sdf_Optional);

    m_pGameClock = CreateGameTimeClock();
    m_pServiceManager->RegisterClock(m_pGameClock);
    // In case multiple people requested the clock start paused we need to pause multiple times
    // since each of those people will resume the clock.
    while (m_startPaused--)
    {
        m_pGameClock->Pause();
    }

    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    if (!m_spMessageService)
    {
        EE_LOG(efd::kScheduler, efd::ILogger::kERR2,
            ("Init: Failed to find MessageService to register callback handlers!"));
        return efd::SyncResult_Failure;
    }

    m_spEntityManager = m_pServiceManager->GetSystemServiceAs<EntityManager>();
    if (!m_spEntityManager)
    {
        EE_LOG(efd::kScheduler, efd::ILogger::kERR2,
            ("Init: Failed to find EntityManager to store entities!"));
        return efd::SyncResult_Failure;
    }

    g_bapiContext.SetServiceManager(m_pServiceManager);

    m_spMessageService->Subscribe(this, kCAT_LocalMessage);

    return efd::SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult Scheduler::OnInit()
{
    InitScriptingRuntimes();

    // now read in config params for scheduler...
    IConfigManager* pConfigManager = m_pServiceManager->GetSystemServiceAs<IConfigManager>();
    if (pConfigManager)
    {
        const ISection *pSchedulerSection = pConfigManager->GetConfiguration()->FindSection(
            kSchedulerSection);

        if (pSchedulerSection)
        {
            // Find the max thread count
            const efd::utf8string& strMaxThread = pSchedulerSection->FindValue(kMaxThread);
            if (!strMaxThread.empty())
            {
                m_maxThread = atoi(strMaxThread.c_str());
                EE_LOG(efd::kScheduler, efd::ILogger::kLVL1,
                    ("Read max thread count from config.  Count=%d",
                    m_maxThread));
            }

            // Find the Shard ID
            const efd::utf8string& strShardID = pSchedulerSection->FindValue(kShardID);
            if (!strShardID.empty())
            {
                m_shardID = atoi(strShardID.c_str());
                EE_LOG(efd::kScheduler, efd::ILogger::kLVL1,
                    ("Shard ID configured as %d.", m_shardID));
            }
            else
            {
                EE_LOG(efd::kScheduler, efd::ILogger::kERR1,
                    ("Unable to read shard ID from config."));
            }

            // Find the entity/behavior statistics dump interval.
            const utf8string& strStatsDumpIntervalInSec =
                pSchedulerSection->FindValue(kStatsDumpIntervalInSec);
            if (!strStatsDumpIntervalInSec.empty())
            {
                TimeType statsDumpIntervalInSec = atof(strStatsDumpIntervalInSec.c_str());
                if (statsDumpIntervalInSec < kMinStatsDumpIntervalInSec ||
                     statsDumpIntervalInSec > kMaxStatsDumpIntervalInSec)
                {
                    EE_LOG(kScheduler, ILogger::kERR1,
                        ("Scheduler StatsDumpIntervalInSec=%.02f out of allowed range "
                         "[%.02f, %.02f], using the default (%.02f seconds)",
                         statsDumpIntervalInSec, kMinStatsDumpIntervalInSec,
                         kMaxStatsDumpIntervalInSec,
                         m_statsDumpIntervalInSec));
                }
                else
                {
                    m_statsDumpIntervalInSec = statsDumpIntervalInSec;
                    EE_LOG(kScheduler, ILogger::kLVL1,
                        ("Scheduler Stats Dump Interval configured as %.02f seconds",
                         m_statsDumpIntervalInSec));
                }
            }

        }
    }

    // a single process can be both a client and a server; if we are a server we will have a unique
    // NetID.
    m_schedulerID = m_spMessageService->GetNetID();

    // DT22018 baseID needs to be read from config/database for this scheduler (based on NetID)
    efd::UInt32 baseID = 2;

    // initialize Entity ID Factory with values from the config manager
    bool retVal = EntityIDFactory::InitIDFactory(m_shardID, m_schedulerID, baseID);
    if (!retVal)
    {
        // if entity id did not initialize, we cannot continue
        return efd::AsyncResult_Failure;
    }

    Category netServiceCat = m_spMessageService->GetServicePublicCategory(INetService::CLASS_ID);
    m_spMessageService->Subscribe(this, netServiceCat);
    m_spMessageService->Subscribe(this, GetAllSchedulersCat());

    EE_LOG(efd::kScheduler, ILogger::kLVL1,
        ("Scheduler::OnInit: Using ServiceManager Channel public=%s",
        GetAllSchedulersCat().ToString().c_str()));

    // Note: we register a separate handler for each different message we
    //  intend to process (even though the message class is the same)
    //  so that we don't have to do a switch statement in our handler --
    //  i.e. one handler per type of message (with message already properly cast)

    // Initialize the event ID creator
    EventID::InitEventID(m_shardID, m_schedulerID);

    // Initialize entity/behavior related stats variables that were not initialized at the C'tor.
    m_schedulerStartTime = m_pServiceManager->GetServiceManagerTime();
    m_lastStatsDumpTime = m_schedulerStartTime;

    return efd::AsyncResult_Complete;
}


//------------------------------------------------------------------------------------------------
void Scheduler::HandleNetIDAssigned(
    const efd::AssignNetIDMessage* pAssignNetIDMessage,
    efd::Category targetChannel)
{
    m_schedulerID = pAssignNetIDMessage->GetAssignedNetID();

    // re-initialize Entity ID Factory
    EntityIDFactory::ReinitIDFactory(m_schedulerID);

    // setup new subscription to private Category
    if (m_simOwnedCat.IsValid())
    {
        m_spMessageService->Unsubscribe(this, m_simOwnedCat);
    }
    m_simOwnedCat = m_spMessageService->GetServicePrivateCategory(Scheduler::CLASS_ID);
    m_spMessageService->Subscribe(this, m_simOwnedCat);

    EE_LOG(efd::kScheduler, ILogger::kLVL1,
        ("Scheduler::HandleNetIDAssigned: private=%s, public=%s",
        m_simOwnedCat.ToString().c_str(), GetAllSchedulersCat().ToString().c_str()));

    // re-initialize the event ID creator
    EventID::InitEventID(m_shardID, m_schedulerID);
}

//------------------------------------------------------------------------------------------------
// ISystemService virtual methods defined in scheduler class...
efd::AsyncResult Scheduler::OnTick()
{
    m_bIsTicking = true;

    efd::UInt32 cTasksProcessed = 0;
    if (!m_pGameClock->IsPaused())
    {
        // Run any tasks in our queue whose time has come
        cTasksProcessed = ProcessQueue();
    }
    else
    {
        EE_LOG(efd::kScheduler, efd::ILogger::kLVL3,
            ("Scheduler paused, not ticking entities."));
    }

    // If script is being debugged, maybe time to resume execution
    if (m_debuggerCallback != NULL)
    {
        m_debuggerCallback->DoDebuggerCallback();
    }

    ++ m_numTicks;
    m_numEntities += m_spEntityManager->GetCount();

    EE_LOG(efd::kScheduler, efd::ILogger::kLVL3,
        ("Scheduler::OnTick executed %d tasks.", cTasksProcessed));

    if (m_bCleanEntitiesOnNextTick)
    {
        CleanEntities();
        m_bCleanEntitiesOnNextTick = false;
    }

    // Log statistics if enough time has passed since the last time.
    if (m_pServiceManager->GetServiceManagerTime() - m_lastStatsDumpTime >=
        m_statsDumpIntervalInSec)
    {
        LogEntityBehaviorStats();
    }

    m_bIsTicking = false;

    // Call this outside the "m_bIsTicking" guard.  This allows potential callback handlers
    // responding to the entity destruction notification to immediately delete other entities
    // without having an additional frame of delay.
    DestroyEntitiesInternal();

    return efd::AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult Scheduler::OnShutdown()
{
    // abort all pending behaviors
    while (!m_globalQueue.empty())
    {
        ScheduledTask* pTask = m_globalQueue.top();
        // Popping the task from the queue might release the last reference, so hold one to be
        // safe.
        pTask->IncRefCount();

        // Pop before running the task.  Its perfectly valid for a running task to add additional
        // tasks to the global queue.
        m_globalQueue.pop();

        EE_LOG_METRIC_DECREMENT(kScheduler, "TASK.TALLY");

        pTask->AbortTask(this);

        // Since we grabbed a reference above we need to release it here
        pTask->DecRefCount();
    }

    for (BlockedTasksMap::iterator iter = m_blockedTasks.begin();
        iter != m_blockedTasks.end();
        ++iter)
    {
        ScheduledTask* pTask = iter->second;
        EE_ASSERT(pTask);
        pTask->AbortTask(this);
        pTask->DecRefCount();
    }

    m_blockedTasks.clear();

    if (m_spMessageService)
    {
        Category netServiceCat =
            m_spMessageService->GetServicePublicCategory(INetService::CLASS_ID);
        m_spMessageService->Unsubscribe(this, netServiceCat);
        m_spMessageService->Unsubscribe(this, GetAllSchedulersCat());
        m_spMessageService->Unsubscribe(this, kCAT_LocalMessage);
        if (m_simOwnedCat.IsValid())
        {
            m_spMessageService->Unsubscribe(this, m_simOwnedCat);
        }
        m_spMessageService = NULL;
    }

    LogEntityBehaviorStats();

    for (ScriptingRuntimeMap::iterator it = m_scriptingRuntimes.begin();
         it != m_scriptingRuntimes.end();
         ++it)
    {
        it->second->Shutdown();
    }

    // Watch these clean up by doing this explicitly. Gets scripting engines out of the way
    // before static dtor time.
    m_scriptingRuntimes.clear();
    m_scriptingRuntimesByID.clear();

    g_bapiContext.SetServiceManager(NULL);

    // Remove the game time clock
    m_pServiceManager->UnregisterClock(m_pGameClock);
    EE_DELETE m_pGameClock;
    m_pGameClock = NULL;

    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
bool Scheduler::QueueTask(ScheduledTask* pTask)
{
    // We need time to exist in order to queue a task. This prevents queuing a task before
    // OnPreInit or after OnShutdown.
    if (!m_pGameClock)
    {
        return false;
    }

    TimeType when = pTask->GetExecuteTime();

    if (when == time_Now)
    {
        pTask->SetExecuteTime(m_pGameClock->GetCurrentTime());
    }

    m_globalQueue.push(pTask);

    EE_LOG_METRIC_INCREMENT(kScheduler, "TASK.TALLY");

    return true;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 Scheduler::ProcessQueue()
{
    efd::UInt32 cTasksProcessed = 0;
    EE_LOG_METRIC(kScheduler, "TASK.TOTAL", (efd::UInt32)m_globalQueue.size());

    TimeType now = m_pGameClock->GetCurrentTime();
    while (!m_globalQueue.empty())
    {
        ScheduledTask* pTask = m_globalQueue.top();
        if (pTask->GetExecuteTime() > now)
        {
            break;
        }

        // Popping the task from the queue might release the last reference, so hold one to be
        // safe.
        pTask->IncRefCount();

        // Pop before running the task.  Its perfectly valid for a running task to add additional
        // tasks to the global queue.
        m_globalQueue.pop();

        m_numBehaviorsExecuted++;

        EE_LOG_METRIC_DECREMENT(kScheduler, "TASK.TALLY");

        pTask->DoTask(this);
        // Since we grabbed a reference above we need to release it here
        pTask->DecRefCount();
        ++cTasksProcessed;

        // Stop processing if a breakpoint has been hit
        if (IsPaused())
            break;
    }

    return cTasksProcessed;
}

//------------------------------------------------------------------------------------------------
bool Scheduler::QueueTaskOnEvent(EventID replyEvent, ScheduledTask* pTask)
{
    // We need time to exist in order to queue a task. This prevents queuing a task before
    // OnPreInit or after OnShutdown.
    if (!m_pGameClock)
    {
        return false;
    }

    EE_ASSERT(0 == m_blockedTasks.count(replyEvent));

    pTask->IncRefCount();
    m_blockedTasks[replyEvent] = pTask;

    return true;
}

//------------------------------------------------------------------------------------------------
bool Scheduler::ProcessReturnEvent(const EventMessage* pMessage)
{
    EventID replyEventID = pMessage->GetEventID();

    BlockedTasksMap::iterator iter = m_blockedTasks.find(replyEventID);
    if (iter != m_blockedTasks.end())
    {
        ScheduledTask* pTask = iter->second;
        EE_ASSERT(pTask);

        pTask->SetExecuteTime(time_Now);
        if (pTask->SetResult(this, pMessage))
        {
            QueueTask(pTask);
        }
        else
        {
            pTask->AbortTask(this);
        }

        m_blockedTasks.erase(iter);
        pTask->DecRefCount();

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool Scheduler::IsBehaviorPending(EventID id) const
{
    return NULL != m_globalQueue.find(id);
}

//------------------------------------------------------------------------------------------------
bool Scheduler::RemovePendingBehavior(EventID id)
{
    ScheduledTaskPtr spTask;
    if (m_globalQueue.erase(id, spTask))
    {
        spTask->AbortTask(this);
        return true;
    }

    BlockedTasksMap::iterator iter = m_blockedTasks.find(id);
    if (iter != m_blockedTasks.end())
    {
        ScheduledTask* pTask = iter->second;
        EE_ASSERT(pTask);
        pTask->AbortTask(this);
        m_blockedTasks.erase(iter);
        pTask->DecRefCount();
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
Entity* Scheduler::LookupEntity(const EntityID &id) const
{
    Entity* spFoundEntity = m_spEntityManager->LookupEntity(id);
    if (spFoundEntity && spFoundEntity->IsOwned())
    {
        return spFoundEntity;
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
//  Note: find searches both OWNED and REPLICATED maps!!
Entity* Scheduler::FindEntity(const EntityID &id) const
{
    return m_spEntityManager->LookupEntity(id);
}

//------------------------------------------------------------------------------------------------
efd::SInt32 Scheduler::GetMaxThread() const
{
    return m_maxThread;
}

//------------------------------------------------------------------------------------------------
void Scheduler::InitScriptingRuntimes()
{
    ScriptingRuntimeMap::iterator it = m_scriptingRuntimes.begin();
    for (; it != m_scriptingRuntimes.end(); ++it)
    {
        efd::utf8string name = (*it).first;
        ISchedulerScripting* engine = (*it).second;
        if (engine->InitForScripting(this) == 0)
        {
            EE_LOG(
                efd::kScheduler,
                efd::ILogger::kERR1,
                ("Unable to initialize scripting for '%s'", name.c_str()));
        }
        else
        {
            EE_LOG(
                efd::kScheduler,
                efd::ILogger::kLVL1,
                ("Successfully initialized scripting for '%s'", name.c_str()));
        }
    }

    if (m_scriptingRuntimes.find("Python") == m_scriptingRuntimes.end())
    {
        EE_LOG(efd::kScheduler, efd::ILogger::kLVL1,
            ("The Python runtime was not configured. Python scripting disabled."));
    }

    if (m_scriptingRuntimes.find("Lua") == m_scriptingRuntimes.end())
    {
        EE_LOG(efd::kScheduler, efd::ILogger::kLVL1,
            ("The Lua runtime was not configured. Lua scripting disabled."));
    }
}

//------------------------------------------------------------------------------------------------
void Scheduler::SubscribeEntity(Entity* pEntity, const efd::Category& cat)
{
    EE_ASSERT(pEntity);
    EE_LOG(efd::kScheduler, ILogger::kLVL1,
        ("Subscribing internal listener %s (%s) to %s",
        pEntity->GetEntityID().ToString().c_str(),
        pEntity->GetModelName().c_str(),
        cat.ToString().c_str()));

    if (m_spMessageService)
    {
        m_spMessageService->Subscribe(pEntity, cat);
    }
}

//------------------------------------------------------------------------------------------------
void Scheduler::UnsubscribeEntity(Entity* pEntity, const efd::Category& cat)
{
    EE_ASSERT(pEntity);
    EE_LOG(efd::kScheduler, ILogger::kLVL1,
        ("Unsubscribing internal listener %s from %s",
        pEntity->GetEntityID().ToString().c_str(), cat.ToString().c_str()));

    if (m_spMessageService)
    {
        m_spMessageService->Unsubscribe(pEntity, cat);
    }
}

//------------------------------------------------------------------------------------------------
void Scheduler::ProcessEventMessage(
    Entity* pEntity,
    const EventMessage* pEventMessage,
    bool isViewEvent)
{
    // Entities directly receive their own EventMessages and then pass them to this method for
    // processing.
    EE_ASSERT(pEntity);
    EE_ASSERT(pEventMessage);

    // Either a non-return event or a return event using the legacy-style handling
    switch (pEventMessage->GetClassID())
    {
    case kMSGID_Event:
        if (!pEntity->IsBehaviorInvokeValid(
            pEventMessage->GetBehaviorID(),
            this,
            true,           // we except this to succeed, log on failure
            isViewEvent))   // whether or not this is a view event effects when we log errors
        {
            EE_LOG(efd::kMessageTrace, ILogger::kLVL3,
                ("%s| Scheduler::HandleMessage !IsBehaviorInvokeValid behavior id 0x%08X, model "
                "id 0x%08X, entity %s",
                pEventMessage->GetDescription().c_str(),
                pEventMessage->GetBehaviorID(),
                pEventMessage->GetMixinModelID(),
                pEntity->GetEntityID().ToString().c_str()));
            return;
        }

        // Just in case, set this.
        pEntity->SetExecutor(this);
        // ... fall through ...

    case kMSGID_EventCancel:
        if (pEntity->PostEvent(pEventMessage))
        {
            EE_LOG(efd::kMessageTrace, efd::ILogger::kLVL2,
                ("%s| Scheduler::HandleMessage successfully posted event to %s",
                pEventMessage->GetDescription().c_str(),
                pEntity->GetEntityID().ToString().c_str()));
        }
        else
        {
            EE_LOG(efd::kMessageTrace, efd::ILogger::kLVL1,
                ("%s| Scheduler::HandleMessage failed to posted event to %s",
                pEventMessage->GetDescription().c_str(),
                pEntity->GetEntityID().ToString().c_str()));
        }
        break;

    case kMSGID_EventReturn:
        ProcessReturnEvent(pEventMessage);
        break;
    }
}

//------------------------------------------------------------------------------------------------
bool Scheduler::DestroyEntity(egf::EntityID entityID)
{
    Entity* pFoundEntity = LookupEntity(entityID);
    // Make sure the entity is in our owned list
    if (NULL == pFoundEntity)
    {
        EE_LOG(efd::kScheduler, efd::ILogger::kERR2,
            ("DestroyEntity() - Specified Entity (%s) not owned by scheduler.  "
            "Attempt to remove ignored.",
            entityID.ToString().c_str()));
        return false;
    }
    if (pFoundEntity->IsDestroyInProgress())
    {
        // we are already destroying this entity, don't try to destroy it again
        return false;
    }

    // To avoid dangerous recursions we delay destruction if we're inside of OnTick
    if (m_bIsTicking && m_destroyedEntitySet.find(pFoundEntity)==m_destroyedEntitySet.end())
    {
        m_destroyedEntitySet.insert(pFoundEntity);
    }
    else
    {
        pFoundEntity->Destroy();
    }
    return true;
}

//------------------------------------------------------------------------------------------------
bool Scheduler::DestroyEntity(egf::Entity* pEntity)
{
    EE_ASSERT(pEntity);
    Entity* pFoundEntity = LookupEntity(pEntity->GetEntityID());
    // Make sure the entity is in our owned list
    if (NULL == pFoundEntity)
    {
        EE_LOG(efd::kScheduler, efd::ILogger::kERR2,
            ("DestroyEntity() - Specified Entity (%s) not owned by scheduler.  "
            "Attempt to remove ignored.",
            pEntity->GetEntityID().ToString().c_str()));
        return false;
    }

    // To avoid dangerous recursions we delay destruction if we're inside of OnTick
    if (m_bIsTicking && m_destroyedEntitySet.find(pFoundEntity)==m_destroyedEntitySet.end())
    {
        m_destroyedEntitySet.insert(pFoundEntity);
    }
    else
    {
        pFoundEntity->Destroy();
    }
    return true;
}

//------------------------------------------------------------------------------------------------
void Scheduler::DestroyEntitiesInternal()
{
    // iterate through all the entities to be removed and remove them
    for (EntitySet::iterator iterEntity = m_destroyedEntitySet.begin();
          iterEntity != m_destroyedEntitySet.end();
          ++iterEntity)
    {
        Entity* pEntity = *iterEntity;
        if (pEntity->GetEntityManager())
        {
            pEntity->Destroy();
        }
    }
    // Clear the list of entities to be removed
    m_destroyedEntitySet.clear();
}

//------------------------------------------------------------------------------------------------
void Scheduler::PauseScheduler(bool pause)
{
    EE_ASSERT(m_pGameClock);
    if (pause)
    {
        m_pGameClock->Pause();
    }
    else
    {
        m_pGameClock->Resume();
    }
}

//------------------------------------------------------------------------------------------------
void Scheduler::DeleteAllEntities()
{
    m_bCleanEntitiesOnNextTick = true;
}

//------------------------------------------------------------------------------------------------
void Scheduler::CleanEntities()
{
    Entity* pEntity;
    EntityManager::EntityMap::const_iterator eePos = m_spEntityManager->GetFirstEntityPos();
    while (m_spEntityManager->GetNextEntity(eePos, pEntity))
    {
        if (pEntity->IsOwned())
        {
            m_spEntityManager->DestroyEntity(pEntity);
        }
    }

#ifndef EE_CONFIG_SHIPPING
    // Notify SimDebugger of the world reset
    if (SimDebugger::Instance())
        SimDebugger::Instance()->StartSession();
#endif

    IMessagePtr spMsgPtr = EE_NEW MessageWrapper< IMessage, kMSGID_SchedulerCleared >;
    MessageService* pMsg = m_pServiceManager->GetSystemServiceAs<MessageService>();
    pMsg->SendLocal(spMsgPtr);
}

//------------------------------------------------------------------------------------------------
bool Scheduler::DoScriptBehavior(PendingBehavior* pPendBehavior)
{
#if defined(DISABLE_SCRIPTING)
    EE_LOG(efd::kEntity, efd::ILogger::kERR1,
        ("Scripted Behaviors are not supported in this build configuration!"));
    EE_FAIL("Scripted Behaviors are not supported in this build configuration!");
    EE_UNUSED_ARG(pPendBehavior);
    return false;
#else
    bool rval = true;

    const BehaviorDescriptor* pBehavior = pPendBehavior->GetBehaviorDescriptor();
    ISchedulerScripting* pSchedulerScripting = GetScriptingRuntime(pBehavior->GetType());

    if (!pSchedulerScripting)
    {
        EE_LOG(
            efd::kScheduler,
            efd::ILogger::kERR1,
            ("Scripting Runtime %d not found, skipped behavior %s:%s on %s",
            pBehavior->GetType(),
            pBehavior->GetModelName().c_str(),
            pBehavior->GetName().c_str(),
            pPendBehavior->GetScriptEntity()->GetEntityID().ToString().c_str()));
        return false;
    }

    if (pSchedulerScripting->GetStatus() == ISchedulerScripting::rtstat_Ready)
    {
        rval = pSchedulerScripting->DoScriptBehavior(pPendBehavior);
    }
    else
    {
        EE_LOG(
            efd::kScheduler,
            efd::ILogger::kERR1,
            ("Scripting Runtime %d not ready, skipped behavior '%s:%s' on %s",
            pBehavior->GetType(),
            pBehavior->GetModelName().c_str(),
            pBehavior->GetName().c_str(),
            pPendBehavior->GetScriptEntity()->GetEntityID().ToString().c_str()));
    }

    return rval;
#endif // defined(DISABLE_SCRIPTING)
}

//------------------------------------------------------------------------------------------------
efd::Bool Scheduler::RegisterScriptingRuntime(
    const efd::utf8string& name,
    UInt32 ID,
    ISchedulerScripting* engine)
{
    efd::Bool rval = false;
    EE_ASSERT(engine);
    if (m_scriptingRuntimes.find(name) == m_scriptingRuntimes.end())
    {
        m_scriptingRuntimes[name] = engine;
        m_scriptingRuntimesByID[ID] = engine;
        rval = true;
    }
    return rval;
}

//------------------------------------------------------------------------------------------------
ISchedulerScripting* Scheduler::GetScriptingRuntime(const efd::utf8string& name)
{
    ISchedulerScripting* rval = NULL;
    if (m_scriptingRuntimes.find(name) != m_scriptingRuntimes.end())
    {
        rval = m_scriptingRuntimes[name];
    }
    return rval;
}

//------------------------------------------------------------------------------------------------
ISchedulerScripting* Scheduler::GetScriptingRuntime(UInt32 ID)
{
    ISchedulerScripting* rval = NULL;
    if (m_scriptingRuntimesByID.find(ID) != m_scriptingRuntimesByID.end())
    {
        rval = m_scriptingRuntimesByID[ID];
    }
    return rval;
}

//------------------------------------------------------------------------------------------------
ISchedulerScripting::RuntimeStatus Scheduler::GetRuntimeStatus(const efd::utf8string& name)
{
    ISchedulerScripting::RuntimeStatus rval = ISchedulerScripting::rtstat_Disabled;
    if (m_scriptingRuntimes.find(name) != m_scriptingRuntimes.end())
    {
        rval = m_scriptingRuntimes[name]->GetStatus();
    }
    return rval;
}

//------------------------------------------------------------------------------------------------
ISchedulerScripting::RuntimeStatus Scheduler::GetRuntimeStatus(efd::UInt32 behaviorType)
{
    ISchedulerScripting::RuntimeStatus rval = ISchedulerScripting::rtstat_Disabled;
    if (m_scriptingRuntimesByID.find(behaviorType) != m_scriptingRuntimesByID.end())
    {
        rval = m_scriptingRuntimesByID[behaviorType]->GetStatus();
    }
    return rval;
}

//------------------------------------------------------------------------------------------------
bool Scheduler::IsScriptedBehaviorType(efd::UInt32 behaviorType)
{
    switch (behaviorType)
    {
    case BehaviorType_Invalid:
    case BehaviorType_C:
    case BehaviorType_Cpp:
    case BehaviorType_Builtin:
    case BehaviorType_Remote:
    case BehaviorType_Virtual:
    case BehaviorType_Abstract:
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------------------------
void Scheduler::LogEntityBehaviorStats()
{
    if (m_numTicks == 0)
        return;

    TimeType currentTime = m_pServiceManager->GetServiceManagerTime();

    // Roll up the counters since the last stats dump to the totals since the system startup.
    m_numTotalTicks += m_numTicks;
    m_numTotalEntities += m_numEntities;
    m_numTotalBehaviorsExecuted += m_numBehaviorsExecuted;

    // Save some work if we are not going to log.
#if !defined(EE_DISABLE_LOGGING)
    if (GetLogger()->IsLogging(kScheduler, efd::ILogger::kLVL2))
    {
        // Calculate the statistics.
        TimeType intervalInSec = currentTime - m_lastStatsDumpTime;
        Float64 avgNumEntities = static_cast<efd::Float64>(m_numEntities) / static_cast<efd::Float64>(m_numTicks);
        Float64 avgNumBehaviorsExecuted = (avgNumEntities > 0) ?
            m_numBehaviorsExecuted / intervalInSec / avgNumEntities : 0;

        TimeType sysUpTimeInSec = currentTime - m_schedulerStartTime;
        Float64 avgTotalNumEntities = static_cast<efd::Float64>(m_numTotalEntities) / static_cast<efd::Float64>(m_numTotalTicks);
        Float64 avgTotalNumBehaviorsExecuted = (avgTotalNumEntities > 0) ?
            m_numTotalBehaviorsExecuted / sysUpTimeInSec / avgTotalNumEntities : 0;

//NOTE: eja: 05/13/09: this formatting seems silly, but for some reason, the data values
//were showing up differently in the "all-in-one" EE_LOG statements.

        //interval
        EE_LOG(kScheduler, ILogger::kLVL2,
            ("Entity/Behavior statistics (average for last %.2f seconds)",
            intervalInSec));

        EE_LOG(kScheduler, ILogger::kLVL2,
            ("Entities per tick: %.2f",
            avgNumEntities));

        EE_LOG(kScheduler, ILogger::kLVL2,
            ("Total Behaviors Executed: %d",
            m_numTotalBehaviorsExecuted));

        EE_LOG(kScheduler, ILogger::kLVL2,
            ("Behaviors per tick per entity: %.2f",
            avgNumBehaviorsExecuted));

        //total
        EE_LOG(kScheduler, ILogger::kLVL2,
            ("Entity/Behavior statistics (average since scheduler startup: %.2f seconds)",
            sysUpTimeInSec));

        EE_LOG(kScheduler, ILogger::kLVL2,
            ("Entities per tick: %.2f",
            avgTotalNumEntities));

        EE_LOG(kScheduler, ILogger::kLVL2,
            ("Total Behaviors Executed: %d",
            m_numTotalBehaviorsExecuted));

        EE_LOG(kScheduler, ILogger::kLVL2,
            ("Behaviors per tick per entity: = %.2f",
            avgTotalNumBehaviorsExecuted));
    }
#endif // !defined(EE_DISABLE_LOGGING)

    // Lastly, reset the per stats dump interval counters
    m_numTicks = 0;
    m_numEntities = 0;
    m_numExecutingEntities = 0;
    m_numBehaviorsExecuted = 0;
    m_lastStatsDumpTime = currentTime;
}

//------------------------------------------------------------------------------------------------
bool Scheduler::QueueTaskOnEntityCreation(const EntityID waitingFor, ScheduledTask* pTask)
{
    // We need time to exist in order to queue a task. This prevents queuing a task before
    // OnPreInit or after OnShutdown.
    if (!m_pGameClock)
    {
        return false;
    }

    EE_ASSERT(0 == m_blockCreationTasks.count(waitingFor));

    pTask->IncRefCount();
    m_blockCreationTasks[waitingFor] = pTask;

    return true;
}


//------------------------------------------------------------------------------------------------
void Scheduler::HandleEntityFactoryResponse(
    const egf::EntityFactoryResponse* pMsg,
    efd::Category)
{
    EntityID eid = pMsg->GetEntityID();

    BlockedCreationTaskMap::iterator iter = m_blockCreationTasks.find(eid);
    if (iter != m_blockCreationTasks.end())
    {
        ScheduledTask* pTask = iter->second;
        EE_ASSERT(pTask);

        pTask->SetExecuteTime(time_Now);
        pTask->SetEntityFactoryResponse(pMsg);
        QueueTask(pTask);

        m_blockCreationTasks.erase(iter);
        pTask->DecRefCount();
    }
}
