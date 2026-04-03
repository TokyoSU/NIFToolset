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

#include "D3D11DomainShader.h"
#include "D3D11ShaderProgramFactory.h"

using namespace ecr;

NiImplementRTTI(D3D11DomainShader, D3D11ShaderProgram);

//------------------------------------------------------------------------------------------------
D3D11DomainShader::D3D11DomainShader() :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_DOMAIN, NULL),
    m_pDomainShader(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11DomainShader::D3D11DomainShader(ID3D11DomainShader* pDomainShader, ID3DBlob* pShaderByteCode) :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_DOMAIN, pShaderByteCode),
    m_pDomainShader(pDomainShader)
{
    if (m_pDomainShader)
        m_pDomainShader->AddRef();
}

//------------------------------------------------------------------------------------------------
D3D11DomainShader::~D3D11DomainShader()
{
    DestroyRendererData();
    D3D11ShaderProgramFactory::GetInstance()->RemoveDomainShaderFromMap(m_name);
}

//------------------------------------------------------------------------------------------------
void D3D11DomainShader::SetDomainShader(ID3D11DomainShader* pDomainShader)
{
    if (pDomainShader == m_pDomainShader)
        return;

    if (pDomainShader)
        pDomainShader->AddRef();
    if (m_pDomainShader)
        m_pDomainShader->Release();
    m_pDomainShader = pDomainShader;
}

//------------------------------------------------------------------------------------------------
void D3D11DomainShader::DestroyRendererData()
{
    if (m_pDomainShader)
    {
        m_pDomainShader->Release();
        m_pDomainShader = NULL;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DomainShader::RecreateRendererData()
{
    D3D11ShaderProgramFactory::RecreateDomainShader(this);
}

//------------------------------------------------------------------------------------------------
