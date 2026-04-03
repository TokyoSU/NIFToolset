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

#include "D3D11RenderStateGroup.h"
#include "D3D11RenderStateManager.h"

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11RenderStateGroup::D3D11RenderStateGroup() :
    m_pSamplers(NULL),
    m_blendValidFlags(0),
    m_depthStencilValidFlags(0),
    m_rasterizerValidFlags(0),
    m_blendFactorValid(false),
    m_sampleMaskValid(false),
    m_stencilRefValid(false),
    m_resetCount(0)
{
    memset(m_renderTargetValidFlags, 0, sizeof(m_renderTargetValidFlags));
    ResetRenderStates();
}

//------------------------------------------------------------------------------------------------
D3D11RenderStateGroup::~D3D11RenderStateGroup()
{
    EE_DELETE m_pSamplers;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBSAlphaToCoverageEnable(
    efd::Bool alphaToCoverageEnable)
{
    m_blendDesc.AlphaToCoverageEnable =alphaToCoverageEnable;
    m_blendValidFlags |= D3D11RenderStateManager::BSVALID_ALPHATOCOVERAGEENABLE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBSAlphaToCoverageEnable(
    efd::Bool& alphaToCoverageEnable) const
{
    if ((m_blendValidFlags & D3D11RenderStateManager::BSVALID_ALPHATOCOVERAGEENABLE) != 0)
    {
        alphaToCoverageEnable = (m_blendDesc.AlphaToCoverageEnable != 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBSAlphaToCoverageEnable()
{
    m_blendValidFlags &= ~D3D11RenderStateManager::BSVALID_ALPHATOCOVERAGEENABLE;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBSIndependentBlendEnable(
    efd::Bool independentBlendEnable)
{
    m_blendDesc.IndependentBlendEnable =independentBlendEnable;
    m_blendValidFlags |= D3D11RenderStateManager::BSVALID_INDEPENDENTBLENDENABLE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBSIndependentBlendEnable(
    efd::Bool& independentBlendEnable) const
{
    if ((m_blendValidFlags & D3D11RenderStateManager::BSVALID_INDEPENDENTBLENDENABLE) != 0)
    {
        independentBlendEnable = (m_blendDesc.IndependentBlendEnable != 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBSIndependentBlendEnable()
{
    m_blendValidFlags &= ~D3D11RenderStateManager::BSVALID_INDEPENDENTBLENDENABLE;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBSBlendEnable(efd::UInt32 renderTarget, efd::Bool blendEnable)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_blendDesc.RenderTarget[renderTarget].BlendEnable = blendEnable;
        m_renderTargetValidFlags[renderTarget] |= D3D11RenderStateManager::BSRTVALID_BLENDENABLE;

        m_blendValidFlags |= (D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBSBlendEnable(
    efd::UInt32 renderTarget, 
    efd::Bool& blendEnable) const
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT &&
        (m_renderTargetValidFlags[renderTarget] & D3D11RenderStateManager::BSRTVALID_BLENDENABLE)
        != 0)
    {
        blendEnable = (m_blendDesc.RenderTarget[renderTarget].BlendEnable != 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBSBlendEnable(efd::UInt32 renderTarget)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_renderTargetValidFlags[renderTarget] &= ~(D3D11RenderStateManager::BSRTVALID_BLENDENABLE);
        if (m_renderTargetValidFlags[renderTarget] == 0)
            m_blendValidFlags &= ~(D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBSSrcBlend(efd::UInt32 renderTarget, D3D11_BLEND blend)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_blendDesc.RenderTarget[renderTarget].SrcBlend = blend;
        m_renderTargetValidFlags[renderTarget] |= D3D11RenderStateManager::BSRTVALID_SRCBLEND;

        m_blendValidFlags |= (D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBSSrcBlend(efd::UInt32 renderTarget, D3D11_BLEND& blend) const
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT &&
        (m_renderTargetValidFlags[renderTarget] & D3D11RenderStateManager::BSRTVALID_SRCBLEND) 
        != 0)
    {
        blend = m_blendDesc.RenderTarget[renderTarget].SrcBlend;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBSSrcBlend(efd::UInt32 renderTarget)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_renderTargetValidFlags[renderTarget] &= ~D3D11RenderStateManager::BSRTVALID_SRCBLEND;
        if (m_renderTargetValidFlags[renderTarget] == 0)
            m_blendValidFlags &= ~(D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBSDestBlend(efd::UInt32 renderTarget, D3D11_BLEND blend)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_blendDesc.RenderTarget[renderTarget].DestBlend = blend;
        m_renderTargetValidFlags[renderTarget] |= D3D11RenderStateManager::BSRTVALID_DESTBLEND;

        m_blendValidFlags |= (D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBSDestBlend(efd::UInt32 renderTarget, D3D11_BLEND& blend) const
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT &&
        (m_renderTargetValidFlags[renderTarget] & D3D11RenderStateManager::BSRTVALID_DESTBLEND) 
        != 0)
    {
        blend = m_blendDesc.RenderTarget[renderTarget].DestBlend;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBSDestBlend(efd::UInt32 renderTarget)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_renderTargetValidFlags[renderTarget] &= ~D3D11RenderStateManager::BSRTVALID_DESTBLEND;
        if (m_renderTargetValidFlags[renderTarget] == 0)
            m_blendValidFlags &= ~(D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBSBlendOp(efd::UInt32 renderTarget, D3D11_BLEND_OP blendOp)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_blendDesc.RenderTarget[renderTarget].BlendOp = blendOp;
        m_renderTargetValidFlags[renderTarget] |= D3D11RenderStateManager::BSRTVALID_BLENDOP;

        m_blendValidFlags |= (D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBSBlendOp(
    efd::UInt32 renderTarget, 
    D3D11_BLEND_OP& blendOp) const
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT &&
        (m_renderTargetValidFlags[renderTarget] & D3D11RenderStateManager::BSRTVALID_BLENDOP) != 0)
    {
        blendOp = m_blendDesc.RenderTarget[renderTarget].BlendOp;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBSBlendOp(efd::UInt32 renderTarget)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_renderTargetValidFlags[renderTarget] &= ~D3D11RenderStateManager::BSRTVALID_BLENDOP;
        if (m_renderTargetValidFlags[renderTarget] == 0)
            m_blendValidFlags &= ~(D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBSSrcBlendAlpha(efd::UInt32 renderTarget, D3D11_BLEND blend)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_blendDesc.RenderTarget[renderTarget].SrcBlendAlpha = blend;
        m_renderTargetValidFlags[renderTarget] |= D3D11RenderStateManager::BSRTVALID_SRCBLENDALPHA;

        m_blendValidFlags |= (D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBSSrcBlendAlpha(
    efd::UInt32 renderTarget, 
    D3D11_BLEND& blend) const
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT &&
        (m_renderTargetValidFlags[renderTarget] & D3D11RenderStateManager::BSRTVALID_SRCBLENDALPHA) 
        != 0)
    {
        blend = m_blendDesc.RenderTarget[renderTarget].SrcBlendAlpha;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBSSrcBlendAlpha(efd::UInt32 renderTarget)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_renderTargetValidFlags[renderTarget] &= ~D3D11RenderStateManager::BSRTVALID_SRCBLENDALPHA;
        if (m_renderTargetValidFlags[renderTarget] == 0)
            m_blendValidFlags &= ~(D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBSDestBlendAlpha(efd::UInt32 renderTarget, D3D11_BLEND blend)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_blendDesc.RenderTarget[renderTarget].DestBlendAlpha = blend;
        m_renderTargetValidFlags[renderTarget] |= D3D11RenderStateManager::BSRTVALID_DESTBLENDALPHA;

        m_blendValidFlags |= (D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBSDestBlendAlpha(
    efd::UInt32 renderTarget, 
    D3D11_BLEND& blend) const
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT &&
        (m_renderTargetValidFlags[renderTarget] & D3D11RenderStateManager::BSRTVALID_DESTBLENDALPHA) 
        != 0)
    {
        blend = m_blendDesc.RenderTarget[renderTarget].DestBlendAlpha;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBSDestBlendAlpha(efd::UInt32 renderTarget)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_renderTargetValidFlags[renderTarget] &= ~D3D11RenderStateManager::BSRTVALID_DESTBLENDALPHA;
        if (m_renderTargetValidFlags[renderTarget] == 0)
            m_blendValidFlags &= ~(D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBSBlendOpAlpha(efd::UInt32 renderTarget, D3D11_BLEND_OP blendOp)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_blendDesc.RenderTarget[renderTarget].BlendOpAlpha = blendOp;
        m_renderTargetValidFlags[renderTarget] |= D3D11RenderStateManager::BSRTVALID_BLENDOPALPHA;

        m_blendValidFlags |= (D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBSBlendOpAlpha(
    efd::UInt32 renderTarget, 
    D3D11_BLEND_OP& blendOp) const
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT &&
        (m_renderTargetValidFlags[renderTarget] & D3D11RenderStateManager::BSRTVALID_BLENDOPALPHA) 
        != 0)
    {
        blendOp = m_blendDesc.RenderTarget[renderTarget].BlendOpAlpha;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBSBlendOpAlpha(efd::UInt32 renderTarget)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_renderTargetValidFlags[renderTarget] &= ~D3D11RenderStateManager::BSRTVALID_BLENDOPALPHA;
        if (m_renderTargetValidFlags[renderTarget] == 0)
            m_blendValidFlags &= ~(D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBSRenderTargetWriteMask(
    efd::UInt32 renderTarget, 
    efd::UInt8 writeMask)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_blendDesc.RenderTarget[renderTarget].RenderTargetWriteMask = writeMask;
        m_renderTargetValidFlags[renderTarget] |= 
            D3D11RenderStateManager::BSRTVALID_RENDERTARGETWRITEMASK;

        m_blendValidFlags |= (D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBSRenderTargetWriteMask(
    efd::UInt32 renderTarget, 
    efd::UInt8& writeMask) const
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT &&
        (m_renderTargetValidFlags[renderTarget] & 
        D3D11RenderStateManager::BSRTVALID_RENDERTARGETWRITEMASK) != 0)
    {
        writeMask = m_blendDesc.RenderTarget[renderTarget].RenderTargetWriteMask;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBSRenderTargetWriteMask(efd::UInt32 renderTarget)
{
    if (renderTarget < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        m_renderTargetValidFlags[renderTarget] &= 
            ~D3D11RenderStateManager::BSRTVALID_RENDERTARGETWRITEMASK;
        if (m_renderTargetValidFlags[renderTarget] == 0)
            m_blendValidFlags &= ~(D3D11RenderStateManager::BSVALID_RENDERTARGET_0 << renderTarget);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBlendFactor(const efd::Float32 blendFactor[4])
{
    for (efd::UInt32 i = 0; i < 4; i++)
        m_blendFactor[i] = blendFactor[i];
    m_blendFactorValid = true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetBlendFactor(efd::Float32 blendFactor[4]) const
{
    if (m_blendFactorValid)
    {
        for (efd::UInt32 i = 0; i < 4; i++)
            blendFactor[i] = m_blendFactor[i];
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveBlendFactor()
{
    m_blendFactorValid = false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetSampleMask(efd::UInt32 sampleMask)
{
    m_sampleMask = sampleMask;
    m_sampleMaskValid = true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetSampleMask(efd::UInt32& sampleMask) const
{
    if (m_sampleMaskValid)
    {
        sampleMask = m_sampleMask;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveSampleMask()
{
    m_sampleMaskValid = false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSDepthEnable(efd::Bool depthEnable)
{
    m_depthStencilDesc.DepthEnable = depthEnable;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_DEPTHENABLE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSDepthEnable(efd::Bool& depthEnable) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_DEPTHENABLE) != 0)
    {
        depthEnable = (m_depthStencilDesc.DepthEnable != 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSDepthEnable()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_DEPTHENABLE;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSDepthWriteMask(D3D11_DEPTH_WRITE_MASK depthWriteMask)
{
    m_depthStencilDesc.DepthWriteMask = depthWriteMask;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_DEPTHWRITEMASK;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSDepthWriteMask(D3D11_DEPTH_WRITE_MASK& depthWriteMask) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_DEPTHWRITEMASK) != 0)
    {
        depthWriteMask = m_depthStencilDesc.DepthWriteMask;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSDepthWriteMask()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_DEPTHWRITEMASK;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSDepthFunc(D3D11_COMPARISON_FUNC depthFunc)
{
    m_depthStencilDesc.DepthFunc = depthFunc;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_DEPTHFUNC;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSDepthFunc(D3D11_COMPARISON_FUNC& depthFunc) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_DEPTHFUNC) != 0)
    {
        depthFunc = m_depthStencilDesc.DepthFunc;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSDepthFunc()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_DEPTHFUNC;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSStencilEnable(efd::Bool stencilEnable)
{
    m_depthStencilDesc.StencilEnable = stencilEnable;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_STENCILENABLE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSStencilEnable(efd::Bool& stencilEnable) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_STENCILENABLE) != 0)
    {
        stencilEnable = (m_depthStencilDesc.StencilEnable != 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSStencilEnable()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_STENCILENABLE;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSStencilReadMask(efd::UInt8 stencilReadMask)
{
    m_depthStencilDesc.StencilReadMask = stencilReadMask;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_STENCILREADMASK;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSStencilReadMask(efd::UInt8& stencilReadMask) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_STENCILREADMASK) != 0)
    {
        stencilReadMask = m_depthStencilDesc.StencilReadMask;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSStencilReadMask()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_STENCILREADMASK;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSStencilWriteMask(efd::UInt8 stencilWriteMask)
{
    m_depthStencilDesc.StencilWriteMask = stencilWriteMask;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_STENCILWRITEMASK;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSStencilWriteMask(efd::UInt8& stencilWriteMask) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_STENCILWRITEMASK) != 0)
    {
        stencilWriteMask = m_depthStencilDesc.StencilWriteMask;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSStencilWriteMask()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_STENCILWRITEMASK;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSFrontFaceStencilFailOp(D3D11_STENCIL_OP stencilOp)
{
    m_depthStencilDesc.FrontFace.StencilFailOp = stencilOp;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILFAILOP;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSFrontFaceStencilFailOp(D3D11_STENCIL_OP& stencilOp) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILFAILOP) != 0)
    {
        stencilOp = m_depthStencilDesc.FrontFace.StencilFailOp;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSFrontFaceStencilFailOp()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILFAILOP;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSFrontFaceStencilDepthFailOp(D3D11_STENCIL_OP stencilOp)
{
    m_depthStencilDesc.FrontFace.StencilDepthFailOp = stencilOp;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILDEPTHFAILOP;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSFrontFaceStencilDepthFailOp(
    D3D11_STENCIL_OP& stencilOp) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILDEPTHFAILOP) 
        != 0)
    {
        stencilOp = m_depthStencilDesc.FrontFace.StencilDepthFailOp;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSFrontFaceStencilDepthFailOp()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILDEPTHFAILOP;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSFrontFaceStencilPassOp(
    D3D11_STENCIL_OP stencilOp)
{
    m_depthStencilDesc.FrontFace.StencilPassOp = stencilOp;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILPASSOP;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSFrontFaceStencilPassOp(D3D11_STENCIL_OP& stencilOp) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILPASSOP) != 0)
    {
        stencilOp = m_depthStencilDesc.FrontFace.StencilPassOp;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSFrontFaceStencilPassOp()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILPASSOP;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSFrontFaceStencilFunc(
    D3D11_COMPARISON_FUNC stencilFunc)
{
    m_depthStencilDesc.FrontFace.StencilFunc = stencilFunc;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILFUNC;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSFrontFaceStencilFunc(
    D3D11_COMPARISON_FUNC& stencilFunc) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILFUNC) != 0)
    {
        stencilFunc = m_depthStencilDesc.FrontFace.StencilFunc;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSFrontFaceStencilFunc()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_FRONTFACE_STENCILFUNC;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSBackFaceStencilFailOp(D3D11_STENCIL_OP stencilOp)
{
    m_depthStencilDesc.BackFace.StencilFailOp = stencilOp;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILFAILOP;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSBackFaceStencilFailOp(D3D11_STENCIL_OP& stencilOp) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILFAILOP) != 0)
    {
        stencilOp = m_depthStencilDesc.BackFace.StencilFailOp;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSBackFaceStencilFailOp()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILFAILOP;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSBackFaceStencilDepthFailOp(D3D11_STENCIL_OP stencilOp)
{
    m_depthStencilDesc.BackFace.StencilDepthFailOp = stencilOp;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILDEPTHFAILOP;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSBackFaceStencilDepthFailOp(D3D11_STENCIL_OP& stencilOp) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILDEPTHFAILOP) 
        != 0)
    {
        stencilOp = m_depthStencilDesc.BackFace.StencilDepthFailOp;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSBackFaceStencilDepthFailOp()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILDEPTHFAILOP;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSBackFaceStencilPassOp(D3D11_STENCIL_OP stencilOp)
{
    m_depthStencilDesc.BackFace.StencilPassOp = stencilOp;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILPASSOP;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSBackFaceStencilPassOp(D3D11_STENCIL_OP& stencilOp) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILPASSOP) != 0)
    {
        stencilOp = m_depthStencilDesc.BackFace.StencilPassOp;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSBackFaceStencilPassOp()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILPASSOP;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDSSBackFaceStencilFunc(D3D11_COMPARISON_FUNC stencilFunc)
{
    m_depthStencilDesc.BackFace.StencilFunc = stencilFunc;
    m_depthStencilValidFlags |= D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILFUNC;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetDSSBackFaceStencilFunc(D3D11_COMPARISON_FUNC& stencilFunc) const
{
    if ((m_depthStencilValidFlags & D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILFUNC) != 0)
    {
        stencilFunc = m_depthStencilDesc.BackFace.StencilFunc;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveDSSBackFaceStencilFunc()
{
    m_depthStencilValidFlags &= ~D3D11RenderStateManager::DSSVALID_BACKFACE_STENCILFUNC;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetStencilRef(efd::UInt32 stencilRef)
{
    m_stencilRef = stencilRef;
    m_stencilRefValid = true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetStencilRef(efd::UInt32& stencilRef) const
{
    if (m_stencilRefValid)
    {
        stencilRef = m_stencilRef;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveStencilRef()
{
    m_stencilRefValid = false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRSFillMode(D3D11_FILL_MODE fillMode)
{
    m_rasterizerDesc.FillMode = fillMode;
    m_rasterizerValidFlags |= D3D11RenderStateManager::RSVALID_FILLMODE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetRSFillMode(D3D11_FILL_MODE& fillMode) const
{
    if ((m_rasterizerValidFlags & D3D11RenderStateManager::RSVALID_FILLMODE) != 0)
    {
        fillMode = m_rasterizerDesc.FillMode;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveRSFillMode()
{
    m_rasterizerValidFlags &= ~D3D11RenderStateManager::RSVALID_FILLMODE;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRSCullMode(D3D11_CULL_MODE cullMode)
{
    m_rasterizerDesc.CullMode = cullMode;
    m_rasterizerValidFlags |= D3D11RenderStateManager::RSVALID_CULLMODE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetRSCullMode(D3D11_CULL_MODE& cullMode) const
{
    if ((m_rasterizerValidFlags &
        D3D11RenderStateManager::RSVALID_CULLMODE) != 0)
    {
        cullMode = m_rasterizerDesc.CullMode;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveRSCullMode()
{
    m_rasterizerValidFlags &= ~D3D11RenderStateManager::RSVALID_CULLMODE;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRSFrontCounterClockwise(efd::Bool frontCounterClockwise)
{
    m_rasterizerDesc.FrontCounterClockwise = frontCounterClockwise;
    m_rasterizerValidFlags |= D3D11RenderStateManager::RSVALID_FRONTCOUNTERCLOCKWISE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetRSFrontCounterClockwise(efd::Bool& frontCounterClockwise) const
{
    if ((m_rasterizerValidFlags & D3D11RenderStateManager::RSVALID_FRONTCOUNTERCLOCKWISE) != 0)
    {
        frontCounterClockwise = (m_rasterizerDesc.FrontCounterClockwise != 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveRSFrontCounterClockwise()
{
    m_rasterizerValidFlags &= ~D3D11RenderStateManager::RSVALID_FRONTCOUNTERCLOCKWISE;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRSDepthBias(efd::SInt32 depthBias)
{
    m_rasterizerDesc.DepthBias = depthBias;
    m_rasterizerValidFlags |= D3D11RenderStateManager::RSVALID_DEPTHBIAS;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetRSDepthBias(efd::SInt32& depthBias) const
{
    if ((m_rasterizerValidFlags & D3D11RenderStateManager::RSVALID_DEPTHBIAS) != 0)
    {
        depthBias = m_rasterizerDesc.DepthBias;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveRSDepthBias()
{
    m_rasterizerValidFlags &= ~D3D11RenderStateManager::RSVALID_DEPTHBIAS;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRSDepthBiasClamp(efd::Float32 depthBiasClamp)
{
    m_rasterizerDesc.DepthBiasClamp = depthBiasClamp;
    m_rasterizerValidFlags |= D3D11RenderStateManager::RSVALID_DEPTHBIASCLAMP;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetRSDepthBiasClamp(efd::Float32& depthBiasClamp) const
{
    if ((m_rasterizerValidFlags & D3D11RenderStateManager::RSVALID_DEPTHBIASCLAMP) != 0)
    {
        depthBiasClamp = m_rasterizerDesc.DepthBiasClamp;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveRSDepthBiasClamp()
{
    m_rasterizerValidFlags &= ~D3D11RenderStateManager::RSVALID_DEPTHBIASCLAMP;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRSSlopeScaledDepthBias(efd::Float32 slopeScaledDepthBias)
{
    m_rasterizerDesc.SlopeScaledDepthBias = slopeScaledDepthBias;
    m_rasterizerValidFlags |= D3D11RenderStateManager::RSVALID_SLOPESCALEDDEPTHBIAS;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetRSSlopeScaledDepthBias(efd::Float32& slopeScaledDepthBias) const
{
    if ((m_rasterizerValidFlags & D3D11RenderStateManager::RSVALID_SLOPESCALEDDEPTHBIAS) != 0)
    {
        slopeScaledDepthBias = m_rasterizerDesc.SlopeScaledDepthBias;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveRSSlopeScaledDepthBias()
{
    m_rasterizerValidFlags &= ~D3D11RenderStateManager::RSVALID_SLOPESCALEDDEPTHBIAS;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRSDepthClipEnable(efd::Bool depthClipEnable)
{
    m_rasterizerDesc.DepthClipEnable = depthClipEnable;
    m_rasterizerValidFlags |= D3D11RenderStateManager::RSVALID_DEPTHCLIPENABLE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetRSDepthClipEnable(efd::Bool& depthClipEnable)
    const
{
    if ((m_rasterizerValidFlags & D3D11RenderStateManager::RSVALID_DEPTHCLIPENABLE) != 0)
    {
        depthClipEnable = (m_rasterizerDesc.DepthClipEnable != 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveRSDepthClipEnable()
{
    m_rasterizerValidFlags &= ~D3D11RenderStateManager::RSVALID_DEPTHCLIPENABLE;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRSScissorEnable(efd::Bool scissorEnable)
{
    m_rasterizerDesc.ScissorEnable = scissorEnable;
    m_rasterizerValidFlags |= D3D11RenderStateManager::RSVALID_SCISSORENABLE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetRSScissorEnable(efd::Bool& scissorEnable) const
{
    if ((m_rasterizerValidFlags & D3D11RenderStateManager::RSVALID_SCISSORENABLE) != 0)
    {
        scissorEnable = (m_rasterizerDesc.ScissorEnable != 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveRSScissorEnable()
{
    m_rasterizerValidFlags &= ~D3D11RenderStateManager::RSVALID_SCISSORENABLE;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRSMultisampleEnable(efd::Bool multisampleEnable)
{
    m_rasterizerDesc.MultisampleEnable= multisampleEnable;
    m_rasterizerValidFlags |= D3D11RenderStateManager::RSVALID_MULTISAMPLEENABLE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetRSMultisampleEnable(efd::Bool& multisampleEnable)
    const
{
    if ((m_rasterizerValidFlags & D3D11RenderStateManager::RSVALID_MULTISAMPLEENABLE) != 0)
    {
        multisampleEnable = (m_rasterizerDesc.MultisampleEnable != 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveRSMultisampleEnable()
{
    m_rasterizerValidFlags &= ~D3D11RenderStateManager::RSVALID_MULTISAMPLEENABLE;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRSAntialiasedLineEnable(efd::Bool antialiasedLineEnable)
{
    m_rasterizerDesc.AntialiasedLineEnable = antialiasedLineEnable;
    m_rasterizerValidFlags |= D3D11RenderStateManager::RSVALID_ANTIALIASEDLINEENABLE;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::GetRSAntialiasedLineEnable(efd::Bool& antialiasedLineEnable) const
{
    if ((m_rasterizerValidFlags & D3D11RenderStateManager::RSVALID_ANTIALIASEDLINEENABLE) != 0)
    {
        antialiasedLineEnable = (m_rasterizerDesc.AntialiasedLineEnable != 0);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::RemoveRSAntialiasedLineEnable()
{
    m_rasterizerValidFlags &= ~D3D11RenderStateManager::RSVALID_ANTIALIASEDLINEENABLE;
}

//------------------------------------------------------------------------------------------------
D3D11RenderStateGroup::Sampler* D3D11RenderStateGroup::AddSampler(
    const efd::FixedString& samplerName)
{
    Sampler* pSampler = GetSampler(samplerName);
    if (pSampler == NULL)
    {
        pSampler = EE_NEW Sampler(samplerName);
        pSampler->m_pNext = m_pSamplers;
        m_pSamplers = pSampler;
    }
    return pSampler;
}

//------------------------------------------------------------------------------------------------
D3D11RenderStateGroup::Sampler* D3D11RenderStateGroup::FindSampler(
    const efd::FixedString& samplerName, 
    D3D11RenderStateGroup::Sampler*& pSamplerBefore) const
{
    pSamplerBefore = NULL;
    Sampler* pSampler = m_pSamplers;
    while (pSampler)
    {
        if (pSampler->m_samplerName == samplerName)
        {
            return pSampler;
        }
        pSamplerBefore = pSampler;
        pSampler = pSampler->m_pNext;
    }

    pSamplerBefore = NULL;
    return NULL;
}

//------------------------------------------------------------------------------------------------
D3D11RenderStateGroup::Sampler* D3D11RenderStateGroup::GetSampler(
    const efd::FixedString& samplerName) const
{
    Sampler* pSamplerBefore = NULL;
    return FindSampler(samplerName, pSamplerBefore);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::RemoveSampler(const efd::FixedString& samplerName)
{
    Sampler* pSamplerBefore = NULL;
    Sampler* pRemoveSampler = FindSampler(samplerName, pSamplerBefore);
    if (pRemoveSampler == NULL)
        return false;

    if (pSamplerBefore)
        pSamplerBefore->m_pNext = pRemoveSampler->m_pNext;
    else
        m_pSamplers = pRemoveSampler->m_pNext;

    pRemoveSampler->m_pNext = NULL;

    EE_DELETE pRemoveSampler;

    // Bump reset count because sampler pointers held elsewhere may be invalid now.
    m_resetCount++;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::RemoveSampler(D3D11RenderStateGroup::Sampler* pSampler)
{
    Sampler* pSamplerBefore = NULL;
    Sampler* pRemoveSampler = FindSampler(
        pSampler->m_samplerName, 
        pSamplerBefore);
    if (pRemoveSampler == NULL)
        return false;

    // It should never be the case that multiple samplers with the same name/program type exist.
    EE_ASSERT(pRemoveSampler == pSampler);

    if (pSamplerBefore)
        pSamplerBefore->m_pNext = pRemoveSampler->m_pNext;
    else
        m_pSamplers = pRemoveSampler->m_pNext;

    pRemoveSampler->m_pNext = NULL;

    EE_DELETE pRemoveSampler;

    // Bump reset count because sampler pointers held elsewhere may be invalid now.
    m_resetCount++;

    return true;
}

//------------------------------------------------------------------------------------------------
D3D11RenderStateGroup::Sampler::Sampler(efd::FixedString samplerName) :
    m_samplerName(samplerName),
    m_samplerValidFlags(0),
    m_pNext(NULL)
{
    m_samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    m_samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_samplerDesc.MipLODBias = 0.0f;
    m_samplerDesc.MaxAnisotropy = 16;
    m_samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    m_samplerDesc.BorderColor[0] = 0.0f;
    m_samplerDesc.BorderColor[1] = 0.0f;
    m_samplerDesc.BorderColor[2] = 0.0f;
    m_samplerDesc.BorderColor[3] = 0.0f;
    m_samplerDesc.MinLOD = -FLT_MAX;
    m_samplerDesc.MaxLOD = FLT_MAX;
}

//------------------------------------------------------------------------------------------------
D3D11RenderStateGroup::Sampler::~Sampler()
{
    EE_DELETE m_pNext;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::SetFilter(D3D11_FILTER filter)
{
    m_samplerDesc.Filter = filter;
    m_samplerValidFlags |= D3D11RenderStateManager::SVALID_FILTER;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::Sampler::GetFilter(D3D11_FILTER& filter) const
{
    if ((m_samplerValidFlags & D3D11RenderStateManager::SVALID_FILTER) != 0)
    {
        filter = m_samplerDesc.Filter;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::RemoveFilter()
{
    m_samplerValidFlags &= ~D3D11RenderStateManager::SVALID_FILTER;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::SetAddressU(D3D11_TEXTURE_ADDRESS_MODE addressU)
{
    m_samplerDesc.AddressU = addressU;
    m_samplerValidFlags |= D3D11RenderStateManager::SVALID_ADDRESSU;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::Sampler::GetAddressU(
    D3D11_TEXTURE_ADDRESS_MODE& addressU) const
{
    if ((m_samplerValidFlags & D3D11RenderStateManager::SVALID_ADDRESSU) != 0)
    {
        addressU = m_samplerDesc.AddressU;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::RemoveAddressU()
{
    m_samplerValidFlags &= ~D3D11RenderStateManager::SVALID_ADDRESSU;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::SetAddressV(D3D11_TEXTURE_ADDRESS_MODE addressV)
{
    m_samplerDesc.AddressV = addressV;
    m_samplerValidFlags |= D3D11RenderStateManager::SVALID_ADDRESSV;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::Sampler::GetAddressV(D3D11_TEXTURE_ADDRESS_MODE& addressV) const
{
    if ((m_samplerValidFlags & D3D11RenderStateManager::SVALID_ADDRESSV) != 0)
    {
        addressV = m_samplerDesc.AddressV;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::RemoveAddressV()
{
    m_samplerValidFlags &= ~D3D11RenderStateManager::SVALID_ADDRESSV;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::SetAddressW(D3D11_TEXTURE_ADDRESS_MODE addressW)
{
    m_samplerDesc.AddressW = addressW;
    m_samplerValidFlags |= D3D11RenderStateManager::SVALID_ADDRESSW;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::Sampler::GetAddressW(D3D11_TEXTURE_ADDRESS_MODE& addressW) const
{
    if ((m_samplerValidFlags & D3D11RenderStateManager::SVALID_ADDRESSW) != 0)
    {
        addressW = m_samplerDesc.AddressW;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::RemoveAddressW()
{
    m_samplerValidFlags &= ~D3D11RenderStateManager::SVALID_ADDRESSW;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::SetMipLODBias(efd::Float32 mipLODBias)
{
    m_samplerDesc.MipLODBias = mipLODBias;
    m_samplerValidFlags |= D3D11RenderStateManager::SVALID_MIPLODBIAS;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::Sampler::GetMipLODBias(efd::Float32& mipLODBias) const
{
    if ((m_samplerValidFlags & D3D11RenderStateManager::SVALID_MIPLODBIAS) != 0)
    {
        mipLODBias = m_samplerDesc.MipLODBias;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::RemoveMipLODBias()
{
    m_samplerValidFlags &= ~D3D11RenderStateManager::SVALID_MIPLODBIAS;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::SetMaxAnisotropy(efd::UInt32 maxAnisotropy)
{
    m_samplerDesc.MaxAnisotropy = maxAnisotropy;
    m_samplerValidFlags |= D3D11RenderStateManager::SVALID_MAXANISOTROPY;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::Sampler::GetMaxAnisotropy(efd::UInt32& maxAnisotropy) const
{
    if ((m_samplerValidFlags & D3D11RenderStateManager::SVALID_MAXANISOTROPY) != 0)
    {
        maxAnisotropy = m_samplerDesc.MaxAnisotropy;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::RemoveMaxAnisotropy()
{
    m_samplerValidFlags &= ~D3D11RenderStateManager::SVALID_MAXANISOTROPY;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::SetComparisonFunc(D3D11_COMPARISON_FUNC comparisonFunc)
{
    m_samplerDesc.ComparisonFunc = comparisonFunc;
    m_samplerValidFlags |= D3D11RenderStateManager::SVALID_COMPARISONFUNC;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::Sampler::GetComparisonFunc(
    D3D11_COMPARISON_FUNC& comparisonFunc) const
{
    if ((m_samplerValidFlags & D3D11RenderStateManager::SVALID_COMPARISONFUNC) != 0)
    {
        comparisonFunc = m_samplerDesc.ComparisonFunc;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::RemoveComparisonFunc()
{
    m_samplerValidFlags &= ~D3D11RenderStateManager::SVALID_COMPARISONFUNC;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::SetBorderColor(const efd::Float32 borderColor[4])
{
    for (efd::UInt32 i = 0; i < 4; i++)
        m_samplerDesc.BorderColor[i] = borderColor[i];

    m_samplerValidFlags |= D3D11RenderStateManager::SVALID_BORDERCOLOR;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::Sampler::GetBorderColor(efd::Float32 borderColor[4]) const
{
    if ((m_samplerValidFlags & D3D11RenderStateManager::SVALID_BORDERCOLOR) != 0)
    {
        for (efd::UInt32 i = 0; i < 4; i++)
            borderColor[i] = m_samplerDesc.BorderColor[i];

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::RemoveBorderColor()
{
    m_samplerValidFlags &= ~D3D11RenderStateManager::SVALID_BORDERCOLOR;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::SetMinLOD(efd::Float32 minLOD)
{
    m_samplerDesc.MinLOD = minLOD;
    m_samplerValidFlags |= D3D11RenderStateManager::SVALID_MINLOD;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::Sampler::GetMinLOD(efd::Float32& minLOD) const
{
    if ((m_samplerValidFlags & D3D11RenderStateManager::SVALID_MINLOD) != 0)
    {
        minLOD = m_samplerDesc.MinLOD;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::RemoveMinLOD()
{
    m_samplerValidFlags &= ~D3D11RenderStateManager::SVALID_MINLOD;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::SetMaxLOD(efd::Float32 maxLOD)
{
    m_samplerDesc.MaxLOD = maxLOD;
    m_samplerValidFlags |= D3D11RenderStateManager::SVALID_MAXLOD;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderStateGroup::Sampler::GetMaxLOD(efd::Float32& maxLOD) const
{
    if ((m_samplerValidFlags & D3D11RenderStateManager::SVALID_MAXLOD) != 0)
    {
        maxLOD = m_samplerDesc.MaxLOD;
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::Sampler::RemoveMaxLOD()
{
    m_samplerValidFlags &= ~D3D11RenderStateManager::SVALID_MAXLOD;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetBlendStateDesc(
    const D3D11_BLEND_DESC& blendDesc, 
    efd::UInt32 validFlags,
    efd::UInt8 renderTargetValidFlags[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT])
{
    m_blendDesc = blendDesc;
    m_blendValidFlags = validFlags;
    for (efd::UInt32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        m_renderTargetValidFlags[i] = renderTargetValidFlags[i];
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::GetBlendStateDesc(
    D3D11_BLEND_DESC& blendDesc, efd::UInt32& validFlags,
    efd::UInt8 renderTargetValidFlags[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]) const
{
    blendDesc = m_blendDesc;
    validFlags = m_blendValidFlags;
    for (efd::UInt32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        renderTargetValidFlags[i] = m_renderTargetValidFlags[i];
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetDepthStencilStateDesc(
    const D3D11_DEPTH_STENCIL_DESC& depthStencilDesc, 
    efd::UInt32 validFlags)
{
    m_depthStencilDesc = depthStencilDesc;
    m_depthStencilValidFlags = validFlags;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::GetDepthStencilStateDesc(
    D3D11_DEPTH_STENCIL_DESC& depthStencilDesc, 
    efd::UInt32& validFlags) const
{
    depthStencilDesc = m_depthStencilDesc;
    validFlags = m_depthStencilValidFlags;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetRasterizerStateDesc(
    const D3D11_RASTERIZER_DESC& rasterizerDesc, 
    efd::UInt32 validFlags)
{
    m_rasterizerDesc = rasterizerDesc;
    m_rasterizerValidFlags = validFlags;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::GetRasterizerStateDesc(
    D3D11_RASTERIZER_DESC& rasterizerDesc, 
    efd::UInt32& validFlags) const
{
    rasterizerDesc = m_rasterizerDesc;
    validFlags = m_rasterizerValidFlags;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::SetSamplerDesc(
    efd::FixedString& samplerName,
    const D3D11_SAMPLER_DESC& samplerDesc, 
    efd::UInt32 samplerValidFlags)
{
    // Will create the sampler if it does not exist
    Sampler* pSampler = AddSampler(samplerName);
    pSampler->m_samplerDesc = samplerDesc;
    pSampler->m_samplerValidFlags = samplerValidFlags;
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::GetSamplerDesc(
    efd::FixedString& samplerName,
    D3D11_SAMPLER_DESC& samplerDesc,
    efd::UInt32& samplerValidFlags) const
{
    Sampler* pSampler = GetSampler(samplerName);
    if (pSampler)
    {
        samplerDesc = pSampler->m_samplerDesc;
        samplerValidFlags = pSampler->m_samplerValidFlags;
    }
    else
    {
        samplerValidFlags = 0;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11RenderStateGroup::ResetRenderStates()
{
    m_blendValidFlags = 0;
    m_depthStencilValidFlags = 0;
    m_rasterizerValidFlags = 0;
    m_blendFactorValid = false;
    m_sampleMaskValid = false;
    m_stencilRefValid = false;

    memset(m_renderTargetValidFlags, 0, sizeof(m_renderTargetValidFlags));

    EE_DELETE m_pSamplers;
    m_pSamplers = NULL;

    m_resetCount++;
}

//------------------------------------------------------------------------------------------------
