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

#include "D3D11TextureData.h"
#include "D3D11Error.h"
#include "D3D11PixelFormat.h"
#include "D3D11Renderer.h"

#include <NiImageConverter.h>

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11TextureData::D3D11TextureData(NiTexture* pTexture) :
    NiTexture::RendererData(pTexture),
    m_levels(0),
    m_resourceType(0),
    m_depthOrArrayCount(0),
    m_pResourceView(NULL),
    m_pD3D11Resource(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11TextureData::~D3D11TextureData()
{
    if (m_pResourceView)
        m_pResourceView->Release();

    if (m_pD3D11Resource)
        m_pD3D11Resource->Release();
}

//------------------------------------------------------------------------------------------------
void D3D11TextureData::ClearTextureData()
{
    NiTexture* pTexture = NiTexture::GetListHead();

    while (pTexture)
    {
        D3D11TextureData* pData = (D3D11TextureData*)(pTexture->GetRendererData());

        if (pData)
        {
            pTexture->SetRendererData(NULL);
            EE_DELETE pData;
        }

        pTexture = pTexture->GetListNext();
    }
}

//------------------------------------------------------------------------------------------------
void D3D11TextureData::SetResourceView(ID3D11ShaderResourceView* pResourceView)
{
    if (pResourceView == m_pResourceView)
        return;

    if (pResourceView)
        pResourceView->AddRef();
    if (m_pResourceView)
        m_pResourceView->Release();
    m_pResourceView = pResourceView;
}

//------------------------------------------------------------------------------------------------
const NiPixelFormat* D3D11TextureData::FindMatchingPixelFormat(
    const NiPixelFormat& srcFmt, 
    const NiTexture::FormatPrefs& formatPrefs,
    D3D11_FORMAT_SUPPORT supportTest)
{
    // * Must always select a pixel format that exists for the renderer
    // * We must select format pairs that can be converted between.  Use a
    // less desirable format if the desirable format cannot supported by the
    // current image converter
    // * In general, prefer matching the desires of the format prefs

    NiPixelFormat::Format format = srcFmt.GetFormat();
    NiImageConverter* pConvert = NiImageConverter::GetImageConverter();
    const NiPixelFormat* pDestFmt = NULL;

    // D3D11 renderer doesn't deal with tiled platform-specific textures.
    if (srcFmt.GetTiling() != NiPixelFormat::TILE_NONE)
        return NULL;

    // D3D11 renderer doesn't deal with Big Endian platform-specific formats.
    if (!srcFmt.GetLittleEndian())
        return NULL;

    // Renderer must exist.
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL)
        return NULL;

    // Examine existing data format.
    // Assume that all channels are basically equivalent.
    NiPixelFormat::Component component = NiPixelFormat::COMP_EMPTY;
    NiPixelFormat::Representation representation = NiPixelFormat::REP_UNKNOWN;
    efd::UInt8 bitsPerComponent = 0;
    efd::Bool isSigned = false;

    if (srcFmt.GetComponent(0, component, representation, bitsPerComponent, isSigned))
    {
        if (representation == NiPixelFormat::REP_COMPRESSED)
        {
            // If it's compressed, leave it as such unless it's supported
            // or an RGBA format is requested
            if ((formatPrefs.m_ePixelLayout != NiTexture::FormatPrefs::HIGH_COLOR_16) &&
                (formatPrefs.m_ePixelLayout != NiTexture::FormatPrefs::TRUE_COLOR_32))
            {
                if (format == NiPixelFormat::FORMAT_DXT1)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC1_UNORM;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
                else if (format == NiPixelFormat::FORMAT_DXT3)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC2_UNORM;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
                else if (format == NiPixelFormat::FORMAT_DXT5)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC3_UNORM;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
            }
        }
        else if (representation == NiPixelFormat::REP_FLOAT ||
            representation == NiPixelFormat::REP_HALF)
        {
            EE_ASSERT((representation == NiPixelFormat::REP_FLOAT && bitsPerComponent == 32) ||
                representation == NiPixelFormat::REP_HALF && bitsPerComponent == 16);
            efd::UInt32 uiNumComponents = srcFmt.GetNumComponents();

            if (bitsPerComponent == 16)
            {
                // Fall back to increasing size, then increasing channels
                if (uiNumComponents < 2)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16_FLOAT;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32_FLOAT;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
                if (uiNumComponents < 3)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16_FLOAT;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32_FLOAT;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
                if (uiNumComponents < 4)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32B32_FLOAT;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_FLOAT;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32B32A32_FLOAT;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }
            }
            else if (bitsPerComponent == 32)
            {
                // Fall back to increasing channels, then decreasing size
                if (uiNumComponents < 2)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32_FLOAT;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
                if (uiNumComponents < 3)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32_FLOAT;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
                if (uiNumComponents < 4)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32B32_FLOAT;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32B32A32_FLOAT;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }
                if (uiNumComponents < 2)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16_FLOAT;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
                if (uiNumComponents < 3)
                {
                    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16_FLOAT;
                    if (pRenderer->DoesFormatSupportFlag(
                        (DXGI_FORMAT)pDestFmt->GetRendererHint(),
                        supportTest) &&
                        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                    {
                        return pDestFmt;
                    }
                }
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_FLOAT;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }
            }
        }
        else if (representation == NiPixelFormat::REP_INT)
        {
            // A non-normalized texture would fall in here.
            EE_FAIL("D3D11 renderer needs to support non-normalized textures.");
        }
    }

    // Default to RGBA case
    if (formatPrefs.m_ePixelLayout == NiTexture::FormatPrefs::BUMPMAP)
    {
        // Bump map -> isSigned normalized
        if ((format == NiPixelFormat::FORMAT_BUMPLUMA) ||
            (format == NiPixelFormat::FORMAT_RGBA))
        {
            // 4-component 32-bit
            pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8G8B8A8_SNORM;
            if (pRenderer->DoesFormatSupportFlag(
                (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                supportTest) &&
                pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
            {
                return pDestFmt;
            }

            // 4-component 64-bit
            pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_SNORM;
            if (pRenderer->DoesFormatSupportFlag(
                (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                supportTest) &&
                pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
            {
                return pDestFmt;
            }

            // 2-component 32-bit
            pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16_SNORM;
            if (pRenderer->DoesFormatSupportFlag(
                (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                supportTest) &&
                pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
            {
                return pDestFmt;
            }

            // 2-component 16-bit
            pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8G8_SNORM;
            if (pRenderer->DoesFormatSupportFlag(
                (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                supportTest) &&
                pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
            {
                return pDestFmt;
            }

            // 2-component compressed
            pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC5_SNORM;
            if (pRenderer->DoesFormatSupportFlag(
                (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                supportTest) &&
                pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
            {
                return pDestFmt;
            }
        }
        else
        {
            // 2-component 32-bit
            pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16_SNORM;
            if (pRenderer->DoesFormatSupportFlag(
                (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                supportTest) &&
                pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
            {
                return pDestFmt;
            }

            // 2-component 16-bit
            pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8G8_SNORM;
            if (pRenderer->DoesFormatSupportFlag(
                (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                supportTest) &&
                pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
            {
                return pDestFmt;
            }

            // 2-component compressed
            pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC5_SNORM;
            if (pRenderer->DoesFormatSupportFlag(
                (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                supportTest) &&
                pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
            {
                return pDestFmt;
            }

            // 4-component 32-bit
            pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8G8B8A8_SNORM;
            if (pRenderer->DoesFormatSupportFlag(
                (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                supportTest) &&
                pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
            {
                return pDestFmt;
            }

            // 4-component 64-bit
            pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_SNORM;
            if (pRenderer->DoesFormatSupportFlag(
                (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                supportTest) &&
                pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
            {
                return pDestFmt;
            }
        }
        // No other signed, normalized formats support more than one channel -
        // return NULL
        return NULL;
    }
    if (formatPrefs.m_ePixelLayout == NiTexture::FormatPrefs::COMPRESSED)
    {
        if (formatPrefs.m_eAlphaFmt == NiTexture::FormatPrefs::BINARY)
        {
            // Compressed formats
            if (srcFmt.GetSRGBSpace())
            {
                // SRGB space

                // BC1
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC1_UNORM_SRGB;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }

                // BC2
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC2_UNORM_SRGB;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }

                // BC3
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC3_UNORM_SRGB;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }
            }
            else
            {
                // Linear RGB space

                // BC1
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC1_UNORM;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }

                // BC2
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC2_UNORM;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }

                // BC3
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC3_UNORM;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }
            }
        }
        else
        {
            // Compressed formats
            if (srcFmt.GetSRGBSpace())
            {
                // SRGB space

                // BC2
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC2_UNORM_SRGB;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }

                // BC3
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC3_UNORM_SRGB;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }

                // BC1
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC1_UNORM_SRGB;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }
            }
            else
            {
                // Linear RGB space

                // BC2
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC2_UNORM;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }

                // BC3
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC3_UNORM;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }

                // BC1
                pDestFmt = &D3D11PixelFormat::EE_FORMAT_BC1_UNORM;
                if (pRenderer->DoesFormatSupportFlag(
                    (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
                    supportTest) &&
                    pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
                {
                    return pDestFmt;
                }
            }
        }
    }

    // Default - RGBA, unsigned normalized values
    // D3D11 has no 16-bit or non-alpha formats, so the format prefs have
    // little relevance anymore.

    if (srcFmt.GetSRGBSpace())
    {
        // SRGB space

        // 4-component 32-bit
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8G8B8A8_UNORM_SRGB;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(),
            supportTest) &&
            pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
        {
            return pDestFmt;
        }

        // Fall through to linear space
    }

    // Linear space

    // 4-component 32-bit
    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8G8B8A8_UNORM;
    if (pRenderer->DoesFormatSupportFlag(
        (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
        supportTest) &&
        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
    {
        return pDestFmt;
    }

    // 4-component 32-bit with 2-bit alpha
    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R10G10B10A2_UNORM;
    if (pRenderer->DoesFormatSupportFlag(
        (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
        supportTest) &&
        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
    {
        return pDestFmt;
    }

    // 4-component 64-bit
    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_UNORM;
    if (pRenderer->DoesFormatSupportFlag(
        (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
        supportTest) &&
        pConvert->CanConvertPixelData(srcFmt, *pDestFmt))
    {
        return pDestFmt;
    }

    // Give up
    return NULL;
}

//------------------------------------------------------------------------------------------------
const NiPixelFormat* D3D11TextureData::FindMatchingPixelFormat(
    const NiTexture::FormatPrefs& formatPrefs, 
    D3D11_FORMAT_SUPPORT supportTest)
{
    // * Must always select a pixel format that exists for the renderer
    // * We must select format pairs that can be converted between.  Use a
    // less desirable format if the desirable format cannot supported by the
    // current image converter
    // * In general, prefer matching the desires of the format prefs

    const NiPixelFormat* pDestFmt = NULL;

    // Renderer must exist.
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL)
        return NULL;

    switch (formatPrefs.m_ePixelLayout)
    {
    case NiTexture::FormatPrefs::BUMPMAP:
        // 4-component 32-bit
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8G8B8A8_SNORM;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 64-bit
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_SNORM;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 2-component 32-bit
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16_SNORM;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 2-component 16-bit
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8G8_SNORM;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // Fall back to 32-bit unsigned formats.
        break;
    case NiTexture::FormatPrefs::SINGLE_COLOR_8:
        // Assume normalized integer
        // Start with smaller formats, then fall back to larger ones
        // Then fall back to more channels

        // 1-component 8-bit norm
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8_UNORM;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 1-component 16-bit norm
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16_UNORM;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 2-component 8-bit norm
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8G8_UNORM;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 2-component 16-bit norm
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16_UNORM;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // Fall back to 32-bit unsigned formats.
        break;
    case NiTexture::FormatPrefs::SINGLE_COLOR_16:
        // Assume float
        // Start with smaller formats, then fall back to larger ones
        // Then fall back to more channels
        // 1-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 1-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 2-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 2-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32B32A32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // Fall back to 32-bit unsigned formats.
        break;
    case NiTexture::FormatPrefs::SINGLE_COLOR_32:
        // Assume float
        // Start with larger formats, then fall back to more channels
        // Then fall back to smaller formats
        // 1-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 2-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32B32A32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 1-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 2-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // Fall back to 32-bit unsigned formats.
        break;
    case NiTexture::FormatPrefs::DOUBLE_COLOR_32:
        // Assume float
        // Start with smaller formats, then fall back to larger ones
        // Then fall back to more channels
        // 2-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 2-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32B32A32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // Fall back to 32-bit unsigned formats.
        break;
    case NiTexture::FormatPrefs::DOUBLE_COLOR_64:
        // Assume float
        // Start with larger formats, then fall back to more channels
        // Then fall back to smaller formats
        // 2-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32B32A32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 2-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // Fall back to 32-bit unsigned formats.
        break;
    case NiTexture::FormatPrefs::FLOAT_COLOR_32:
    case NiTexture::FormatPrefs::FLOAT_COLOR_64:
        // Assume float
        // Start with smaller format, then fall back to larger one
        // 4-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32B32A32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // Fall back to 32-bit unsigned formats.
        break;
    case NiTexture::FormatPrefs::FLOAT_COLOR_128:
        // Assume float
        // Start with larger format, then fall back to smaller one
        // 4-component 32-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R32G32B32A32_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // 4-component 16-bit float
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R16G16B16A16_FLOAT;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }

        // Fall back to 32-bit unsigned formats.
        break;

    case NiTexture::FormatPrefs::HIGH_COLOR_16:
    case NiTexture::FormatPrefs::PALETTIZED_8:
    case NiTexture::FormatPrefs::PALETTIZED_4:
    case NiTexture::FormatPrefs::COMPRESSED:
        // Not supported - fall through to 32-bit unsigned formats.
    case NiTexture::FormatPrefs::TRUE_COLOR_32:
    case NiTexture::FormatPrefs::PIX_DEFAULT:
        break;
    }

    // At this point, either the requested format is not supported or a
    // 32-bit unsigned format was requested.

    if (formatPrefs.m_ePixelLayout == NiTexture::FormatPrefs::BINARY)
    {
        // Try 10-10-10-2
        pDestFmt = &D3D11PixelFormat::EE_FORMAT_R10G10B10A2_UNORM;
        if (pRenderer->DoesFormatSupportFlag(
            (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
            supportTest))
        {
            return pDestFmt;
        }
    }

    // 4-component 32-bit norm
    pDestFmt = &D3D11PixelFormat::EE_FORMAT_R8G8B8A8_UNORM;
    if (pRenderer->DoesFormatSupportFlag(
        (DXGI_FORMAT)pDestFmt->GetRendererHint(), 
        supportTest))
    {
        return pDestFmt;
    }

    // Give up
    return NULL;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11TextureData::InitializeFromD3D11Resource(
    ID3D11Resource* pD3D11Resource, 
    ID3D11ShaderResourceView* pResourceView)
{
    if (pD3D11Resource == NULL)
        return false;

    if (pResourceView == NULL)
    {
        // Newly created resource view will have a reference to it; transfer that reference to
        // the D3D11TextureData object rather than adding a reference now.
        pResourceView = CreateDefaultResourceView(pD3D11Resource);
        if (pResourceView == NULL)
            return false;
    }
    else
    {
        // Acquire new reference to passed-in resource view
        pResourceView->AddRef();
    }

    m_pResourceView = pResourceView;

    // Acquire new reference to passed-in resource
    m_pD3D11Resource = pD3D11Resource;
    m_pD3D11Resource->AddRef();

    // Set the appropriate flags for the resource dimension
    m_resourceType &= ~RESOURCETYPE_DIMENSIONMASK;

    D3D11_RESOURCE_DIMENSION resourceType = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    m_pD3D11Resource->GetType(&resourceType);
    EE_ASSERT(resourceType != D3D11_RESOURCE_DIMENSION_UNKNOWN);

    if (resourceType == D3D11_RESOURCE_DIMENSION_BUFFER)
    {
        m_resourceType |= RESOURCETYPE_BUFFER;
    }
    else if (resourceType == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
    {
        m_resourceType |= RESOURCETYPE_1D;
    }
    else if (resourceType == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
        m_resourceType |= RESOURCETYPE_2D;
        // Check for cube
        ID3D11Texture2D* pTexture = (ID3D11Texture2D*)m_pD3D11Resource;
        D3D11_TEXTURE2D_DESC textureDesc;
        pTexture->GetDesc(&textureDesc);
        if ((textureDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0)
            m_resourceType |= RESOURCETYPE_CUBE;

        m_levels = textureDesc.MipLevels;

        m_uiWidth = textureDesc.Width;
        m_uiHeight = textureDesc.Height;
        
        m_depthOrArrayCount = textureDesc.ArraySize;

        D3D11PixelFormat::InitFromDXGIFormat(textureDesc.Format, m_kPixelFormat);
    }
    else if (resourceType == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
    {
        m_resourceType |= RESOURCETYPE_3D;
    }
     
    return true;
}

//------------------------------------------------------------------------------------------------
ID3D11ShaderResourceView* D3D11TextureData::CreateDefaultResourceView(ID3D11Resource* pResource)
{
    if (pResource == NULL)
        return NULL;

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL)
        return NULL;

    ID3D11Device* pD3D11Device = pRenderer->GetD3D11Device();
    if (pD3D11Device == NULL)
        return NULL;

    // Construct an appropriate resource view
    D3D11_RESOURCE_DIMENSION resourceType = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&resourceType);
    EE_ASSERT(resourceType != D3D11_RESOURCE_DIMENSION_UNKNOWN);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    memset(&srvDesc, 0, sizeof(srvDesc));
    if (resourceType == D3D11_RESOURCE_DIMENSION_BUFFER)
    {
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.ElementOffset = 0;

        ID3D11Buffer* pBuffer = (ID3D11Buffer*)pResource;
        D3D11_BUFFER_DESC bufferDesc;
        pBuffer->GetDesc(&bufferDesc);

        if ((bufferDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0)
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SHADER_RESOURCE_VIEW_CREATION_FAILED,
                " Cannot create an shader resource view for a resource created without the"
                " D3D11_BIND_SHADER_RESOURCE flag.");
            return nullptr;
        }

        if ((bufferDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS) != 0)
        {
            // Raw data
            srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            srvDesc.Buffer.ElementWidth = bufferDesc.ByteWidth / 4; 
        }
        else if ((bufferDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED))
        {
            // Structured data
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            EE_ASSERT(bufferDesc.StructureByteStride != 0);
            srvDesc.Buffer.ElementWidth = bufferDesc.ByteWidth / bufferDesc.StructureByteStride;
        }
        else
        {
            // Will need to update this code if this assertion is ever hit...
            EE_FAIL("Buffer exists without either D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS "
                "or D3D11_RESOURCE_MISC_BUFFER_STRUCTURED.");
        }
    }
    else if (resourceType == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
    {
        ID3D11Texture1D* pTexture = (ID3D11Texture1D*)pResource;
        D3D11_TEXTURE1D_DESC textureDesc;
        pTexture->GetDesc(&textureDesc);

        if ((textureDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0)
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SHADER_RESOURCE_VIEW_CREATION_FAILED,
                " Cannot create an shader resource view for a resource created without the"
                " D3D11_BIND_SHADER_RESOURCE flag.");
            return nullptr;
        }

        srvDesc.Format = textureDesc.Format;

        EE_ASSERT(textureDesc.ArraySize != 0);
        if (textureDesc.ArraySize == 1)
        {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;

            srvDesc.Texture1D.MostDetailedMip = 0;
            srvDesc.Texture1D.MipLevels = textureDesc.MipLevels;
        }
        else
        {
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;

            srvDesc.Texture1DArray.MostDetailedMip = 0;
            srvDesc.Texture1DArray.MipLevels = textureDesc.MipLevels;
            srvDesc.Texture1DArray.FirstArraySlice = 0;
            srvDesc.Texture1DArray.ArraySize = textureDesc.ArraySize;
        }
    }
    else if (resourceType == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
        ID3D11Texture2D* pTexture = (ID3D11Texture2D*)pResource;
        D3D11_TEXTURE2D_DESC textureDesc;
        pTexture->GetDesc(&textureDesc);

        if ((textureDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0)
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SHADER_RESOURCE_VIEW_CREATION_FAILED,
                " Cannot create an shader resource view for a resource created without the"
                " D3D11_BIND_SHADER_RESOURCE flag.");
            return nullptr;
        }

        srvDesc.Format = textureDesc.Format;

        EE_ASSERT(textureDesc.ArraySize != 0);
        if (textureDesc.ArraySize == 1)
        {
            if (textureDesc.SampleDesc.Count != 1 ||
                textureDesc.SampleDesc.Quality != 0)
            {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                // Nothing more to set
            }
            else
            {
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
            }
        }
        else
        {
            if ((textureDesc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) != 0)
            {
                if (textureDesc.ArraySize == 6)
                {
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;

                    srvDesc.TextureCube.MostDetailedMip = 0;
                    srvDesc.TextureCube.MipLevels = textureDesc.MipLevels;
                }
                else
                {
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;

                    EE_ASSERT((textureDesc.ArraySize % 6) == 0);
                    srvDesc.TextureCubeArray.MostDetailedMip = 0;
                    srvDesc.TextureCubeArray.MipLevels = textureDesc.MipLevels;
                    srvDesc.TextureCubeArray.First2DArrayFace = 0;
                    srvDesc.TextureCubeArray.NumCubes = textureDesc.ArraySize / 6;
                }
            }
            else
            {
                if (textureDesc.SampleDesc.Count != 1 ||
                    textureDesc.SampleDesc.Quality != 0)
                {
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;

                    srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                    srvDesc.Texture2DMSArray.ArraySize = textureDesc.ArraySize;
                }
                else
                {
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;

                    srvDesc.Texture2D.MostDetailedMip = 0;
                    srvDesc.Texture2D.MipLevels = textureDesc.MipLevels;
                    srvDesc.Texture2DArray.FirstArraySlice = 0;
                    srvDesc.Texture2DArray.ArraySize = textureDesc.ArraySize;
                }
            }
        }
    }
    else if (resourceType == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
    {
        ID3D11Texture3D* pTexture = (ID3D11Texture3D*)pResource;
        D3D11_TEXTURE3D_DESC textureDesc;
        pTexture->GetDesc(&textureDesc);

        if ((textureDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0)
        {
            D3D11Error::ReportError(
                D3D11Error::D3D11ERROR_SHADER_RESOURCE_VIEW_CREATION_FAILED,
                " Cannot create an shader resource view for a resource created without the"
                " D3D11_BIND_SHADER_RESOURCE flag.");
            return nullptr;
        }

        srvDesc.Format = textureDesc.Format;

        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;

        srvDesc.Texture3D.MostDetailedMip = 0;
        srvDesc.Texture3D.MipLevels = textureDesc.MipLevels;
    }

    ID3D11ShaderResourceView* pResourceView = NULL;
    HRESULT hr = pD3D11Device->CreateShaderResourceView(
        pResource, 
        &srvDesc, 
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
            pResourceView = NULL;
        }
    }

    return pResourceView;
}

//------------------------------------------------------------------------------------------------
