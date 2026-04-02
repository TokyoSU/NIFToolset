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

#include <efd/AssetLocatorService.h>
#include <efd/AssetFactoryManager.h>
#include <efd/AssetLocatorRequest.h>
#include <efd/AssetLoaderThreadFunctor.h>
#include <efd/ServiceManager.h>

using namespace efd;

// The map of registered asset factories.
AssetFactoryManager::AssetFactoryMap* AssetFactoryManager::ms_pAssetFactoryMap = NULL;

// Map of asset factories waiting on shutdown.
AssetFactoryManager::AssetFactoryList* AssetFactoryManager::ms_pFactoriesPendingShutdown = NULL;

// The map of asset locate requestors.
AssetFactoryManager::AssetLocatorMap* AssetFactoryManager::ms_pAssetLocatorMap = NULL;


// A critical section protecting the asset factory map.
CriticalSection AssetFactoryManager::ms_assetFactoryCritSect;

// A critical section protecting the locator handler map.
CriticalSection AssetFactoryManager::ms_locatorHandlersCritSect;

// The category on which to send messages
Category AssetFactoryManager::ms_privateCategory = Category(
    efd::UniversalID::ECU_PrivateChannel,
    kNetID_Any,
    efd::kCLASSID_AssetFactoryManager);

// An error checking variable to verify there is only one foreground instance.
volatile bool AssetFactoryManager::ms_isMasterSet = false;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(AssetFactoryManager);

//------------------------------------------------------------------------------------------------

// Handler for AssetLocatorResponse messages
EE_HANDLER_WRAP(
    AssetFactoryManager,
    AssetLocatorResponseHandler,
    AssetLocatorResponse,
    kMSGID_AssetLocatorResponse);

// Handler for AssetLoadRequest messages
EE_HANDLER(AssetFactoryManager, AssetLoadRequestHandler, AssetLoadRequest);

