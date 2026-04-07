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

#include <egf/BehaviorAPI.h>
#include <egf/Entity.h>
#include <egf/FlatModel.h>
#include <efd/EEHelpers.h>
#include <efd/ParameterListMessage.h>
#include <efd/ServiceManager.h>
#include <egf/Scheduler.h>
#include <egf/EventMessage.h>
#include <egf/FlatModelManager.h>
#include <egf/ScriptContext.h>
#include <egf/egfLogIDs.h>
#include <egf/egfBaseIDs.h>
#include <efd/EnumManager.h>
#include <egf/NotificationService.h>

using namespace efd;
using namespace egf;


//------------------------------------------------------------------------------------------------
efd::Float64 BehaviorAPI::GetServiceManagerTime()
{
    return (efd::Float64)g_bapiContext.GetServiceManager()->GetServiceManagerTime();
}


//------------------------------------------------------------------------------------------------
efd::Float64 BehaviorAPI::GetGameTime()
{
    Scheduler* pScheduler = g_bapiContext.GetSystemServiceAs<Scheduler>();
    if (EE_VERIFY(pScheduler))
    {
        return pScheduler->GetGameTime();
    }
    return 0.0;
}


//------------------------------------------------------------------------------------------------
const efd::utf8string& BehaviorAPI::GetModelName()
{
    // Check that the executing entity exists
    if (g_bapiContext.GetScriptEntity())
    {
        return g_bapiContext.GetScriptEntity()->GetModelName();
    }
    return efd::utf8string::NullString();
}

//------------------------------------------------------------------------------------------------
efd::Category BehaviorAPI::CreateCategory(efd::UInt8 usage, efd::UInt32 netID, efd::UInt32 baseID)
{
    return efd::Category((efd::Category::ExpectedChannelUsage)usage, netID, baseID);
}

//------------------------------------------------------------------------------------------------
efd::Category BehaviorAPI::CreateApplicationCategory(efd::UInt32 appCategoryID)
{
    return efd::Category(
        efd::UniversalID::ECU_Any,
        appCategoryID,
        efd::kBASEID_ApplicationCategory);
}

//------------------------------------------------------------------------------------------------
efd::Category BehaviorAPI::GetServicePublicCategory(
    efd::UInt32 serviceID,
    efd::UInt32 categoryIndex)
{
    if (categoryIndex == 0xFFFFFFFF)
    {
        categoryIndex = kNetID_ISystemService;
    }
    return MessageService::GetServicePublicCategory(serviceID, categoryIndex);
}

//------------------------------------------------------------------------------------------------
Category BehaviorAPI::GetServiceProcessUniqueCategory(UInt32 serviceID)
{
    MessageService* pMessageService = g_bapiContext.GetSystemServiceAs<MessageService>();
    if (!pMessageService)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("GetServiceProcessUniqueCategory() failed to find MessageService"));
        return kCAT_INVALID;
    }
    return pMessageService->GetServiceProcessUniqueCategory(serviceID);
}

//------------------------------------------------------------------------------------------------
efd::Category BehaviorAPI::GetServicePrivateCategory(UInt32 serviceID)
{
    MessageService* pMessageService = g_bapiContext.GetSystemServiceAs<MessageService>();
    if (!pMessageService)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("GetServicePrivateCategory() failed to find MessageService"));
        return kCAT_INVALID;
    }
    return pMessageService->GetServicePrivateCategory(serviceID);
}

//------------------------------------------------------------------------------------------------
efd::Category BehaviorAPI::GetServicePrivateCategory(UInt32 serviceID, efd::UInt32 netID)
{
    MessageService* pMessageService = g_bapiContext.GetSystemServiceAs<MessageService>();
    if (!pMessageService)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("GetServicePrivateCategory() failed to find MessageService"));
        return kCAT_INVALID;
    }
    return pMessageService->GetServicePrivateCategory(serviceID, netID);
}

