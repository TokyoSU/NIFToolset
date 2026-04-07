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

#include "D3D11RenderStateManager.h"
#include "D3D11DeviceState.h"
#include "D3D11Error.h"
#include "D3D11RenderStateGroup.h"

// D3DX11_STATE_BLOCK_MASK definition from Effects11 (d3dx11effect.h)
struct _D3DX11_STATE_BLOCK_MASK
{
    BYTE VS;
    BYTE VSSamplers[2];
    BYTE VSShaderResources[16];
    BYTE VSConstantBuffers[2];
    BYTE VSInterfaces[32];
    BYTE HS;
    BYTE HSSamplers[2];
    BYTE HSShaderResources[16];
    BYTE HSConstantBuffers[2];
    BYTE HSInterfaces[32];
    BYTE DS;
    BYTE DSSamplers[2];
    BYTE DSShaderResources[16];
    BYTE DSConstantBuffers[2];
    BYTE DSInterfaces[32];
    BYTE GS;
    BYTE GSSamplers[2];
    BYTE GSShaderResources[16];
    BYTE GSConstantBuffers[2];
    BYTE GSInterfaces[32];
    BYTE PS;
    BYTE PSSamplers[2];
    BYTE PSShaderResources[16];
    BYTE PSConstantBuffers[2];
    BYTE PSInterfaces[32];
    BYTE CS;
    BYTE CSSamplers[2];
    BYTE CSShaderResources[16];
    BYTE CSConstantBuffers[2];
    BYTE CSInterfaces[32];
    BYTE CSUnorderedAccessViews[1];
    BYTE IAVertexBuffers[4];
    BYTE IAIndexBuffer;
    BYTE IAInputLayout;
    BYTE IAPrimitiveTopology;
    BYTE OMRenderTargets;
    BYTE OMDepthStencilState;
    BYTE OMBlendState;
    BYTE OMBlendFactor[4];
    BYTE OMSampleMask;
    BYTE RSViewports;
    BYTE RSScissorRects;
    BYTE RSRasterizerState;
    BYTE SOBuffers;
    BYTE Predication;
};

#include <NiWireframeProperty.h>

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11RenderStateManager::D3D11RenderStateManager(
    ID3D11Device* pDevice,
    D3D11DeviceState* pDeviceState) :
    m_pDevice(pDevice),
    m_pDeviceState(pDeviceState),
    m_defaultSampleMask(EE_UINT32_MAX),
    m_defaultStencilRef(0),
    m_currentSampleMask(EE_UINT32_MAX),
    m_currentStencilRef(0),
    m_pDefaultBlendState(NULL),
    m_pDefaultDepthStencilState(NULL),
    m_pDefaultRasterizerState(NULL),
    m_pDefaultSamplerState(NULL),
    m_leftRightSwap(false),
    m_blendStateDirty(false),
    m_depthStencilStateDirty(false),
    m_rasterizerStateDirty(false)
{
    EE_ASSERT(m_pDevice);
    if (m_pDevice)
        m_pDevice->AddRef();

    // Fill in with D3D11's default values, not Gamebryo's default values.
    memset(&m_defaultBlendDesc, 0, sizeof(m_defaultBlendDesc));
    m_defaultBlendDesc.AlphaToCoverageEnable = false;
    m_defaultBlendDesc.IndependentBlendEnable = false;
    for (efd::UInt32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        m_defaultBlendDesc.RenderTarget[i].BlendEnable = false;
        m_defaultBlendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
        m_defaultBlendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
        m_defaultBlendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
        m_defaultBlendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
        m_defaultBlendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
        m_defaultBlendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        m_defaultBlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }

    memset(&m_currentBlendDesc, 0, sizeof(m_currentBlendDesc));
    m_currentBlendDesc.AlphaToCoverageEnable = false;
    m_currentBlendDesc.IndependentBlendEnable = false;
    for (efd::UInt32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        m_currentBlendDesc.RenderTarget[i].BlendEnable = false;
        m_currentBlendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
        m_currentBlendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
        m_currentBlendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
        m_currentBlendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
        m_currentBlendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
        m_currentBlendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        m_currentBlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }

    memset(&m_defaultDepthStencilDesc, 0, sizeof(m_defaultDepthStencilDesc));
    m_defaultDepthStencilDesc.DepthEnable = true;
    m_defaultDepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    m_defaultDepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    m_defaultDepthStencilDesc.StencilEnable = false;
    m_defaultDepthStencilDesc.StencilReadMask = 0;
    m_defaultDepthStencilDesc.StencilWriteMask = 0;
    m_defaultDepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    m_defaultDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    m_defaultDepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    m_defaultDepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    m_defaultDepthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    m_defaultDepthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    m_defaultDepthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    m_defaultDepthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

    memset(&m_currentDepthStencilDesc, 0, sizeof(m_currentDepthStencilDesc));
    m_currentDepthStencilDesc.DepthEnable = true;
    m_currentDepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    m_currentDepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    m_currentDepthStencilDesc.StencilEnable = false;
    m_currentDepthStencilDesc.StencilReadMask = 0;
    m_currentDepthStencilDesc.StencilWriteMask = 0;
    m_currentDepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    m_currentDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    m_currentDepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    m_currentDepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    m_currentDepthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    m_currentDepthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    m_currentDepthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    m_currentDepthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

    memset(&m_defaultRasterizerDesc, 0, sizeof(m_defaultRasterizerDesc));
    m_defaultRasterizerDesc.FillMode = D3D11_FILL_SOLID;
    m_defaultRasterizerDesc.CullMode = D3D11_CULL_BACK;
    m_defaultRasterizerDesc.FrontCounterClockwise = false;
    m_defaultRasterizerDesc.DepthBias = 0;
    m_defaultRasterizerDesc.DepthBiasClamp = 0.0f;
    m_defaultRasterizerDesc.SlopeScaledDepthBias = 0.0f;
    m_defaultRasterizerDesc.DepthClipEnable = true;
    m_defaultRasterizerDesc.ScissorEnable = false;
    m_defaultRasterizerDesc.MultisampleEnable = false;
    m_defaultRasterizerDesc.AntialiasedLineEnable = false;

    memset(&m_currentRasterizerDesc, 0, sizeof(m_currentRasterizerDesc));
    m_currentRasterizerDesc.FillMode = D3D11_FILL_SOLID;
    m_currentRasterizerDesc.CullMode = D3D11_CULL_BACK;
    m_currentRasterizerDesc.FrontCounterClockwise = false;
    m_currentRasterizerDesc.DepthBias = 0;
    m_currentRasterizerDesc.DepthBiasClamp = 0.0f;
    m_currentRasterizerDesc.SlopeScaledDepthBias = 0.0f;
    m_currentRasterizerDesc.DepthClipEnable = true;
    m_currentRasterizerDesc.ScissorEnable = false;
    m_currentRasterizerDesc.MultisampleEnable = false;
    m_currentRasterizerDesc.AntialiasedLineEnable = false;

    memset(&m_defaultSamplerDesc, 0, sizeof(m_defaultSamplerDesc));
    m_defaultSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    m_defaultSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_defaultSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_defaultSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_defaultSamplerDesc.MipLODBias = 0.0f;
    m_defaultSamplerDesc.MaxAnisotropy = 16;
    m_defaultSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    m_defaultSamplerDesc.BorderColor[0] = 0.0f;
    m_defaultSamplerDesc.BorderColor[1] = 0.0f;
    m_defaultSamplerDesc.BorderColor[2] = 0.0f;
    m_defaultSamplerDesc.BorderColor[3] = 0.0f;
    m_defaultSamplerDesc.MinLOD = -FLT_MAX;
    m_defaultSamplerDesc.MaxLOD = FLT_MAX;

    for (efd::UInt32 i = 0; i < NiGPUProgram::PROGRAM_MAX; i++)
    {
        memset(
            m_currentSamplerDescArray[i],
            0,
            sizeof(m_currentSamplerDescArray[i]));
        memset(
            m_samplersDirtyArray[i],
            0,
            sizeof(m_samplersDirtyArray[i]));
        for (efd::UInt32 j = 0; j < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; j++)
        {

            m_currentSamplerDescArray[i][j].Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            m_currentSamplerDescArray[i][j].AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            m_currentSamplerDescArray[i][j].AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            m_currentSamplerDescArray[i][j].AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            m_currentSamplerDescArray[i][j].MipLODBias = 0.0f;
            m_currentSamplerDescArray[i][j].MaxAnisotropy = 16;
            m_currentSamplerDescArray[i][j].ComparisonFunc = D3D11_COMPARISON_NEVER;
            m_currentSamplerDescArray[i][j].BorderColor[0] = 0.0f;
            m_currentSamplerDescArray[i][j].BorderColor[1] = 0.0f;
            m_currentSamplerDescArray[i][j].BorderColor[2] = 0.0f;
            m_currentSamplerDescArray[i][j].BorderColor[3] = 0.0f;
            m_currentSamplerDescArray[i][j].MinLOD = -FLT_MAX;
            m_currentSamplerDescArray[i][j].MaxLOD = FLT_MAX;
        }
    }

    memset(m_samplerCountArray, 0, sizeof(m_samplerCountArray));
    memset(m_samplerArray, 0, sizeof(m_samplerArray));

    memset(m_defaultBlendFactor, 0, sizeof(m_defaultBlendFactor));
    memset(m_currentBlendFactor, 0, sizeof(m_currentBlendFactor));

    InitDefaultValues();
}

