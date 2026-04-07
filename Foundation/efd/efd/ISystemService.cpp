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

#include <efd/ISystemService.h>
#include <efd/IServiceDetailRegister.h>


using namespace efd;

//------------------------------------------------------------------------------------------------
void ISystemService::OnServiceRegistered(efd::IAliasRegistrar*)
{
}

//------------------------------------------------------------------------------------------------
SyncResult ISystemService::OnPreInit(efd::IDependencyRegistrar*)
{
    return SyncResult_Success;
}

//------------------------------------------------------------------------------------------------
AsyncResult ISystemService::OnInit()
{
    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
AsyncResult ISystemService::OnTick()
{
    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
AsyncResult ISystemService::OnShutdown()
{
    return AsyncResult_Complete;
}

//------------------------------------------------------------------------------------------------
void ISystemService::OnPreInit()
{
    EE_FAIL("This is a refactoring method, it should never be called.");
}
