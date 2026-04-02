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

#include <efd/IMessageHelperSystemService.h>
#include <efd/ServiceManager.h>


EE_HANDLER(efd::IMessageHelperSystemService, HandleAssignNetIDMessage, efd::AssignNetIDMessage);

//------------------------------------------------------------------------------------------------
efd::IMessageHelperSystemService::IMessageHelperSystemService(
    efd::ClassID derivedID,
    efd::ClassID messageServiceClass)
: m_classId(derivedID)
, m_messageServiceId(messageServiceClass)
, m_netId(kNetID_Unassigned)
{
}

//------------------------------------------------------------------------------------------------
efd::Category efd::IMessageHelperSystemService::GetPrivateCategory() const
{
    return MessageService::GetServicePrivateCategory(m_classId, m_netId);
}

//------------------------------------------------------------------------------------------------
efd::Category efd::IMessageHelperSystemService::GetPublicCategory() const
{
    return MessageService::GetServicePublicCategory(m_classId);
}

//------------------------------------------------------------------------------------------------
efd::SyncResult efd::IMessageHelperSystemService::OnPreInit(
    IDependencyRegistrar* pDependencyRegistrar)
{
    // Although not strictly required, this will delay derived services from ticking until the
    // message service is initialized.
    pDependencyRegistrar->AddDependency(m_messageServiceId);

    MessageService* pMS = m_pServiceManager->GetSystemServiceAs<MessageService>(m_messageServiceId);
    if (!pMS)
    {
        EE_LOG(efd::kMessageService, efd::ILogger::kERR2,
            ("OnPreInit failed to find MessageService 0x%08X to register subscriptions!",
            m_messageServiceId));
        return efd::SyncResult_Failure;
    }

    m_netId = pMS->GetNetID();

    // Subscribe to the channels for this service. It is assumed that derived classes will add
    // various message handlers using EE_*HANDLER macros for messages that will be handled by the
    // final class.
    pMS->Subscribe(this, GetPrivateCategory());
    pMS->Subscribe(this, GetPublicCategory());

    // We handle the AssignNetIDMessage which is sent to the NetService category. Of course the
    // NetService might not be used in this application if it is stand-alone, but that simply
    // means we won't get any messages of the resulting category.
    efd::Category net = pMS->GetServicePublicCategory(efd::kCLASSID_INetService);
    pMS->Subscribe(this, net);

    return efd::SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
efd::AsyncResult efd::IMessageHelperSystemService::OnShutdown()
{
    MessageService* pMS = m_pServiceManager->GetSystemServiceAs<MessageService>(m_messageServiceId);
    if (pMS)
    {
        pMS->Unsubscribe(this, GetPrivateCategory());
        pMS->Unsubscribe(this, GetPublicCategory());
        efd::Category net = pMS->GetServicePublicCategory(efd::kCLASSID_INetService);
        pMS->Unsubscribe(this, net);
    }
    return efd::AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void efd::IMessageHelperSystemService::HandleAssignNetIDMessage(
    const efd::AssignNetIDMessage* pMessage,
    efd::Category targetChannel)
{
    MessageService* pMS = m_pServiceManager->GetSystemServiceAs<MessageService>(m_messageServiceId);
    if (pMS)
    {
        if (pMessage->GetAssignedNetID() != m_netId)
        {
            pMS->Unsubscribe(this, GetPrivateCategory());
            m_netId = pMessage->GetAssignedNetID();
            pMS->Subscribe(this, GetPrivateCategory());
        }
    }
}

//------------------------------------------------------------------------------------------------
