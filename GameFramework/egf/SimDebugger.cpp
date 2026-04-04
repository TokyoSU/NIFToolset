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

// SimDebugger is compiled out of Shipping configurations
#ifndef EE_CONFIG_SHIPPING

#include <egf/egfLogIDs.h>
#include <egf/Scheduler.h>
#include <egf/SimDebugger.h>
#include <egf/SimDebuggerMessages.h>
#include <egf/StandardModelLibraryFlatModelIDs.h>
#include <egf/StandardModelLibraryPropertyIDs.h>
#include <efd/ServiceManager.h>

using namespace efd;
using namespace egf;


EE_IMPLEMENT_CONCRETE_CLASS_INFO(SimDebugger);


/// The Category for messages sent from the Toolbench Sim Debugger.
static const efd::Category kCAT_FromToolbenchSimDebugger = efd::Category(
    efd::UniversalID::ECU_Any, efd::kNetID_Any, efd::kBASEID_FromToolbenchSimDebugger);
/// The Category for messages sent to the Toolbench Sim Debugger.
static const efd::Category kCAT_ToToolbenchSimDebugger = efd::Category(
    efd::UniversalID::ECU_Any, efd::kNetID_Any, efd::kBASEID_ToToolbenchSimDebugger);


SimDebuggerPtr SimDebugger::ms_spInstance;


//-------------------------------------------------------------------------------------------------
SimDebugger::SimDebugger()
    : m_initialized(false)
    , m_pServiceManager(NULL)
    , m_pMessageService(NULL)
    , m_pFlatModelManager(NULL)
    , m_pEntityManager(NULL)
    , m_pScheduler(NULL)
    , m_sessionActive(false)
    , m_positionUpdateThresholdSquared(50.0f * 50.0f)
    , m_rotationUpdateThreshold(45.0f)
    , m_nextActorBucket(0)
{
    EE_ASSERT_MESSAGE(NULL == ms_spInstance, ("Only one SimDebugger should be created!"));
}

//-------------------------------------------------------------------------------------------------
SimDebugger::~SimDebugger()
{
}

EE_HANDLER_WRAP(
    SimDebugger,
    HandleSimDebuggerCommand,
    efd::StreamMessage,
    kMSGID_SimDebuggerCommand);

