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

#include "D3D11GPUProgramCache.h"
#include "D3D11Error.h"
#include "D3D11ComputeShader.h"
#include "D3D11DomainShader.h"
#include "D3D11GeometryShader.h"
#include "D3D11HullShader.h"
#include "D3D11PixelShader.h"
#include "D3D11Renderer.h"
#include "D3D11ShaderProgramFactory.h"
#include "D3D11VertexShader.h"

#include <efd/File.h>

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11GPUProgramCache::D3D11GPUProgramCache(
    efd::UInt32 version,
    const efd::Char* pWorkingDir, 
    NiGPUProgram::ProgramType shaderType,
    const efd::FixedString& shaderProfile, 
    const efd::Char* pMaterialIdentifier,
    efd::Bool autoWriteCacheToDisk, 
    efd::Bool writeDebugHLSLFile,
    efd::Bool isLocked, 
    efd::Bool loadShaders) :
    NiGPUProgramCache(version, autoWriteCacheToDisk, isLocked),
    m_writeDebugHLSLFile(writeDebugHLSLFile),
    m_shaderType(shaderType),
    m_materialIdentifier(pMaterialIdentifier)
{
    efd::Char fileName[efd::EE_MAX_PATH];
    efd::Sprintf(
        fileName, 
        efd::EE_MAX_PATH,
#if defined(EE_CONFIG_DEBUG)
        "%s_%s_%s_%s_D.cache",
#else
        "%s_%s_%s_%s.cache",
#endif
        pMaterialIdentifier, 
        (const efd::Char*)shaderProfile,
        (const efd::Char*)GetPlatform(),
        GetArchitectureSuffix());

    m_kFilename = fileName;
    SetWorkingDirectory(pWorkingDir);

    m_kShaderProfile = shaderProfile;

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    D3D11Renderer::FeatureLevel featureLevel = pRenderer->GetFeatureLevel();

    switch (shaderType)
    {
    case NiGPUProgram::PROGRAM_VERTEX:
        if (featureLevel >= D3D_FEATURE_LEVEL_11_0)
        {
            m_kValidTargets.Add("vs_5_0");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_10_1)
        {
            m_kValidTargets.Add("vs_4_1");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_10_0)
        {
            m_kValidTargets.Add("vs_4_0");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_9_3)
        {
            m_kValidTargets.Add("vs_4_0_level_9_3");
            m_kValidTargets.Add("vs_3_0");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_9_1)
        {
            m_kValidTargets.Add("vs_4_0_level_9_1");
            m_kValidTargets.Add("vs_2_a");
            m_kValidTargets.Add("vs_2_0");
            m_kValidTargets.Add("vs_1_1");
        }
        break;
    case NiGPUProgram::PROGRAM_HULL:
        if (featureLevel >= D3D_FEATURE_LEVEL_11_0)
        {
            m_kValidTargets.Add("hs_5_0");
        }
        break;
    case NiGPUProgram::PROGRAM_DOMAIN:
        if (featureLevel >= D3D_FEATURE_LEVEL_11_0)
        {
            m_kValidTargets.Add("hs_5_0");
        }
        break;
    case NiGPUProgram::PROGRAM_GEOMETRY:
        if (featureLevel >= D3D_FEATURE_LEVEL_11_0)
        {
            m_kValidTargets.Add("gs_5_0");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_10_1)
        {
            m_kValidTargets.Add("gs_4_1");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_10_0)
        {
            m_kValidTargets.Add("gs_4_0");
        }
        break;
    case NiGPUProgram::PROGRAM_PIXEL:
        if (featureLevel >= D3D_FEATURE_LEVEL_11_0)
        {
            m_kValidTargets.Add("ps_5_0");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_10_1)
        {
            m_kValidTargets.Add("ps_4_1");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_10_0)
        {
            m_kValidTargets.Add("ps_4_0");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_9_3)
        {
            m_kValidTargets.Add("ps_4_0_level_9_3");
            m_kValidTargets.Add("ps_3_0");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_9_2)
        {
            m_kValidTargets.Add("ps_2_b");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_9_1)
        {
            m_kValidTargets.Add("ps_4_0_level_9_1");
            m_kValidTargets.Add("ps_2_a");
            m_kValidTargets.Add("ps_2_0");
        }
        break;
    case NiGPUProgram::PROGRAM_COMPUTE:
        if (featureLevel >= D3D_FEATURE_LEVEL_11_0)
        {
            m_kValidTargets.Add("cs_5_0");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_10_1 && 
            D3D11_SHVER_GET_MAJOR(pRenderer->GetMaxComputeShader()) > 0)
        {
            m_kValidTargets.Add("cs_4_1");
        }
        if (featureLevel >= D3D_FEATURE_LEVEL_10_0 && 
            D3D11_SHVER_GET_MAJOR(pRenderer->GetMaxComputeShader()) > 0)
        {
            m_kValidTargets.Add("cs_4_0");
        }
        break;
    }

    if (loadShaders && pWorkingDir)
    {
        Load();
    }
}

