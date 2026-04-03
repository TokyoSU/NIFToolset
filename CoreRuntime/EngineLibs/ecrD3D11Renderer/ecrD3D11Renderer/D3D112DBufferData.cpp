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

#include "D3D112DBufferData.h"
#include "D3D11Error.h"
#include "D3D11Renderer.h"
#include "D3D11PixelFormat.h"

#include <NiDepthStencilBuffer.h>

using namespace ecr;

NiImplementRootRTTI(D3D112DBufferData);

//------------------------------------------------------------------------------------------------
D3D112DBufferData::~D3D112DBufferData()
{
    if (m_pTexture)
        m_pTexture->Release();
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D112DBufferData::CanDisplayFrame()
{
    return false;
}

//------------------------------------------------------------------------------------------------
HRESULT D3D112DBufferData::DisplayFrame(efd::UInt32, efd::Bool)
{
    return E_NOTIMPL;
}

//------------------------------------------------------------------------------------------------
ID3D11Texture2D* D3D112DBufferData::GetTexture2D() const
{
    return m_pTexture;
}

//------------------------------------------------------------------------------------------------
NiImplementRTTI(D3D11RenderTargetBufferData, D3D112DBufferData);

//------------------------------------------------------------------------------------------------
D3D11RenderTargetBufferData::D3D11RenderTargetBufferData() :
    m_pRenderTargetView(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11RenderTargetBufferData::~D3D11RenderTargetBufferData()
{
    if (m_pRenderTargetView)
        m_pRenderTargetView->Release();
}

//------------------------------------------------------------------------------------------------
D3D11RenderTargetBufferData* D3D11RenderTargetBufferData::Create(
    ID3D11Texture2D* pD3DTexture, 
    Ni2DBuffer*& pBuffer,
    D3D11_RENDER_TARGET_VIEW_DESC* pRTViewDesc)
{
    if (pD3DTexture == NULL)
        return NULL;

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT (pRenderer);
    ID3D11Device* pDevice = pRenderer->GetD3D11Device();
    EE_ASSERT (pDevice);

    ID3D11RenderTargetView* pRTView = NULL;
    HRESULT hr = pDevice->CreateRenderTargetView(pD3DTexture, pRTViewDesc, &pRTView);
    if (FAILED(hr) || pRTView == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_RENDER_TARGET_VIEW_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_RENDER_TARGET_VIEW_CREATION_FAILED,
                "No error message from D3D11, but render target view is "
                "NULL.");
        }

        if (pRTView)
            pRTView->Release();

        return NULL;
    }

    D3D11RenderTargetBufferData* pThis = EE_NEW D3D11RenderTargetBufferData;

    pThis->m_pRenderTargetView = pRTView;
    pThis->m_pTexture = pD3DTexture;
    pThis->m_pTexture->AddRef();

    D3D11_TEXTURE2D_DESC textureDesc;
    pD3DTexture->GetDesc(&textureDesc);

    pThis->m_pkPixelFormat = D3D11PixelFormat::ObtainFromDXGIFormat(textureDesc.Format);

    pThis->m_eMSAAPref = Ni2DBuffer::GetMSAAPrefFromCountAndQuality(
        textureDesc.SampleDesc.Count, 
        textureDesc.SampleDesc.Quality);

    if (pBuffer)
    {
        // Check that pBuffer's dimensions are set up correctly - it may have been initially 
        // created with a different set of dimensions.
        if (pBuffer->GetHeight() != textureDesc.Height ||
            pBuffer->GetWidth() != textureDesc.Width)
        {
            pBuffer = NULL;
        }
    }

    if (pBuffer == NULL)
    {
        // Create the buffer if it did not exist prior to this. Note
        // that it is assumed that the app will refcount this
        // buffer after this has been created otherwise it will leak.
        pBuffer = Ni2DBuffer::Create(textureDesc.Width, textureDesc.Height, pThis);
    }
    else
    {
        pBuffer->SetRendererData(pThis);
    }

    pThis->m_pkBuffer = pBuffer;

    return pThis;
}

//------------------------------------------------------------------------------------------------
NiImplementRTTI(D3D11SwapChainBufferData, D3D11RenderTargetBufferData);

//------------------------------------------------------------------------------------------------
D3D11SwapChainBufferData::~D3D11SwapChainBufferData()
{
    if (m_pSwapChain)
    {
        // Get out of fullscreen mode before releasing
        m_pSwapChain->SetFullscreenState(false, NULL);
        m_pSwapChain->Release();
    }
}

