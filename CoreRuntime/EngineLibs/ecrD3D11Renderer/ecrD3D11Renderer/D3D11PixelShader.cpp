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

#include "D3D11PixelShader.h"
#include "D3D11ShaderProgramFactory.h"

using namespace ecr;

NiImplementRTTI(D3D11PixelShader, D3D11ShaderProgram);

//------------------------------------------------------------------------------------------------
D3D11PixelShader::D3D11PixelShader() :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_PIXEL, NULL),
    m_pPixelShader(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11PixelShader::D3D11PixelShader(ID3D11PixelShader* pPixelShader, ID3DBlob* pShaderByteCode) :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_PIXEL, pShaderByteCode),
    m_pPixelShader(pPixelShader)
{
    if (m_pPixelShader)
        m_pPixelShader->AddRef();
}

//------------------------------------------------------------------------------------------------
D3D11PixelShader::~D3D11PixelShader()
{
    if (m_pPixelShader)
        m_pPixelShader->Release();
    D3D11ShaderProgramFactory::GetInstance()->RemovePixelShaderFromMap(m_name);
}

//------------------------------------------------------------------------------------------------
void D3D11PixelShader::SetPixelShader(ID3D11PixelShader* pPixelShader)
{
    if (pPixelShader == m_pPixelShader)
        return;

    if (pPixelShader)
        pPixelShader->AddRef();
    if (m_pPixelShader)
        m_pPixelShader->Release();
    m_pPixelShader = pPixelShader;
}

//------------------------------------------------------------------------------------------------
void D3D11PixelShader::DestroyRendererData()
{
    if (m_pPixelShader)
    {
        m_pPixelShader->Release();
        m_pPixelShader = NULL;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11PixelShader::RecreateRendererData()
{
    D3D11ShaderProgramFactory::RecreatePixelShader(this);
}

//------------------------------------------------------------------------------------------------

