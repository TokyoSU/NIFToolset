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

#include <egf/NotificationService.h>

#include <efd/IConfigManager.h>
#include <efd/ILogger.h>
#include <efd/Metrics.h>
#include <egf/egfLogIDs.h>
#include <efd/ServiceManager.h>

using namespace efd;
using namespace egf;


//------------------------------------------------------------------------------------------------
// NotificationService class method implementations
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NotificationService);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NotificationMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NotificationService::NotifyWrapper);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NotificationService::NotifyByMessage);
EE_IMPLEMENT_CONCRETE_CLASS_INFO(NotificationService::NotifyByBehavior);

EE_HANDLER_WRAP(
    NotificationService,
    HandleEntityDiscoverMessage,
    egf::EntityChangeMessage,
    kMSGID_OwnedEntityAdded);

EE_HANDLER_WRAP(
    NotificationService,
    HandleEntityRemovedMessage,
    egf::EntityChangeMessage,
    kMSGID_OwnedEntityRemoved);

EE_HANDLER_WRAP(
    NotificationService,
    HandleEntityUpdatedMessage,
    egf::EntityChangeMessage,
    kMSGID_OwnedEntityUpdated);


//------------------------------------------------------------------------------------------------
NotificationMessage::NotificationMessage()
{
    m_entitySet.clear();
}

//------------------------------------------------------------------------------------------------
NotificationMessage::~NotificationMessage()
{
    m_entitySet.clear();
}

//------------------------------------------------------------------------------------------------
void NotificationMessage::Serialize(efd::Archive& ar)
{
    DirectedMessage::Serialize(ar);
    SR_StdSet<>::Serialize(m_entitySet, ar);
}

//------------------------------------------------------------------------------------------------
void NotificationMessage::SetEntities(const efd::set<egf::Entity*>& entitySet)
{
    m_entitySet.clear();
    efd::set<egf::Entity*>::const_iterator iter;
    for (iter = entitySet.begin(); iter != entitySet.end(); iter++)
    {
        if (*iter != NULL)
            m_entitySet.insert((*iter)->GetEntityID());
    }
}

//------------------------------------------------------------------------------------------------
void NotificationService::NotifyWrapper::AddNotice(egf::Entity* pEntity)
{
    // Add the entity to the set to be sent later
    m_entitySet.insert(pEntity);
}

//------------------------------------------------------------------------------------------------
NotificationService::NotifyByMessage::NotifyByMessage(
    const efd::Category& sendCat,
    efd::MessageService* pMessageService)
{
    m_sendCat = sendCat;
    m_spMessageService = pMessageService;
}

//------------------------------------------------------------------------------------------------
NotificationService::NotifyByMessage::~NotifyByMessage() {}

//------------------------------------------------------------------------------------------------
void NotificationService::NotifyByMessage::SendNotification(efd::ClassID notificationType)
{
    NotificationMessagePtr pMsg = NULL;

    switch (notificationType)
    {
    case kMSGID_NoticeEntityDiscovered:
        pMsg = EE_NEW MessageWrapper<NotificationMessage, kMSGID_NoticeEntityDiscovered>;
        break;
    case kMSGID_NoticeEntityUpdated:
        pMsg = EE_NEW MessageWrapper<NotificationMessage, kMSGID_NoticeEntityUpdated>;
        break;
    case kMSGID_NoticeEntityRemoved:
        pMsg = EE_NEW MessageWrapper<NotificationMessage, kMSGID_NoticeEntityRemoved>;
        break;
    }

    // Set the category to send the message to
    pMsg->SetDestinationCategory(m_sendCat);

    // Add the entities that were part of the notification
    pMsg->SetEntities(m_entitySet);

    // Send the message locally.
    m_spMessageService->SendImmediate(pMsg);
}

//------------------------------------------------------------------------------------------------
NotificationService::NotifyByBehavior::NotifyByBehavior(
    const egf::EntityID& sendToEntity,
    const egf::BehaviorID sendToBehavior,
    efd::MessageService* pMessageService)
{
    m_sendToEntity = sendToEntity;
    m_sendToBehavior = sendToBehavior;
    m_spMessageService = pMessageService;
    EE_ASSERT(pMessageService);
}

//------------------------------------------------------------------------------------------------
NotificationService::NotifyByBehavior::~NotifyByBehavior() {}

