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

#include "D3D11DeviceState.h"

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11DeviceState::D3D11DeviceState(
    ID3D11DeviceContext* pDeviceContext, 
    efd::Bool isHSDSSupported, 
    efd::Bool isGSSupported, 
    efd::Bool isCSSupported,
    efd::Bool isCSDownLevel) :
    m_pDeviceContext(pDeviceContext),
    m_pBlendState(NULL),
    m_pDepthStencilState(NULL),
    m_pRasterizerState(NULL),
    m_sampleMask(EE_UINT32_MAX),
    m_stencilRef(0),
    m_pVertexShader(NULL),
    m_pHullShader(NULL),
    m_pDomainShader(NULL),
    m_pGeometryShader(NULL),
    m_pPixelShader(NULL),
    m_pComputeShader(NULL),
    m_blendStateUnchanged(false),
    m_depthStencilStateUnchanged(false),
    m_isHSDSSupported(isHSDSSupported),
    m_isGSSupported(isGSSupported),
    m_isCSSupported(isCSSupported),
    m_isCSDownLevel(isCSDownLevel)
{
    EE_ASSERT(m_pDeviceContext);
    if (m_pDeviceContext)
        m_pDeviceContext->AddRef();

    for (efd::UInt32 i = 0; i < NiGPUProgram::PROGRAM_MAX; i++)
    {
        memset(m_samplerArray[i], 0, sizeof(m_samplerArray[i]));
        memset(m_resourceArray[i], 0, sizeof(m_resourceArray[i]));
        memset(m_bufferArray[i], 0, sizeof(m_bufferArray[i]));
        memset(m_classInstanceArray[i], 0, sizeof(m_classInstanceArray[i]));
    }

    memset(m_blendFactor, 0, sizeof(m_blendFactor));
    memset(m_csUnorderedAccessViews, 0, sizeof(m_csUnorderedAccessViews));
    memset(m_classInstanceCount, 0, sizeof(m_classInstanceCount));
}

