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
#include <efd/UniversalID.h>
#include <efd/efdLogIDs.h>
#include <efd/ILogger.h>
#include <efd/Serialize.h>


//--------------------------------------------------------------------------------------------------
namespace efd
{

//--------------------------------------------------------------------------------------------------
efd::utf8string UniversalID::ToString() const
{
    efd::utf8string str;
    if (0 == GetSystem())
    {
        str.sprintf("UserDefinedID(base)=%llu (0x%016llX)",
            GetBaseID(),
            GetValue());
    }
    else
    {
        switch (GetType())
        {
        case kIDT_UnknownType:
            str.sprintf("UnknownTypeID(base)=%llu (0x%016llX)",
                GetBaseID(),
                GetValue());
            break;

        case kIDT_EventID:
            str.sprintf("EventID(shrd:net:base)=%u:%u:%llu (0x%016llX)",
                GetShardID(),
                GetNetID(),
                GetBaseID(),
                GetValue());
            break;

        case kIDT_ChannelID:
            str.sprintf("ChannelID(usg:net:base)=%u:%u:%llu (0x%016llX)",
                GetUsage(),
                GetNetID(),
                GetBaseID(),
                GetValue());
            break;

        case kIDT_EntityID:
            str.sprintf("EntityID(usg:shrd:net:base)=%u:%u:%u:%llu (0x%016llX)",
                GetUsage(),
                GetShardID(),
                GetNetID(),
                GetBaseID(),
                GetValue());
            break;
        }
    }

    return str;
}

//--------------------------------------------------------------------------------------------------
UniversalID::IdType UniversalID::GetType() const
{
    // Only system ids have a type.
    EE_ASSERT(!IsValid() || UID_GET_VALUE(UnknownTypeID,system) == 1);

    return static_cast<IdType>(UID_GET_VALUE(UnknownTypeID,type));
}

//--------------------------------------------------------------------------------------------------
efd::Bool UniversalID::SetType(UniversalID::IdType i_type)
{
    if (i_type > (IdType)efd::BitUtils::MakeBitMask<efd::UInt32, kNumTypeBits, 0>())
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR0,
            ("ID Error: Type value (%d) exceeds space for type bits (%d).",
            i_type, kNumTypeBits));
        return false;
    }
    UID_SET_VALUE(UnknownTypeID,type, i_type);
    return true;
}

//--------------------------------------------------------------------------------------------------
UniversalID::ExpectedChannelUsage UniversalID::GetUsage() const
{
    EE_ASSERT(!IsValid() || UID_GET_VALUE(UnknownTypeID,system) == 1);

    // Only categories (channel or entity) have usage.
    EE_ASSERT(!IsValid() || UID_GET_VALUE(InternalMasks,isCategory) == 1);

    return static_cast<ExpectedChannelUsage>(UID_GET_VALUE(ChannelID,usage));
}

//--------------------------------------------------------------------------------------------------
efd::Bool UniversalID::SetUsage(UniversalID::ExpectedChannelUsage i_usage)
{
    if (i_usage > (ExpectedChannelUsage)efd::BitUtils::MakeBitMask<efd::UInt32, kNumUsageBits, 0>())
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR0,
            ("ID Error: Usage value (%d) exceeds space for usage bits (%d).",
            i_usage, kNumUsageBits));
        return false;
    }
    UID_SET_VALUE(ChannelID,usage,i_usage);
    return true;
}


//--------------------------------------------------------------------------------------------------
efd::UInt32 UniversalID::GetShardID() const
{
    switch (GetType())
    {
    case kIDT_EventID:
        return UID_GET_VALUE(EventID,shard);

    case kIDT_EntityID:
        return UID_GET_VALUE(EntityID,shard);

    default:
        EE_ASSERT_MESSAGE(!IsValid(), ("ID has no Shard bits"));
        return 0;
    }
}


//--------------------------------------------------------------------------------------------------
efd::Bool UniversalID::SetShardID(efd::UInt32 i_shard)
{
    if (i_shard > efd::BitUtils::MakeBitMask<efd::UInt32, kNumShardBits, 0>())
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR0,
            ("ID Error: Shard value (%d) exceeds available space for shard ID bits (%d).",
            i_shard, kNumShardBits));
        return false;
    }

    switch (GetType())
    {
    case kIDT_EventID:
        UID_SET_VALUE(EventID,shard , i_shard);
        break;

    case kIDT_EntityID:
        UID_SET_VALUE(EntityID, shard, i_shard);
        break;

    default:
        EE_FAIL("Type must be set to Event or Entity before the shard can be set");
        return false;
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
efd::UInt32 UniversalID::GetNetID() const
{
    switch (GetType())
    {
    case kIDT_EventID:
        return UID_GET_VALUE(EventID,net);

    case kIDT_ChannelID:
        return UID_GET_VALUE(ChannelID,net);

    case kIDT_EntityID:
        return UID_GET_VALUE(EntityID,net);

    default:
        EE_FAIL("ID has no Shard bits");
        return 0;
    }
}


//--------------------------------------------------------------------------------------------------
efd::Bool UniversalID::SetNetID(efd::UInt32 i_netID)
{
    if (i_netID > efd::BitUtils::MakeBitMask<efd::UInt32, kNumNetBits, 0>())
    {
        EE_LOG(efd::kFoundation, efd::ILogger::kERR0,
            ("ID Error: Net value (%d) exceeds available space for net ID bits (%d).",
            i_netID, kNumNetBits));
        return false;
    }

    switch (GetType())
    {
    case kIDT_EventID:
        UID_SET_VALUE(EventID,net , i_netID);
        break;

    case kIDT_ChannelID:
        UID_SET_VALUE(ChannelID,net , i_netID);
        break;

    case kIDT_EntityID:
        UID_SET_VALUE(EntityID,net,i_netID);
        break;

    default:
        EE_FAIL("Type must be set to Event, Channel or Entity before the NetID can be set");
        return false;
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
efd::UInt64 UniversalID::GetBaseID() const
{
    if (0 == UID_GET_VALUE(UserDefinedID,system))
    {
        return UID_GET_VALUE(UserDefinedID,base);
    }

    switch (GetType())
    {
    default:
        EE_FAIL("This should be unreachable code");
        // fall through ...

    case kIDT_UnknownType:
        return UID_GET_VALUE(UnknownTypeID,base);

    case kIDT_EventID:
        return UID_GET_VALUE(EventID,base);

    case kIDT_ChannelID:
        return UID_GET_VALUE(ChannelID,base);

    case kIDT_EntityID:
        return UID_GET_VALUE(EntityID,base);
    }
}

//--------------------------------------------------------------------------------------------------
efd::Bool UniversalID::SetBaseID(efd::UInt64 i_base)
{
    if (0 == UID_GET_VALUE(UserDefinedID,system))
    {
        UID_SET_VALUE(UserDefinedID, base , i_base);
    }
    else
    {
        switch (GetType())
        {
        case kIDT_UnknownType:
            UID_SET_VALUE(UnknownTypeID,base , i_base);
            break;

        case kIDT_EventID:
            UID_SET_VALUE(EventID,base , i_base);
            break;

        case kIDT_ChannelID:
            UID_SET_VALUE(ChannelID,base , i_base);
            break;

        case kIDT_EntityID:
            UID_SET_VALUE(EntityID,base , i_base);
            break;
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void UniversalID::Serialize(efd::Archive& ar)
{
    Serializer::SerializeObject(m_uid, ar);
}

} // end namespace efd