//------------------------------------------------------------------------------------------------
D3D11RenderStateManager::~D3D11RenderStateManager()
{
    if (m_pDevice)
        m_pDevice->Release();

    if (m_pDefaultBlendState)
        m_pDefaultBlendState->Release();

    if (m_pDefaultDepthStencilState)
        m_pDefaultDepthStencilState->Release();

    if (m_pDefaultRasterizerState)
        m_pDefaultRasterizerState->Release();

    if (m_pDefaultSamplerState)
        m_pDefaultSamplerState->Release();
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::InitDefaultValues()
{
    // Fill in Gamebryo's default values.

    // Winding order
    m_defaultRasterizerDesc.FrontCounterClockwise = true;

    // Enable multisampling
    m_defaultRasterizerDesc.MultisampleEnable = true;
    m_defaultRasterizerDesc.AntialiasedLineEnable = true;

    // Default depth test mode
    m_defaultDepthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

    // Default sampler states
    m_defaultSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    m_defaultSamplerDesc.MaxAnisotropy = 1;

    // Current values will be updated in ResetCurrentState.
    UpdateDefaultBlendStateObject();
    UpdateDefaultDepthStencilStateObject();
    UpdateDefaultRasterizerStateObject();
    UpdateDefaultSamplerObject();

    ResetCurrentState();
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::UpdateDefaultBlendStateObject()
{
    ID3D11BlendState* pNewDefaultBlendState = NULL;
    HRESULT hr = m_pDevice->CreateBlendState(
        &m_defaultBlendDesc,
        &pNewDefaultBlendState);

    if (FAILED(hr) || pNewDefaultBlendState == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_BLEND_STATE_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_BLEND_STATE_CREATION_FAILED,
                "No error message from D3D11, but blend state is NULL.");
        }
    }

    if (m_pDefaultBlendState)
        m_pDefaultBlendState->Release();
    m_pDefaultBlendState = pNewDefaultBlendState;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::UpdateDefaultDepthStencilStateObject()
{
    ID3D11DepthStencilState* pNewDefaultDepthStencilState = NULL;
    HRESULT hr = m_pDevice->CreateDepthStencilState(
        &m_defaultDepthStencilDesc,
        &pNewDefaultDepthStencilState);

    if (FAILED(hr) || pNewDefaultDepthStencilState == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_DEPTH_STENCIL_STATE_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(D3D11Error::D3D11ERROR_DEPTH_STENCIL_STATE_CREATION_FAILED,
                "No error message from D3D11, but depth stencil state is "
                "NULL.");
        }
    }

    if (m_pDefaultDepthStencilState)
        m_pDefaultDepthStencilState->Release();
    m_pDefaultDepthStencilState = pNewDefaultDepthStencilState;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::UpdateDefaultRasterizerStateObject()
{
    ID3D11RasterizerState* pNewDefaultRasterizerState = NULL;

    HRESULT hr = m_pDevice->CreateRasterizerState(
        &m_defaultRasterizerDesc,
        &pNewDefaultRasterizerState);

    if (FAILED(hr) || pNewDefaultRasterizerState == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_RASTERIZER_STATE_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_RASTERIZER_STATE_CREATION_FAILED,
                "No error message from D3D11, but rasterizer state is "
                "NULL.");
        }
    }

    if (m_pDefaultRasterizerState)
        m_pDefaultRasterizerState->Release();
    m_pDefaultRasterizerState = pNewDefaultRasterizerState;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::UpdateDefaultSamplerObject()
{
    ID3D11SamplerState* pNewDefaultSamplerState = NULL;
    HRESULT hr = m_pDevice->CreateSamplerState(
        &m_defaultSamplerDesc,
        &pNewDefaultSamplerState);

    if (FAILED(hr) || pNewDefaultSamplerState == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SAMPLER_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SAMPLER_CREATION_FAILED,
                "No error message from D3D11, but sampler is NULL.");
        }
    }

    if (m_pDefaultSamplerState)
        m_pDefaultSamplerState->Release();
    m_pDefaultSamplerState = pNewDefaultSamplerState;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateManager::ApplyProperties(const NiPropertyState* pState)
{
    if (pState == NULL)
        return false;

    // Note that Gamebryo's Dither, Fog, Material, Shading, and Specular
    // properties are either entirely implemented by shaders or are not
    // supported in D3D11.
    if ((ApplyAlphaProperty(pState->GetAlpha()) == false) ||
        (ApplyStencilProperty(pState->GetStencil()) == false) ||
        (ApplyWireframeProperty(pState->GetWireframe()) == false) ||
        (ApplyZBufferProperty(pState->GetZBuffer()) == false))
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateManager::ApplyAlphaProperty(const NiAlphaProperty* pNew)
{
    if (pNew == NULL)
    {
        D3D11Error::ReportWarning("Cannot apply alpha state with NULL alpha property.");
        return false;
    }

    m_currentBlendDesc.IndependentBlendEnable = false;
    if (pNew->GetAlphaBlending())
    {
        m_currentBlendDesc.RenderTarget[0].BlendEnable = true;
        m_currentBlendDesc.RenderTarget[0].SrcBlend = ConvertGbBlendToD3D11Blend(
            pNew->GetSrcBlendMode());
        m_currentBlendDesc.RenderTarget[0].DestBlend = ConvertGbBlendToD3D11Blend(
            pNew->GetDestBlendMode());
        m_currentBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    }
    else
    {
        m_currentBlendDesc.RenderTarget[0].BlendEnable = false;
    }

    m_blendStateDirty = true;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateManager::ApplyStencilProperty(const NiStencilProperty* pNew)
{
    if (pNew == NULL)
    {
        D3D11Error::ReportWarning("Cannot apply stencil state with NULL stencil property.");
        return false;
    }

    if (pNew->GetStencilOn())
    {
        m_currentDepthStencilDesc.StencilEnable = true;

        m_currentDepthStencilDesc.StencilWriteMask = static_cast<efd::UInt8>(
            pNew->GetStencilWriteMask() & EE_UINT8_MAX);
        m_currentDepthStencilDesc.StencilReadMask = static_cast<efd::UInt8>(
            pNew->GetStencilMask() & EE_UINT8_MAX);

        m_currentStencilRef = pNew->GetStencilReference();

        m_currentDepthStencilDesc.FrontFace.StencilFunc =
            ConvertGbStencilFuncToD3D11Comparison(pNew->GetStencilFunction());
        m_currentDepthStencilDesc.FrontFace.StencilDepthFailOp =
            ConvertGbStencilActionToD3D11StencilOp(
            pNew->GetStencilPassZFailAction());
        m_currentDepthStencilDesc.FrontFace.StencilPassOp =
            ConvertGbStencilActionToD3D11StencilOp(
            pNew->GetStencilPassAction());
        m_currentDepthStencilDesc.FrontFace.StencilFailOp =
            ConvertGbStencilActionToD3D11StencilOp(
            pNew->GetStencilFailAction());
        m_currentDepthStencilDesc.BackFace = m_currentDepthStencilDesc.FrontFace;
    }
    else
    {
        m_currentDepthStencilDesc.StencilEnable = false;
    }

    NiStencilProperty::DrawMode drawMode = pNew->GetDrawMode();

    m_currentRasterizerDesc.FrontCounterClockwise = true;
    if (drawMode == NiStencilProperty::DRAW_CW)
    {
        m_currentRasterizerDesc.CullMode = D3D11_CULL_FRONT;
    }
    else if (drawMode == NiStencilProperty::DRAW_BOTH)
    {
        m_currentRasterizerDesc.CullMode = D3D11_CULL_NONE;
    }
    else //(drawMode == NiStencilProperty::DRAW_CCW_OR_BOTH ||
         // drawMode == NiStencilProperty::DRAW_CCW)
    {
        m_currentRasterizerDesc.CullMode = D3D11_CULL_BACK;
    }

    m_depthStencilStateDirty = true;
    m_rasterizerStateDirty = true;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateManager::ApplyWireframeProperty(const NiWireframeProperty* pNew)
{
    if (pNew == NULL)
    {
        D3D11Error::ReportWarning("Cannot apply wireframe state with NULL wireframe property.");
        return false;
    }

    if (pNew->GetWireframe())
        m_currentRasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    else
        m_currentRasterizerDesc.FillMode = D3D11_FILL_SOLID;

    m_rasterizerStateDirty = true;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateManager::ApplyZBufferProperty(const NiZBufferProperty* pNew)
{
    if (pNew == NULL)
    {
        D3D11Error::ReportWarning("Cannot apply Z buffer state with NULL Z buffer property.");
        return false;
    }

    m_currentDepthStencilDesc.DepthEnable = (pNew->GetZBufferTest() || pNew->GetZBufferWrite());
    if (pNew->GetZBufferWrite())
    {
        m_currentDepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    }
    else
    {
        m_currentDepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    }

    if (pNew->GetZBufferTest())
    {
        m_currentDepthStencilDesc.DepthFunc =
            ConvertGbDepthFuncToD3D11Comparison(pNew->GetTestFunction());
    }
    else
    {
        m_currentDepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    }

    m_depthStencilStateDirty = true;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateManager::ApplyRenderStateGroup(const D3D11RenderStateGroup* pRSGroup)
{
    D3D11_BLEND_DESC blendDesc;
    efd::UInt32 blendStateValid = 0;
    efd::UInt8 renderTargetValid[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    pRSGroup->GetBlendStateDesc(blendDesc, blendStateValid, renderTargetValid);

    SetBlendStateDesc(blendDesc, blendStateValid, renderTargetValid);

    float blendFactor[4];
    if (pRSGroup->GetBlendFactor(blendFactor))
    {
        SetBlendFactor(blendFactor);
    }

    efd::UInt32 sampleMask = 0;
    if (pRSGroup->GetSampleMask(sampleMask))
    {
        SetSampleMask(sampleMask);
    }

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    efd::UInt32 depthStencilStateValid = 0;
    pRSGroup->GetDepthStencilStateDesc(depthStencilDesc, depthStencilStateValid);

    SetDepthStencilStateDesc(depthStencilDesc, depthStencilStateValid);

    efd::UInt32 stencilRef = 0;
    if (pRSGroup->GetStencilRef(stencilRef))
    {
        SetStencilRef(stencilRef);
    }

    D3D11_RASTERIZER_DESC rasterizerDesc;
    efd::UInt32 rasterizerStateValid = 0;
    pRSGroup->GetRasterizerStateDesc(rasterizerDesc, rasterizerStateValid);

    SetRasterizerStateDesc(rasterizerDesc, rasterizerStateValid);

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer);
    const efd::UInt32 shaderTypeCount = pRenderer->GetSupportedShaderTypeCount();
    for (efd::UInt32 i = 0; i < shaderTypeCount; i++)
    {
        const NiGPUProgram::ProgramType programType = pRenderer->GetSupportedShaderType(i);
        const efd::UInt8 samplerCount = m_samplerCountArray[programType];
        for (efd::UInt32 j = 0; j < samplerCount; j++)
        {
            if (m_samplerArray[programType][j])
            {
                SetSamplerDesc(
                    programType,
                    j,
                    m_samplerArray[programType][j]->GetSamplerDesc(),
                    m_samplerArray[programType][j]->GetSamplerValidFlags());
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::SetBlendStateDesc(
    const D3D11_BLEND_DESC& blendDesc, 
    efd::UInt32 validFlags, 
    efd::UInt8 renderTargetValidFlags[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT])
{
    if ((validFlags & BSVALID_ALPHATOCOVERAGEENABLE) != 0)
    {
        m_currentBlendDesc.AlphaToCoverageEnable = blendDesc.AlphaToCoverageEnable;
    }
    if ((validFlags & BSVALID_INDEPENDENTBLENDENABLE) != 0)
    {
        m_currentBlendDesc.IndependentBlendEnable = blendDesc.IndependentBlendEnable;
    }

    for (efd::UInt32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        efd::UInt32 flag = 1 << (i + 2);
        if ((validFlags & flag) != 0)
        {
            if ((renderTargetValidFlags[i] & BSRTVALID_BLENDENABLE) != 0)
            {
                m_currentBlendDesc.RenderTarget[i].BlendEnable = 
                    blendDesc.RenderTarget[i].BlendEnable;
            }
            if ((renderTargetValidFlags[i] & BSRTVALID_SRCBLEND) != 0)
            {
                m_currentBlendDesc.RenderTarget[i].SrcBlend = 
                    blendDesc.RenderTarget[i].SrcBlend;
            }
            if ((renderTargetValidFlags[i] & BSRTVALID_DESTBLEND) != 0)
            {
                m_currentBlendDesc.RenderTarget[i].DestBlend = 
                    blendDesc.RenderTarget[i].DestBlend;
            }
            if ((renderTargetValidFlags[i] & BSRTVALID_BLENDOP) != 0)
            {
                m_currentBlendDesc.RenderTarget[i].BlendOp = 
                    blendDesc.RenderTarget[i].BlendOp;
            }
            if ((renderTargetValidFlags[i] & BSRTVALID_SRCBLENDALPHA) != 0)
            {
                m_currentBlendDesc.RenderTarget[i].SrcBlendAlpha = 
                    blendDesc.RenderTarget[i].SrcBlendAlpha;
            }
            if ((renderTargetValidFlags[i] & BSRTVALID_DESTBLENDALPHA) != 0)
            {
                m_currentBlendDesc.RenderTarget[i].DestBlendAlpha = 
                    blendDesc.RenderTarget[i].DestBlendAlpha;
            }
            if ((renderTargetValidFlags[i] & BSRTVALID_BLENDOPALPHA) != 0)
            {
                m_currentBlendDesc.RenderTarget[i].BlendOpAlpha = 
                    blendDesc.RenderTarget[i].BlendOpAlpha;
            }
            if ((renderTargetValidFlags[i] & BSRTVALID_RENDERTARGETWRITEMASK) != 0)
            {
                m_currentBlendDesc.RenderTarget[i].RenderTargetWriteMask =
                    blendDesc.RenderTarget[i].RenderTargetWriteMask;
            }
        }
    }

    if (validFlags != 0)
        m_blendStateDirty = true;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::SetDepthStencilStateDesc(
    const D3D11_DEPTH_STENCIL_DESC& depthStencilDesc, 
    efd::UInt32 validFlags)
{
    if ((validFlags & DSSVALID_DEPTHENABLE) != 0)
    {
        m_currentDepthStencilDesc.DepthEnable = depthStencilDesc.DepthEnable;
    }
    if ((validFlags & DSSVALID_DEPTHWRITEMASK) != 0)
    {
        m_currentDepthStencilDesc.DepthWriteMask = depthStencilDesc.DepthWriteMask;
    }
    if ((validFlags & DSSVALID_DEPTHFUNC) != 0)
    {
        m_currentDepthStencilDesc.DepthFunc = depthStencilDesc.DepthFunc;
    }
    if ((validFlags & DSSVALID_STENCILENABLE) != 0)
    {
        m_currentDepthStencilDesc.StencilEnable = depthStencilDesc.StencilEnable;
    }
    if ((validFlags & DSSVALID_STENCILREADMASK) != 0)
    {
        m_currentDepthStencilDesc.StencilReadMask = depthStencilDesc.StencilReadMask;
    }
    if ((validFlags & DSSVALID_STENCILWRITEMASK) != 0)
    {
        m_currentDepthStencilDesc.StencilWriteMask = depthStencilDesc.StencilWriteMask;
    }
    if ((validFlags & DSSVALID_FRONTFACE_STENCILFAILOP) != 0)
    {
        m_currentDepthStencilDesc.FrontFace.StencilFailOp =
            depthStencilDesc.FrontFace.StencilFailOp;
    }
    if ((validFlags & DSSVALID_FRONTFACE_STENCILDEPTHFAILOP) != 0)
    {
        m_currentDepthStencilDesc.FrontFace.StencilDepthFailOp =
            depthStencilDesc.FrontFace.StencilDepthFailOp;
    }
    if ((validFlags & DSSVALID_FRONTFACE_STENCILPASSOP) != 0)
    {
        m_currentDepthStencilDesc.FrontFace.StencilPassOp =
            depthStencilDesc.FrontFace.StencilPassOp;
    }
    if ((validFlags & DSSVALID_FRONTFACE_STENCILFUNC) != 0)
    {
        m_currentDepthStencilDesc.FrontFace.StencilFunc =
            depthStencilDesc.FrontFace.StencilFunc;
    }
    if ((validFlags & DSSVALID_BACKFACE_STENCILFAILOP) != 0)
    {
        m_currentDepthStencilDesc.BackFace.StencilFailOp =
            depthStencilDesc.BackFace.StencilFailOp;
    }
    if ((validFlags & DSSVALID_BACKFACE_STENCILDEPTHFAILOP) != 0)
    {
        m_currentDepthStencilDesc.BackFace.StencilDepthFailOp =
            depthStencilDesc.BackFace.StencilDepthFailOp;
    }
    if ((validFlags & DSSVALID_BACKFACE_STENCILPASSOP) != 0)
    {
        m_currentDepthStencilDesc.BackFace.StencilPassOp =
            depthStencilDesc.BackFace.StencilPassOp;
    }
    if ((validFlags & DSSVALID_BACKFACE_STENCILFUNC) != 0)
    {
        m_currentDepthStencilDesc.BackFace.StencilFunc =
            depthStencilDesc.BackFace.StencilFunc;
    }

    if (validFlags != 0)
        m_depthStencilStateDirty = true;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::SetRasterizerStateDesc(
    const D3D11_RASTERIZER_DESC& rasterizerDesc, 
    efd::UInt32 validFlags)
{
    if ((validFlags & RSVALID_FILLMODE) != 0)
    {
        m_currentRasterizerDesc.FillMode = rasterizerDesc.FillMode;
    }
    if ((validFlags & RSVALID_CULLMODE) != 0)
    {
        m_currentRasterizerDesc.CullMode = rasterizerDesc.CullMode;
    }
    if ((validFlags & RSVALID_FRONTCOUNTERCLOCKWISE) != 0)
    {
        m_currentRasterizerDesc.FrontCounterClockwise =
            rasterizerDesc.FrontCounterClockwise;
    }
    if ((validFlags & RSVALID_DEPTHBIAS) != 0)
    {
        m_currentRasterizerDesc.DepthBias = rasterizerDesc.DepthBias;
    }
    if ((validFlags & RSVALID_DEPTHBIASCLAMP) != 0)
    {
        m_currentRasterizerDesc.DepthBiasClamp = rasterizerDesc.DepthBiasClamp;
    }
    if ((validFlags & RSVALID_SLOPESCALEDDEPTHBIAS) != 0)
    {
        m_currentRasterizerDesc.SlopeScaledDepthBias =
            rasterizerDesc.SlopeScaledDepthBias;
    }
    if ((validFlags & RSVALID_DEPTHCLIPENABLE) != 0)
    {
        m_currentRasterizerDesc.DepthClipEnable = rasterizerDesc.DepthClipEnable;
    }
    if ((validFlags & RSVALID_SCISSORENABLE) != 0)
    {
        m_currentRasterizerDesc.ScissorEnable = rasterizerDesc.ScissorEnable;
    }
    if ((validFlags & RSVALID_MULTISAMPLEENABLE) != 0)
    {
        m_currentRasterizerDesc.MultisampleEnable = rasterizerDesc.MultisampleEnable;
    }
    if ((validFlags & RSVALID_ANTIALIASEDLINEENABLE) != 0)
    {
        m_currentRasterizerDesc.AntialiasedLineEnable =
            rasterizerDesc.AntialiasedLineEnable;
    }

    if (validFlags != 0)
        m_rasterizerStateDirty = true;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::SetSamplerDesc(
    NiGPUProgram::ProgramType shaderType, 
    efd::UInt32 sampler,
    const D3D11_SAMPLER_DESC& samplerDesc, 
    efd::UInt32 validFlags)
{
    if (sampler >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)
        return;

    D3D11_SAMPLER_DESC& currentDesc =
        m_currentSamplerDescArray[(efd::UInt32)shaderType][sampler];

    if ((validFlags & SVALID_FILTER) != 0)
    {
        currentDesc.Filter = samplerDesc.Filter;
    }
    if ((validFlags & SVALID_ADDRESSU) != 0)
    {
        currentDesc.AddressU = samplerDesc.AddressU;
    }
    if ((validFlags & SVALID_ADDRESSV) != 0)
    {
        currentDesc.AddressV = samplerDesc.AddressV;
    }
    if ((validFlags & SVALID_ADDRESSW) != 0)
    {
        currentDesc.AddressW = samplerDesc.AddressW;
    }
    if ((validFlags & SVALID_MIPLODBIAS) != 0)
    {
        currentDesc.MipLODBias = samplerDesc.MipLODBias;
    }
    if ((validFlags & SVALID_MAXANISOTROPY) != 0)
    {
        currentDesc.MaxAnisotropy = samplerDesc.MaxAnisotropy;
    }
    if ((validFlags & SVALID_COMPARISONFUNC) != 0)
    {
        currentDesc.ComparisonFunc = samplerDesc.ComparisonFunc;
    }
    if ((validFlags & SVALID_BORDERCOLOR) != 0)
    {
        currentDesc.BorderColor[0] = samplerDesc.BorderColor[0];
        currentDesc.BorderColor[1] = samplerDesc.BorderColor[1];
        currentDesc.BorderColor[2] = samplerDesc.BorderColor[2];
        currentDesc.BorderColor[3] = samplerDesc.BorderColor[3];
    }
    if ((validFlags & SVALID_MINLOD) != 0)
    {
        currentDesc.MinLOD = samplerDesc.MinLOD;
    }
    if ((validFlags & SVALID_MAXLOD) != 0)
    {
        currentDesc.MaxLOD = samplerDesc.MaxLOD;
    }

    if (validFlags)
        m_samplersDirtyArray[shaderType][sampler] = true;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::ResetCurrentState()
{
    m_currentBlendDesc = m_defaultBlendDesc;
    m_currentDepthStencilDesc = m_defaultDepthStencilDesc;
    m_currentRasterizerDesc = m_defaultRasterizerDesc;

    m_blendStateDirty = false;
    m_depthStencilStateDirty = false;
    m_rasterizerStateDirty = false;

    efd::UInt32 i = 0;
    for (; i < NiGPUProgram::PROGRAM_MAX; i++)
    {
        for (efd::UInt32 j = 0; j < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; j++)
        {
            m_currentSamplerDescArray[i][j] = m_defaultSamplerDesc;
            m_samplersDirtyArray[i][j] = false;
        }
    }

    for (i = 0; i < 4; i++)
        m_currentBlendFactor[i] = m_defaultBlendFactor[i];

    m_currentSampleMask = m_defaultSampleMask;
    m_currentStencilRef = m_defaultStencilRef;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::SetDefaultSamplerDesc(
    const D3D11_SAMPLER_DESC& samplerDesc)
{
    m_defaultSamplerDesc = samplerDesc;
    UpdateDefaultSamplerObject();
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::ApplyCurrentState(
    const D3DX11_STATE_BLOCK_MASK* pMask)
{
    if (pMask == NULL || pMask->OMBlendState != 0)
        ApplyCurrentBlendState();
    if (pMask == NULL || pMask->OMDepthStencilState != 0)
        ApplyCurrentDepthStencilState();
    if (pMask == NULL || pMask->RSRasterizerState != 0)
        ApplyCurrentRasterizerState();
    ApplyCurrentSamplers(pMask);
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::ApplyCurrentBlendState()
{
    ID3D11BlendState* pBState = NULL;
    if (m_blendStateDirty)
    {
        HRESULT hr = m_pDevice->CreateBlendState(
            &m_currentBlendDesc,
            &pBState);

        if (FAILED(hr) || pBState == NULL)
        {
            if (FAILED(hr))
            {
                D3D11Error::ReportError(
                    D3D11Error::D3D11ERROR_BLEND_STATE_CREATION_FAILED,
                    "Error HRESULT = 0x%08X.", 
                    (efd::UInt32)hr);
            }
            else
            {
                D3D11Error::ReportError(
                    D3D11Error::D3D11ERROR_BLEND_STATE_CREATION_FAILED,
                    "No error message from D3D11, but blend state is NULL.");
            }
        }
        m_blendStateDirty = false;
    }

    if (pBState == NULL)
    {
        m_pDeviceState->OMSetBlendState(
            m_pDefaultBlendState,
            m_currentBlendFactor,
            m_currentSampleMask);
    }
    else
    {
        m_pDeviceState->OMSetBlendState(
            pBState,
            m_currentBlendFactor,
            m_currentSampleMask);
        pBState->Release();
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::ApplyCurrentDepthStencilState()
{
    ID3D11DepthStencilState* pDSState = NULL;
    if (m_depthStencilStateDirty)
    {
        HRESULT hr = m_pDevice->CreateDepthStencilState(
            &m_currentDepthStencilDesc,
            &pDSState);

        if (FAILED(hr) || pDSState == NULL)
        {
            if (FAILED(hr))
            {
                D3D11Error::ReportError(D3D11Error::D3D11ERROR_DEPTH_STENCIL_STATE_CREATION_FAILED,
                    "Error HRESULT = 0x%08X.", 
                    (efd::UInt32)hr);
            }
            else
            {
                D3D11Error::ReportError(D3D11Error::D3D11ERROR_DEPTH_STENCIL_STATE_CREATION_FAILED,
                    "No error message from D3D11, but depth stencil state is NULL.");
            }
        }
        m_depthStencilStateDirty = false;
    }

    if (pDSState == NULL)
    {
        m_pDeviceState->OMSetDepthStencilState(
            m_pDefaultDepthStencilState,
            m_currentStencilRef);
    }
    else
    {
        m_pDeviceState->OMSetDepthStencilState(
            pDSState,
            m_currentStencilRef);
        pDSState->Release();
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::ApplyCurrentRasterizerState()
{
    // If left/right swap is enabled, the front/back state must be swapped.
    if (m_leftRightSwap)
    {
        m_currentRasterizerDesc.FrontCounterClockwise =
            !m_currentRasterizerDesc.FrontCounterClockwise;
        m_rasterizerStateDirty = true;
    }

    ID3D11RasterizerState* pRState = NULL;
    if (m_rasterizerStateDirty)
    {
        HRESULT hr = m_pDevice->CreateRasterizerState(
            &m_currentRasterizerDesc,
            &pRState);

        if (FAILED(hr) || pRState == NULL)
        {
            if (FAILED(hr))
            {
                D3D11Error::ReportError(D3D11Error::D3D11ERROR_RASTERIZER_STATE_CREATION_FAILED,
                    "Error HRESULT = 0x%08X.", 
                    (efd::UInt32)hr);
            }
            else
            {
                D3D11Error::ReportError(D3D11Error::D3D11ERROR_RASTERIZER_STATE_CREATION_FAILED,
                    "No error message from D3D11, but rasterizer state is NULL.");
            }
        }
        m_rasterizerStateDirty = false;
    }

    if (pRState == NULL)
    {
        m_pDeviceState->RSSetState(m_pDefaultRasterizerState);
    }
    else
    {
        m_pDeviceState->RSSetState(pRState);
        pRState->Release();
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::ApplyCurrentSamplers(
    const D3DX11_STATE_BLOCK_MASK* pMask)
{
    // Don't set the samplers for null shaders.
    if (m_pDeviceState->VSGetShader() != NULL)
    {
        ApplyCurrentSamplers(
            NiGPUProgram::PROGRAM_VERTEX,
            0,
            D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
            pMask);
    }
    if (m_pDeviceState->HSGetShader() != NULL)
    {
        ApplyCurrentSamplers(
            NiGPUProgram::PROGRAM_HULL,
            0,
            D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
            pMask);
    }
    if (m_pDeviceState->DSGetShader() != NULL)
    {
        ApplyCurrentSamplers(
            NiGPUProgram::PROGRAM_DOMAIN,
            0,
            D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
            pMask);
    }
    if (m_pDeviceState->GSGetShader() != NULL)
    {
        ApplyCurrentSamplers(
            NiGPUProgram::PROGRAM_GEOMETRY,
            0,
            D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
            pMask);
    }
    if (m_pDeviceState->PSGetShader() != NULL)
    {
        ApplyCurrentSamplers(
            NiGPUProgram::PROGRAM_PIXEL,
            0,
            D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
            pMask);
    }
    if (m_pDeviceState->CSGetShader() != NULL)
    {
        ApplyCurrentSamplers(
            NiGPUProgram::PROGRAM_COMPUTE,
            0,
            D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
            pMask);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateManager::ApplyCurrentSamplers(
    NiGPUProgram::ProgramType shaderType,
    efd::UInt32 samplerStart,
    efd::UInt32 samplerCount,
    const D3DX11_STATE_BLOCK_MASK* pMask)
{
    if (samplerStart >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)
        return;

    if (samplerCount > D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - samplerStart)
        samplerCount = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - samplerStart;

    // Create bit array of which samplers to set
    efd::UInt16 samplersToSet = 0;
    EE_ASSERT(sizeof(samplersToSet) >= ((D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT + 7) / 8));
    if (pMask)
    {
        const efd::UInt8* pSamplerArrayMask = NULL;
        if (shaderType == NiGPUProgram::PROGRAM_VERTEX)
            pSamplerArrayMask = pMask->VSSamplers;
        else if (shaderType == NiGPUProgram::PROGRAM_HULL)
            pSamplerArrayMask = pMask->HSSamplers;
        else if (shaderType == NiGPUProgram::PROGRAM_DOMAIN)
            pSamplerArrayMask = pMask->DSSamplers;
        else if (shaderType == NiGPUProgram::PROGRAM_GEOMETRY)
            pSamplerArrayMask = pMask->GSSamplers;
        else if (shaderType == NiGPUProgram::PROGRAM_PIXEL)
            pSamplerArrayMask = pMask->PSSamplers;
        else if (shaderType == NiGPUProgram::PROGRAM_COMPUTE)
            pSamplerArrayMask = pMask->CSSamplers;
        EE_ASSERT(pSamplerArrayMask);

        samplersToSet = pSamplerArrayMask[0] |
            pSamplerArrayMask[1] << 8;
    }
    else
    {
        samplersToSet = (efd::UInt16) ~0;
    }

    // Mask out sampler range indicated by samplerStart and samplerCount
    efd::UInt16 uiMask = static_cast<efd::UInt16>(((1 << samplerCount) - 1) << samplerStart);
    samplersToSet &= uiMask;

    D3D11_SAMPLER_DESC* pVertexSamplerDescs = m_currentSamplerDescArray[shaderType];

    ID3D11SamplerState* samplerArray[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    memset(samplerArray, 0, sizeof(samplerArray));

    // Loop one additional time so the sampler block will be set if the
    // last sampler needs to be set.
    efd::Bool buildingSamplerBlock = false;
    efd::UInt32 blockSamplerStart = 0;
    efd::UInt32 blockSamplerCount = 0;
    for (efd::UInt32 i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT + 1; i++)
    {
        efd::Bool setCurrentSampler =
            (i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT) &&
            ((samplersToSet & (1 << i)) != 0);

        if (setCurrentSampler == false)
        {
            if (buildingSamplerBlock)
            {
                // If we're building a block of samplers to set, and this
                // sampler doesn't need to be set, then set the current block
                // and move on.

                if (shaderType == NiGPUProgram::PROGRAM_VERTEX)
                {
                    m_pDeviceState->VSSetSamplers(
                        blockSamplerStart,
                        blockSamplerCount,
                        samplerArray);
                }
                else if (shaderType == NiGPUProgram::PROGRAM_HULL)
                {
                    m_pDeviceState->HSSetSamplers(
                        blockSamplerStart,
                        blockSamplerCount,
                        samplerArray);
                }
                else if (shaderType == NiGPUProgram::PROGRAM_DOMAIN)
                {
                    m_pDeviceState->DSSetSamplers(
                        blockSamplerStart,
                        blockSamplerCount,
                        samplerArray);
                }
                else if (shaderType == NiGPUProgram::PROGRAM_GEOMETRY)
                {
                    m_pDeviceState->GSSetSamplers(
                        blockSamplerStart,
                        blockSamplerCount,
                        samplerArray);
                }
                else if (shaderType == NiGPUProgram::PROGRAM_PIXEL)
                {
                    m_pDeviceState->PSSetSamplers(
                        blockSamplerStart,
                        blockSamplerCount,
                        samplerArray);
                }
                else if (shaderType == NiGPUProgram::PROGRAM_COMPUTE)
                {
                    m_pDeviceState->CSSetSamplers(
                        blockSamplerStart,
                        blockSamplerCount,
                        samplerArray);
                }

                // Release samplers, some of which may have been newly created.
                // The D3D11DeviceState will keep references to the samplers.
                for (efd::UInt32 j = 0; j < blockSamplerCount; j++)
                {
                    if (samplerArray[j])
                        samplerArray[j]->Release();
                }

                buildingSamplerBlock = false;
            }
            else
            {
                // If we're not building a block of samplers to set, and this
                // sampler doesn't need to be set, then just move on.
            }
            continue;
        }
        else
        {
            if (buildingSamplerBlock == false)
            {
                // If we're not building a block of samplers to set, and this
                // sampler does need to be set, then start a new block and add
                // this one.
                buildingSamplerBlock = true;
                blockSamplerStart = i;
                blockSamplerCount = 0;
            }
            else
            {
                // If we're building a block of samplers to set, and this
                // sampler does need to be set, then just add this one.
            }

            // Add current sampler to the block
            ID3D11SamplerState* pSampler = NULL;
            if (m_samplersDirtyArray[shaderType][i])
            {
                HRESULT hr = m_pDevice->CreateSamplerState(
                    &pVertexSamplerDescs[i],
                    &pSampler);

                if (FAILED(hr) || pSampler == NULL)
                {
                    if (FAILED(hr))
                    {
                        D3D11Error::ReportError(D3D11Error::D3D11ERROR_SAMPLER_CREATION_FAILED,
                            "Error HRESULT = 0x%08X.", 
                            (efd::UInt32)hr);
                    }
                    else
                    {
                        D3D11Error::ReportError(D3D11Error::D3D11ERROR_SAMPLER_CREATION_FAILED,
                            "No error message from D3D11, but sampler is NULL.");
                    }
                }
                m_samplersDirtyArray[shaderType][i] = false;
            }
            else
            {
                pSampler = m_pDefaultSamplerState;
                // We will be calling Release after setting the samplers, so
                // call AddRef now.
                if (pSampler)
                    pSampler->AddRef();
            }
            samplerArray[blockSamplerCount++] = pSampler;
        }
    }

    // The last block should have been set in the extra loop, so a block
    // should not be building now.
    EE_ASSERT(buildingSamplerBlock == false);
}

//------------------------------------------------------------------------------------------------
D3D11_BLEND D3D11RenderStateManager::ConvertGbBlendToD3D11Blend(
    NiAlphaProperty::AlphaFunction alphaFunction)
{
    switch (alphaFunction)
    {
    case NiAlphaProperty::ALPHA_ONE:
        return D3D11_BLEND_ONE;
    case NiAlphaProperty::ALPHA_ZERO:
        return D3D11_BLEND_ZERO;
    case NiAlphaProperty::ALPHA_SRCCOLOR:
        return D3D11_BLEND_SRC_COLOR;
    case NiAlphaProperty::ALPHA_INVSRCCOLOR:
        return D3D11_BLEND_INV_SRC_COLOR;
    case NiAlphaProperty::ALPHA_DESTCOLOR:
        return D3D11_BLEND_DEST_COLOR;
    case NiAlphaProperty::ALPHA_INVDESTCOLOR:
        return D3D11_BLEND_INV_DEST_COLOR;
    case NiAlphaProperty::ALPHA_SRCALPHA:
        return D3D11_BLEND_SRC_ALPHA;
    case NiAlphaProperty::ALPHA_INVSRCALPHA:
        return D3D11_BLEND_INV_SRC_ALPHA;
    case NiAlphaProperty::ALPHA_DESTALPHA:
        return D3D11_BLEND_DEST_ALPHA;
    case NiAlphaProperty::ALPHA_INVDESTALPHA:
        return D3D11_BLEND_INV_DEST_ALPHA;
    case NiAlphaProperty::ALPHA_SRCALPHASAT:
        return D3D11_BLEND_SRC_ALPHA_SAT;
    }
    D3D11Error::ReportWarning("Invalid alpha function passed into %s", __FUNCTION__);
    return D3D11_BLEND_ZERO;
}

//------------------------------------------------------------------------------------------------
D3D11_COMPARISON_FUNC D3D11RenderStateManager::ConvertGbStencilFuncToD3D11Comparison(
    NiStencilProperty::TestFunc testFunction)
{
    switch (testFunction)
    {
    case NiStencilProperty::TEST_NEVER:
        return D3D11_COMPARISON_NEVER;
    case NiStencilProperty::TEST_LESS:
        return D3D11_COMPARISON_LESS;
    case NiStencilProperty::TEST_EQUAL:
        return D3D11_COMPARISON_EQUAL;
    case NiStencilProperty::TEST_LESSEQUAL:
        return D3D11_COMPARISON_LESS_EQUAL;
    case NiStencilProperty::TEST_GREATER:
        return D3D11_COMPARISON_GREATER;
    case NiStencilProperty::TEST_NOTEQUAL:
        return D3D11_COMPARISON_NOT_EQUAL;
    case NiStencilProperty::TEST_GREATEREQUAL:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case NiStencilProperty::TEST_ALWAYS:
        return D3D11_COMPARISON_ALWAYS;
    }
    D3D11Error::ReportWarning("Invalid stencil function passed into %s", __FUNCTION__);
    return D3D11_COMPARISON_NEVER;
}

//------------------------------------------------------------------------------------------------
D3D11_STENCIL_OP D3D11RenderStateManager::ConvertGbStencilActionToD3D11StencilOp(
    NiStencilProperty::Action action)
{
    switch (action)
    {
    case NiStencilProperty::ACTION_KEEP:
        return D3D11_STENCIL_OP_KEEP;
    case NiStencilProperty::ACTION_ZERO:
        return D3D11_STENCIL_OP_ZERO;
    case NiStencilProperty::ACTION_REPLACE:
        return D3D11_STENCIL_OP_REPLACE;
    case NiStencilProperty::ACTION_INCREMENT:
        return D3D11_STENCIL_OP_INCR_SAT;
    case NiStencilProperty::ACTION_DECREMENT:
        return D3D11_STENCIL_OP_DECR_SAT;
    case NiStencilProperty::ACTION_INVERT:
        return D3D11_STENCIL_OP_INVERT;
    }
    D3D11Error::ReportWarning("Invalid stencil action passed into %s", __FUNCTION__);
    return D3D11_STENCIL_OP_KEEP;
}

//------------------------------------------------------------------------------------------------
D3D11_COMPARISON_FUNC D3D11RenderStateManager::ConvertGbDepthFuncToD3D11Comparison(
    NiZBufferProperty::TestFunction testFunction)
{
    switch (testFunction)
    {
    case NiZBufferProperty::TEST_ALWAYS:
        return D3D11_COMPARISON_ALWAYS;
    case NiZBufferProperty::TEST_LESS:
        return D3D11_COMPARISON_LESS;
    case NiZBufferProperty::TEST_EQUAL:
        return D3D11_COMPARISON_EQUAL;
    case NiZBufferProperty::TEST_LESSEQUAL:
        return D3D11_COMPARISON_LESS_EQUAL;
    case NiZBufferProperty::TEST_GREATER:
        return D3D11_COMPARISON_GREATER;
    case NiZBufferProperty::TEST_NOTEQUAL:
        return D3D11_COMPARISON_NOT_EQUAL;
    case NiZBufferProperty::TEST_GREATEREQUAL:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case NiZBufferProperty::TEST_NEVER:
        return D3D11_COMPARISON_NEVER;
    }
    D3D11Error::ReportWarning("Invalid Z buffer test function passed into %s", __FUNCTION__);
    return D3D11_COMPARISON_NEVER;
}

//------------------------------------------------------------------------------------------------
D3D11_FILTER D3D11RenderStateManager::ConvertGbFilterModeToD3D11Filter(
    NiTexturingProperty::FilterMode filterMode)
{
    switch (filterMode)
    {
    case NiTexturingProperty::FILTER_NEAREST:
    case NiTexturingProperty::FILTER_NEAREST_MIPNEAREST:
        return D3D11_FILTER_MIN_MAG_MIP_POINT;
    case NiTexturingProperty::FILTER_BILERP:
    case NiTexturingProperty::FILTER_BILERP_MIPNEAREST:
        return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    case NiTexturingProperty::FILTER_TRILERP:
        return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    case NiTexturingProperty::FILTER_NEAREST_MIPLERP:
        return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
    case NiTexturingProperty::FILTER_ANISOTROPIC:
         return D3D11_FILTER_ANISOTROPIC;
    }
    D3D11Error::ReportWarning("Invalid filter mode passed into %s", __FUNCTION__);
    return D3D11_FILTER_MIN_MAG_MIP_POINT;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateManager::ConvertGbFilterModeToMipmapEnable(
    NiTexturingProperty::FilterMode filterMode)
{
    switch (filterMode)
    {
    case NiTexturingProperty::FILTER_NEAREST:
    case NiTexturingProperty::FILTER_BILERP:
        return false;
    case NiTexturingProperty::FILTER_NEAREST_MIPNEAREST:
    case NiTexturingProperty::FILTER_BILERP_MIPNEAREST:
    case NiTexturingProperty::FILTER_TRILERP:
    case NiTexturingProperty::FILTER_NEAREST_MIPLERP:
    case NiTexturingProperty::FILTER_ANISOTROPIC:
        return true;
    }
    D3D11Error::ReportWarning("Invalid filter mode passed into %s", __FUNCTION__);
    return true;
}

//------------------------------------------------------------------------------------------------
D3D11_TEXTURE_ADDRESS_MODE D3D11RenderStateManager::ConvertGbClampModeToD3D11AddressU(
    NiTexturingProperty::ClampMode clampMode)
{
    switch (clampMode)
    {
    case NiTexturingProperty::CLAMP_S_CLAMP_T:
    case NiTexturingProperty::CLAMP_S_WRAP_T:
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    case NiTexturingProperty::WRAP_S_CLAMP_T:
    case NiTexturingProperty::WRAP_S_WRAP_T:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    }
    D3D11Error::ReportWarning("Invalid address mode passed into %s", __FUNCTION__);
    return D3D11_TEXTURE_ADDRESS_CLAMP;
}

//------------------------------------------------------------------------------------------------
D3D11_TEXTURE_ADDRESS_MODE D3D11RenderStateManager::ConvertGbClampModeToD3D11AddressV(
    NiTexturingProperty::ClampMode clampMode)
{
    switch (clampMode)
    {
    case NiTexturingProperty::CLAMP_S_CLAMP_T:
    case NiTexturingProperty::WRAP_S_CLAMP_T:
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    case NiTexturingProperty::CLAMP_S_WRAP_T:
    case NiTexturingProperty::WRAP_S_WRAP_T:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    }
    D3D11Error::ReportWarning("Invalid address mode passed into %s", __FUNCTION__);
    return D3D11_TEXTURE_ADDRESS_CLAMP;
}

//------------------------------------------------------------------------------------------------
