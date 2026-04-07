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
#include <egf/EntityID.h>
#include <egf/EntityFactoryRequest.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(EntityFactoryRequest);

//------------------------------------------------------------------------------------------------
EntityFactoryRequest::EntityFactoryRequest(
    const utf8string& urn,
    const Category& responseCategory,
    const EntityID& entityID,
    const utf8string& assetPath,
    bool isMasterEntity,
    bool isBackground,
    bool isPreemptive,
    bool reload)
    : FlatModelFactoryRequest(urn, responseCategory, assetPath, isBackground, isPreemptive, reload)
    , m_entityID(entityID)
    , m_bIsMaster(isMasterEntity)
{
    EE_ASSERT(m_entityID != egf::kENTITY_INVALID);
}

//------------------------------------------------------------------------------------------------
const EntityID EntityFactoryRequest::GetEntityID() const
{
    return m_entityID;
}

//------------------------------------------------------------------------------------------------
bool EntityFactoryRequest::IsMasterEntity() const
{
    return m_bIsMaster;
}
