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

#include "D3D11ShaderProgram.h"
#include "D3D11Error.h"

using namespace ecr;

NiImplementRTTI(D3D11ShaderProgram, NiGPUProgram);

//------------------------------------------------------------------------------------------------
D3D11ShaderProgram::D3D11ShaderProgram(
    NiGPUProgram::ProgramType shaderType,
    ID3DBlob* pShaderByteCode) :
    NiGPUProgram(shaderType),
    m_pReflection(NULL),
    m_pShaderByteCode(pShaderByteCode)
{
    if (m_pShaderByteCode)
        m_pShaderByteCode->AddRef();
}

//------------------------------------------------------------------------------------------------
D3D11ShaderProgram::~D3D11ShaderProgram()
{
    ClearShaderReflection();

    if (m_pShaderByteCode)
        m_pShaderByteCode->Release();
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgram::SetShaderByteCode(ID3DBlob* pShaderByteCode)
{
    if (pShaderByteCode == m_pShaderByteCode)
        return;

    if (pShaderByteCode)
        pShaderByteCode->AddRef();
    if (m_pShaderByteCode)
        m_pShaderByteCode->Release();
    m_pShaderByteCode = pShaderByteCode;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgram::SetShaderReflection(ID3D11ShaderReflection* pReflection)
{
    if (m_pReflection != pReflection)
    {
        if (pReflection)
            pReflection->AddRef();
        if (m_pReflection)
            m_pReflection->Release();
        m_pReflection = pReflection;
    }
}

//------------------------------------------------------------------------------------------------
