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

#include <efd/ServiceManager.h>
#include <efd/IConfigManager.h>
#include <efd/Metrics.h>
#include <efd/ILogger.h>
#include <efd/BitUtils.h>
#include <efd/StdContainers.h>
#include <efd/ILogger.h>
#include <efd/MessageService.h>
#include <efd/efdLogIDs.h>
#include <efd/DependencyGraph.h>
#include <efd/IFrameEvent.h>
#include <efd/Profiler.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ServiceManager);
EE_PROFILER_CONTEXT(Foundation);
//------------------------------------------------------------------------------------------------
const /*static*/ char* ServiceManager::kConfigFramerate = "ServiceManager";
const /*static*/ char* ServiceManager::kSleepKey = "Sleep";
const /*static*/ char* ServiceManager::kDeactivatedSleepKey = "DeactivatedSleep";
/*static*/ bool ServiceManager::ms_bGlobalShutdownRequested = false;

// no need to catch ctrl-C on PS3
#if !defined(EE_PLATFORM_PS3)
#include <signal.h>
#endif

//------------------------------------------------------------------------------------------------
void ServiceManager::HandleSignal(efd::SInt32 sig)
{
    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL0,
        ("Caught Signal: %d, shutting down framework",
        sig));
    EE_UNUSED_ARG(sig);

    // flush any outstanding log messages.
    efd::GetLogger()->Flush();

    ServiceManager::ms_bGlobalShutdownRequested = true;
}

//------------------------------------------------------------------------------------------------
// This should be called after any Scripting Runtimes have
void ServiceManager::RegisterSignalHandlers()
{
    // no need to catch ctrl-C on PS3
#if !defined(EE_PLATFORM_PS3)
    signal(SIGINT, ServiceManager::HandleSignal);
#if defined(EE_PLATFORM_WIN32) || defined(EE_PLATFORM_XBOX360)
    signal(SIGBREAK, ServiceManager::HandleSignal);
#elif defined(EE_PLATFORM_LINUX)
    signal(SIGQUIT, ServiceManager::HandleSignal);
#endif //defined(EE_PLATFORM_WIN32) || defined(EE_PLATFORM_XBOX360)
#endif //!defined(EE_PLATFORM_PS3)
}


