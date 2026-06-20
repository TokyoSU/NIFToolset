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

#include "D3D11HullShader.h"
#include "D3D11ShaderProgramFactory.h"

using namespace ecr;

NiImplementRTTI(D3D11HullShader, D3D11ShaderProgram, NiTypeMask::D3D11HullShader);

//------------------------------------------------------------------------------------------------
D3D11HullShader::D3D11HullShader() :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_HULL, NULL),
    m_pHullShader(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11HullShader::D3D11HullShader(ID3D11HullShader* pHullShader, ID3DBlob* pShaderByteCode) :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_HULL, pShaderByteCode),
    m_pHullShader(pHullShader)
{
    if (m_pHullShader)
        m_pHullShader->AddRef();
}

//------------------------------------------------------------------------------------------------
D3D11HullShader::~D3D11HullShader()
{
    DestroyRendererData();
    D3D11ShaderProgramFactory::GetInstance()->RemoveHullShaderFromMap(m_name);
}

//------------------------------------------------------------------------------------------------
void D3D11HullShader::SetHullShader(ID3D11HullShader* pHullShader)
{
    if (pHullShader == m_pHullShader)
        return;

    if (pHullShader)
        pHullShader->AddRef();
    if (m_pHullShader)
        m_pHullShader->Release();
    m_pHullShader = pHullShader;
}

//------------------------------------------------------------------------------------------------
void D3D11HullShader::DestroyRendererData()
{
    if (m_pHullShader)
    {
        m_pHullShader->Release();
        m_pHullShader = NULL;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11HullShader::RecreateRendererData()
{
    D3D11ShaderProgramFactory::RecreateHullShader(this);
}

//------------------------------------------------------------------------------------------------
