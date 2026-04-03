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

#include "D3D11Error.h"
#include "D3D11PixelFormat.h"
#include "D3D11Renderer.h"
#include "D3D11DynamicTextureData.h"
#include "D3D11ResourceManager.h"

#include <NiDynamicTexture.h>

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11DynamicTextureData::D3D11DynamicTextureData(NiDynamicTexture* pTexture) :
    D3D11TextureData(pTexture), 
    m_isTextureLocked(false),
    m_pStagingTexture(NULL),
    m_mapFlags(D3D11_MAP_READ_WRITE)
{
    EE_ASSERT(pTexture);
    m_resourceType |= RESOURCETYPE_DYNAMIC;
}

//------------------------------------------------------------------------------------------------
D3D11DynamicTextureData::~D3D11DynamicTextureData()
{
    if (m_pStagingTexture)
        m_pStagingTexture->Release();
    m_pStagingTexture = NULL;
}

//------------------------------------------------------------------------------------------------
D3D11DynamicTextureData* D3D11DynamicTextureData::Create(NiDynamicTexture* pTexture)
{
    D3D11DynamicTextureData* pThis = EE_NEW D3D11DynamicTextureData(
        pTexture);

    const NiPixelFormat* pFmt = pThis->CreateTexture(pTexture);
    if (!pFmt)
    {
        EE_DELETE pThis;
        return NULL;
    }

    pThis->m_pkTexture->SetRendererData(pThis);

    return pThis;
}

