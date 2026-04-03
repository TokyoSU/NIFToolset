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

#include "D3D11RenderedTextureData.h"
#include "D3D11Error.h"
#include "D3D11PixelFormat.h"
#include "D3D11Renderer.h"
#include "D3D11ResourceManager.h"

#include <NiRenderedTexture.h>
#include <NiRenderedCubeMap.h>

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11RenderedTextureData::D3D11RenderedTextureData(
    NiRenderedTexture* pTexture) :
    D3D11TextureData(pTexture),
    m_numTextures(1)
{
    m_resourceType |= RESOURCETYPE_SOURCE;
    if (NiIsKindOf(NiRenderedCubeMap, pTexture))
    {
        m_numTextures = 6;
        m_resourceType |= RESOURCETYPE_CUBE;
    }
}

//------------------------------------------------------------------------------------------------
D3D11RenderedTextureData::~D3D11RenderedTextureData()
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11RenderedTextureData* D3D11RenderedTextureData::Create(
    NiRenderedTexture* pTexture, Ni2DBuffer::MultiSamplePreference msaaPref)
{
    D3D11RenderedTextureData* pThis = EE_NEW D3D11RenderedTextureData(pTexture);

    efd::Bool success = pThis->PrepareTexture(msaaPref);

    EE_ASSERT(success == false || pTexture->GetRendererData() == pThis);

    if (success)
    {
        return pThis;
    }
    else
    {
        EE_DELETE pThis;
        return NULL;
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11RenderedTextureData::PrepareTexture(Ni2DBuffer::MultiSamplePreference msaaPref)
{
    if (m_pkTexture == NULL)
        return false;

    EE_ASSERT(NiIsKindOf(NiRenderedTexture, m_pkTexture));
    NiRenderedTexture* pTexture = (NiRenderedTexture*)m_pkTexture;

    m_uiWidth = pTexture->GetWidth();
    m_uiHeight = pTexture->GetHeight();
    m_levels = 1;

    EE_ASSERT(m_numTextures == (IsCubeMap() ? 6 : 1));

    const NiTexture::FormatPrefs formatPrefs = pTexture->GetFormatPreferences();
    efd::Bool isCube = IsCubeMap();

    const NiPixelFormat* pFmt = FindMatchingPixelFormat(
        formatPrefs, 
        isCube ? D3D11_FORMAT_SUPPORT_TEXTURECUBE : D3D11_FORMAT_SUPPORT_TEXTURE2D);

    m_kPixelFormat = *pFmt;

    DXGI_FORMAT format = D3D11PixelFormat::DetermineDXGIFormat(m_kPixelFormat);

    efd::UInt32 msaaCount = 1;
    efd::UInt32 msaaQuality = 0;
    Ni2DBuffer::GetMSAACountAndQualityFromPref(msaaPref, msaaCount, msaaQuality);
    EE_ASSERT(msaaCount != 0);

    D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
    efd::UInt32 cpuAccessFlags = 0;

    efd::UInt32 bindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    EE_ASSERT(m_levels != 0 && m_levels < D3D11_MAX_TEXTURE_DIMENSION_2_TO_EXP);

    efd::UInt32 uiMiscFlags = (isCube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0);

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer && pRenderer->GetResourceManager() && pRenderer->GetD3D11Device());

    ID3D11Texture2D* pD3D11Texture = pRenderer->GetResourceManager()->CreateTexture2D(
        m_uiWidth, 
        m_uiHeight, 
        m_levels, 
        m_numTextures, 
        format,
        msaaCount, 
        msaaQuality, 
        usage, 
        bindFlags, 
        cpuAccessFlags,
        uiMiscFlags, 
        NULL);

    if (pD3D11Texture == NULL)
    {
        D3D11Error::ReportWarning(__FUNCTION__ " failed to obtain texture.");
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
    viewDesc.Format = format;
    if (isCube)
    {
        EE_ASSERT(m_numTextures == 6);
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        viewDesc.TextureCube.MostDetailedMip = 0;
        viewDesc.TextureCube.MipLevels = m_levels;
    }
    else
    {
        if (m_numTextures == 1)
        {
            if (msaaCount == 1)
            {
                viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                viewDesc.Texture2D.MostDetailedMip = 0;
                viewDesc.Texture2D.MipLevels = m_levels;
            }
            else
            {
                viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                // No other parameters
            }
        }
        else
        {
            if (msaaCount == 1)
            {
                viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                viewDesc.Texture2DArray.MostDetailedMip = 0;
                viewDesc.Texture2DArray.MipLevels = m_levels;
                viewDesc.Texture2DArray.FirstArraySlice = 0;
                viewDesc.Texture2DArray.ArraySize = m_numTextures;
            }
            else
            {
                viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                viewDesc.Texture2DMSArray.FirstArraySlice = 0;
                viewDesc.Texture2DMSArray.ArraySize = m_numTextures;
            }
        }
    }

    ID3D11ShaderResourceView* pResourceView = NULL;
    HRESULT hr = pRenderer->GetD3D11Device()->CreateShaderResourceView(
        pD3D11Texture, 
        &viewDesc, 
        &pResourceView);
    if (FAILED(hr) || pResourceView == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SHADER_RESOURCE_VIEW_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SHADER_RESOURCE_VIEW_CREATION_FAILED,
                "No error message from D3D11, but resource view is NULL.");
        }

        if (pResourceView)
        {
            pResourceView->Release();
        }
    }

    // Create the 2D buffer data. This will manage the D3D11 surface
    // and automatically fills in the Ni2DBuffer::RendererData for us
    // as a side effect.
    if (pTexture->GetBuffer())
    {
        EE_ASSERT(IsCubeMap() == false);
        Ni2DBuffer* pBuffer = pTexture->GetBuffer();
        D3D112DBufferData* pBuffData = D3D11RenderTargetBufferData::Create(
            pD3D11Texture, 
            pBuffer);

        if (!pBuffData)
        {
            D3D11Renderer::Warning("D3D11RenderTargetBufferData::Create failed in " __FUNCTION__);
            return false;
        }

    }

    if (IsCubeMap())
    {
        EE_ASSERT(NiIsKindOf(NiRenderedCubeMap, pTexture));
        NiRenderedCubeMap* pRenderedCubeMap = (NiRenderedCubeMap*)pTexture;

        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        rtvDesc.Format = format;
        if (msaaCount == 1)
        {
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice = 0;
            rtvDesc.Texture2DArray.FirstArraySlice = 0;
            rtvDesc.Texture2DArray.ArraySize = 1;
        }
        else
        {
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
            rtvDesc.Texture2DMSArray.FirstArraySlice = 0;
            rtvDesc.Texture2DMSArray.ArraySize = 1;
        }

        for (efd::UInt32 i = 0; i < NiRenderedCubeMap::FACE_NUM; i++)
        {
            Ni2DBuffer* pBuffer = pRenderedCubeMap->GetFaceBuffer((NiRenderedCubeMap::FaceID)i);
            EE_ASSERT(pBuffer != NULL);

            if (msaaCount == 1)
            {
                rtvDesc.Texture2DArray.FirstArraySlice = i;
            }
            else
            {
                rtvDesc.Texture2DMSArray.FirstArraySlice = i;
            }

            D3D112DBufferData* pBuffData = D3D11RenderTargetBufferData::Create(
                pD3D11Texture, 
                pBuffer, 
                &rtvDesc);

            if (!pBuffData)
            {
                D3D11Renderer::Warning(
                    "D3D11RenderTargetBufferData::Create failed in " 
                    __FUNCTION__);
                return false;
            }
        }
    }

    efd::Bool success = InitializeFromD3D11Resource(pD3D11Texture, pResourceView);
    if (success)
        pTexture->SetRendererData(this);

    pD3D11Texture->Release();
    pResourceView->Release();
    return success;
}

//------------------------------------------------------------------------------------------------
