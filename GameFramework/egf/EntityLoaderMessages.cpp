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
// Emergent Game Technologies, Calabasas, California 91302
// http://www.emergent.net

#include "egfPCH.h"

#include <egf/EntityLoaderMessages.h>


//------------------------------------------------------------------------------------------------
using namespace efd;
using namespace egf;


EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntityLoadRequest);


//------------------------------------------------------------------------------------------------
EntityLoadRequest::EntityLoadRequest()
    : m_op(elo_Invalid)
    , m_blockID()
    , m_settings()
{
}

//------------------------------------------------------------------------------------------------
EntityLoadRequest::EntityLoadRequest(
    const BlockIdentification& i_blockID,
    const BlockLoadParameters* i_pParameters,
    bool i_isReload)
    : m_op(i_isReload ? elo_Reload : elo_Load)
    , m_blockID(i_blockID)
    , m_settings()
{
    if (i_pParameters)
    {
        m_settings = *i_pParameters;
    }
}

//------------------------------------------------------------------------------------------------
EntityLoadRequest::EntityLoadRequest(
    Operation i_op,
    const BlockIdentification& i_blockID,
    efd::Category i_cb,
    Context i_ctx,
    bool i_autoEnterWorld)
    : m_op(i_op)
    , m_blockID(i_blockID)
    , m_settings()
{
    m_settings.SetMessageCallback(i_cb, i_ctx);
    m_settings.SetAutoEnterWorld(i_autoEnterWorld);
}

//------------------------------------------------------------------------------------------------
EntityLoadRequest::EntityLoadRequest(
    Operation i_op,
    const BlockIdentification& i_blockID,
    efd::Category i_cbEntity,
    egf::BehaviorID i_cbBehavior,
    Context i_ctx,
    bool i_autoEnterWorld)
    : m_op(i_op)
    , m_blockID(i_blockID)
    , m_settings()
{
    m_settings.SetBehaviorCallback(i_cbEntity, i_cbBehavior, i_ctx);
    m_settings.SetAutoEnterWorld(i_autoEnterWorld);
}

//------------------------------------------------------------------------------------------------
EntityLoadRequest::~EntityLoadRequest()
{
}

//------------------------------------------------------------------------------------------------
void EntityLoadRequest::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);

    Serializer::SerializeObject(m_op, ar);
    Serializer::SerializeObject(m_blockID, ar);
    Serializer::SerializeObject(m_settings, ar);
}


//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntityLoadResult);

//------------------------------------------------------------------------------------------------
EntityLoadResult::EntityLoadResult()
    : m_assetID()
    , m_context(0)
    , m_result(elr_Unknown)
{
}

//------------------------------------------------------------------------------------------------
EntityLoadResult::EntityLoadResult(
    const BlockIdentification& i_blockID,
    efd::UInt32 i_ctx,
    Result i_result)
    : m_assetID( i_blockID )
    , m_context( i_ctx )
    , m_result( i_result )
{
}

//------------------------------------------------------------------------------------------------
EntityLoadResult::~EntityLoadResult()
{
}

//------------------------------------------------------------------------------------------------
void EntityLoadResult::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);

    Serializer::SerializeObject(m_assetID, ar);
    Serializer::SerializeObject(m_context, ar);
    Serializer::SerializeObject(m_result, ar);
}


//------------------------------------------------------------------------------------------------
// EntityPreloadRequest
//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntityPreloadRequest);

//------------------------------------------------------------------------------------------------
EntityPreloadRequest::EntityPreloadRequest()
{
}

//------------------------------------------------------------------------------------------------
EntityPreloadRequest::~EntityPreloadRequest()
{
    // trivial destructor
}

//------------------------------------------------------------------------------------------------
Entity* EntityPreloadRequest::GetEntity() const
{
    return m_pEntity;
}

//------------------------------------------------------------------------------------------------
EntityID EntityPreloadRequest::GetEntityID() const
{
    return m_pEntity->GetEntityID();
}

//------------------------------------------------------------------------------------------------
void EntityPreloadRequest::Serialize(efd::Archive&)
{
    EE_FAIL("Streaming an EntityPreloadRequest is not supported.");
}


//------------------------------------------------------------------------------------------------
// EntityPreloadResponse
//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntityPreloadResponse);

//------------------------------------------------------------------------------------------------
EntityPreloadResponse::EntityPreloadResponse()
{
}

//------------------------------------------------------------------------------------------------
EntityPreloadResponse::~EntityPreloadResponse()
{
    // trivial destructor
}

//------------------------------------------------------------------------------------------------
Entity* EntityPreloadResponse::GetEntity() const
{
    return m_pEntity;
}

//------------------------------------------------------------------------------------------------
EntityID EntityPreloadResponse::GetEntityID() const
{
    return m_pEntity->GetEntityID();
}

//------------------------------------------------------------------------------------------------
void EntityPreloadResponse::Serialize(efd::Archive&)
{
    EE_FAIL("Streaming an EntityPreloadResponse is not supported.");
}

//------------------------------------------------------------------------------------------------
