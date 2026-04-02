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

#include <efd/AssetLoadResponse.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(AssetLoadResponse);

//------------------------------------------------------------------------------------------------
void AssetLoadResponse::Serialize(efd::Archive&)
{
    // These messages are not intended to go over the network. If you hit this assert, an attempt
    // was made to use MessageService::Send instead of SendLocal for this Asset Factory message.
    EE_ASSERT(false);
}

//------------------------------------------------------------------------------------------------
