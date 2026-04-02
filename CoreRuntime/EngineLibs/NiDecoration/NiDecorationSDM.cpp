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

#include "NiDecorationPCH.h"
#include "NiDecorationSDM.h"

#include "NiDecorationPlane.h"
#include "NiDecorationCrossBillBoardGenerator.h"
#include "NiDecorationBillBoardGenerator.h"
#include "NiDecorationSimpleMeshGenerator.h"

#include "NiDecorationFactories.h"

#ifdef _USRDLL
NiImplementDllMain(NiDecoration);
#endif

//------------------------------------------------------------------------------------------------
NiImplementSDMConstructor(NiDecoration, "NiMesh NiFloodgate NiMain");
//------------------------------------------------------------------------------------------------
void NiDecorationSDM::Init()
{
    NiImplementSDMInitCheck();

    NiDecorationPlane::_SDMInit();
    NiDecorationGenerator::_SDMInit();
    NiDecorationCrossBillBoardGenerator::_SDMInit();
    NiDecorationBillBoardGenerator::_SDMInit();
    NiDecorationSimpleMeshGenerator::_SDMInit();
    NiDecorationFactories::_SDMInit();

    NiDecorationFactories::_SDMRegister();
}

//------------------------------------------------------------------------------------------------
void NiDecorationSDM::Shutdown()
{    
    NiImplementSDMShutdownCheck();

    NiDecorationPlane::_SDMShutdown();
    NiDecorationGenerator::_SDMShutdown();
    NiDecorationCrossBillBoardGenerator::_SDMShutdown();
    NiDecorationBillBoardGenerator::_SDMShutdown();
    NiDecorationSimpleMeshGenerator::_SDMShutdown();
    NiDecorationFactories::_SDMShutdown();
}

//------------------------------------------------------------------------------------------------
