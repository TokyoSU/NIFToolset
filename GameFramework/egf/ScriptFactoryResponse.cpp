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
#include <egf/ScriptFactoryResponse.h>

using namespace efd;
using namespace egf;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ScriptFactoryResponse);

//------------------------------------------------------------------------------------------------
ScriptFactoryResponse::ScriptFactoryResponse()
    : AssetLoadResponse()
    , m_scriptName("")
{

}

//------------------------------------------------------------------------------------------------
ScriptFactoryResponse::ScriptFactoryResponse(
    const efd::utf8string& urn,
    const efd::utf8string& scriptName,
    const efd::Category& responseCategory,
    efd::AssetLoadResponse::AssetLoadResult result,
    const efd::utf8string& assetPath,
    bool isReload,
    const ScriptContentList& scriptList)
    : AssetLoadResponse(urn, responseCategory, result, assetPath, isReload)
    , m_scriptList(scriptList)
    , m_scriptName(scriptName)
{
}

//------------------------------------------------------------------------------------------------
ScriptFactoryResponse::~ScriptFactoryResponse()
{
    m_scriptList.clear();
}

//------------------------------------------------------------------------------------------------
const ScriptFactoryResponse::ScriptContentList& ScriptFactoryResponse::GetScripts() const
{
    return m_scriptList;
}

//------------------------------------------------------------------------------------------------
void ScriptFactoryResponse::AddScript(const utf8string& scriptName, const utf8string& scriptData)
{
    ScriptContent data;
    data.m_name = scriptName;
    data.m_data = scriptData;

    m_scriptList.push_back(data);
}

//------------------------------------------------------------------------------------------------
const utf8string& ScriptFactoryResponse::GetScriptName() const
{
    return m_scriptName;
}