//------------------------------------------------------------------------------------------------
void NotificationService::NotifyByBehavior::SendNotification(efd::ClassID notificationType)
{
    ParameterListPtr pPayload = EE_NEW ParameterList();

    // Payload is always the entity ID and model name
    pPayload->AddParameter(notificationType);
    pPayload->AddParameter(m_entitySet.size());

    efd::set<egf::Entity*>::const_iterator iter;
    for (iter = m_entitySet.begin(); iter != m_entitySet.end(); iter++)
    {
        if (*iter != NULL)
            pPayload->AddParameter((*iter)->GetEntityID());
    }

    EventID retVal = 0;
    EventMessagePtr spEvent = EventMessage::CreateEvent(m_sendToEntity,
        0, 0, m_sendToBehavior, 0, pPayload);

    EE_ASSERT(spEvent);
    if (spEvent && m_spMessageService)
    {
        // Send the event
        m_spMessageService->Send(spEvent, m_sendToEntity, QOS_RELIABLE);
    }
}

//------------------------------------------------------------------------------------------------
NotificationService::NotificationService()
    : m_spMessageService(0)
    , m_spFMM(0)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 2020;
    //m_entityIDCBs;
    //m_modelIDCBs;
    //m_noticesToSend;
}

//------------------------------------------------------------------------------------------------
NotificationService::~NotificationService()
{
    m_spMessageService = NULL;
    m_entityIDCBs.clear();
    m_modelIDCBs.clear();
    m_discoversToSend.clear();
    m_updatesToSend.clear();
    m_removesToSend.clear();
}

