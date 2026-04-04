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

#include <efd/ILogger.h>
#include <efd/Metrics.h>
#include <egf/EventMessage.h>
#include <efdNetwork/NetCategory.h>
#include <efd/ServiceManager.h>


//------------------------------------------------------------------------------------------------
using namespace efd;
using namespace egf;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(EventMessage);


//------------------------------------------------------------------------------------------------
EventMessage::EventMessage()
    : m_eventID(EventID::CreateEventID())
    , m_senderID(0)
    , m_behaviorID(0)
    , m_mixinModelID(0)
    , m_invokeModelID(0)
    , m_spParameterList()
    , m_delay(0.0f)
    , m_needResponse(false)
{
}


//------------------------------------------------------------------------------------------------
EventMessage::EventMessage(efd::ClassID msgClassID,
                            EntityID senderID,
                            EventID eventID,
                            BehaviorID behaviorID,
                            FlatModelID mixinModelID,
                            FlatModelID invokeModelID,
                            efd::TimeType delay /*= 0*/,
                            ParameterList* pParams /*= NULL*/,
                            bool needResponse /*= false*/)
    : m_senderID(senderID)
    , m_behaviorID(behaviorID)
    , m_mixinModelID(mixinModelID)
    , m_invokeModelID(invokeModelID)
    , m_spParameterList(pParams)
    , m_delay(delay)
    , m_needResponse(needResponse)
{
    switch (msgClassID)
    {
    case kMSGID_EventReturn:
    case kMSGID_EventCancel:
        // for return and cancel use eventID given
        m_eventID = eventID;
        break;
    default:

    case kMSGID_Event:
        // get new event ID for event message
        m_eventID = EventID::CreateEventID();
        break;
    }
}




//------------------------------------------------------------------------------------------------
/*static*/ EventMessage* EventMessage::CreateEvent(EntityID senderID,
                                      FlatModelID mixinModelID,
                                      FlatModelID invokeModelID,
                                      BehaviorID behaviorID,
                                      efd::TimeType delay /*= 0*/,
                                      ParameterList* pParams /*= NULL*/,
                                      bool needResponse /*= false*/)
{
    EventMessage* pEventMsg = EE_NEW MessageWrapper< EventMessage, kMSGID_Event >;
    EE_ASSERT(pEventMsg);
    pEventMsg->SetSenderID(senderID);
    pEventMsg->SetEventID(EventID::CreateEventID());
    pEventMsg->SetBehavior(mixinModelID, invokeModelID, behaviorID, pParams, needResponse);
    pEventMsg->SetDelay(delay);
    return pEventMsg;
}


//------------------------------------------------------------------------------------------------
/*static*/ EventMessage* EventMessage::CreateReturn(EntityID senderID,
                                       EventID eventID,
                                       ParameterList* pReturnVals /*= NULL*/)
{
    EventMessage* pEventMsg = EE_NEW MessageWrapper< EventMessage, kMSGID_EventReturn >;
    EE_ASSERT(pEventMsg);
    pEventMsg->SetSenderID(senderID);
    pEventMsg->SetEventID(eventID);
    pEventMsg->SetParams(pReturnVals);
    pEventMsg->SetNeedResponse(false);
    return pEventMsg;
}


//------------------------------------------------------------------------------------------------
/*static*/ EventMessage* EventMessage::CreateCancel(EntityID senderID, EventID eventID)
{
    EventMessage* pEventMsg = EE_NEW MessageWrapper< EventMessage, kMSGID_EventCancel >;
    EE_ASSERT(pEventMsg);
    pEventMsg->SetSenderID(senderID);
    pEventMsg->SetEventID(eventID);
    pEventMsg->SetNeedResponse(false);
    return pEventMsg;
}


//------------------------------------------------------------------------------------------------
/*virtual*/ void EventMessage::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);

    Serializer::SerializeObject(m_eventID, ar);

    switch (GetClassID())
    {
    case kMSGID_Event:
        Serializer::SerializeObject(m_senderID, ar);
        Serializer::SerializeObject(m_behaviorID, ar);
        Serializer::SerializeObject(m_mixinModelID, ar);
        Serializer::SerializeObject(m_invokeModelID, ar);
        Serializer::SerializeObject(m_delay, ar);
        // Serialize a potentially NULL pointer:
        SR_Allocate<SR_DefaultAllocator, SR_Default, true>::Serialize(m_spParameterList, ar);
        Serializer::SerializeObject(m_needResponse, ar);
        break;

    case kMSGID_EventReturn:
        Serializer::SerializeObject(m_senderID, ar);
        // Serialize a potentially NULL pointer:
        SR_Allocate<SR_DefaultAllocator, SR_Default, true>::Serialize(m_spParameterList, ar);
        break;

    case kMSGID_EventCancel:
        // no additional parameters for cancel (just need event ID)
        break;

    default:
        EE_FAIL("Invalid EventMessage type");
        ar.RaiseError();
        break;
    }
}

//------------------------------------------------------------------------------------------------
/*virtual*/ void EventMessage::SetBehavior(FlatModelID mixinModelID,
                                            FlatModelID invokeModelID,
                                            BehaviorID behaviorID,
                                            ParameterListPtr spStrm,
                                            bool needResponse)
{
    m_mixinModelID = mixinModelID;
    m_invokeModelID = invokeModelID;
    m_behaviorID = behaviorID;
    m_spParameterList = spStrm;
    m_needResponse = needResponse;
}