//------------------------------------------------------------------------------------------------
const efd::Char* D3D11GPUProgramCache::GetPlatformSpecificCodeID() const
{
    return "hlsl";
}

//------------------------------------------------------------------------------------------------
NiGPUProgram* D3D11GPUProgramCache::GenerateProgram(
    const efd::Char* pName,
    const efd::Char* pProgramText,
    NiTObjectPtrSet<NiMaterialResourcePtr>& uniformResources)
{
#ifdef EE_ASSERTS_ARE_ENABLED
    efd::Bool didNotCompilePreviously = false;
    EE_ASSERT(!FindCachedProgram(pName, uniformResources, didNotCompilePreviously));
    EE_ASSERT(didNotCompilePreviously == false);
#endif
    if (m_bLocked)
        return NULL;

    if (pProgramText == NULL)
        return NULL;

    efd::UInt32 programLength = efd::Strlen(pProgramText);
    if (programLength == 0)
        return NULL;

    ID3DBlob* pShaderCode = NULL;
    HRESULT hr = D3D11Renderer::D3D10CreateBlob(programLength + 1, &pShaderCode);
    EE_UNUSED_ARG(hr);
    EE_ASSERT(SUCCEEDED(hr) && pShaderCode);

    efd::Strcpy(
        (efd::Char*)pShaderCode->GetBufferPointer(),
        pShaderCode->GetBufferSize(), 
        pProgramText);

    efd::UInt32 flags = 0;

    // Set it to DX9-compatibility mode
    flags |= D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY;

    // Debug flags
#if defined(EE_CONFIG_DEBUG)
    flags |=  D3D10_SHADER_DEBUG;
    // Don't set D3D10_SHADER_SKIP_OPTIMIZATION because it can prevent down-level shaders from 
    //   compiling due to length
    efd::Bool downLevel = m_kShaderProfile.Contains("_level_");
    if (!downLevel)
        flags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

    efd::Char fileName[efd::EE_MAX_PATH];
    NiGPUProgramPtr spProgram;
    if (m_shaderType == NiGPUProgram::PROGRAM_VERTEX)
    {
        efd::UInt32 entryCount = m_kCachedPrograms.GetCount() + m_kFailedPrograms.GetCount();

        efd::Sprintf(
            fileName, 
            efd::EE_MAX_PATH, 
            "%s\\%s\\Shader%04d-V.%s",
            GetWorkingDirectory(), 
            m_materialIdentifier, 
            entryCount,
            GetPlatformSpecificCodeID());

        D3D11VertexShaderPtr spVSProgram;
        D3D11ShaderProgramFactory::CreateVertexShaderFromBlob(
            pShaderCode, 
            fileName, 
            NULL, 
            NULL, 
            "Main",
            (const efd::Char*)m_kShaderProfile, 
            flags, 
            fileName, 
            false,
            spVSProgram);
        spProgram = spVSProgram;
    }
    else if (m_shaderType == NiGPUProgram::PROGRAM_PIXEL)
    {
        efd::UInt32 entryCount = m_kCachedPrograms.GetCount() + m_kFailedPrograms.GetCount();

        efd::Sprintf(
            fileName, 
            efd::EE_MAX_PATH, 
            "%s\\%s\\Shader%04d-P.%s",
            GetWorkingDirectory(), 
            m_materialIdentifier, 
            entryCount,
            GetPlatformSpecificCodeID());

        D3D11PixelShaderPtr spPSProgram;
        D3D11ShaderProgramFactory::CreatePixelShaderFromBlob(
            pShaderCode, 
            fileName, 
            NULL, 
            NULL, 
            "Main",
            (const efd::Char*)m_kShaderProfile, 
            flags, 
            fileName, 
            false,
            spPSProgram);
        spProgram = spPSProgram;
    }
    if (m_writeDebugHLSLFile)
    {
        SaveDebugHLSLFile(fileName, (const efd::Char*)pShaderCode->GetBufferPointer());
    }

    pShaderCode->Release();

    // Don't prune unused constants here (as many of the other renderers do)
    // - if the constants are in the shader, then they must be included
    // in the shader constant buffer.
    if (spProgram)
    {
        NiGPUProgramCache::NiGPUProgramDesc* pDesc = EE_NEW NiGPUProgramCache::NiGPUProgramDesc();

        for (efd::UInt32 i = 0; i < uniformResources.GetSize(); i++)
            pDesc->m_kUniforms.Add(uniformResources.GetAt(i));

        pDesc->m_spProgram = spProgram;

        NiFixedString shaderName = pName;

        if (m_bAutoWriteCacheToDisk)
        {
            if (!AppendEntry(pDesc, shaderName))
            {
                D3D11Error::ReportWarning(
                    "Failed to write cache entry, \"%s\", to cache file: \"%s\"\n",
                    (const efd::Char*) shaderName,
                    (const efd::Char*) m_kPathAndFilename);
            }
        }

        m_kCachedPrograms.SetAt(shaderName, pDesc);
    }

    if (spProgram != NULL)
        return spProgram;

    uniformResources.RemoveAll();
    return NULL;
}

