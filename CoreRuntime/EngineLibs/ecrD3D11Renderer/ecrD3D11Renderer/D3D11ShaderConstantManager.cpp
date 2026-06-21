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

#include "D3D11ShaderConstantManager.h"
#include "D3D11DeviceState.h"
#include "D3D11DataStream.h"

#include "D3D11ShaderConstantMap.h"
#include "D3D11ShaderProgram.h"

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11ShaderConstantManager::D3D11ShaderConstantManager(
    D3D11DeviceState* pkDeviceState) :
    m_pDeviceState(pkDeviceState)
{
    EE_ASSERT(m_pDeviceState);

    for (efd::UInt32 i = 0; i < NiGPUProgram::PROGRAM_MAX; i++)
        memset(m_constantBufferArray[i], 0, sizeof(m_constantBufferArray[i]));
}

//------------------------------------------------------------------------------------------------
D3D11ShaderConstantManager::~D3D11ShaderConstantManager()
{
    for (efd::UInt32 i = 0; i < NiGPUProgram::PROGRAM_MAX; i++)
    {
        for (efd::UInt32 j = 0; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT; j++)
        {
            if (m_constantBufferArray[i][j])
                m_constantBufferArray[i][j]->Release();
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantManager::SetShaderConstantMap(
    NiGPUProgram::ProgramType programType,
    efd::UInt32 bufferIndex,
    D3D11ShaderConstantMap* pConstantMap)
{
    if (pConstantMap == NULL)
        return;

    D3D11DataStream* pDataStream =
        pConstantMap->GetShaderConstantDataStream();

    if (pDataStream == NULL)
        return;

    // Upload only if the stream is dirty. UpdateD3D11Buffers already checks m_dirty.
    pDataStream->UpdateD3D11Buffers();

    ID3D11Buffer* pShaderConstantBuffer = pDataStream->GetBuffer();
    if (pShaderConstantBuffer == NULL)
        return;

    EE_ASSERT(bufferIndex < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);

    ID3D11Buffer*& pCurrent =
        m_constantBufferArray[programType][bufferIndex];

    // New fast path.
    if (pCurrent == pShaderConstantBuffer)
        return;

    pShaderConstantBuffer->AddRef();
    if (pCurrent)
        pCurrent->Release();

    pCurrent = pShaderConstantBuffer;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantManager::SetConstantBuffers(
    NiGPUProgram::ProgramType programType,
    efd::UInt32 startSlot, 
    efd::UInt32 numBuffers,
    ID3D11Buffer*const* pBuffers)
{
    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    if (pBuffers)
    {
        EE_ASSERT(startSlot + numBuffers <= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        for (efd::UInt32 i = 0; i < numBuffers; i++)
        {
            if (pBuffers[i] != m_constantBufferArray[programType][startSlot + i])
            {
                if (pBuffers[i])
                    pBuffers[i]->AddRef();
                if (m_constantBufferArray[programType][startSlot + i])
                    m_constantBufferArray[programType][startSlot + i]->Release();
                m_constantBufferArray[programType][startSlot + i] = pBuffers[i];
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantManager::GetConstantBuffers(
    NiGPUProgram::ProgramType programType,
    efd::UInt32 startSlot, 
    efd::UInt32 numBuffers,
    ID3D11Buffer** pBuffers) const
{
    if (startSlot >= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT)
        return;
    if (numBuffers > D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot)
        numBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT - startSlot;

    if (pBuffers)
    {
        EE_ASSERT(startSlot + numBuffers <= D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT);
        for (efd::UInt32 i = 0; i < numBuffers; i++)
        {
            pBuffers[i] = m_constantBufferArray[programType][startSlot + i];
        }
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantManager::ClearConstantBuffers(NiGPUProgram::ProgramType programType)
{
    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT; i++)
    {
        if (m_constantBufferArray[programType][i])
            m_constantBufferArray[programType][i]->Release();
        m_constantBufferArray[programType][i] = NULL;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderConstantManager::ApplyShaderConstants()
{
    m_pDeviceState->VSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        m_constantBufferArray[NiGPUProgram::PROGRAM_VERTEX]);
    m_pDeviceState->HSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        m_constantBufferArray[NiGPUProgram::PROGRAM_HULL]);
    m_pDeviceState->DSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        m_constantBufferArray[NiGPUProgram::PROGRAM_DOMAIN]);
    m_pDeviceState->GSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        m_constantBufferArray[NiGPUProgram::PROGRAM_GEOMETRY]);
    m_pDeviceState->PSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        m_constantBufferArray[NiGPUProgram::PROGRAM_PIXEL]);
    m_pDeviceState->CSSetConstantBuffers(
        0,
        D3D11_COMMONSHADER_CONSTANT_BUFFER_REGISTER_COUNT,
        m_constantBufferArray[NiGPUProgram::PROGRAM_COMPUTE]);
}

//------------------------------------------------------------------------------------------------
