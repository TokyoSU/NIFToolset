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
#include "D3D11ShaderFactory.h"
#include "D3D11ShaderProgramCreatorHLSL.h"
#include "D3D11ShaderProgramFactory.h"

#include <NiFilename.h>

using namespace ecr;

D3D11ShaderProgramCreatorHLSL* D3D11ShaderProgramCreatorHLSL::ms_pCreator = NULL;

//------------------------------------------------------------------------------------------------
D3D11ShaderProgramCreatorHLSL::D3D11ShaderProgramCreatorHLSL()
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11ShaderProgramCreatorHLSL::~D3D11ShaderProgramCreatorHLSL()
{
    /* */
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::CreateVertexShader(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName, 
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude, 
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget, 
    efd::UInt32 flags,
    const efd::Char*, 
    efd::Bool isCompiled,
    D3D11VertexShaderPtr& spVertexShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    if (pEntryPoint == NULL)
        pEntryPoint = "main";
    if (pShaderTarget == NULL)
    {
        pShaderTarget = pRenderer->GetVertexShaderProfile();
    }

    ID3DBlob* pShaderByteCode = NULL;
    if (isCompiled)
    {
        pShaderByteCode = pShaderCode;
        pShaderByteCode->AddRef();
    }
    else
    {
        // Update macro definitions table and shader creation flags
        NiFilename fileName(pFileName);
        const efd::Char* pFileNameExtension = fileName.GetExt();
        if (pFileNameExtension == NULL)
            pFileNameExtension = "hlsl";

        pDefines = pRenderer->GetD3D11MacroList(pFileNameExtension, pDefines);
        flags = pRenderer->GetAllShaderCreationFlags(pFileNameExtension, flags);

        ID3DBlob* pError = NULL;
        HRESULT hr = D3DX11CompileFromMemory(
            (efd::Char*)pShaderCode->GetBufferPointer(),
            pShaderCode->GetBufferSize(),
            pFileName,
            pDefines,
            pInclude,
            pEntryPoint,
            pShaderTarget,
            flags,
            0,
            NULL,
            &pShaderByteCode,
            &pError,
            NULL);

        if (FAILED(hr) || pShaderByteCode == NULL)
        {
            if (pShaderByteCode)
                pShaderByteCode->Release();
            if (pError)
            {
                D3D11ShaderFactory::ReportError(
                    NISHADERERR_UNKNOWN,
                    true,
                    __FUNCTION__
                    " failed to compile vertex shader %s in file %s\nError: %s\n",
                    pEntryPoint, 
                    pFileName,
                    (const efd::Char*)pError->GetBufferPointer());
                pError->Release();
            }
            return false;
        }

        if (pError)
        {
            pError->Release();
            pError = NULL;
        }
    }

    ID3D11VertexShader* pVertexShader = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreateVertexShader(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        NULL, //ID3D11ClassLinkage *pClassLinkage
        &pVertexShader);

    if (FAILED(hr) || pVertexShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to create vertex shader %s from compiled code in file %s\n",
            pEntryPoint, 
            pFileName);
        if (pVertexShader)
            pVertexShader->Release();
        pShaderByteCode->Release();
        return false;
    }
    spVertexShader = EE_NEW D3D11VertexShader(pVertexShader, pShaderByteCode);
    EE_ASSERT(spVertexShader);
    pVertexShader->Release();

    // Store shader reflection
    ID3D11ShaderReflection* pVertexShaderReflection = NULL;
    hr = D3DReflect(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        IID_ID3D11ShaderReflection,
        (void**)&pVertexShaderReflection);
    if (SUCCEEDED(hr) && pVertexShaderReflection)
    {
        spVertexShader->SetShaderReflection(pVertexShaderReflection);

        pVertexShaderReflection->Release();
    }

    pShaderByteCode->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::CreateHullShader(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName, 
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude, 
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget, 
    efd::UInt32 flags,
    const efd::Char*, 
    efd::Bool isCompiled,
    D3D11HullShaderPtr& spHullShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    if (pEntryPoint == NULL)
        pEntryPoint = "main";
    if (pShaderTarget == NULL)
    {
        pShaderTarget = pRenderer->GetHullShaderProfile();
    }

    ID3DBlob* pShaderByteCode = NULL;
    if (isCompiled)
    {
        pShaderByteCode = pShaderCode;
        pShaderByteCode->AddRef();
    }
    else
    {
        // Update macro definitions table and shader creation flags
        NiFilename fileName(pFileName);
        const efd::Char* pFileNameExtension = fileName.GetExt();
        if (pFileNameExtension == NULL)
            pFileNameExtension = "hlsl";

        pDefines = pRenderer->GetD3D11MacroList(pFileNameExtension, pDefines);
        flags = pRenderer->GetAllShaderCreationFlags(pFileNameExtension, flags);

        ID3DBlob* pError = NULL;
        HRESULT hr = D3DX11CompileFromMemory(
            (efd::Char*)pShaderCode->GetBufferPointer(),
            pShaderCode->GetBufferSize(),
            pFileName,
            pDefines,
            pInclude,
            pEntryPoint,
            pShaderTarget,
            flags,
            0,
            NULL,
            &pShaderByteCode,
            &pError,
            NULL);

        if (FAILED(hr) || pShaderByteCode == NULL)
        {
            if (pShaderByteCode)
                pShaderByteCode->Release();
            if (pError)
            {
                D3D11ShaderFactory::ReportError(
                    NISHADERERR_UNKNOWN,
                    true,
                    __FUNCTION__
                    " failed to compile Hull shader %s in file %s\nError: %s\n",
                    pEntryPoint, 
                    pFileName,
                    (const efd::Char*)pError->GetBufferPointer());
                pError->Release();
            }
            return false;
        }

        if (pError)
        {
            pError->Release();
            pError = NULL;
        }
    }

    ID3D11HullShader* pHullShader = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreateHullShader(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        NULL, //ID3D11ClassLinkage *pClassLinkage
        &pHullShader);

    if (FAILED(hr) || pHullShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to create Hull shader %s from compiled code in file %s\n",
            pEntryPoint, 
            pFileName);
        if (pHullShader)
            pHullShader->Release();
        pShaderByteCode->Release();
        return false;
    }
    spHullShader = EE_NEW D3D11HullShader(pHullShader, pShaderByteCode);
    EE_ASSERT(spHullShader);
    pHullShader->Release();

    // Store shader reflection
    ID3D11ShaderReflection* pHullShaderReflection = NULL;
    hr = D3DReflect(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        IID_ID3D11ShaderReflection,
        (void**)&pHullShaderReflection);
    if (SUCCEEDED(hr) && pHullShaderReflection)
    {
        spHullShader->SetShaderReflection(pHullShaderReflection);

        pHullShaderReflection->Release();
    }

    pShaderByteCode->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::CreateDomainShader(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName, 
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude, 
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget, 
    efd::UInt32 flags,
    const efd::Char*, 
    efd::Bool isCompiled,
    D3D11DomainShaderPtr& spDomainShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    if (pEntryPoint == NULL)
        pEntryPoint = "main";
    if (pShaderTarget == NULL)
    {
        pShaderTarget = pRenderer->GetDomainShaderProfile();
    }

    ID3DBlob* pShaderByteCode = NULL;
    if (isCompiled)
    {
        pShaderByteCode = pShaderCode;
        pShaderByteCode->AddRef();
    }
    else
    {
        // Update macro definitions table and shader creation flags
        NiFilename fileName(pFileName);
        const efd::Char* pFileNameExtension = fileName.GetExt();
        if (pFileNameExtension == NULL)
            pFileNameExtension = "hlsl";

        pDefines = pRenderer->GetD3D11MacroList(pFileNameExtension, pDefines);
        flags = pRenderer->GetAllShaderCreationFlags(pFileNameExtension, flags);

        ID3DBlob* pError = NULL;
        HRESULT hr = D3DX11CompileFromMemory(
            (efd::Char*)pShaderCode->GetBufferPointer(),
            pShaderCode->GetBufferSize(),
            pFileName,
            pDefines,
            pInclude,
            pEntryPoint,
            pShaderTarget,
            flags,
            0,
            NULL,
            &pShaderByteCode,
            &pError,
            NULL);

        if (FAILED(hr) || pShaderByteCode == NULL)
        {
            if (pShaderByteCode)
                pShaderByteCode->Release();
            if (pError)
            {
                D3D11ShaderFactory::ReportError(
                    NISHADERERR_UNKNOWN,
                    true,
                    __FUNCTION__
                    " failed to compile Domain shader %s in file %s\nError: %s\n",
                    pEntryPoint, 
                    pFileName,
                    (const efd::Char*)pError->GetBufferPointer());
                pError->Release();
            }
            return false;
        }

        if (pError)
        {
            pError->Release();
            pError = NULL;
        }
    }

    ID3D11DomainShader* pDomainShader = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreateDomainShader(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        NULL, //ID3D11ClassLinkage *pClassLinkage
        &pDomainShader);

    if (FAILED(hr) || pDomainShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to create Domain shader %s from compiled code in file %s\n",
            pEntryPoint, 
            pFileName);
        if (pDomainShader)
            pDomainShader->Release();
        pShaderByteCode->Release();
        return false;
    }
    spDomainShader = EE_NEW D3D11DomainShader(pDomainShader, pShaderByteCode);
    EE_ASSERT(spDomainShader);
    pDomainShader->Release();

    // Store shader reflection
    ID3D11ShaderReflection* pDomainShaderReflection = NULL;
    hr = D3DReflect(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        IID_ID3D11ShaderReflection,
        (void**)&pDomainShaderReflection);
    if (SUCCEEDED(hr) && pDomainShaderReflection)
    {
        spDomainShader->SetShaderReflection(pDomainShaderReflection);

        pDomainShaderReflection->Release();
    }

    pShaderByteCode->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::CreateGeometryShader(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName, 
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude, 
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget, 
    efd::UInt32 flags,
    const efd::Char*,
    const D3D11_SO_DECLARATION_ENTRY* pSODeclaration,
    efd::UInt32 numEntries, 
    efd::UInt32* outputStreamStrideArray,
    efd::UInt32 numOutputStreams,
    efd::UInt32 rasterizedStream,
    efd::Bool isCompiled,
    D3D11GeometryShaderPtr& spGeometryShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    if (pEntryPoint == NULL)
        pEntryPoint = "main";
    if (pShaderTarget == NULL)
    {
        pShaderTarget = pRenderer->GetGeometryShaderProfile();
    }

    ID3DBlob* pShaderByteCode = NULL;
    if (isCompiled)
    {
        pShaderByteCode = pShaderCode;
        pShaderByteCode->AddRef();
    }
    else
    {
        // Update macro definitions table and shader creation flags
        NiFilename fileName(pFileName);
        const efd::Char* pFileNameExtension = fileName.GetExt();
        if (pFileNameExtension == NULL)
            pFileNameExtension = "hlsl";

        pDefines = pRenderer->GetD3D11MacroList(pFileNameExtension, pDefines);
        flags = pRenderer->GetAllShaderCreationFlags(pFileNameExtension, flags);

        ID3DBlob* pError = NULL;
        HRESULT hr = D3DX11CompileFromMemory(
            (efd::Char*)pShaderCode->GetBufferPointer(),
            pShaderCode->GetBufferSize(),
            pFileName,
            pDefines,
            pInclude,
            pEntryPoint,
            pShaderTarget,
            flags,
            0,
            NULL,
            &pShaderByteCode,
            &pError,
            NULL);

        if (FAILED(hr) || pShaderByteCode == NULL)
        {
            if (pShaderByteCode)
                pShaderByteCode->Release();
            if (pError)
            {
                D3D11ShaderFactory::ReportError(
                    NISHADERERR_UNKNOWN,
                    true,
                    __FUNCTION__
                    " failed to compile geometry shader %s in file %s\nError: %s\n",
                    pEntryPoint, pFileName,
                    (const efd::Char*)pError->GetBufferPointer());
                pError->Release();
            }
            return false;
        }
        if (pError)
        {
            pError->Release();
            pError = NULL;
        }
    }

    HRESULT hr;
    ID3D11GeometryShader* pGeometryShader = NULL;
    if (pSODeclaration && numEntries != 0)
    {
        hr = pRenderer->GetD3D11Device()->CreateGeometryShaderWithStreamOutput(
            pShaderByteCode->GetBufferPointer(),
            pShaderByteCode->GetBufferSize(), 
            pSODeclaration,
            numEntries, 
            outputStreamStrideArray,
            numOutputStreams,
            rasterizedStream,
            NULL, //ID3D11ClassLinkage *pClassLinkage
            &pGeometryShader);
    }
    else
    {
        hr = pRenderer->GetD3D11Device()->CreateGeometryShader(
            pShaderByteCode->GetBufferPointer(),
            pShaderByteCode->GetBufferSize(), 
            NULL, //ID3D11ClassLinkage *pClassLinkage
            &pGeometryShader);

        // Be sure these values are all 0.
        pSODeclaration = NULL;
        numEntries = 0;
        outputStreamStrideArray = NULL;
        numOutputStreams = 0;
        rasterizedStream = 0;
    }
    if (FAILED(hr) || pGeometryShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to create geometry shader %s from compiled code in file %s\n",
            pEntryPoint, 
            pFileName);
        if (pGeometryShader)
            pGeometryShader->Release();
        pShaderByteCode->Release();
        return false;
    }
    spGeometryShader = EE_NEW D3D11GeometryShader(pGeometryShader, pShaderByteCode);
    EE_ASSERT(spGeometryShader);
    pGeometryShader->Release();

    // Fill in stream output arrays
    spGeometryShader->SetStreamOutputDeclaration(
        pSODeclaration, 
        numEntries,
        outputStreamStrideArray,
        numOutputStreams,
        rasterizedStream);

    // Store shader reflection
    ID3D11ShaderReflection* pGeometryShaderReflection = NULL;
    hr = D3DReflect(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        IID_ID3D11ShaderReflection,
        (void**)&pGeometryShaderReflection);
    if (SUCCEEDED(hr) && pGeometryShaderReflection)
    {
        spGeometryShader->SetShaderReflection(pGeometryShaderReflection);

        pGeometryShaderReflection->Release();
    }

    pShaderByteCode->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::CreatePixelShader(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName, 
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude, 
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget, 
    efd::UInt32 flags,
    const efd::Char*, 
    efd::Bool isCompiled,
    D3D11PixelShaderPtr& spPixelShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    if (pEntryPoint == NULL)
        pEntryPoint = "main";
    if (pShaderTarget == NULL)
    {
        pShaderTarget = pRenderer->GetPixelShaderProfile();
    }

    ID3DBlob* pShaderByteCode = NULL;
    if (isCompiled)
    {
        pShaderByteCode = pShaderCode;
        pShaderByteCode->AddRef();
    }
    else
    {
        // Update macro definitions table and shader creation flags
        NiFilename fileName(pFileName);
        const efd::Char* pFileNameExtension = fileName.GetExt();
        if (pFileNameExtension == NULL)
            pFileNameExtension = "hlsl";

        pDefines = pRenderer->GetD3D11MacroList(pFileNameExtension, pDefines);
        flags = pRenderer->GetAllShaderCreationFlags(pFileNameExtension, flags);

        ID3DBlob* pError = NULL;
        HRESULT hr = D3DX11CompileFromMemory(
            (efd::Char*)pShaderCode->GetBufferPointer(),
            pShaderCode->GetBufferSize(),
            pFileName,
            pDefines,
            pInclude,
            pEntryPoint,
            pShaderTarget,
            flags,
            0,
            NULL,
            &pShaderByteCode,
            &pError,
            NULL);

        if (FAILED(hr) || pShaderByteCode == NULL)
        {
            if (pShaderByteCode)
                pShaderByteCode->Release();
            if (pError)
            {
                D3D11ShaderFactory::ReportError(
                    NISHADERERR_UNKNOWN,
                    true,
                    __FUNCTION__
                    " failed to compile pixel shader %s in file %s\nError: %s\n",
                    pEntryPoint, pFileName,
                    (const efd::Char*)pError->GetBufferPointer());
                pError->Release();
            }
            return false;
        }
        if (pError)
        {
            pError->Release();
            pError = NULL;
        }
    }

    ID3D11PixelShader* pPixelShader = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreatePixelShader(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        NULL, //ID3D11ClassLinkage *pClassLinkage
        &pPixelShader);
    if (FAILED(hr) || pPixelShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to create pixel shader %s from compiled code in file %s\n",
            pEntryPoint, 
            pFileName);
        if (pPixelShader)
            pPixelShader->Release();
        pShaderByteCode->Release();
        return false;
    }
    spPixelShader = EE_NEW D3D11PixelShader(pPixelShader, pShaderByteCode);
    EE_ASSERT(spPixelShader);
    pPixelShader->Release();

    // Store shader reflection
    ID3D11ShaderReflection* pPixelShaderReflection = NULL;
    hr = D3DReflect(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        IID_ID3D11ShaderReflection,
        (void**)&pPixelShaderReflection);
    if (SUCCEEDED(hr) && pPixelShaderReflection)
    {
        spPixelShader->SetShaderReflection(pPixelShaderReflection);

        pPixelShaderReflection->Release();
    }

    pShaderByteCode->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::CreateComputeShader(
    ID3DBlob* pShaderCode,
    const efd::Char* pFileName, 
    const D3D_SHADER_MACRO* pDefines,
    ID3DInclude* pInclude, 
    const efd::Char* pEntryPoint,
    const efd::Char* pShaderTarget, 
    efd::UInt32 flags,
    const efd::Char*, 
    efd::Bool isCompiled,
    D3D11ComputeShaderPtr& spComputeShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    if (pEntryPoint == NULL)
        pEntryPoint = "main";
    if (pShaderTarget == NULL)
    {
        pShaderTarget = pRenderer->GetComputeShaderProfile();
    }

    ID3DBlob* pShaderByteCode = NULL;
    if (isCompiled)
    {
        pShaderByteCode = pShaderCode;
        pShaderByteCode->AddRef();
    }
    else
    {
        // Update macro definitions table and shader creation flags
        NiFilename fileName(pFileName);
        const efd::Char* pFileNameExtension = fileName.GetExt();
        if (pFileNameExtension == NULL)
            pFileNameExtension = "hlsl";

        pDefines = pRenderer->GetD3D11MacroList(pFileNameExtension, pDefines);
        flags = pRenderer->GetAllShaderCreationFlags(pFileNameExtension, flags);

        ID3DBlob* pError = NULL;
        HRESULT hr = D3DX11CompileFromMemory(
            (efd::Char*)pShaderCode->GetBufferPointer(),
            pShaderCode->GetBufferSize(),
            pFileName,
            pDefines,
            pInclude,
            pEntryPoint,
            pShaderTarget,
            flags,
            0,
            NULL,
            &pShaderByteCode,
            &pError,
            NULL);

        if (FAILED(hr) || pShaderByteCode == NULL)
        {
            if (pShaderByteCode)
                pShaderByteCode->Release();
            if (pError)
            {
                D3D11ShaderFactory::ReportError(
                    NISHADERERR_UNKNOWN,
                    true,
                    __FUNCTION__
                    " failed to compile Compute shader %s in file %s\nError: %s\n",
                    pEntryPoint, 
                    pFileName,
                    (const efd::Char*)pError->GetBufferPointer());
                pError->Release();
            }
            return false;
        }

        if (pError)
        {
            pError->Release();
            pError = NULL;
        }
    }

    ID3D11ComputeShader* pComputeShader = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreateComputeShader(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        NULL, //ID3D11ClassLinkage *pClassLinkage
        &pComputeShader);

    if (FAILED(hr) || pComputeShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to create Compute shader %s from compiled code in file %s\n",
            pEntryPoint, 
            pFileName);
        if (pComputeShader)
            pComputeShader->Release();
        pShaderByteCode->Release();
        return false;
    }
    spComputeShader = EE_NEW D3D11ComputeShader(pComputeShader, pShaderByteCode);
    EE_ASSERT(spComputeShader);
    pComputeShader->Release();

    // Store shader reflection
    ID3D11ShaderReflection* pComputeShaderReflection = NULL;
    hr = D3DReflect(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        IID_ID3D11ShaderReflection,
        (void**)&pComputeShaderReflection);
    if (SUCCEEDED(hr) && pComputeShaderReflection)
    {
        spComputeShader->SetShaderReflection(pComputeShaderReflection);

        pComputeShaderReflection->Release();
    }

    pShaderByteCode->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::RecreateVertexShader(D3D11VertexShader* pVertexShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    ID3DBlob* pShaderByteCode = pVertexShader->GetShaderByteCode();
    if (pShaderByteCode == NULL)
        return false;

    ID3D11VertexShader* pD3DVertexShader = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreateVertexShader(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        NULL, //ID3D11ClassLinkage *pClassLinkage
        &pD3DVertexShader);

    if (FAILED(hr) || pD3DVertexShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to recreate vertex shader %s",
            pVertexShader->GetName());
        if (pD3DVertexShader)
            pD3DVertexShader->Release();
        return false;
    }

    pVertexShader->SetVertexShader(pD3DVertexShader);

    pD3DVertexShader->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::RecreateHullShader(D3D11HullShader* pHullShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    ID3DBlob* pShaderByteCode = pHullShader->GetShaderByteCode();
    if (pShaderByteCode == NULL)
        return false;

    ID3D11HullShader* pD3DHullShader = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreateHullShader(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        NULL, //ID3D11ClassLinkage *pClassLinkage
        &pD3DHullShader);

    if (FAILED(hr) || pD3DHullShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to recreate Hull shader %s",
            pHullShader->GetName());
        if (pD3DHullShader)
            pD3DHullShader->Release();
        return false;
    }

    pHullShader->SetHullShader(pD3DHullShader);

    pD3DHullShader->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::RecreateDomainShader(D3D11DomainShader* pDomainShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    ID3DBlob* pShaderByteCode = pDomainShader->GetShaderByteCode();
    if (pShaderByteCode == NULL)
        return false;

    ID3D11DomainShader* pD3DDomainShader = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreateDomainShader(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        NULL, //ID3D11ClassLinkage *pClassLinkage
        &pD3DDomainShader);

    if (FAILED(hr) || pD3DDomainShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to recreate Domain shader %s",
            pDomainShader->GetName());
        if (pD3DDomainShader)
            pD3DDomainShader->Release();
        return false;
    }

    pDomainShader->SetDomainShader(pD3DDomainShader);

    pD3DDomainShader->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::RecreateGeometryShader(
    D3D11GeometryShader* pGeometryShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    ID3DBlob* pShaderByteCode = pGeometryShader->GetShaderByteCode();
    if (pShaderByteCode == NULL)
        return false;

    ID3D11GeometryShader* pD3DGeometryShader = NULL;
    HRESULT hr = S_OK;
    const D3D11_SO_DECLARATION_ENTRY* pSODeclaration = 
        pGeometryShader->GetStreamOutputDeclaration();
    efd::UInt32 numEntries = pGeometryShader->GetNumStreamOutputEntries();
    const efd::UInt32* outputStreamStrideArray = pGeometryShader->GetOutputStreamStrideArray();
    efd::UInt32 numOutputStreams = pGeometryShader->GetNumStreamOutputStreams();
    efd::UInt32 rasterizedStream = pGeometryShader->GetRasterizedStreamOutputStream();

    if (pSODeclaration && numEntries != 0 && outputStreamStrideArray && numOutputStreams != 0)
    {
        hr = pRenderer->GetD3D11Device()->CreateGeometryShaderWithStreamOutput(
            pShaderByteCode->GetBufferPointer(),
            pShaderByteCode->GetBufferSize(), 
            pSODeclaration,
            numEntries, 
            outputStreamStrideArray,
            numOutputStreams,
            rasterizedStream,
            NULL, //ID3D11ClassLinkage *pClassLinkage
            &pD3DGeometryShader);
    }
    else
    {
        hr = pRenderer->GetD3D11Device()->CreateGeometryShader(
            pShaderByteCode->GetBufferPointer(),
            pShaderByteCode->GetBufferSize(), 
            NULL, //ID3D11ClassLinkage *pClassLinkage
            &pD3DGeometryShader);
    }
    if (FAILED(hr) || pD3DGeometryShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to recreate geometry shader %s",
            pGeometryShader->GetName());
        if (pD3DGeometryShader)
            pD3DGeometryShader->Release();
        return false;
    }

    pGeometryShader->SetGeometryShader(pD3DGeometryShader);

    pD3DGeometryShader->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::RecreatePixelShader(D3D11PixelShader* pPixelShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    ID3DBlob* pShaderByteCode = pPixelShader->GetShaderByteCode();
    if (pShaderByteCode == NULL)
        return false;

    ID3D11PixelShader* pD3DPixelShader = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreatePixelShader(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        NULL, //ID3D11ClassLinkage *pClassLinkage
        &pD3DPixelShader);
    if (FAILED(hr) || pD3DPixelShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to recreate pixel shader %s",
            pPixelShader->GetName());
        if (pD3DPixelShader)
            pD3DPixelShader->Release();
        return false;
    }

    pPixelShader->SetPixelShader(pD3DPixelShader);

    pD3DPixelShader->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderProgramCreatorHLSL::RecreateComputeShader(D3D11ComputeShader* pComputeShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL || pRenderer->GetD3D11Device() == NULL)
    {
        return false;
    }

    ID3DBlob* pShaderByteCode = pComputeShader->GetShaderByteCode();
    if (pShaderByteCode == NULL)
        return false;

    ID3D11ComputeShader* pD3DComputeShader = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreateComputeShader(
        pShaderByteCode->GetBufferPointer(),
        pShaderByteCode->GetBufferSize(), 
        NULL, //ID3D11ClassLinkage *pClassLinkage
        &pD3DComputeShader);

    if (FAILED(hr) || pD3DComputeShader == NULL)
    {
        D3D11ShaderFactory::ReportError(
            NISHADERERR_UNKNOWN,
            true,
            __FUNCTION__
            " failed to recreate Compute shader %s",
            pComputeShader->GetName());
        if (pD3DComputeShader)
            pD3DComputeShader->Release();
        return false;
    }

    pComputeShader->SetComputeShader(pD3DComputeShader);

    pD3DComputeShader->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramCreatorHLSL::_SDMInit()
{
    ms_pCreator = EE_NEW D3D11ShaderProgramCreatorHLSL();

    D3D11ShaderProgramFactory::RegisterShaderCreator("hlsl", ms_pCreator);
    D3D11ShaderProgramFactory::RegisterShaderCreator("vsh", ms_pCreator);
    D3D11ShaderProgramFactory::RegisterShaderCreator("hsh", ms_pCreator);
    D3D11ShaderProgramFactory::RegisterShaderCreator("dsh", ms_pCreator);
    D3D11ShaderProgramFactory::RegisterShaderCreator("gsh", ms_pCreator);
    D3D11ShaderProgramFactory::RegisterShaderCreator("psh", ms_pCreator);
    D3D11ShaderProgramFactory::RegisterShaderCreator("csh", ms_pCreator);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramCreatorHLSL::_SDMShutdown()
{
    Shutdown();
}

//------------------------------------------------------------------------------------------------
D3D11ShaderProgramCreatorHLSL* D3D11ShaderProgramCreatorHLSL::GetInstance()
{
    return ms_pCreator;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramCreatorHLSL::Shutdown()
{
    if (ms_pCreator)
        D3D11ShaderProgramFactory::UnregisterShaderCreator(ms_pCreator);
    EE_DELETE ms_pCreator;
    ms_pCreator = NULL;
}

//------------------------------------------------------------------------------------------------