//------------------------------------------------------------------------------------------------
const NiPixelFormat* D3D11DynamicTextureData::CreateTexture(const NiDynamicTexture* pTexture)
{
    if (pTexture == 0)
        return NULL;

    //for now all we do is 2D dynamic textures.
    m_uiWidth = pTexture->GetWidth();
    m_uiHeight = pTexture->GetHeight();
    m_levels = 1;
    efd::UInt32 textureType = RESOURCETYPE_2D;

    // Renderer must exist.
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL)
        return NULL;

    // Need a format.  Determine the desired pixel format for the buffer.  For
    // now, just look at the alpha.
    const NiTexture::FormatPrefs formatPrefs = pTexture->GetFormatPreferences();

    // Dynamic Textures are always write discard.
    m_mapFlags = D3D11_MAP_WRITE;

    const NiPixelFormat* pFmt = D3D11TextureData::FindMatchingPixelFormat(
        formatPrefs,
        D3D11_FORMAT_SUPPORT_TEXTURE2D);

    if (pFmt == NULL)
        return NULL;

    m_kPixelFormat = *pFmt;

    DXGI_FORMAT format = (DXGI_FORMAT)pFmt->GetRendererHint();

    ID3D11Device* pD3DDevice11 = pRenderer->GetD3D11Device();

    HRESULT hr = E_FAIL;
    ID3D11ShaderResourceView* pResourceView = NULL;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

    if (m_pD3D11Resource)
        m_pD3D11Resource->Release();
    m_pD3D11Resource = NULL;

    if (m_pStagingTexture)
        m_pStagingTexture->Release();
    m_pStagingTexture = NULL;

    D3D11ResourceManager* pResourceManager = pRenderer->GetResourceManager();

    ID3D11Resource* pD3D11Texture = NULL;
    ID3D11Resource* pD3D11StagingTexture = NULL;

    if ((textureType & RESOURCETYPE_1D) != 0)
    {
        pD3D11Texture = pResourceManager->CreateTexture1D(
            m_uiWidth, 
            1, 
            1, 
            format,
            D3D11_USAGE_DEFAULT, 
            D3D11_BIND_SHADER_RESOURCE, 
            0,
            0);

        if (pD3D11Texture == NULL)
        {
            // Error already reported
            pD3D11Texture = NULL;
            return NULL;
        }

        //create the staging texture
        pD3D11StagingTexture = pResourceManager->CreateTexture1D(
            m_uiWidth, 
            1, 
            1, 
            format,
            D3D11_USAGE_STAGING, 
            0, 
            D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ, 
            0);

        // Test for failure.
        if (pD3D11StagingTexture == NULL)
        {
            // Error already reported
            pD3D11Texture->Release();
            pD3D11Texture = NULL;
            D3D11Error::ReportWarning("Staging texture creation failed in " __FUNCTION__);
            return NULL;
        }

        //set format and dimension
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;

        // Create the shader-resource view
        srvDesc.Texture1D.MostDetailedMip = 0;
        srvDesc.Texture1D.MipLevels = 1;

        hr = pD3DDevice11->CreateShaderResourceView(
            pD3D11Texture,
            &srvDesc, 
            &pResourceView);

    }
    else if ((textureType & RESOURCETYPE_2D) != 0 || (textureType & RESOURCETYPE_CUBE) != 0)
    {
        efd::UInt32 miscFlags = IsCubeMap() ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

        pD3D11Texture = pResourceManager->CreateTexture2D(
            m_uiWidth, 
            m_uiHeight, 
            1, 
            IsCubeMap() ? 6 : 1, 
            format,
            1, 
            0, 
            D3D11_USAGE_DEFAULT, 
            D3D11_BIND_SHADER_RESOURCE, 
            0,
            miscFlags);

        if (pD3D11Texture == NULL)
        {
            // Error already reported
            pD3D11Texture = NULL;
            return NULL;
        }

        // Create the staging texture.
        pD3D11StagingTexture = pResourceManager->CreateTexture2D(
            m_uiWidth, 
            m_uiHeight, 
            1, 
            IsCubeMap() ? 6 : 1, 
            format,
            1, 
            0, 
            D3D11_USAGE_STAGING, 
            0, 
            D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ, 
            miscFlags);

        // Test for failure.
        if (pD3D11StagingTexture == NULL)
        {
            // Error already reported
            pD3D11Texture->Release();
            pD3D11Texture = NULL;
            D3D11Error::ReportWarning("Staging texture creation failed in " __FUNCTION__);
            return NULL;
        }

        // set format and dimension for creating
        // resource view.
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

        // Create the shader-resource view
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        hr = pD3DDevice11->CreateShaderResourceView(
            pD3D11Texture,
            &srvDesc, 
            &pResourceView);
    }
    else if ((textureType & RESOURCETYPE_3D) != 0)
    {
        pD3D11Texture = pResourceManager->CreateTexture3D(
            m_uiWidth, 
            m_uiHeight,
            1,
            1, 
            format,
            D3D11_USAGE_DEFAULT, 
            D3D11_BIND_SHADER_RESOURCE, 
            0,
            0);

        if (pD3D11Texture == NULL)
        {
            // Error already reported
            pD3D11Texture = NULL;
            return NULL;
        }

        //create the staging texture
        pD3D11StagingTexture = pResourceManager->CreateTexture3D(
            m_uiWidth, 
            m_uiHeight, 
            1,
            1, 
            format,
            D3D11_USAGE_STAGING, 
            0, 
            D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ, 
            0);

        // Test for failure.
        if (pD3D11StagingTexture == NULL)
        {
            // Error already reported
            pD3D11Texture->Release();
            pD3D11Texture = NULL;
            D3D11Error::ReportWarning("Staging texture creation failed in " __FUNCTION__);
            return NULL;
        }

        //set format and dimension
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;

        // Create the shader-resource view
        srvDesc.Texture3D.MostDetailedMip = 0;
        srvDesc.Texture3D.MipLevels = 1;

        hr = pD3DDevice11->CreateShaderResourceView(
            pD3D11Texture,
            &srvDesc, 
            &pResourceView);
    }

    if (FAILED(hr) || pResourceView == NULL)
    {
        if (pD3D11Texture)
        {
            pD3D11Texture->Release();
            pD3D11Texture = NULL;
        }
        if (pD3D11StagingTexture)
        {
            pD3D11StagingTexture->Release();
            pD3D11StagingTexture = NULL;
        }

        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SHADER_RESOURCE_VIEW_CREATION_FAILED,
                "Failure in " 
                __FUNCTION__ 
                " with error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SHADER_RESOURCE_VIEW_CREATION_FAILED,
                "Failure in " 
                __FUNCTION__ 
                " with no error message from D3D11, but resource view is NULL.");

        }

        pResourceView = NULL;
        return NULL;
    }

    EE_ASSERT(pD3D11Texture && pD3D11StagingTexture && pResourceView);

    InitializeFromD3D11Resource(pD3D11Texture, pResourceView);
    pD3D11Texture->Release();
    pResourceView->Release();
    m_pStagingTexture = pD3D11StagingTexture;

    return pFmt;
}

//------------------------------------------------------------------------------------------------
void* D3D11DynamicTextureData::Lock(efd::SInt32& pitch)
{
    void* pMem = NULL;
    if (m_pD3D11Resource)
    {
        if (IsTexture1D())
        {
            pMem = Texture1DLock();
        }
        else if (IsTexture2D() || IsCubeMap())
        {
            pMem = Texture2DLock(pitch);
        }
        else if (IsTexture3D())
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_UNSUPPORTED_RESOURCE_LOCK_FAILED,
                "3D dynamic texture locking is unsupported.");
            EE_ASSERT(false);
        }
    }
    return pMem;
}

