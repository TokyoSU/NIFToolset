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

#include <egf/ISchedulerScripting.h>
#include <egf/egfLibType.h>
#include <egf/Scheduler.h>
#include <egf/EntityManager.h>

using namespace efd;
using namespace egf;

//-------------------------------------------------------------------------------------------------
// Class method implementations
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ISchedulerScripting);
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
ISchedulerScripting::~ISchedulerScripting()
{
}

//-------------------------------------------------------------------------------------------------
bool ISchedulerScripting::split(
    const efd::utf8string& i_str,
    char i_ch,
    efd::utf8string& o_strFirst,
    efd::utf8string& o_strSecond)
{
    size_t index = i_str.find(i_ch);
    if (efd::utf8string::npos == index)
    {
        o_strFirst = i_str;
        o_strSecond = efd::utf8string::NullString();
        return false;
    }
    o_strFirst = i_str.substr(0, index);
    o_strSecond = i_str.substr(index+1);
    return true;
}

//-------------------------------------------------------------------------------------------------
