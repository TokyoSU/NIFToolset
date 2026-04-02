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

#include <efd/FoundationSDM.h>
#include <efd/MemManager.h>
#include <efd/GlobalStringTable.h>
#include <efd/SystemDesc.h>
#include <efd/Point3.h>
#include <efd/TimeType.h>
#include <efd/StackUtils.h>
#include <efd/AssertDialog.h>
#include <efd/AssetUriReader.h>
#include <efd/GlobalStringTable.h>
#include <efd/Logger.h>
#include <efd/DDEParser.h>
#include <efd/ParameterList.h>
#include <efd/StackUtils.h>
#include <efd/StaticDataManager.h>

#if defined(EE_PLATFORM_PS3)
    #include <efd/PS3/SpursManager_PS3.h>
#endif

//CODEBLOCK(1) - DO NOT DELETE THIS LINE

using namespace efd;

bool FoundationSDM::ms_initialized = false;

//------------------------------------------------------------------------------------------------
void FoundationSDM::Init()
{
    EE_IMPLEMENT_SDM_INIT_CHECK();

    if (StaticDataManager::GetInitOptions()->GetUseUnhandledExceptionFilter())
    {
        efd::StackUtils::LogOnUnhandledException(true);
    }

//CODEBLOCK(2) - DO NOT DELETE THIS LINE

    MemManager::_SDMInit();

    GlobalStringTable::_SDMInit();

    SystemDesc::_SDMInit();

    Point3::_SDMInit();
    SetInitialTimeInSec();

#if defined(EE_PLATFORM_PS3)
    SpursManager::_SDMInit();
#endif

    URIReader::_SDMInit();

    // Register the default assert dialog handler.
    AssertHandler handler = efd::AssertHelper::SetAssertHandler(efd::DisplayAssertDialog);
    // If there was already a handler installed, reinstall it because that is what the application
    // really wants.
    if (handler != NULL)
    {
        efd::AssertHelper::SetAssertHandler(handler);
    }
}

//------------------------------------------------------------------------------------------------
void FoundationSDM::Shutdown()
{
    EE_IMPLEMENT_SDM_SHUTDOWN_CHECK();

    ParameterConverterManager::RemoveAllConverters();

#if defined(EE_PLATFORM_PS3)
    SpursManager::_SDMShutdown();
#endif

    URIReader::_SDMShutdown();

    Point3::_SDMShutdown();

    SystemDesc::_SDMShutdown();

    GlobalStringTable::_SDMShutdown();

    efd::LoggerSingleton::DestroyInstance();

    MemManager::_SDMShutdown();

    if (StaticDataManager::GetInitOptions()->GetUseUnhandledExceptionFilter())
    {
        efd::StackUtils::LogOnUnhandledException(false);
    }

    StackUtils::TurnOffStackTracing();
}

//------------------------------------------------------------------------------------------------
void FoundationSDM::PerThreadInit()
{
    MemManager::_SDMPerThreadInit();
}

//------------------------------------------------------------------------------------------------
void FoundationSDM::PerThreadShutdown()
{
    MemManager::_SDMPerThreadShutdown();
}

//------------------------------------------------------------------------------------------------
