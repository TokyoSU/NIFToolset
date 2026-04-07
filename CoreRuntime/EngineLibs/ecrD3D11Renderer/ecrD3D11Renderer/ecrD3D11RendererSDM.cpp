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
#include "ecrD3D11RendererPCH.h"

#include "ecrD3D11RendererSDM.h"
#include "D3D11Error.h"
#include "D3D11Renderer.h"
#include "D3D11ShaderCore.h"
#include "D3D11ShaderConstantMap.h"
#include "D3D11ShaderProgramCreatorHLSL.h"
#include "D3D11ShaderProgramFactory.h"

using namespace ecr;

EE_IMPLEMENT_SDM_CONSTRUCTOR(ecrD3D11Renderer, "NiMesh NiFloodgate NiMain");

#ifdef EE_ECRD3D11RENDERER_EXPORT
EE_IMPLEMENT_DLLMAIN(ecrD3D11Renderer);
#endif

//------------------------------------------------------------------------------------------------
void ecrD3D11RendererSDM::Init()
{
    EE_IMPLEMENT_SDM_INIT_CHECK();

    D3D11Error::_SDMInit();
    D3D11ShaderCore::_SDMInit();
    D3D11ShaderFactory::_SDMInit();
    D3D11ShaderProgramFactory::_SDMInit();
    D3D11ShaderProgramCreatorHLSL::_SDMInit();
    D3D11ShaderConstantMap::_SDMInit();

    D3D11Renderer::_SDMInit();
}

//------------------------------------------------------------------------------------------------
void ecrD3D11RendererSDM::Shutdown()
{
    EE_IMPLEMENT_SDM_SHUTDOWN_CHECK();

    D3D11Renderer::_SDMShutdown();

    D3D11ShaderConstantMap::_SDMShutdown();
    D3D11ShaderProgramCreatorHLSL::_SDMShutdown();
    D3D11ShaderProgramFactory::_SDMShutdown();
    D3D11ShaderFactory::_SDMShutdown();
    D3D11ShaderCore::_SDMShutdown();
    D3D11Error::_SDMShutdown();
}

//------------------------------------------------------------------------------------------------
