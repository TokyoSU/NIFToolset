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

#include "D3D11ComputeShader.h"
#include "D3D11ShaderProgramFactory.h"

using namespace ecr;

NiImplementRTTI(D3D11ComputeShader, D3D11ShaderProgram);

//------------------------------------------------------------------------------------------------
D3D11ComputeShader::D3D11ComputeShader() :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_COMPUTE, NULL),
    m_pComputeShader(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11ComputeShader::D3D11ComputeShader(ID3D11ComputeShader* pComputeShader, ID3DBlob* pShaderByteCode) :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_COMPUTE, pShaderByteCode),
    m_pComputeShader(pComputeShader)
{
    if (m_pComputeShader)
        m_pComputeShader->AddRef();
}

//------------------------------------------------------------------------------------------------
D3D11ComputeShader::~D3D11ComputeShader()
{
    DestroyRendererData();
    D3D11ShaderProgramFactory::GetInstance()->RemoveComputeShaderFromMap(m_name);
}

//------------------------------------------------------------------------------------------------
void D3D11ComputeShader::SetComputeShader(ID3D11ComputeShader* pComputeShader)
{
    if (pComputeShader == m_pComputeShader)
        return;

    if (pComputeShader)
        pComputeShader->AddRef();
    if (m_pComputeShader)
        m_pComputeShader->Release();
    m_pComputeShader = pComputeShader;
}

//------------------------------------------------------------------------------------------------
void D3D11ComputeShader::DestroyRendererData()
{
    if (m_pComputeShader)
    {
        m_pComputeShader->Release();
        m_pComputeShader = NULL;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ComputeShader::RecreateRendererData()
{
    D3D11ShaderProgramFactory::RecreateComputeShader(this);
}

//------------------------------------------------------------------------------------------------
