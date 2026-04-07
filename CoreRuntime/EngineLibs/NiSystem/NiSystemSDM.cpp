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

// Precompiled Header
#include "NiSystemPCH.h"

#include "NiSystem.h"
#include "NiSystemSDM.h"
#include "NiVersion.h"
#include "efd/Helpers.h"

//--------------------------------------------------------------------------------------------------

NiImplementSDMConstructor(NiSystem, "");

//--------------------------------------------------------------------------------------------------
// The following copyright notice may not be removed.
static char EmergentCopyright[] EE_UNUSED =
    "Copyright (c) 1996-2009 Emergent Game Technologies.";
//--------------------------------------------------------------------------------------------------
static char acGamebryoVersion[] EE_UNUSED =
    GAMEBRYO_MODULE_VERSION_STRING(NiSystem);
//--------------------------------------------------------------------------------------------------

// Sanity checks
EE_COMPILETIME_ASSERT(sizeof(NiInt8) == 1);
EE_COMPILETIME_ASSERT(sizeof(NiUInt8) == 1);
EE_COMPILETIME_ASSERT(sizeof(NiInt16) == 2);
EE_COMPILETIME_ASSERT(sizeof(NiUInt16) == 2);
EE_COMPILETIME_ASSERT(sizeof(NiInt32) == 4);
EE_COMPILETIME_ASSERT(sizeof(NiUInt32) == 4);
EE_COMPILETIME_ASSERT(sizeof(NiInt64) == 8);
EE_COMPILETIME_ASSERT(sizeof(NiUInt64) == 8);

EE_COMPILETIME_ASSERT(sizeof(NiWChar) == 2);

//--------------------------------------------------------------------------------------------------

#ifdef NISYSTEM_EXPORT
NiImplementDllMain(NiSystem);
#endif

//--------------------------------------------------------------------------------------------------
void NiSystemSDM::Init()
{
    NiImplementSDMInitCheck();

#ifndef NI_LOGGER_DISABLE
    NiLogger::_SDMInit();
#endif
}

//--------------------------------------------------------------------------------------------------
void NiSystemSDM::Shutdown()
{
    NiImplementSDMShutdownCheck();

#ifndef NI_LOGGER_DISABLE
    NiLogger::_SDMShutdown();
#endif
}

//--------------------------------------------------------------------------------------------------
