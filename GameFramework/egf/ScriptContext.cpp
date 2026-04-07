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

#include <egf/ScriptContext.h>

namespace egf
{

//------------------------------------------------------------------------------------------------
EE_EGF_ENTRY ScriptContext g_bapiContext;

//------------------------------------------------------------------------------------------------
/*static*/ void ScriptContext::_SDMInit()
{
    g_bapiContext.m_pBehaviorStack = EE_EXTERNAL_NEW efd::stack<egf::PendingBehaviorPtr>();
}

//------------------------------------------------------------------------------------------------
/*static*/ void ScriptContext::_SDMShutdown()
{
    EE_EXTERNAL_DELETE g_bapiContext.m_pBehaviorStack;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string ScriptContext::GetCurrentBehaviorContextString() const
{
    efd::utf8string result;
    const PendingBehavior* pCurrentBehavior = GetCurrentBehavior();
    if (pCurrentBehavior)
    {
        const BehaviorDescriptor *pDescriptor = pCurrentBehavior->GetBehaviorDescriptor();
        if (pDescriptor)
        {
            result = pDescriptor->GetModelName() + "." + pDescriptor->GetName();
        }
        else
        {
            result = "<no descriptor>";
        }
    }
    else
    {
        result = "<unknown>";
    }

    const egf::Entity* pEntity = GetScriptEntity();
    if (pEntity)
    {
        result += " (via ";
        result += pEntity->GetModelName();
        result += ")";
    }
    return result;
}

//------------------------------------------------------------------------------------------------
} // end namespace egf
