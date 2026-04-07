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

#include <efd/AssetApplicationInfoMsg.h>
#include <efd/ILogger.h>
#include <efd/efdLogIDs.h>

using namespace efd;

EE_IMPLEMENT_CONCRETE_CLASS_INFO(AssetApplicationInfoMsg);


//------------------------------------------------------------------------------------------------
AssetApplicationInfoMsg::AssetApplicationInfoMsg()
{
}

//------------------------------------------------------------------------------------------------
AssetApplicationInfoMsg::AssetApplicationInfoMsg(const AppInfo& ci)
{
    m_info = ci;
}

//------------------------------------------------------------------------------------------------
void AssetApplicationInfoMsg::Serialize(efd::Archive& ar)
{
    IMessage::Serialize(ar);

    Serializer::SerializeObject(m_info.m_PrivateCategory, ar);
    Serializer::SerializeObject(m_info.m_Port, ar);
    Serializer::SerializeObject(m_info.m_bConnected, ar);
    Serializer::SerializeObject(m_info.m_strAppName, ar);
    Serializer::SerializeObject(m_info.m_strAppType, ar);
    Serializer::SerializeObject(m_info.m_strExtraInfo, ar);
}

//------------------------------------------------------------------------------------------------
void AssetApplicationInfoMsg::SetPrivateCategory(UInt64 category)
{
    m_info.m_PrivateCategory = category;
}

//------------------------------------------------------------------------------------------------
UInt64 AssetApplicationInfoMsg::GetPrivateCategory() const
{
    return m_info.m_PrivateCategory;
}

//------------------------------------------------------------------------------------------------
void AssetApplicationInfoMsg::SetAppPort(int port)
{
    m_info.m_Port = port;
}

//------------------------------------------------------------------------------------------------
int AssetApplicationInfoMsg::GetAppPort() const
{
    return m_info.m_Port;
}

//------------------------------------------------------------------------------------------------
void AssetApplicationInfoMsg::SetConnected(bool bConnected)
{
    m_info.m_bConnected = bConnected;
}

//------------------------------------------------------------------------------------------------
bool AssetApplicationInfoMsg::GetConnected() const
{
    return m_info.m_bConnected;
}

//------------------------------------------------------------------------------------------------
void AssetApplicationInfoMsg::SetApplicationName(const utf8string& appName)
{
    m_info.m_strAppName = appName;
}

//------------------------------------------------------------------------------------------------
const utf8string& AssetApplicationInfoMsg::GetApplicationName() const
{
    return m_info.m_strAppName;
}

//------------------------------------------------------------------------------------------------
void AssetApplicationInfoMsg::SetApplicationType(const utf8string& appType)
{
    m_info.m_strAppType = appType;
}

//------------------------------------------------------------------------------------------------
const utf8string& AssetApplicationInfoMsg::GetApplicationType() const
{
    return m_info.m_strAppType;
}

//------------------------------------------------------------------------------------------------
void AssetApplicationInfoMsg::SetExtraInfo(const utf8string& extraInfo)
{
    m_info.m_strExtraInfo = extraInfo;
}

//------------------------------------------------------------------------------------------------
const utf8string& AssetApplicationInfoMsg::GetExtraInfo() const
{
    return m_info.m_strExtraInfo;
}

//------------------------------------------------------------------------------------------------
void AssetApplicationInfoMsg::SetApplicationInfo(const AssetApplicationInfoMsg::AppInfo& ai)
{
    m_info = ai;
}

//------------------------------------------------------------------------------------------------
AssetApplicationInfoMsg::AppInfo AssetApplicationInfoMsg::GetApplicationInfo() const
{
    return m_info;
}

//------------------------------------------------------------------------------------------------

