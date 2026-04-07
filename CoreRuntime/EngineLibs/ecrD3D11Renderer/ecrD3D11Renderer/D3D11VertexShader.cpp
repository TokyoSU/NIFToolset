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

#include "D3D11VertexShader.h"
#include "D3D11ShaderProgramFactory.h"

using namespace ecr;

NiImplementRTTI(D3D11VertexShader, D3D11ShaderProgram);

//------------------------------------------------------------------------------------------------
D3D11VertexShader::D3D11VertexShader() :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_VERTEX, NULL),
    m_pVertexShader(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11VertexShader::D3D11VertexShader(ID3D11VertexShader* pVertexShader, ID3DBlob* pShaderByteCode) :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_VERTEX, pShaderByteCode),
    m_pVertexShader(pVertexShader)
{
    if (m_pVertexShader)
        m_pVertexShader->AddRef();
}

//------------------------------------------------------------------------------------------------
D3D11VertexShader::~D3D11VertexShader()
{
    DestroyRendererData();
    D3D11ShaderProgramFactory::GetInstance()->RemoveVertexShaderFromMap(m_name);
}

//------------------------------------------------------------------------------------------------
void D3D11VertexShader::SetVertexShader(ID3D11VertexShader* pVertexShader)
{
    if (pVertexShader == m_pVertexShader)
        return;

    if (pVertexShader)
        pVertexShader->AddRef();
    if (m_pVertexShader)
        m_pVertexShader->Release();
    m_pVertexShader = pVertexShader;
}

//------------------------------------------------------------------------------------------------
void D3D11VertexShader::DestroyRendererData()
{
    if (m_pVertexShader)
    {
        m_pVertexShader->Release();
        m_pVertexShader = NULL;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11VertexShader::RecreateRendererData()
{
    D3D11ShaderProgramFactory::RecreateVertexShader(this);
}

//------------------------------------------------------------------------------------------------
