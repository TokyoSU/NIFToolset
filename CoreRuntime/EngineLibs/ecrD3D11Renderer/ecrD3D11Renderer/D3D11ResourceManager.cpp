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

#include "D3D11ResourceManager.h"
#include "D3D11Error.h"

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11ResourceManager::D3D11ResourceManager(ID3D11Device* pDevice) :
    m_pDevice(pDevice)
{
    EE_ASSERT (m_pDevice);
    if (m_pDevice)
        m_pDevice->AddRef();
}

//------------------------------------------------------------------------------------------------
D3D11ResourceManager::~D3D11ResourceManager()
{
    if (m_pDevice)
        m_pDevice->Release();
}

//------------------------------------------------------------------------------------------------
ID3D11Buffer* D3D11ResourceManager::CreateBuffer(
    efd::UInt32 bufferSize,
    efd::UInt32 bindFlags, 
    D3D11_USAGE usage,
    efd::UInt32 cpuAccessFlags, 
    efd::UInt32 miscFlags,
    efd::UInt32 structureByteStride,
    D3D11_SUBRESOURCE_DATA* pInitialData)
{
    ID3D11Buffer* pBuffer = NULL;
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.ByteWidth = bufferSize;
    bufferDesc.BindFlags = bindFlags;
    bufferDesc.Usage = usage;
    bufferDesc.CPUAccessFlags = cpuAccessFlags;
    bufferDesc.MiscFlags = miscFlags;
    bufferDesc.StructureByteStride = structureByteStride;

    HRESULT hr = m_pDevice->CreateBuffer(&bufferDesc, pInitialData, &pBuffer);
    if (FAILED(hr) || pBuffer == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_BUFFER_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_BUFFER_CREATION_FAILED,
                "No error message from D3D11, but buffer is NULL.");
        }

        if (pBuffer)
        {
            pBuffer->Release();
            pBuffer = NULL;
        }
    }

    return pBuffer;
}

//------------------------------------------------------------------------------------------------
ID3D11Texture1D* D3D11ResourceManager::CreateTexture1D(
    efd::UInt32 width,
    efd::UInt32 mipLevels,
    efd::UInt32 arraySize, 
    DXGI_FORMAT format,
    D3D11_USAGE usage, 
    efd::UInt32 bindFlags,
    efd::UInt32 cpuAccessFlags, 
    efd::UInt32 miscFlags,
    D3D11_SUBRESOURCE_DATA* pInitialData)
{
    ID3D11Texture1D* pTexture = NULL;
    D3D11_TEXTURE1D_DESC texturedesc;
    texturedesc.Width = width;
    texturedesc.MipLevels = mipLevels;
    texturedesc.ArraySize = arraySize;
    texturedesc.Format = format;
    texturedesc.Usage = usage;
    texturedesc.BindFlags = bindFlags;
    texturedesc.CPUAccessFlags = cpuAccessFlags;
    texturedesc.MiscFlags = miscFlags;

    HRESULT hr = m_pDevice->CreateTexture1D(&texturedesc, pInitialData, &pTexture);
    if (FAILED(hr) || pTexture == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_TEXTURE1D_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_TEXTURE1D_CREATION_FAILED,
                "No error message from D3D11, but texture is NULL.");
        }

        if (pTexture)
        {
            pTexture->Release();
            pTexture = NULL;
        }
    }

    return pTexture;
}
//------------------------------------------------------------------------------------------------
ID3D11Texture2D* D3D11ResourceManager::CreateTexture2D(
    efd::UInt32 width,
    efd::UInt32 height, 
    efd::UInt32 mipLevels,
    efd::UInt32 arraySize, 
    DXGI_FORMAT format,
    efd::UInt32 msaaCount, 
    efd::UInt32 msaaQuality,
    D3D11_USAGE usage, 
    efd::UInt32 bindFlags,
    efd::UInt32 cpuAccessFlags, 
    efd::UInt32 miscFlags,
    D3D11_SUBRESOURCE_DATA* pInitialData)
{
    ID3D11Texture2D* pTexture = NULL;
    D3D11_TEXTURE2D_DESC texturedesc;
    texturedesc.Width = width;
    texturedesc.Height = height;
    texturedesc.MipLevels = mipLevels;
    texturedesc.ArraySize = arraySize;
    texturedesc.Format = format;
    texturedesc.SampleDesc.Count = msaaCount;
    texturedesc.SampleDesc.Quality = msaaQuality;
    texturedesc.Usage = usage;
    texturedesc.BindFlags = bindFlags;
    texturedesc.CPUAccessFlags = cpuAccessFlags;
    texturedesc.MiscFlags = miscFlags;

    HRESULT hr = m_pDevice->CreateTexture2D(&texturedesc, pInitialData, &pTexture);
    if (FAILED(hr) || pTexture == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_TEXTURE2D_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_TEXTURE2D_CREATION_FAILED,
                "No error message from D3D11, but texture is NULL.");
        }

        if (pTexture)
        {
            pTexture->Release();
            pTexture = NULL;
        }
    }

    return pTexture;
}

//------------------------------------------------------------------------------------------------
ID3D11Texture3D* D3D11ResourceManager::CreateTexture3D(
    efd::UInt32 width,
    efd::UInt32 height, 
    efd::UInt32 depth, 
    efd::UInt32 mipLevels,
    DXGI_FORMAT format,
    D3D11_USAGE usage, 
    efd::UInt32 bindFlags,
    efd::UInt32 cpuAccessFlags, 
    efd::UInt32 miscFlags,
    D3D11_SUBRESOURCE_DATA* pInitialData)
{
    ID3D11Texture3D* pTexture = NULL;
    D3D11_TEXTURE3D_DESC texturedesc;
    texturedesc.Width = width;
    texturedesc.Height = height;
    texturedesc.Depth = depth;
    texturedesc.MipLevels = mipLevels;
    texturedesc.Format = format;
    texturedesc.Usage = usage;
    texturedesc.BindFlags = bindFlags;
    texturedesc.CPUAccessFlags = cpuAccessFlags;
    texturedesc.MiscFlags = miscFlags;

    HRESULT hr = m_pDevice->CreateTexture3D(&texturedesc, pInitialData, &pTexture);
    if (FAILED(hr) || pTexture == NULL)
    {
        if (FAILED(hr))
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_TEXTURE3D_CREATION_FAILED,
                "Error HRESULT = 0x%08X.", 
                (efd::UInt32)hr);
        }
        else
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_TEXTURE3D_CREATION_FAILED,
                "No error message from D3D11, but texture is NULL.");
        }

        if (pTexture)
        {
            pTexture->Release();
            pTexture = NULL;
        }
    }

    return pTexture;
}

//------------------------------------------------------------------------------------------------
