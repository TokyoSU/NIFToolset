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

#include "D3D11ShaderProgramFactory.h"
#include "D3D11Error.h"
#include "D3D11Renderer.h"
#include "D3D11ShaderProgramCreator.h"
#include "D3D11ShaderFactory.h"

#include <NiFilename.h>

using namespace ecr;

D3D11ShaderProgramFactory* D3D11ShaderProgramFactory::ms_pFactory = NULL;

//------------------------------------------------------------------------------------------------
D3D11ShaderProgramFactory::D3D11ShaderProgramFactory() :
    m_vertexShaderMap(59),
    m_hullShaderMap(59),
    m_domainShaderMap(59),
    m_geometryShaderMap(59),
    m_pixelShaderMap(59),
    m_computeShaderMap(59),
    m_shaderCreators(5)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11ShaderProgramFactory::~D3D11ShaderProgramFactory()
{
    m_vertexShaderMap.RemoveAll();
    m_hullShaderMap.RemoveAll();
    m_domainShaderMap.RemoveAll();
    m_geometryShaderMap.RemoveAll();
    m_pixelShaderMap.RemoveAll();
    m_computeShaderMap.RemoveAll();

    m_shaderCreators.RemoveAll();

    RemoveAllProgramDirectories();
}

//------------------------------------------------------------------------------------------------
D3D11ShaderProgramFactory* D3D11ShaderProgramFactory::GetInstance()
{
    return ms_pFactory;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateVertexShaderFromFile(
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    D3D11VertexShaderPtr& spVertexShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pFileName == NULL || pFileName[0] == '\0')
        return NULL;

    // See if it exists already
    D3D11VertexShader* pVertexShader = ms_pFactory->GetVertexShader(pShaderName);
    if (pVertexShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pVertexShader->GetVertexShader() != NULL);
        spVertexShader = pVertexShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    ID3DBlob* pShaderCode = ReadShaderFile(pFileName);
    if (pShaderCode == NULL)
    {
        // Can't find shader file
        D3D11Error::ReportWarning(
            "Failure reading shader program file %s in "
            __FUNCTION__,
            pFileName);

        return NULL;
    }

    efd::Bool success = pCreator->CreateVertexShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        false,
        spVertexShader);

    EE_ASSERT(success == false || spVertexShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertVertexShaderIntoMap(spVertexShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateVertexShaderFromBlob(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    efd::Bool isCompiled,
    D3D11VertexShaderPtr& spVertexShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pShaderCode == 0)
        return NULL;

    // See if it exists already
    D3D11VertexShader* pVertexShader = ms_pFactory->GetVertexShader(pShaderName);
    if (pVertexShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pVertexShader->GetVertexShader() != NULL);
        spVertexShader = pVertexShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    efd::Bool success = pCreator->CreateVertexShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        isCompiled,
        spVertexShader);

    EE_ASSERT(success == false || spVertexShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertVertexShaderIntoMap(spVertexShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateHullShaderFromFile(
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    D3D11HullShaderPtr& spHullShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pFileName == NULL || pFileName[0] == '\0')
        return NULL;

    // See if it exists already
    D3D11HullShader* pHullShader = ms_pFactory->GetHullShader(pShaderName);
    if (pHullShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pHullShader->GetHullShader() != NULL);
        spHullShader = pHullShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    ID3DBlob* pShaderCode = ReadShaderFile(pFileName);
    if (pShaderCode == NULL)
    {
        // Can't find shader file
        D3D11Error::ReportWarning(
            "Failure reading shader program file %s in "
            __FUNCTION__,
            pFileName);

        return NULL;
    }

    efd::Bool success = pCreator->CreateHullShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        false,
        spHullShader);

    EE_ASSERT(success == false || spHullShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertHullShaderIntoMap(spHullShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateHullShaderFromBlob(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    efd::Bool isCompiled,
    D3D11HullShaderPtr& spHullShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pShaderCode == 0)
        return NULL;

    // See if it exists already
    D3D11HullShader* pHullShader = ms_pFactory->GetHullShader(pShaderName);
    if (pHullShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pHullShader->GetHullShader() != NULL);
        spHullShader = pHullShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    efd::Bool success = pCreator->CreateHullShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        isCompiled,
        spHullShader);

    EE_ASSERT(success == false || spHullShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertHullShaderIntoMap(spHullShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateDomainShaderFromFile(
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    D3D11DomainShaderPtr& spDomainShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pFileName == NULL || pFileName[0] == '\0')
        return NULL;

    // See if it exists already
    D3D11DomainShader* pDomainShader = ms_pFactory->GetDomainShader(pShaderName);
    if (pDomainShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pDomainShader->GetDomainShader() != NULL);
        spDomainShader = pDomainShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    ID3DBlob* pShaderCode = ReadShaderFile(pFileName);
    if (pShaderCode == NULL)
    {
        // Can't find shader file
        D3D11Error::ReportWarning(
            "Failure reading shader program file %s in "
            __FUNCTION__,
            pFileName);

        return NULL;
    }

    efd::Bool success = pCreator->CreateDomainShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        false,
        spDomainShader);

    EE_ASSERT(success == false || spDomainShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertDomainShaderIntoMap(spDomainShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateDomainShaderFromBlob(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    efd::Bool isCompiled,
    D3D11DomainShaderPtr& spDomainShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pShaderCode == 0)
        return NULL;

    // See if it exists already
    D3D11DomainShader* pDomainShader = ms_pFactory->GetDomainShader(pShaderName);
    if (pDomainShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pDomainShader->GetDomainShader() != NULL);
        spDomainShader = pDomainShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    efd::Bool success = pCreator->CreateDomainShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        isCompiled,
        spDomainShader);

    EE_ASSERT(success == false || spDomainShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertDomainShaderIntoMap(spDomainShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateGeometryShaderFromFile(
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    D3D11GeometryShaderPtr& spGeometryShader)
{
    return CreateGeometryShaderWithStreamOutputFromFile(
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        NULL,
        0,
        NULL,
        0,
        D3D11_SO_NO_RASTERIZED_STREAM,
        spGeometryShader);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateGeometryShaderFromBlob(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    efd::Bool isCompiled,
    D3D11GeometryShaderPtr& spGeometryShader)
{
    return CreateGeometryShaderWithStreamOutputFromBlob(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        NULL,
        0,
        NULL,
        0,
        D3D11_SO_NO_RASTERIZED_STREAM,
        isCompiled,
        spGeometryShader);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateGeometryShaderWithStreamOutputFromFile(
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    const D3D11_SO_DECLARATION_ENTRY* pSODeclaration,
    efd::UInt32 numEntries,
    efd::UInt32* outputStreamStrideArray,
    efd::UInt32 numOutputStreams,
    efd::UInt32 rasterizedStream,
    D3D11GeometryShaderPtr& spGeometryShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pFileName == NULL || pFileName[0] == '\0')
        return NULL;

    // See if it exists already
    D3D11GeometryShader* pGeometryShader = ms_pFactory->GetGeometryShader(pShaderName);
    if (pGeometryShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pGeometryShader->GetGeometryShader() != NULL);
        spGeometryShader = pGeometryShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    ID3DBlob* pShaderCode = ReadShaderFile(pFileName);
    if (pShaderCode == NULL)
    {
        D3D11Error::ReportWarning(
            "Failure reading shader program file %s in "
            __FUNCTION__,
            pFileName);

        // Can't find shader file
        return NULL;
    }

    efd::Bool success = pCreator->CreateGeometryShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        pSODeclaration,
        numEntries,
        outputStreamStrideArray,
        numOutputStreams,
        rasterizedStream,
        false,
        spGeometryShader);

    EE_ASSERT(success == false || spGeometryShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertGeometryShaderIntoMap(spGeometryShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateGeometryShaderWithStreamOutputFromBlob(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    const D3D11_SO_DECLARATION_ENTRY* pSODeclaration,
    efd::UInt32 numEntries,
    efd::UInt32* outputStreamStrideArray,
    efd::UInt32 numOutputStreams,
    efd::UInt32 rasterizedStream,
    efd::Bool isCompiled,
    D3D11GeometryShaderPtr& spGeometryShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pShaderCode == NULL)
        return NULL;

    // See if it exists already
    D3D11GeometryShader* pGeometryShader = ms_pFactory->GetGeometryShader(pShaderName);
    if (pGeometryShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pGeometryShader->GetGeometryShader() != NULL);
        spGeometryShader = pGeometryShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    efd::Bool success = pCreator->CreateGeometryShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        pSODeclaration,
        numEntries,
        outputStreamStrideArray,
        numOutputStreams,
        rasterizedStream,
        isCompiled,
        spGeometryShader);

    EE_ASSERT(success == false || spGeometryShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertGeometryShaderIntoMap(spGeometryShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreatePixelShaderFromFile(
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    D3D11PixelShaderPtr& spPixelShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pFileName == NULL || pFileName[0] == '\0')
        return NULL;

    // See if it exists already
    D3D11PixelShader* pPixelShader = ms_pFactory->GetPixelShader(pShaderName);
    if (pPixelShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pPixelShader->GetPixelShader() != NULL);
        spPixelShader = pPixelShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    ID3DBlob* pShaderCode = ReadShaderFile(pFileName);
    if (pShaderCode == NULL)
    {
        D3D11Error::ReportWarning(
            "Failure reading shader program file %s in "
            __FUNCTION__,
            pFileName);

        // Can't find shader file
        return NULL;
    }

    efd::Bool success = pCreator->CreatePixelShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        false,
        spPixelShader);

    EE_ASSERT(success == false || spPixelShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertPixelShaderIntoMap(spPixelShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreatePixelShaderFromBlob(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    efd::Bool isCompiled,
    D3D11PixelShaderPtr& spPixelShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pShaderCode == 0)
        return NULL;

    // See if it exists already
    D3D11PixelShader* pPixelShader = ms_pFactory->GetPixelShader(pShaderName);
    if (pPixelShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pPixelShader->GetPixelShader() != NULL);
        spPixelShader = pPixelShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    efd::Bool success = pCreator->CreatePixelShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        isCompiled,
        spPixelShader);

    EE_ASSERT(success == false || spPixelShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertPixelShaderIntoMap(spPixelShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateComputeShaderFromFile(
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    D3D11ComputeShaderPtr& spComputeShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pFileName == NULL || pFileName[0] == '\0')
        return NULL;

    // See if it exists already
    D3D11ComputeShader* pComputeShader = ms_pFactory->GetComputeShader(pShaderName);
    if (pComputeShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pComputeShader->GetComputeShader() != NULL);
        spComputeShader = pComputeShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    ID3DBlob* pShaderCode = ReadShaderFile(pFileName);
    if (pShaderCode == NULL)
    {
        // Can't find shader file
        D3D11Error::ReportWarning(
            "Failure reading shader program file %s in "
            __FUNCTION__,
            pFileName);

        return NULL;
    }

    efd::Bool success = pCreator->CreateComputeShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        false,
        spComputeShader);

    EE_ASSERT(success == false || spComputeShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertComputeShaderIntoMap(spComputeShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::CreateComputeShaderFromBlob(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName,
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude,
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget,
    efd::UInt32 flags,
    const efd::Char* pShaderName,
    efd::Bool isCompiled,
    D3D11ComputeShaderPtr& spComputeShader)
{
    if (ms_pFactory == NULL)
        return NULL;

    if (pShaderCode == 0)
        return NULL;

    // See if it exists already
    D3D11ComputeShader* pComputeShader = ms_pFactory->GetComputeShader(pShaderName);
    if (pComputeShader)
    {
        // Already loaded... Return it
        EE_ASSERT(pComputeShader->GetComputeShader() != NULL);
        spComputeShader = pComputeShader;
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = ms_pFactory->GetShaderCreator(pFileName);

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        return NULL;
    }

    efd::Bool success = pCreator->CreateComputeShader(
        pShaderCode,
        pFileName,
        pDefines,
        pInclude,
        pEntryPoint,
        pShaderTarget,
        flags,
        pShaderName,
        isCompiled,
        spComputeShader);

    EE_ASSERT(success == false || spComputeShader != NULL);

    // Insert it in the list
    if (success)
        ms_pFactory->InsertComputeShaderIntoMap(spComputeShader);

    return success;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderProgramFactory::GetUniversalShaderCreationFlags()
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    return pRenderer->GetGlobalShaderCreationFlags();
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::SetUniversalShaderCreationFlags(
    efd::UInt32 flags)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    pRenderer->SetGlobalShaderCreationFlags(flags);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::RecreateVertexShader(D3D11VertexShader* pShader)
{
    if (!ms_pFactory || !pShader)
        return false;

    if (pShader->GetVertexShader())
    {
        // Prevent shader from being reloaded multiple times
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = pShader->GetCreator();

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        // How was it loaded in the first place?
        D3D11Error::ReportWarning(
            "Could not identify shader program creator for shader program %s in "
            __FUNCTION__,
            pShader->GetName());
        return false;
    }

    return pCreator->RecreateVertexShader(pShader);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::RecreateHullShader(D3D11HullShader* pShader)
{
    if (!ms_pFactory || !pShader)
        return false;

    if (pShader->GetHullShader())
    {
        // Prevent shader from being reloaded multiple times
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = pShader->GetCreator();

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        // How was it loaded in the first place?
        D3D11Error::ReportWarning(
            "Could not identify shader program creator for shader program %s in "
            __FUNCTION__,
            pShader->GetName());
        return false;
    }

    return pCreator->RecreateHullShader(pShader);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::RecreateDomainShader(D3D11DomainShader* pShader)
{
    if (!ms_pFactory || !pShader)
        return false;

    if (pShader->GetDomainShader())
    {
        // Prevent shader from being reloaded multiple times
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = pShader->GetCreator();

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        // How was it loaded in the first place?
        D3D11Error::ReportWarning(
            "Could not identify shader program creator for shader program %s in "
            __FUNCTION__,
            pShader->GetName());
        return false;
    }

    return pCreator->RecreateDomainShader(pShader);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::RecreateGeometryShader(D3D11GeometryShader* pShader)
{
    if (!ms_pFactory || !pShader)
        return false;

    if (pShader->GetGeometryShader())
    {
        // Prevent shader from being reloaded multiple times
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = pShader->GetCreator();

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        // How was it loaded in the first place?
        D3D11Error::ReportWarning(
            "Could not identify shader program creator for shader program %s in "
            __FUNCTION__,
            pShader->GetName());
        return false;
    }

    return pCreator->RecreateGeometryShader(pShader);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::RecreatePixelShader(D3D11PixelShader* pShader)
{
    if (!ms_pFactory || !pShader)
        return false;

    if (pShader->GetPixelShader())
    {
        // Prevent shader from being reloaded multiple times
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = pShader->GetCreator();

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        // How was it loaded in the first place?
        D3D11Error::ReportWarning(
            "Could not identify shader program creator for shader program %s in "
            __FUNCTION__,
            pShader->GetName());
        return false;
    }

    return pCreator->RecreatePixelShader(pShader);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::RecreateComputeShader(D3D11ComputeShader* pShader)
{
    if (!ms_pFactory || !pShader)
        return false;

    if (pShader->GetComputeShader())
    {
        // Prevent shader from being reloaded multiple times
        return true;
    }

    D3D11ShaderProgramCreator* pCreator = pShader->GetCreator();

    if (pCreator == NULL)
    {
        // No knowledge of this shader format - can't load it
        // How was it loaded in the first place?
        D3D11Error::ReportWarning(
            "Could not identify shader program creator for shader program %s in "
            __FUNCTION__,
            pShader->GetName());
        return false;
    }

    return pCreator->RecreateComputeShader(pShader);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::ReleaseVertexShader(D3D11VertexShader* pShader)
{
    if (!ms_pFactory)
        return;

    if (!pShader)
        return;

    efd::FixedString shaderName = pShader->GetName();
    ms_pFactory->RemoveVertexShaderFromMap(shaderName);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::ReleaseHullShader(D3D11HullShader* pShader)
{
    if (!ms_pFactory)
        return;

    if (!pShader)
        return;

    efd::FixedString shaderName = pShader->GetName();
    ms_pFactory->RemoveHullShaderFromMap(shaderName);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::ReleaseDomainShader(D3D11DomainShader* pShader)
{
    if (!ms_pFactory)
        return;

    if (!pShader)
        return;

    efd::FixedString shaderName = pShader->GetName();
    ms_pFactory->RemoveDomainShaderFromMap(shaderName);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::ReleaseGeometryShader(D3D11GeometryShader* pShader)
{
    if (!ms_pFactory)
        return;

    if (!pShader)
        return;

    efd::FixedString shaderName = pShader->GetName();
    ms_pFactory->RemoveGeometryShaderFromMap(shaderName);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::ReleasePixelShader(D3D11PixelShader* pShader)
{
    if (!ms_pFactory)
        return;

    if (!pShader)
        return;

    efd::FixedString shaderName = pShader->GetName();
    ms_pFactory->RemovePixelShaderFromMap(shaderName);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::ReleaseComputeShader(D3D11ComputeShader* pShader)
{
    if (!ms_pFactory)
        return;

    if (!pShader)
        return;

    efd::FixedString shaderName = pShader->GetName();
    ms_pFactory->RemoveComputeShaderFromMap(shaderName);
}

//------------------------------------------------------------------------------------------------
const efd::Char* D3D11ShaderProgramFactory::GetFirstProgramDirectory(NiTListIterator& iter)
{
    iter = m_programDirectories.GetHeadPos();
    return GetNextProgramDirectory(iter);
}

//------------------------------------------------------------------------------------------------
const efd::Char* D3D11ShaderProgramFactory::GetNextProgramDirectory(NiTListIterator& iter)
{
    if (iter)
        return m_programDirectories.GetNext(iter);

    return 0;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::AddProgramDirectory(const efd::Char* pDirectory)
{
    if (pDirectory == NULL || pDirectory[0] == '\0')
        return;

    // First, check if it's already in there...
    efd::Char* pCheckName;
    efd::Char sourcePath[efd::EE_MAX_PATH];

    efd::Strcpy(sourcePath, efd::EE_MAX_PATH, pDirectory);
    const efd::UInt32 stringLen = efd::Strlen(sourcePath);
    for (efd::UInt32 i = 0; i < stringLen; i++)
    {
        if (sourcePath[i] == '/')
            sourcePath[i] = '\\';
    }

    NiTListIterator iter = m_programDirectories.GetHeadPos();
    while (iter)
    {
        pCheckName = m_programDirectories.GetNext(iter);
        if (pCheckName)
        {
            // We know that the stored one is correct as we convert it when
            // we add it...
            if (efd::Stricmp(pCheckName, sourcePath) == 0)
            {
                // Already in there...
                return;
            }
        }
    }

    // Not in there... add it!
    const efd::UInt32 newStringLen = efd::Strlen(sourcePath) + 1;
    efd::Char* pNewAdd = EE_ALLOC(efd::Char, newStringLen);
    EE_ASSERT(pNewAdd);
    efd::Strcpy(pNewAdd, newStringLen, sourcePath);

    m_programDirectories.AddHead(pNewAdd);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::RemoveProgramDirectory(const efd::Char* pDirectory)
{
    if (pDirectory == NULL || pDirectory[0] == '\0')
        return;

    // First, check if it's already in there...
    efd::Char* pCheckName;
    efd::Char sourcePath[efd::EE_MAX_PATH];

    efd::Strcpy(sourcePath, efd::EE_MAX_PATH, pDirectory);
    const efd::UInt32 stringLen = efd::Strlen(sourcePath);
    for (efd::UInt32 ui = 0; ui < stringLen; ui++)
    {
        if (sourcePath[ui] == '/')
            sourcePath[ui] = '\\';
    }

    NiTListIterator iter = m_programDirectories.GetHeadPos();
    while (iter)
    {
        pCheckName = m_programDirectories.GetNext(iter);
        if (pCheckName)
        {
            // We know that the stored one is correct as we convert it when
            // we add it...
            if (efd::Stricmp(pCheckName, sourcePath) == 0)
            {
                m_programDirectories.Remove(pCheckName);
                return;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::RemoveAllProgramDirectories()
{
    // First, check if it's already in there...
    efd::Char* pCheckName;

    NiTListIterator iter = m_programDirectories.GetHeadPos();
    while (iter)
    {
        pCheckName = m_programDirectories.GetNext(iter);
        if (pCheckName)
        {
            m_programDirectories.Remove(pCheckName);
            EE_FREE(pCheckName);
        }
    }
}

//------------------------------------------------------------------------------------------------
D3D11VertexShader* D3D11ShaderProgramFactory::GetVertexShader(const efd::FixedString& shaderName)
{
    D3D11VertexShader* pVertexShader = NULL;

    NiFixedString temp((const char*)shaderName);
    if (m_vertexShaderMap.GetAt(temp, pVertexShader))
        return pVertexShader;
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
D3D11HullShader* D3D11ShaderProgramFactory::GetHullShader(const efd::FixedString& shaderName)
{
    D3D11HullShader* pHullShader = NULL;

    NiFixedString temp((const char*)shaderName);
    if (m_hullShaderMap.GetAt(temp, pHullShader))
        return pHullShader;
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
D3D11DomainShader* D3D11ShaderProgramFactory::GetDomainShader(const efd::FixedString& shaderName)
{
    D3D11DomainShader* pDomainShader = NULL;

    NiFixedString temp((const char*)shaderName);
    if (m_domainShaderMap.GetAt(temp, pDomainShader))
        return pDomainShader;
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
D3D11GeometryShader* D3D11ShaderProgramFactory::GetGeometryShader(
    const efd::FixedString& shaderName)
{
    D3D11GeometryShader* pGeometryShader = NULL;

    NiFixedString temp((const char*)shaderName);
    if (m_geometryShaderMap.GetAt(temp, pGeometryShader))
        return pGeometryShader;
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
D3D11PixelShader* D3D11ShaderProgramFactory::GetPixelShader(const efd::FixedString& shaderName)
{
    D3D11PixelShader* pPixelShader = NULL;

    NiFixedString temp((const char*)shaderName);
    if (m_pixelShaderMap.GetAt(temp, pPixelShader))
        return pPixelShader;
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
D3D11ComputeShader* D3D11ShaderProgramFactory::GetComputeShader(const efd::FixedString& shaderName)
{
    D3D11ComputeShader* pComputeShader = NULL;

    NiFixedString temp((const char*)shaderName);
    if (m_computeShaderMap.GetAt(temp, pComputeShader))
        return pComputeShader;
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::InsertVertexShaderIntoMap(D3D11VertexShader* pShader)
{
    if (!pShader)
        return;

    NiFixedString shaderName((const char*)pShader->GetName());
    m_vertexShaderMap.SetAt(shaderName, pShader);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::InsertHullShaderIntoMap(D3D11HullShader* pShader)
{
    if (!pShader)
        return;

    NiFixedString shaderName((const char*)pShader->GetName());
    m_hullShaderMap.SetAt(shaderName, pShader);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::InsertDomainShaderIntoMap(D3D11DomainShader* pShader)
{
    if (!pShader)
        return;

    NiFixedString shaderName((const char*)pShader->GetName());
    m_domainShaderMap.SetAt(shaderName, pShader);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::InsertGeometryShaderIntoMap(D3D11GeometryShader* pShader)
{
    if (!pShader)
        return;

    NiFixedString kTemp((const char*)pShader->GetName());
    m_geometryShaderMap.SetAt(kTemp, pShader);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::InsertPixelShaderIntoMap(D3D11PixelShader* pShader)
{
    if (!pShader)
        return;

    NiFixedString kTemp((const char*)pShader->GetName());
    m_pixelShaderMap.SetAt(kTemp, pShader);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::InsertComputeShaderIntoMap(D3D11ComputeShader* pShader)
{
    if (!pShader)
        return;

    NiFixedString shaderName((const char*)pShader->GetName());
    m_computeShaderMap.SetAt(shaderName, pShader);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::ResolveShaderFileName(
    const efd::FixedString& originalName,
    efd::FixedString& trueName)
{
    // See if the file exists as-is
    if (efd::File::Access(originalName, efd::File::READ_ONLY))
    {
        trueName = originalName;
        return true;
    }
    else
    {
        if (!ms_pFactory)
        {
            // We don't have a valid factory, so we can't grab the directory
            // the app set to check for the file.
            D3D11Error::ReportWarning(
                "No valid shader program factory found in "
                __FUNCTION__);
            return false;
        }

        NiShaderFactory* pFactory = NiShaderFactory::GetInstance();
        EE_ASSERT(pFactory);
        if (pFactory)
        {
            NiFixedString temp((const char*)originalName);
            NiFixedString filePath;
            if (pFactory->GetShaderProgramLocation(temp, filePath))
            {
                if (efd::File::Access(filePath, efd::File::READ_ONLY))
                    return true;
            }
        }

        // Path was not valid.
        NiFilename fileName(originalName);
        const efd::Char* pFilename = fileName.GetFilename();
        const efd::Char* pFileExt = fileName.GetExt();

        efd::Char pTrueName[efd::EE_MAX_PATH];

        NiTListIterator iter;
        const efd::Char* pProgDir = ms_pFactory->GetFirstProgramDirectory(iter);
        while (pProgDir)
        {
            efd::Bool success = true;

            if (pProgDir == NULL || pProgDir[0] == '\0')
            {
                success = false;
            }
            else if ((pProgDir[efd::Strlen(pProgDir) - 1] != '/') &&
                (pProgDir[efd::Strlen(pProgDir) - 1] != '\\'))
            {
                efd::Sprintf(pTrueName,
                    efd::EE_MAX_PATH,
                    "%s\\%s%s",
                    pProgDir,
                    pFilename,
                    pFileExt);
            }
            else
            {
                efd::Sprintf(pTrueName,
                    efd::EE_MAX_PATH,
                    "%s%s%s",
                    pProgDir,
                    pFilename,
                    pFileExt);
            }

            if (success)
            {
                if (!efd::File::Access(pTrueName, efd::File::READ_ONLY))
                {
                    // Not found!
                    success = false;
                }
            }

            if (!success)
            {
                pProgDir = ms_pFactory->GetNextProgramDirectory(iter);
            }
            else
            {
                // Found it...
                trueName = pTrueName;
                return true;
            }
        }
    }

    // It can be assumed the file was NOT found at this point!
    D3D11Error::ReportWarning(
        "Shader program file %s not found in "
        __FUNCTION__,
        originalName);
    trueName = 0;
    return false;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramFactory::RegisterShaderCreator(
    const efd::Char* pExt,
    D3D11ShaderProgramCreator* pCreator)
{
    // Make sure pExt refers to the extension only!
    if (pExt == NULL || strchr(pExt, '.') != NULL)
        return false;

    // Only store lowercase extension
    efd::Char fileExt[_MAX_EXT];
    efd::UInt32 i = 0;
    for (; i < _MAX_EXT; i++)
    {
        fileExt[i] = static_cast<efd::Char>(tolower(pExt[i]));
        if (pExt[i] == '\0')
            break;
    }
    EE_ASSERT(i < _MAX_EXT);

    EE_ASSERT(ms_pFactory);

    NiFixedString fileExtString = fileExt;
    ms_pFactory->m_shaderCreators.SetAt(fileExtString, pCreator);
    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::UnregisterShaderCreator(D3D11ShaderProgramCreator* pCreator)
{
    EE_ASSERT(ms_pFactory);

    NiTMapIterator iter = ms_pFactory->m_shaderCreators.GetFirstPos();
    while (iter)
    {
        NiFixedString fileExtString = NULL;
        D3D11ShaderProgramCreator* pMapCreator = NULL;
        ms_pFactory->m_shaderCreators.GetNext(iter, fileExtString, pMapCreator);
        if (pCreator == pMapCreator)
            ms_pFactory->m_shaderCreators.RemoveAt(fileExtString);
    }
}

//------------------------------------------------------------------------------------------------
D3D11ShaderProgramCreator* D3D11ShaderProgramFactory::GetShaderCreator(const efd::Char* pFilename)
{
    EE_ASSERT(ms_pFactory);

    if (pFilename == NULL || pFilename[0] == '\0')
        return NULL;

    NiFilename fileName(pFilename);
    efd::Char fileExt[_MAX_EXT];
    efd::Strcpy(fileExt, _MAX_EXT, fileName.GetExt());

    // Check for NULL string.
    if (fileExt[0] == '\0')
        return NULL;

    // Only look for lowercase extension
    const efd::Char* pSrc = fileExt + 1; // skip the '.'
    if (pSrc == NULL || pSrc[0] == '\0')
        pSrc = pFilename;

    efd::UInt32 i = 0;
    for (; i < _MAX_EXT; i++)
    {
        fileExt[i] = static_cast<efd::Char>(tolower(pSrc[i]));
        if (fileExt[i] == '\0')
            break;
    }
    EE_ASSERT(i < _MAX_EXT);

    D3D11ShaderProgramCreator* pCreator = NULL;
    NiFixedString fileExtString = fileExt;
    if (ms_pFactory->m_shaderCreators.GetAt(fileExtString, pCreator))
        return pCreator;
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
ID3DBlob* D3D11ShaderProgramFactory::ReadShaderFile(const efd::FixedString& fileName)
{
    // Resolve shader program file
    efd::FixedString shaderPath;
    if (!ResolveShaderFileName(fileName, shaderPath))
    {
        // Can't resolve the shader!
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            "Failed to find shader program file %s\n",
            fileName);
        return NULL;
    }

    efd::File* pIstr = efd::File::GetFile(shaderPath, efd::File::READ_ONLY);
    if ((!pIstr) || (!(*pIstr)))
    {
        EE_DELETE pIstr;
        return NULL;
    }

    efd::UInt32 bufferSize = pIstr->GetFileSize();
    if (bufferSize == 0)
    {
        EE_DELETE pIstr;
        return NULL;
    }

    ID3DBlob* pBlob = NULL;
    HRESULT hr = D3D11Renderer::D3D10CreateBlob(bufferSize, &pBlob);
    EE_ASSERT(SUCCEEDED(hr) && pBlob != NULL);
    EE_UNUSED_ARG(hr);

    // Read file into memory
    pIstr->Read(pBlob->GetBufferPointer(), (efd::UInt32)pBlob->GetBufferSize());
    EE_DELETE pIstr;

    return pBlob;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::_SDMInit()
{
    if (ms_pFactory == NULL)
        ms_pFactory = EE_NEW D3D11ShaderProgramFactory();
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramFactory::_SDMShutdown()
{
    EE_DELETE ms_pFactory;
    ms_pFactory = NULL;
}

//------------------------------------------------------------------------------------------------