//------------------------------------------------------------------------------------------------
const efd::Char* D3D11GPUProgramCache::GetPlatform() const
{
    return "D3D11";
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11GPUProgramCache::SaveGPUProgram(
    efd::BinaryStream& stream,
    NiGPUProgram* pProgram)
{
    D3D11ShaderProgram* pShader = (D3D11ShaderProgram*)pProgram;

    if (!pShader)
        return false;

    const NiRTTI* pRTTI = pShader->GetRTTI();
    SaveString(stream, pRTTI->GetName());

    ID3DBlob* pShaderByteCode = pShader->GetShaderByteCode();

    SIZE_T stCodeSize = pShaderByteCode->GetBufferSize();
    efd::StreamSaveBinary(stream, stCodeSize);
    EE_ASSERT(stCodeSize > 0);
    efd::StreamSaveBinary(stream,
        (const efd::Char*)pShaderByteCode->GetBufferPointer(),
        (efd::UInt32)stCodeSize);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11GPUProgramCache::LoadGPUProgram(
    efd::BinaryStream& stream, 
    const NiFixedString& shaderName, 
    NiGPUProgramPtr& spProgram, 
    efd::Bool skip)
{
    spProgram = NULL;
    efd::UInt32 codeSize = 0;

    efd::Char name[efd::EE_MAX_PATH];
    LoadString(stream, name, efd::EE_MAX_PATH);

    NiGPUProgram::ProgramType shaderType;
    if (efd::Stricmp(name, D3D11VertexShader::ms_RTTI.GetName()) == 0)
    {
        shaderType = NiGPUProgram::PROGRAM_VERTEX;
    }
    else if (efd::Stricmp(name, D3D11GeometryShader::ms_RTTI.GetName()) == 0)
    {
        shaderType = NiGPUProgram::PROGRAM_GEOMETRY;
    }
    else if (efd::Stricmp(name, D3D11PixelShader::ms_RTTI.GetName()) == 0)
    {
        shaderType = NiGPUProgram::PROGRAM_PIXEL;
    }
    else
    {
        return false;
    }

    efd::StreamLoadBinary(stream, codeSize);
    EE_ASSERT(codeSize > 0);

    if (skip)
    {
        stream.Seek(codeSize);
        return true;
    }

    ID3DBlob* pShaderByteCode = NULL;
    HRESULT hr = D3D11Renderer::D3D10CreateBlob(codeSize, &pShaderByteCode);
    if (FAILED(hr) || pShaderByteCode == NULL)
    {
        if (pShaderByteCode)
            pShaderByteCode->Release();
        return false;
    }

    efd::StreamLoadBinary(stream, (efd::Char*)pShaderByteCode->GetBufferPointer(), codeSize);

    efd::Char fileName[efd::EE_MAX_PATH];
    efd::Sprintf(fileName, efd::EE_MAX_PATH, "%s.%s", shaderName, GetPlatformSpecificCodeID());

    if (shaderType == NiGPUProgram::PROGRAM_VERTEX)
    {
        D3D11VertexShaderPtr spVSProgram;
        D3D11ShaderProgramFactory::CreateVertexShaderFromBlob(
            pShaderByteCode, 
            fileName, 
            NULL, 
            NULL, 
            "Main",
            GetShaderProfile(), 
            0, 
            shaderName, 
            true, 
            spVSProgram);
        spProgram = spVSProgram;
    }
    else if (shaderType == NiGPUProgram::PROGRAM_GEOMETRY)
    {
        D3D11GeometryShaderPtr spGSProgram;
        D3D11ShaderProgramFactory::CreateGeometryShaderFromBlob(
            pShaderByteCode, 
            fileName, 
            NULL, 
            NULL, 
            "Main",
            GetShaderProfile(), 
            0, 
            shaderName, 
            true, 
            spGSProgram);
        spProgram = spGSProgram;
    }
    else
    {
        EE_ASSERT(shaderType == NiGPUProgram::PROGRAM_PIXEL);
        D3D11PixelShaderPtr spPSProgram;
        D3D11ShaderProgramFactory::CreatePixelShaderFromBlob(
            pShaderByteCode, 
            fileName, 
            NULL, 
            NULL, 
            "Main",
            GetShaderProfile(), 
            0, 
            shaderName, 
            true, 
            spPSProgram);
        spProgram = spPSProgram;
    }

    pShaderByteCode->Release();

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11GPUProgramCache::SaveDebugHLSLFile(
    const efd::Char* pFilename,
    const efd::Char* pProgramText)
{
    // Validate directory will validate the working directory
    if (ValidateDirectory())
    {
        // Now the material identifier directory needs to be validated.
        efd::Char fullPath[efd::EE_MAX_PATH];
        efd::Strcpy(fullPath, efd::EE_MAX_PATH, pFilename);
        efd::PathUtils::Standardize(fullPath);
        efd::Char* pPathEnd = strrchr(fullPath, '/');
        if (pPathEnd)
        {
            pPathEnd[0] = '\0';
        }
        else
        {
            pPathEnd = strrchr(fullPath, '\\');
            if (pPathEnd)
                pPathEnd[0] = '\0';
        }

        if (!efd::File::DirectoryExists(fullPath) &&
            !efd::File::CreateDirectoryRecursive(fullPath))
        {
            return false;
        }

        // Write out the HLSL file
        EE_OUTPUT_DEBUG_STRING(pFilename);
        EE_OUTPUT_DEBUG_STRING("(0)\n");
        efd::File* pFile = efd::File::GetFile(pFilename, efd::File::WRITE_ONLY);
        if (pFile)
        {
            pFile->Write(pProgramText, efd::Strlen(pProgramText));
            EE_DELETE pFile;
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------------------------
