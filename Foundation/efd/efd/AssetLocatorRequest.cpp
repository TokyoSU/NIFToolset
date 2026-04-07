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

#include <efd/AssetLocatorRequest.h>
#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>

using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(AssetLocatorRequest);

//------------------------------------------------------------------------------------------------
AssetLocatorRequest::AssetLocatorRequest()
{
}

//------------------------------------------------------------------------------------------------
void AssetLocatorRequest::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);

    // stream the URI
    Serializer::SerializeObject(m_uri, ar);

    // stream the path hint
    Serializer::SerializeObject(m_pathHint, ar);

    // stream the process callback category
    Serializer::SerializeObject(m_process_callback, ar);

    // stream the client callback category
    Serializer::SerializeObject(m_client_callback, ar);
    EE_ASSERT(m_client_callback != kCAT_INVALID);
}

//------------------------------------------------------------------------------------------------
void AssetLocatorRequest::SetURI(const efd::utf8string& uri_request)
{
    m_uri = uri_request;
}

//------------------------------------------------------------------------------------------------
const efd::utf8string& AssetLocatorRequest::GetURI() const
{
    return m_uri;
}

//------------------------------------------------------------------------------------------------
void AssetLocatorRequest::SetPathHint(const efd::utf8string& pathHint)
{
    m_pathHint = pathHint;
}

//--------------------------------------------------------------------------------------------------
const efd::utf8string& AssetLocatorRequest::GetPathHint() const
{
    return m_pathHint;
}

//--------------------------------------------------------------------------------------------------
void AssetLocatorRequest::CallbackCategory(const efd::Category& callback)
{
    m_process_callback = callback;
    EE_ASSERT(m_process_callback.IsValid());
}

//------------------------------------------------------------------------------------------------
const efd::Category& AssetLocatorRequest::CallbackCategory() const
{
    return m_process_callback;
}

//------------------------------------------------------------------------------------------------
void AssetLocatorRequest::ClientCategory(const efd::Category& callback)
{
    m_client_callback = callback;
    EE_ASSERT(m_client_callback.IsValid());
}

//------------------------------------------------------------------------------------------------
/*virtual */
const efd::Category& AssetLocatorRequest::ClientCategory() const
{
    EE_ASSERT(m_client_callback.IsValid());
    return m_client_callback;
}

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(FetchAssetDataRequest);

//------------------------------------------------------------------------------------------------
void FetchAssetDataRequest::Serialize(efd::Archive& ar)
{
    AssetLocatorRequest::Serialize(ar);
    Serializer::SerializeObject(m_instanceId, ar);
    Serializer::SerializeObject(m_fullFetch, ar);
    Serializer::SerializeObject(m_offset, ar);
    Serializer::SerializeObject(m_size, ar);
}

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(AssetConfigurationMessage);

//------------------------------------------------------------------------------------------------
void AssetConfigurationMessage::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_variant, ar);
    Serializer::SerializeObject(m_webRoot, ar);
    Serializer::SerializeObject(m_bForceRescan, ar);
}

//------------------------------------------------------------------------------------------------
void AssetTagsInfoRequest::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);
    Serializer::SerializeObject(m_category, ar);
}

//------------------------------------------------------------------------------------------------
