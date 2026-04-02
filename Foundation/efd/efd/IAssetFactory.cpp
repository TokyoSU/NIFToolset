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
#include <efd/IAssetFactory.h>

//------------------------------------------------------------------------------------------------
void efd::IAssetFactory::HandleAssetLocate(
    const efd::AssetLocatorResponse* pResponse,
    efd::Category targetCategory)
{
    EE_UNUSED_ARG(pResponse);
    EE_UNUSED_ARG(targetCategory);
}

//------------------------------------------------------------------------------------------------
void efd::IAssetFactory::Shutdown()
{
    // do nothing
}