//------------------------------------------------------------------------------------------------
AssetFactoryManager::AssetFactoryManager(
    const size_t requestQueueSize,
    const UInt32 sleepInterval,
    AssetFactoryManager* pForeInstance)
    : m_pLoaderThreadFunctor(NULL)
    , m_pLoaderThread(NULL)
    , m_sendQueue(requestQueueSize)
    , m_pReceiveQueue(NULL)
    , m_assetLoadRequestCategory(kCAT_INVALID)
    , m_spMessageService(0)
    , m_spAssetLocatorService(0)
    , m_sleepInterval(sleepInterval)
    , m_backgroundMessageCapPerTick(10)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 900;

    if (!pForeInstance)
    {
        // Note that, technically speaking, this is not thread safe as two threads may do the
        // if statement simultaneously and then both think they are master. This is error
        // checking code only so we are content to leave the potential race condition as is.
        EE_ASSERT(!ms_isMasterSet);
        if (ms_isMasterSet)
            return;
        ms_isMasterSet = true;

        FgConstructor(requestQueueSize);
    }
    else
    {
        BgConstructor(pForeInstance);
    }
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::FgConstructor(const size_t requestQueueSize)
{
    // Create an instance of the thread procedure for the background loading thread.
    m_pLoaderThreadFunctor = EE_NEW AssetLoaderThreadFunctor(requestQueueSize, this);

    // Start the new thread.
    m_pLoaderThread =
        Thread::Create(m_pLoaderThreadFunctor, Thread::DEFAULT_STACK_SIZE, "AssetLoaderThread");

#if (EE_PLATFORM_WIN32)
    m_pLoaderThread->SetThreadAffinity(ProcessorAffinity(
        ProcessorAffinity::PROCESSOR_1,
        (efd::UInt32)ProcessorAffinity::PROCESSOR_DONT_CARE));
#elif (EE_PLATFORM_PS3)
    m_pLoaderThread->SetThreadAffinity(ProcessorAffinity(
        ProcessorAffinity::PROCESSOR_DONT_CARE,
        (efd::UInt32)ProcessorAffinity::PROCESSOR_DONT_CARE));
#elif (EE_PLATFORM_XBOX360)
    m_pLoaderThread->SetThreadAffinity(ProcessorAffinity(
        ProcessorAffinity::PROCESSOR_XENON_CORE_2_THREAD_0,
        (efd::UInt32)ProcessorAffinity::PROCESSOR_DONT_CARE));
#endif

    m_pLoaderThread->SetPriority(Thread::NORMAL);

    m_pLoaderThread->Resume();
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::BgConstructor(AssetFactoryManager* pForeInstance)
{
    m_pReceiveQueue = &(pForeInstance->m_sendQueue);
    pForeInstance->m_pReceiveQueue = &m_sendQueue;
}

//------------------------------------------------------------------------------------------------
AssetFactoryManager::~AssetFactoryManager()
{
    // If you get this assert, you've registered a factory but failed to unregister
    // it before the AssetFactoryManager was destroyed.
    EE_ASSERT(ms_pAssetFactoryMap == NULL);
    ms_locatorHandlersCritSect.Lock();
    if (ms_pAssetLocatorMap)
    {
        ms_pAssetLocatorMap->clear();
        EE_DELETE ms_pAssetLocatorMap;
    }
    ms_locatorHandlersCritSect.Unlock();
}

//------------------------------------------------------------------------------------------------
efd::SyncResult AssetFactoryManager::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    pDependencyRegistrar->AddDependency<MessageService>();
    pDependencyRegistrar->AddDependency<AssetLocatorService>();
    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult AssetFactoryManager::OnInit()
{
    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    EE_ASSERT(m_spMessageService);

    m_spAssetLocatorService = m_pServiceManager->GetSystemServiceAs<AssetLocatorService>();
    EE_ASSERT(m_spAssetLocatorService);

    m_spMessageService->Subscribe(this, MessageCategory());

    m_assetLoadRequestCategory = GenerateAssetResponseCategory();
    m_spMessageService->Subscribe(this, m_assetLoadRequestCategory);

    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult AssetFactoryManager::OnTick()
{
    if (IsFgInstance())
    {
        return FgTick();
    }
    else
    {
        return BgTick();
    }
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult AssetFactoryManager::FgTick()
{
    if (!m_pReceiveQueue)
    {
        return efd::AsyncResult_Pending;
    }

    // Process new messages
    PullMessagesFromBackground();

    // Push anything to the background that can go
    PushMessagesToBackground();

    // Process preemptive, and only process pending if we did no preemptive
    if (!ProcessPreemptiveLoads())
        ProcessPendingLoads();

    // Send out messages for things that are done in the foreground
    SendResponseMessages(false);

    return efd::AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::PullMessagesFromBackground()
{
    efd::UInt32 messageCount = 0;
    while (messageCount < m_backgroundMessageCapPerTick)
    {
        messageCount++;

        IMessageConstPtr spMessage = 0;
        if (!m_pReceiveQueue->Consume(spMessage))
            break;

        // Response from a load request. Forward to the response category.
        AssetLoadResponseConstPtr spLoadResponse = EE_DYNAMIC_CAST(AssetLoadResponse, spMessage);
        if (spLoadResponse)
        {
            m_spMessageService->SendLocal(spMessage, spLoadResponse->GetResponseCategory());

            IAssetFactory* pFactory = GetAssetFactory(spLoadResponse->GetResponseCategory());
            if (pFactory) // Factory may have been deleted during shutdown
            {
                ForwardToCallbackListeners(pFactory->GetClassID(), spLoadResponse);
            }
            continue;
        }

        // Asset locator requests from the background thread. Forward these to the ALS.
        AssetLocatorRequestConstPtr spLocatorRequest =
            EE_DYNAMIC_CAST(AssetLocatorRequest, spMessage);
        if (spLocatorRequest)
        {
            Category clientCategory = spLocatorRequest->ClientCategory();

            LocatorRequestDataPtr pRequestData = EE_NEW LocatorRequestData();
            utf8string requestURN = spLocatorRequest->GetURI();
            pRequestData->m_urn = requestURN;
            pRequestData->m_spSourceMessage = spMessage;
            pRequestData->m_spLoadRequestData = NULL;
            pRequestData->m_responseCategory = clientCategory;
            m_pendingLocatorRequests.push_back(pRequestData);

            if (!m_spAssetLocatorService->AssetLocate(requestURN, m_assetLoadRequestCategory))
            {
                // Construct a response message to send back, with no assets
                AssetLocatorResponse* pRespMessage = EE_NEW AssetLocatorResponse();
                pRespMessage->SetResponseURI(requestURN);
                pRespMessage->ClientCategory(clientCategory);
                m_sendQueue.Produce(pRespMessage);

                // Remove the thing we just added
                m_pendingLocatorRequests.pop_back();
            }

            continue;
        }
    }
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::PushMessagesToBackground()
{
    // Process preemptive requests
    LoadRequestQueue::iterator requestIter = m_preemptiveLoadRequests.begin();
    while (requestIter != m_preemptiveLoadRequests.end())
    {
        LoadRequestDataPtr spData = *requestIter;

        if (!spData->m_isForBackground)
        {
            // Jump over any foreground pending
            requestIter++;
            continue;
        }

        if (spData->m_status == LRS_LOCATING || spData->m_status == LRS_COMPLETED)
        {
            // Jump over blocked preemptive loads.
            // Or, we may have already pulled some messages from the background, but not sent
            // their responses yet. We jump over those too.
            requestIter++;
            continue;
        }

        // Not locating, and for the background
        EE_ASSERT(spData->m_status == LRS_PENDING);
        LoadRequestQueue::iterator toRemove = requestIter++;
        m_preemptiveLoadRequests.erase(toRemove);

        EE_ASSERT(EE_DYNAMIC_CAST(IMessage, spData->m_spRequest));
        m_sendQueue.Produce(EE_DYNAMIC_CAST(IMessage, spData->m_spRequest));

        spData = 0;
    }

    // Process regular requests
    requestIter = m_pendingLoadRequests.begin();
    while (requestIter != m_pendingLoadRequests.end())
    {
        LoadRequestDataPtr spData = *requestIter;

        if (!spData->m_isForBackground)
        {
            // Jump over any foreground pending
            requestIter++;
            continue;
        }

        if (spData->m_status == LRS_LOCATING)
        {
            // We are blocked on this request, so to maintain order don't send any more.
            break;
        }

        if (spData->m_status == LRS_COMPLETED)
        {
            // We may have already pulled some messages from the background, but not sent
            // their responses yet. We jump over those as their responses will be sent before
            // any later in the list.
            requestIter++;
            continue;
        }

        // Not locating, and for the background
        EE_ASSERT(spData->m_status == LRS_PENDING);
        LoadRequestQueue::iterator toRemove = requestIter++;
        m_pendingLoadRequests.erase(toRemove);

        EE_ASSERT(EE_DYNAMIC_CAST(IMessage, spData->m_spRequest));
        m_sendQueue.Produce(EE_DYNAMIC_CAST(IMessage, spData->m_spRequest));

        spData = 0;
    }
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult AssetFactoryManager::BgTick()
{
    ShutdownFactories();

    if (!m_pReceiveQueue)
    {
        return efd::AsyncResult_Pending;
    }

    // Process new messages
    PullMessagesFromForeground();

    // Process preemptive, and only process pending if we did no preemptive
    if (!ProcessPreemptiveLoads())
    {
        if (!ProcessPendingLoads())
        {
            Sleep(m_sleepInterval);
        }
    }

    // Process any messages we need to send
    SendResponseMessages(true);

    return efd::AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::PullMessagesFromForeground()
{
    // We need to lock access to the map of locate handlers. Code in this method
    // or invoked by this method must not rely on getting a lock from another thread that also
    // requires this lock, because that thread may be blocked on this lock.
    ms_locatorHandlersCritSect.Lock();

    IMessageConstPtr spMessage = 0;
    while (m_pReceiveQueue->Consume(spMessage))
    {
        // Message should have at least 1 reference.
        EE_ASSERT(spMessage->GetRefCount() != 0);

        AssetLoadRequestConstPtr spLoadRequest = EE_DYNAMIC_CAST(AssetLoadRequest, spMessage);
        if (spLoadRequest)
        {
            // Add the request to the queue
            LoadRequestData* pData = EE_NEW LoadRequestData;
            pData->m_spRequest = spLoadRequest;
            pData->m_status = LRS_PENDING;
            pData->m_isForBackground = true;

            if (spLoadRequest->GetIsPreemptive())
                m_preemptiveLoadRequests.push_back(pData);
            else
                m_pendingLoadRequests.push_back(pData);

            continue;
        }

        AssetLocatorResponseConstPtr spLocatorResponse =
            EE_DYNAMIC_CAST(AssetLocatorResponse, spMessage);
        if (spLocatorResponse)
        {
            // Send out using our limited message sending capabilities.
            AssetLocatorResponse* pResponse = EE_DYNAMIC_CAST(AssetLocatorResponse, spMessage);

            // We've received an asset locate response -- forwarded it to the waiting factory.
            Category category = pResponse->ClientCategory();

            // Check we have someone to handle the response
            if (!ms_pAssetLocatorMap)
                continue;

            AssetLocatorMap::iterator it = ms_pAssetLocatorMap->find(category);

            // It is possible during shutdown to still pull a message off for a now removed
            // factory.
            if (it == ms_pAssetLocatorMap->end())
                continue;

            IAssetFactory* waiting = it->second;

            // Should never get a response if we don't have a request
            EE_ASSERT(waiting);

            waiting->HandleAssetLocate(pResponse, category);

            continue;
        }
    }

    ms_locatorHandlersCritSect.Unlock();
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::SendResponseMessages(bool isBackground)
{
    LoadRequestQueue::iterator requestIter = m_preemptiveLoadRequests.begin();
    while (requestIter != m_preemptiveLoadRequests.end())
    {
        LoadRequestDataPtr spData = *requestIter;

        if (spData->m_status != LRS_COMPLETED)
        {
            requestIter++;
            continue;
        }

        LoadRequestQueue::iterator eraseIter = requestIter++;
        m_preemptiveLoadRequests.erase(eraseIter);

        EE_ASSERT(EE_DYNAMIC_CAST(IMessage, spData->m_spResponse));
        if (isBackground)
        {
            m_sendQueue.Produce(EE_DYNAMIC_CAST(IMessage, spData->m_spResponse));
        }
        else
        {
            m_spMessageService->SendLocal(
                spData->m_spResponse,
                spData->m_spResponse->GetResponseCategory());

            ForwardToCallbackListeners(spData->m_factoryClassID, spData->m_spResponse);
        }
    }

    requestIter = m_pendingLoadRequests.begin();
    while (requestIter != m_pendingLoadRequests.end())
    {
        LoadRequestDataPtr spData = *requestIter;

        if (spData->m_status != LRS_COMPLETED)
        {
            // Stop sending as soon as we find an incomplete, to retain in-order.
            break;
        }

        LoadRequestQueue::iterator eraseIter = requestIter++;
        m_pendingLoadRequests.erase(eraseIter);

        EE_ASSERT(EE_DYNAMIC_CAST(IMessage, spData->m_spResponse));
        if (isBackground)
        {
            m_sendQueue.Produce(EE_DYNAMIC_CAST(IMessage, spData->m_spResponse));
        }
        else
        {
            m_spMessageService->SendLocal(
                spData->m_spResponse,
                spData->m_spResponse->GetResponseCategory());

            ForwardToCallbackListeners(spData->m_factoryClassID, spData->m_spResponse);
        }
    }
}

//------------------------------------------------------------------------------------------------
bool AssetFactoryManager::ProcessPreemptiveLoads()
{
    // Process everything we can on the preemptive list
    bool bDidWork = false;
    LoadRequestQueue::iterator requestIter = m_preemptiveLoadRequests.begin();
    while (requestIter != m_preemptiveLoadRequests.end())
    {
        LoadRequestDataPtr pData = *requestIter;

        EE_ASSERT(pData->m_status != LRS_COMPLETED && pData->m_status != LRS_CANCELLED);

        if (pData->m_status == LRS_LOCATING)
        {
            requestIter++;
            continue;
        }

        Category responseCategory = pData->m_spRequest->GetResponseCategory();
        IAssetFactory* pFactory = GetAssetFactory(responseCategory);

        if (!pFactory)
        {
            // We can't find the loader for a load request. Construct a response.
            pData->m_spResponse = EE_NEW AssetLoadResponse(
                pData->m_spRequest->GetURN(),
                pData->m_spRequest->GetResponseCategory(),
                AssetLoadResponse::ALR_FactoryNotFound);
            pData->m_status = LRS_COMPLETED;

            requestIter++;

            continue;
        }

        pData->m_factoryClassID = pFactory->GetClassID();

        AssetLoadResponsePtr spLoadResponse;
        switch (pFactory->LoadAsset(this, pData->m_spRequest, spLoadResponse))
        {
        case IAssetFactory::LOAD_COMPLETE:
            /// The load has complete processing, and the response
            /// is available. The response may contain an error
            /// condition, but it is ready to send.
            pData->m_spResponse = spLoadResponse;
            pData->m_status = LRS_COMPLETED;
            requestIter++;
            bDidWork = true;
            break;

        case IAssetFactory::LOAD_RUNNING:
            /// The load is partially complete but did enough work
            /// to preclude another loader from running in the same
            /// tick.
            EE_ASSERT(!spLoadResponse);
            pData->m_status = LRS_IN_PROGRESS;
            bDidWork = true;
            break;

        case IAssetFactory::LOAD_BLOCKED:
            /// The load is blocked on something and did no work.
            EE_ASSERT(!spLoadResponse);
            pData->m_status = LRS_IN_PROGRESS;
            requestIter++;
            break;
        }
    }

    return bDidWork;
}

//------------------------------------------------------------------------------------------------
bool AssetFactoryManager::ProcessPendingLoads()
{
    // Process the load
    bool bDidWork = false;
    LoadRequestQueue::iterator requestIter = m_pendingLoadRequests.begin();
    while (!bDidWork && requestIter != m_pendingLoadRequests.end())
    {
        LoadRequestDataPtr pData = *requestIter;

        if (pData->m_status == LRS_COMPLETED || pData->m_status == LRS_LOCATING)
        {
            requestIter++;
            continue;
        }

        EE_ASSERT(pData->m_status != LRS_CANCELLED);

        Category responseCategory = pData->m_spRequest->GetResponseCategory();
        IAssetFactory* pFactory = GetAssetFactory(responseCategory);

        if (!pFactory)
        {
            // We can't find the loader for a load request. Construct a response.
            pData->m_spResponse = EE_NEW AssetLoadResponse(
                pData->m_spRequest->GetURN(),
                pData->m_spRequest->GetResponseCategory(),
                AssetLoadResponse::ALR_FactoryNotFound);
            pData->m_status = LRS_COMPLETED;

            requestIter++;

            continue;
        }

        pData->m_factoryClassID = pFactory->GetClassID();

        AssetLoadResponsePtr spLoadResponse;
        switch (pFactory->LoadAsset(this, pData->m_spRequest, spLoadResponse))
        {
            case IAssetFactory::LOAD_COMPLETE:
                /// The load has complete processing, and the response
                /// is available. The response may contain an error
                /// condition, but it is ready to send.
                pData->m_spResponse = spLoadResponse;
                pData->m_status = LRS_COMPLETED;
                bDidWork = true;
                break;

            case IAssetFactory::LOAD_RUNNING:
                /// The load is partially complete but did enough work
                /// to preclude another loader from running in the same
                /// tick.
                EE_ASSERT(!spLoadResponse);
                pData->m_status = LRS_IN_PROGRESS;
                bDidWork = true;
                break;

            case IAssetFactory::LOAD_BLOCKED:
                /// The load is blocked on something and did no work.
                EE_ASSERT(!spLoadResponse);
                pData->m_status = LRS_IN_PROGRESS;
                break;
        }

        requestIter++;
    }

    return bDidWork;
}

//------------------------------------------------------------------------------------------------
bool AssetFactoryManager::LoadsInBackground(const AssetLoadRequest* pMessage)
{
    Category responseCategory = pMessage->GetResponseCategory();
    IAssetFactory* pFactory = GetAssetFactory(responseCategory);

    if (!pFactory)
        return false; // Have the system generate a foreground error

    IAssetFactory::ThreadSafetyDescription loaderTS = pFactory->GetThreadSafety();
    if (pMessage->GetIsBackground() &&
        (loaderTS == IAssetFactory::LOADER_BG_ONLY ||
        loaderTS == IAssetFactory::LOADER_FG_AND_BG))
    {
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult AssetFactoryManager::OnShutdown()
{
    m_pReceiveQueue = 0;

    m_pendingLoadRequests.clear();
    m_pendingLocatorRequests.clear();

    // Wait for other services with factories to shut down.  This will be an infinite loop if there
    // is a logic error that prevents a factory from being destroyed.
    if (ms_pAssetFactoryMap)
        return efd::AsyncResult_Pending;

    ms_assetFactoryCritSect.Lock();
    if (ms_pFactoriesPendingShutdown)
    {
        ms_assetFactoryCritSect.Unlock();
        return efd::AsyncResult_Pending;
    }
    ms_assetFactoryCritSect.Unlock();

    // The locator handler map should be empty
    ms_locatorHandlersCritSect.Lock();
    if (ms_pAssetLocatorMap)
    {
        EE_ASSERT(ms_pAssetLocatorMap->empty());
        EE_EXTERNAL_DELETE ms_pAssetLocatorMap;
        ms_pAssetLocatorMap = 0;
    }
    ms_locatorHandlersCritSect.Unlock();

    if (m_pLoaderThreadFunctor)
    {
        EE_ASSERT(m_pLoaderThread);
        m_pLoaderThreadFunctor->Shutdown();
        m_pLoaderThread->WaitForCompletion();

        EE_DELETE m_pLoaderThread;
        EE_DELETE m_pLoaderThreadFunctor;
    }

    if (m_spMessageService)
    {
        m_spMessageService->Unsubscribe(this, MessageCategory());
        m_spMessageService->Unsubscribe(this, m_assetLoadRequestCategory);

        m_spMessageService = 0;
    }

    m_spAssetLocatorService = 0;

    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
const char* efd::AssetFactoryManager::GetDisplayName() const
{
    return "AssetFactoryManager";
}

//------------------------------------------------------------------------------------------------
// Requests for Service Functions
//------------------------------------------------------------------------------------------------
void AssetFactoryManager::AssetLocate(const utf8string& urn, Category category)
{
    EE_ASSERT(category.IsValid());

    if (IsFgInstance())
    {
        // If you hit this assert, you attempted to locate assets before OnInit was called.
        EE_ASSERT(m_spAssetLocatorService);

        // Foreground just forwards the message to the locator service.
        m_spAssetLocatorService->AssetLocate(urn, category);
    }
    else
    {
        IAssetFactory* waitingFactory = 0;
        ms_locatorHandlersCritSect.Lock();
        // It is possible to not have a factory if the background thread is processing a load
        // after/while the foreground thread is trying to shut down. The factory calling
        // this function will return, waiting for the response, and then be shut down before
        // it completes. Or, if it forgot to register for the locate in the first place
        // it will hang. We can't assert because we don't know if this is shutdown or not.
        if (ms_pAssetLocatorMap)
        {
            ms_pAssetLocatorMap->find(category, waitingFactory);

            if (waitingFactory)
            {
                AssetLocatorRequest* pRequest = EE_NEW AssetLocatorRequest();
                pRequest->SetURI(urn);
                pRequest->ClientCategory(category);

                m_sendQueue.Produce(pRequest);
            }
        }

        ms_locatorHandlersCritSect.Unlock();
    }
}

//------------------------------------------------------------------------------------------------
// Asset Factory Registration Functions
//------------------------------------------------------------------------------------------------
bool AssetFactoryManager::RegisterAssetFactory(
    const Category responseCategory,
    IAssetFactory* pAssetFactory)
{
    AssetFactoryInfo* pInfo = EE_NEW AssetFactoryInfo(pAssetFactory, responseCategory);

    ms_assetFactoryCritSect.Lock();

    if (!ms_pAssetFactoryMap)
    {
        ms_pAssetFactoryMap = EE_EXTERNAL_NEW map<Category, AssetFactoryInfo*>;
    }

    AssetFactoryInfo* pTest = NULL;
    if (ms_pAssetFactoryMap->find(responseCategory, pTest))
    {
        ms_assetFactoryCritSect.Unlock();
        EE_DELETE pInfo;
        return false;
    }

    (*ms_pAssetFactoryMap)[responseCategory] = pInfo;

    ms_assetFactoryCritSect.Unlock();

    return true;
}

//------------------------------------------------------------------------------------------------
IAssetFactory* AssetFactoryManager::UnregisterAssetFactory(const Category responseCategory)
{
    ms_assetFactoryCritSect.Lock();

    if (!ms_pAssetFactoryMap)
    {
        ms_assetFactoryCritSect.Unlock();
        return NULL;
    }

    map<Category, AssetFactoryInfo*>::iterator eraseIter =
        ms_pAssetFactoryMap->find(responseCategory);
    if (eraseIter == ms_pAssetFactoryMap->end())
    {
        ms_assetFactoryCritSect.Unlock();
        return NULL;
    }

    AssetFactoryInfo* pInfo = eraseIter->second;

    ms_pAssetFactoryMap->erase(eraseIter);

    if (ms_pAssetFactoryMap->empty())
    {
        EE_EXTERNAL_DELETE ms_pAssetFactoryMap;
        ms_pAssetFactoryMap = NULL;
    }

    IAssetFactory* pAssetFactory = pInfo->m_spAssetFactory;
    if (!ms_pFactoriesPendingShutdown)
    {
        ms_pFactoriesPendingShutdown = EE_EXTERNAL_NEW AssetFactoryList();
    }
    ms_pFactoriesPendingShutdown->push_back(pAssetFactory);
    ms_assetFactoryCritSect.Unlock();

    // find any IAssetFactory instances registered for AssetLocatorResponses.
    ms_locatorHandlersCritSect.Lock();
    if (ms_pAssetLocatorMap)
    {
        for (AssetLocatorMap::iterator it = ms_pAssetLocatorMap->begin();
            it != ms_pAssetLocatorMap->end();
            /* empty */)
        {
            if (it->second == pAssetFactory)
            {
                ms_pAssetLocatorMap->erase(it++);
            }
            else
            {
                ++it;
            }
        }
    }
    ms_locatorHandlersCritSect.Unlock();

    EE_DELETE pInfo;

    return pAssetFactory;
}

//------------------------------------------------------------------------------------------------
IAssetFactory* AssetFactoryManager::GetAssetFactory(const Category responseCategory)
{
    ms_assetFactoryCritSect.Lock();

    IAssetFactory* pResult = GetAssetFactoryInternal(responseCategory);

    ms_assetFactoryCritSect.Unlock();

    return pResult;
}

//------------------------------------------------------------------------------------------------
IAssetFactory* AssetFactoryManager::GetAssetFactoryInternal(const Category responseCategory)
{
    if (!ms_pAssetFactoryMap)
    {
        return NULL;
    }

    map<Category, AssetFactoryInfo*>::iterator iter =
        ms_pAssetFactoryMap->find(responseCategory);
    if (iter == ms_pAssetFactoryMap->end())
    {
        return NULL;
    }

    AssetFactoryInfo* pInfo = iter->second;

    return pInfo->m_spAssetFactory;
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::RegisterAssetLocateHandler(
    efd::Category targetCategory,
    efd::IAssetFactory* factory)
{
    EE_ASSERT(targetCategory.IsValid());
    EE_ASSERT(factory);

    ms_locatorHandlersCritSect.Lock();

    if (!ms_pAssetLocatorMap)
    {
        ms_pAssetLocatorMap = EE_EXTERNAL_NEW AssetLocatorMap;
    }
    (*ms_pAssetLocatorMap)[targetCategory] = factory;

    ms_locatorHandlersCritSect.Unlock();
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::ShutdownFactories()
{
    EE_ASSERT(IsFgInstance() == false);

    if (ms_pFactoriesPendingShutdown)
    {
        ms_assetFactoryCritSect.Lock();
        AssetFactoryList::iterator it = ms_pFactoriesPendingShutdown->begin();
        for (; it != ms_pFactoriesPendingShutdown->end(); ++it)
        {
            (*it)->Shutdown();
        }
        ms_pFactoriesPendingShutdown->clear();

        // Yes, this deletes it and forces recreation every time a factory is shut down.
        // That's OK, we do not expect to have factories shutting down often and this simplifies,
        // a lot, the memory management.
        EE_EXTERNAL_DELETE ms_pFactoriesPendingShutdown;
        ms_pFactoriesPendingShutdown = NULL;
        ms_assetFactoryCritSect.Unlock();
    }
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::SetBackgroundMessageCapPerTick(efd::UInt32 maxMessageCountPerTick)
{
    EE_ASSERT(maxMessageCountPerTick > 0);
    if (maxMessageCountPerTick == 0)
        maxMessageCountPerTick = 1;
    m_backgroundMessageCapPerTick = maxMessageCountPerTick;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 AssetFactoryManager::GetBackgroundMessageCapPerTick() const
{
    return m_backgroundMessageCapPerTick;
}

//------------------------------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------------------------------
void AssetFactoryManager::AssetLoadRequestHandler(
    const AssetLoadRequest* pMessage,
    Category targetChannel)
{
    EE_UNUSED_ARG(targetChannel);
    EE_ASSERT(targetChannel == MessageCategory());

    bool isBackgroundLoad = LoadsInBackground(pMessage);

    // make sure the load request matches the capabilities of the factory.
    const Category& responseCategory = pMessage->GetResponseCategory();
    IAssetFactory* pFactory = GetAssetFactory(responseCategory);

    if (!pFactory)
    {
        EE_LOG(efd::kAssetLoading,
            efd::ILogger::kERR1,
            ("No registered factory available to load request URN \'%s\' ",
            pMessage->GetURN().c_str()));

        AssetLoadResponsePtr spResponse = EE_NEW AssetLoadResponse(
            pMessage->GetURN(),
            pMessage->GetResponseCategory(),
            AssetLoadResponse::ALR_FactoryNotFound);

        m_spMessageService->SendLocal(spResponse, pMessage->GetResponseCategory());

        return;
    }

    if ((pFactory->GetThreadSafety() == IAssetFactory::LOADER_FG_ONLY && isBackgroundLoad) ||
        (pFactory->GetThreadSafety() == IAssetFactory::LOADER_BG_ONLY && !isBackgroundLoad))
    {
        utf8string t;
        isBackgroundLoad ? t = "Background" : t = "Foreground";

        // The request specifies a load thread to use, but the loader does not support the request.
        EE_LOG(efd::kAssetLoading,
            efd::ILogger::kERR1,
            ("A load request for URN \'%s\' specified a loading thread (%s), but the factory that "
            "handles the load request does not support loading the asset on this thread.",
             pMessage->GetURN().c_str(),
             t.c_str()));

        AssetLoadResponsePtr spResponse = EE_NEW AssetLoadResponse(
            pMessage->GetURN(),
            pMessage->GetResponseCategory(),
            AssetLoadResponse::ALR_RequestNotSupported);

        m_spMessageService->SendLocal(spResponse, pMessage->GetResponseCategory());
        ForwardToCallbackListeners(pFactory->GetClassID(), spResponse);

        return;
    }

    // A message that is background loaded, has a file name and is preemptive
    // goes direct to the background
    if (isBackgroundLoad && pMessage->GetIsPreemptive() && !pMessage->GetAssetPath().empty())
    {
        m_sendQueue.Produce(pMessage);
        return;
    }

    // Any other message needs to be queued, somewhere
    LoadRequestDataPtr pData = EE_NEW LoadRequestData;
    pData->m_spRequest = pMessage;
    pData->m_spResponse = 0;
    pData->m_status = pMessage->GetAssetPath().empty() ? LRS_LOCATING : LRS_PENDING;
    pData->m_isForBackground = isBackgroundLoad;

    // Do we need a locate for this one?
    if (pData->m_status == LRS_LOCATING)
    {
        utf8string requestURN = pMessage->GetURN();
        EE_ASSERT(requestURN.find("urn:") != utf8string::npos);

        // Locate the asset. We need to store a copy of the message while we hold onto
        // it for further processing.
        LocatorRequestDataPtr pRequestData = EE_NEW LocatorRequestData();
        pRequestData->m_urn = requestURN;
        pRequestData->m_spSourceMessage = pMessage;
        pRequestData->m_spLoadRequestData = pData;
        pRequestData->m_responseCategory = m_assetLoadRequestCategory;
        pData->m_factoryClassID = pFactory->GetClassID();

        m_pendingLocatorRequests.push_back(pRequestData);

        if (!m_spAssetLocatorService->AssetLocate(requestURN, m_assetLoadRequestCategory))
        {
            // We can't find the asset for a load request. Construct a response and send it.
            AssetLoadResponsePtr spResponse = EE_NEW AssetLoadResponse(
                pMessage->GetURN(),
                pMessage->GetResponseCategory(),
                AssetLoadResponse::ALR_AssetNotFound);

            m_spMessageService->SendLocal(spResponse, pMessage->GetResponseCategory());
            ForwardToCallbackListeners(pFactory->GetClassID(), spResponse);

            m_pendingLocatorRequests.pop_back();

            return;
        }
    }

    // What queue do we go into?
    if (pMessage->GetIsPreemptive())
    {
        m_preemptiveLoadRequests.push_back(pData);
    }
    else
    {
        m_pendingLoadRequests.push_back(pData);
    }
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::AssetLocatorResponseHandler(
    const AssetLocatorResponse* pMessage,
    Category targetChannel)
{
    EE_ASSERT(targetChannel == m_assetLoadRequestCategory);
    EE_UNUSED_ARG(targetChannel);

    // Get the original request URI
    const utf8string& uri = pMessage->GetResponseURI();
    const AssetLocatorResponse::AssetURLMap& assets = pMessage->GetAssetURLMap();

    LocatorRequestList::iterator locatorDataIter = m_pendingLocatorRequests.begin();
    for (;locatorDataIter != m_pendingLocatorRequests.end(); ++locatorDataIter)
    {
        if ((*locatorDataIter)->m_urn == uri)
        {
            break;
        }
    }

    if (locatorDataIter == m_pendingLocatorRequests.end())
    {
        return;
    }
    LocatorRequestDataPtr spData = *locatorDataIter;

    // Handle a response that this service generated for an asset load
    if (spData->m_spLoadRequestData != NULL)
    {
        // Because we have a LoadRequestData pointer we know this is a foreground request. We can
        // verify this because the m_responseCategory is always m_assetLoadRequestCategory for
        // foreground requests.
        EE_ASSERT(spData->m_responseCategory == m_assetLoadRequestCategory);

        AssetLoadRequest* pLoadRequest =
            EE_DYNAMIC_CAST(AssetLoadRequest, spData->m_spSourceMessage);
        EE_ASSERT(pLoadRequest);

        LoadRequestData* pLoadData = spData->m_spLoadRequestData;
        EE_ASSERT(pLoadData->m_status == LRS_LOCATING);

        m_pendingLocatorRequests.erase(locatorDataIter);

        if (assets.size() == 0)
        {
            // We can't find the asset for a load request. Construct a response and send it.
            AssetLoadResponsePtr spResponse = EE_NEW AssetLoadResponse(
                pLoadRequest->GetURN(),
                pLoadRequest->GetResponseCategory(),
                AssetLoadResponse::ALR_AssetNotFound);

            m_spMessageService->SendLocal(spResponse, pLoadRequest->GetResponseCategory());

            IAssetFactory* pFactory = GetAssetFactory(pLoadRequest->GetResponseCategory());
            EE_ASSERT(pFactory);

            ForwardToCallbackListeners(pFactory->GetClassID(), spResponse);

            // Remove it from any other queues
            if (pLoadData->m_spRequest->GetIsPreemptive())
            {
                m_preemptiveLoadRequests.remove(pLoadData);
            }
            else
            {
                m_pendingLoadRequests.remove(pLoadData);
            }

            spData = 0;

            return;
        }

        // @todo: need to load them all if I want to preload a tagged group
        if (assets.size() > 1)
        {
            EE_LOG(efd::kAssetLoading, efd::Logger::kERR2,
                ("AssetFactoryManager: Request for urn %s returned more than one asset. "
                "Only the first will be loaded.",
                pLoadRequest->GetURN().c_str()));
        }

        AssetLocatorResponse::AssetURLMap::const_iterator assetIterator = assets.begin();
        pLoadRequest->SetAssetPath(assetIterator->second.url);

        // Update the status of the load request. Further action will be taken in OnTick.
        pLoadData->m_status = LRS_PENDING;

        spData = 0;

        return;
    }
    else
    {
        // No LoadRequestData pointer means the message is destined for the background thread
        EE_ASSERT(!spData->m_spLoadRequestData);

        // Because we receive this message on a private callback category, we *should* be the only
        // recipient of the message. As such, it should be safe for us to adopt this message and
        // pass it to the background thread without fear of anyone else modifying the message data.
        // But to do this we need to re-target the ClientCategory from our category to the one the
        // background thread expects, which requires a const_cast.
        const_cast<AssetLocatorResponse*>(pMessage)->ClientCategory(spData->m_responseCategory);

        m_pendingLocatorRequests.erase(locatorDataIter);
        m_sendQueue.Produce(pMessage);
    }
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::RegisterAssetCallback(
    efd::ClassID factoryClassID,
    const efd::Category& callback)
{
    m_callbacks[factoryClassID].insert(callback);
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::UnregisterAssetCallback(
    efd::ClassID factoryClassID,
    const efd::Category& callback)
{
    m_callbacks[factoryClassID].erase(callback);

    if (!m_callbacks[factoryClassID].size())
    {
        m_callbacks.erase(factoryClassID);
    }
}

//------------------------------------------------------------------------------------------------
void AssetFactoryManager::ForwardToCallbackListeners(
    efd::ClassID factoryClassID,
    const AssetLoadResponse* pMsg)
{
    CallbackCategoryMap::iterator cbIt = m_callbacks.find(factoryClassID);
    if (cbIt != m_callbacks.end())
    {
        efd::set<Category>::const_iterator catIt = cbIt->second.begin();
        for (; catIt != cbIt->second.end(); ++catIt)
        {
            m_spMessageService->SendLocal(pMsg, *catIt);
        }
    }
}

//------------------------------------------------------------------------------------------------
void efd::AssetFactoryManager::SetSleepInterval(const efd::UInt32 sleepInterval)
{
    m_sleepInterval = sleepInterval;

    if (m_pLoaderThreadFunctor)
    {
        m_pLoaderThreadFunctor->SetSleepInterval(sleepInterval);
    }
}


//------------------------------------------------------------------------------------------------
// AssetFactoryInfo functions
//------------------------------------------------------------------------------------------------
AssetFactoryManager::AssetFactoryInfo::AssetFactoryInfo(
    IAssetFactory* pAssetFactory,
    const Category& reponseCategory)
    : m_spAssetFactory(pAssetFactory)
    , m_responseCategory(reponseCategory)
{
}

//------------------------------------------------------------------------------------------------
// LocatorRequestData functions
//------------------------------------------------------------------------------------------------
AssetFactoryManager::LocatorRequestData::~LocatorRequestData()
{
    m_spSourceMessage = 0;
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// LoadRequestData functions
//------------------------------------------------------------------------------------------------
AssetFactoryManager::LoadRequestData::~LoadRequestData()
{
    m_spRequest = 0;
    m_spResponse = 0;
}
