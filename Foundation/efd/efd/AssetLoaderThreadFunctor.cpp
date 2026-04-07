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

#include <efd/AssetLoaderThreadFunctor.h>
#include <efd/AssetFactoryManager.h>
#include <efd/Thread.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
AssetLoaderThreadFunctor::AssetLoaderThreadFunctor(
    const size_t requestQueueSize,
    AssetFactoryManager* pForeAssetFactoryManager)
    : AssetFactoryManager(
        requestQueueSize,
        pForeAssetFactoryManager->GetSleepInterval(),
        pForeAssetFactoryManager)
{
    m_shutdown = false;
}

//------------------------------------------------------------------------------------------------
AssetLoaderThreadFunctor::~AssetLoaderThreadFunctor()
{
}

//------------------------------------------------------------------------------------------------
efd::UInt32 AssetLoaderThreadFunctor::Execute(efd::Thread* pArg)
{
    EE_UNUSED_ARG(pArg);

    while (!m_shutdown)
    {
        OnTick();
        efd::YieldThread();
    }

    DoShutdown();

    return 0;
}

//------------------------------------------------------------------------------------------------
void AssetLoaderThreadFunctor::DoShutdown()
{
    OnShutdown();
}

