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

#include "D3D11ShaderInterface.h"
#include "D3D11MeshMaterialBinding.h"
#include "D3D11Renderer.h"

using namespace ecr;

NiImplementRTTI(ecr::D3D11ShaderInterface, NiShader, NiTypeMask::D3D11ShaderInterface);

//------------------------------------------------------------------------------------------------
D3D11ShaderInterface::D3D11ShaderInterface()
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    pRenderer->RegisterD3D11Shader(this);
}

//------------------------------------------------------------------------------------------------
D3D11ShaderInterface::~D3D11ShaderInterface()
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer)
        pRenderer->ReleaseD3D11Shader(this);
}

//------------------------------------------------------------------------------------------------
const D3D11StateBlockMask* D3D11ShaderInterface::GetStateBlockMask(
    const NiRenderCallContext&) const
{
    return NULL;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderInterface::ReferenceVertexDeclarationCache(
    NiVertexDeclarationCache vdCache) const
{
    // In D3D11, NiVertexDeclarationCache is a D3D11MeshMaterialBinding
    if (vdCache)
    {
        ((D3D11MeshMaterialBinding*)vdCache)->IncRefCount();
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderInterface::ReleaseVertexDeclarationCache(
    NiVertexDeclarationCache vdCache) const
{
    // In D3D11, NiVertexDeclarationCache is D3D11MeshMaterialBinding
    if (vdCache)
    {
        ((D3D11MeshMaterialBinding*)vdCache)->DecRefCount();
    }
}

//------------------------------------------------------------------------------------------------