//------------------------------------------------------------------------------------------------
efd::SyncResult NotificationService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    // Because all I do is subscribe and send messages
    pDependencyRegistrar->AddDependency<MessageService>();
    // Because all I do is respond to EntityManager messages
    pDependencyRegistrar->AddDependency<EntityManager>();
    // To ensure model data is available
    pDependencyRegistrar->AddDependency<FlatModelManager>();

    m_spMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    if (!m_spMessageService)
    {
        EE_LOG(efd::kNotificationService, efd::ILogger::kERR2,
            ("Init: Failed to find MessageService to register callback handlers!"));
        return efd::SyncResult_Failure;
    }

    m_spFMM = m_pServiceManager->GetSystemServiceAs<FlatModelManager>();
    if (!m_spFMM)
    {
        EE_LOG(efd::kNotificationService, efd::ILogger::kERR2,
            ("Init: Failed to find Flat Model Manager for behavior ID look ups!"));
        return efd::SyncResult_Failure;
    }

    m_spMessageService->Subscribe(this, kCAT_LocalMessage);

    return efd::SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult NotificationService::OnTick()
{
    // Send execute the callbacks that have been collected
    CallbackSet::iterator iter;

    // Loop through all the discovers and notify them
    for (iter = m_discoversToSend.begin(); iter != m_discoversToSend.end(); iter++)
    {
        (*iter)->SendNotification(kMSGID_NoticeEntityDiscovered);
    }
    // Loop through all the updates and notify them
    for (iter = m_updatesToSend.begin(); iter != m_updatesToSend.end(); iter++)
    {
        (*iter)->SendNotification(kMSGID_NoticeEntityUpdated);
    }
    // Loop through all the removes and notify them
    for (iter = m_removesToSend.begin(); iter != m_removesToSend.end(); iter++)
    {
        (*iter)->SendNotification(kMSGID_NoticeEntityRemoved);
    }
    m_discoversToSend.clear();
    m_updatesToSend.clear();
    m_removesToSend.clear();

    return efd::AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult NotificationService::OnShutdown()
{
    if (m_spMessageService != NULL)
    {
        m_spMessageService->Unsubscribe(this, kCAT_LocalMessage);
        m_spMessageService = NULL;
    }

    m_spFMM = NULL;

    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
const char* NotificationService::GetDisplayName() const
{
    return "NotifcationService";
}

//------------------------------------------------------------------------------------------------
bool NotificationService::AddNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const efd::Category& sendTo,
    const egf::FlatModelID modelID)
{
    NotifyWrapper* pDiscoverWrapper = NULL;
    NotifyWrapper* pUpdateWrapper = NULL;
    NotifyWrapper* pRemoveWrapper = NULL;

    // Create the desired wrappers for the notifications
    if (notifyDiscover)
    {
        pDiscoverWrapper = EE_NEW NotifyByMessage(sendTo, m_spMessageService);
        EE_ASSERT(pDiscoverWrapper);
    }
    if (notifyUpdate)
    {
        pUpdateWrapper = EE_NEW NotifyByMessage(sendTo, m_spMessageService);
        EE_ASSERT(pUpdateWrapper);
    }
    if (notifyRemoval)
    {
        pRemoveWrapper = EE_NEW NotifyByMessage(sendTo, m_spMessageService);
        EE_ASSERT(pRemoveWrapper);
    }

    // Call the helper to add the wrappers to the maps
    return AddNotification(
        pDiscoverWrapper,
        pUpdateWrapper,
        pRemoveWrapper,
        modelID);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::AddNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const efd::Category& sendTo,
    const egf::EntityID& entityID)
{
    NotifyWrapper* pDiscoverWrapper = NULL;
    NotifyWrapper* pUpdateWrapper = NULL;
    NotifyWrapper* pRemoveWrapper = NULL;

    // Create the desired wrappers for the notifications
    if (notifyDiscover)
    {
        pDiscoverWrapper = EE_NEW NotifyByMessage(sendTo, m_spMessageService);
        EE_ASSERT(pDiscoverWrapper);
    }
    if (notifyUpdate)
    {
        pUpdateWrapper = EE_NEW NotifyByMessage(sendTo, m_spMessageService);
        EE_ASSERT(pUpdateWrapper);
    }
    if (notifyRemoval)
    {
        pRemoveWrapper = EE_NEW NotifyByMessage(sendTo, m_spMessageService);
        EE_ASSERT(pRemoveWrapper);
    }

    // Call the helper to add the wrappers to the maps
    return AddNotification(
        pDiscoverWrapper,
        pUpdateWrapper,
        pRemoveWrapper,
        entityID);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::AddNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const egf::EntityID& sendToEntity,
    const egf::BehaviorID sendToBehavior,
    const egf::FlatModelID modelID)
{
    NotifyWrapper* pDiscoverWrapper = NULL;
    NotifyWrapper* pUpdateWrapper = NULL;
    NotifyWrapper* pRemoveWrapper = NULL;

    // Create the desired wrappers for the notifications
    if (notifyDiscover)
    {
        pDiscoverWrapper = EE_NEW NotifyByBehavior(
            sendToEntity,
            sendToBehavior,
            m_spMessageService);
        EE_ASSERT(pDiscoverWrapper);
    }
    if (notifyUpdate)
    {
        pUpdateWrapper = EE_NEW NotifyByBehavior(
            sendToEntity,
            sendToBehavior,
            m_spMessageService);
        EE_ASSERT(pUpdateWrapper);
    }
    if (notifyRemoval)
    {
        pRemoveWrapper = EE_NEW NotifyByBehavior(
            sendToEntity,
            sendToBehavior,
            m_spMessageService);
        EE_ASSERT(pRemoveWrapper);
    }

    // Call the helper to add the wrappers to the maps
    return AddNotification(
        pDiscoverWrapper,
        pUpdateWrapper,
        pRemoveWrapper,
        modelID);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::AddNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const egf::EntityID& sendToEntity,
    const egf::BehaviorID sendToBehavior,
    const egf::EntityID& entityID)
{
    NotifyWrapper* pDiscoverWrapper = NULL;
    NotifyWrapper* pUpdateWrapper = NULL;
    NotifyWrapper* pRemoveWrapper = NULL;

    // Create the desired wrappers for the notifications
    if (notifyDiscover)
    {
        pDiscoverWrapper = EE_NEW NotifyByBehavior(
            sendToEntity,
            sendToBehavior,
            m_spMessageService);
        EE_ASSERT(pDiscoverWrapper);
    }
    if (notifyUpdate)
    {
        pUpdateWrapper = EE_NEW NotifyByBehavior(
            sendToEntity,
            sendToBehavior,
            m_spMessageService);
        EE_ASSERT(pUpdateWrapper);
    }
    if (notifyRemoval)
    {
        pRemoveWrapper = EE_NEW NotifyByBehavior(
            sendToEntity,
            sendToBehavior,
            m_spMessageService);
        EE_ASSERT(pRemoveWrapper);
    }

    // Call the helper to add the wrappers to the maps
    return AddNotification(
        pDiscoverWrapper,
        pUpdateWrapper,
        pRemoveWrapper,
        entityID);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::AddNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const egf::EntityID& sendToEntity,
    const efd::utf8string& sendToBehavior,
    const egf::FlatModelID modelID)
{
    egf::BehaviorID behaviorID= m_spFMM->GetBehaviorIDByName(sendToBehavior);

    return AddNotification(
        notifyDiscover,
        notifyUpdate,
        notifyRemoval,
        sendToEntity,
        behaviorID,
        modelID);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::AddNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const egf::EntityID& sendToEntity,
    const efd::utf8string& sendToBehavior,
    const egf::EntityID& entityID)
{
    egf::BehaviorID behaviorID= m_spFMM->GetBehaviorIDByName(sendToBehavior);

    return AddNotification(
        notifyDiscover,
        notifyUpdate,
        notifyRemoval,
        sendToEntity,
        behaviorID,
        entityID);
}

//------------------------------------------------------------------------------------------------
void NotificationService::RemoveAllNotifications()
{
    // Clear all the entity based notifications
    m_entityIDCBs.clear();

    // Clear all the model based notifications
    m_modelIDCBs.clear();
}

//------------------------------------------------------------------------------------------------
bool NotificationService::RemoveNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const efd::Category& sendTo,
    const egf::FlatModelID modelID)
{
    efd::ClassID notifyType = 0;
    if (notifyDiscover)
        notifyType = kMSGID_NoticeEntityDiscovered;
    if (notifyUpdate)
        notifyType = kMSGID_NoticeEntityUpdated;
    if (notifyRemoval)
        notifyType = kMSGID_NoticeEntityRemoved;

    DeleteData Data(sendTo, 0, 0);

    return RemoveNotification(notifyType, &Data, modelID);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::RemoveNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const efd::Category& sendTo,
    const egf::EntityID& entityID)
{
    efd::ClassID notifyType = 0;
    if (notifyDiscover)
        notifyType = kMSGID_NoticeEntityDiscovered;
    if (notifyUpdate)
        notifyType = kMSGID_NoticeEntityUpdated;
    if (notifyRemoval)
        notifyType = kMSGID_NoticeEntityRemoved;

    DeleteData Data(sendTo, 0, 0);

    return RemoveNotification(notifyType, &Data, entityID);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::RemoveNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const egf::EntityID& sendToEntity,
    const egf::BehaviorID sendToBehavior,
    const egf::FlatModelID modelID)
{
    efd::ClassID notifyType = 0;
    if (notifyDiscover)
        notifyType = kMSGID_NoticeEntityDiscovered;
    if (notifyUpdate)
        notifyType = kMSGID_NoticeEntityUpdated;
    if (notifyRemoval)
        notifyType = kMSGID_NoticeEntityRemoved;

    DeleteData Data(0, sendToEntity, sendToBehavior);

    return RemoveNotification(notifyType, &Data, modelID);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::RemoveNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const egf::EntityID& sendToEntity,
    const egf::BehaviorID sendToBehavior,
    const egf::EntityID& entityID)
{
    efd::ClassID notifyType = 0;
    if (notifyDiscover)
        notifyType = kMSGID_NoticeEntityDiscovered;
    if (notifyUpdate)
        notifyType = kMSGID_NoticeEntityUpdated;
    if (notifyRemoval)
        notifyType = kMSGID_NoticeEntityRemoved;

    DeleteData Data(0, sendToEntity, sendToBehavior);

    return RemoveNotification(notifyType, &Data, entityID);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::RemoveNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const egf::EntityID& sendToEntity,
    const efd::utf8string& sendToBehavior,
    const egf::FlatModelID modelID)
{
    egf::BehaviorID behaviorID = m_spFMM->GetBehaviorIDByName(sendToBehavior);

    return RemoveNotification(
        notifyDiscover,
        notifyUpdate,
        notifyRemoval,
        sendToEntity,
        behaviorID,
        modelID);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::RemoveNotification(
    bool notifyDiscover,
    bool notifyUpdate,
    bool notifyRemoval,
    const egf::EntityID& sendToEntity,
    const efd::utf8string& sendToBehavior,
    const egf::EntityID& entityID)
{
    egf::BehaviorID behaviorID = m_spFMM->GetBehaviorIDByName(sendToBehavior);

    return RemoveNotification(
        notifyDiscover,
        notifyUpdate,
        notifyRemoval,
        sendToEntity,
        behaviorID,
        entityID);
}

//------------------------------------------------------------------------------------------------
void NotificationService::HandleEntityDiscoverMessage(
    const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    return HandleEntityMessage(
        kMSGID_NoticeEntityDiscovered,
        pMessage,
        targetChannel);
}

//------------------------------------------------------------------------------------------------
void NotificationService::HandleEntityRemovedMessage(
    const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    return HandleEntityMessage(
        kMSGID_NoticeEntityRemoved,
        pMessage,
        targetChannel);
}

//------------------------------------------------------------------------------------------------
void NotificationService::HandleEntityUpdatedMessage(
    const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    return HandleEntityMessage(
        kMSGID_NoticeEntityUpdated,
        pMessage,
        targetChannel);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::AddNotification(
    NotifyWrapper* pDiscoverWrapper,
    NotifyWrapper* pUpdateWrapper,
    NotifyWrapper* pRemoveWrapper,
    CallbackMaps* pMap)
{
    // Make sure a valid map was passed in
    EE_ASSERT(pMap);

    // Add the callback wrappers to the callback lists
    if (pDiscoverWrapper)
        pMap->m_callbacks.m_discovers.push_back(pDiscoverWrapper);
    if (pUpdateWrapper)
        pMap->m_callbacks.m_updates.push_back(pUpdateWrapper);
    if (pRemoveWrapper)
        pMap->m_callbacks.m_removes.push_back(pRemoveWrapper);
    return true;
}

//------------------------------------------------------------------------------------------------
bool NotificationService::AddNotification(
    NotifyWrapper* pDiscoverWrapper,
    NotifyWrapper* pUpdateWrapper,
    NotifyWrapper* pRemoveWrapper,
    const egf::FlatModelID modelID)
{
    // Check to see if there is an entry for this model
    CallbackMapsPtr spModelMap;
    if (!m_modelIDCBs.find(modelID, spModelMap))
    {
        // If not, create a new one
        spModelMap = EE_NEW CallbackMaps;
        // Add the callback map to the model map at the model ID's position
        m_modelIDCBs[modelID] = spModelMap;
    }

    // Call the helper to add the wrappers to the model map
    return AddNotification(
        pDiscoverWrapper,
        pUpdateWrapper,
        pRemoveWrapper,
        spModelMap);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::AddNotification(
    NotifyWrapper* pDiscoverWrapper,
    NotifyWrapper* pUpdateWrapper,
    NotifyWrapper* pRemoveWrapper,
    const egf::EntityID& entityID)
{
    // Check to see if there is already an entry for this entity
    CallbackMapsPtr spEntityMap;
    if (!m_entityIDCBs.find(entityID, spEntityMap))
    {
        // If not, create a new one
        spEntityMap = EE_NEW CallbackMaps;

        // Add the callback map to the entity map at the entity ID's position
        m_entityIDCBs[entityID] = spEntityMap;
    }

    // Call the helper to add the wrappers to the entity map
    return AddNotification(
        pDiscoverWrapper,
        pUpdateWrapper,
        pRemoveWrapper,
        spEntityMap);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::RemoveNotification(
    efd::ClassID notifyType,
    DeleteData* pData,
    CallbackMaps* pMap)
{
    bool retVal = false;
    // Make sure a valid map was passed in
    EE_ASSERT(pMap);

    // Get the correct list to look in
    CallbackList* pList = NULL;
    CallbackSet* pSet = NULL;
    switch (notifyType)
    {
    case kMSGID_NoticeEntityDiscovered:
        pList = &(pMap->m_callbacks.m_discovers);
        pSet = &m_discoversToSend;
        break;
    case kMSGID_NoticeEntityUpdated:
        pList = &(pMap->m_callbacks.m_updates);
        pSet = &m_updatesToSend;
        break;
    case kMSGID_NoticeEntityRemoved:
        pList = &(pMap->m_callbacks.m_removes);
        pSet = &m_removesToSend;
        break;
    }

    CallbackList::iterator iter;
    for (iter = pList->begin(); iter != pList->end(); iter++)
    {
        NotifyWrapper* pBase = *iter;
        if (EE_IS_KIND_OF(NotifyByMessage, pBase))
        {
            // message deletion
            NotifyByMessage* pWrapper = (NotifyByMessage*)(pBase);
            if (pWrapper->GetCategory() == pData->m_sendTo)
            {
                pList->erase(iter);
                retVal = true;
                break;
            }
        }
        if (EE_IS_KIND_OF(NotifyByBehavior, pBase))
        {
            // Behavior invocation deletion
            NotifyByBehavior* pWrapper = (NotifyByBehavior*)(pBase);
            if ((pWrapper->GetEntity() == pData->m_sendToEntity) && (pWrapper->GetBehavior() == pData->m_sendToBehavior))
            {
                pList->erase(iter);
                retVal = true;
                break;
            }
        }
    }

    return retVal;
}

//------------------------------------------------------------------------------------------------
bool NotificationService::RemoveNotification(
    efd::ClassID notifyType,
    DeleteData* pData,
    const egf::FlatModelID modelID)
{
    // Check to see if there is an entry for this model
    CallbackMapsPtr spModelMap;
    if (!m_modelIDCBs.find(modelID, spModelMap))
    {
        // If not, create a new one
        spModelMap = EE_NEW CallbackMaps;
        // Add the callback map to the model map at the model ID's position
        m_modelIDCBs[modelID] = spModelMap;
    }

    // Call the helper to add the wrappers to the model map
    return RemoveNotification(
        notifyType,
        pData,
        spModelMap);
}

//------------------------------------------------------------------------------------------------
bool NotificationService::RemoveNotification(
    efd::ClassID notifyType,
    DeleteData* pData,
    const egf::EntityID& entityID)
{
    // Check to see if there is already an entry for this entity
    CallbackMapsPtr spEntityMap;
    if (!m_entityIDCBs.find(entityID, spEntityMap))
    {
        // If not, create a new one
        spEntityMap = EE_NEW CallbackMaps;

        // Add the callback map to the entity map at the entity ID's position
        m_entityIDCBs[entityID] = spEntityMap;
    }

    // Call the helper to add the wrappers to the entity map
    return RemoveNotification(
        notifyType,
        pData,
        spEntityMap);
}

//------------------------------------------------------------------------------------------------
void NotificationService::HandleCallbackMapCheck(
    efd::ClassID notifyType,
    efd::Category targetChannel,
    CallbackMaps* pMap,
    Entity* pEntity)
{
    // Get the correct list to look in
    CallbackList* pList = NULL;
    CallbackSet* pSet = NULL;
    switch (notifyType)
    {
    case kMSGID_NoticeEntityDiscovered:
        pList = &(pMap->m_callbacks.m_discovers);
        pSet = &m_discoversToSend;
        break;
    case kMSGID_NoticeEntityUpdated:
        pList = &(pMap->m_callbacks.m_updates);
        pSet = &m_updatesToSend;
        break;
    case kMSGID_NoticeEntityRemoved:
        pList = &(pMap->m_callbacks.m_removes);
        pSet = &m_removesToSend;
        break;
    }
    // Loop through the list of callbacks and add them to the set that should
    // be sent.
    CallbackList::iterator iterCBs;
    for (iterCBs = pList->begin(); iterCBs != pList->end(); iterCBs++)
    {
        // Add the entity to the callback wrapper
        (*iterCBs)->AddNotice(pEntity);
        // Add the callback wrapper to the set to call
        pSet->insert(*iterCBs);
    }
}

//------------------------------------------------------------------------------------------------
void NotificationService::HandleEntityMessage(
    efd::ClassID notifyType,
    const egf::EntityChangeMessage* pMessage,
    efd::Category targetChannel)
{
    if (pMessage)
    {
        FlatModelID modelID = 0;
        EntityPtr spEntity = pMessage->GetEntity();
        EE_ASSERT(spEntity);

        if (spEntity)
        {
            // Check to see what model this entity is
            modelID = spEntity->GetModelID();

            // Check to see if there are any callbacks registered for any model
            CallbackMapsPtr spModelMap;
            if (m_modelIDCBs.find(0, spModelMap))
            {
                HandleCallbackMapCheck(notifyType, targetChannel, spModelMap, spEntity);
            }

            // Check to see if there are any callbacks registered for this model
            efd::map<egf::FlatModelID, CallbackMapsPtr>::iterator it = m_modelIDCBs.begin();
            for (; it != m_modelIDCBs.end(); ++it)
            {
                if (spEntity->GetModel()->ContainsModel(it->first))
                {
                    HandleCallbackMapCheck(notifyType, targetChannel, it->second, spEntity);
                }
            }

            // Check to see if there are any callbacks registered for this entity
            CallbackMapsPtr spEntityMap;
            if (m_entityIDCBs.find(spEntity->GetEntityID(), spEntityMap))
            {
                HandleCallbackMapCheck(notifyType, targetChannel, spEntityMap, spEntity);
            }
        }
    }
}

