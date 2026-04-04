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

#include <efd/utf8string.h>
#include <egf/Entity.h>
#include <egf/EntityFactoryResponse.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntityFactoryResponse);

//------------------------------------------------------------------------------------------------
EntityFactoryResponse::EntityFactoryResponse()
    : FlatModelFactoryResponse()
    , m_entityID(kENTITY_INVALID)
    , m_entityPtr(0)
{
}

//------------------------------------------------------------------------------------------------
EntityFactoryResponse::EntityFactoryResponse(
    const efd::utf8string& urn,
    const efd::Category& responseCategory,
    efd::AssetLoadResponse::AssetLoadResult result,
    const egf::EntityID& entityID,
    const efd::utf8string& assetPath,
    bool isReload,
    egf::Entity* entity,
    const efd::utf8string& flatModelName,
    FlatModelFactoryResponse::FlatModelMap flatModelMap,
    FlatModelFactoryResponse::DependentScriptSet scripts)
    : FlatModelFactoryResponse(
        urn,
        responseCategory,
        result,
        assetPath,
        isReload,
        flatModelName,
        flatModelMap,
        scripts)
    , m_entityID(entityID)
    , m_entityPtr(entity)
{
}

//------------------------------------------------------------------------------------------------
const egf::EntityID& EntityFactoryResponse::GetEntityID() const
{
    return m_entityID;
}

//------------------------------------------------------------------------------------------------
EntityPtr EntityFactoryResponse::GetEntity() const
{
    return m_entityPtr;
}

//------------------------------------------------------------------------------------------------
void EntityFactoryResponse::SetEntity(Entity* entity) const
{
    m_entityPtr = entity;
}
