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

#include <efd/MessageService.h>
#include <efd/StreamMessage.h>
#include <efd/ServiceManager.h>
#include <efd/Metrics.h>
#include <efd/SystemLogger.h>
#include <efd/Utilities.h>
#include <efd/INetService.h>
#include <efd/NetMessage.h>
#include <efd/efdLogIDs.h>
#include <efd/QOSCompare.h>
#include <efd/Profiler.h>

//------------------------------------------------------------------------------------------------
using namespace efd;
//------------------------------------------------------------------------------------------------
EE_PROFILER_CONTEXT_IMPL(MessageService); // Previously defined inside MessageHandlerBase.h
//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(MessageService);
BaseMessageHandler* efd::BaseMessageHandler::s_firstMessageHandler=NULL;
//------------------------------------------------------------------------------------------------
MessageService::MessageService()
    : m_pMessageList(NULL)
    , m_pMessageListInternal(NULL)
    , m_spNetService(NULL)
    , m_ticking(0)
    , m_initialized(false)
    , m_sendRemote(false)
    , m_subscriptionsDirty(false)
    , m_offlineMode(false)
    , m_lastHandlerProcessed(NULL)
{
    // If this default priority is changed, also update the service quick reference documentation
    m_defaultPriority = 6000;

    // register any statically declared message handlers
    RegisterMessageHandlers();


    // Lock the message list and remove all the elements
    m_messageListLock.Lock();

    m_pMessageList = EE_NEW MessageList();
    EE_ASSERT(m_pMessageList);
    m_messageListLock.Unlock();

    m_pMessageListInternal = EE_NEW MessageList();
    EE_ASSERT(m_pMessageListInternal);

    // Lock the delayed message list and remove all the elements
    m_delayedMessageListLock.Lock();
    m_delayedMessageList.clear();
    m_delayedMessageListLock.Unlock();
}

//------------------------------------------------------------------------------------------------
MessageService::~MessageService()
{
    if (m_pMessageList)
        EE_DELETE m_pMessageList;
    if (m_pMessageListInternal)
        EE_DELETE m_pMessageListInternal;

    IMessage::DeleteParentClassMap();
}

