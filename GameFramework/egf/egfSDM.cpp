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

#include <egf/egfSDM.h>
#include <egf/BehaviorDescriptor.h>
#include <egf/IBuiltinModelImpl.h>
#include <egf/SAXEntityParser.h>
#include <egf/SAXModelParser.h>
#include <egf/ScriptContext.h>
#include <egf/PlaceableModel.h>

// The second argument is dependancies, but efd is always implicitly included
EE_IMPLEMENT_SDM_CONSTRUCTOR(egf, "");

#if defined(EE_EGF_EXPORT) && !defined(EE_PLATFORM_LINUX)
EE_IMPLEMENT_DLLMAIN(egf);
#endif

//--------------------------------------------------------------------------------------------------
void egfSDM::Init()
{
    EE_IMPLEMENT_SDM_INIT_CHECK();

    egf::BehaviorDescriptor::_SDMInit();
    egf::SAXEntityParser::_SDMInit();
    egf::SAXModelParser::_SDMInit();
    egf::ScriptContext::_SDMInit();
    egf::PlaceableModel::_SDMInit();
}
//--------------------------------------------------------------------------------------------------
void egfSDM::Shutdown()
{
    EE_IMPLEMENT_SDM_SHUTDOWN_CHECK();

    egf::ScriptContext::_SDMShutdown();
    egf::SAXModelParser::_SDMShutdown();
    egf::SAXEntityParser::_SDMShutdown();
    egf::BehaviorDescriptor::_SDMShutdown();
    egf::BuiltinModelStaticMapCleaner::_SDMShutdown();
    egf::PlaceableModel::_SDMShutdown();
}
//--------------------------------------------------------------------------------------------------
