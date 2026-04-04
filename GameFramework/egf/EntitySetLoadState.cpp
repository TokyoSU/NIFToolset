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
#include <egf/EntitySetLoadState.h>
#include <egf/egfLogIDs.h>
#include <egf/egfBaseIDs.h>
#include <efd/ServiceManager.h>
#include <egf/WorldFactory.h>
#include <egf/WorldFactoryRequest.h>
#include <egf/WorldFactoryResponse.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntitySetLoadState);

//------------------------------------------------------------------------------------------------
EE_HANDLER(EntitySetLoadState, HandleAssetLoadMsg, AssetLoadResponse);
EE_HANDLER_SUBCLASS(EntitySetLoadState, HandleAssetLoadMsg, AssetLoadResponse, WorldFactoryResponse);

//------------------------------------------------------------------------------------------------
EntitySetLoadState::EntitySetLoadState(
    const egf::EntityLoadRequest* pMsg,
    ServiceManager* pServiceManager)
    : m_requestID(pMsg->m_blockID)
    , m_settings(pMsg->m_settings)
    , m_asset_status(LOAD_NOT_STARTED)
    , m_op(pMsg->m_op)
    , m_spWorldFactoryResponse(0)
    , m_pServiceManager(pServiceManager)
{
    EE_ASSERT(m_pServiceManager);

    AddCallback(m_settings);
}

//------------------------------------------------------------------------------------------------
EntitySetLoadState::~EntitySetLoadState()
{
}

//------------------------------------------------------------------------------------------------
void EntitySetLoadState::BeginAssetLoad()
{
    m_asset_status = LOAD_STARTED;

    MessageService* pMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    EE_ASSERT(pMessageService);

    // REVIEW: why not use a completely unique callback category here? That would eliminate cross
    // talk from other in-progress block loads.
    m_asset_load_callback = GetCategory();

    pMessageService->Subscribe(this, m_asset_load_callback);

    WorldFactoryRequestPtr spLoadRequest = EE_NEW WorldFactoryRequest(
        m_requestID.GetURN(),
        m_asset_load_callback);

    if (m_op == EntityLoadRequest::elo_Reload)
    {
        spLoadRequest->SetIsReload(true);
    }

    pMessageService->SendLocal(spLoadRequest, AssetFactoryManager::MessageCategory());
}

//------------------------------------------------------------------------------------------------
void EntitySetLoadState::HandleAssetLoadMsg(
    const efd::AssetLoadResponse* pMessage,
    efd::Category targetChannel)
{
    if (pMessage->GetURN() != m_requestID.GetURN())
    {
        return;
    }

    WorldFactoryResponse* pWorldMessage = const_cast<WorldFactoryResponse *>(
        EE_DYNAMIC_CAST(WorldFactoryResponse, pMessage));

    if (pMessage->GetResult() == AssetLoadResponse::ALR_Success)
    {
        // Success message must be of our expected message type
        EE_ASSERT(pWorldMessage);

        m_spWorldFactoryResponse = pWorldMessage;
        m_entity_buffer = pWorldMessage->GetAssetData();
        m_entity_file = pMessage->GetURN();

        // Did the best we could.
        m_asset_status = LOAD_COMPLETE;
    }
    else
    {
        m_asset_status = LOAD_FAILED;
        m_entity_buffer = "";
        m_entity_file = "<load failed>";

        // Could be a base class message depending on where the failure occured, in which case
        // pWorldMessage will be NULL.
        if (pWorldMessage)
        {
            m_spWorldFactoryResponse = pWorldMessage;
        }
    }

    MessageService* pMessageService = m_pServiceManager->GetSystemServiceAs<MessageService>();
    pMessageService->Unsubscribe(this, m_asset_load_callback);
}

//------------------------------------------------------------------------------------------------
bool EntitySetLoadState::AddCallback(const BlockLoadParameters& settings)
{
    if (settings.GetNotificationCategory().IsValid())
    {
        CallbackIdentity ci(settings.GetNotificationCategory(), settings.GetNotificationBehavior());
        CallbackMap::iterator iter = m_callbacks.find(ci);
        if (iter == m_callbacks.end())
        {
            m_callbacks[ci] =
                CallbackData(settings.GetNotificationContext(), settings.GetActiveCallbacks());
        }
        else
        {
            // Previous identical callback found, what should we do?
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------------------------
bool EntitySetLoadState::IsReload() const
{
    return m_op == EntityLoadRequest::elo_Reload;
}

//------------------------------------------------------------------------------------------------
Category EntitySetLoadState::GetCategory()
{
    return Category(
        UniversalID::ECU_PrivateChannel,
        efd::kNetID_Client,
        kBASEID_EntitySetLoadState);
}

//------------------------------------------------------------------------------------------------
