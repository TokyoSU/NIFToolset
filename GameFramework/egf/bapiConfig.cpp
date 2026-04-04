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

#include <egf/BehaviorAPI.h>
#include <egf/bapiConfig.h>
#include <egf/Entity.h>
#include <efd/ServiceManager.h>
#include <efd/IConfigManager.h>
#include <egf/ScriptContext.h>


using namespace efd;
using namespace egf;




//-------------------------------------------------------------------------------------------------
efd::utf8string BehaviorAPI::GetConfigValue(const efd::utf8string& paramName)
{
    EE_ASSERT(!paramName.empty());

    IConfigManager* pConfigManager = g_bapiContext.GetSystemServiceAs<IConfigManager>();
    if (pConfigManager)
    {
        return pConfigManager->FindValue(paramName);
    }
    return utf8string::NullString();
}

//-------------------------------------------------------------------------------------------------
const char* BehaviorAPI::GetPlatformName()
{
#if defined(EE_PLATFORM_LINUX)
    return "Linux";
#elif defined(EE_PLATFORM_PS3)
    return "PS3";
#elif defined(EE_PLATFORM_WIN32)
    return "Win32";
#elif defined(EE_PLATFORM_XBOX360)
    return "Xbox360";
#else
    return "Unknown";
#endif
}

//-------------------------------------------------------------------------------------------------
const char* BehaviorAPI::GetBuildConfiguration()
{
#if defined(EE_CONFIG_DEBUG)
    return "Debug";
#elif defined(EE_CONFIG_RELEASE)
    return "Release";
#elif defined(EE_CONFIG_SHIPPING)
    return "Shipping";
#else
    return "Unknown";
#endif
}
