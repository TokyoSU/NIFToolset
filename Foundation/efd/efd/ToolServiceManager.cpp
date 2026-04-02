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

#include <efd/ToolServiceManager.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
EE_IMPLEMENT_CONCRETE_CLASS_INFO(ToolServiceManager);

//------------------------------------------------------------------------------------------------
void ToolServiceManager::InitializeServices()
{
    while (!m_preInitList.empty() || !m_initList.empty())
    {
        RunOnce();
    }
}

//------------------------------------------------------------------------------------------------
void ToolServiceManager::InitializeService(SSID idSysSer)
{
    // Map contains all aliases, so idSysSer can be any valid service or alias ID.
    SystemServiceMap::iterator itor = m_SysSerMap.find(idSysSer);
    if (itor != m_SysSerMap.end())
    {
        efd::SmartPointer<ServiceInfo> spServiceToInit = itor->second;
        while (spServiceToInit->m_state <= kSysServState_Initializing)
        {
            RunOnce();
        }
    }
}

//------------------------------------------------------------------------------------------------