//------------------------------------------------------------------------------------------------
D3D11SwapChainBufferData* D3D11SwapChainBufferData::Create(
    IDXGISwapChain* pSwapChain, 
    Ni2DBuffer*& pBuffer)
{
    if (pSwapChain == NULL)
        return NULL;

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT (pRenderer);
    ID3D11Device* pDevice = pRenderer->GetD3D11Device();
    EE_ASSERT (pDevice);

    ID3D11Texture2D* pBackBuffer = NULL;
    HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(*pBackBuffer), (LPVOID*)&pBackBuffer);

    if (FAILED(hr) || pBackBuffer == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_GET_BUFFER_FROM_SWAP_CHAIN_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_GET_BUFFER_FROM_SWAP_CHAIN_FAILED,
                "No error message from D3D11, but buffer is NULL.");
        }

        if (pBackBuffer)
            pBackBuffer->Release();

        return NULL;
    }

    ID3D11RenderTargetView* pRTView = NULL;
    hr = pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRTView);
    if (FAILED(hr) || pRTView == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_RENDER_TARGET_VIEW_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_RENDER_TARGET_VIEW_CREATION_FAILED,
                "No error message from D3D11, but render target view is "
                "NULL.");
        }

        if (pRTView)
            pRTView->Release();
        if (pBackBuffer)
            pBackBuffer->Release();

        return NULL;
    }

    D3D11SwapChainBufferData* pThis = EE_NEW D3D11SwapChainBufferData;

    pThis->m_pSwapChain = pSwapChain;
    pThis->m_pRenderTargetView = pRTView;
    pThis->m_pTexture = pBackBuffer;

    D3D11_TEXTURE2D_DESC textureDesc;
    pBackBuffer->GetDesc(&textureDesc);

    pThis->m_pkPixelFormat = D3D11PixelFormat::ObtainFromDXGIFormat(textureDesc.Format);

    pThis->m_eMSAAPref = Ni2DBuffer::GetMSAAPrefFromCountAndQuality(
        textureDesc.SampleDesc.Count, 
        textureDesc.SampleDesc.Quality);

    if (pBuffer)
    {
        // Check that pBuffer's dimensions are set up correctly - it may have been initially 
        // created with a different set of dimensions.
        if (pBuffer->GetHeight() != textureDesc.Height ||
            pBuffer->GetWidth() != textureDesc.Width)
        {
            pBuffer = NULL;
        }
    }

    if (pBuffer == NULL)
    {
        // Create the buffer if it did not exist prior to this. Note
        // that it is assumed that the app will refcount this
        // buffer after this has been created otherwise it will leak.
        pBuffer = Ni2DBuffer::Create(textureDesc.Width, textureDesc.Height, pThis);
    }
    else
    {
        pBuffer->SetRendererData(pThis);
    }

    pThis->m_pkBuffer = pBuffer;

    return pThis;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SwapChainBufferData::ResizeSwapChain(efd::UInt32 width, efd::UInt32 height)
{
    DXGI_SWAP_CHAIN_DESC kDesc;
    HRESULT hr = m_pSwapChain->GetDesc(&kDesc);
    if (FAILED(hr))
    {
        D3D11Error::ReportWarning(
            "IDXGISwapChain::GetDesc failed in " 
            __FUNCTION__ 
            " with error HRESULT = 0x%08X.", 
            (efd::UInt32)hr);
        return false;
    }

    m_pRenderTargetView->Release();
    m_pRenderTargetView = NULL;
    m_pTexture->Release();
    m_pTexture = NULL;

    hr = m_pSwapChain->ResizeBuffers(
        kDesc.BufferCount, 
        width, 
        height,
        kDesc.BufferDesc.Format, 
        kDesc.Flags);
    if (FAILED(hr))
    {
        D3D11Error::ReportWarning(
            "IDXGISwapChain::ResizeBuffers failed in " 
            __FUNCTION__ 
            " with error HRESULT = 0x%08X.", 
            (efd::UInt32)hr);

        // Don't return - still need to recreate render target view.
    }

    // Recreate render target view
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT (pRenderer);
    ID3D11Device* pDevice = pRenderer->GetD3D11Device();
    EE_ASSERT (pDevice);

    ID3D11Texture2D* pBackBuffer = NULL;
    hr = m_pSwapChain->GetBuffer(0, __uuidof(*pBackBuffer), (LPVOID*)&pBackBuffer);

    if (FAILED(hr) || pBackBuffer == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_GET_BUFFER_FROM_SWAP_CHAIN_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_GET_BUFFER_FROM_SWAP_CHAIN_FAILED,
                "No error message from D3D11, but buffer is NULL.");
        }

        if (pBackBuffer)
            pBackBuffer->Release();

        return NULL;
    }

    ID3D11RenderTargetView* pRTView = NULL;
    hr = pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRTView);
    if (FAILED(hr) || pRTView == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_RENDER_TARGET_VIEW_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_RENDER_TARGET_VIEW_CREATION_FAILED,
                "No error message from D3D11, but render target view is "
                "NULL.");
        }

        if (pRTView)
            pRTView->Release();
        if (pBackBuffer)
            pBackBuffer->Release();

        return NULL;
    }

    m_pRenderTargetView = pRTView;
    m_pTexture = pBackBuffer;

    D3D11_TEXTURE2D_DESC textureDesc;
    m_pTexture->GetDesc(&textureDesc);

    m_pkPixelFormat = D3D11PixelFormat::ObtainFromDXGIFormat(textureDesc.Format);

    m_pkBuffer->ResetDimensions(textureDesc.Width, textureDesc.Height);

    if (textureDesc.SampleDesc.Count == 1)
        m_eMSAAPref = Ni2DBuffer::MULTISAMPLE_NONE;
    else if (textureDesc.SampleDesc.Count < 4)
        m_eMSAAPref = Ni2DBuffer::MULTISAMPLE_2;
    else
        m_eMSAAPref = Ni2DBuffer::MULTISAMPLE_4;

    return true;
}

