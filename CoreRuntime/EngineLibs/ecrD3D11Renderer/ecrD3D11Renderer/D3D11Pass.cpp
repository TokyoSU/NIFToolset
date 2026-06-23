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

#include "D3D11Pass.h"

#include "D3D11DeviceState.h"
#include "D3D11Error.h"
#include "D3D11Headers.h"
#include "D3D11Renderer.h"
#include "D3D11RenderStateGroup.h"
#include "D3D11RenderStateManager.h"
#include "D3D11ShaderCore.h"
#include "D3D11ShaderConstantMap.h"
#include "D3D11ShaderConstantManager.h"
#include "D3D11TextureData.h"

#include <NiShaderError.h>

using namespace ecr;

//------------------------------------------------------------------------------------------------
ConstantBufferDesc::ConstantBufferDesc(efd::UInt32 variableCount) :
    m_variableCount(variableCount),
    m_shaderTypes(0),
    m_constantMapIndex(EE_UINT32_MAX),
    m_pNext(NULL)
{
    if (variableCount)
    {
        m_nameArray = EE_NEW efd::FixedString[m_variableCount];
        m_startOffsetArray = EE_ALLOC(efd::UInt32, m_variableCount);
        m_sizeArray = EE_ALLOC(efd::UInt32, m_variableCount);
        m_typeArray = EE_ALLOC(D3D11_SHADER_TYPE_DESC, m_variableCount);
    }
    else
    {
        m_nameArray = NULL;
        m_startOffsetArray = NULL;
        m_sizeArray = NULL;
        m_typeArray = NULL;
    }
}

//------------------------------------------------------------------------------------------------
ConstantBufferDesc::~ConstantBufferDesc()
{
    EE_DELETE[] m_nameArray;
    EE_FREE(m_startOffsetArray);
    EE_FREE(m_sizeArray);
    EE_FREE(m_typeArray);

    EE_DELETE m_pNext;
}

//------------------------------------------------------------------------------------------------
ConstantBufferDesc& ConstantBufferDesc::operator= (const ConstantBufferDesc&)
{
    EE_FAIL("ConstantBufferDesc assignment operator should not be called.");
    return *this;
}

//------------------------------------------------------------------------------------------------
D3D11Pass::ResourceSource::~ResourceSource()
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11Pass::D3D11Pass() :
    m_pInputSignature(NULL),
    m_pVertexShaderProgramFile(NULL),
    m_pVertexShaderEntryPoint(NULL),
    m_pVertexShaderTarget(NULL),
    m_pHullShaderProgramFile(NULL),
    m_pHullShaderEntryPoint(NULL),
    m_pHullShaderTarget(NULL),
    m_pDomainShaderProgramFile(NULL),
    m_pDomainShaderEntryPoint(NULL),
    m_pDomainShaderTarget(NULL),
    m_pGeometryShaderProgramFile(NULL),
    m_pGeometryShaderEntryPoint(NULL),
    m_pGeometryShaderTarget(NULL),
    m_pPixelShaderProgramFile(NULL),
    m_pPixelShaderEntryPoint(NULL),
    m_pPixelShaderTarget(NULL),
    m_pComputeShaderProgramFile(NULL),
    m_pComputeShaderEntryPoint(NULL),
    m_pComputeShaderTarget(NULL),
    m_constantBufferDescs(NULL),
    m_renderStateGroupResetCount(EE_UINT32_MAX),
    m_unorderedAccessViewResetCount(EE_UINT32_MAX),
    m_shaderResourcesLinked(false)
{
    m_name[0] = '\0';

}

