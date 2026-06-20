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

#include "D3D11GeometryShader.h"
#include "D3D11ShaderProgramFactory.h"

using namespace ecr;

NiImplementRTTI(D3D11GeometryShader, D3D11ShaderProgram, NiTypeMask::D3D11GeometryShader);

//------------------------------------------------------------------------------------------------
D3D11GeometryShader::D3D11GeometryShader() :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_GEOMETRY, NULL),
    m_pGeometryShader(NULL),
    m_pSODeclaration(NULL),
    m_numEntries(0),
    m_numOutputStreams(0),
    m_rasterizedStream(D3D11_SO_NO_RASTERIZED_STREAM)
{
    memset(m_outputStreamStrideArray, 0, sizeof(m_outputStreamStrideArray));
}

//------------------------------------------------------------------------------------------------
D3D11GeometryShader::D3D11GeometryShader(
    ID3D11GeometryShader* pGeometryShader,
    ID3DBlob* pShaderByteCode) :
    D3D11ShaderProgram(NiGPUProgram::PROGRAM_GEOMETRY, pShaderByteCode),
    m_pGeometryShader(pGeometryShader),
    m_pSODeclaration(NULL),
    m_numEntries(0),
    m_numOutputStreams(0),
    m_rasterizedStream(D3D11_SO_NO_RASTERIZED_STREAM)
{
    if (m_pGeometryShader)
        m_pGeometryShader->AddRef();
}

//------------------------------------------------------------------------------------------------
D3D11GeometryShader::~D3D11GeometryShader()
{
    if (m_pGeometryShader)
        m_pGeometryShader->Release();
    D3D11ShaderProgramFactory::GetInstance()->RemoveGeometryShaderFromMap(m_name);

    EE_FREE(m_pSODeclaration);
}

//------------------------------------------------------------------------------------------------
void D3D11GeometryShader::SetGeometryShader(ID3D11GeometryShader* pGeometryShader)
{
    if (pGeometryShader == m_pGeometryShader)
        return;

    if (pGeometryShader)
        pGeometryShader->AddRef();
    if (m_pGeometryShader)
        m_pGeometryShader->Release();
    m_pGeometryShader = pGeometryShader;
}

//------------------------------------------------------------------------------------------------
void D3D11GeometryShader::DestroyRendererData()
{
    if (m_pGeometryShader)
    {
        m_pGeometryShader->Release();
        m_pGeometryShader = NULL;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11GeometryShader::RecreateRendererData()
{
    D3D11ShaderProgramFactory::RecreateGeometryShader(this);
}

//------------------------------------------------------------------------------------------------
void D3D11GeometryShader::SetStreamOutputDeclaration(
    const D3D11_SO_DECLARATION_ENTRY* pSODeclaration,
    efd::UInt32 numEntries, 
    efd::UInt32* outputStreamStrideArray,
    efd::UInt32 numOutputStreams,
    efd::UInt32 rasterizedStream)
{
    // If any of these values is NULL/0, ensure that they all are.
    if (pSODeclaration == NULL || 
        numEntries == 0 || 
        outputStreamStrideArray == NULL ||
        numOutputStreams == 0)
    {
        EE_FREE(m_pSODeclaration);
        m_pSODeclaration = NULL;
        m_numEntries = 0;
        m_numOutputStreams = 0;
        m_rasterizedStream = D3D11_SO_NO_RASTERIZED_STREAM;
        return;
    }

    if (numOutputStreams > D3D11_SO_STREAM_COUNT)
        numOutputStreams = D3D11_SO_STREAM_COUNT;
    if (numEntries > numOutputStreams * D3D11_SO_OUTPUT_COMPONENT_COUNT)
        numEntries = numOutputStreams * D3D11_SO_OUTPUT_COMPONENT_COUNT;
    if (rasterizedStream > numOutputStreams)
        rasterizedStream = D3D11_SO_NO_RASTERIZED_STREAM;

    // Release m_pSODeclaration so it can be resized
    if (numEntries != m_numEntries)
    {
        EE_FREE(m_pSODeclaration);
        m_pSODeclaration = NULL;
        m_numEntries = numEntries;
    }

    // Allocate m_pSODeclaration
    if (m_pSODeclaration == NULL)
        m_pSODeclaration = EE_ALLOC(D3D11_SO_DECLARATION_ENTRY, numEntries);

    efd::UInt32 i = 0;
    for (; i < numEntries; i++)
        m_pSODeclaration[i] = pSODeclaration[i];

    m_numOutputStreams = numOutputStreams;
    for (i = 0; i < numOutputStreams; i++)
        m_outputStreamStrideArray[i] = outputStreamStrideArray[i];
    for (; i < D3D11_SO_STREAM_COUNT; i++)
        m_outputStreamStrideArray[i] = 0;

    m_rasterizedStream = rasterizedStream;
}

//------------------------------------------------------------------------------------------------
