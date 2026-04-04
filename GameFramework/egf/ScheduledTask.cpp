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
#include <egf/EntityFactoryResponse.h>
#include <egf/ScheduledTask.h>

using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(egf::ScheduledTask);

//------------------------------------------------------------------------------------------------
void ScheduledTask::AbortTask(egf::Scheduler* pScheduler)
{
    EE_UNUSED_ARG(pScheduler);
}

//------------------------------------------------------------------------------------------------
bool ScheduledTask::SetResult(egf::Scheduler* pScheduler, const egf::EventMessage* pMessage)
{
    EE_UNUSED_ARG(pScheduler);
    EE_UNUSED_ARG(pMessage);
    return true;
}

//------------------------------------------------------------------------------------------------
void ScheduledTask::SetEntityFactoryResponse(const EntityFactoryResponse* pResponse)
{
    EE_UNUSED_ARG(pResponse);
}