//------------------------------------------------------------------------------------------------
SyncResult MessageService::OnPreInit(efd::IDependencyRegistrar* pDependencyRegistrar)
{
    pDependencyRegistrar->AddDependency<INetService>(sdf_Optional);
    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult MessageService::OnInit()
{
    // If there is a NetService don't return AsyncResult_Complete until it has initialized to the
    // point of assigning a NetID.
    if (m_spNetService && m_spNetService->GetNetID() == kNetID_Unassigned)
    {
        return OnTick();
    }
    EE_LOG(
        efd::kMessageService,
        efd::ILogger::kLVL0,
        ("MessageService::OnInit Complete, m_sendRemote=%d", m_sendRemote));
    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult MessageService::OnTick()
{
    // Subscribe / Unsubscribe targets that are waiting
    RegisterMessageHandlers();
    SubscribeUnsubscribe();

    // Process posted and delayed messages
    ProcessMessages();
    return efd::AsyncResult_Pending;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult MessageService::OnShutdown()
{
    if (m_spNetService)
    {
        m_spNetService->Unsubscribe(this);
    }

    // Lock the message list and remove all the elements
    m_messageListLock.Lock();
    m_pMessageList->clear();
    m_messageListLock.Unlock();
    m_pMessageListInternal->clear();

    // Lock the delayed message list and remove all the elements
    m_delayedMessageListLock.Lock();
    m_delayedMessageList.clear();
    m_delayedMessageListLock.Unlock();

    m_targetsListsByCategory.clear();
    m_producerListsByCategory.clear();

    m_spNetService = NULL;
    m_sendRemote = false;

	DeleteCachedHandlers();

    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
bool MessageService::GetSendRemote()
{
    return m_sendRemote;
}

//------------------------------------------------------------------------------------------------
void MessageService::SetSendRemote(bool sendRemote)
{
    EE_LOG(
        efd::kMessageService,
        efd::ILogger::kLVL0,
        ("MessageService::SetSendRemote %d", sendRemote));
    m_sendRemote = sendRemote;
    // MessageService must have been added to ServiceManager before the connection can be
    // established
    EE_ASSERT(m_pServiceManager);
    m_pServiceManager->SetVirtualProcessID(m_spNetService->GetNetID());
}

//------------------------------------------------------------------------------------------------
void MessageService::SetNetService(efd::INetService* pNetService)
{
    if (pNetService)
    {
        EE_ASSERT(!m_spNetService);
        m_spNetService = pNetService;
        m_spNetService->Subscribe(kCAT_SendToProducer, this);
        // we now have a NetService pointer, iterate any existing Subscriptions and notify
        // NetService of them
        NetSubscribeUnsubscribe();
    }
}

//------------------------------------------------------------------------------------------------
void  MessageService::NetSubscribeUnsubscribe()
{
    for (TargetSubscriptionList::iterator iter = m_pendingNetSubscriptions.begin();
        iter != m_pendingNetSubscriptions.end();
        ++iter)
    {
        Category targetChannel = (*iter).m_targetChannel;
        QualityOfService qualityOfService = (*iter).m_qualityOfService;

        switch ((*iter).m_subscribe)
        {
        case TargetSubscription::SUBSCRIBE:
            m_spNetService->Subscribe(targetChannel, this);
            break;
        case TargetSubscription::PRODUCER_SUBSCRIBE:
            EE_VERIFY(m_spNetService->BeginCategoryProduction(
                targetChannel,
                qualityOfService,
                this));
            break;
        default:
            EE_FAIL("Unsubscribe before NetService and MessageService are both added "
                "to ServiceManager is currently unsupported.");
        }
    }
    m_pendingNetSubscriptions.clear();
}

//------------------------------------------------------------------------------------------------
void MessageService::UpdateOrCreateCachedHandlers( const Category& targetChannel, 
    const IMessage* pMessage, HandlersForMessAndCatArray* pHandlers )
{
	// all systems subscribed to category
	TargetsListsByCategory* pTargetsListsByCategory = &m_targetsListsByCategory;
	TargetsListsByCategory::iterator mapEltForCat = pTargetsListsByCategory->find(targetChannel);

	// if any subscribed to the category...
	if (mapEltForCat != pTargetsListsByCategory->end())
	{
		TargetsList::iterator firstTargetForCat = mapEltForCat->second.begin();

		// all handlers for this message
		efd::ClassID msgClassID = pMessage->GetClassID();
		HandlersListsByMessageClass::iterator handlersForMsgClass = m_handlersListsByMessageClass.find(msgClassID);

		// if any handls this message...
		if( handlersForMsgClass != m_handlersListsByMessageClass.end() )
		{
			MessageHandlersList::iterator possibleHandler = handlersForMsgClass->second.begin();

			// double loop to detect those handlers that actually handle the systems
			while (possibleHandler != handlersForMsgClass->second.end())
			{
				TargetsList::iterator possibleTarget = firstTargetForCat; // copy the iterator

				while (possibleTarget != mapEltForCat->second.end())
				{
					// check if this will be handled
					if( (*possibleHandler)->CanHandleMessage( (*possibleTarget).first, pMessage ) )
					{
						struct BaseAndHandler baseAndHandler;
						baseAndHandler.m_pBase = (*possibleTarget).first;
						baseAndHandler.m_pHandler = *possibleHandler;

						// add base and handler to array
						pHandlers->push_back( baseAndHandler );
					}

					++possibleTarget;
				}
				++possibleHandler;
			}
		}
	}
}

//------------------------------------------------------------------------------------------------
void MessageService::SetCacheHandlersDirty()
{
	AllHandlersForAllCats::iterator allHandlersIter = m_AllHandlersForAllCats.begin( );
	while( allHandlersIter != m_AllHandlersForAllCats.end() )
	{
		allHandlersIter->second.m_bDirty = true;
		++allHandlersIter;
	}
}

//------------------------------------------------------------------------------------------------
void MessageService::DeleteCachedHandlers()
{
	AllHandlersForAllCats::iterator allHandlersIter = m_AllHandlersForAllCats.begin( );
	while( allHandlersIter != m_AllHandlersForAllCats.end() )
	{
		if( allHandlersIter->second.m_pHandlersForMessAndCat )
		{
			delete allHandlersIter->second.m_pHandlersForMessAndCat;
			allHandlersIter->second.m_pHandlersForMessAndCat = NULL;
		}
		allHandlersIter->second.m_bDirty = true;
		++allHandlersIter;
	}
}

//------------------------------------------------------------------------------------------------
void MessageService::SendImmediate(
    const IMessage* pMessage,
    Category targetChannel,
    bool producerMessage)
{
    EE_PROFILER_ZONE(MessageService, 0, "MessageService::SendImmediate");

    // make sure any pending subscriptions are processed before we attempt to send the message
    // we can't process subscriptions if OnTick is in progress as it is iterating the subscriptions
    // that need updating
    if (!m_ticking && m_subscriptionsDirty)
    {
        SubscribeUnsubscribe();
    }

    ++m_ticking;
    if (!pMessage)
    {
        EE_LOG(
            efd::kMessageService,
            efd::ILogger::kERR0,
            ("Error: NULL Message - Failed to send a new Message\n"));
        --m_ticking;
        return;
    }

    EE_MESSAGE_BREAK(
        pMessage,
        IMessage::mdf_BreakOnFirstLocalDelivery,
        ("%s| First Local Delivery", pMessage->GetDescription().c_str()));

    EE_LOG_METRIC_COUNT(kMessageService, "SEND.MESSAGE.LOCAL.IMMEDIATE");

	// if those conditions met, use (faster) path to handle messages
	if((targetChannel == kCAT_LocalMessage) && !producerMessage )
	{
		// message/category pair
		MessageAndCategory	messAndCat;
		//messAndCat.m_Category = targetChannel;
		messAndCat.m_MsgClassID = pMessage->GetClassID();

		// try to find handlers for Message/Category pair
		AllHandlersForAllCats::iterator allHandlersIter = m_AllHandlersForAllCats.find( messAndCat );
		HandlersForMessAndCatArray* pHandlers = NULL;

		if( allHandlersIter == m_AllHandlersForAllCats.end() )
		{
			// no handlers cached for this message. create them.
			HandlersForMessageAndCategory handlersForMAndC;
			handlersForMAndC.m_pHandlersForMessAndCat = new HandlersForMessAndCatArray;
			pHandlers = handlersForMAndC.m_pHandlersForMessAndCat;

			// find and fill array of handlers
			UpdateOrCreateCachedHandlers( targetChannel, pMessage, handlersForMAndC.m_pHandlersForMessAndCat );
			handlersForMAndC.m_bDirty = false;

			// Insert into the map. array will be actually filled after that
			EE_STL_NAMESPACE::pair<AllHandlersForAllCats::iterator, bool> insertedHandlersIter = 
				m_AllHandlersForAllCats.insert( EE_STL_NAMESPACE::pair<MessageAndCategory, HandlersForMessageAndCategory>( 
					messAndCat, handlersForMAndC) );
			EE_ASSERT( insertedHandlersIter.second );
		}
		else
		{
			pHandlers = allHandlersIter->second.m_pHandlersForMessAndCat;
			if( allHandlersIter->second.m_bDirty )
			{
				// clear previos handlers, we will re-create them
				pHandlers->clear();
				
				// find and fill array of handlers
				UpdateOrCreateCachedHandlers( targetChannel, pMessage, pHandlers );
				allHandlersIter->second.m_bDirty = false;
			}
		}

		// use handlers for the message/cat pair

		if( pHandlers )
		{
			HandlersForMessAndCatArray::iterator handlersIter = pHandlers->begin();
			const HandlersForMessAndCatArray::iterator handlersEnd = pHandlers->end();
			while( handlersIter != handlersEnd )
			{
				efd::IBase* pBase = (*handlersIter).m_pBase;
				efd::BaseMessageHandler* pHandler = (*handlersIter).m_pHandler;
				pHandler->HandleMessage( pBase, pMessage, targetChannel );
				++handlersIter;
			}
		}
	    --m_ticking;
		return;
	}

    TargetsListsByCategory* pTargetsListsByCategory;
    if (producerMessage)
    {
        pTargetsListsByCategory = &m_producerListsByCategory;
    }
    else
    {
        pTargetsListsByCategory = &m_targetsListsByCategory;
    }
    TargetsListsByCategory::iterator mapEltForCat =
        pTargetsListsByCategory->find(targetChannel);

    // If no one wants this message, drop it
    if (mapEltForCat == pTargetsListsByCategory->end())
    {
        if (!producerMessage)
        {
            EE_MESSAGE_TRACE(
                pMessage,
                efd::kMessageTrace,
                efd::ILogger::kERR3,
                efd::ILogger::kLVL2,
                ("%s| no local subscribers on channel %s",
                pMessage->GetDescription().c_str(),
                targetChannel.ToString().c_str()));
        }
        else
        {
            EE_MESSAGE_TRACE(
                pMessage,
                efd::kMessageTrace,
                efd::ILogger::kERR3,
                efd::ILogger::kLVL2,
                ("%s| no local producer subscribers on channel %s",
                pMessage->GetDescription().c_str(),
                targetChannel.ToString().c_str()));
        }
        --m_ticking;
        return;
    }

    TargetsList::iterator firstTargetForCat = mapEltForCat->second.begin();

    efd::ClassID msgClassID = pMessage->GetClassID();
    HandlersListsByMessageClass::iterator handlersForMsgClass =
        m_handlersListsByMessageClass.find(msgClassID);

    // If there are no handlers for this message, drop it
    if (handlersForMsgClass == m_handlersListsByMessageClass.end())
    {
        EE_MESSAGE_TRACE(
            pMessage,
            efd::kMessageTrace,
            efd::ILogger::kERR1,
            efd::ILogger::kLVL2,
            ("%s| no local handlers on channel %s",
            pMessage->GetDescription().c_str(),
            targetChannel.ToString().c_str()));
        --m_ticking;
        return;
    }

    // We always associate the handler with the most-derived message class, even if the handler
    // expects a less derived message class as an argument. I.e. there is no support for
    // subscribing to all message classes that derive from one with a given classID.
    MessageHandlersList::iterator possibleHandler = handlersForMsgClass->second.begin();

    // For each method handler that understands this message class...
    // Note that some of these possible handlers will not be for the class of target we
    // consider below, and we will skip running them. This is because the handler list is
    // currently global, not per target class.
    // There are complexity issues with base classes and subclasses of targets that are hard
    // to deal with in such a lookup list. The below solution relies instead on dynamic cast,
    // which will get it right, even though this is more expensive. We argue that there will not
    // be a large number of handlers per message type.
    while (possibleHandler != handlersForMsgClass->second.end())
    {
        // Now consider each target that wants the Category...
        // Note that only some targets on this category registered a handler for this class
        //   of message.
        TargetsList::iterator possibleTarget = firstTargetForCat; // copy the iterator

        // If we find the # of targets considered unacceptably high, it is time to think about
        // a better application-specific categorization scheme. Break the message sequences into
        // smaller pieces/direct the sequences more tightly. But don't feel forced to send things
        // point to point.
        while (possibleTarget != mapEltForCat->second.end())
        {
            // HandleMessage casts the target and message to the desired type, and only if they
            // both match calls the method.
            (*possibleHandler)->HandleMessage((*possibleTarget).first, pMessage, targetChannel);

            ++possibleTarget;
        }

        ++possibleHandler;
    }

    --m_ticking;
}

//------------------------------------------------------------------------------------------------
void MessageService::SendLocal(
    const IMessage* pMessage,
    Category targetChannel,
    bool isProducerMessage)
{
    if (!pMessage)
    {
        EE_LOG(
            efd::kMessageService,
            efd::ILogger::kERR0,
            ("Error: NULL Message - Failed to post a new  Message\n"));
        return;
    }

    EE_MESSAGE_BREAK(
        pMessage,
        IMessage::mdf_BreakOnLocalPost,
        ("%s| SendLocal to %s for producer=%d",
        pMessage->GetDescription().c_str(),
        targetChannel.ToString().c_str(),
        (int)isProducerMessage));

    EE_LOG_METRIC_COUNT(kMessageService, "SEND.MESSAGE.LOCAL");

    // Insert it at the end of the list
    m_messageListLock.Lock();
    m_pMessageList->push_back(MessageWithCategory(pMessage, targetChannel, isProducerMessage));
    m_messageListLock.Unlock();
}

//------------------------------------------------------------------------------------------------
void MessageService::SendLocalDelayed(
    const IMessage* pMessage,
    efd::TimeType fDelay,
    Category targetChannel)
{
    if (!pMessage)
    {
        EE_LOG(
            efd::kMessageService,
            efd::ILogger::kERR0,
            ("Error: NULL Message - Failed to post a new Delayed Message\n"));
        return;
    }

    // Calculate the time the Delayed Message is supposed to be delivered
    efd::TimeType fTime = efd::GetCurrentTimeInSec() + fDelay;

    EE_MESSAGE_BREAK(
        pMessage,
        IMessage::mdf_BreakOnDelayedLocalPost,
        ("%s| SendLocalDelayed with delay %.3f yeilding time %.3f",
        pMessage->GetDescription().c_str(), fDelay, fTime));

    EE_LOG_METRIC_COUNT(kMessageService, "SEND.MESSAGE.LOCAL");

    // Create the Delayed Message structure to store on the list
    DelayedMessage delayedMessage(pMessage, targetChannel, fTime);

    // Lock the list while we iterate through the list
    m_delayedMessageListLock.Lock();

    // Grab an iterator pointing to the beginning of the Delayed Message list
    DelayedMsgList::iterator iterDelayedMessage = m_delayedMessageList.begin();

    // Loop through all the Delayed Messages and find the point (time) to insert
    // this Delayed Message
    while (m_delayedMessageList.end() != iterDelayedMessage
        && ((*iterDelayedMessage).m_sendTime < fTime))
    {
        // Move to the next
        ++iterDelayedMessage;
    }

    if (m_delayedMessageList.end() != iterDelayedMessage)
    {
        // If we found a lower priority insert the service before it
        m_delayedMessageList.insert(iterDelayedMessage, delayedMessage);
    }
    else
    {
        // If did not find a lower priority insert it at the end of the list
        m_delayedMessageList.push_back(delayedMessage);
    }

    // Unlock the list
    m_delayedMessageListLock.Unlock();
}

//------------------------------------------------------------------------------------------------
void MessageService::HandleNetMessage(
    const IMessage* pIncomingMessage,
    const ConnectionID& senderConnectionID)
{
    // DT32410 This message handler should take an EnvelopeMessage instead of IMessage
    EE_ASSERT(pIncomingMessage);
    const EnvelopeMessage* pEnvelopeMessage = EE_DYNAMIC_CAST(EnvelopeMessage, pIncomingMessage);
    EE_ASSERT(pEnvelopeMessage);
    EE_ASSERT(pEnvelopeMessage->GetDestinationCategory() != kCAT_LocalMessage);

    const IMessage* pMessageToDeliver = pEnvelopeMessage->GetContents(this);

    if (!pMessageToDeliver)
    {
        // If you hit this case it means that your message failed to factory.
        // This means that a factory function is not registered for the message being delivered
        EE_LOG(
            efd::kMessageService,
            efd::ILogger::kERR1,
            ("%s: Failed to factory messageid 0x%08X. Did you"
            "forget to setup an EE_HANDLER* macro for this type in this process?",
            __FUNCTION__,
            pEnvelopeMessage->GetContentsClassID()));
        return;
    }

    //Make sure that the message is not from us
    // check to see if the message originated from us through the ChannelManager
    EE_ASSERT(m_spNetService);
    efd::ConnectionID channelManagerConnectionID =
        m_spNetService->GetChannelManagerConnectionID(senderConnectionID.GetQualityOfService());
    EE_ASSERT(channelManagerConnectionID != kCID_INVALID);
    if (pEnvelopeMessage->GetSenderNetID() == m_spNetService->GetNetID() &&
        senderConnectionID == channelManagerConnectionID
        && !(senderConnectionID.GetQualityOfService() & NET_TOTAL_ORDER))
    {
        // if here, received a message for which we don't have a listener...
        EE_MESSAGE_TRACE(
            pEnvelopeMessage,
            efd::kNetMessage,
            ILogger::kLVL1,
            ILogger::kLVL3,
            ("%s> Discarding message from self received from ChannelManager %s, cat = %s, "
            "connection = %s\n",
            __FUNCTION__,
            senderConnectionID.ToString().c_str(),
            pEnvelopeMessage->GetDestinationCategory().ToString().c_str(),
            senderConnectionID.ToString().c_str()));
        return;
    }
    m_spNetService->CheckPendingSubscriptions(
        pEnvelopeMessage->GetDestinationCategory(),
        pEnvelopeMessage->GetQualityOfService());

    switch (pMessageToDeliver->GetClassID())
    {
    case kMSGID_SendToProducer:
        {
            EE_ASSERT(pEnvelopeMessage->GetDestinationCategory() == kCAT_SendToProducer);
            EnvelopeMessage* pInnerEnvelopeMessage =
                EE_DYNAMIC_CAST(EnvelopeMessage, const_cast<IMessage*>(pMessageToDeliver));
            EE_ASSERT(pInnerEnvelopeMessage);

            EE_MESSAGE_TRACE(
                pEnvelopeMessage,
                efd::kNetMessage,
                ILogger::kLVL1,
                ILogger::kLVL3,
                ("%s> SendToProducer %s for %s, from %s NetID=%d\n",
                __FUNCTION__,
                pInnerEnvelopeMessage->GetDescription().c_str(),
                pInnerEnvelopeMessage->GetDestinationCategory().ToString().c_str(),
                senderConnectionID.ToString().c_str(),
                pEnvelopeMessage->GetSenderNetID()));
            const IMessage* pProducerMessage = pInnerEnvelopeMessage->GetContents(this);
            EE_ASSERT(pProducerMessage);
            SendImmediate(pProducerMessage, pInnerEnvelopeMessage->GetDestinationCategory(), true);
            return;
        }
        return;

    default:
        {
            // Message has already been queued by INetLib for delivery on Tick, go ahead and
            // delivery immediately.
            SendImmediate(pMessageToDeliver, pEnvelopeMessage->GetDestinationCategory(), false);

            EE_LOG_METRIC_COUNT(kMessageService, "RECEIVE.MESSAGE.REMOTE");
        }
        return;
    }

}

//------------------------------------------------------------------------------------------------
void MessageService::RegisterMessageHandlers()
{
    // Iterate all instances that are in the static list
    BaseMessageHandler* h = BaseMessageHandler::s_firstMessageHandler;
    while (h != NULL && h != m_lastHandlerProcessed)
    {
        // Get them registered in the optimized map
        RegisterMessageHandler(h);

        // drain the list.
        h = h->m_nextMessageHandler;
    }
    m_lastHandlerProcessed = BaseMessageHandler::s_firstMessageHandler;
}

//------------------------------------------------------------------------------------------------
void MessageService::RegisterMessageHandler(BaseMessageHandler *h)
{

    HandlersListsByMessageClass::iterator handlersElt =
        m_handlersListsByMessageClass.find(h->m_messageClassID);

    if (handlersElt == m_handlersListsByMessageClass.end())
    {
        // add a new list
        MessageHandlersList l;

        // insert returns a pair, the first of which is an iterator to the inserted elt.
        handlersElt = m_handlersListsByMessageClass.
            insert(HandlersListsByMessageClass::value_type(h->m_messageClassID,l)).first;

        EE_LOG(
            efd::kMessageService,
            efd::ILogger::kLVL2,
            ("MessageService::RegisterMessageHandler(): Target class id: 0x%08X, message id: "
            "0x%08X",
            h->m_targetClassID,
            h->m_messageClassID));
    }
    // attempt to register the factory every time we see this type.  That way we hit the assert
    // if there is a type/class id mismatch
    h->RegisterMessageFactory(this);
    (*handlersElt).second.push_back(h);

	SetCacheHandlersDirty();
}

//------------------------------------------------------------------------------------------------
void MessageService::SubscribeUnsubscribe()
{
    // It is not safe to call SubscribeUnsubscribe while ticking
    EE_ASSERT(!m_ticking);

    // Loop through the list of targets to subscribe and unsubscribe and do it
    // Grab and hold the lock during the entire process because it is not only
    // easier, but will ensure the framework will only have to wait for the lock
    // once per cycle.
    m_subUnsubLock.Lock();
    TargetSubscription targetSubscription;
    while (m_subUnsubTargetList.pop_front(targetSubscription))
    {
        switch (targetSubscription.m_subscribe)
        {
        case TargetSubscription::SUBSCRIBE:
            {
                InternalSubscribe(
                    targetSubscription.m_pIAppTarget,
                    targetSubscription.m_targetChannel,
                    false);
            }
            break;
        case TargetSubscription::UNSUBSCRIBE:
            {
                InternalUnsubscribe(
                    targetSubscription.m_pIAppTarget,
                    targetSubscription.m_targetChannel,
                    false);
            }
            break;
        case TargetSubscription::PRODUCER_SUBSCRIBE:
            {
                InternalSubscribe(
                    targetSubscription.m_pIAppTarget,
                    targetSubscription.m_targetChannel,
                    true);
            }
            break;
        case TargetSubscription::PRODUCER_UNSUBSCRIBE:
            {
                InternalUnsubscribe(
                    targetSubscription.m_pIAppTarget,
                    targetSubscription.m_targetChannel,
                    true);
            }
            break;
        default:
            EE_LOG(
                efd::kMessageService,
                ILogger::kERR0,
                ("MessageService::Unsupported subscription type (%d).",
                 targetSubscription.m_subscribe));
            break;
        }
    }
    m_subscriptionsDirty = false;
    m_subUnsubLock.Unlock();
}

//------------------------------------------------------------------------------------------------
void MessageService::ProcessMessages()
{
    bool bDeliver = true;

    // Get the current time
    double fTime = efd::GetCurrentTimeInSec();

    // Lock the list while we iterate through the list
    m_delayedMessageListLock.Lock();

    while (!m_delayedMessageList.empty() && bDeliver)
    {
        // Grab the first element in the Delayed Message list
        DelayedMessage delayedMessage = m_delayedMessageList.front();

        if (delayedMessage.m_sendTime <= fTime)
        {
            EE_LOG(
                efd::kMessageService,
                efd::ILogger::kLVL3,
                ("Actually Sending Delayed message 0x%08X type %i send time %4.3f "
                "current time %4.3f\n",
                (const efd::IMessage*)(delayedMessage.m_spMessage),
                delayedMessage.m_spMessage->GetClassID(),
                delayedMessage.m_sendTime, fTime));

            // Send the message
            if (delayedMessage.m_isProducerMessage)
            {
                DeliverToProducers(delayedMessage.m_spMessage, delayedMessage.m_targetCategory);
            }
            else
            {
                SendImmediate(delayedMessage.m_spMessage, delayedMessage.m_targetCategory);
            }
            m_delayedMessageList.pop_front();
            if (m_subscriptionsDirty)
                SubscribeUnsubscribe();
        }
        else
            bDeliver = false;
    }

    // Unlock the list
    m_delayedMessageListLock.Unlock();

    // Lock the list while we iterate through the list
    m_messageListLock.Lock();

    MessageList* pTemp = m_pMessageList;
    m_pMessageList = m_pMessageListInternal;

    // Unlock the list
    m_messageListLock.Unlock();

    m_pMessageListInternal = pTemp;

    MessageWithCategory messageWithCategory;
    while (m_pMessageListInternal->pop_front(messageWithCategory))
    {
        if (messageWithCategory.m_isProducerMessage)
        {
            DeliverToProducers(
                messageWithCategory.m_spMessage,
                messageWithCategory.m_targetCategory);
        }
        else
        {
            SendImmediate(messageWithCategory.m_spMessage, messageWithCategory.m_targetCategory);
        }
        if (m_subscriptionsDirty)
        {
            SubscribeUnsubscribe();
        }
    }
}

//------------------------------------------------------------------------------------------------
void MessageService::SendRemote(
    IMessage* pMessage,
    const Category &cat,
    QualityOfService defaultQOS)
{
    EE_LOG(efd::kMessageService, ILogger::kLVL3, ("MessageService::SendRemote"));
    if (!m_sendRemote)
    {
        EE_LOG(
            efd::kMessageService,
            ILogger::kERR3,
            ("INetService not available. SendRemote unavailable."));
        return;
    }

    EE_LOG_METRIC_COUNT(kMessageService, "SEND.MESSAGE.REMOTE");

    EE_ASSERT(m_spNetService);
    m_spNetService->SendRemote(pMessage, cat, defaultQOS);
}

//------------------------------------------------------------------------------------------------
void MessageService::SendRemote(
    IMessage* pMessage,
    const Category &cat,
    const ConnectionID& cid)
{
    EE_LOG(efd::kMessageService, ILogger::kLVL3, ("MessageService::SendRemote"));
    if (!m_sendRemote)
    {
        EE_LOG(
            efd::kMessageService,
            ILogger::kERR3,
            ("INetService not available. SendRemote unavailable."));
        return;
    }

    EE_LOG_METRIC_COUNT(kMessageService, "SEND.MESSAGE.REMOTE");

    EE_ASSERT(m_spNetService);
    m_spNetService->SendRemote(pMessage, cat, cid);
}

//------------------------------------------------------------------------------------------------
void MessageService::ProducerSend(
    IMessage* pMessage,
    const Category& categoryProduced,
    QualityOfService qualityOfService)
{
    QualityOfService physicalQOS = QOSCompare::LookupPhysical(qualityOfService);
    if (!m_sendRemote || !(physicalQOS & NET_TOTAL_ORDER))
    {
        SendLocal(pMessage, categoryProduced, true);
    }
    if (m_sendRemote)
    {
        ProducerSendRemote(pMessage, categoryProduced, qualityOfService);
    }
}

//------------------------------------------------------------------------------------------------
void MessageService::ProducerSendRemote(
    IMessage* pMessage,
    const Category& categoryProduced,
    QualityOfService qualityOfService)
{
    if (!m_sendRemote)
    {
        EE_LOG(
            efd::kMessageService,
            ILogger::kERR2,
            ("INetService not available. ProducerSendRemote unavailable."));
        return;
    }
    EE_ASSERT(m_spNetService);
    m_spNetService->ProducerSendRemote(pMessage, categoryProduced, qualityOfService);
}

//------------------------------------------------------------------------------------------------
void MessageService::Send(
    const IMessage* pMessage,
    const Category &cat,
    efd::QualityOfService defaultQOS)
{
    QualityOfService physicalQOS = QOSCompare::LookupPhysical(defaultQOS);
    if (!m_sendRemote || !(physicalQOS & NET_TOTAL_ORDER))
    {
        SendLocal(pMessage, cat);
    }
    if (m_sendRemote)
    {
        SendRemote(const_cast<IMessage*>(pMessage), cat, defaultQOS);
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 MessageService::GetNetID() const
{
    if (m_spNetService)
    {
        return m_spNetService->GetNetID();
    }
    else
    {
        return kNetID_Unassigned;
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 MessageService::GetVirtualProcessID() const
{
    return m_pServiceManager->GetVirtualProcessID();
}

//------------------------------------------------------------------------------------------------
void MessageService::BeginCategoryProduction(
    const Category& cat,
    efd::QualityOfService qualityOfService,
    IBase* pProducerCallback)
{
    EE_ASSERT(cat.IsValid());
    Subscribe(pProducerCallback, cat, qualityOfService, true);
}


//------------------------------------------------------------------------------------------------
void MessageService::EndCategoryProduction(
    const Category& cat,
    IBase* pProducerCallback)
{
    EE_ASSERT(cat.IsValid());
    Unsubscribe(pProducerCallback, cat, true);
}

//------------------------------------------------------------------------------------------------
void MessageService::DeliverToProducers(const IMessage* pMessage, Category targetChannel)
{
    EE_MESSAGE_BREAK(
        pMessage,
        IMessage::mdf_BreakOnFirstLocalDelivery,
        ("%s: First Local Delivery to Producers", pMessage->GetDescription().c_str()));

    SendImmediate(pMessage, targetChannel, true);
}

//------------------------------------------------------------------------------------------------
void MessageService::Subscribe(
    IBase* pOriginalTarget,
    const Category& consumeFrom,
    QualityOfService qualityOfService,
    bool producerSubscribe)
{
    // Make sure that a target or producer subscribe has been specified
    EE_ASSERT_MESSAGE(
        pOriginalTarget || producerSubscribe,
        ("Message target is NULL! Only MessageService::BeginCategoryProduction is allowed to call"
        "MessageService::Subscribe with a NULL message target."));

    // In the case of multiple inheritance where two parent classes have derived from IBase
    // the IBase* passed in may have different values for the same object depending on which
    // IBase ancestor the object is cast to. To work around this problem we use our RTTI
    // system to dynamic cast the IBase* to an IBase*. This guarantees that even in the
    // case of multiple inheritance pTarget will always have a consistent value for the same
    // object.
    IBase* pTarget = EE_DYNAMIC_CAST(IBase, pOriginalTarget);

    // If you hit this assert it means that you do not have RTTI properly setup for your class.
    // see Programmer/Foundation/foundation_rtti.htm for details.
    EE_ASSERT_MESSAGE(
        pTarget || !pOriginalTarget,
        ("If you hit this assert it means that you do "
        "not have RTTI properly setup for your class. see "
        "Programmer/Foundation/foundation_rtti.htm for details."));

    TargetSubscription::SUBSCRIBE_TYPE subscriptionType;
    if (producerSubscribe)
    {
        subscriptionType = TargetSubscription::PRODUCER_SUBSCRIBE;
    }
    else
    {
        subscriptionType = TargetSubscription::SUBSCRIBE;
    }
    // Create the target structure to add to the list

    TargetSubscription targetSubscription(
        k_invalidMessageClassID,
        pTarget,
        consumeFrom,
        qualityOfService,
        subscriptionType);
    if (pTarget)
    {
        // If you hit this assert it means that pTarget is not properly following SmartPointer/Ref
        // counting semantics.  Note: you cannot call Subscribe from a constructor.
        // See SmartPointer documentation and MessageService documentation.
        EE_ASSERT_MESSAGE(
            pTarget->GetRefCount() > 0,
            ("If you hit this assert it means that pTarget is not properly following "
            "SmartPointer/Ref counting semantics.  Note: you cannot call Subscribe from a "
            "constructor. See SmartPointer documentation and MessageService documentation."));
        EE_LOG(
            efd::kMessageService,
            efd::ILogger::kLVL0,
            ("Scheduling to Subscribe a IBase:, %s target 0x%08X",
            consumeFrom.ToString().c_str(),
            pTarget));

        EE_ASSERT_MESSAGE(
            consumeFrom.IsValid(),
            ("Attempt to subscribe to an invalid category."));

        // Add it to the list of targets to subscribe and unsubscribe
        m_subUnsubLock.Lock();
        m_subUnsubTargetList.push_back(targetSubscription);
        m_subUnsubLock.Unlock();
        if (!m_ticking)
        {
            SubscribeUnsubscribe();
        }
        else
        {
            m_subscriptionsDirty = true;
        }
    }

    if (consumeFrom != kCAT_LocalMessage)
    {
        if (m_spNetService || !m_offlineMode)
        {
            m_pendingNetSubscriptions.push_back(targetSubscription);
        }
        if (m_spNetService)
        {
            NetSubscribeUnsubscribe();
        }
    }
}

//------------------------------------------------------------------------------------------------
void MessageService::Unsubscribe(
    IBase* pOriginalTarget,
    const Category& consumeFrom,
    bool producerSubscribe)
{
    // Make sure that a target or producer subscribe has been specified
    EE_ASSERT_MESSAGE(
        pOriginalTarget || producerSubscribe,
        ("Message target is NULL! Only MessageService::EndCategoryProduction is allowed to call"
        "MessageService::Unsubscribe with a NULL message target."));

    // In the case of multiple inheritance where two parent classes have derived from IBase
    // the IBase* passed in may have different values for the same object depending on which
    // IBase ancestor the object is cast to. To work around this problem we use our RTTI
    // system to dynamic cast the IBase* to an IBase*. This guarantees that even in the
    // case of multiple inheritance pTarget will always have a consistent value for the same
    // object.
    IBase* pTarget = EE_DYNAMIC_CAST(IBase, pOriginalTarget);

    // If you hit this assert it means that you do not have RTTI properly setup for your class.
    // see Programmer/Foundation/foundation_rtti.htm for details.
    EE_ASSERT_MESSAGE(
        pTarget || !pOriginalTarget,
        ("If you hit this assert it means that you do "
        "not have RTTI properly setup for your class. see "
        "Programmer/Foundation/foundation_rtti.htm for details."));

    EE_ASSERT_MESSAGE(
        consumeFrom.IsValid(),
        ("Attempt to unsubscribe to an invalid category."));

    TargetSubscription::SUBSCRIBE_TYPE subscriptionType;
    if (producerSubscribe)
    {
        subscriptionType = TargetSubscription::PRODUCER_UNSUBSCRIBE;
    }
    else
    {
        subscriptionType = TargetSubscription::UNSUBSCRIBE;
    }
    // Create the target structure to add to the list
    TargetSubscription targetSubscription(
        k_invalidMessageClassID,
        pTarget,
        consumeFrom,
        QOS_INVALID,
        subscriptionType);
    EE_LOG(
        efd::kMessageService,
        efd::ILogger::kLVL0,
        ("Scheduling to Unsubscribe:, %s target 0x%08X",
        consumeFrom.ToString().c_str(),
        pTarget));

    // Add it to the list of targets to subscribe and unsubscribe
    m_subUnsubLock.Lock();
    m_subUnsubTargetList.push_back(targetSubscription);
    m_subUnsubLock.Unlock();
    if (!m_ticking)
    {
        SubscribeUnsubscribe();
    }
    else
    {
        m_subscriptionsDirty = true;
    }

    if ((consumeFrom != kCAT_LocalMessage) && m_spNetService)
    {
        if (producerSubscribe)
        {
            m_spNetService->EndCategoryProduction(consumeFrom, this);
        }
        else
        {
            m_spNetService->Unsubscribe(consumeFrom, this);
        }
    }
}

//------------------------------------------------------------------------------------------------
void MessageService::InternalSubscribe(
    IBase* pTarget,
    const Category& consumeFrom,
    bool producerSubscribe)
{
    TargetsListsByCategory* pTargetsListsByCategory;
    if (producerSubscribe)
    {
        pTargetsListsByCategory = &m_producerListsByCategory;
    }
    else
    {
        pTargetsListsByCategory = &m_targetsListsByCategory;
    }
    EE_ASSERT(pTargetsListsByCategory);

    // Append the target directly into the list of targets of this Category
    TargetsListsByCategory::iterator t = pTargetsListsByCategory->find(consumeFrom);
    if (t == pTargetsListsByCategory->end())
    {
        // Temporary empty list to initialize new entry (a map<target, refcount>) for this
        // Category. insert() below takes a copy, not a ref.
        TargetsList l;

        // .first is the iterator of the inserted element, .second is a bool indicating succ/fail
        t = pTargetsListsByCategory->insert(TargetsListsByCategory::value_type(consumeFrom,l)).
            first;
        // The newly inserted pair (cat,list) has new target added below.
    }

    if (pTarget != NULL)
    {
        EE_LOG(efd::kMessageService, efd::ILogger::kLVL2,
            ("Actually Subscribing: %s target 0x%08X target class id: 0x%08X",
            consumeFrom.ToString().c_str(),
            pTarget,
            pTarget->GetClassID()
));
    }
    else
    {
        EE_LOG(efd::kMessageService, efd::ILogger::kERR2,
            ("Actually Subscribing: %s target 0x%08X, but target is NULL...",
            consumeFrom.ToString().c_str(),
            pTarget
));
    }

    // Include the requested target in the list of targets that get this Category of message.
    TargetsList::iterator existingTarget = (*t).second.find(pTarget);
    if (existingTarget == (*t).second.end())
    {
        // The target isn't already in the existing list of targets. Simply add it.
        (*t).second[pTarget] = 1;

		if( (consumeFrom == kCAT_LocalMessage) && !producerSubscribe )
		{
			SetCacheHandlersDirty();
		}
    }
    else
    {
        // If the target has already subscribed to this channel, simply increment the refcount, so
        // the next subsequent unsubscribe doesn't unsubscribe everybody.
        ++(*existingTarget).second;
    }

}

//------------------------------------------------------------------------------------------------
void MessageService::InternalUnsubscribe(
    IBase* pTarget,
    const Category& consumeFrom,
    bool producerSubscribe)
{
    TargetsListsByCategory* pTargetsListsByCategory;
    if (producerSubscribe)
    {
        pTargetsListsByCategory = &m_producerListsByCategory;
    }
    else
    {
        pTargetsListsByCategory = &m_targetsListsByCategory;
    }
    EE_ASSERT(pTargetsListsByCategory);
    // Append the target directly into the list of targets of this Category
    TargetsListsByCategory::iterator t = pTargetsListsByCategory->find(consumeFrom);
    if (t == pTargetsListsByCategory->end())
    {
        // There are no more targets for this Category. Normally this is an application error since
        // it is an "extra" unsubscribe. However, depending on service shutdown order this can
        // happen a lot during shutdown at which point it is harmless. So log only if we are not
        // shutting down. We detect "shutdown" because our service manager pointer is cleared once
        // OnShutdown completes.
        if (m_pServiceManager)
        {
            EE_LOG(kMessageService, ILogger::kERR2,
                ("MessageService::Unsubscribe, no targets remain for cat: %s",
                consumeFrom.ToString().c_str()));
        }
        return;
    }

    // MessageService holds a SmartPointer to IBase so the RefCount should always be at least 1.
    // If you hit this assert it means that the RefCount of pTarget has been mismanaged
    // (i.e creating a RefCounted object on the Stack, or calling EE_NEW and initially assigning
    // the object to a non-SmartPointer).
    EE_ASSERT_MESSAGE(
        pTarget->GetRefCount() > 0,
        ("If you hit this assert it means that pTarget is not properly following "
        "SmartPointer/Ref counting semantics.  Note: you cannot call Subscribe from a "
        "constructor. See SmartPointer documentation and MessageService documentation."));

    // Reduce the refcount for the target's subscription to the category. If the refcount hits
    // 0, remove the subscription from the map.
    TargetsList::iterator existingTarget = (*t).second.find(pTarget);
    if (existingTarget == (*t).second.end())
    {
        // The target isn't in the list of targets. This is also an extra unsubscribe error.
        if (m_pServiceManager)
        {
            EE_LOG(kMessageService,ILogger::kERR2,
                ("MessageService::Unsubscribe, target at %p is not subscribed to channel: %s",
                pTarget,
                consumeFrom.ToString().c_str()));
        }
        return;
    }
    else
    {
        // Decrement the refcount.
        --(*existingTarget).second;

        EE_ASSERT((*existingTarget).second >= 0);

        if ((*existingTarget).second == 0)
        {
            (*t).second.erase(pTarget);

			// target unsubscribed, remove from caching
			if( (consumeFrom == kCAT_LocalMessage) && !producerSubscribe )
			{
				SetCacheHandlersDirty();
			}

            // If this was the last target subscribed to the channel, remove the channel.
            if ((*t).second.empty())
            {
                pTargetsListsByCategory->erase(t);
            }
        }
    }

    // Could assert that there was at least one removed.
}

//------------------------------------------------------------------------------------------------
const char* MessageService::GetDisplayName() const
{
    return "MessageService";
}

//------------------------------------------------------------------------------------------------
Category MessageService::GetServicePublicCategory(
    ClassID serviceID,
    UInt32 categoryIndex)
{
    return Category(UniversalID::ECU_Any, categoryIndex, serviceID);
}

//------------------------------------------------------------------------------------------------
Category MessageService::GetServicePrivateCategory(ClassID serviceID)
{
    //EE_ASSERT(GetNetID() != kNetID_Unassigned);
    return Category(UniversalID::ECU_PrivateChannel, GetNetID(), serviceID);
}

//------------------------------------------------------------------------------------------------
Category MessageService::GetServicePrivateCategory(ClassID serviceID, UInt32 netID)
{
    //EE_ASSERT(GetNetID() != kNetID_Unassigned);
    //EE_ASSERT(netID != kNetID_Unassigned);
    return Category(UniversalID::ECU_PrivateChannel, netID, serviceID);
}

//------------------------------------------------------------------------------------------------
Category MessageService::GetServiceProcessUniqueCategory(ClassID serviceID)
{
    //EE_ASSERT(GetNetID() != kNetID_Unassigned);
    UInt32 currentCounter = 0;
    m_counterLock.Lock();
    m_counters.find(serviceID, currentCounter);
    m_counters[serviceID] = ++currentCounter;
    m_counterLock.Unlock();
    return Category(UniversalID::ECU_EventChannel, currentCounter, serviceID);
}

//------------------------------------------------------------------------------------------------
// @todo: review all usage of this method. None of the categories passed out are truly globally
// unique unless we are already connected to a ChannelManager. We need a generally better way to
// deal with "my netid" before it's value is assigned.
Category MessageService::GetGloballyUniqueCategory()
{
    //EE_ASSERT(GetNetID() != kNetID_Unassigned);
    UInt32 currentCounter = 0;
    m_counterLock.Lock();
    m_counters.find(kCLASSID_NullSystemService, currentCounter);
    m_counters[kCLASSID_NullSystemService] = ++currentCounter;
    m_counterLock.Unlock();
    return Category(UniversalID::ECU_Point2Point, GetNetID(), currentCounter);
}

//------------------------------------------------------------------------------------------------
void MessageService::SetOfflineMode(bool isOffline)
{
    m_offlineMode = isOffline;
    if (m_offlineMode)
    {
        m_pendingNetSubscriptions.clear();
    }
}

//------------------------------------------------------------------------------------------------

