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

#include "NiDecorationFactories.h"
#include "NiDecorationCrossBillBoardGenerator.h"
#include "NiDecorationBillBoardGenerator.h"
#include "NiDecorationSimpleMeshGenerator.h"

NiTFunctorFactory<NiDecorationFunctorBase*>*
NiDecorationFactories::ms_pkFunctorFactory = NULL;
NiTFactory<NiDecorationGenerator*>* NiDecorationFactories::ms_pkGeneratorFactory = NULL;

NiFactoryDeclareDecorationGenerator(NiDecorationCrossBillBoardGenerator);
NiFactoryDeclareDecorationGenerator(NiDecorationBillBoardGenerator);
NiFactoryDeclareDecorationGenerator(NiDecorationSimpleMeshGenerator);

//------------------------------------------------------------------------------------------------
void NiDecorationFactories::_SDMInit()
{
    ms_pkFunctorFactory = NiNew NiTFunctorFactory<NiDecorationFunctorBase*>;
    ms_pkGeneratorFactory = NiNew NiTFactory<NiDecorationGenerator*>;
}

//------------------------------------------------------------------------------------------------
void NiDecorationFactories::_SDMShutdown()
{
    NiDelete ms_pkFunctorFactory;
    NiDelete ms_pkGeneratorFactory;
}

//------------------------------------------------------------------------------------------------
void NiDecorationFactories::_SDMRegister()
{
    NiFactoryRegisterDecorationGenerator(NiDecorationCrossBillBoardGenerator);
    NiFactoryRegisterDecorationGenerator(NiDecorationBillBoardGenerator);
    NiFactoryRegisterDecorationGenerator(NiDecorationSimpleMeshGenerator);
}

//------------------------------------------------------------------------------------------------
NiTFunctorFactory<NiDecorationFunctorBase*>* NiDecorationFactories::GetFunctorFactory()
{
    return ms_pkFunctorFactory;
}

//------------------------------------------------------------------------------------------------
NiTFactory<NiDecorationGenerator*>* NiDecorationFactories::GetGeneratorFactory()
{
    return ms_pkGeneratorFactory;
}

//------------------------------------------------------------------------------------------------