//------------------------------------------------------------------------------------------------
void ServiceManager::ConfigureFramerate()
{
    // Try to find the configuration manager
    IConfigManagerPtr spConfigManager = GetSystemServiceAs<IConfigManager>();

    if (spConfigManager)
    {
        // Find the logging section in the configuration file
        const ISection *pFramerateSection =
            spConfigManager->GetConfiguration()->FindSection(kConfigFramerate);

        if (pFramerateSection)
        {
            // Read the sleep times if they exist
            efd::utf8string val;
            if (pFramerateSection->FindValue(kSleepKey, val))
            {
                if (!val.empty())
                    m_sleepTime = atoi(val.c_str());
            }
            if (pFramerateSection->FindValue(kDeactivatedSleepKey, val))
            {
                if (!val.empty())
                    m_deactivatedSleepTime = atoi(val.c_str());
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
ServiceManager::ServiceManager()
: m_threadState(kSysServState_PreInit)
, m_bShutdownRequested(false)
, m_programTypes(kProgType_Invalid)
, m_nextTimingDump(0)
, m_dependenciesChanged(false)
, m_aliasesAllowed(false)
, m_sleepTime(0)
, m_deactivatedSleepTime(100)
, m_useDeactivatedSleepTime(false)
, m_kTimingDumpInterval(20)
, m_kMaxAcceptableTickTime(100) // Dump a log if a frame takes > 100 ms
, m_bLogPeriodicServiceTiming(true)
, m_inRunOnce(false)
, m_virtualProcessID(kNetID_Unassigned)
{
    efd::Profiler::Initialize();

    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL1,
        ("Creating a ServiceManager instance 0x%08X", this));

    // By default, all timing logs are active:
    SetServiceTimeStatTypes(SERVICETIME_LOG | SERVICETIME_ACCUMULATE);

    m_spStartTickMsg = EE_NEW efd::MessageWrapper<efd::IMessage, kMSGID_StartTick >;
}


//------------------------------------------------------------------------------------------------
/*virtual*/ ServiceManager::~ServiceManager()
{
    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL1,
        ("Destroying a ServiceManager instance 0x%08X", this));

    m_spStartTickMsg = NULL;

    m_pendingUnregisterList.clear();
    m_preInitList.clear();
    m_initList.clear();
    m_tickList.clear();
    m_shutdownList.clear();
    m_servicePriorityList.clear();
    m_SysSerMap.clear();

    // empty out the clocks list.
    for (ClockList::iterator iter = m_clocks.begin();
        iter != m_clocks.end();
        ++iter)
    {
        IClock* pClock = iter->second;
        EE_DELETE pClock;
    }
    m_clocks.clear();

    efd::Profiler::Shutdown();
}

//------------------------------------------------------------------------------------------------
AsyncResult ServiceManager::RegisterSystemService(ISystemService* pService, efd::SInt32 iPriority)
{
    if (iPriority == kUseDefaultPriority)
    {
        iPriority = pService->GetDefaultPriority();
    }
    return RegisterSystemService(pService, pService->GetClassID(), iPriority, true);
}

//------------------------------------------------------------------------------------------------
AsyncResult ServiceManager::RegisterAlternateSystemService(
    ISystemService* pService,
    SSID idSysSer,
    efd::SInt32 iPriority)
{
    if (iPriority == kUseDefaultPriority)
    {
        iPriority = pService->GetDefaultPriority();
    }
    return RegisterSystemService(pService, idSysSer, iPriority, false);
}

//------------------------------------------------------------------------------------------------
AsyncResult ServiceManager::RegisterSystemService(
    ISystemService *pSysSer,
    SSID idSysSer,
    int iPriority,
    bool allowAliases)
{
    // If you hit this assert it means that you did not declare your ISystemService CLASS_ID with
    // the EE_DECLARE_SYSTEMSERVICE_ID macro and in the required range.
    EE_ASSERT_SERVICEID_RANGE(idSysSer);

    // Check for valid system service information
    if (!pSysSer || (idSysSer == INVALID_SSID) || (idSysSer == efd::kCLASSID_ISystemService))
    {
        EE_LOG(efd::kServiceManager, efd::ILogger::kERR1,
            ("%s - Failed to register system service %i priority %i",
            !pSysSer ? "NULL pointer" : "Invalid SSID",
            idSysSer,
            iPriority));
        return AsyncResult_Failure;
    }

    if (m_threadState >= kSysServState_ShuttingDown)
    {
        EE_LOG(efd::kServiceManager, efd::ILogger::kERR1,
            ("Service '%s' 0x%08X cannot be registered because the thread is shutting down.",
            pSysSer->GetDisplayName(),
            idSysSer));
        return AsyncResult_Failure;
    }

    ISystemService* pExistingService = GetSystemService(idSysSer);
    if (NULL != pExistingService)
    {
        EE_LOG(efd::kServiceManager, efd::ILogger::kERR1,
            ("Service '%s' 0x%08X cannot be registered due to conflict with service '%s' 0x%08X.",
            pSysSer->GetDisplayName(),
            idSysSer,
            pExistingService->GetDisplayName(),
            pExistingService->GetClassID()));
        return AsyncResult_Failure;
    }

    // We use m_spCurrentService to remember which service is the target for any callbacks made
    // during OnServiceRegistered.
    m_spCurrentService = EE_NEW ServiceInfo;
    m_spCurrentService->m_idSysSer = idSysSer;
    m_spCurrentService->m_state = kSysServState_PreInit;
    m_spCurrentService->m_priority = iPriority;
    m_spCurrentService->m_spSysSer = pSysSer;

    // remember whether aliases are allowed so we can ignore callbacks during OnServiceRegistered
    // if needed.
    m_aliasesAllowed = allowAliases;

    pSysSer->SetServiceManager(this);

    // Add service to lookup map, this allows the service to be found and prevents registration of
    // a duplicate service. We will not be cleared from the m_SysSerMap map until the service is
    // completely shut down.
    m_SysSerMap[idSysSer] = m_spCurrentService;

    // OnServiceRegistered will make callbacks to register any aliases. Registered aliases are
    // immediately placed into the m_SysSerMap during those callbacks. It can also register
    // service dependencies which are resolved later when services are ticked.
    pSysSer->OnServiceRegistered(this);

    efd::AsyncResult ar = RegisterSystemService(m_spCurrentService);
    if (ar == AsyncResult_Failure)
    {
        RemoveAliases(m_spCurrentService);
    }

    m_spCurrentService = NULL;
    return ar;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::UnregisterSystemService(ISystemService *pSysSer)
{
    // Check for valid system service information
    if (!pSysSer)
    {
        EE_LOG(efd::kServiceManager, efd::ILogger::kERR1,
            ("NULL pointer - Failed to unregister system service"));
        return;
    }

    // First we check for the service in the active services list.  We must check by pointer only,
    // we cannot reply on asking the service for its ID due to odd cases where a service registers
    // with non-default IDs.
    for (SystemServiceList::iterator iter = m_servicePriorityList.begin();
        iter != m_servicePriorityList.end();
        ++iter)
    {
        if ((*iter)->m_spSysSer == pSysSer)
        {
            ServiceInfo* pServiceToUnregister = *iter;
            EE_LOG(efd::kServiceManager, efd::ILogger::kLVL0,
                ("Scheduling to unregister system service '%s' 0x%08X @ 0x%p",
                pServiceToUnregister->m_spSysSer->GetDisplayName(),
                pServiceToUnregister->m_idSysSer,
                pServiceToUnregister->m_spSysSer.data()));

            // We remove all aliases for this service right away even though we delay the actual
            // shutdown and removal from the tick-list.
            RemoveAliases(pServiceToUnregister);

            // Add it to the list of services to unregister. Delay actual removal since we may be
            // iterating over the various service lists that get updated during unregister.
            m_pendingUnregisterList.push_back(pServiceToUnregister);
            return;
        }
    }

    EE_FAIL_MESSAGE(("Request to unregister unknown service '%s' 0x%p",
        pSysSer->GetDisplayName(),
        pSysSer));
}

//------------------------------------------------------------------------------------------------
/*virtual*/ void ServiceManager::UnregisterSystemService(SSID idSysSer)
{
    // Check for valid system service information
    if (idSysSer == INVALID_SSID)
    {
        EE_LOG(efd::kServiceManager, efd::ILogger::kERR1,
            ("Invalid SSID - Failed to unregister system service"));
        return;
    }

    // First we check for the service in the active services map.  This map will be immediately
    // cleared for all aliases of the service down below, so if you try to unregister the same
    // service twice the second call won't find an active service.
    SystemServiceMap::iterator itor = m_SysSerMap.find(idSysSer);
    if (itor != m_SysSerMap.end())
    {
        ServiceInfo* pServiceToUnregister = itor->second;
        EE_LOG(efd::kServiceManager, efd::ILogger::kLVL0,
            ("Scheduling to unregister system service '%s' 0x%08X @ 0x%p",
            pServiceToUnregister->m_spSysSer->GetDisplayName(),
            pServiceToUnregister->m_idSysSer,
            pServiceToUnregister->m_spSysSer.data()));

        // We remove all aliases for this service right away even though we delay the actual
        // shutdown and removal from the tick-list.
        RemoveAliases(pServiceToUnregister);

        // Add it to the list of services to unregister. Delay actual removal since we may be
        // iterating over the various service lists that get updated during unregister.
        m_pendingUnregisterList.push_back(pServiceToUnregister);
    }
    else
    {
        EE_FAIL_MESSAGE(("Cannot unregister system service 0x%08X, service not found", idSysSer));
    }
}

//------------------------------------------------------------------------------------------------
ISystemService* ServiceManager::GetSystemService(SSID idSysSer) const
{
    // Lookup the SystemService by SSID
    SystemServiceMap::const_iterator itor = m_SysSerMap.find(idSysSer);
    if (itor != m_SysSerMap.end())
    {
        return itor->second->m_spSysSer;
    }

    // NOTE: This is not necessarily an error condition, this method is used to check for the
    // existence of optional services.
    return NULL;
}

//------------------------------------------------------------------------------------------------
ISystemService* ServiceManager::GetSystemServiceByName(const char* name) const
{
    for (SystemServiceList::const_iterator iterSysSer = m_servicePriorityList.begin();
        m_servicePriorityList.end() != iterSysSer;
        ++iterSysSer)
    {
        ISystemService* pService = (*iterSysSer)->m_spSysSer;
        EE_ASSERT(pService);
        if (0 == strcmp(name, pService->GetDisplayName()))
        {
            return pService;
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
ServiceManager::ServiceState ServiceManager::CheckSystemServiceState(SSID idSysSer) const
{
    SystemServiceMap::const_iterator iter = m_SysSerMap.find(idSysSer);
    if (iter != m_SysSerMap.end())
    {
        // If the service was found, return it's state
        return iter->second->m_state;
    }

    return kSysServState_Invalid;
}

//------------------------------------------------------------------------------------------------
SInt32 ServiceManager::CheckSystemServicePriority(SSID idSysSer) const
{
    SystemServiceMap::const_iterator iter = m_SysSerMap.find(idSysSer);
    if (iter != m_SysSerMap.end())
    {
        // If the service was found, return it's priority
        return iter->second->m_priority;
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::RegisterClock(IClock* pClock)
{
    EE_ASSERT(pClock);
    ClockList::const_iterator iter = m_clocks.find(pClock->GetClockClassID());
    if (iter != m_clocks.end())
    {
        return false;
    }

    m_clocks[pClock->GetClockClassID()] = pClock;
    return true;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::UnregisterClock(IClock* pClock)
{
    EE_ASSERT(pClock);
    ClockList::iterator iter = m_clocks.find(pClock->GetClockClassID());
    if (iter == m_clocks.end() || iter->second != pClock)
    {
        return false;
    }
    m_clocks.erase(iter);
    return true;
}

//------------------------------------------------------------------------------------------------
TimeType ServiceManager::GetTime(efd::ClassID id) const
{
    IClock* internalClock = GetClock(id);

    if (internalClock)
        return internalClock->GetCurrentTime();
    else
        return 0.0f;
}

//------------------------------------------------------------------------------------------------
IClock* ServiceManager::GetClock(efd::ClassID id) const
{
    ClockList::const_iterator iter = m_clocks.find(id);

    if (iter != m_clocks.end())
        return iter->second;

    return NULL;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::Run()
{
    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL0,
        ("Starting to run the ServiceManager 0x%08X", this));

    while (m_threadState != kSysServState_Complete)
    {
        // Tick all the services once
        RunOnce();

        // Yield some CPU to other applications, if desired
        // A sleep time of kNoSleep means don't sleep at all
        if (m_useDeactivatedSleepTime)
        {
            if (m_deactivatedSleepTime != kNoSleep)
                efd::Sleep(m_deactivatedSleepTime);
        }
        else
        {
            if (m_sleepTime != kNoSleep)
                efd::Sleep(m_sleepTime);
        }
    }

    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL0,
        ("Finished running the ServiceManager 0x%08X", this));
}

//------------------------------------------------------------------------------------------------
/*virtual*/ void ServiceManager::RunOnce()
{
    efd::Profiler::Tick();

    // ServiceManager is not re-entrant, so make sure we don't call RunOnce from RunOnce
    EE_ASSERT(!m_inRunOnce);
    m_inRunOnce = true;

    // ServiceManager pClock is updated once per main loop:
    m_clock.Update();

    // Update the internal clocks
    for (ClockList::iterator iter = m_clocks.begin();
        iter != m_clocks.end();
        ++iter)
    {
        IClock* pClock = iter->second;
        if (pClock)
        {
            pClock->Update(m_clock.GetCurrentTime());
        }
    }

    // Grab the current time
#ifndef EE_DISABLE_LOGGING
    TimeType cycleStartTime = GetServiceManagerTime();

    EE_LOG(efd::kTime, efd::ILogger::kLVL3, ("New frame at time %.4f", cycleStartTime));
#endif

    EE_ASSERT(m_threadState >= kSysServState_PreInit);

    // Check for new services to register or existing services to unregister. Do this before the
    // TickServices call so that we don't call Init on an already removed service. This may be
    // called a second time on this tick in the event that service dependencies are changed but we
    // still need to process the list here in case a service that hasn't had OnPreInit called yet
    // is unregistered.
    UnregisterSystemServices();

    // Tick all the services.  This will either call OnInit, OnTick, or OnShutdown depending
    // on the state of the ServiceManager and of all the services.
    TickServices();

#ifndef EE_DISABLE_LOGGING
    TimeType cycleEndTime = m_clock.ComputeCurrentTime();
    // compute the delta in milliseconds:
    TimeType delta = 1000.0 * (cycleEndTime - cycleStartTime);

    if (delta > m_kMaxAcceptableTickTime)
    {
        EE_LOG(efd::kServiceManager, efd::ILogger::kLVL3, ("Slow frame: %.4fms", delta));
        LogServiceTiming();
    }

    // Send the time it took to tick the service to the metrics system
    EE_LOG_METRIC(kServiceManager, "TICK.SEC", delta);
    METRICS_TICK();
#endif

    m_inRunOnce = false;
}

//------------------------------------------------------------------------------------------------
// Requests a shutdown. OnShutdown actually does the shutting down.
void ServiceManager::Shutdown(bool i_Immediate /*=false*/)
{
    if (m_bShutdownRequested == false)
        DumpAccumulatedServiceTimes();

    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL0,
        ("Signaling the ServiceManager 0x%08X to shutdown", this));

    m_bShutdownRequested = true;

    // If the shutdown was requested by someone running outside of the ServiceManager::TickServices
    // function then we should immediately go to the shutting down state.  Otherwise we will
    // wait until the current TickServices call completes before changing states.
    if (!IsRunning() && m_threadState < kSysServState_ShuttingDown)
    {
        EE_LOG(efd::kServiceManager, efd::ILogger::kLVL1,
            ("ServiceManager immediately entering the ShuttingDown state."));
        m_threadState = kSysServState_WaitingToShutdown;
    }

    // flush any outstanding log messages.
    efd::GetLogger()->Flush();

    EE_UNUSED_ARG(i_Immediate);
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::GetShutdown()
{
    return m_bShutdownRequested || ms_bGlobalShutdownRequested;
}

//------------------------------------------------------------------------------------------------
// The "real" registration happens here. The other call is just a "request"
AsyncResult ServiceManager::RegisterSystemService(ServiceInfo* pServiceInfo)
{
    // We must have a pointer to the service.
    EE_ASSERT(pServiceInfo->m_spSysSer);

    SystemServiceList::iterator iterInsertSysSer = m_servicePriorityList.end();

    // Loop through all the services to make sure there is not a duplicate service ID registered
    // already.  Also look for the priority list insertion point for this new service.
    for (SystemServiceList::iterator iterSysSer = m_servicePriorityList.begin();
        m_servicePriorityList.end() != iterSysSer;
        ++iterSysSer)
    {
        ServiceInfo* ptr = *iterSysSer;

        // Check for duplicate IDs, which is strictly verboten
        if (ptr->IsType(pServiceInfo->m_idSysSer))
        {
            EE_LOG(efd::kServiceManager, efd::ILogger::kERR0,
                ("Duplicate SSID - Failed to actually register a new system service "
                "%s (0x%08X) priority %i due to conflict with %s (0x%08X)",
                pServiceInfo->m_spSysSer->GetDisplayName(),
                pServiceInfo->m_idSysSer,
                pServiceInfo->m_priority,
                ptr->m_spSysSer->GetDisplayName(),
                ptr->m_idSysSer));

            return AsyncResult_Failure;
        }

        // If the current item is a lower priority save the iterator so we can
        // insert at that point
        if ((m_servicePriorityList.end() == iterInsertSysSer) &&
            (ptr->m_priority < pServiceInfo->m_priority))
        {
            iterInsertSysSer = iterSysSer;
        }
    }

    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL1,
        ("Actually registering a new system service %s (id=0x%08X, this=0x%08X) priority %i",
        pServiceInfo->m_spSysSer->GetDisplayName(), pServiceInfo->m_idSysSer,
        (efd::ISystemService*)pServiceInfo->m_spSysSer, pServiceInfo->m_priority));

    if (m_servicePriorityList.end() != iterInsertSysSer)
    {
        // If we found a lower priority insert the service before it
        m_servicePriorityList.insert(iterInsertSysSer, pServiceInfo);
    }
    else
    {
        // If did not find a lower priority insert it at the end of the list
        m_servicePriorityList.push_back(pServiceInfo);
    }

    // The pre-init list is simply always in the order the services are registered.
    m_preInitList.push_back(pServiceInfo);
    m_dependenciesChanged = true;

    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::UnregisterSystemServices()
{
    // Loop through the list of services to register and unregister and do it

    while (!m_pendingUnregisterList.empty())
    {
        SystemServiceList::iterator iter = m_pendingUnregisterList.begin();
        UnregisterSystemService(*iter);
        m_pendingUnregisterList.erase(iter);
    }
}

//------------------------------------------------------------------------------------------------
AsyncResult ServiceManager::UnregisterSystemService(ServiceInfo* pServiceInfo)
{
    // We only support unregistering by service pointer (we used to also support service ID but
    // that can be problematic).
    EE_ASSERT(pServiceInfo->m_spSysSer);

    switch (pServiceInfo->m_state)
    {
    default:
    case kSysServState_Invalid:
        EE_FAIL("Should be impossible.");
        // ... fall through ...
    case kSysServState_PreInit:
        // If I haven't had OnPreInit called yet then skip it and skip OnShutdown.
        m_preInitList.remove(pServiceInfo);
        m_initList.remove(pServiceInfo);
        m_tickList.remove(pServiceInfo);
        m_shutdownList.remove(pServiceInfo);
        pServiceInfo->m_state = kSysServState_Complete;
        // ... fall through ...
    case kSysServState_Complete:
        // Or if OnShutdown has finished, Unregister is done.
        m_servicePriorityList.remove(pServiceInfo);
        m_dependenciesChanged = true;
        return AsyncResult_Complete;

    case kSysServState_WaitingToInitialize:
    case kSysServState_Initializing:
        m_initList.remove(pServiceInfo);
        // ... fall through ...
    case kSysServState_WaitingToRun:
    case kSysServState_Running:
    case kSysServState_WaitingToShutdown:
        m_tickList.remove(pServiceInfo);
        // ... fall through ...
    case kSysServState_WaitingForThreadShutdown:
        // If I'm in Init or Tick then proceed with the shutdown. Go directly to ShuttingDown
        // bypassing the WaitingToShutdown state.
        EE_LOG(efd::kServiceManager, efd::ILogger::kLVL1,
            ("Begin shutdown on system service '%s' (0x%p) priority %i",
            pServiceInfo->m_spSysSer->GetDisplayName(),
            (efd::ISystemService*)(pServiceInfo->m_spSysSer),
            pServiceInfo->m_priority));
        pServiceInfo->m_state = kSysServState_ShuttingDown;
        return AsyncResult_Pending;

    case kSysServState_ShuttingDown:
        // A shutdown is already in progress, nothing else to do. This method will get called
        // again until the service completes the shutdown process.
        return AsyncResult_Pending;
    }
}

//------------------------------------------------------------------------------------------------
void ServiceManager::TickServices()
{
    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL3, ("Beginning a new tick cycle"));
    bool didWork;

    switch (m_threadState)
    {
    default:
    case kSysServState_Invalid:
    case kSysServState_Initializing:
    case kSysServState_WaitingToRun:
    case kSysServState_WaitingForThreadShutdown:
        // These states are not used for thread state, only for service states.
        EE_FAIL("Invalid thread state");
        break;

    case kSysServState_PreInit:
        ConfigureFramerate();
        RunPreInitServices();
        m_threadState = kSysServState_Running;
        break;

    case kSysServState_Running:
        // When running, we can have services initializing, ticking, or shutting down. Remember
        // that services can be registered and unregistered at run-time which makes them initialize
        // or shutdown respectively.
        RunPreInitServices();
        RunInitingServices();
        RunTickingServices();
        RunShuttingDownServices();
        break;

    case kSysServState_WaitingToShutdown:
        // move initing and/or ticking services into the waiting to shutdown state
        CancelActiveServices();
        m_threadState = kSysServState_ShuttingDown;
        // ... fall through ...
    case kSysServState_ShuttingDown:
        // We shut down services in reverse dependency order. While some services are being shut
        // down those not dependent on them continue ticking.
        didWork = RunShuttingDownServices();
        didWork = RunTickingServices() || didWork;
        if (!didWork)
        {
            // no services left running so we're complete
            m_threadState = kSysServState_Complete;
        }
        break;

    case kSysServState_Complete:
        // Anything to do here? Call OnPostShutdown on everybody.
        break;
    }

#ifndef EE_DISABLE_LOGGING
    if (m_bLogPeriodicServiceTiming && GetServiceManagerTime() > m_nextTimingDump)
    {
        LogServiceTiming();
    }
#endif

    // If a shutdown was requested this loop, process it:
    if (GetShutdown() && m_threadState < kSysServState_ShuttingDown)
    {
#ifndef EE_DISABLE_LOGGING
        EE_LOG(efd::kServiceManager, efd::ILogger::kLVL1,
            ("ServiceManager entering the ShuttingDown state."));
        LogServiceTiming();
#endif
        m_threadState = kSysServState_WaitingToShutdown;
    }
}

//------------------------------------------------------------------------------------------------
ServiceManager::RunningServiceTimer::RunningServiceTimer(
    ServiceManager& srvMgr,
    ServiceManager::ServiceInfo* pServiceInfo)
    : m_srvMgr(srvMgr)
    , m_pServiceInfo(pServiceInfo)
    , m_startTime(0.0)
{
    if (m_srvMgr.m_uiServiceTimeStatTypes)
    {
        m_startTime = m_srvMgr.m_clock.ComputeCurrentTime();
    }
}

//------------------------------------------------------------------------------------------------
ServiceManager::RunningServiceTimer::~RunningServiceTimer()
{
    if (m_srvMgr.m_uiServiceTimeStatTypes)
    {
        TimeType endTime = m_srvMgr.m_clock.ComputeCurrentTime();
        TimeType delta = endTime - m_startTime;
        m_pServiceInfo->StoreLastTickTime(delta);

        // Send the time it took to tick the service to the metrics system
        EE_LOG_METRIC_FMT(kServiceManager, ("TICK.SERVICE.SEC.%s",
            m_pServiceInfo->m_spSysSer->GetDisplayName()), delta);
    }
}

//------------------------------------------------------------------------------------------------
ServiceManager::RunningServiceTimer& ServiceManager::RunningServiceTimer::operator=(
    const ServiceManager::RunningServiceTimer& other)
{
    m_srvMgr = other.m_srvMgr;
    m_pServiceInfo = other.m_pServiceInfo;
    m_startTime = other.m_startTime;
    return *this;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::RunPreInitServices()
{
    // If the thread is in the PreInit state than we move services directly into the Initializing
    // state once OnPreInit is called, but if the thread is already in the Initializing state then
    // we instead advance into the WaitingToInitialize state so that we don't call OnPreInit and
    // OnInit both on the same frame. This is because during the running thread states we call both
    // RunPreInitServices and RunInitServices each tick (since services can be added at runtime).
    ServiceState nextState = kSysServState_Initializing;
    if (m_threadState >= kSysServState_Initializing)
    {
        nextState = kSysServState_WaitingToInitialize;
    }

    for (SystemServiceList::iterator iterSysSer = m_preInitList.begin();
        iterSysSer != m_preInitList.end();
        ++iterSysSer)
    {
        ServiceInfo* pServiceInfo = *iterSysSer;
        if (pServiceInfo->m_state == kSysServState_PreInit)
        {
            m_spCurrentService = pServiceInfo;
            RunningServiceTimer ar(*this, pServiceInfo);
            switch (pServiceInfo->m_spSysSer->OnPreInit(this))
            {
            case SyncResult_Success:
                pServiceInfo->m_state = nextState;
                break;

            case SyncResult_Failure:
                pServiceInfo->m_state = kSysServState_Complete;
                EE_LOG(efd::kServiceManager, efd::ILogger::kERR0,
                    ("OnPreInit critical failure in service %s (0x%08X), shutting down.",
                    pServiceInfo->m_spSysSer->GetDisplayName(),
                    pServiceInfo->m_idSysSer));
                Shutdown();
                break;
            }
            m_spCurrentService = NULL;
        }
    }

    if (m_dependenciesChanged)
    {
        ComputeDependencyOrder();
    }

    if (m_preInitList.empty())
    {
        return false;
    }

    m_preInitList.clear();
    return true;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::RunInitingServices()
{
    bool didWork = false;
    bool hitPendingService = false;

    for (SystemServiceList::iterator iterSysSer = m_initList.begin();
        iterSysSer != m_initList.end();
        /*do nothing - iterSysSer is either advanced or removed below*/)
    {
        bool shouldRemove = false;

        ServiceInfo* pServiceInfo = *iterSysSer;
        switch (pServiceInfo->m_state)
        {
        case kSysServState_WaitingToInitialize:
            // We just ran OnPreInit this tick, so wait until next tick to call OnInit.
            pServiceInfo->m_state = kSysServState_Initializing;
            // This counts as doing work because this service still needs to have OnInit called.
            didWork = true;
            break;

        case kSysServState_Initializing:
            // Once we hit the first pending service we don't call any more OnInit's but we do
            // finish the loop so that any WaitingToInitialize services move into the Initializing
            // state.
            if (!hitPendingService)
            {
                didWork = true;
                RunningServiceTimer ar(*this, pServiceInfo);
                switch (pServiceInfo->m_spSysSer->OnInit())
                {
                case AsyncResult_Complete:
                    pServiceInfo->m_state = kSysServState_WaitingToRun;
                    // This one is no longer initializing so remove it from the list
                    shouldRemove = true;
                    break;

                case AsyncResult_Pending:
                    // Stop calling OnInit on services at the first one that is pending
                    hitPendingService = true;
                    break;

                case AsyncResult_Failure:
                    pServiceInfo->m_state = kSysServState_Complete;
                    shouldRemove = true;
                    EE_LOG(efd::kServiceManager, efd::ILogger::kERR0,
                        ("OnInit critical failure in service %s (0x%08X), shutting down.",
                        pServiceInfo->m_spSysSer->GetDisplayName(),
                        pServiceInfo->m_idSysSer));
                    Shutdown();
                    break;
                }
            }
            break;

        default:
            // The m_initList should only have services waiting to init in it because when the list
            // was computed in ComputeDependencyOrder we filtered the services.
            EE_FAIL("Only services needing OnInit called should be in this list.");
            shouldRemove = true;
            break;
        }

        if (shouldRemove)
        {
            iterSysSer = m_initList.erase(iterSysSer);
        }
        else
        {
            ++iterSysSer;
        }
    }
    return didWork;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::RunTickingServices()
{
    EE_PROFILER_ZONE(Foundation, 0, ("Tick Services"));
    bool didWork = false;

    FrameEvents::iterator iterEvent = m_frameEvents.begin();

    /// Services initialize in dependency order:
    for (SystemServiceList::iterator iterSysSer = m_tickList.begin();
        iterSysSer != m_tickList.end();
        /*do nothing - iterSysSer is either advanced or removed below*/)
    {
        ServiceInfo* pServiceInfo = *iterSysSer;
        bool shouldRemove = false;

        // Run any frame events with a higher priority than the current service to tick.
        while (iterEvent != m_frameEvents.end())
        {
            if (iterEvent->first > pServiceInfo->m_priority)
            {
                iterEvent->second->DoEvent();
                ++iterEvent;
            }
            else
            {
                break;
            }
        }

        // Now tick the current service if it's ready for that
        switch (pServiceInfo->m_state)
        {
        case kSysServState_PreInit:
        case kSysServState_WaitingToInitialize:
        case kSysServState_Initializing:
            // Service isn't ready to tick yet, leave it here until it's ready.
            break;

        case kSysServState_WaitingToRun:
            // We just finished OnInit this tick, so wait until next tick to call OnTick:
            pServiceInfo->m_state = kSysServState_Running;
            break;

        case kSysServState_WaitingToShutdown:
            // While waiting for it's turn to shut-down services continue to tick.
            // ... fall through ...
        case kSysServState_Running:
            {
                EE_PROFILER_ZONE(Foundation, 0, (pServiceInfo->m_spSysSer->GetDisplayName()));

                didWork = true;
                RunningServiceTimer ar(*this, pServiceInfo);
                switch (pServiceInfo->m_spSysSer->OnTick())
                {
                case AsyncResult_Complete:
                    pServiceInfo->m_state = kSysServState_WaitingForThreadShutdown;
                    // This one is no longer initializing so remove it from the list
                    shouldRemove = true;
                    break;

                case AsyncResult_Pending:
                    break;

                case AsyncResult_Failure:
                    pServiceInfo->m_state = kSysServState_Complete;
                    shouldRemove = true;
                    EE_LOG(efd::kServiceManager, efd::ILogger::kERR0,
                        ("OnTick critical failure in service %s (0x%08X), shutting down.",
                        pServiceInfo->m_spSysSer->GetDisplayName(),
                        pServiceInfo->m_idSysSer));
                    Shutdown();
                    break;
                }
            }
            break;

        default:
            // The m_tickList should only have running or initing services in it because when the
            // list was computed in ComputeDependencyOrder we filtered the services.
            EE_FAIL("Only services needing OnInit called should be in this list.");
            shouldRemove = true;
            break;
        }

        if (shouldRemove)
        {
            iterSysSer = m_tickList.erase(iterSysSer);
        }
        else
        {
            ++iterSysSer;
        }
    }

    // Run any remaining frame events
    for (;iterEvent != m_frameEvents.end(); ++iterEvent)
    {
        iterEvent->second->DoEvent();
    }

    return didWork;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::RunShuttingDownServices()
{
    bool didWork = false;
    bool hitPendingService = false;

    for (SystemServiceList::iterator iterSysSer = m_shutdownList.begin();
        iterSysSer != m_shutdownList.end();
        /*do nothing - iterSysSer is either advanced or removed below*/)
    {
        bool shouldRemove = false;

        ServiceInfo* pServiceInfo = *iterSysSer;
        switch (pServiceInfo->m_state)
        {
        case kSysServState_PreInit:
        case kSysServState_WaitingToInitialize:
        case kSysServState_Initializing:
        case kSysServState_WaitingToRun:
        case kSysServState_Running:
            // Service isn't ready to shutdown yet, leave it here until it's ready. If the process
            // is in the shutting down state then there should not be any services left in these
            // states.
            EE_ASSERT(m_threadState != kSysServState_ShuttingDown);
            break;

        case kSysServState_WaitingForThreadShutdown:
            // If we are in the running state and a service finished OnTick we wait until process
            // shutdown to start service shutdown. Note: if the service is unregistered it will go
            // into the ShuttingDown state immediate and bypass the Waiting*Shutdown states.
            if (m_threadState < kSysServState_ShuttingDown)
            {
                break;
            }
            // ... fall through ...

        case kSysServState_WaitingToShutdown:
            // Process is now shutting down, proceed with this service.
            pServiceInfo->m_state = kSysServState_ShuttingDown;
            // Since OnShutdown is now being called, we no longer need to be in the tick list.
            m_tickList.remove(pServiceInfo);
            // ... fall through ...

        case kSysServState_ShuttingDown:
            {
                didWork = true;
                RunningServiceTimer ar(*this, pServiceInfo);
                switch (pServiceInfo->m_spSysSer->OnShutdown())
                {
                case AsyncResult_Complete:
                    pServiceInfo->m_state = kSysServState_Complete;
                    // This one is done so remove it from the list
                    shouldRemove = true;
                    break;

                case AsyncResult_Pending:
                    // Stop calling OnShutdown on services at the first one that is pending
                    hitPendingService = true;
                    break;

                case AsyncResult_Failure:
                    pServiceInfo->m_state = kSysServState_Complete;
                    shouldRemove = true;
                    EE_LOG(efd::kServiceManager, efd::ILogger::kERR0,
                        ("OnShutdown critical failure in service %s (0x%08X), shutting down.",
                        pServiceInfo->m_spSysSer->GetDisplayName(),
                        pServiceInfo->m_idSysSer));
                    // If we are already shutting down this call safely does nothing.
                    Shutdown();
                    break;
                }
            }
            break;

        default:
            EE_FAIL("Unexpected state");
            pServiceInfo->m_state = kSysServState_Complete;
            // ... fall through ...

        case kSysServState_Complete:
            shouldRemove = true;
            break;
        }

        if (shouldRemove)
        {
            iterSysSer = m_shutdownList.erase(iterSysSer);

            // Go ahead and remove the service from the priority list.
            m_servicePriorityList.remove(pServiceInfo);
        }
        else if (hitPendingService)
        {
            break;
        }
        else
        {
            ++iterSysSer;
        }
    }
    return didWork;
}

//------------------------------------------------------------------------------------------------
/*static*/ efd::utf8string ServiceManager::IDToString(efd::ClassID typeID)
{
    efd::utf8string ccstr(efd::Formatted, "0x%08X", typeID);
    return ccstr;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::SetProgramType(ProgramType i_pt, bool i_on)
{
    m_programTypes = BitUtils::SetBitsOnOrOff(m_programTypes, (UInt32)i_pt, i_on);
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::IsProgramType(ProgramType i_pt)
{
    return BitUtils::AllBitsAreSet(m_programTypes, (UInt32)i_pt);
}

//------------------------------------------------------------------------------------------------
void ServiceManager::SetServiceTimeStatTypes(unsigned int uiTypes)
{
    m_uiServiceTimeStatTypes = uiTypes;
#ifdef EE_ENABLE_METRICS_LOGGING
    m_uiServiceTimeStatTypes |= SERVICETIME_METRICS;
#endif
}

//------------------------------------------------------------------------------------------------
unsigned int ServiceManager::GetServiceTimeStatTypes()
{
    return m_uiServiceTimeStatTypes;
}

//------------------------------------------------------------------------------------------------
const efd::map<SSID, TimeType>& ServiceManager::GetAccumulatedServiceTimes()
{
    return m_mapAccumulatedServiceTimes;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::ResetAccumulatedServiceTimes()
{
    m_mapAccumulatedServiceTimes.clear();
}

//------------------------------------------------------------------------------------------------
TimeType ServiceManager::GetServiceLastTickTime(SSID idSysSer)
{
    SystemServiceMap::iterator itor = m_SysSerMap.find(idSysSer);
    if (itor != m_SysSerMap.end())
    {
        return itor->second->GetLastTickTime();
    }

    return 0.0f;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::LogServiceTiming()
{
    if (m_uiServiceTimeStatTypes & SERVICETIME_ACCUMULATE)
    {
        for (SystemServiceList::iterator iterSysSer = m_servicePriorityList.begin();
              iterSysSer != m_servicePriorityList.end();
              ++iterSysSer)
        {
            ServiceInfo* pServiceInfo = *iterSysSer;
            TimeType LastTickTime = pServiceInfo->GetLastTickTime();
            // We use INVALID_SSID as a slot to store the total time for all services
            // This lets us pass that value in the map since no service will have that ID
            m_mapAccumulatedServiceTimes[INVALID_SSID] += LastTickTime;
            m_mapAccumulatedServiceTimes[pServiceInfo->m_idSysSer] += LastTickTime;
        }
    }
    if (m_uiServiceTimeStatTypes & SERVICETIME_LOG)
    {
        // No sense doing all the work if the logging is disabled
        if (!efd::GetLogger()->IsLogging(efd::kServiceManager, efd::ILogger::kLVL2))
        {
            return;
        }

        efd::priority_queue<ServiceSorter> sorter;
        TimeType totalActiveTime = 0.0;

        for (SystemServiceList::iterator iterSysSer = m_servicePriorityList.begin();
              iterSysSer != m_servicePriorityList.end();
              ++iterSysSer)
        {
            ServiceInfo* pServiceInfo = *iterSysSer;
            totalActiveTime += pServiceInfo->GetLastTickTime();
            sorter.push(ServiceSorter(pServiceInfo));
        }

        utf8string timingMessage;
        timingMessage.sprintf("Service Timing:\n%20s %10s %10s %10s %10s %5s notes",
            "Service Name", "SSID", "LAT(ms)", "RAT(ms)", "LTT(ms)", "%time");
        while (!sorter.empty())
        {
            const ServiceSorter& ss = sorter.top();
            ServiceInfo& service = *ss.m_pService;

            TimeType lastTick = service.GetLastTickTime();
            TimeType recentAverage = service.ComputeRecentAverageSPF();

            utf8string notes;

            // If a single service takes more than 10ms, its slow!
            if (lastTick >= 10.0)
            {
                notes.append(" SLOW!");
            }

            // If a service is more than 50% of the total time, its a hog!
            if (lastTick > 0.5 * totalActiveTime)
            {
                notes.append(" Hog!");
            }

            // If a tick takes twice as long as average, its a spike!
            if (lastTick > 2.0 * recentAverage)
            {
                notes.append(" Spike!");
            }

            timingMessage.sprintf_append("\n%20s 0x%08X %10.6f %10.6f %10.6f %5.2f%s",
                service.m_spSysSer->GetDisplayName(), service.m_idSysSer,
                ss.m_time, recentAverage, lastTick, 100.0 * lastTick / totalActiveTime,
                notes.c_str());

            sorter.pop();
        }
        EE_LOG(efd::kServiceManager, efd::ILogger::kLVL2, ("%s", timingMessage.c_str()));
    }
    m_nextTimingDump = GetServiceManagerTime() + m_kTimingDumpInterval;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::DumpAccumulatedServiceTimes()
{
#if !defined(EE_DISABLE_LOGGING)
    // No sense doing all the work if the logging or time accumulation is disabled
    if (!(m_uiServiceTimeStatTypes & SERVICETIME_ACCUMULATE))
        return;

    if (!efd::GetLogger()->IsLogging(efd::kServiceManager, efd::ILogger::kLVL1))
    {
        return;
    }

    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL1, ("Accumulated Service Timing:"));

    // First get the total time for all services.  It's stored in the map
    // under INVALID_SSID.
    efd::TimeType totalTime = 1.0;

    efd::map<efd::SSID, efd::TimeType>::const_iterator iter =
        m_mapAccumulatedServiceTimes.find(INVALID_SSID);
    if (iter != m_mapAccumulatedServiceTimes.end())
    {
        totalTime = iter->second;
        EE_LOG(efd::kServiceManager, efd::ILogger::kLVL1,
            ("TOTAL TIME (All Services): %g", totalTime));
    }

    // Now iterate through the valid services and print each one's share.
    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL1,
        ("%20s %10s %10s %10s",
        "Service Name", "SSID", "ACC(ms)", "%time"));
    for (iter = m_mapAccumulatedServiceTimes.begin();
         iter != m_mapAccumulatedServiceTimes.end();
         ++iter)
    {
        efd::SSID ssid = iter->first;
        if (ssid != INVALID_SSID)
        {
            efd::ISystemService* pService = GetSystemService(ssid);
            const char *pcServiceName = pService ? pService->GetDisplayName() : "NULL_SERVICE_PTR";
            efd::TimeType timeType = iter->second;

            EE_LOG(efd::kServiceManager, efd::ILogger::kLVL1,
                ("%20s 0x%08X %10.6f %10.6f",
                pcServiceName, ssid, timeType, (timeType/totalTime)*100.0));
        }
    }
#endif
}

//------------------------------------------------------------------------------------------------
efd::UInt32 ServiceManager::GetVirtualProcessID() const
{
    return m_virtualProcessID;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::SetVirtualProcessID(efd::UInt32 procID)
{
    m_virtualProcessID = procID;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string ServiceManager::GetVirtualProcessName() const
{
    return m_virtualProcessName;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::SetVirtualProcessName(const efd::utf8string& procName)
{
    // it is only valid to set the virtual process name once
    EE_ASSERT(m_virtualProcessName.empty());
    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL0,
        ("*******************************************************************************"));
    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL0,
        ("* VirtualProcess: %s", procName.c_str()));
    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL0,
        ("*******************************************************************************"));
    m_virtualProcessName = procName;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::AddIdentity(SSID idSysSer)
{
    EE_ASSERT(m_spCurrentService);
    EE_ASSERT(idSysSer != efd::kInvalidClassID);
    EE_ASSERT(idSysSer != 0xFFFFFFFF);  // IBase uses -1 for it's ClassID
    EE_ASSERT(idSysSer != efd::kCLASSID_ISystemService);
    EE_ASSERT_SERVICEID_RANGE(idSysSer);

    // If an alternate service calls back to AddIdentity we simply ignore the request since
    // alternate services are not allowed to register identities.
    if (m_aliasesAllowed)
    {
        // Silently ignore any attempt to register our primary ID as an alias
        if (m_spCurrentService->m_idSysSer != idSysSer)
        {
            if (NULL != GetSystemService(idSysSer))
            {
                EE_LOG(efd::kServiceManager, efd::ILogger::kERR1,
                    ("Cannot add identity, service id 0x%08X is already registered.",
                    idSysSer));
                return false;
            }

            m_spCurrentService->m_aliases.push_back(idSysSer);
            m_SysSerMap[idSysSer] = m_spCurrentService;
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::RemoveAliases(ServiceInfo* pServiceInfo)
{
    m_SysSerMap.erase(pServiceInfo->m_idSysSer);
    for (efd::vector<SSID>::iterator iter = pServiceInfo->m_aliases.begin();
        iter != pServiceInfo->m_aliases.end();
        ++iter)
    {
        m_SysSerMap.erase(*iter);
    }
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::AddDependency(SSID idSysSer, efd::UInt32 flags)
{
    EE_ASSERT(m_spCurrentService);

    m_spCurrentService->m_dependencies.push_back(DependencyData(idSysSer, flags));
    m_dependenciesChanged = true;

    return true;
}

//------------------------------------------------------------------------------------------------
ServiceManager::ServiceInfoProxy::ServiceInfoProxy(ServiceManager::ServiceInfo* pSI)
    : m_pSI(pSI)
{
}

//------------------------------------------------------------------------------------------------
SSID ServiceManager::ServiceInfoProxy::GetClassID() const
{
    return m_pSI ? m_pSI->m_idSysSer : kInvalidClassID;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::ServiceInfoProxy::operator<(const ServiceManager::ServiceInfoProxy& rhs) const
{
    return GetClassID() < rhs.GetClassID();
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::ServiceInfoProxy::operator==(const ServiceManager::ServiceInfoProxy& rhs) const
{
    return GetClassID() == rhs.GetClassID();
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::ServiceInfoProxy::operator!=(const ServiceManager::ServiceInfoProxy& rhs) const
{
    return GetClassID() != rhs.GetClassID();
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::PrioritySortFunction(
    const ServiceManager::ServiceInfoProxy& one,
    const ServiceManager::ServiceInfoProxy& two)
{
    return one.m_pSI->m_priority > two.m_pSI->m_priority;
}

//------------------------------------------------------------------------------------------------
void ServiceManager::ComputeDependencyOrder()
{
    m_dependenciesChanged = false;

    bool failed = false;

    // The tick list is recomputed here to avoid doing this loop once per service added. The tick
    // list has the same order as the m_servicePriorityList but with completed services removed.
    m_tickList.clear();

    efd::DirectedGraph<ServiceInfoProxy> graph;
    for (SystemServiceList::iterator iterSysSer = m_servicePriorityList.begin();
        iterSysSer != m_servicePriorityList.end();
        ++iterSysSer)
    {
        ServiceInfo* ptr = *iterSysSer;
        if (ptr->m_state < kSysServState_WaitingForThreadShutdown)
        {
            m_tickList.push_back(ptr);
        }

        // In case the service has no dependencies, ensure it is still included:
        graph.AddNode(ptr);

        for (efd::vector<DependencyData>::iterator it = ptr->m_dependencies.begin();
            it != ptr->m_dependencies.end();
            ++it)
        {
            DependencyData& dd = *it;

            // We need to find the ServiceInfo structure for this service. We must use the SSID
            // from the ServiceInfo in the graph and not the dd.m_service as the dependent service
            // might be implemented by a further derived service.
            ServiceInfo* pTargetService = NULL;
            SystemServiceMap::const_iterator itor = m_SysSerMap.find(dd.m_service);
            if (itor != m_SysSerMap.end())
            {
                pTargetService = itor->second;
            }

            if (!pTargetService)
            {
                if (dd.m_flags & efd::sdf_Required)
                {
                    failed = true;
                    EE_LOG(efd::kServiceManager, efd::ILogger::kERR0,
                        ("Service %s (0x%08X) has a %sdependency on missing service 0x%08X",
                        ptr->m_spSysSer->GetDisplayName(),
                        ptr->m_idSysSer,
                        (dd.m_flags & efd::sdf_Reverse) ? "reverse " : "",
                        dd.m_service));
                    EE_FAIL_MESSAGE(
                        ("Service %s (0x%08X) has a %sdependency on missing service 0x%08X",
                        ptr->m_spSysSer->GetDisplayName(),
                        ptr->m_idSysSer,
                        (dd.m_flags & efd::sdf_Reverse) ? "reverse " : "",
                        dd.m_service));
                }
            }
            else
            {
                bool added;
                if (dd.m_flags & efd::sdf_Reverse)
                {
                    added = graph.AddDirectedEdge(pTargetService, ptr);
                }
                else
                {
                    added = graph.AddDirectedEdge(ptr, pTargetService);
                }
                if (added)
                {
                    EE_LOG(efd::kServiceManager, efd::ILogger::kLVL3,
                        ("Added %sdependency from service %s (0x%08X) to service %s (0x%08X)",
                        (dd.m_flags & efd::sdf_Reverse) ? "reverse " : "",
                        ptr->m_spSysSer->GetDisplayName(),
                        ptr->m_idSysSer,
                        pTargetService->m_spSysSer->GetDisplayName(),
                        pTargetService->m_idSysSer));
                }
                else
                {
                    // The only reason this graph should fail to add an edge is if doing so would
                    // create a cycle.
                    failed = true;
                    EE_LOG(efd::kServiceManager, efd::ILogger::kERR0,
                        ("Circual dependency detected: service %s (0x%08X) cannot have a %s"
                        "dependency on service %s (0x%08X) via SSID (0x%08X)",
                        ptr->m_spSysSer->GetDisplayName(),
                        ptr->m_idSysSer,
                        (dd.m_flags & efd::sdf_Reverse) ? "reverse " : "",
                        pTargetService->m_spSysSer->GetDisplayName(),
                        pTargetService->m_idSysSer,
                        dd.m_service));
                    EE_FAIL_MESSAGE(
                        ("Circual dependency detected: service %s (0x%08X) cannot have a %s"
                        "dependency on service %s (0x%08X) via SSID (0x%08X)",
                        ptr->m_spSysSer->GetDisplayName(),
                        ptr->m_idSysSer,
                        (dd.m_flags & efd::sdf_Reverse) ? "reverse " : "",
                        pTargetService->m_spSysSer->GetDisplayName(),
                        pTargetService->m_idSysSer,
                        dd.m_service));
                }
            }
        }
    }

    if (!failed)
    {
        efd::list<ServiceInfoProxy*> ordered;
        failed = !graph.GetTopologicalOrder(ordered, &ServiceManager::PrioritySortFunction);

        // We should have every service from the priority list in the dependency list:
        EE_ASSERT(ordered.size() == m_servicePriorityList.size());
        m_initList.clear();
        m_shutdownList.clear();

        for (efd::list<ServiceInfoProxy*>::iterator it = ordered.begin();
            it != ordered.end();
            ++it)
        {
            ServiceInfo* pSI = (*it)->m_pSI;
            switch (pSI->m_state)
            {
            case kSysServState_Invalid:
            case kSysServState_PreInit:
                // The pre-init list is simply the order in which services are added.
                // ... fall through ...

            case kSysServState_WaitingToInitialize:
            case kSysServState_Initializing:
                m_initList.push_back(pSI);
                // ... fall through ...

            case kSysServState_WaitingToRun:
            case kSysServState_Running:
                // ticking services use priority order which was applied above.
                // ... fall through ...

            case kSysServState_WaitingToShutdown:
            case kSysServState_WaitingForThreadShutdown:
            case kSysServState_ShuttingDown:
                // push new items to the front because shutdown runs in reverse dependency order.
                m_shutdownList.push_front(pSI);
                // ... fall through ...

            case kSysServState_Complete:
                break;
            }
        }
    }

    if (failed)
    {
        EE_LOG(efd::kServiceManager, efd::ILogger::kERR0,
            ("Shutting down due to service dependency failure"));
        Shutdown(true);
    }
    else
    {
#if !defined(EE_DISABLE_LOGGING)
        utf8string ordering;
        ordering.reserve(1024);
        if (!m_initList.empty())
        {
            ordering.append("Service Initialization Order:");
            for (SystemServiceList::iterator iter = m_initList.begin();
                iter != m_initList.end();
                ++iter)
            {
                ServiceInfo* pSI = *iter;
                ordering.sprintf_append(
                    "\n\t%s (0x%08X)",
                    pSI->m_spSysSer->GetDisplayName(),
                    pSI->m_idSysSer);
            }
        }
        if (!m_tickList.empty())
        {
            if (!ordering.empty())
                ordering.append("\n");
            ordering.append("Service Tick Order:");
            for (SystemServiceList::iterator iter = m_tickList.begin();
                iter != m_tickList.end();
                ++iter)
            {
                ServiceInfo* pSI = *iter;
                ordering.sprintf_append(
                    "\n\t%s (0x%08X) - %d",
                    pSI->m_spSysSer->GetDisplayName(),
                    pSI->m_idSysSer,
                    pSI->m_priority);
            }
        }
        if (!ordering.empty())
        {
            EE_LOG(efd::kServiceManager, efd::ILogger::kLVL2, ("%s", ordering.c_str()));
        }
#endif // !defined(EE_DISABLE_LOGGING)
    }
}

//------------------------------------------------------------------------------------------------
void ServiceManager::CancelActiveServices()
{
    for (SystemServiceList::iterator iterSysSer = m_servicePriorityList.begin();
        iterSysSer != m_servicePriorityList.end();
        ++iterSysSer)
    {
        ServiceInfo* pServiceInfo = *iterSysSer;
        switch (pServiceInfo->m_state)
        {
        default:
        case kSysServState_Invalid:
            EE_FAIL("Should be impossible.");
            // ... fall through ...
        case kSysServState_PreInit:
            m_preInitList.remove(pServiceInfo);
            // ... fall through ...
        case kSysServState_WaitingToInitialize:
        case kSysServState_Initializing:
            m_initList.remove(pServiceInfo);
            // If we interrupted OnInit or OnPreInit in order to enter the shutdown state then we
            // remove the entry from the Tick List and go directly to ShuttingDown to avoid ticking
            // a service that never completed initialization.
            m_tickList.remove(pServiceInfo);
            pServiceInfo->m_state = kSysServState_ShuttingDown;
            break;

        case kSysServState_WaitingToRun:
        case kSysServState_Running:
            // NOTE: items are left in the m_tickList while in the WaitingToShutdown state until
            // they come to the head of the shutdown list. RunShuttingDownServices will move
            // services from either the WaitingToShutdown or WaitingForThreadShutdown states into
            // the ShuttingDown once all services that depend on the service complete shutdown.
            pServiceInfo->m_state = kSysServState_WaitingToShutdown;
            break;

        case kSysServState_WaitingToShutdown:
            // If I was already waiting for shutdown I can simply stay in this state and
            // RunShuttingDownServices will do the right thing.
            break;

        case kSysServState_WaitingForThreadShutdown:
            // If I was waiting for the thread to shutdown, well now it is. I can simply stay in
            // this state and RunShuttingDownServices will do the right thing. Also, this service
            // should have already been removed from the tick list:
            EE_ASSERT(m_tickList.end() == m_tickList.find(pServiceInfo));
            break;

        case kSysServState_ShuttingDown:
        case kSysServState_Complete:
            break;
        }
    }

    // @todo: old code dumped service timing info when Shutdown began, do that here?
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::RegisterFrameEvent(IFrameEvent* pEvent, SInt32 priority)
{
    if (priority == kUseDefaultPriority)
    {
        priority = pEvent->GetDefaultPriority();
    }

    IFrameEvent* pExisting = GetFrameEvent(pEvent->GetName().c_str());
    if (!pExisting)
    {
        m_frameEvents.insert(FrameEventEntry(priority, pEvent));
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::AddFrameEventHandler(const char* name, IBase* pHandler, SInt32 priority)
{
    IFrameEvent* pExisting = GetFrameEvent(name);
    if (pExisting)
    {
        return pExisting->AddHandler(pHandler, priority);
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::RemoveFrameEventHandler(const char* name, IBase* pHandler)
{
    IFrameEvent* pExisting = GetFrameEvent(name);
    if (pExisting)
    {
        return pExisting->RemoveHandler(pHandler);
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::UnregisterFrameEvent(const char* name)
{
    for (FrameEvents::iterator iter = m_frameEvents.begin();
        iter != m_frameEvents.end();
        ++iter)
    {
        if (iter->second->GetName() == name)
        {
            EE_ASSERT(0 == iter->second->CountHandlers());
            m_frameEvents.erase(iter);
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------------------------
IFrameEvent* ServiceManager::GetFrameEvent(const char* name)
{
    for (FrameEvents::iterator iter = m_frameEvents.begin();
        iter != m_frameEvents.end();
        ++iter)
    {
        if (iter->second->GetName() == name)
        {
            return iter->second;
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::RegisterFrameEventAndAddHandler(
    IFrameEvent* pEvent,
    IBase* pHandler,
    SInt32 eventPriority,
    SInt32 handlerPriority)
{
    if (RegisterFrameEvent(pEvent, eventPriority))
    {
        return AddFrameEventHandler(pEvent->GetName().c_str(), pHandler, handlerPriority);
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool ServiceManager::RemoveHandlerAndUnregisterFrameEvent(const char* name, IBase* pHandler)
{
    RemoveFrameEventHandler(name, pHandler);
    return UnregisterFrameEvent(name);
}
