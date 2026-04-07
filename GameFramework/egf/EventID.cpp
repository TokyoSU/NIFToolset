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

#include <efd/ILogger.h>
#include <efd/Metrics.h>
#include <egf/EventID.h>

//-------------------------------------------------------------------------------------------------
using namespace efd;
using namespace egf;

//-------------------------------------------------------------------------------------------------
//  Static members of EventID
EventID EventID::ms_rootID(0);  // Starts out invalid, becomes valid in InitEventID
efd::UInt64 EventID::ms_baseID = 0;


//-------------------------------------------------------------------------------------------------
//Initialization functions for the static class data
bool EventID::InitEventID(efd::UInt32 shardID, efd::UInt32 netID)
{
    bool result = true;
    result &= ms_rootID.SetSystem(true);
    result &= ms_rootID.SetType(UniversalID::kIDT_EventID);
    result &= ms_rootID.SetShardID(shardID);
    result &= ms_rootID.SetNetID(netID);
    return result;
}


//-------------------------------------------------------------------------------------------------
EventID EventID::CreateEventID()
{
    // Assert that InitEventID has been called:
    EE_ASSERT(ms_rootID.IsValid());

    // Create the next ID, incrementing the base ID value
    EventID result(ms_rootID);
    result.SetBaseID(++ms_baseID);
    return result;
}

