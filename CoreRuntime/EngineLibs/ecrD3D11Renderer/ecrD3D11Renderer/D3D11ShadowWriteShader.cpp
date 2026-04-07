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

#include "D3D11Renderer.h"
#include "D3D11RenderStateManager.h"
#include "D3D11ShadowWriteShader.h"

using namespace ecr;

efd::Bool D3D11ShadowWriteShader::ms_renderBackfaces = true;
NiImplementRTTI(D3D11ShadowWriteShader, D3D11FragmentShader);

//------------------------------------------------------------------------------------------------
D3D11ShadowWriteShader::D3D11ShadowWriteShader(NiMaterialDescriptor* pDesc) : 
    D3D11FragmentShader(pDesc)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11ShadowWriteShader::~D3D11ShadowWriteShader()
{
    /* */
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShadowWriteShader::PreProcessPipeline(const NiRenderCallContext& callContext)
{
    efd::UInt32 returnValue = D3D11FragmentShader::PreProcessPipeline(callContext);

    // If set to render backfaces, flip the direction of the cull mode render
    // state.
    if (ms_renderBackfaces)
    {
        D3D11RenderStateManager* pRenderState = 
            D3D11Renderer::GetRenderer()->GetRenderStateManager();

        D3D11_RASTERIZER_DESC rasterizerDesc;
        pRenderState->GetRasterizerStateDesc(rasterizerDesc);

        if (rasterizerDesc.CullMode == D3D11_CULL_FRONT)
            rasterizerDesc.CullMode = D3D11_CULL_BACK;
        else if (rasterizerDesc.CullMode == D3D11_CULL_BACK)
            rasterizerDesc.CullMode = D3D11_CULL_FRONT;

        pRenderState->SetRasterizerStateDesc(
            rasterizerDesc, D3D11RenderStateManager::RSVALID_CULLMODE);
    }

    return returnValue;
}

//------------------------------------------------------------------------------------------------
