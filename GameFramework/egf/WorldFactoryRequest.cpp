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
#include <egf/WorldFactoryRequest.h>
#include <efd/utf8string.h>
#include <efd/Category.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(WorldFactoryRequest);

//------------------------------------------------------------------------------------------------
WorldFactoryRequest::WorldFactoryRequest()
    : AssetLoadRequest()
{
}

//------------------------------------------------------------------------------------------------
WorldFactoryRequest::WorldFactoryRequest(
    const utf8string& urn,
    const Category& responseCategory,
    const utf8string& assetPath,
    bool isBackground,
    bool isPreemptive,
    bool isReload)
    : AssetLoadRequest(urn, responseCategory, assetPath, isBackground, isPreemptive, isReload)
{
}
