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
#include <efd/GenericAssetLoadResponse.h>
#include <efd/utf8string.h>
#include <efd/Category.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(GenericAssetLoadResponse);

//------------------------------------------------------------------------------------------------
GenericAssetLoadResponse::GenericAssetLoadResponse()
     : AssetLoadResponse()
     , m_assetSize(0)
     , m_assetData(0)
{
}

//------------------------------------------------------------------------------------------------
GenericAssetLoadResponse::GenericAssetLoadResponse(
     const utf8string& urn,
     const Category& responseCategory,
     AssetLoadResponse::AssetLoadResult result,
     const utf8string& assetPath,
     bool isReload,
     UInt32 assetSizeInBytes,
     char* assetData)
     : AssetLoadResponse(urn, responseCategory, result, assetPath, isReload)
     , m_assetSize(assetSizeInBytes)
     , m_assetData(assetData)
{
}

//------------------------------------------------------------------------------------------------
GenericAssetLoadResponse::~GenericAssetLoadResponse()
{
    EE_FREE(m_assetData);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 GenericAssetLoadResponse::GetAssetSize() const
{
    return m_assetSize;
}

//------------------------------------------------------------------------------------------------
void GenericAssetLoadResponse::SetAssetSize(efd::UInt32 size)
{
    m_assetSize = size;
}

//------------------------------------------------------------------------------------------------
const char* GenericAssetLoadResponse::GetAssetData() const
{
    return m_assetData;
}

//------------------------------------------------------------------------------------------------
void GenericAssetLoadResponse::SetAssetData(char *data)
{
    m_assetData = data;
}
