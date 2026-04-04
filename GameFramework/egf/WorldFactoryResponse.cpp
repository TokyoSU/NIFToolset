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
#include <egf/WorldFactoryResponse.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(WorldFactoryResponse);

//------------------------------------------------------------------------------------------------
WorldFactoryResponse::WorldFactoryResponse()
    : GenericAssetLoadResponse()
    , m_flatModels()
{
}

//------------------------------------------------------------------------------------------------
WorldFactoryResponse::WorldFactoryResponse(
    const utf8string& urn,
    const Category& responseCategory,
    AssetLoadResponse::AssetLoadResult result,
    const utf8string& assetPath,
    bool isReload,
    UInt32 worldFileSize,
    char* worldFileData,
    FlatModelResponseSet modelSet)
    : GenericAssetLoadResponse(
        urn,
        responseCategory,
        result,
        assetPath,
        isReload,
        worldFileSize,
        worldFileData)
    , m_flatModels(modelSet)
{
}

//------------------------------------------------------------------------------------------------
const WorldFactoryResponse::FlatModelResponseSet& WorldFactoryResponse::GetFlatModels() const
{
    return m_flatModels;
}

//------------------------------------------------------------------------------------------------
void WorldFactoryResponse::AddFlatModel(egf::FlatModelFactoryResponse* pFlatModelResponse)
{
    m_flatModels.insert(pFlatModelResponse);
}
