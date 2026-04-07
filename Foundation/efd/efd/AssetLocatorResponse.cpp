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

#include <efd/AssetLocatorResponse.h>
#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>
#include <efd/SerializeRoutines.h>

using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(AssetLocatorResponse);


//------------------------------------------------------------------------------------------------
AssetLocatorResponse::AssetLocatorResponse()
{
}

//------------------------------------------------------------------------------------------------
void AssetLocatorResponse::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);

    Serializer::SerializeObject(m_uri, ar);
    Serializer::SerializeObject(m_client_category, ar);

    SR_StdMap<>::Serialize(m_AssetLocations, ar);
}

//------------------------------------------------------------------------------------------------
void AssetLocatorResponse::AssetLoc::Serialize(efd::Archive& ar)
{
    Serializer::SerializeObject(name, ar);
    Serializer::SerializeObject(tagset, ar);
    Serializer::SerializeObject(classes, ar);
    Serializer::SerializeObject(url, ar);
    Serializer::SerializeObject(llid, ar);
}

//------------------------------------------------------------------------------------------------
void AssetSizeResponse::Serialize(efd::Archive& ar)
{
    AssetLocatorResponse::Serialize(ar);
    Serializer::SerializeObject(m_size, ar);
}

//------------------------------------------------------------------------------------------------
void BrowseInfoDescriptor::Serialize(efd::Archive& ar)
{
    Serializer::SerializeObject(ttype, ar);
    Serializer::SerializeObject(tvalue, ar);
    Serializer::SerializeObject(count, ar);
}

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(AssetTagsInfoResponse);

//------------------------------------------------------------------------------------------------
void AssetTagsInfoResponse::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);

    SR_ResizableArray<>::Serialize(m_TagInfo, ar);
}

//------------------------------------------------------------------------------------------------
void AssetBrowseInfoResponse::SetTagInfo(
    const efd::AWebTagType& tag_type,
    const efd::utf8string& tag_value,
    efd::UInt32 count)
{
    BrowseInfoDescriptor bi;
    bi.ttype = tag_type;
    bi.tvalue = tag_value;
    bi.count = count;
    m_TagBrowseInfo.push_back(bi);
}

//------------------------------------------------------------------------------------------------
void AssetBrowseInfoResponse::Serialize(efd::Archive& ar)
{
    AssetLocatorResponse::Serialize(ar);

    SR_ResizableArray<>::Serialize(m_TagBrowseInfo, ar);

    Serializer::SerializeObject(m_root, ar);
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
FetchAssetDataResponse::FetchAssetDataResponse()
    : m_size(0)
    , m_result(kFETCH_ASSET_NOT_FOUND)
    , m_buffer(0)
{}

//------------------------------------------------------------------------------------------------
FetchAssetDataResponse::~FetchAssetDataResponse()
{
    if (m_buffer)
    {
        EE_FREE(m_buffer);
    }
}

//------------------------------------------------------------------------------------------------
char* FetchAssetDataResponse::AllocateBuffer(efd::UInt32 size)
{
    if (m_buffer) EE_FREE(m_buffer);
    m_size = size;
    m_buffer = (char *)EE_MALLOC(size * sizeof(char));
    return m_buffer;
}

//------------------------------------------------------------------------------------------------
void FetchAssetDataResponse::Serialize(efd::Archive& ar)
{
    AssetLocatorResponse::Serialize(ar);
    Serializer::SerializeObject(m_result, ar);
    Serializer::SerializeObject(m_size, ar);
    if (ar.IsUnpacking())
    {
        AllocateBuffer(m_size);
    }
    Serializer::SerializeRawBytes((UInt8*)m_buffer, m_size, ar);
}

//------------------------------------------------------------------------------------------------
