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

#include <efd/utf8string.h>
#include <egf/FlatModel.h>
#include <egf/FlatModelFactoryResponse.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(FlatModelFactoryResponse);

//------------------------------------------------------------------------------------------------
FlatModelFactoryResponse::FlatModelFactoryResponse()
    : AssetLoadResponse()
    , m_modelName("")
    , m_flatModelMap()
{
}

//------------------------------------------------------------------------------------------------
FlatModelFactoryResponse::FlatModelFactoryResponse(
    const efd::utf8string& urn,
    const efd::Category& responseCategory,
    efd::AssetLoadResponse::AssetLoadResult result,
    const efd::utf8string& assetPath,
    bool isReload,
    const efd::utf8string& modelName,
    FlatModelMap flatModelMap,
    DependentScriptSet scriptFactoryResponseSet)
    : AssetLoadResponse(urn, responseCategory, result, assetPath, isReload)
    , m_modelName(modelName)
    , m_flatModelMap(flatModelMap)
    , m_dependentScriptSet(scriptFactoryResponseSet)
{
}

//------------------------------------------------------------------------------------------------
FlatModelPtr FlatModelFactoryResponse::GetFlatModel() const
{
    FlatModelMap::iterator it = m_flatModelMap.find(m_modelName);
    if (it != m_flatModelMap.end())
    {
        return it->second;
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
const utf8string& FlatModelFactoryResponse::GetModelName() const
{
    return m_modelName;
}

//------------------------------------------------------------------------------------------------
const FlatModelFactoryResponse::FlatModelMap& FlatModelFactoryResponse::GetFlatModelMap() const
{
    return m_flatModelMap;
}

//------------------------------------------------------------------------------------------------
void FlatModelFactoryResponse::AddDependentScript(egf::ScriptFactoryResponse* pResponse)
{
    m_dependentScriptSet.insert(pResponse);
}

//------------------------------------------------------------------------------------------------
const FlatModelFactoryResponse::DependentScriptSet&
FlatModelFactoryResponse::GetDependentScripts() const
{
    return m_dependentScriptSet;
}