//------------------------------------------------------------------------------------------------
bool D3D11DynamicTextureData::UnLock() const
{
    if (!m_pD3D11Resource)
        return false;

    // Renderer must exist.
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL)
        return NULL;

    // DT33837 Support multiple device contexts.
    ID3D11DeviceContext* pContext = pRenderer->GetCurrentD3D11DeviceContext();
    if (pContext == NULL)
        return NULL;

    if (IsTexture1D())
    {
        ID3D11Texture1D* pTexture = (ID3D11Texture1D*)m_pD3D11Resource;
        if (pTexture)
        {
            ID3D11Texture1D* pStagingTexture = (ID3D11Texture1D*)m_pStagingTexture;

            if (pStagingTexture == NULL)
                return false;

            pContext->Unmap(pStagingTexture, D3D11CalcSubresource(0, 0, 1));

            pContext->CopyResource(pTexture, pStagingTexture);
            return true;
        }
    }
    else if (IsTexture2D() || IsCubeMap())
    {

        ID3D11Texture2D* pTexture = (ID3D11Texture2D*)m_pD3D11Resource;
        if (pTexture)
        {
            ID3D11Texture2D* pStagingTexture = (ID3D11Texture2D*)m_pStagingTexture;

            if (pStagingTexture == NULL)
                return false;

            pContext->Unmap(pStagingTexture, D3D11CalcSubresource(0, 0, 1));

            pContext->CopyResource(pTexture, pStagingTexture);
            return true;
        }
    }
    else if (IsTexture3D())
    {
        ID3D11Texture3D* pTexture = (ID3D11Texture3D*)m_pD3D11Resource;
        if (pTexture)
        {
            ID3D11Texture3D* pStagingTexture = (ID3D11Texture3D*)m_pStagingTexture;

            if (pStagingTexture == NULL)
                return false;

            pContext->Unmap(pStagingTexture, D3D11CalcSubresource(0, 0, 1));

            pContext->CopyResource(pTexture, pStagingTexture);
            return true;
        }
    }
    else
    {
        D3D11Error::ReportError(D3D11Error::D3D11ERROR_UNSUPPORTED_RESOURCE_LOCK_FAILED);
    }
    return false;
}

//------------------------------------------------------------------------------------------------
void* D3D11DynamicTextureData::Texture1DLock()
{
    if (!m_pD3D11Resource || !m_pStagingTexture)
        return NULL;

    if (!IsTexture1D())
        return NULL;

    // Renderer must exist.
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL)
        return NULL;

    // DT33837 Support multiple device contexts.
    ID3D11DeviceContext* pContext = pRenderer->GetCurrentD3D11DeviceContext();
    if (pContext == NULL)
        return NULL;

    ID3D11Texture1D* pStagingTexture = (ID3D11Texture1D*)m_pStagingTexture;

    D3D11_MAPPED_SUBRESOURCE mappedTex;
    HRESULT hr = pContext->Map(
        pStagingTexture, 
        D3D11CalcSubresource(0, 0, 1),
        m_mapFlags, 
        0, 
        &mappedTex);

    if (SUCCEEDED(hr))
    {
        void* pMem = mappedTex.pData;
        return pMem;
    }

    D3D11Error::ReportError(D3D11Error::D3D11ERROR_TEXTURE1D_LOCK_FAILED);

    return NULL;
}

//------------------------------------------------------------------------------------------------
void* D3D11DynamicTextureData::Texture2DLock(efd::SInt32& pitch)
{
    if (!m_pD3D11Resource || !m_pStagingTexture)
        return NULL;

    if (!IsTexture2D() && !IsCubeMap())
        return NULL;

    // Renderer must exist.
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL)
        return NULL;

    ID3D11DeviceContext* pContext = pRenderer->GetCurrentD3D11DeviceContext();
    if (pContext == NULL)
        return NULL;

    ID3D11Texture2D* pStagingTexture = (ID3D11Texture2D*)m_pStagingTexture;

    D3D11_MAPPED_SUBRESOURCE mappedTex;
    HRESULT hr = pContext->Map(
        pStagingTexture,
        D3D11CalcSubresource(0, 0, 1),
        m_mapFlags, 
        0, 
        &mappedTex);
    if (SUCCEEDED(hr))
    {
        void* pMem = mappedTex.pData;
        pitch = mappedTex.RowPitch;
        return pMem;
    }

    D3D11Error::ReportError(D3D11Error::D3D11ERROR_TEXTURE2D_LOCK_FAILED);

    return NULL;
}

//------------------------------------------------------------------------------------------------
void* D3D11DynamicTextureData::Texture3DLock(efd::SInt32& rowPitch,efd::SInt32& depthPitch)
{
    if (!m_pD3D11Resource || !m_pStagingTexture)
        return NULL;

    if (!IsTexture3D())
        return NULL;

    // Renderer must exist.
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL)
        return NULL;

    // DT33837 Support multiple device contexts.
    ID3D11DeviceContext* pContext = pRenderer->GetCurrentD3D11DeviceContext();
    if (pContext == NULL)
        return NULL;

    ID3D11Texture3D* pStagingTexture = (ID3D11Texture3D*)m_pStagingTexture;

    D3D11_MAPPED_SUBRESOURCE mappedTex;
    HRESULT hr = pContext->Map(
        pStagingTexture,
        D3D11CalcSubresource(0, 0, 1),
        m_mapFlags, 
        0, 
        &mappedTex);
    if (SUCCEEDED(hr))
    {
        void* pMem = mappedTex.pData;
        rowPitch = mappedTex.RowPitch;
        depthPitch = mappedTex.DepthPitch;
        return pMem;
    }

    D3D11Error::ReportError(D3D11Error::D3D11ERROR_TEXTURE3D_LOCK_FAILED);

    return NULL;
}

//------------------------------------------------------------------------------------------------
