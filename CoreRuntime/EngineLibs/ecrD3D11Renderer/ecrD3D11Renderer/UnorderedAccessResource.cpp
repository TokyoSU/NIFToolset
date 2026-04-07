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

#include "UnorderedAccessResource.h"
#include "D3D11DataStream.h"
#include "D3D11Renderer.h"
#include "D3D11TextureData.h"

using namespace ecr;

//------------------------------------------------------------------------------------------------
UnorderedAccessResource::UnorderedAccessResource() :
    m_uavFlags(0),
    m_pResource(NULL), 
    m_pUAV(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
UnorderedAccessResource::~UnorderedAccessResource()
{
    if (m_pResource)
        m_pResource->Release();
    if (m_pUAV)
        m_pUAV->Release();
}

//------------------------------------------------------------------------------------------------
void UnorderedAccessResource::UpdateUnorderedAccessView()
{
    if (m_spDataStream != NULL)
    {
        EE_ASSERT(m_spTexture == NULL);
        NiDataStream* pDataStream = m_spDataStream;
        D3D11DataStream* pD3D11DataStream = NiVerifyStaticCast(D3D11DataStream, pDataStream);
        ID3D11Resource* pResource = (ID3D11Resource*)pD3D11DataStream->GetBuffer();

        if (pResource != m_pResource)
        {
            ReleaseResourceAndUAV();
            SetResourceAndUAV(pResource, NULL);
        }
    }
    else if (m_spTexture != NULL)
    {
        D3D11TextureData* pTextureData = (D3D11TextureData*)m_spTexture->GetRendererData();
        if (pTextureData != NULL)
        {
            ID3D11Resource* pResource = pTextureData->GetResource();
            if (pResource != m_pResource)
            {
                ReleaseResourceAndUAV();
                SetResourceAndUAV(pResource, NULL);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
void UnorderedAccessResource::SetResourceAndUAV(
    ID3D11Resource* pResource, 
    ID3D11UnorderedAccessView* pUAV)
{
    if (pResource == NULL)
        return;

    if (pUAV)
    {
        pUAV->AddRef();
    }
    else
    {
        D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
        if (pRenderer == NULL)
            return;

        ID3D11Device* pD3D11Device = pRenderer->GetD3D11Device();
        if (pD3D11Device == NULL)
            return;

        // Create a new UAV
        D3D11_RESOURCE_DIMENSION resourceType = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        pResource->GetType(&resourceType);
        EE_ASSERT(resourceType != D3D11_RESOURCE_DIMENSION_UNKNOWN);

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        memset(&uavDesc, 0, sizeof(uavDesc));
        if (resourceType == D3D11_RESOURCE_DIMENSION_BUFFER)
        {
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement = 0;

            ID3D11Buffer* pBuffer = (ID3D11Buffer*)pResource;
            D3D11_BUFFER_DESC bufferDesc;
            pBuffer->GetDesc(&bufferDesc);

            if ((bufferDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) == 0)
            {
                D3D11Error::ReportError(
                    D3D11Error::D3D11ERROR_UNORDERED_ACCESS_VIEW_CREATION_FAILED,
                    __FUNCTION__
                    " cannot create an unordered access view for a resource created without the"
                    " D3D11_BIND_UNORDERED_ACCESS flag.");
                return;
            }

            if ((bufferDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS) != 0)
            {
                // Raw data
                m_uavFlags = D3D11_BUFFER_UAV_FLAG_RAW; // Other flags not valid for raw data
                uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
                uavDesc.Buffer.NumElements = bufferDesc.ByteWidth / 4; 
            }
            else if ((bufferDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED))
            {
                // Structured data
                m_uavFlags &= ~D3D11_BUFFER_UAV_FLAG_RAW;
                uavDesc.Format = DXGI_FORMAT_UNKNOWN;
                EE_ASSERT(bufferDesc.StructureByteStride != 0);
                uavDesc.Buffer.NumElements = bufferDesc.ByteWidth / bufferDesc.StructureByteStride;
            }
            else
            {
                // Will need to update this code if this assertion is ever hit...
                EE_FAIL("Buffer exists without either D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS "
                    "or D3D11_RESOURCE_MISC_BUFFER_STRUCTURED.");
            }

            uavDesc.Buffer.Flags = m_uavFlags;
        }
        else if (resourceType == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
        {
            ID3D11Texture1D* pTexture = (ID3D11Texture1D*)pResource;
            D3D11_TEXTURE1D_DESC textureDesc;
            pTexture->GetDesc(&textureDesc);

            if ((textureDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) == 0)
            {
                D3D11Error::ReportError(
                    D3D11Error::D3D11ERROR_UNORDERED_ACCESS_VIEW_CREATION_FAILED,
                    __FUNCTION__
                    " cannot create an unordered access view for a resource created without the"
                    " D3D11_BIND_UNORDERED_ACCESS flag.");
                return;
            }

            uavDesc.Format = textureDesc.Format;

            EE_ASSERT(textureDesc.ArraySize != 0);
            if (textureDesc.ArraySize == 1)
            {
                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;

                uavDesc.Texture1D.MipSlice = 0;
            }
            else
            {
                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;

                uavDesc.Texture1DArray.MipSlice = 0;
                uavDesc.Texture1DArray.FirstArraySlice = 0;
                uavDesc.Texture1DArray.ArraySize = textureDesc.ArraySize;
            }
        }
        else if (resourceType == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        {
            ID3D11Texture2D* pTexture = (ID3D11Texture2D*)pResource;
            D3D11_TEXTURE2D_DESC textureDesc;
            pTexture->GetDesc(&textureDesc);

            if ((textureDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) == 0)
            {
                D3D11Error::ReportError(
                    D3D11Error::D3D11ERROR_UNORDERED_ACCESS_VIEW_CREATION_FAILED,
                    __FUNCTION__
                    " cannot create an unordered access view for a resource created without the"
                    " D3D11_BIND_UNORDERED_ACCESS flag.");
                return;
            }

            uavDesc.Format = textureDesc.Format;

            EE_ASSERT(textureDesc.ArraySize != 0);
            if (textureDesc.ArraySize == 1)
            {
                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

                uavDesc.Texture2D.MipSlice = 0;
            }
            else
            {
                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;

                uavDesc.Texture2DArray.MipSlice = 0;
                uavDesc.Texture2DArray.FirstArraySlice = 0;
                uavDesc.Texture2DArray.ArraySize = textureDesc.ArraySize;
            }
        }
        else if (resourceType == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
        {
            ID3D11Texture3D* pTexture = (ID3D11Texture3D*)pResource;
            D3D11_TEXTURE3D_DESC textureDesc;
            pTexture->GetDesc(&textureDesc);

            if ((textureDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) == 0)
            {
                D3D11Error::ReportError(
                    D3D11Error::D3D11ERROR_UNORDERED_ACCESS_VIEW_CREATION_FAILED,
                    __FUNCTION__
                    " cannot create an unordered access view for a resource created without the"
                    " D3D11_BIND_UNORDERED_ACCESS flag.");
                return;
            }

            uavDesc.Format = textureDesc.Format;

            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;

            uavDesc.Texture3D.MipSlice = 0;
            uavDesc.Texture3D.FirstWSlice = 0;
            uavDesc.Texture3D.WSize = textureDesc.Depth;
        }

        pUAV = NULL;
        HRESULT hr = pD3D11Device->CreateUnorderedAccessView(
            pResource, 
            &uavDesc, 
            &pUAV);
        if (FAILED(hr) || pUAV == NULL)
        {
            if (FAILED(hr))
            {
                D3D11Error::ReportError(
                    D3D11Error::D3D11ERROR_UNORDERED_ACCESS_VIEW_CREATION_FAILED,
                    "Error HRESULT = 0x%08X.", 
                    (efd::UInt32)hr);
            }
            else
            {
                D3D11Error::ReportError(
                    D3D11Error::D3D11ERROR_UNORDERED_ACCESS_VIEW_CREATION_FAILED,
                    "No error message from D3D11, but unordered access view is NULL.");
            }

            if (pUAV)
            {
                pUAV->Release();
                pUAV = NULL;
            }
        }


    }
    m_pUAV = pUAV;

    m_pResource = pResource;
    m_pResource->AddRef();
}

//------------------------------------------------------------------------------------------------