//------------------------------------------------------------------------------------------------
IDXGISwapChain* D3D11SwapChainBufferData::GetSwapChain() const
{
    return m_pSwapChain;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SwapChainBufferData::CanDisplayFrame()
{
    return true;
}

//------------------------------------------------------------------------------------------------
HRESULT D3D11SwapChainBufferData::DisplayFrame(efd::UInt32 syncInterval, efd::Bool presentTest)
{
    if (m_pSwapChain == NULL)
        return false;

    efd::UInt32 flags = (presentTest ? DXGI_PRESENT_TEST : 0);

    return m_pSwapChain->Present(syncInterval, flags);
}

//------------------------------------------------------------------------------------------------
NiImplementRTTI(D3D11DepthStencilBufferData, D3D112DBufferData);

//------------------------------------------------------------------------------------------------
D3D11DepthStencilBufferData::D3D11DepthStencilBufferData() :
    m_pDepthStencilView(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11DepthStencilBufferData::~D3D11DepthStencilBufferData()
{
    if (m_pDepthStencilView)
        m_pDepthStencilView->Release();
}

//------------------------------------------------------------------------------------------------
ID3D11DepthStencilView* D3D11DepthStencilBufferData::GetDepthStencilView()
    const
{
    return m_pDepthStencilView;
}

//------------------------------------------------------------------------------------------------
ID3D11Texture2D* D3D11DepthStencilBufferData::GetDepthStencilBuffer() const
{
    return m_pTexture;
}

//------------------------------------------------------------------------------------------------
D3D11DepthStencilBufferData* D3D11DepthStencilBufferData::Create(
    ID3D11Texture2D* pD3DTexture, 
    NiDepthStencilBuffer*& pBuffer,
    D3D11_DEPTH_STENCIL_VIEW_DESC* pDSViewDesc)
{
    if (pD3DTexture == NULL)
        return NULL;

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT (pRenderer);
    ID3D11Device* pDevice = pRenderer->GetD3D11Device();
    EE_ASSERT (pDevice);

    ID3D11DepthStencilView* pDSView = NULL;
    HRESULT hr = pDevice->CreateDepthStencilView(pD3DTexture, pDSViewDesc, &pDSView);
    if (FAILED(hr) || pDSView == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_DEPTH_STENCIL_VIEW_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_DEPTH_STENCIL_VIEW_CREATION_FAILED,
                "No error message from D3D11, but depth stencil view is "
                "NULL.");
        }

        if (pDSView)
            pDSView->Release();

        return NULL;
    }

    D3D11DepthStencilBufferData* pThis = EE_NEW D3D11DepthStencilBufferData;

    pThis->m_pDepthStencilView = pDSView;
    pThis->m_pTexture = pD3DTexture;
    pThis->m_pTexture->AddRef();

    D3D11_TEXTURE2D_DESC textureDesc;
    pD3DTexture->GetDesc(&textureDesc);

    pThis->m_pkPixelFormat = D3D11PixelFormat::ObtainFromDXGIFormat(textureDesc.Format);

    pThis->m_eMSAAPref = Ni2DBuffer::GetMSAAPrefFromCountAndQuality(
        textureDesc.SampleDesc.Count, 
        textureDesc.SampleDesc.Quality);

    if (pBuffer)
    {
        // Check that pBuffer's dimensions are set up correctly - it may have been initially 
        // created with a different set of dimensions.
        if (pBuffer->GetHeight() != textureDesc.Height ||
            pBuffer->GetWidth() != textureDesc.Width)
        {
            pBuffer = NULL;
        }
    }

    if (pBuffer == NULL)
    {
        // Create the buffer if it did not exist prior to this. Note
        // that it is assumed that the app will refcount this
        // buffer after this has been created otherwise it will leak.
        pBuffer = NiDepthStencilBuffer::Create(
            textureDesc.Width, 
            textureDesc.Height,
            pThis);
    }
    else
    {
        pBuffer->SetRendererData(pThis);
    }

    pThis->m_pkBuffer = pBuffer;

    return pThis;
}

//------------------------------------------------------------------------------------------------