//-------------------------------------------------------------------------------------------------
void SimDebugger::Initialize(ServiceManager* pServiceMgr)
{
    if (m_initialized)
        return;

    m_pServiceManager = pServiceMgr;
    m_pFlatModelManager = m_pServiceManager->GetSystemServiceAs<FlatModelManager>();
    m_pEntityManager = m_pServiceManager->GetSystemServiceAs<EntityManager>();
    m_pScheduler = m_pServiceManager->GetSystemServiceAs<Scheduler>();

    // Prepare to receive command messages from Toolbench
    m_pMessageService = pServiceMgr->GetSystemServiceAs<MessageService>(
        kCLASSID_ToolsMessageService);
    m_pMessageService->Subscribe(this, kCAT_FromToolbenchSimDebugger);

    m_initialized = true;
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::StartSession()
{
    if (m_sessionActive == false)
        return;

    m_sessionStartTime = m_pScheduler->GetGameTime();

    // Send notification to Toolbench
    DebugNoteSessionStartedPtr spDebugNote = EE_NEW DebugNoteSessionStarted();
    if (spDebugNote)
        SendEvent(spDebugNote);

    // Reset SimDebugger state
    m_flatModelsDefined.clear();
    for (efd::UInt32 i = 0; i < m_actors.size(); ++i)
    {
        m_actors[i].clear();
    }
    m_properties.clear();
    // Position and Rotation have special update handling via UpdateActorSimEvent
    m_properties[m_pFlatModelManager->GetPropertyIDByName("Position")] = UINT_MAX;
    m_properties[m_pFlatModelManager->GetPropertyIDByName("Rotation")] = UINT_MAX;

    // Update Toolbench with current entities
    Entity* pEntity;
    EntityManager::EntityMap::const_iterator eePos = m_pEntityManager->GetFirstEntityPos();
    while (m_pEntityManager->GetNextEntity(eePos, pEntity))
    {
        StartTrackingEntity(pEntity);
    }

    // Allow other instrumented code to send initial session state to Toolbench
    // Be sure to use the standard MessageService, not tools
    StreamMessagePtr spMessage = EE_NEW MessageWrapper<StreamMessage, kMSGID_SimDebuggerStartup>;
    m_pServiceManager->GetSystemServiceAs<MessageService>()->SendLocal(spMessage,
        kCAT_LocalMessage);
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::DefineFlatModel(const egf::FlatModel* pFlatModel)
{
    // Check whether this model definition has already been sent
    if (m_flatModelsDefined.find(pFlatModel->GetID()) != m_flatModelsDefined.end())
        return;

    DebugNoteFlatModelDefinedPtr spDebugNote = EE_NEW DebugNoteFlatModelDefined(
        pFlatModel->GetName());
    if (spDebugNote == NULL)
        return;

    // Classify model
    if (pFlatModel->ContainsModel("Mesh"))
        spDebugNote->SetIsMesh();
    if (pFlatModel->ContainsModel("Actor"))
        spDebugNote->SetIsActor();

    // Add all model properties
    efd::list<efd::utf8string> propertyNames;
    pFlatModel->GetPropertyNames(propertyNames);
    efd::utf8string value;

    efd::list<efd::utf8string>::const_iterator nameIt = propertyNames.begin();
    for (; nameIt != propertyNames.end(); ++nameIt)
    {
        const PropertyDescriptor* pPropertyDescriptor = pFlatModel->GetPropertyDescriptor(*nameIt);
        egf::PropertyID propertyID = pPropertyDescriptor->GetPropertyID();
        const egf::IProperty* pDefaultProperty = pPropertyDescriptor->GetDefaultProperty();

        efd::UInt32 valueCount;
        if (pDefaultProperty->GetPropertyCount(propertyID, valueCount) == PropertyResult_OK)
        {
            // Record each item in collection property
            efd::utf8string key;
            efd::utf8string prevKey = "";
            pDefaultProperty->GetNextPropertyKey(propertyID, prevKey, key);
            while (!key.empty())
            {
                efd::utf8string propNameKey;
                propNameKey.sprintf("%s[%s]", nameIt->c_str(), key.c_str());

                if (pDefaultProperty->GetValueAsString(propertyID, key, value) == PropertyResult_OK)
                    spDebugNote->AddProperty(propNameKey, value);
                else
                    spDebugNote->AddProperty(propNameKey, "???");

                prevKey = key;
                pDefaultProperty->GetNextPropertyKey(propertyID, prevKey, key);
            }
        }
        else
        {
            // Record scalar property
            if (pDefaultProperty->GetValueAsString(propertyID, value) == PropertyResult_OK)
                spDebugNote->AddProperty(*nameIt, value);
            else
                spDebugNote->AddProperty(*nameIt, "???");
        }
    }

    // Notify Toolbench of the model definition
    SendEvent(spDebugNote);

    // Note that this model definition has been handled
    m_flatModelsDefined.insert(pFlatModel->GetID());
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::StartTrackingEntity(const egf::Entity* pEntity)
{
    if (m_sessionActive == false)
        return;

    // Be sure Toolbench is aware of the model used to create this entity
    DefineFlatModel(pEntity->GetModel());

    // Find initial position
    Point3 position;
    if (pEntity->GetPropertyValue(egf::kPropertyID_StandardModelLibrary_Position, position) !=
        PropertyResult_OK)
    {
        position = Point3::ZERO;
    }

    // Find initial facing
    Point3 rotation;
    if (pEntity->GetPropertyValue(egf::kPropertyID_StandardModelLibrary_Rotation, rotation) !=
        PropertyResult_OK)
    {
        rotation = Point3::ZERO;
    }

    // Prepare the entity creation notification
    DebugNoteEntityCreatedPtr spDebugNote = EE_NEW DebugNoteEntityCreated(
        pEntity->GetEntityID(),
        pEntity->GetDataFileID(),
        pEntity->GetModelName(),
        position,
        rotation.z);
    if (spDebugNote == NULL)
        return;

    // Add overridden properties
    efd::list<efd::utf8string> propertyNames;
    pEntity->GetModel()->GetPropertyNames(propertyNames);
    efd::utf8string value;

    efd::list<efd::utf8string>::const_iterator nameIt = propertyNames.begin();
    for (; nameIt != propertyNames.end(); ++nameIt)
    {
        // Check whether property is overridden by the entity
        const PropertyDescriptor* pPropertyDescriptor = pEntity->GetModel()->GetPropertyDescriptor(
            *nameIt);
        egf::PropertyID propertyID = pPropertyDescriptor->GetPropertyID();
        if (pEntity->IsPropertyOverridden(propertyID))
        {
            efd::UInt32 valueCount;
            if (pEntity->GetPropertyCount(propertyID, valueCount) == PropertyResult_OK)
            {
                // Record each item in collection property
                efd::utf8string key;
                efd::utf8string prevKey = "";
                pEntity->GetNextPropertyKey(propertyID, prevKey, key);
                while (!key.empty())
                {
                    efd::utf8string propNameKey;
                    propNameKey.sprintf("%s[%s]", nameIt->c_str(), key.c_str());

                    if (pEntity->GetValueAsString(propertyID, key, value) == PropertyResult_OK)
                        spDebugNote->AddProperty(propNameKey, value);
                    else
                        spDebugNote->AddProperty(propNameKey, "???");

                    prevKey = key;
                    pEntity->GetNextPropertyKey(propertyID, prevKey, key);
                }
            }
            else
            {
                // Record scalar property
                if (pEntity->GetValueAsString(propertyID, value) == PropertyResult_OK)
                    spDebugNote->AddProperty(*nameIt, value);
                else
                    spDebugNote->AddProperty(*nameIt, "???");
            }
        }
    }

    // Send notification to Toolbench
    SendEvent(spDebugNote);

    // Prepare for periodic updates if entity is an Actor
    if (pEntity->GetModel()->ContainsModel(
        egf::kFlatModelID_StandardModelLibrary_Actor))
    {
        // Store tracking data
        ActorTrackingData data;
        efd::UInt32 bucket = (efd::UInt32)(pEntity->GetEntityID().GetBaseID() % m_actors.size());
        if (m_actors[bucket].find(pEntity, data) == false)
        {
            ActorTrackingData newData;
            newData.m_lastReportedPosition = position;
            newData.m_lastReportedFacing = rotation.z;

            m_actors[bucket].insert(ActorDataMap::value_type(pEntity, newData));
        }
        else
        {
            EE_ASSERT_MESSAGE(false,
                ("Already tracking %s", pEntity->GetEntityID().ToString().c_str()));
        }
    }
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::StopTrackingEntity(const egf::Entity* pEntity)
{
    if (m_sessionActive == false)
        return;

    // Purge tracking data
    m_actors[(efd::UInt32)(pEntity->GetEntityID().GetBaseID() % m_actors.size())].erase(pEntity);

    // Send notification to Toolbench
    DebugNoteEntityDestroyedPtr spDebugNote = EE_NEW DebugNoteEntityDestroyed(
        pEntity->GetEntityID());
    if (spDebugNote)
        SendEvent(spDebugNote);
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::SendPropertyUpdate(
    egf::Entity* pEntity,
    egf::PropertyID propID,
    const egf::IProperty* pProperty)
{
    // Rapid iteration on flat models is not supported by the Sim Debugger
    if (pProperty == NULL)
        return;

    // Make sure entity has been fully initialized
    if (pEntity->GetEntityManager() == NULL)
        return;

    // Check whether updates have been disabled for this property. A heuristic is used to determine
    // whether a given property is flooding the network with updates.
    efd::UInt32 updateCount;
    if (m_properties.find(propID, updateCount))
    {
        if (updateCount == UINT_MAX)
        {
            // Property has already been blacklisted for updates
            return;
        }
        else if (updateCount > (efd::UInt32)(m_pScheduler->GetGameTime() - m_sessionStartTime) + 16)
        {
            m_properties[propID] = UINT_MAX;

            // Notify Toolbench that property is going stale
            SendEvent("Emergent.Toolbench.SimDebugger.PropertyDroppedSimEvent", 
                m_pFlatModelManager->GetPropertyNameByID(propID));
            return;
        }
        else
        {
            ++m_properties[propID];
        }
    }
    else
    {
        m_properties[propID] = 1;
    }

    efd::utf8string value;
    efd::UInt32 valueCount;
    if (pProperty->GetPropertyCount(propID, valueCount) == PropertyResult_OK)
    {
        // Collection property
        DebugNoteCollectionPropertyChangedPtr spDebugNote =
            EE_NEW DebugNoteCollectionPropertyChanged(pEntity->GetEntityID(),
                m_pFlatModelManager->GetPropertyNameByID(propID));

        // Record each item
        efd::utf8string key;
        efd::utf8string prevKey = "";
        pProperty->GetNextPropertyKey(propID, prevKey, key);
        while (!key.empty())
        {
            if (pProperty->GetValueAsString(propID, key, value) == PropertyResult_OK)
                spDebugNote->AddItem(key, value);
            else
                spDebugNote->AddItem(key, "???");

            prevKey = key;
            pProperty->GetNextPropertyKey(propID, prevKey, key);
        }

        SendEvent(spDebugNote);
    }
    else
    {
        // Scalar property
        if (pProperty->GetValueAsString(propID, value) != PropertyResult_OK)
            value = "???";
        DebugNotePropertyChangedPtr spDebugNote = EE_NEW DebugNotePropertyChanged(
            pEntity->GetEntityID(), m_pFlatModelManager->GetPropertyNameByID(propID), value);
        SendEvent(spDebugNote);
    }
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::OnTick()
{
    if (m_actors.size() == 0)
        return;

    // Process only one bucket of Actor entities per tick
    ActorDataMap::iterator i = m_actors[m_nextActorBucket].begin();
    ActorDataMap::iterator end = m_actors[m_nextActorBucket].end();
    for (; i != end; ++i)
    {
        // Get current position
        efd::Point3 position;
        if (i->first->GetPropertyValue(egf::kPropertyID_StandardModelLibrary_Position, position) !=
            egf::PropertyResult_OK)
        {
            position = efd::Point3::ZERO;
        }
        // Get current facing
        efd::Point3 rotation;
        if (i->first->GetPropertyValue(egf::kPropertyID_StandardModelLibrary_Rotation, rotation) !=
            egf::PropertyResult_OK)
        {
            rotation = efd::Point3::ZERO;
        }

        // Find deltas since last notification
        // Note that position test is in 3D, rotation check is yaw only
        efd::Float32 posDiff2 = (position - i->second.m_lastReportedPosition).SqrLength();
        efd::Float32 angleDiff = abs(rotation.z - i->second.m_lastReportedFacing);
        if (angleDiff > 180.0f)
            angleDiff = 360.0f - angleDiff;

        // Check whether entity has changed enough to merit an update
        if (posDiff2 > m_positionUpdateThresholdSquared ||
            angleDiff > m_rotationUpdateThreshold)
        {
            // Send notification to Toolbench
            efd::utf8string payloads;
            payloads.sprintf("%f|%f|%f|%f|%f|%f", position.x, position.y, position.z, rotation.x,
                rotation.y, rotation.z);
            SendEvent("Emergent.Toolbench.SimDebugger.UpdateActorSimEvent", payloads,
                i->first->GetEntityID());

            // Store updated tracking data
            i->second.m_lastReportedPosition = position;
            i->second.m_lastReportedFacing = rotation.z;
        }
    }

    // Move on to next bucket
    if (++m_nextActorBucket == m_actors.size())
        m_nextActorBucket = 0;
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::Shutdown()
{
    if (m_initialized)
    {
        // Cleanup messaging
        m_pMessageService->Unsubscribe(this, kCAT_FromToolbenchSimDebugger);
    }
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::HandleSimDebuggerCommand(
    const efd::StreamMessage* pMessage,
    efd::Category targetChannel)
{
    EE_LOG(efd::kEntity, efd::ILogger::kLVL2, ("SimDebugger: Request received from Toolbench"));

    pMessage->ResetForUnpacking();

    // Find command
    utf8string command;
    *pMessage >> command;

    if (command == "BEGIN_SESSION")
    {
        // Toolbench has requested a session
        efd::Bool forceRestart;
        *pMessage >> forceRestart;
        if (m_sessionActive && !forceRestart)
            return;

        // Read configuration information
        efd::UInt32 bucketCount;
        *pMessage >> bucketCount;
        if (bucketCount > 1000)
            bucketCount = 1000;
        m_actors.resize(0);
        m_actors.resize(bucketCount);
        m_nextActorBucket = 0;

        efd::Float32 positionThreshold;
        *pMessage >> positionThreshold;
        m_positionUpdateThresholdSquared = positionThreshold * positionThreshold;

        *pMessage >> m_rotationUpdateThreshold;

        m_sessionActive = true;
        StartSession();
    }
    else if (command == "SET_PROPERTY")
    {
        // Get parameters of set property request
        egf::EntityID entityID;
        *pMessage >> entityID;
        efd::utf8string propertyName;
        *pMessage >> propertyName;
        efd::utf8string propertyKey;
        *pMessage >> propertyKey;
        efd::utf8string propertyValue;
        *pMessage >> propertyValue;

        // Find the entity to modify
        Entity* pEntity = m_pEntityManager->LookupEntity(entityID);
        if (!pEntity)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                ("Sim Debugger failed to set property, entity %s not found",
                entityID.ToString().c_str()));
            return;
        }

        // Attempt to set the property value
        egf::PropertyResult result;
        if (propertyKey.empty())
        {
            // Scalar property
            result = pEntity->SetPropertyValueByString(propertyName, propertyValue);
        }
        else
        {
            // Collection property
            result = pEntity->SetPropertyValueByString(propertyName, propertyKey, propertyValue);
        }
        if (result != egf::PropertyResult_OK)
        {
            EE_LOG(efd::kEntity, efd::ILogger::kERR1,
                ("Sim Debugger failed to set property %s on entity %s (result %d)",
                entityID.ToString().c_str(), propertyName.c_str(), result));
        }
    }
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::SendEvent(egf::DebugNoteBase* pMessage)
{
    if (m_sessionActive == false)
        return;

    // Send the event message to Toolbench
    pMessage->SetTimestamp(m_pScheduler->GetGameTime());
    m_pMessageService->Send(pMessage, kCAT_ToToolbenchSimDebugger);
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::SendEvent(
    const efd::utf8string& eventName,
    egf::EntityID gameID /*= egf::kENTITY_INVALID*/)
{
    if (m_sessionActive == false)
        return;

    // Construct a generic event message and send to Toolbench
    DebugNoteEventPtr spDebugNote = EE_NEW DebugNoteEvent(gameID, eventName, "");
    if (spDebugNote)
    {
        spDebugNote->SetTimestamp(m_pScheduler->GetGameTime());
        m_pMessageService->Send(spDebugNote, kCAT_ToToolbenchSimDebugger);
    }
}

//-------------------------------------------------------------------------------------------------
void SimDebugger::SendEvent(
    const efd::utf8string& eventName,
    const efd::utf8string& params,
    egf::EntityID gameID /*= egf::kENTITY_INVALID*/)
{
    if (m_sessionActive == false)
        return;

    // Construct a generic event message and send to Toolbench
    DebugNoteEventPtr spDebugNote = EE_NEW DebugNoteEvent(gameID, eventName, params);
    if (spDebugNote)
    {
        spDebugNote->SetTimestamp(m_pScheduler->GetGameTime());
        m_pMessageService->Send(spDebugNote, kCAT_ToToolbenchSimDebugger);
    }
}

#endif // EE_CONFIG_SHIPPING
