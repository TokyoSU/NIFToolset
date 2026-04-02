// GAMEBASE USA LLC PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Gamebase USA LLC and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Gamebase USA LLC.
//      All Rights Reserved.
//
// Gamebase USA LLC, Research Triangle Park, NC 27709
// http://www.gamebryo.com

//--------------------------------------------------------------------------------------------------
#include "NiLightPrePassPCH.h"
#include "NiLightPrePassSDM.h"
#include <NiStream.h>

//--------------------------------------------------------------------------------------------------
NiImplementSDMConstructor(NiLightPrePass, "NiSystem NiMesh NiFloodgate NiMain");

//--------------------------------------------------------------------------------------------------
#ifdef EE_NiRenderer_EXPORT
EE_IMPLEMENT_DLLMAIN(NiRenderer);
#endif

//--------------------------------------------------------------------------------------------------
void NiLightPrePassSDM::Init()
{
    NiImplementSDMInitCheck();

    // Register implementation classes
    
}

//--------------------------------------------------------------------------------------------------
void NiLightPrePassSDM::Shutdown()
{
    NiImplementSDMShutdownCheck();

    
}

//--------------------------------------------------------------------------------------------------
