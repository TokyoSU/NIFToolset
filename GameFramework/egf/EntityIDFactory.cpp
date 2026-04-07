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

#include <egf/Scheduler.h>
#include <egf/EntityIDFactory.h>
#include <egf/EntityID.h>
#include <efd/ILogger.h>
#include <efd/ServiceManager.h>
#include <efd/ConfigManager.h>
#include <egf/egfLogIDs.h>

//------------------------------------------------------------------------------------------------
using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
// Static members of EntityIDFactory
EntityID    EntityIDFactory::ms_rootID(0);
efd::UInt64 EntityIDFactory::ms_baseID = 0;

//------------------------------------------------------------------------------------------------
// Initialization functions for the static class data
bool EntityIDFactory::InitIDFactory(
    efd::UInt32 shardID,
    efd::UInt32 netID,
    efd::UInt64 baseID)
{
    ms_rootID = EntityID();

    bool result = true;
    result &= ms_rootID.SetShardID(shardID);
    result &= ms_rootID.SetNetID(netID);
    result &= ms_rootID.SetBaseID(baseID);

    ms_baseID = baseID;

    return result;
}

//------------------------------------------------------------------------------------------------
bool EntityIDFactory::InitIDFactory(EntityID entityIDValue)
{
    bool result = true;
    result &= ms_rootID.SetShardID(entityIDValue.GetShardID());
    result &= ms_rootID.SetNetID(entityIDValue.GetNetID());
    result &= ms_rootID.SetBaseID(entityIDValue.GetBaseID());

    ms_baseID = entityIDValue.GetBaseID();

    return result;
}

//------------------------------------------------------------------------------------------------
bool EntityIDFactory::ReinitIDFactory(efd::UInt32 netID)
{
    return ms_rootID.SetNetID(netID);
}

//------------------------------------------------------------------------------------------------
EntityID EntityIDFactory::GetNextID()
{
    // if ms_rootID is not valid InitIDFactory has not yet been called.
    // InitIDFactory must be called before it is valid to call GetNextID
    EE_ASSERT(ms_rootID.IsValid());

    EntityID result(ms_rootID);

    // make sure the base ID didn't roll into the scheduler ID bits.  if this happened
    // it's very bad.  log and return a zero.
    if (!result.SetBaseID(++ms_baseID))
    {
        EE_LOG(efd::kEntity, efd::ILogger::kERR0,
            ("EntityID Error: Base ID exceeded allowed range.  "
            "No more new EntityIDs will be returned until this is fixed."));

        return 0;
    }

    return result;
}
