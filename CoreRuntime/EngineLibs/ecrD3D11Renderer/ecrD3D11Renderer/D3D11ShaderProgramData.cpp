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

#include "D3D11ShaderProgramData.h"

using namespace ecr;

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramData::SetShaderConstantBufferAllocatedSize(efd::UInt8 maxCount)
{
    EE_ASSERT(maxCount < D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT);

    if (maxCount > m_shaderConstantBufferAllocatedSize)
    {
        ConstantBufferDesc** pNewArray = EE_ALLOC(ConstantBufferDesc*, maxCount);
        efd::Memcpy(
            pNewArray, 
            m_shaderConstantBufferDescs, 
            m_shaderConstantBufferAllocatedSize * sizeof(*pNewArray));
        memset(
            pNewArray + m_shaderConstantBufferAllocatedSize,
            NULL,
            (maxCount - m_shaderConstantBufferAllocatedSize) * sizeof(*pNewArray));
        EE_FREE(m_shaderConstantBufferDescs);
        m_shaderConstantBufferDescs = pNewArray;

        m_shaderConstantBufferAllocatedSize = maxCount;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramData::SetShaderResourceAllocatedSize(efd::UInt8 maxCount)
{
    EE_ASSERT(maxCount < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

    if (maxCount > m_shaderResourceAllocatedSize)
    {
        efd::UInt8* pNewArray = EE_ALLOC(efd::UInt8, maxCount);
        efd::Memcpy(
            pNewArray, 
            m_shaderResourceIndices, 
            m_shaderResourceAllocatedSize * sizeof(*pNewArray));
        memset(
            pNewArray + m_shaderResourceAllocatedSize,
            0xFF,
            (maxCount - m_shaderResourceAllocatedSize) * sizeof(*pNewArray));
        EE_FREE(m_shaderResourceIndices);
        m_shaderResourceIndices = pNewArray;

        efd::FixedString* pNewStringArray = EE_NEW efd::FixedString[maxCount];
        for (efd::UInt32 i = 0; i < m_shaderResourceAllocatedSize; i++)
            pNewStringArray[i] = m_shaderResourceNames[i];
        EE_DELETE[] m_shaderResourceNames;
        m_shaderResourceNames = pNewStringArray;

        m_shaderResourceAllocatedSize = maxCount;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramData::SetSamplerAllocatedSize(efd::UInt8 maxCount)
{
    EE_ASSERT(maxCount < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    if (maxCount > m_shaderSamplerAllocatedSize)
    {
        D3D11RenderStateGroup::Sampler** pNewArray = 
            EE_ALLOC(D3D11RenderStateGroup::Sampler*, maxCount);
        efd::Memcpy(
            pNewArray, 
            m_shaderSamplers, 
            m_shaderSamplerAllocatedSize * sizeof(*pNewArray));
        memset(
            pNewArray + m_shaderSamplerAllocatedSize,
            NULL,
            (maxCount - m_shaderSamplerAllocatedSize) * sizeof(*pNewArray));
        EE_FREE(m_shaderSamplers);
        m_shaderSamplers = pNewArray;

        efd::FixedString* pNewStringArray = EE_NEW efd::FixedString[maxCount];
        for (efd::UInt32 i = 0; i < m_shaderSamplerAllocatedSize; i++)
            pNewStringArray[i] = m_shaderSamplerNames[i];
        EE_DELETE[] m_shaderSamplerNames;
        m_shaderSamplerNames = pNewStringArray;

        m_shaderSamplerAllocatedSize = maxCount;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderProgramData::SetUAVAllocatedSize(efd::UInt8 maxCount)
{
    EE_ASSERT(maxCount < D3D11_PS_CS_UAV_REGISTER_COUNT);

    if (maxCount > m_shaderUAVAllocatedSize)
    {
        UAVSlot** pNewArray = EE_ALLOC(UAVSlot*, maxCount);
        efd::Memcpy(
            pNewArray, 
            m_shaderUAVs, 
            m_shaderUAVAllocatedSize * sizeof(*pNewArray));
        memset(
            pNewArray + m_shaderUAVAllocatedSize,
            NULL,
            (maxCount - m_shaderUAVAllocatedSize) * sizeof(*pNewArray));
        EE_FREE(m_shaderUAVs);
        m_shaderUAVs = pNewArray;

        efd::FixedString* pNewStringArray = EE_NEW efd::FixedString[maxCount];
        for (efd::UInt32 i = 0; i < m_shaderUAVAllocatedSize; i++)
            pNewStringArray[i] = m_shaderUAVNames[i];
        EE_DELETE[] m_shaderUAVNames;
        m_shaderUAVNames = pNewStringArray;

        m_shaderUAVAllocatedSize = maxCount;
    }
}

//------------------------------------------------------------------------------------------------