//------------------------------------------------------------------------------------------------
D3D11Pass::~D3D11Pass()
{
    EE_FREE(m_pVertexShaderProgramFile);
    EE_FREE(m_pVertexShaderEntryPoint);
    EE_FREE(m_pVertexShaderTarget);
    EE_FREE(m_pHullShaderProgramFile);
    EE_FREE(m_pHullShaderEntryPoint);
    EE_FREE(m_pHullShaderTarget);
    EE_FREE(m_pDomainShaderProgramFile);
    EE_FREE(m_pDomainShaderEntryPoint);
    EE_FREE(m_pDomainShaderTarget);
    EE_FREE(m_pGeometryShaderProgramFile);
    EE_FREE(m_pGeometryShaderEntryPoint);
    EE_FREE(m_pGeometryShaderTarget);
    EE_FREE(m_pPixelShaderProgramFile);
    EE_FREE(m_pPixelShaderEntryPoint);
    EE_FREE(m_pPixelShaderTarget);
    EE_FREE(m_pComputeShaderProgramFile);
    EE_FREE(m_pComputeShaderEntryPoint);
    EE_FREE(m_pComputeShaderTarget);

    EE_DELETE m_constantBufferDescs;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Pass::CreateNewPass(D3D11PassPtr& spNewPass)
{
    spNewPass = EE_NEW D3D11Pass;
    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Pass::ExposeShaderParameters(D3D11ShaderCore* pShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    const efd::UInt32 shaderTypeCount = pRenderer->GetSupportedShaderTypeCount();
    for (efd::UInt32 i = 0; i < shaderTypeCount; i++)
    {
        const NiGPUProgram::ProgramType programType = pRenderer->GetSupportedShaderType(i);
        D3D11ShaderProgram* pShaderProgram = m_shaders[programType].GetShaderProgram();
        if (pShaderProgram)
        {
            ID3D11ShaderReflection* pShaderReflection = pShaderProgram->GetShaderReflection();
            if (pShaderReflection)
            {
                D3D11_SHADER_DESC shaderDesc;

                HRESULT hr = pShaderReflection->GetDesc(&shaderDesc);
                if (FAILED(hr))
                {
                    D3D11Error::ReportWarning(
                        "ID3D11ShaderReflection::GetDesc failed in "
                        __FUNCTION__
                        " with error HRESULT = 0x%08X.",
                        (efd::UInt32)hr);
                    return false;
                }
                const efd::UInt32 resources = shaderDesc.BoundResources;
                for (efd::UInt32 j = 0; j < resources; j++)
                {
                    D3D11_SHADER_INPUT_BIND_DESC inputDesc;
                    hr = pShaderReflection->GetResourceBindingDesc(j, &inputDesc);
                    if (FAILED(hr))
                    {
                        D3D11Error::ReportWarning(
                            "ID3D11ShaderReflection::GetResourceBindingDesc failed in "
                            __FUNCTION__
                            " with error HRESULT = 0x%08X.",
                            (efd::UInt32)hr);
                        return false;
                    }

                    if (inputDesc.Type == D3D10_SIT_CBUFFER)
                    {
                        EE_ASSERT(inputDesc.BindCount == 1);

                        efd::FixedString bufferName = inputDesc.Name;

                        ConstantBufferDesc* pCBDesc = FindConstantBufferDesc(
                            bufferName,
                            pShaderReflection);

                        if (pCBDesc == NULL)
                        {
                            pCBDesc = AddConstantBufferDesc(bufferName, pShaderReflection);
                        }
                        EE_ASSERT(pCBDesc);
                        pCBDesc->m_shaderTypes |= (1 << programType);

                        EE_ASSERT(inputDesc.BindPoint < EE_UINT8_MAX);
                        m_shaders[programType].SetShaderConstantBufferDesc(
                            (efd::UInt8)inputDesc.BindPoint,
                            pCBDesc);
                    }
                    else if (inputDesc.Type == D3D10_SIT_TEXTURE ||
                        inputDesc.Type == D3D11_SIT_STRUCTURED)
                    {
                        efd::FixedString resourceName = inputDesc.Name;
                        EE_ASSERT(inputDesc.BindPoint < EE_UINT8_MAX);
                        m_shaders[programType].SetShaderResourceName(
                            (efd::UInt8)inputDesc.BindPoint,
                            resourceName);
                    }
                    else if (inputDesc.Type == D3D10_SIT_SAMPLER)
                    {
                        EE_ASSERT(inputDesc.BindCount == 1);

                        efd::FixedString samplerName = inputDesc.Name;
                        EE_ASSERT(inputDesc.BindPoint < EE_UINT8_MAX);
                        m_shaders[programType].SetSamplerName((efd::UInt8)inputDesc.BindPoint, samplerName);
                    }
                    else if (
                        inputDesc.Type == D3D11_SIT_UAV_RWTYPED ||
                        inputDesc.Type == D3D11_SIT_UAV_RWSTRUCTURED ||
                        inputDesc.Type == D3D11_SIT_BYTEADDRESS ||
                        inputDesc.Type == D3D11_SIT_UAV_RWBYTEADDRESS ||
                        inputDesc.Type == D3D11_SIT_UAV_APPEND_STRUCTURED ||
                        inputDesc.Type == D3D11_SIT_UAV_CONSUME_STRUCTURED ||
                        inputDesc.Type == D3D11_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
                    {
                        EE_ASSERT(inputDesc.BindCount == 1);

                        efd::FixedString uavName = inputDesc.Name;
                        EE_ASSERT(inputDesc.BindPoint < EE_UINT8_MAX);
                        m_shaders[programType].SetUAVName((efd::UInt8)inputDesc.BindPoint, uavName);
                        pShader->AddUAVSlot(uavName);
                    }
                }
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Pass::LinkUpShaderResources()
{
    // Link up constant maps
    ConstantBufferDesc* pDesc = m_constantBufferDescs;
    while (pDesc)
    {
        efd::FixedString& bufferName = pDesc->m_bufferName;

        efd::Bool found = false;
        const efd::UInt32 constantMapCount = m_shaderConstantMapArray.GetSize();
        for (efd::UInt32 i = 0; i < constantMapCount; i++)
        {
            D3D11ShaderConstantMap* pMap = m_shaderConstantMapArray.GetAt(i);
            if (pMap && pMap->IsLinkable(pDesc))
            {
                pDesc->m_constantMapIndex = i;
                found = true;
                pMap->LinkShaderConstantBuffer(pDesc);
                break;
            }
        }

        if (!found)
        {
            D3D11Error::ReportWarning(
                __FUNCTION__
                ": Constant map not found for constant buffer %s.",
                bufferName);
            return false;
        }

        pDesc = pDesc->m_pNext;
    }

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    const efd::UInt32 shaderTypeCount = pRenderer->GetSupportedShaderTypeCount();
    for (efd::UInt32 i = 0; i < shaderTypeCount; i++)
    {
        const NiGPUProgram::ProgramType programType = pRenderer->GetSupportedShaderType(i);
        D3D11ShaderProgram* pShaderProgram = m_shaders[programType].GetShaderProgram();
        if (pShaderProgram)
        {
            // Link up shader resources
            const efd::UInt8 resourceViewCount = m_shaders[programType].GetShaderResourceCount();
            for (efd::UInt8 j = 0; j < resourceViewCount; j++)
            {
                efd::FixedString resourceName;
                m_shaders[programType].GetShaderResourceName(j, resourceName);

                if (resourceName.Exists())
                {
                    efd::UInt8 resourceIndex = EE_UINT8_MAX;
                    efd::Bool found = false;
                    const efd::UInt32 resourceSourceCount = m_shaderResources.GetSize();
                    for (efd::UInt32 k = 0; k < resourceSourceCount; k++)
                    {
                        ResourceSource& source = m_shaderResources.GetAt(k);

                        if (source.GetName() == resourceName)
                        {
                            resourceIndex = (efd::UInt8)k;
                            found = true;
                            break;
                        }
                    }
                    if (found)
                    {
                        m_shaders[programType].SetShaderResourceIndex(j, resourceIndex);
                    }
                    else
                    {
                        D3D11Error::ReportWarning(
                            "Resource not found for shader resource %s "
                            "used by shader program %s in "
                            __FUNCTION__,
                            resourceName,
                            pShaderProgram->GetName());
                        return false;
                    }
                }
            }
        }
    }

    m_shaderResourcesLinked = true;

    return true;
}

//------------------------------------------------------------------------------------------------
ID3DBlob* D3D11Pass::GetInputSignature() const
{
    return m_pInputSignature;
}

//------------------------------------------------------------------------------------------------
void D3D11Pass::SetInputSignature(ID3DBlob* pInputSignature)
{
    if (m_pInputSignature != pInputSignature)
    {
        if (m_pInputSignature)
            m_pInputSignature->Release();
        m_pInputSignature = pInputSignature;
        if (m_pInputSignature)
            m_pInputSignature->AddRef();
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Pass::ApplyShaderPrograms(const NiRenderCallContext& callContext)
{
    EE_ASSERT(D3D11Renderer::GetRenderer());

    D3D11DeviceState* pDeviceState = D3D11Renderer::GetRenderer()->GetDeviceState();
    EE_ASSERT(pDeviceState);

    if (GetComputeShader())
    {
        // ComputeShader pass
        pDeviceState->CSSetShader(GetComputeShader()->GetComputeShader());
        pDeviceState->VSClearShader();
        pDeviceState->HSClearShader();
        pDeviceState->DSClearShader();
        pDeviceState->GSClearShader();
        pDeviceState->PSClearShader();
    }
    else
    {
        // Rendering pass
        pDeviceState->CSClearShader();

        // Vertex Shader
        if (GetVertexShader())
        {
            pDeviceState->VSSetShader(GetVertexShader()->GetVertexShader());
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SHADER_MISSING,
                "The vertex shader is null for object %s.",
                callContext.m_pkMesh->GetName());
            pDeviceState->VSClearShader();
        }

        // Hull Shader
        if (GetHullShader())
            pDeviceState->HSSetShader(GetHullShader()->GetHullShader());
        else
            pDeviceState->HSClearShader();

        // Domain Shader
        if (GetDomainShader())
            pDeviceState->DSSetShader(GetDomainShader()->GetDomainShader());
        else
            pDeviceState->DSClearShader();

        // Geometry Shader
        if (GetGeometryShader())
            pDeviceState->GSSetShader(GetGeometryShader()->GetGeometryShader());
        else
            pDeviceState->GSClearShader();

        // Pixel Shader
        if (GetPixelShader())
            pDeviceState->PSSetShader(GetPixelShader()->GetPixelShader());
        else
            pDeviceState->PSClearShader();
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Pass::UpdateShaderConstants(const NiRenderCallContext& callContext)
{
    // Done here to prevent shared constant buffers from being updated multiple times
    ConstantBufferDesc* pCBDesc = m_constantBufferDescs;
    while (pCBDesc)
    {
        if (pCBDesc->m_constantMapIndex == EE_UINT32_MAX)
        {
            // Bound resource not found - continue to next.
            pCBDesc = pCBDesc->m_pNext;
            continue;
        }

        EE_ASSERT(pCBDesc->m_constantMapIndex < m_shaderConstantMapArray.GetSize());
        D3D11ShaderConstantMap* pMap = m_shaderConstantMapArray.GetAt(
            pCBDesc->m_constantMapIndex);

        if (pMap == NULL)
        {
            NiShaderFactory::ReportError(
                NISHADERERR_UNKNOWN,
                false,
                "Missing shader constant map in pass %d of shader applied to mesh %s",
                callContext.m_uiPass,
                callContext.m_pkMesh->GetName());
            return EE_UINT32_MAX;
        }

        NiShaderError shaderErr = pMap->UpdateShaderConstants(callContext, true);

        if (shaderErr != NISHADERERR_OK)
        {
            NiShaderFactory::ReportError(
                shaderErr,
                true,
                "Error updating shader constants in pass %d of shader applied to mesh %s",
                callContext.m_uiPass,
                callContext.m_pkMesh->GetName());
            // Don't return failure, since shaders may still be able to run without all SCM
            // entries being set
        }

        pCBDesc = pCBDesc->m_pNext;
    }
    return 0;
}

//------------------------------------------------------------------------------------------------
void ecr::D3D11Pass::InvalidateShaderConstantUpdateCaches()
{
    const efd::UInt32 uiMapCount = m_shaderConstantMapArray.GetSize();
    for (efd::UInt32 i = 0; i < uiMapCount; ++i)
    {
        D3D11ShaderConstantMap* pMap = m_shaderConstantMapArray.GetAt(i);
        if (pMap)
            pMap->InvalidateUpdateCache();
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Pass::ApplyShaderConstants(const NiRenderCallContext& callContext)
{
    // Update the constants first
    efd::UInt32 result = UpdateShaderConstants(callContext);
    if (result != 0)
        return result;

    D3D11ShaderConstantManager* pShaderConstantManager =
        D3D11Renderer::GetRenderer()->GetShaderConstantManager();
    EE_ASSERT(pShaderConstantManager);

    // Now set them on the device context
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    const efd::UInt32 shaderTypeCount = pRenderer->GetSupportedShaderTypeCount();
    for (efd::UInt32 i = 0; i < shaderTypeCount; i++)
    {
        const NiGPUProgram::ProgramType programType = pRenderer->GetSupportedShaderType(i);
        D3D11ShaderProgramData* pShaderProgramData = &m_shaders[programType];
        if (pShaderProgramData->GetShaderProgram())
        {
            const efd::UInt32 bufferCount = pShaderProgramData->GetShaderConstantBufferCount();
            for (efd::UInt32 j = 0; j < bufferCount; j++)
            {
                ConstantBufferDesc* pCBDesc = NULL;
                if (pShaderProgramData->GetShaderConstantBufferDesc(
                    (efd::UInt8)j,
                    pCBDesc) &&
                    pCBDesc != NULL)
                {
                    if (pCBDesc->m_constantMapIndex == EE_UINT32_MAX)
                    {
                        // Bound resource not found - continue to next.
                        continue;
                    }

                    EE_ASSERT(pCBDesc->m_constantMapIndex < m_shaderConstantMapArray.GetSize());
                    D3D11ShaderConstantMap* pMap = m_shaderConstantMapArray.GetAt(
                        pCBDesc->m_constantMapIndex);

                    if (pMap == NULL)
                    {
                        NiShaderFactory::ReportError(
                            NISHADERERR_UNKNOWN,
                            false,
                            "Missing shader constant map in pass %d of shader applied to mesh %s",
                            callContext.m_uiPass,
                            callContext.m_pkMesh->GetName());
                        return EE_UINT32_MAX;
                    }

                    pShaderConstantManager->SetShaderConstantMap(
                        programType,
                        j,
                        pMap);
                }
            }
        }
    }

    return result;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Pass::ApplyTextures()
{
    ID3D11ShaderResourceView* resourceViewArray[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];

    EE_ASSERT(D3D11Renderer::GetRenderer());

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    D3D11DeviceState* pDeviceState = pRenderer->GetDeviceState();
    EE_ASSERT(pDeviceState);

    // Before we start, clear UAVs in case they overlap any SRVs we will set
    pDeviceState->CSClearUnorderedAccessViews();

    const efd::UInt32 shaderTypeCount = pRenderer->GetSupportedShaderTypeCount();
    for (efd::UInt32 i = 0; i < shaderTypeCount; i++)
    {
        const NiGPUProgram::ProgramType programType = pRenderer->GetSupportedShaderType(i);
        D3D11ShaderProgramData* pShaderProgramData = &m_shaders[programType];
        if (pShaderProgramData->GetShaderProgram())
        {
            memset(resourceViewArray, 0, sizeof(resourceViewArray));

            const efd::UInt32 resourceCount = pShaderProgramData->GetShaderResourceCount();
            for (efd::UInt32 j = 0; j < resourceCount; j++)
            {
                efd::UInt8 resourceIndex;
                if (pShaderProgramData->GetShaderResourceIndex((efd::UInt8)j, resourceIndex) &&
                    resourceIndex != 0xFF)
                {
                    const ResourceSource& resourceSource = m_shaderResources.GetAt(resourceIndex);
                    NiTexture* pResource = resourceSource.GetCachedResource();
                    if (pResource == NULL)
                    {
                        D3D11Error::ReportWarning(
                            "Shader resource %s not found for shader program %s in function "
                            __FUNCTION__,
                            resourceSource.GetName(),
                            pShaderProgramData->GetShaderProgram()->GetName());
                        continue;
                    }

                    D3D11TextureData* pTextureData =
                        (D3D11TextureData*)pResource->GetRendererData();
                    if (pTextureData == NULL)
                    {
                        // Pack texture if necessary
                        NiRenderer::GetRenderer()->PrecacheTexture(pResource);
                        pTextureData = (D3D11TextureData*)pResource->GetRendererData();
                    }

                    if (pTextureData)
                        resourceViewArray[j] = pTextureData->GetResourceView();
                }
            }
            switch (programType)
            {
            case NiGPUProgram::PROGRAM_VERTEX:
                pDeviceState->VSSetShaderResources(
                    0,
                    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
                    resourceViewArray);
                break;
            case NiGPUProgram::PROGRAM_HULL:
                pDeviceState->HSSetShaderResources(
                    0,
                    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
                    resourceViewArray);
                break;
            case NiGPUProgram::PROGRAM_DOMAIN:
                pDeviceState->DSSetShaderResources(
                    0,
                    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
                    resourceViewArray);
                break;
            case NiGPUProgram::PROGRAM_GEOMETRY:
                pDeviceState->GSSetShaderResources(
                    0,
                    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
                    resourceViewArray);
                break;
            case NiGPUProgram::PROGRAM_PIXEL:
                pDeviceState->PSSetShaderResources(
                    0,
                    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
                    resourceViewArray);
                break;
            case NiGPUProgram::PROGRAM_COMPUTE:
                pDeviceState->CSSetShaderResources(
                    0,
                    D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
                    resourceViewArray);
                break;
            }
        }
        else
        {
            switch (programType)
            {
            case NiGPUProgram::PROGRAM_VERTEX:
                pDeviceState->VSClearShaderResources();
                break;
            case NiGPUProgram::PROGRAM_HULL:
                pDeviceState->HSClearShaderResources();
                break;
            case NiGPUProgram::PROGRAM_DOMAIN:
                pDeviceState->DSClearShaderResources();
                break;
            case NiGPUProgram::PROGRAM_GEOMETRY:
                pDeviceState->GSClearShaderResources();
                break;
            case NiGPUProgram::PROGRAM_PIXEL:
                pDeviceState->PSClearShaderResources();
                break;
            case NiGPUProgram::PROGRAM_COMPUTE:
                pDeviceState->CSClearShaderResources();
                break;
            }
        }
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Pass::ApplyUAVs()
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);

    if (pRenderer->GetMaxComputeShader() == 0)
        return 0;

    ID3D11UnorderedAccessView* unorderedAccessViewArray[D3D11_PS_CS_UAV_REGISTER_COUNT];

    D3D11DeviceState* pDeviceState = D3D11Renderer::GetRenderer()->GetDeviceState();
    EE_ASSERT(pDeviceState);

    const NiGPUProgram::ProgramType programType = NiGPUProgram::PROGRAM_COMPUTE;

    D3D11ShaderProgramData* pShaderProgramData = &m_shaders[programType];
    if (pShaderProgramData->GetShaderProgram())
    {
        memset(unorderedAccessViewArray, 0, sizeof(unorderedAccessViewArray));

        const efd::UInt32 uavCount = pShaderProgramData->GetUAVCount();
        EE_ASSERT(uavCount < EE_UINT8_MAX);
        for (efd::UInt32 j = 0; j < uavCount; j++)
        {
            UAVSlot* pUAVSlot;
            if (pShaderProgramData->GetUAVSlot((efd::UInt8)j, pUAVSlot))
            {
                EE_ASSERT(pUAVSlot);
                UnorderedAccessResource* pUAR = pUAVSlot->GetUnorderedAccessResource();
                unorderedAccessViewArray[j] = pUAR->GetUnorderedAccessView();
            }
        }

        pDeviceState->CSSetUnorderedAccessViews(
            0,
            D3D11_PS_CS_UAV_REGISTER_COUNT,
            unorderedAccessViewArray,
            NULL);
    }
    else
    {
        pDeviceState->CSClearUnorderedAccessViews();
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Pass::SetupRenderingPass(const D3D11ShaderCore* pShader)
{
    EE_ASSERT(D3D11Renderer::GetRenderer());

    if (m_shaderResourcesLinked == false && LinkUpShaderResources() == false)
    {
        // Most warnings would have already been reported
        D3D11Error::ReportWarning("Error linking shader resources in " __FUNCTION__);
    }

    if (m_spRenderStateGroup &&
        m_renderStateGroupResetCount != m_spRenderStateGroup->GetResetCount())
    {
        UpdateSamplers();
        m_renderStateGroupResetCount = m_spRenderStateGroup->GetResetCount();
    }

    EE_ASSERT(pShader);
    if (m_unorderedAccessViewResetCount != pShader->GetUAVSlotResetCount())
    {
        UpdateUAVs(pShader);
        m_unorderedAccessViewResetCount = pShader->GetUAVSlotResetCount();
    }

    D3D11RenderStateManager* pRenderState =
        D3D11Renderer::GetRenderer()->GetRenderStateManager();
    EE_ASSERT(pRenderState);

    if (m_spRenderStateGroup)
    {
        D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
        EE_ASSERT(pRenderer);
        const efd::UInt32 shaderTypeCount = pRenderer->GetSupportedShaderTypeCount();
        for (efd::UInt32 i = 0; i < shaderTypeCount; i++)
        {
            const NiGPUProgram::ProgramType programType = pRenderer->GetSupportedShaderType(i);
            pRenderState->SetSamplerArray(
                programType,
                m_shaders[programType].GetSamplerCount(),
                m_shaders[programType].m_shaderSamplers);
        }
        pRenderState->ApplyRenderStateGroup(m_spRenderStateGroup);
        pRenderState->ClearSamplerArrays();
    }

    ApplyTextures();
    ApplyUAVs();

    // A new mesh/pass is about to begin. The first submesh should rebuild
    // constants normally; later submeshes can skip maps that do not depend
    // on callContext.m_uiSubmesh.
    InvalidateShaderConstantUpdateCaches();

    return NULL;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Pass::PostProcessRenderingPass(efd::UInt32)
{
    // Does nothing
    return 0;
}

//------------------------------------------------------------------------------------------------
void D3D11Pass::SetShaderResourceAsDirectResource(
    const efd::FixedString& resourceName,
    NiTexture* pResource)
{
    efd::UInt32 resourceIndex = FindShaderResourceSource(resourceName);
    if (resourceIndex == EE_UINT32_MAX)
    {
        ResourceSource newResource;
        newResource.SetName(resourceName);
        newResource.SetDirectResource(pResource);
        m_shaderResources.Add(newResource);
    }
    else
    {
        m_shaderResources.GetAt(resourceIndex).SetDirectResource(pResource);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11Pass::SetShaderResourceAsGamebryoMap(
    const efd::FixedString& resourceName,
    efd::UInt32 gbMap,
    efd::UInt16 instanceID)
{
    efd::UInt32 resourceIndex = FindShaderResourceSource(resourceName);
    if (resourceIndex == EE_UINT32_MAX)
    {
        ResourceSource newResource;
        newResource.SetName(resourceName);
        newResource.SetGamebryoMapFlags(gbMap | instanceID);
        m_shaderResources.Add(newResource);
    }
    else
    {
        m_shaderResources.GetAt(resourceIndex).SetGamebryoMapFlags(gbMap | instanceID);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11Pass::SetShaderResourceAsTextureObject(
    const efd::FixedString& resourceName,
    efd::UInt16 objectFlags)
{
    efd::UInt32 resourceIndex = FindShaderResourceSource(resourceName);
    if (resourceIndex == EE_UINT32_MAX)
    {
        ResourceSource newResource;
        newResource.SetName(resourceName);
        newResource.SetTextureObjectFlags(objectFlags);
        m_shaderResources.Add(newResource);
    }
    else
    {
        m_shaderResources.GetAt(resourceIndex).SetTextureObjectFlags(objectFlags);
    }
}

//------------------------------------------------------------------------------------------------
NiTexture* D3D11Pass::GetShaderResourceAsDirectResource(
    const efd::FixedString& resourceName) const
{
    efd::UInt32 index = FindShaderResourceSource(resourceName);
    if (index != EE_UINT32_MAX)
        return m_shaderResources.GetAt(index).GetDirectResource();

    return NULL;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Pass::GetShaderResourceAsGamebryoMap(
    const efd::FixedString& resourceName,
    efd::UInt32& gbMapFlags) const
{
    efd::UInt32 index = FindShaderResourceSource(resourceName);
    if (index != EE_UINT32_MAX)
    {
        gbMapFlags = m_shaderResources.GetAt(index).GetGamebryoMapFlags();
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11Pass::GetShaderResourceAsTextureObject(
    const efd::FixedString& resourceName,
    efd::UInt16& objectFlags) const
{
    efd::UInt32 index = FindShaderResourceSource(resourceName);
    if (index != EE_UINT32_MAX)
    {
        objectFlags = m_shaderResources.GetAt(index).GetTextureObjectFlags();
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
const efd::Bool D3D11Pass::ObtainAndCacheResource(
    efd::UInt8 shaderResourceID,
    const NiRenderCallContext& callContext,
    efd::FixedString& resourceName,
    NiTexturingProperty::ClampMode& clampMode,
    NiTexturingProperty::FilterMode& filterMode,
    efd::UInt16& maxAnisotropy)
{
    if (shaderResourceID >= m_shaderResources.GetSize())
        return false;

    ResourceSource& resourceSource = m_shaderResources.GetAt(shaderResourceID);
    resourceName = resourceSource.GetName();

    NiTexture* pResource = resourceSource.GetDirectResource();
    if (pResource)
        return true;

    if (callContext.m_pkState == NULL)
        return false;

    efd::UInt16 objectTextureFlags = resourceSource.GetTextureObjectFlags();
    efd::UInt32 gbMapFlags = resourceSource.GetGamebryoMapFlags();

    efd::Bool success = D3D11ShaderCore::ObtainTexture(
        gbMapFlags,
        objectTextureFlags,
        callContext,
        pResource,
        clampMode,
        filterMode,
        maxAnisotropy);

    if (success)
    {
        resourceSource.SetCachedResource(pResource);
    }

    return success;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Pass::FindShaderResourceSource(const efd::FixedString& resourceName) const
{
    const efd::UInt32 resourceCount = m_shaderResources.GetSize();
    for (efd::UInt32 i = 0; i < resourceCount; i++)
    {
        if (m_shaderResources.GetAt(i).GetName() == resourceName)
            return i;
    }

    return EE_UINT32_MAX;
}

//------------------------------------------------------------------------------------------------
void D3D11Pass::UpdateSamplers()
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    const efd::UInt32 shaderTypeCount = pRenderer->GetSupportedShaderTypeCount();
    for (efd::UInt32 i = 0; i < shaderTypeCount; i++)
    {
        const NiGPUProgram::ProgramType programType = pRenderer->GetSupportedShaderType(i);
        D3D11ShaderProgramData* pShaderProgramData = &m_shaders[programType];
        memset(
            pShaderProgramData->m_shaderSamplers,
            0,
            pShaderProgramData->m_shaderSamplerAllocatedSize *
            sizeof(*pShaderProgramData->m_shaderSamplers));

        const efd::UInt32 samplerCount = pShaderProgramData->GetSamplerCount();
        for (efd::UInt32 j = 0; j < samplerCount; j++)
        {
            pShaderProgramData->m_shaderSamplers[j] = m_spRenderStateGroup->GetSampler(
                pShaderProgramData->m_shaderSamplerNames[j]);
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11Pass::UpdateUAVs(const D3D11ShaderCore* pShader)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    const efd::UInt32 shaderTypeCount = pRenderer->GetSupportedShaderTypeCount();
    for (efd::UInt32 i = 0; i < shaderTypeCount; i++)
    {
        const NiGPUProgram::ProgramType programType = pRenderer->GetSupportedShaderType(i);
        D3D11ShaderProgramData* pShaderProgramData = &m_shaders[programType];
        memset(
            pShaderProgramData->m_shaderUAVs,
            0,
            pShaderProgramData->m_shaderUAVAllocatedSize *
            sizeof(*pShaderProgramData->m_shaderUAVs));

        const efd::UInt32 uavCount = pShaderProgramData->GetUAVCount();
        for (efd::UInt32 j = 0; j < uavCount; j++)
        {
            pShaderProgramData->m_shaderUAVs[j] = pShader->GetUAVSlot(
                pShaderProgramData->m_shaderUAVNames[j]);
        }
    }
}

//------------------------------------------------------------------------------------------------
ConstantBufferDesc* D3D11Pass::FindConstantBufferDesc(
    const efd::FixedString& bufferName,
    ID3D11ShaderReflection* pShaderReflection) const
{
    EE_ASSERT(pShaderReflection);
    ID3D11ShaderReflectionConstantBuffer* pBufferReflection = NULL;
    D3D11_SHADER_BUFFER_DESC bufferDesc;
    memset(&bufferDesc, 0, sizeof(bufferDesc));

    ConstantBufferDesc* pDesc = m_constantBufferDescs;
    while (pDesc)
    {
        // Check the buffer name
        if (pDesc->m_bufferName != bufferName)
        {
            pDesc = pDesc->m_pNext;
            continue;
        }

        // Obtain the reflection if it hasn't been found yet
        if (pBufferReflection == NULL)
        {
            pBufferReflection = pShaderReflection->GetConstantBufferByName(bufferName);

            // If it's still NULL, no point in looking any further, since the buffer isn't
            // in this shader
            if (pBufferReflection == NULL)
                return NULL;

            HRESULT hr = pBufferReflection->GetDesc(&bufferDesc);
            EE_ASSERT(SUCCEEDED(hr));
            EE_UNUSED_ARG(hr);
        }

        // Check the variable count name
        if (pDesc->m_variableCount != bufferDesc.Variables)
        {
            pDesc = pDesc->m_pNext;
            continue;
        }

        // Check each individual variable - name, size, offset, and type
        efd::Bool variablesMatch = true;
        const efd::UInt32 variableCount = pDesc->m_variableCount;
        for (efd::UInt32 i = 0; variablesMatch && i < variableCount; i++)
        {
            ID3D11ShaderReflectionVariable* pVariableReflection =
                pBufferReflection->GetVariableByIndex(i);
            EE_ASSERT(pVariableReflection);

            D3D11_SHADER_VARIABLE_DESC variableDesc;
            memset(&variableDesc, 0, sizeof(variableDesc));
            HRESULT hr = pVariableReflection->GetDesc(&variableDesc);
            EE_ASSERT(SUCCEEDED(hr));

            if (pDesc->m_startOffsetArray[i] != variableDesc.StartOffset ||
                pDesc->m_sizeArray[i] != variableDesc.Size ||
                pDesc->m_nameArray[i] != variableDesc.Name)
            {
                variablesMatch = false;
                continue;
            }

            ID3D11ShaderReflectionType* pTypeReflection = pVariableReflection->GetType();
            EE_ASSERT(pTypeReflection);

            D3D11_SHADER_TYPE_DESC typeDesc;
            memset(&typeDesc, 0, sizeof(typeDesc));
            hr = pTypeReflection->GetDesc(&typeDesc);
            EE_ASSERT(SUCCEEDED(hr));

            if (pDesc->m_typeArray[i].Class != typeDesc.Class ||
                pDesc->m_typeArray[i].Type != typeDesc.Type ||
                pDesc->m_typeArray[i].Rows != typeDesc.Rows ||
                pDesc->m_typeArray[i].Columns != typeDesc.Columns ||
                pDesc->m_typeArray[i].Elements != typeDesc.Elements ||
                pDesc->m_typeArray[i].Members != typeDesc.Members ||
                pDesc->m_typeArray[i].Offset != typeDesc.Offset)
            {
                variablesMatch = false;
            }
        }

        // if the variables all match, then pDesc is the desc we're looking for
        if (variablesMatch)
        {
            break;
        }

        pDesc = pDesc->m_pNext;
    }

    return pDesc;
}

//------------------------------------------------------------------------------------------------
ConstantBufferDesc* D3D11Pass::AddConstantBufferDesc(
    const efd::FixedString& bufferName,
    ID3D11ShaderReflection* pShaderReflection)
{
    EE_ASSERT(pShaderReflection);
    ID3D11ShaderReflectionConstantBuffer* pBufferReflection =
        pShaderReflection->GetConstantBufferByName(bufferName);

    // If it's not found, then we don't want to add this buffer desc, since the buffer isn't
    // in this shader
    if (pBufferReflection == NULL)
        return NULL;

    D3D11_SHADER_BUFFER_DESC bufferDesc;
    memset(&bufferDesc, 0, sizeof(bufferDesc));
    HRESULT hr = pBufferReflection->GetDesc(&bufferDesc);
    EE_ASSERT(SUCCEEDED(hr));
    EE_UNUSED_ARG(hr);

    // Create the new buffer desc
    EE_ASSERT(bufferDesc.Variables != 0);
    ConstantBufferDesc* pDesc = EE_NEW ConstantBufferDesc(bufferDesc.Variables);
    pDesc->m_bufferName = bufferName;

    const efd::UInt32 variableCount = pDesc->m_variableCount;
    for (efd::UInt32 i = 0; i < variableCount; i++)
    {
        ID3D11ShaderReflectionVariable* pVariableReflection =
            pBufferReflection->GetVariableByIndex(i);
        EE_ASSERT(pVariableReflection);

        D3D11_SHADER_VARIABLE_DESC variableDesc;
        memset(&variableDesc, 0, sizeof(variableDesc));
        HRESULT hr = pVariableReflection->GetDesc(&variableDesc);
        EE_ASSERT(SUCCEEDED(hr));

        pDesc->m_nameArray[i] = variableDesc.Name;
        pDesc->m_startOffsetArray[i] = variableDesc.StartOffset;
        pDesc->m_sizeArray[i] = variableDesc.Size;

        ID3D11ShaderReflectionType* pTypeReflection = pVariableReflection->GetType();
        EE_ASSERT(pTypeReflection);

        D3D11_SHADER_TYPE_DESC typeDesc;
        memset(&typeDesc, 0, sizeof(typeDesc));
        hr = pTypeReflection->GetDesc(&typeDesc);
        EE_ASSERT(SUCCEEDED(hr));

        pDesc->m_typeArray[i].Class = typeDesc.Class;
        pDesc->m_typeArray[i].Type = typeDesc.Type;
        pDesc->m_typeArray[i].Rows = typeDesc.Rows;
        pDesc->m_typeArray[i].Columns = typeDesc.Columns;
        pDesc->m_typeArray[i].Elements = typeDesc.Elements;
        pDesc->m_typeArray[i].Members = typeDesc.Members;
        pDesc->m_typeArray[i].Offset = typeDesc.Offset;
    }

    pDesc->m_pNext = m_constantBufferDescs;
    m_constantBufferDescs = pDesc;

    return pDesc;
}

//------------------------------------------------------------------------------------------------