//------------------------------------------------------------------------------------------------
D3D11DeviceState::~D3D11DeviceState()
{
    if (m_pDeviceContext)
        m_pDeviceContext->Release();

    if (m_pBlendState)
        m_pBlendState->Release();

    if (m_pDepthStencilState)
        m_pDepthStencilState->Release();

    if (m_pRasterizerState)
        m_pRasterizerState->Release();

    for (efd::UInt32 i = 0; i < NiGPUProgram::PROGRAM_MAX; i++)
    {
        efd::UInt32 j = 0;
        for (; j < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; j++)
        {
            if (m_samplerArray[i][j])
                m_samplerArray[i][j]->Release();
        }

        for (j = 0; j < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; j++)
        {
            if (m_resourceArray[i][j])
                m_resourceArray[i][j]->Release();
        }
    
        for (j = 0; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT; j++)
        {
            if (m_bufferArray[i][j])
                m_bufferArray[i][j]->Release();
        }
    
        for (j = 0; j < D3D11_SHADER_MAX_INTERFACES; j++)
        {
            if (m_classInstanceArray[i][j])
                m_classInstanceArray[i][j]->Release();
        }
    }

    if (m_pVertexShader)
        m_pVertexShader->Release();

    if (m_pHullShader)
        m_pHullShader->Release();

    if (m_pDomainShader)
        m_pDomainShader->Release();

    if (m_pGeometryShader)
        m_pGeometryShader->Release();

    if (m_pPixelShader)
        m_pPixelShader->Release();

    if (m_pComputeShader)
        m_pComputeShader->Release();
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::OMSetBlendState(
    ID3D11BlendState* pBlendState,
    const efd::Float32 blendFactor[4], 
    efd::UInt32 sampleMask)
{

    if (m_blendFactor[0] != blendFactor[0] ||
        m_blendFactor[1] != blendFactor[1] ||
        m_blendFactor[2] != blendFactor[2] ||
        m_blendFactor[3] != blendFactor[3] ||
        m_pBlendState != pBlendState ||
        m_sampleMask != sampleMask)
    {
        m_pDeviceContext->OMSetBlendState(pBlendState, blendFactor, sampleMask);
    }

    if (m_pBlendState != pBlendState)
    {
        if (m_pBlendState)
            m_pBlendState->Release();
        m_pBlendState = pBlendState;
        if (m_pBlendState)
            m_pBlendState->AddRef();
    }

    m_blendFactor[0] = blendFactor[0];
    m_blendFactor[1] = blendFactor[1];
    m_blendFactor[2] = blendFactor[2];
    m_blendFactor[3] = blendFactor[3];
    m_sampleMask = sampleMask;
    m_blendStateUnchanged = false;
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::OMGetBlendState(
    ID3D11BlendState** pBlendState,
    efd::Float32 blendFactor[4], 
    efd::UInt32* pSampleMask) const
{
    if (pBlendState)
        *pBlendState = m_pBlendState;

    blendFactor[0] = m_blendFactor[0];
    blendFactor[1] = m_blendFactor[1];
    blendFactor[2] = m_blendFactor[2];
    blendFactor[3] = m_blendFactor[3];

    if (pSampleMask)
        *pSampleMask = m_sampleMask;
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::OMClearBlendState()
{
    if (m_blendStateUnchanged)
        return;

    memset(m_blendFactor, 0, sizeof(m_blendFactor));
    m_sampleMask = UINT_MAX;

    m_pDeviceContext->OMSetBlendState(NULL, m_blendFactor, m_sampleMask);

    if (m_pBlendState)
    {
        m_pBlendState->Release();
        m_pBlendState = NULL;
    }

    m_blendStateUnchanged = true;
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::OMSetDepthStencilState(
    ID3D11DepthStencilState* pDepthStencilState, 
    efd::UInt32 stencilRef)
{
    if (m_stencilRef != stencilRef ||
        m_pDepthStencilState != pDepthStencilState)
    {
        m_pDeviceContext->OMSetDepthStencilState(pDepthStencilState, stencilRef);
    }

    if (m_pDepthStencilState != pDepthStencilState)
    {
        if (m_pDepthStencilState)
            m_pDepthStencilState->Release();
        m_pDepthStencilState = pDepthStencilState;
        if (m_pDepthStencilState)
            m_pDepthStencilState->AddRef();
    }

    m_stencilRef = stencilRef;

    m_depthStencilStateUnchanged = false;
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::OMGetDepthStencilState(
    ID3D11DepthStencilState** pDepthStencilState,
    efd::UInt32* pStencilRef) const
{
    if (pDepthStencilState)
        *pDepthStencilState = m_pDepthStencilState;
    if (pStencilRef)
        *pStencilRef = m_stencilRef;
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::OMClearDepthStencilState()
{
    if (m_depthStencilStateUnchanged)
        return;

    m_stencilRef = 0;

    m_pDeviceContext->OMSetDepthStencilState(NULL, m_stencilRef);

    if (m_pDepthStencilState)
    {
        m_pDepthStencilState->Release();
        m_pDepthStencilState = NULL;
    }

    m_depthStencilStateUnchanged = true;
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::RSSetState(ID3D11RasterizerState* pRasterizerState)
{
    if (m_pRasterizerState != pRasterizerState)
    {
        m_pDeviceContext->RSSetState(pRasterizerState);

        if (m_pRasterizerState)
            m_pRasterizerState->Release();
        m_pRasterizerState = pRasterizerState;
        if (m_pRasterizerState)
            m_pRasterizerState->AddRef();
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::RSGetState(ID3D11RasterizerState** pRasterizerState)
    const
{
    if (pRasterizerState)
        *pRasterizerState = m_pRasterizerState;
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::RSClearState()
{
    m_pDeviceContext->RSSetState(NULL);

    if (m_pRasterizerState)
    {
        m_pRasterizerState->Release();
        m_pRasterizerState = NULL;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSSetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState*const* pSamplers)
{
    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState** pIterator = m_samplerArray[NiGPUProgram::PROGRAM_VERTEX] + startSlot;

    efd::SInt32 lowerBound = numSamplers;
    efd::SInt32 upperBound = 0;

    for (efd::SInt32 i = 0; i < (efd::SInt32)numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        if (*pIterator != pSamplers[i])
        {
            lowerBound = efd::Min(i, lowerBound);
            upperBound = efd::Max(i, upperBound);
        }
        pIterator++;
    }
    if (lowerBound <= upperBound)
    {
        // Only load the span of the samplers that got changed
        // Offset the sampler array by the offset that this function introduced
        m_pDeviceContext->VSSetSamplers(
            startSlot + lowerBound,
            upperBound - lowerBound + 1,
            pSamplers + lowerBound);

        // Add/release references to samplers after set on the device to
        // silence warnings
        pIterator = m_samplerArray[NiGPUProgram::PROGRAM_VERTEX] + startSlot + lowerBound;
        for (efd::SInt32 i = lowerBound; i <= upperBound; i++)
        {
            EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
            if (*pIterator != pSamplers[i])
            {
                if (*pIterator)
                    (*pIterator)->Release();
                *pIterator = pSamplers[i];
                if (*pIterator)
                    (*pIterator)->AddRef();
            }
            pIterator++;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSGetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState** pSamplers) const
{
    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState*const* pIterator = m_samplerArray[NiGPUProgram::PROGRAM_VERTEX] + startSlot;

    for (efd::UInt32 i = 0; i < numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        pSamplers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSClearSamplers()
{
    ID3D11SamplerState* tempSamplerArray[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    memset(tempSamplerArray, 0, sizeof(tempSamplerArray));
    m_pDeviceContext->VSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, tempSamplerArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; i++)
    {
        if (m_samplerArray[NiGPUProgram::PROGRAM_VERTEX][i])
        {
            m_samplerArray[NiGPUProgram::PROGRAM_VERTEX][i]->Release();
            m_samplerArray[NiGPUProgram::PROGRAM_VERTEX][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSSetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState*const* pSamplers)
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState** pIterator = m_samplerArray[NiGPUProgram::PROGRAM_HULL] + startSlot;

    efd::SInt32 lowerBound = numSamplers;
    efd::SInt32 upperBound = 0;

    for (efd::SInt32 i = 0; i < (efd::SInt32)numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        if (*pIterator != pSamplers[i])
        {
            lowerBound = efd::Min(i, lowerBound);
            upperBound = efd::Max(i, upperBound);
        }
        pIterator++;
    }
    if (lowerBound <= upperBound)
    {
        // Only load the span of the samplers that got changed
        // Offset the sampler array by the offset that this function introduced
        m_pDeviceContext->HSSetSamplers(
            startSlot + lowerBound,
            upperBound - lowerBound + 1,
            pSamplers + lowerBound);

        // Add/release references to samplers after set on the device to
        // silence warnings
        pIterator = m_samplerArray[NiGPUProgram::PROGRAM_HULL] + startSlot + lowerBound;
        for (efd::SInt32 i = lowerBound; i <= upperBound; i++)
        {
            EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
            if (*pIterator != pSamplers[i])
            {
                if (*pIterator)
                    (*pIterator)->Release();
                *pIterator = pSamplers[i];
                if (*pIterator)
                    (*pIterator)->AddRef();
            }
            pIterator++;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSGetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState** pSamplers) const
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState*const* pIterator = m_samplerArray[NiGPUProgram::PROGRAM_HULL] + startSlot;

    for (efd::UInt32 i = 0; i < numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        pSamplers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSClearSamplers()
{
    if (!m_isHSDSSupported)
        return;

    ID3D11SamplerState* tempSamplerArray[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    memset(tempSamplerArray, 0, sizeof(tempSamplerArray));
    m_pDeviceContext->HSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, tempSamplerArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; i++)
    {
        if (m_samplerArray[NiGPUProgram::PROGRAM_HULL][i])
        {
            m_samplerArray[NiGPUProgram::PROGRAM_HULL][i]->Release();
            m_samplerArray[NiGPUProgram::PROGRAM_HULL][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSSetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState*const* pSamplers)
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState** pIterator = m_samplerArray[NiGPUProgram::PROGRAM_DOMAIN] + startSlot;

    efd::SInt32 lowerBound = numSamplers;
    efd::SInt32 upperBound = 0;

    for (efd::SInt32 i = 0; i < (efd::SInt32)numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        if (*pIterator != pSamplers[i])
        {
            lowerBound = efd::Min(i, lowerBound);
            upperBound = efd::Max(i, upperBound);
        }
        pIterator++;
    }
    if (lowerBound <= upperBound)
    {
        // Only load the span of the samplers that got changed
        // Offset the sampler array by the offset that this function introduced
        m_pDeviceContext->DSSetSamplers(
            startSlot + lowerBound,
            upperBound - lowerBound + 1,
            pSamplers + lowerBound);

        // Add/release references to samplers after set on the device to
        // silence warnings
        pIterator = m_samplerArray[NiGPUProgram::PROGRAM_DOMAIN] + startSlot + lowerBound;
        for (efd::SInt32 i = lowerBound; i <= upperBound; i++)
        {
            EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
            if (*pIterator != pSamplers[i])
            {
                if (*pIterator)
                    (*pIterator)->Release();
                *pIterator = pSamplers[i];
                if (*pIterator)
                    (*pIterator)->AddRef();
            }
            pIterator++;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSGetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState** pSamplers) const
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState*const* pIterator = m_samplerArray[NiGPUProgram::PROGRAM_DOMAIN] + startSlot;

    for (efd::UInt32 i = 0; i < numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        pSamplers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSClearSamplers()
{
    if (!m_isHSDSSupported)
        return;

    ID3D11SamplerState* tempSamplerArray[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    memset(tempSamplerArray, 0, sizeof(tempSamplerArray));
    m_pDeviceContext->DSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, tempSamplerArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; i++)
    {
        if (m_samplerArray[NiGPUProgram::PROGRAM_DOMAIN][i])
        {
            m_samplerArray[NiGPUProgram::PROGRAM_DOMAIN][i]->Release();
            m_samplerArray[NiGPUProgram::PROGRAM_DOMAIN][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSSetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState*const* pSamplers)
{
    if (!m_isGSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState** pIterator = m_samplerArray[NiGPUProgram::PROGRAM_GEOMETRY] + startSlot;

    efd::SInt32 lowerBound = numSamplers;
    efd::SInt32 upperBound = 0;

    for (efd::SInt32 i = 0; i < (efd::SInt32)numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        if (*pIterator != pSamplers[i])
        {
            lowerBound = efd::Min(i, lowerBound);
            upperBound = efd::Max(i, upperBound);
        }
        pIterator++;
    }
    if (lowerBound <= upperBound)
    {
        // Only load the span of the samplers that got changed
        // Offset the sampler array by the offset that this function introduced
        m_pDeviceContext->GSSetSamplers(
            startSlot + lowerBound,
            upperBound - lowerBound + 1,
            pSamplers + lowerBound);

        // Add/release references to samplers after set on the device to
        // silence warnings
        pIterator = m_samplerArray[NiGPUProgram::PROGRAM_GEOMETRY] + startSlot + lowerBound;
        for (efd::SInt32 i = lowerBound; i <= upperBound; i++)
        {
            EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
            if (*pIterator != pSamplers[i])
            {
                if (*pIterator)
                    (*pIterator)->Release();
                *pIterator = pSamplers[i];
                if (*pIterator)
                    (*pIterator)->AddRef();
            }
            pIterator++;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSGetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState** pSamplers) const
{
    if (!m_isGSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState*const* pIterator = 
        m_samplerArray[NiGPUProgram::PROGRAM_GEOMETRY] + startSlot;

    for (efd::UInt32 i = 0; i < numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        pSamplers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSClearSamplers()
{
    if (!m_isGSSupported)
        return;

    ID3D11SamplerState* tempSamplerArray[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    memset(tempSamplerArray, 0, sizeof(tempSamplerArray));
    m_pDeviceContext->GSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, tempSamplerArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; i++)
    {
        if (m_samplerArray[NiGPUProgram::PROGRAM_GEOMETRY][i])
        {
            m_samplerArray[NiGPUProgram::PROGRAM_GEOMETRY][i]->Release();
            m_samplerArray[NiGPUProgram::PROGRAM_GEOMETRY][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSSetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState*const* pSamplers)
{
    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState** pIterator = m_samplerArray[NiGPUProgram::PROGRAM_PIXEL] + startSlot;

    efd::SInt32 lowerBound = numSamplers;
    efd::SInt32 upperBound = 0;

    for (efd::SInt32 i = 0; i < (efd::SInt32)numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        if (*pIterator != pSamplers[i])
        {
            lowerBound = efd::Min(i, lowerBound);
            upperBound = efd::Max(i, upperBound);
        }
        pIterator++;
    }
    if (lowerBound <= upperBound)
    {
        // Only load the span of the samplers that got changed
        // Offset the sampler array by the offset that this function introduced
        m_pDeviceContext->PSSetSamplers(
            startSlot + lowerBound,
            upperBound - lowerBound + 1,
            pSamplers + lowerBound);

        // Add/release references to samplers after set on the device to
        // silence warnings
        pIterator = m_samplerArray[NiGPUProgram::PROGRAM_PIXEL] + startSlot + lowerBound;
        for (efd::SInt32 i = lowerBound; i <= upperBound; i++)
        {
            EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
            if (*pIterator != pSamplers[i])
            {
                if (*pIterator)
                    (*pIterator)->Release();
                *pIterator = pSamplers[i];
                if (*pIterator)
                    (*pIterator)->AddRef();
            }
            pIterator++;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSGetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState** pSamplers) const
{
    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState*const* pIterator = m_samplerArray[NiGPUProgram::PROGRAM_PIXEL] + startSlot;

    for (efd::UInt32 i = 0; i < numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        pSamplers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSClearSamplers()
{
    ID3D11SamplerState* tempSamplerArray[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    memset(tempSamplerArray, 0, sizeof(tempSamplerArray));
    m_pDeviceContext->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, tempSamplerArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; i++)
    {
        if (m_samplerArray[NiGPUProgram::PROGRAM_PIXEL][i])
        {
            m_samplerArray[NiGPUProgram::PROGRAM_PIXEL][i]->Release();
            m_samplerArray[NiGPUProgram::PROGRAM_PIXEL][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSSetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState*const* pSamplers)
{
    if (!m_isCSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState** pIterator = m_samplerArray[NiGPUProgram::PROGRAM_COMPUTE] + startSlot;

    efd::SInt32 lowerBound = numSamplers;
    efd::SInt32 upperBound = 0;

    for (efd::SInt32 i = 0; i < (efd::SInt32)numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        if (*pIterator != pSamplers[i])
        {
            lowerBound = efd::Min(i, lowerBound);
            upperBound = efd::Max(i, upperBound);
        }
        pIterator++;
    }
    if (lowerBound <= upperBound)
    {
        // Only load the span of the samplers that got changed
        // Offset the sampler array by the offset that this function introduced
        m_pDeviceContext->CSSetSamplers(
            startSlot + lowerBound,
            upperBound - lowerBound + 1,
            pSamplers + lowerBound);

        // Add/release references to samplers after set on the device to
        // silence warnings
        pIterator = m_samplerArray[NiGPUProgram::PROGRAM_COMPUTE] + startSlot + lowerBound;
        for (efd::SInt32 i = lowerBound; i <= upperBound; i++)
        {
            EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
            if (*pIterator != pSamplers[i])
            {
                if (*pIterator)
                    (*pIterator)->Release();
                *pIterator = pSamplers[i];
                if (*pIterator)
                    (*pIterator)->AddRef();
            }
            pIterator++;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSGetSamplers(
    efd::UInt32 startSlot,
    efd::UInt32 numSamplers, 
    ID3D11SamplerState** pSamplers) const
{
    if (!m_isCSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT || pSamplers == NULL)
        return;
    if (numSamplers > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot)
        numSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - startSlot;

    ID3D11SamplerState*const* pIterator = m_samplerArray[NiGPUProgram::PROGRAM_COMPUTE] + startSlot;

    for (efd::UInt32 i = 0; i < numSamplers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);
        pSamplers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSClearSamplers()
{
    if (!m_isCSSupported)
        return;

    ID3D11SamplerState* tempSamplerArray[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    memset(tempSamplerArray, 0, sizeof(tempSamplerArray));
    m_pDeviceContext->CSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, tempSamplerArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; i++)
    {
        if (m_samplerArray[NiGPUProgram::PROGRAM_COMPUTE][i])
        {
            m_samplerArray[NiGPUProgram::PROGRAM_COMPUTE][i]->Release();
            m_samplerArray[NiGPUProgram::PROGRAM_COMPUTE][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSSetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView*const* pResourceViews)
{
    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    m_pDeviceContext->VSSetShaderResources(startSlot, numViews, pResourceViews);

    ID3D11ShaderResourceView** pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_VERTEX] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        if (*pIterator != pResourceViews[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pResourceViews[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSGetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView** pResourceViews) const
{
    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    ID3D11ShaderResourceView*const* pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_VERTEX] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        pResourceViews[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSClearShaderResources()
{
    ID3D11ShaderResourceView* tempResourceArray[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    memset(tempResourceArray, 0, sizeof(tempResourceArray));
    m_pDeviceContext->VSSetShaderResources(
        0, 
        D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, 
        tempResourceArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
        if (m_resourceArray[NiGPUProgram::PROGRAM_VERTEX][i])
        {
            m_resourceArray[NiGPUProgram::PROGRAM_VERTEX][i]->Release();
            m_resourceArray[NiGPUProgram::PROGRAM_VERTEX][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSSetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView*const* pResourceViews)
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    m_pDeviceContext->HSSetShaderResources(startSlot, numViews, pResourceViews);

    ID3D11ShaderResourceView** pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_HULL] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        if (*pIterator != pResourceViews[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pResourceViews[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSGetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView** pResourceViews) const
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    ID3D11ShaderResourceView*const* pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_HULL] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        pResourceViews[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSClearShaderResources()
{
    if (!m_isHSDSSupported)
        return;

    ID3D11ShaderResourceView* tempResourceArray[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    memset(tempResourceArray, 0, sizeof(tempResourceArray));
    m_pDeviceContext->HSSetShaderResources(
        0, 
        D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, 
        tempResourceArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
        if (m_resourceArray[NiGPUProgram::PROGRAM_HULL][i])
        {
            m_resourceArray[NiGPUProgram::PROGRAM_HULL][i]->Release();
            m_resourceArray[NiGPUProgram::PROGRAM_HULL][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSSetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView*const* pResourceViews)
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    m_pDeviceContext->DSSetShaderResources(startSlot, numViews, pResourceViews);

    ID3D11ShaderResourceView** pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_DOMAIN] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        if (*pIterator != pResourceViews[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pResourceViews[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSGetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView** pResourceViews) const
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    ID3D11ShaderResourceView*const* pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_DOMAIN] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        pResourceViews[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSClearShaderResources()
{
    if (!m_isHSDSSupported)
        return;

    ID3D11ShaderResourceView* tempResourceArray[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    memset(tempResourceArray, 0, sizeof(tempResourceArray));
    m_pDeviceContext->DSSetShaderResources(
        0, 
        D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, 
        tempResourceArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
        if (m_resourceArray[NiGPUProgram::PROGRAM_DOMAIN][i])
        {
            m_resourceArray[NiGPUProgram::PROGRAM_DOMAIN][i]->Release();
            m_resourceArray[NiGPUProgram::PROGRAM_DOMAIN][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSSetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView*const* pResourceViews)
{
    if (!m_isGSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    m_pDeviceContext->GSSetShaderResources(startSlot, numViews, pResourceViews);

    ID3D11ShaderResourceView** pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_GEOMETRY] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        if (*pIterator != pResourceViews[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pResourceViews[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSGetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView** pResourceViews) const
{
    if (!m_isGSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    ID3D11ShaderResourceView*const* pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_GEOMETRY] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        pResourceViews[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSClearShaderResources()
{
    if (!m_isGSSupported)
        return;

    ID3D11ShaderResourceView* tempResourceArray[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    memset(tempResourceArray, 0, sizeof(tempResourceArray));
    m_pDeviceContext->GSSetShaderResources(
        0, 
        D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, 
        tempResourceArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
        if (m_resourceArray[NiGPUProgram::PROGRAM_GEOMETRY][i])
        {
            m_resourceArray[NiGPUProgram::PROGRAM_GEOMETRY][i]->Release();
            m_resourceArray[NiGPUProgram::PROGRAM_GEOMETRY][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSSetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView*const* pResourceViews)
{
    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    m_pDeviceContext->PSSetShaderResources(startSlot, numViews, pResourceViews);

    ID3D11ShaderResourceView** pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_PIXEL] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        if (*pIterator != pResourceViews[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pResourceViews[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSGetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView** pResourceViews) const
{
    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    ID3D11ShaderResourceView*const* pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_PIXEL] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        pResourceViews[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSClearShaderResources()
{
    ID3D11ShaderResourceView* tempResourceArray[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    memset(tempResourceArray, 0, sizeof(tempResourceArray));
    m_pDeviceContext->PSSetShaderResources(
        0, 
        D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, 
        tempResourceArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
        if (m_resourceArray[NiGPUProgram::PROGRAM_PIXEL][i])
        {
            m_resourceArray[NiGPUProgram::PROGRAM_PIXEL][i]->Release();
            m_resourceArray[NiGPUProgram::PROGRAM_PIXEL][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSSetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView*const* pResourceViews)
{
    if (!m_isCSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    m_pDeviceContext->CSSetShaderResources(startSlot, numViews, pResourceViews);

    ID3D11ShaderResourceView** pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_COMPUTE] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        if (*pIterator != pResourceViews[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pResourceViews[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSGetShaderResources(
    efd::UInt32 startSlot,
    efd::UInt32 numViews, 
    ID3D11ShaderResourceView** pResourceViews) const
{
    if (!m_isCSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT || pResourceViews == NULL)
        return;
    if (numViews > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot)
        numViews = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - startSlot;

    ID3D11ShaderResourceView*const* pIterator = 
        m_resourceArray[NiGPUProgram::PROGRAM_COMPUTE] + startSlot;

    for (efd::UInt32 i = 0; i < numViews; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
        pResourceViews[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSClearShaderResources()
{
    if (!m_isCSSupported)
        return;

    ID3D11ShaderResourceView* tempResourceArray[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    memset(tempResourceArray, 0, sizeof(tempResourceArray));
    m_pDeviceContext->CSSetShaderResources(
        0, 
        D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, 
        tempResourceArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
        if (m_resourceArray[NiGPUProgram::PROGRAM_COMPUTE][i])
        {
            m_resourceArray[NiGPUProgram::PROGRAM_COMPUTE][i]->Release();
            m_resourceArray[NiGPUProgram::PROGRAM_COMPUTE][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSSetConstantBuffers(
    efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer*const* pConstantBuffers)
{
    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    // DT33840 Limit range of input values wherever possible.
    efd::UInt32 uiTempNumBuffers = numBuffers;
    while (uiTempNumBuffers > 0)
    {
        if (pConstantBuffers[uiTempNumBuffers - 1] != NULL)
            break;
        uiTempNumBuffers--;
    }
    numBuffers = uiTempNumBuffers;

    if (numBuffers != 0)
        m_pDeviceContext->VSSetConstantBuffers(startSlot, numBuffers, pConstantBuffers);

    ID3D11Buffer** pIterator = m_bufferArray[NiGPUProgram::PROGRAM_VERTEX] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        if (*pIterator != pConstantBuffers[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pConstantBuffers[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSGetConstantBuffers(efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer** pConstantBuffers) const
{
    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    ID3D11Buffer*const* pIterator = m_bufferArray[NiGPUProgram::PROGRAM_VERTEX] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        pConstantBuffers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSClearConstantBuffers()
{
    ID3D11Buffer* tempBufferArray[D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT];
    memset(tempBufferArray, 0, sizeof(tempBufferArray));
    m_pDeviceContext->VSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        tempBufferArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT; i++)
    {
        if (m_bufferArray[NiGPUProgram::PROGRAM_VERTEX][i])
        {
            m_bufferArray[NiGPUProgram::PROGRAM_VERTEX][i]->Release();
            m_bufferArray[NiGPUProgram::PROGRAM_VERTEX][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSSetConstantBuffers(
    efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer*const* pConstantBuffers)
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    // DT33840 Limit range of input values wherever possible.
    efd::UInt32 uiTempNumBuffers = numBuffers;
    while (uiTempNumBuffers > 0)
    {
        if (pConstantBuffers[uiTempNumBuffers - 1] != NULL)
            break;
        uiTempNumBuffers--;
    }
    numBuffers = uiTempNumBuffers;

    if (numBuffers != 0)
        m_pDeviceContext->HSSetConstantBuffers(startSlot, numBuffers, pConstantBuffers);

    ID3D11Buffer** pIterator = m_bufferArray[NiGPUProgram::PROGRAM_HULL] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        if (*pIterator != pConstantBuffers[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pConstantBuffers[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSGetConstantBuffers(
    efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer** pConstantBuffers) const
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    ID3D11Buffer*const* pIterator = m_bufferArray[NiGPUProgram::PROGRAM_HULL] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        pConstantBuffers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSClearConstantBuffers()
{
    if (!m_isHSDSSupported)
        return;

    ID3D11Buffer* tempBufferArray[D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT];
    memset(tempBufferArray, 0, sizeof(tempBufferArray));
    m_pDeviceContext->HSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        tempBufferArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT; i++)
    {
        if (m_bufferArray[NiGPUProgram::PROGRAM_HULL][i])
        {
            m_bufferArray[NiGPUProgram::PROGRAM_HULL][i]->Release();
            m_bufferArray[NiGPUProgram::PROGRAM_HULL][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSSetConstantBuffers(
    efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer*const* pConstantBuffers)
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    // DT33840 Limit range of input values wherever possible.
    efd::UInt32 uiTempNumBuffers = numBuffers;
    while (uiTempNumBuffers > 0)
    {
        if (pConstantBuffers[uiTempNumBuffers - 1] != NULL)
            break;
        uiTempNumBuffers--;
    }
    numBuffers = uiTempNumBuffers;

    if (numBuffers != 0)
        m_pDeviceContext->DSSetConstantBuffers(startSlot, numBuffers, pConstantBuffers);

    ID3D11Buffer** pIterator = m_bufferArray[NiGPUProgram::PROGRAM_DOMAIN] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        if (*pIterator != pConstantBuffers[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pConstantBuffers[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSGetConstantBuffers(efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer** pConstantBuffers) const
{
    if (!m_isHSDSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    ID3D11Buffer*const* pIterator = m_bufferArray[NiGPUProgram::PROGRAM_DOMAIN] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        pConstantBuffers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSClearConstantBuffers()
{
    if (!m_isHSDSSupported)
        return;

    ID3D11Buffer* tempBufferArray[D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT];
    memset(tempBufferArray, 0, sizeof(tempBufferArray));
    m_pDeviceContext->DSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        tempBufferArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT; i++)
    {
        if (m_bufferArray[NiGPUProgram::PROGRAM_DOMAIN][i])
        {
            m_bufferArray[NiGPUProgram::PROGRAM_DOMAIN][i]->Release();
            m_bufferArray[NiGPUProgram::PROGRAM_DOMAIN][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSSetConstantBuffers(
    efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer*const* pConstantBuffers)
{
    if (!m_isGSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    // DT33840 Limit range of input values wherever possible.
    efd::UInt32 uiTempNumBuffers = numBuffers;
    while (uiTempNumBuffers > 0)
    {
        if (pConstantBuffers[uiTempNumBuffers - 1] != NULL)
            break;
        uiTempNumBuffers--;
    }
    numBuffers = uiTempNumBuffers;

    if (numBuffers != 0)
        m_pDeviceContext->GSSetConstantBuffers(startSlot, numBuffers, pConstantBuffers);

    ID3D11Buffer** pIterator = m_bufferArray[NiGPUProgram::PROGRAM_GEOMETRY] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        if (*pIterator != pConstantBuffers[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pConstantBuffers[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSGetConstantBuffers(efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer** pConstantBuffers) const
{
    if (!m_isGSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    ID3D11Buffer*const* pIterator = m_bufferArray[NiGPUProgram::PROGRAM_GEOMETRY] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        pConstantBuffers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSClearConstantBuffers()
{
    if (!m_isGSSupported)
        return;

    ID3D11Buffer* tempBufferArray[D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT];
    memset(tempBufferArray, 0, sizeof(tempBufferArray));
    m_pDeviceContext->GSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        tempBufferArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT; i++)
    {
        if (m_bufferArray[NiGPUProgram::PROGRAM_GEOMETRY][i])
        {
            m_bufferArray[NiGPUProgram::PROGRAM_GEOMETRY][i]->Release();
            m_bufferArray[NiGPUProgram::PROGRAM_GEOMETRY][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSSetConstantBuffers(
    efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer*const* pConstantBuffers)
{
    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    // DT33840 Limit range of input values wherever possible.
    efd::UInt32 uiTempNumBuffers = numBuffers;
    while (uiTempNumBuffers > 0)
    {
        if (pConstantBuffers[uiTempNumBuffers - 1] != NULL)
            break;
        uiTempNumBuffers--;
    }
    numBuffers = uiTempNumBuffers;

    if (numBuffers != 0)
        m_pDeviceContext->PSSetConstantBuffers(startSlot, numBuffers, pConstantBuffers);

    ID3D11Buffer** pIterator = m_bufferArray[NiGPUProgram::PROGRAM_PIXEL] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        if (*pIterator != pConstantBuffers[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pConstantBuffers[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSGetConstantBuffers(efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer** pConstantBuffers) const
{
    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    ID3D11Buffer*const* pIterator = m_bufferArray[NiGPUProgram::PROGRAM_PIXEL] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        pConstantBuffers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSClearConstantBuffers()
{
    ID3D11Buffer* tempBufferArray[D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT];
    memset(tempBufferArray, 0, sizeof(tempBufferArray));
    m_pDeviceContext->PSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        tempBufferArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT; i++)
    {
        if (m_bufferArray[NiGPUProgram::PROGRAM_PIXEL][i])
        {
            m_bufferArray[NiGPUProgram::PROGRAM_PIXEL][i]->Release();
            m_bufferArray[NiGPUProgram::PROGRAM_PIXEL][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSSetConstantBuffers(
    efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer*const* pConstantBuffers)
{
    if (!m_isCSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    // DT33840 Limit range of input values wherever possible.
    efd::UInt32 uiTempNumBuffers = numBuffers;
    while (uiTempNumBuffers > 0)
    {
        if (pConstantBuffers[uiTempNumBuffers - 1] != NULL)
            break;
        uiTempNumBuffers--;
    }
    numBuffers = uiTempNumBuffers;

    if (numBuffers != 0)
        m_pDeviceContext->CSSetConstantBuffers(startSlot, numBuffers, pConstantBuffers);

    ID3D11Buffer** pIterator = m_bufferArray[NiGPUProgram::PROGRAM_COMPUTE] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        if (*pIterator != pConstantBuffers[i])
        {
            if (*pIterator)
                (*pIterator)->Release();
            *pIterator = pConstantBuffers[i];
            if (*pIterator)
                (*pIterator)->AddRef();
        }
        pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSGetConstantBuffers(efd::UInt32 startSlot,
    efd::UInt32 numBuffers, 
    ID3D11Buffer** pConstantBuffers) const
{
    if (!m_isCSSupported)
        return;

    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT || pConstantBuffers == NULL)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    ID3D11Buffer*const* pIterator = m_bufferArray[NiGPUProgram::PROGRAM_COMPUTE] + startSlot;

    for (efd::UInt32 i = 0; i < numBuffers; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        pConstantBuffers[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSClearConstantBuffers()
{
    if (!m_isCSSupported)
        return;

    ID3D11Buffer* tempBufferArray[D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT];
    memset(tempBufferArray, 0, sizeof(tempBufferArray));
    m_pDeviceContext->CSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        tempBufferArray);

    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT; i++)
    {
        if (m_bufferArray[NiGPUProgram::PROGRAM_COMPUTE][i])
        {
            m_bufferArray[NiGPUProgram::PROGRAM_COMPUTE][i]->Release();
            m_bufferArray[NiGPUProgram::PROGRAM_COMPUTE][i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSSetShader(
    ID3D11VertexShader* pVertexShader,
    ID3D11ClassInstance*const* pClassInstances,
    efd::UInt32 numClassInstances)
{
    EE_ASSERT(pClassInstances != NULL || numClassInstances == 0);
    if (m_pVertexShader != pVertexShader ||
        numClassInstances != m_classInstanceCount[NiGPUProgram::PROGRAM_VERTEX] ||
        (pClassInstances != NULL && 
        memcmp(pClassInstances, m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX], 
        numClassInstances * sizeof(ID3D11ClassInstance*)) != 0))
    {
        m_pDeviceContext->VSSetShader(
            pVertexShader, 
            (numClassInstances == 0 ? NULL : pClassInstances), 
            numClassInstances);

        if (m_pVertexShader)
            m_pVertexShader->Release();
        m_pVertexShader = pVertexShader;
        if (pVertexShader)
            pVertexShader->AddRef();

        efd::UInt32 i = 0;
        for (; i < numClassInstances; i++)
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i] == pClassInstances[i])
                continue;

            if (m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i] = pClassInstances[i];
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i]->AddRef();
        }

        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_VERTEX])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_VERTEX] = numClassInstances;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::VSClearShader()
{
    m_pDeviceContext->VSSetShader(NULL, NULL, 0);

    if (m_pVertexShader)
    {
        m_pVertexShader->Release();
        m_pVertexShader = NULL;

        efd::UInt32 i = 0;
        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_VERTEX])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_VERTEX][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_VERTEX] = 0;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSSetShader(
    ID3D11HullShader* pHULLShader,
    ID3D11ClassInstance*const* pClassInstances,
    efd::UInt32 numClassInstances)
{
    if (!m_isHSDSSupported)
        return;

    EE_ASSERT(pClassInstances != NULL || numClassInstances == 0);
    if (m_pHullShader != pHULLShader ||
        numClassInstances != m_classInstanceCount[NiGPUProgram::PROGRAM_HULL] ||
        (pClassInstances != NULL && 
        memcmp(pClassInstances, m_classInstanceArray[NiGPUProgram::PROGRAM_HULL], 
        numClassInstances * sizeof(ID3D11ClassInstance*)) != 0))
    {
        m_pDeviceContext->HSSetShader(
            pHULLShader, 
            (numClassInstances == 0 ? NULL : pClassInstances), 
            numClassInstances);

        if (m_pHullShader)
            m_pHullShader->Release();
        m_pHullShader = pHULLShader;
        if (pHULLShader)
            pHULLShader->AddRef();

        efd::UInt32 i = 0;
        for (; i < numClassInstances; i++)
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i] == pClassInstances[i])
                continue;

            if (m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i] = pClassInstances[i];
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i]->AddRef();
        }

        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_HULL])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_HULL] = numClassInstances;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::HSClearShader()
{
    if (!m_isHSDSSupported)
        return;

    m_pDeviceContext->HSSetShader(NULL, NULL, 0);

    if (m_pHullShader)
    {
        m_pHullShader->Release();
        m_pHullShader = NULL;

        efd::UInt32 i = 0;
        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_HULL])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_HULL][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_HULL] = 0;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSSetShader(
    ID3D11DomainShader* pDOMAINShader,
    ID3D11ClassInstance*const* pClassInstances,
    efd::UInt32 numClassInstances)
{
    if (!m_isHSDSSupported)
        return;

    EE_ASSERT(pClassInstances != NULL || numClassInstances == 0);
    if (m_pDomainShader != pDOMAINShader ||
        numClassInstances != m_classInstanceCount[NiGPUProgram::PROGRAM_DOMAIN] ||
        (pClassInstances != NULL && 
        memcmp(pClassInstances, m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN], 
        numClassInstances * sizeof(ID3D11ClassInstance*)) != 0))
    {
        m_pDeviceContext->DSSetShader(
            pDOMAINShader, 
            (numClassInstances == 0 ? NULL : pClassInstances), 
            numClassInstances);

        if (m_pDomainShader)
            m_pDomainShader->Release();
        m_pDomainShader = pDOMAINShader;
        if (pDOMAINShader)
            pDOMAINShader->AddRef();

        efd::UInt32 i = 0;
        for (; i < numClassInstances; i++)
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i] == pClassInstances[i])
                continue;

            if (m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i] = pClassInstances[i];
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i]->AddRef();
        }

        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_DOMAIN])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_DOMAIN] = numClassInstances;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::DSClearShader()
{
    if (!m_isHSDSSupported)
        return;

    m_pDeviceContext->DSSetShader(NULL, NULL, 0);

    if (m_pDomainShader)
    {
        m_pDomainShader->Release();
        m_pDomainShader = NULL;

        efd::UInt32 i = 0;
        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_DOMAIN])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_DOMAIN][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_DOMAIN] = 0;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSSetShader(
    ID3D11GeometryShader* pGEOMETRYShader,
    ID3D11ClassInstance*const* pClassInstances,
    efd::UInt32 numClassInstances)
{
    if (!m_isGSSupported)
        return;

    EE_ASSERT(pClassInstances != NULL || numClassInstances == 0);
    if (m_pGeometryShader != pGEOMETRYShader ||
        numClassInstances != m_classInstanceCount[NiGPUProgram::PROGRAM_GEOMETRY] ||
        (pClassInstances != NULL && 
        memcmp(pClassInstances, m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY], 
        numClassInstances * sizeof(ID3D11ClassInstance*)) != 0))
    {
        m_pDeviceContext->GSSetShader(
            pGEOMETRYShader, 
            (numClassInstances == 0 ? NULL : pClassInstances), 
            numClassInstances);

        if (m_pGeometryShader)
            m_pGeometryShader->Release();
        m_pGeometryShader = pGEOMETRYShader;
        if (pGEOMETRYShader)
            pGEOMETRYShader->AddRef();

        efd::UInt32 i = 0;
        for (; i < numClassInstances; i++)
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i] == pClassInstances[i])
                continue;

            if (m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i] = pClassInstances[i];
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i]->AddRef();
        }

        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_GEOMETRY])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_GEOMETRY] = numClassInstances;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::GSClearShader()
{
    if (!m_isGSSupported)
        return;

    m_pDeviceContext->GSSetShader(NULL, NULL, 0);

    if (m_pGeometryShader)
    {
        m_pGeometryShader->Release();
        m_pGeometryShader = NULL;

        efd::UInt32 i = 0;
        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_GEOMETRY])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_GEOMETRY][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_GEOMETRY] = 0;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSSetShader(
    ID3D11PixelShader* pPIXELShader,
    ID3D11ClassInstance*const* pClassInstances,
    efd::UInt32 numClassInstances)
{
    EE_ASSERT(pClassInstances != NULL || numClassInstances == 0);
    if (m_pPixelShader != pPIXELShader ||
        numClassInstances != m_classInstanceCount[NiGPUProgram::PROGRAM_PIXEL] ||
        (pClassInstances != NULL && 
        memcmp(pClassInstances, m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL], 
        numClassInstances * sizeof(ID3D11ClassInstance*)) != 0))
    {
        m_pDeviceContext->PSSetShader(
            pPIXELShader, 
            (numClassInstances == 0 ? NULL : pClassInstances), 
            numClassInstances);

        if (m_pPixelShader)
            m_pPixelShader->Release();
        m_pPixelShader = pPIXELShader;
        if (pPIXELShader)
            pPIXELShader->AddRef();

        efd::UInt32 i = 0;
        for (; i < numClassInstances; i++)
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i] == pClassInstances[i])
                continue;

            if (m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i] = pClassInstances[i];
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i]->AddRef();
        }

        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_PIXEL])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_PIXEL] = numClassInstances;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::PSClearShader()
{
    m_pDeviceContext->PSSetShader(NULL, NULL, 0);

    if (m_pPixelShader)
    {
        m_pPixelShader->Release();
        m_pPixelShader = NULL;

        efd::UInt32 i = 0;
        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_PIXEL])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_PIXEL][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_PIXEL] = 0;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSSetShader(
    ID3D11ComputeShader* pCOMPUTEShader,
    ID3D11ClassInstance*const* pClassInstances,
    efd::UInt32 numClassInstances)
{
    if (!m_isCSSupported)
        return;

    EE_ASSERT(pClassInstances != NULL || numClassInstances == 0);
    if (m_pComputeShader != pCOMPUTEShader ||
        numClassInstances != m_classInstanceCount[NiGPUProgram::PROGRAM_COMPUTE] ||
        (pClassInstances != NULL && 
        memcmp(pClassInstances, m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE], 
        numClassInstances * sizeof(ID3D11ClassInstance*)) != 0))
    {
        m_pDeviceContext->CSSetShader(
            pCOMPUTEShader, 
            (numClassInstances == 0 ? NULL : pClassInstances), 
            numClassInstances);

        if (m_pComputeShader)
            m_pComputeShader->Release();
        m_pComputeShader = pCOMPUTEShader;
        if (pCOMPUTEShader)
            pCOMPUTEShader->AddRef();

        efd::UInt32 i = 0;
        for (; i < numClassInstances; i++)
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i] == pClassInstances[i])
                continue;

            if (m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i] = pClassInstances[i];
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i]->AddRef();
        }

        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_COMPUTE])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_COMPUTE] = numClassInstances;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSClearShader()
{
    if (!m_isCSSupported)
        return;

    m_pDeviceContext->CSSetShader(NULL, NULL, 0);

    if (m_pComputeShader)
    {
        m_pComputeShader->Release();
        m_pComputeShader = NULL;

        efd::UInt32 i = 0;
        while (i < m_classInstanceCount[NiGPUProgram::PROGRAM_COMPUTE])
        {
            if (m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i])
                m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i]->Release();
            m_classInstanceArray[NiGPUProgram::PROGRAM_COMPUTE][i++] = NULL;
        }

        m_classInstanceCount[NiGPUProgram::PROGRAM_COMPUTE] = 0;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSSetUnorderedAccessViews(
    efd::UInt32 startSlot,
    efd::UInt32 numUAVs, 
    ID3D11UnorderedAccessView*const* pUAVs,
    const efd::UInt32* pUAVInitialCounts)
{
    if (!m_isCSSupported)
        return;

    if (m_isCSDownLevel && numUAVs > 1)
        numUAVs = 1;

    if (startSlot >= D3D11_PS_CS_UAV_REGISTER_COUNT || pUAVs == NULL)
        return;
    if (numUAVs > D3D11_PS_CS_UAV_REGISTER_COUNT - startSlot)
        numUAVs = D3D11_PS_CS_UAV_REGISTER_COUNT - startSlot;

    ID3D11UnorderedAccessView** pIterator = m_csUnorderedAccessViews + startSlot;

    efd::SInt32 lowerBound = numUAVs;
    efd::SInt32 upperBound = 0;

    for (efd::SInt32 i = 0; i < (efd::SInt32)numUAVs; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_PS_CS_UAV_REGISTER_COUNT);
        if (*pIterator != pUAVs[i])
        {
            lowerBound = efd::Min(i, lowerBound);
            upperBound = efd::Max(i, upperBound);
        }
        pIterator++;
    }
    if (lowerBound <= upperBound)
    {
        // Only load the span of the samplers that got changed
        // Offset the sampler array by the offset that this function introduced
        m_pDeviceContext->CSSetUnorderedAccessViews(
            startSlot + lowerBound,
            upperBound - lowerBound + 1,
            pUAVs + lowerBound,
            pUAVInitialCounts + lowerBound);

        // Add/release references to samplers after set on the device to
        // silence warnings
        pIterator = m_csUnorderedAccessViews + startSlot + lowerBound;
        for (efd::SInt32 i = lowerBound; i <= upperBound; i++)
        {
            EE_ASSERT(i + startSlot < D3D11_PS_CS_UAV_REGISTER_COUNT);
            if (*pIterator != pUAVs[i])
            {
                if (*pIterator)
                    (*pIterator)->Release();
                *pIterator = pUAVs[i];
                if (*pIterator)
                    (*pIterator)->AddRef();
            }
            pIterator++;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSGetUnorderedAccessViews(
    efd::UInt32 startSlot,
    efd::UInt32 numUAVs, 
    ID3D11UnorderedAccessView** pUAVs) const
{
    if (!m_isCSSupported)
        return;

    if (startSlot >= D3D11_PS_CS_UAV_REGISTER_COUNT || pUAVs == NULL)
        return;
    if (numUAVs > D3D11_PS_CS_UAV_REGISTER_COUNT - startSlot)
        numUAVs = D3D11_PS_CS_UAV_REGISTER_COUNT - startSlot;

    ID3D11UnorderedAccessView*const* pIterator = m_csUnorderedAccessViews + startSlot;

    for (efd::UInt32 i = 0; i < numUAVs; i++)
    {
        EE_ASSERT(i + startSlot < D3D11_PS_CS_UAV_REGISTER_COUNT);
        pUAVs[i] = *pIterator++;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::CSClearUnorderedAccessViews()
{
    if (!m_isCSSupported)
        return;

    ID3D11UnorderedAccessView* tempSamplerArray[D3D11_PS_CS_UAV_REGISTER_COUNT];
    memset(tempSamplerArray, 0, sizeof(tempSamplerArray));
    //m_pDeviceContext->CSSetUnorderedAccessViews(
    //    0, 
    //    D3D11_PS_CS_UAV_REGISTER_COUNT, 
    //    tempSamplerArray, 
    //    (efd::UInt32*)tempSamplerArray); 

    for (efd::UInt32 i = 0; i < D3D11_PS_CS_UAV_REGISTER_COUNT; i++)
    {
        if (m_csUnorderedAccessViews[i])
        {
            m_csUnorderedAccessViews[i]->Release();
            m_csUnorderedAccessViews[i] = NULL;
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11DeviceState::InvalidateDeviceState()
{
    if (m_pBlendState)
        m_pBlendState->Release();
    m_pBlendState = NULL;
    if (m_pDepthStencilState)
        m_pDepthStencilState->Release();
    m_pDepthStencilState = NULL;
    if (m_pRasterizerState)
        m_pRasterizerState->Release();
    m_pRasterizerState = NULL;

    efd::UInt32 i = 0;
    for (; i < NiGPUProgram::PROGRAM_MAX; i++)
    {
        efd::UInt32 j = 0;
        for (; j < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; j++)
        {
            if (m_samplerArray[i][j])
                m_samplerArray[i][j]->Release();
            m_samplerArray[i][j] = NULL;
        }

        for (j = 0; j < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; j++)
        {
            if (m_resourceArray[i][j])
                m_resourceArray[i][j]->Release();
            m_resourceArray[i][j] = NULL;
        }

        for (j = 0; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT; j++)
        {
            if (m_bufferArray[i][j])
                m_bufferArray[i][j]->Release();
            m_bufferArray[i][j] = NULL;
        }
        for (j = 0; j < D3D11_SHADER_MAX_INTERFACES; j++)
        {
            if (m_classInstanceArray[i][j])
                m_classInstanceArray[i][j]->Release();
            m_classInstanceArray[i][j] = NULL;
        }
    }

    for (; i < D3D11_PS_CS_UAV_REGISTER_COUNT; i++)
    {
        if (m_csUnorderedAccessViews[i])
            m_csUnorderedAccessViews[i]->Release();
        m_csUnorderedAccessViews[i] = NULL;
    }
    memset(m_blendFactor, 0, sizeof(m_blendFactor));
    m_sampleMask = EE_UINT32_MAX;
    m_stencilRef = 0;

    memset(m_classInstanceCount, 0, sizeof(m_classInstanceCount));

    if (m_pVertexShader)
        m_pVertexShader->Release();
    m_pVertexShader = NULL;
    if (m_pHullShader)
        m_pHullShader->Release();
    m_pHullShader = NULL;
    if (m_pDomainShader)
        m_pDomainShader->Release();
    m_pDomainShader = NULL;
    if (m_pGeometryShader)
        m_pGeometryShader->Release();
    m_pGeometryShader = NULL;
    if (m_pPixelShader)
        m_pPixelShader->Release();
    m_pPixelShader = NULL;
    if (m_pComputeShader)
        m_pComputeShader->Release();
    m_pComputeShader = NULL;

    m_blendStateUnchanged = false;
    m_depthStencilStateUnchanged = false;
}

//------------------------------------------------------------------------------------------------
D3D11DeviceState& D3D11DeviceState::operator= (const D3D11DeviceState&)
{
    EE_FAIL("D3D11DeviceState assignment operator should not be called.");
    return *this;
}
//------------------------------------------------------------------------------------------------