//------------------------------------------------------------------------------------------------
Category BehaviorAPI::GetGloballyUniqueCategory()
{
    MessageService* pMessageService = g_bapiContext.GetSystemServiceAs<MessageService>();
    if (!pMessageService)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("GetGloballyUniqueCategory() failed to find MessageService"));
        return kCAT_INVALID;
    }
    return pMessageService->GetGloballyUniqueCategory();
}

//------------------------------------------------------------------------------------------------
UInt32 BehaviorAPI::GetEnumValue(const char* strEnumName, const char* strIDName)
{
    EnumManager* pEnumManager = g_bapiContext.GetSystemServiceAs<EnumManager>();
    if (!pEnumManager)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("GetEnumValue() failed to find EnumManager"));
        return 0;
    }
    efd::DataDrivenEnumBase* pDataDrivenEnumBase = pEnumManager->FindOrLoadEnum(strEnumName);
    if (!pDataDrivenEnumBase)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("GetEnumValue() failed to find enum '%s'", strEnumName));
        return 0;
    }
    UInt32 foundID;
    efd::SmartPointer< DataDrivenEnum<efd::UInt32> > spDataDrivenEnum =
        pDataDrivenEnumBase->CastTo<efd::UInt32>();;
    if (spDataDrivenEnum->FindEnumValue(strIDName, foundID))
    {
        return foundID;
    }
    else
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("GetEnumValue() failed to find value %s in enum '%s'", strIDName, strEnumName));
        return 0;
    }
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::DoesModelExist(const char* strModelName)
{
    EE_ASSERT(strlen(strModelName) != 0);

    // Find the flat model manager
    FlatModelManager* pfmm = g_bapiContext.GetSystemServiceAs<FlatModelManager>();
    if (!pfmm)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("DoesModelExist() failed to find FlatModelManager"));
        return false;
    }

    // Find or load the model in the flat model manager
    const FlatModel* pModel = pfmm->FindOrLoadModel(strModelName);
    if (!pModel)
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR1,
            ("DoesModelExist() failed to find model '%s'",
            strModelName));
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::SendEvent(
    efd::Category categoryID,
    const char* strBehavior,
    efd::ParameterList* pStream,  /* if NULL no arguments */
    const char* strCallback,   /* if NULL or empty, no callback */
    double delay)
{
    if (!strBehavior || !*strBehavior)
    {
        EE_LOG(efd::kBehaviorAPI, efd::ILogger::kERR1,
            ("SendEvent() Error in %s, invalid behavior name (empty string)",
            g_bapiContext.GetCurrentBehaviorContextString().c_str()));
        return false;
    }

    if (!categoryID.IsValid())
    {
        EE_LOG(efd::kBehaviorAPI, efd::ILogger::kERR1,
            ("SendEvent() Error in %s, calling behavior='%s' with invalid category %s",
            g_bapiContext.GetCurrentBehaviorContextString().c_str(),
            strBehavior,
            categoryID.ToString().c_str()));
        return false;
    }

    EventID eventID = g_bapiContext.GetScriptEntity()->SendEvent(
        categoryID,
        NULL,
        strBehavior,
        pStream,
        strCallback,
        delay);

    EE_LOG(efd::kBehaviorAPI, efd::ILogger::kLVL2,
        ("SendEvent() API sending to category(%s), behavior=(%s), eventID=(0x%016llX)",
        categoryID.ToString().c_str(),
        strBehavior,
        eventID.GetValue()));

    return eventID.IsValid();
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::CallBehavior(
    egf::EntityID entityID,
    const efd::utf8string& strBehavior,
    efd::ParameterList* pArgs)
{
    EE_ASSERT(entityID.IsValid());
    EE_ASSERT(!strBehavior.empty());

    EntityManager* pem = g_bapiContext.GetSystemServiceAs<EntityManager>();
    Entity* pEntity = pem->LookupEntity(entityID);
    if (pEntity)
    {
        return pEntity->CallImmediateBehavior(strBehavior, "", pArgs);
    }

    EE_LOG(efd::kEntity, efd::ILogger::kERR2,
        ("Error: Trying to call Behavior '%s' on unknown entity '%s'...",
        strBehavior.c_str(),
        entityID.ToString().c_str()));

    return false;
}

//------------------------------------------------------------------------------------------------
void BehaviorAPI::SendReply(efd::ParameterList* pStream)
{
    egf::PendingBehavior* pBehavior = g_bapiContext.GetCurrentBehavior();
    pBehavior->GetScriptEntity()->SendReturnValue(pBehavior, pStream);
}

//------------------------------------------------------------------------------------------------
void BehaviorAPI::SendStream(
    Category catID,
    efd::ClassID msgClass,
    efd::ParameterList* pStream)
{
    EE_ASSERT(catID.IsValid());
    EE_ASSERT(msgClass != 0);

    if (!EE_VERIFY(pStream))
    {
        EE_LOG(efd::kBehaviorAPI, efd::ILogger::kERR2,
            ("SendStream() BehaviorAPI could not send stream to %s, %s",
            catID.ToString().c_str(), IMessage::ClassIDToString(msgClass).c_str()));
        return;
    }

    // Grab a pointer to the message service so we can send the event
    MessageService* pMessageService = g_bapiContext.GetSystemServiceAs<MessageService>();
    EE_ASSERT(pMessageService);

    efd::IMessagePtr spBase = pMessageService->CreateObject(msgClass);

    // if this asserts, you may have forgotten to call RegisterMessageWrapperFactory with
    // this type.  The sender needs the type registered if you are creating the messages
    // dynamically, which you must be if you're calling this method.
    EE_ASSERT_MESSAGE(spBase, ("ParameterListMessage type 0x%08d not registered", msgClass));
    ParameterListMessagePtr spStreamMsg = EE_DYNAMIC_CAST(ParameterListMessage, spBase);
    EE_ASSERT(spStreamMsg);

    if (!spStreamMsg)
    {
        EE_LOG(efd::kBehaviorAPI, efd::ILogger::kERR2,
            ("SendStream() BehaviorAPI could not create a ParameterListMessage class to send the stream to %s, %s",
            catID.ToString().c_str(),
            spStreamMsg->GetDescription().c_str()));
        return;
    }

    spStreamMsg->GetParameters() = *pStream;

    pMessageService->Send(spStreamMsg, catID, QOS_RELIABLE);

    EE_LOG(efd::kBehaviorAPI, efd::ILogger::kLVL2,
        ("SendStream() BehaviorAPI sending a stream to %s, %s",
        catID.ToString().c_str(), spStreamMsg->GetDescription().c_str()));
}

//------------------------------------------------------------------------------------------------
void BehaviorAPI::SendStreamLocal(
    efd::ClassID msgClass,
    efd::ParameterList* pStream)
{
    EE_ASSERT(msgClass != 0);

    // Get a pointer to the real ParameterList and make sure it exists
    if (!EE_VERIFY(pStream))
    {
        EE_LOG(efd::kBehaviorAPI, efd::ILogger::kERR2,
            ("SendStreamLocal() BehaviorAPI could not send stream to %s",
            IMessage::ClassIDToString(msgClass).c_str()));
        return;
    }

    // Grab a pointer to the message service so we can send the event
    MessageService* pMessageService = g_bapiContext.GetSystemServiceAs<MessageService>();
    EE_ASSERT(pMessageService);
    efd::IMessagePtr spBase = pMessageService->CreateObject(msgClass);
    // if this asserts, you may have forgotten to call RegisterMessageWrapperFactory with
    // this type.  The sender needs the type registered if you are creating the messages
    // dynamically, which you must be if you're calling this method.
    EE_ASSERT_MESSAGE(spBase, ("ParameterListMessage type 0x%08d not registered", msgClass));
    ParameterListMessagePtr spStreamMsg = EE_DYNAMIC_CAST(ParameterListMessage, spBase);
    EE_ASSERT(spStreamMsg);

    if (!spStreamMsg)
    {
        EE_LOG(efd::kBehaviorAPI, efd::ILogger::kERR2,
            ("SendStream() BehaviorAPI could not create a ParameterListMessage class %s",
            IMessage::ClassIDToString(msgClass).c_str()));
        return;
    }

    spStreamMsg->GetParameters() = *pStream;

    pMessageService->SendLocal(spStreamMsg, kCAT_LocalMessage);

    EE_LOG(efd::kBehaviorAPI, efd::ILogger::kLVL2,
        ("SendStreamLocal() BehaviorAPI sending a stream to %s, %s",
        kCAT_LocalMessage.ToString().c_str(), spStreamMsg->GetDescription().c_str()));
}


//------------------------------------------------------------------------------------------------
// table of message types used by API methods for C++ and scripting languages
efd::UInt8 msgLevel[] =
{
    efd::ILogger::kLVL0,
    efd::ILogger::kLVL1,
    efd::ILogger::kLVL2,
    efd::ILogger::kLVL3
};

// table of error types used by API methods for C++ and scripting languages
efd::UInt8 errLevel[] =
{
    efd::ILogger::kERR0,
    efd::ILogger::kERR1,
    efd::ILogger::kERR2,
    efd::ILogger::kERR3
};


//------------------------------------------------------------------------------------------------
void BehaviorAPI::FlexLogMessage(
    efd::UInt16 module,
    efd::UInt32 i_level,
    const char* i_pszMessage,
    bool i_echoToStdOut)
{
    // normalize i_level to size of the msgLevel
    static const efd::UInt32 maxSize = EE_ARRAYSIZEOF(msgLevel)-1;
    efd::UInt8 level = i_level > maxSize ? msgLevel[maxSize] : msgLevel[i_level];

    EE_LOG(module, level, ("%s", i_pszMessage));

    if (i_echoToStdOut)
    {
        ILogger* logger = efd::GetLogger();
        fprintf(stdout, "%s|%s| %s\n",
            logger->GetLevelName(level),
            logger->GetModuleName(module).c_str(),
            i_pszMessage);
    }
}


//------------------------------------------------------------------------------------------------
void BehaviorAPI::FlexLogError(
    efd::UInt16 module,
    efd::UInt32 i_level,
    const char* i_pszMessage,
    bool i_echoToStdErr)
{
    // normalize i_level to size of the msgLevel
    static const efd::UInt32 maxSize = EE_ARRAYSIZEOF(errLevel)-1;
    efd::UInt8 level = i_level > maxSize ? errLevel[maxSize] : errLevel[i_level];

    EE_LOG(module, level, ("%s", i_pszMessage));

    if (i_echoToStdErr)
    {
        ILogger* logger = efd::GetLogger();
        fprintf(stderr, "%s|%s| %s\n",
            logger->GetLevelName(level),
            logger->GetModuleName(module).c_str(),
            i_pszMessage);
    }
}


//------------------------------------------------------------------------------------------------
void BehaviorAPI::LogMessagePython(
    efd::UInt32 i_level,
    const char* i_pszMessage,
    bool i_echoToStdOut)
{
    FlexLogMessage(efd::kPython, i_level, i_pszMessage, i_echoToStdOut);
}


//------------------------------------------------------------------------------------------------
void BehaviorAPI::LogErrorPython(
    efd::UInt32 i_level,
    const char* i_pszMessage,
    bool i_echoToStdOut)
{
    FlexLogError(efd::kPython, i_level, i_pszMessage, i_echoToStdOut);
}


//------------------------------------------------------------------------------------------------
void BehaviorAPI::LogMessageLua(
    efd::UInt32 i_level,
    const char* i_pszMessage,
    bool i_echoToStdOut)
{
    FlexLogMessage(efd::kLua, i_level, i_pszMessage, i_echoToStdOut);
}


//------------------------------------------------------------------------------------------------
void BehaviorAPI::LogErrorLua(
    efd::UInt32 i_level,
    const char* i_pszMessage,
    bool i_echoToStdOut)
{
    FlexLogError(efd::kLua, i_level, i_pszMessage, i_echoToStdOut);
}


//------------------------------------------------------------------------------------------------
void BehaviorAPI::OutputDebugMessage(const char* pMessage)
{
#if defined(_DEBUG)
    EE_OUTPUT_DEBUG_STRING(pMessage);
    EE_OUTPUT_DEBUG_STRING("\n");
#else // avoid Release build-breaking warning
    EE_UNUSED_ARG(pMessage);
#endif
}

//------------------------------------------------------------------------------------------------
efd::UInt32 BehaviorAPI::GetPropertyCount(
    egf::EntityID entityID,
    const efd::utf8string& strPropertyName,
    efd::UInt32* count)
{
    EE_ASSERT(entityID.IsValid());
    EE_ASSERT(count);

    EntityManager* pem = g_bapiContext.GetSystemServiceAs<EntityManager>();
    Entity* pEntity = pem->LookupEntity(entityID);
    if (pEntity)
    {
        return pEntity->GetPropertyCount(strPropertyName, *count);
    }
    EE_ASSERT(pEntity);

    EE_LOG(efd::kBehaviorAPI, efd::ILogger::kLVL1,
        ("GetPropertyCount() BehaviorAPI entity (%s) not found for property (%s)",
        entityID.ToString().c_str(), strPropertyName.c_str()));

    return PropertyResult_PropertyNotFound;
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::AddUpdateNotificationByEntity(
    egf::EntityID callbackEntityID,
    const efd::utf8string& strCallbackBehavior,
    egf::EntityID targetEntityID)
{
    NotificationService* pNotificationService =
        g_bapiContext.GetSystemServiceAs<NotificationService>();

    if (!pNotificationService)
    {
        EE_LOG(efd::kBehaviorAPI, efd::ILogger::kERR2,
            ("AddUpdateNotificationByEntity NotificationService does not exist"));
        return false;
    }

    return pNotificationService->AddNotification(
        false,
        true,
        false,
        callbackEntityID,
        strCallbackBehavior,
        targetEntityID);
}

//------------------------------------------------------------------------------------------------
bool BehaviorAPI::AddUpdateNotificationByModel(
    egf::EntityID callbackEntityID,
    const efd::utf8string& strCallbackBehavior,
    const efd::utf8string& modelName)
{
    NotificationService* pNotificationService =
        g_bapiContext.GetSystemServiceAs<NotificationService>();

    if (!pNotificationService)
    {
        EE_LOG(efd::kBehaviorAPI, efd::ILogger::kERR2,
            ("AddUpdateNotificationByModel NotificationService does not exist"));
        return false;
    }

    FlatModelManager* pFlatModelManager = g_bapiContext.GetSystemServiceAs<FlatModelManager>();
    if (!pFlatModelManager)
    {
        EE_LOG(efd::kBehaviorAPI, efd::ILogger::kERR2,
            ("AddUpdateNotificationByModel FlatModelManager does not exist"));
        return false;
    }

    FlatModelID modelID = pFlatModelManager->GetModelIDByName(modelName);

    if (kFlatModelID_INVALID == modelID)
    {
        EE_LOG(efd::kBehaviorAPI, efd::ILogger::kERR2,
            ("AddUpdateNotificationByModel FlatModel '%s' does not exist", modelName.c_str()));
        return false;
    }

    return pNotificationService->AddNotification(
        false,
        true,
        false,
        callbackEntityID,
        strCallbackBehavior,
        modelID);
}

/*! @} */
