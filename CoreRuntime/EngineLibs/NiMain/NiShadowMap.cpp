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
#include "NiMainPCH.h"

#include "NiShadowGenerator.h"
#include "NiShadowMap.h"
#include "NiShadowManager.h"
#include "NiStandardMaterial.h"

NiImplementRootRTTI(NiShadowMap);
NiFixedString NiShadowMap::ms_akMapName[NiStandardMaterial::LIGHT_MAX];

//--------------------------------------------------------------------------------------------------
bool NiShadowMap::Initialize(NiShadowMap* pkShadowMap,
    unsigned int uiWidth, unsigned int uiHeight,
    NiTexture::FormatPrefs& kPrefs,
    const NiPixelFormat* pkDepthFormat,
    NiTexturingProperty::ClampMode eClampMode,
    NiTexturingProperty::FilterMode eFilterMode)
{
    EE_ASSERT(pkShadowMap);

    Destroy(pkShadowMap);
    pkShadowMap->SetTextureType(TT_SINGLE);

    // Get renderer.
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    EE_ASSERT(pkRenderer);

    // Create ShadowRenderClick.
    pkShadowMap->m_spRenderClick = NiNew NiShadowRenderClick;
    pkShadowMap->m_spRenderClick->SetClearAllBuffers(true);
    pkShadowMap->m_spRenderClick->SetValidator(
        NiShadowManager::GetShadowClickValidator());
    pkShadowMap->m_spRenderClick->AppendRenderView(NiNew Ni3DRenderView(
        NiNew NiCamera, NiShadowManager::GetCullingProcess()));

    // Create RenderedTexture
    pkShadowMap->m_spTexture = NiRenderedTexture::Create(uiWidth,
        uiHeight, pkRenderer, kPrefs);
    if (!pkShadowMap->m_spTexture)
    {
        // Requested rendered texture could not be created on the current
        // hardware. Fail out.
        Destroy(pkShadowMap);
        return false;
    }

    // Grab/potentially create the pointers to the buffers needed for the render target group
    Ni2DBuffer* pkColorBuffer;
    NiDepthStencilBuffer* pkDepthStencilBuffer;
    if (kPrefs.m_ePixelLayout == NiTexture::FormatPrefs::DEPTH_24_X8)
    {
        // Since the render target is a depth-stencil format, the technique must be requesting
        // a depth-only render, and will not need a separate 'color' float buffer to store depth
        // in, so the render texture is for the depth-stencil buffer, not the color buffer.
        pkColorBuffer = NULL;

        pkDepthStencilBuffer =
            NiDynamicCast(NiDepthStencilBuffer, pkShadowMap->m_spTexture->GetBuffer());

        // Disable clearing the color buffer since there isn't a color buffer.
        pkShadowMap->m_spRenderClick->SetClearAllBuffers(false);
        pkShadowMap->m_spRenderClick->SetClearDepthBuffer(true);
        pkShadowMap->m_spRenderClick->SetClearStencilBuffer(true);
        pkShadowMap->m_spRenderClick->SetClearColorBuffers(false);
    }
    else
    {
        // Grab the render buffer for the color rendering pass
        pkColorBuffer = pkShadowMap->m_spTexture->GetBuffer();

    // Obtain compatible depth/stencil buffer.
        pkDepthStencilBuffer = NiShadowManager::GetCompatibleDepthStencil(
            pkColorBuffer, pkDepthFormat);
    }

    if (!pkDepthStencilBuffer)
    {
        // Requested depth/stencil buffer could not be created on the current
        // hardware. Fail out.
        Destroy(pkShadowMap);
        return false;
    }

    // Create a RenderTargetGroup object from the color and depth stencil buffers
    NiRenderTargetGroup* pkRenderTargetGroup;
    if (pkColorBuffer != NULL)
    {
        pkRenderTargetGroup = NiRenderTargetGroup::Create(
            pkColorBuffer,
            pkRenderer,
            pkDepthStencilBuffer);
    }
    else
    {
        pkRenderTargetGroup = NiRenderTargetGroup::Create(0, pkRenderer);
        if (pkRenderTargetGroup)
        {   
            pkRenderTargetGroup->AttachDepthStencilBuffer(pkDepthStencilBuffer);
        }
    }

    if (!pkRenderTargetGroup)
    {
        // Could not create requested render target group. Fail out.
        Destroy(pkShadowMap);
        return false;
    }

    pkShadowMap->m_spRenderClick->SetRenderTargetGroup(pkRenderTargetGroup);
    pkShadowMap->SetClampMode(eClampMode);
    pkShadowMap->SetFilterMode(eFilterMode);
    pkShadowMap->SetMaxAnisotropy(1);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiShadowMap::Destroy(NiShadowMap* pkShadowMap)
{
    pkShadowMap->ClearFlags();
    pkShadowMap->m_spTexture = NULL;

    if (pkShadowMap->m_spRenderClick)
    {
        pkShadowMap->m_spRenderClick->SetRenderTargetGroup(NULL);
        pkShadowMap->m_spRenderClick = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
