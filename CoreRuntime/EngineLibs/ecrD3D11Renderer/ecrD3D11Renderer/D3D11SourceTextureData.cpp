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

#include "D3D11PixelFormat.h"
#include "D3D11Renderer.h"
#include "D3D11ResourceManager.h"
#include "D3D11SourceTextureData.h"
#include "D3D11PersistentSrcTextureRendererData.h"
#include "D3D11Error.h"

#include <NiFilename.h>

#include <NiImageConverter.h>
#include <NiSourceCubeMap.h>
#include <NiSourceTexture.h>

using namespace ecr;

namespace
{
class ScopedCOMInitialization
{
public:
    ScopedCOMInitialization() :
        m_result(CoInitializeEx(NULL, COINIT_MULTITHREADED))
    {
    }

    ~ScopedCOMInitialization()
    {
        if (SUCCEEDED(m_result))
            CoUninitialize();
    }

private:
    HRESULT m_result;
};
}

efd::UInt16 D3D11SourceTextureData::ms_skipLevels = 0;

//------------------------------------------------------------------------------------------------
D3D11SourceTextureData::D3D11SourceTextureData(NiSourceTexture* pTexture) : 
    D3D11TextureData(pTexture),
    m_replacementData(false),
    m_mipmap(false),
    m_sourceRevID(0),
    m_formattedSize(0),
    m_levelsSkipped(0),
    m_numTextures(0),
    m_paletteRevID(0)
{
    m_resourceType |= RESOURCETYPE_SOURCE;
    if (NiIsKindOf(NiSourceCubeMap, pTexture))
        m_resourceType |= RESOURCETYPE_CUBE;
}

//------------------------------------------------------------------------------------------------
D3D11SourceTextureData::~D3D11SourceTextureData()
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11SourceTextureData* D3D11SourceTextureData::Create(NiSourceTexture* pTexture)
{
    D3D11SourceTextureData* pThis = EE_NEW D3D11SourceTextureData(pTexture);

    efd::Bool success = pThis->LoadTexture();

    EE_ASSERT(success == false || pTexture->GetRendererData() == pThis);

    if (success)
        return pThis;
    else
        return NULL;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SourceTextureData::LoadTexture()
{
    EE_ASSERT(NiIsKindOf(NiSourceTexture, m_pkTexture));
    NiSourceTexture* pTexture = (NiSourceTexture*)m_pkTexture;

    const NiTexture::FormatPrefs& formatPrefs = m_pkTexture->GetFormatPreferences();
    efd::Bool isCube = IsCubeMap();

    // It is safe to use regular pointers, since the image converter functions
    // never assign incoming or outgoing data to smart pointers.
    NiPixelData* pSrcPixels = NULL;

    m_replacementData = false;

    // If there's D3D11PersistentSrcTextureRendererData, use it.
    efd::Bool foundFile = false;
    efd::Bool canLoadFile = false;
    D3D11PersistentSrcTextureRendererData* pPersistentSrcRendererData =
        (D3D11PersistentSrcTextureRendererData*)pTexture->GetPersistentSourceRendererData();
    if (!pPersistentSrcRendererData)
    {
        pSrcPixels = pTexture->GetSourcePixelData();

        if (!pSrcPixels)
        {
            // Import file into pixel data object
            const efd::Char* pFilename = pTexture->GetPlatformSpecificFilename();
            if (pFilename)
            {
                if (pTexture->GetLoadDirectToRendererHint())
                {
                    efd::Bool success = LoadTextureFile(pFilename, formatPrefs);

                    if (success)
                    {
                        pTexture->SetRendererData(this);
                        return true;
                    }
                }

                pSrcPixels = NiImageConverter::GetImageConverter()->ReadImageFile(
                    pFilename, 
                    NULL);

                if (pSrcPixels)
                {
                    foundFile = true;
                    canLoadFile = false;
                }
                else
                {
                    // Try to figure out why the file didn't load

                    // Check for missing file
                    efd::Char standardFilename[efd::EE_MAX_PATH];
                    efd::Strcpy(standardFilename, efd::EE_MAX_PATH, pFilename);
                    efd::PathUtils::Standardize(standardFilename);
                    efd::File* pIstr = efd::File::GetFile(standardFilename, efd::File::READ_ONLY);
                    if ((pIstr) && ((*pIstr)))
                    {
                        foundFile = true;
                    }
                    EE_DELETE pIstr;

                    if (foundFile)
                    {
                        // Check for unsupported format
                        canLoadFile = NiImageConverter::GetImageConverter()->CanReadImageFile(
                            pFilename);
                    }
                }
            }
        }
    }

    // This smart pointer exists to allow pSrcPixels to be deleted if it
    // needs to be.
    NiPixelDataPtr spSrcPixels = pSrcPixels;

    const NiPixelFormat* pDestFmt = NULL;
    if ((!pPersistentSrcRendererData) && (!pSrcPixels))
    {
        if (!foundFile)
        {
            pSrcPixels = GetReplacementData(
                pTexture,
                isCube ? CUBEMAPFILENOTFOUND : FILENOTFOUND);
            NiRenderer::Error(
                "Failed to find texture %s\n",
                pTexture->GetPlatformSpecificFilename());
        }
        else if (!canLoadFile)
        {
            // File found, but format not supported.
            pSrcPixels = GetReplacementData(
                pTexture,
                isCube ? CUBEMAPFILEFORMATNOTSUPPORTED : FILEFORMATNOTSUPPORTED);
            NiRenderer::Error(
                "Texture file %s found, but format is not supported\n",
                pTexture->GetPlatformSpecificFilename());
        }
        else
        {
            // File found, but couldn't be read.
            pSrcPixels = GetReplacementData(
                pTexture,
                isCube ? CUBEMAPFILELOADFAILED : FILELOADFAILED);
            NiRenderer::Error(
                "Texture file %s found and supported, but could not be loaded\n",
                pTexture->GetPlatformSpecificFilename());
        }

        m_replacementData = true;
    }
    else if (pPersistentSrcRendererData)
    {
        const NiPixelFormat& srcFmt = pPersistentSrcRendererData->GetPixelFormat();

        if ((pPersistentSrcRendererData->GetPixels() == NULL) ||
            (pPersistentSrcRendererData->GetTargetPlatform() != efd::SystemDesc::RENDERER_D3D11))
        {
            pSrcPixels = GetReplacementData(pTexture, FAILEDCONVERT);
            m_replacementData = true;

            NiRenderer::Error(
                "Texture %s contains persistent source texture data for a platform "
                "other than D3D11; cannot convert.\n",
                (const efd::Char*)pTexture->GetPlatformSpecificFilename());
        }

        else if (isCube && 
            (pPersistentSrcRendererData->GetWidth() != pPersistentSrcRendererData->GetHeight()))
        {
            pSrcPixels = GetReplacementData(pTexture, CUBEMAPBADDIMENSIONS);
            m_replacementData = true;
        }
        else
        {
            // Determine the D3D format from the pixel format.
            pDestFmt = FindMatchingPixelFormat(
                srcFmt, 
                formatPrefs, 
                isCube ? D3D11_FORMAT_SUPPORT_TEXTURECUBE : D3D11_FORMAT_SUPPORT_TEXTURE2D);

            DXGI_FORMAT format = D3D11PixelFormat::DetermineDXGIFormat(*pDestFmt);
            if (format == DXGI_FORMAT_UNKNOWN)
            {
                pSrcPixels = GetReplacementData(
                    pTexture, 
                    isCube ? CUBEMAPFAILEDCONVERT : FAILEDCONVERT);
                m_replacementData = true;
            }

            // Determine if the D3D format is compatible with the device.
            D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
            HRESULT hr = pRenderer->DoesFormatSupportFlag(
                format, 
                isCube ? D3D11_FORMAT_SUPPORT_TEXTURECUBE : D3D11_FORMAT_SUPPORT_TEXTURE2D);
            if (FAILED(hr))
            {
                pSrcPixels = GetReplacementData(
                    pTexture, 
                    isCube ? CUBEMAPFAILEDCONVERT : FAILEDCONVERT);
                m_replacementData = true;
            }
        }
    }
    else
    {
        EE_ASSERT(pSrcPixels);
        const NiPixelFormat& srcFmt = pSrcPixels->GetPixelFormat();

        if (isCube && (pSrcPixels->GetWidth() != pSrcPixels->GetHeight()))
        {
            pSrcPixels = GetReplacementData(pTexture, CUBEMAPBADDIMENSIONS);
            m_replacementData = true;
        }
        else
        {
            pDestFmt = FindMatchingPixelFormat(
                srcFmt, 
                formatPrefs, 
                isCube ? D3D11_FORMAT_SUPPORT_TEXTURECUBE : D3D11_FORMAT_SUPPORT_TEXTURE2D);

            if (pDestFmt == NULL)
            {
                pSrcPixels = GetReplacementData(
                    pTexture, 
                    isCube ? CUBEMAPFAILEDCONVERT : FAILEDCONVERT);
                m_replacementData = true;
            }
        }
    }

    if (m_replacementData)
    {
        EE_ASSERT(pSrcPixels);
        pDestFmt = &pSrcPixels->GetPixelFormat();
        EE_ASSERT(pDestFmt->GetRendererHint() == DXGI_FORMAT_R8G8B8A8_UNORM);

        pPersistentSrcRendererData = NULL; // Sanity checks.
    }

    // persistent source data
    if (pPersistentSrcRendererData)
    {
        // Update texture attributes from the D3D11SourceTextureData.
        const NiPixelFormat& srcFmt = pPersistentSrcRendererData->GetPixelFormat();

        // Determine the D3D format from the pixel format.
        m_kPixelFormat = *(FindMatchingPixelFormat(
            srcFmt, 
            formatPrefs, 
            isCube ? D3D11_FORMAT_SUPPORT_TEXTURECUBE : D3D11_FORMAT_SUPPORT_TEXTURE2D));

        m_mipmap = (pPersistentSrcRendererData->GetNumMipmapLevels() != 1);
        m_formattedSize = (efd::UInt32)pPersistentSrcRendererData->GetTotalSizeInBytes();

        // Attach palette if the surface is palettized
        UpdatePalette(pPersistentSrcRendererData->GetPalette());

        // Allocate and fill texture
        EE_ASSERT(m_pD3D11Resource == NULL);
        InitializeTexture(pPersistentSrcRendererData, isCube, pTexture->GetStatic());
    }
    else
    {
        m_mipmap =
            (!m_replacementData &&
            ((formatPrefs.m_eMipMapped == NiTexture::FormatPrefs::YES) ||
            ((formatPrefs.m_eMipMapped == NiTexture::FormatPrefs::MIP_DEFAULT) &&
            NiTexture::GetMipmapByDefault())));

        NiPixelData* pFormatted = NULL;
        if (m_replacementData)
        {
            pFormatted = pSrcPixels;  // Do not convert replacement data.
        }
        else
        {
            // Convert data format
            pFormatted = NiImageConverter::GetImageConverter()->ConvertPixelData(
                *pSrcPixels, 
                *pDestFmt, 
                pSrcPixels, 
                m_mipmap);
        }
        EE_ASSERT(pFormatted);

        m_sourceRevID = pSrcPixels->GetRevisionID();
        m_kPixelFormat = *pDestFmt;
        m_formattedSize = (efd::UInt32)pFormatted->GetTotalSizeInBytes();

        // Allocate and fill texture
        EE_ASSERT(m_pD3D11Resource == NULL);
        InitializeTexture(pFormatted, isCube, pTexture->GetStatic());

        // Attach palette - no need to check result because palette was just
        // used to un-palettize the data.
        UpdatePalette(pSrcPixels->GetPalette());

        // Some of the objects used locally may have zero refcounts. Smart
        // pointers have been avoided in the function for speed. Here, at the
        // end of the function, the pFormatted pointer is assigned to a smart
        // pointer to force destruction, as needed.
        NiPixelDataPtr spDestructor = pFormatted;
    }

    pTexture->SetRendererData(this);

    if (NiSourceTexture::GetDestroyAppDataFlag() && pTexture->GetStatic())
        pTexture->DestroyAppPixelData();

    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11SourceTextureData::Update()
{
    // Textures using pixel replacement data needs not be updated
    if (m_replacementData)
        return;

    // Can't update static textures
    if (((NiSourceTexture*)m_pkTexture)->GetStatic())
        return;

    NiPixelData* pSrcPixels = ((NiSourceTexture*)m_pkTexture)->GetSourcePixelData();

    if (pSrcPixels)
    {
        // Check to see if palette has changed
        efd::Bool paletteExpand = UpdatePalette(pSrcPixels->GetPalette());

        // The most common reason for this case below is a change to the
        // pixels in the pixel data object. We may also have to enter this
        // case when a palette changes and the destination format has no
        // palette (in which case the palette must be re-expanded into the
        // RGB(A) texture data, and expensive operation)
        if (pSrcPixels->GetRevisionID() != m_sourceRevID || paletteExpand)
        {
            // Convert data format
            NiPixelData* pFormatted = NiImageConverter::GetImageConverter()->ConvertPixelData(
                *pSrcPixels, 
                m_kPixelFormat, 
                pSrcPixels, 
                m_mipmap);

            EE_ASSERT(pFormatted);

            m_sourceRevID = pSrcPixels->GetRevisionID();

            // Copy to D3D11 texture
            if (m_pD3D11Resource)
                UpdateTexture(pFormatted);

            // Some of the objects used locally may have zero refcounts.
            // We have avoided using smart pointers in the function for speed.
            // Here, at the end of the function, we assign them to smart
            // pointers to force destruction as needed
            NiPixelDataPtr spDestructor = pFormatted;
        }
    }
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SourceTextureData::SetMipmapSkipLevel(efd::UInt16 skip)
{
    ms_skipLevels = skip;
    return true;
}

//------------------------------------------------------------------------------------------------
efd::UInt16 D3D11SourceTextureData::GetMipmapSkipLevel()
{
    return ms_skipLevels;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SourceTextureData::InitializeTexture(
    const D3D11PersistentSrcTextureRendererData* pData,
    efd::Bool isCube, 
    efd::Bool isStatic)
{
    EE_ASSERT(pData);

    DXGI_FORMAT format = D3D11PixelFormat::DetermineDXGIFormat(m_kPixelFormat);

    if (!D3D11PixelFormat::IsDXGIFormatSupported(format))
    {
        D3D11Error::ReportError(
            D3D11Error::D3D11ERROR_TEXTURE2D_CREATION_FAILED,
            "%s format depreciated or unsupported.",
            D3D11PixelFormat::GetFormatName(format));
        return false;
    }

    m_levels = (efd::UInt16)pData->GetNumMipmapLevels();

    m_levelsSkipped = ms_skipLevels;
    if (m_levelsSkipped > m_levels - 1)
        m_levelsSkipped = m_levels - 1;
    m_levels = (efd::UInt16)(m_levels - m_levelsSkipped);

    m_uiWidth = pData->GetWidth(m_levelsSkipped);
    m_uiHeight = pData->GetHeight(m_levelsSkipped);
    m_numTextures = (efd::UInt16) pData->GetNumFaces();

    // Ensure that if we are skipping mipmap levels, then there is at least
    // one mipmap level that is not getting skipped
    EE_ASSERT(ms_skipLevels == 0 || m_levels != 0);

    efd::UInt32 msaaCount = 1;
    efd::UInt32 msaaQuality = 0;

    D3D11_USAGE usage = (isStatic ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT);
    efd::UInt32 cpuAccessFlags = 0;

    efd::UInt32 bindFlags = D3D11_BIND_SHADER_RESOURCE;

    EE_ASSERT(m_levels != 0 && m_levels < D3D11_MAX_TEXTURE_DIMENSION_2_TO_EXP);

    efd::UInt32 miscFlags = (isCube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0);

    D3D11_SUBRESOURCE_DATA* initialDataArray = EE_STACK_ALLOC(
        D3D11_SUBRESOURCE_DATA,
        m_numTextures * D3D11_MAX_TEXTURE_DIMENSION_2_TO_EXP);

    // BC2 and BC3 have 16 bytes per block, and BC1, BC4, and BC5 have 8 bytes
    efd::UInt32 compressedBytesPerBlock = 16;
    efd::Bool isCompressed = m_kPixelFormat.GetCompressed();
    if (isCompressed)
    {
        efd::UInt32 rendererHint = m_kPixelFormat.GetRendererHint();
        if (rendererHint == DXGI_FORMAT_BC1_TYPELESS ||
            rendererHint == DXGI_FORMAT_BC1_UNORM ||
            rendererHint == DXGI_FORMAT_BC1_UNORM_SRGB ||
            rendererHint == DXGI_FORMAT_BC4_TYPELESS ||
            rendererHint == DXGI_FORMAT_BC4_UNORM ||
            rendererHint == DXGI_FORMAT_BC4_SNORM ||
            rendererHint == DXGI_FORMAT_BC5_TYPELESS ||
            rendererHint == DXGI_FORMAT_BC5_UNORM ||
            rendererHint == DXGI_FORMAT_BC5_SNORM)
        {
            compressedBytesPerBlock = 8;
        }

        // Width and height must be multiples of 4 pixels
        m_uiWidth = (m_uiWidth + 3) & 0xFFFFFFFC;
        m_uiHeight = (m_uiHeight + 3) & 0xFFFFFFFC;
    }

    efd::UInt32 entry = 0;
    for (efd::UInt16 i = 0; i < m_numTextures; i++)
    {
        for (efd::UInt16 j = 0; j < m_levels; j++)
        {
            efd::UInt16 level = j + m_levelsSkipped;
            efd::UInt32 width = pData->GetWidth(level); 
            efd::UInt32 pitch = width * pData->GetPixelStride(); 
            if (isCompressed)
            {
                // pPixels->GetPixelStride = 0 for compressed formats; need
                // to recalculate and account for non-divisible-by-four pixel
                // counts, such as smaller mipmap levels.
                efd::UInt32 numBlocks = ((width + 3) / 4);

                pitch = numBlocks * compressedBytesPerBlock;
            }

            // Texture dimensions must match.
            EE_ASSERT(pData->GetWidth(level, i) == pData->GetWidth(level, 0));
            EE_ASSERT(pData->GetHeight(level, i) == pData->GetHeight(level, 0));
            initialDataArray[entry].pSysMem = pData->GetPixels(level, i);
            initialDataArray[entry].SysMemPitch = pitch;

            // MS recommends this, but it's not strictly necessary.
            initialDataArray[entry].SysMemSlicePitch = (UINT)pData->GetSizeInBytes(level, i);

            entry++;
        }
    }

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
        miscFlags, 
        initialDataArray);

    EE_STACK_FREE(initialDataArray);

    if (pD3D11Texture == NULL)
    {
        D3D11Error::ReportError(D3D11Error::D3D11ERROR_TEXTURE2D_CREATION_FAILED);

        return false;
    }

    // Create default resource view
    efd::Bool success = InitializeFromD3D11Resource(pD3D11Texture);
    pD3D11Texture->Release();
    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SourceTextureData::InitializeTexture(
    const NiPixelData* pPixels,
    efd::Bool isCube, 
    efd::Bool isStatic)
{
    EE_ASSERT(pPixels);

    DXGI_FORMAT format = D3D11PixelFormat::DetermineDXGIFormat(m_kPixelFormat);

    m_levels = (efd::UInt16)pPixels->GetNumMipmapLevels();

    m_levelsSkipped = ms_skipLevels;
    if (m_levelsSkipped > m_levels - 1)
        m_levelsSkipped = m_levels - 1;
    m_levels = (efd::UInt16)(m_levels - m_levelsSkipped);

    m_uiWidth = pPixels->GetWidth(m_levelsSkipped);
    m_uiHeight = pPixels->GetHeight(m_levelsSkipped);
    m_numTextures = (efd::UInt16)pPixels->GetNumFaces();

    // Ensure that if we are skipping mipmap levels, then there is at least
    // one mipmap level that is not getting skipped
    EE_ASSERT(ms_skipLevels == 0 || m_levels != 0);

    efd::UInt32 msaaCount = 1;
    efd::UInt32 msaaQuality = 0;

    D3D11_USAGE usage = (isStatic ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT);
    efd::UInt32 cpuAccessFlags = 0;

    efd::UInt32 bindFlags = D3D11_BIND_SHADER_RESOURCE;

    EE_ASSERT(m_levels != 0 && m_levels < D3D11_MAX_TEXTURE_DIMENSION_2_TO_EXP);

    efd::UInt32 miscFlags = (isCube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0);

    D3D11_SUBRESOURCE_DATA* initialDataArray = EE_STACK_ALLOC(
        D3D11_SUBRESOURCE_DATA,
        m_numTextures * D3D11_MAX_TEXTURE_DIMENSION_2_TO_EXP);

    // BC2 and BC3 have 16 bytes per block, and BC1, BC4, and BC5 have 8 bytes
    efd::UInt32 compressedBytesPerBlock = 16;
    efd::Bool isCompressed = m_kPixelFormat.GetCompressed();
    if (isCompressed)
    {
        efd::UInt32 rendererHint = m_kPixelFormat.GetRendererHint();
        if (rendererHint == DXGI_FORMAT_BC1_TYPELESS ||
            rendererHint == DXGI_FORMAT_BC1_UNORM ||
            rendererHint == DXGI_FORMAT_BC1_UNORM_SRGB ||
            rendererHint == DXGI_FORMAT_BC4_TYPELESS ||
            rendererHint == DXGI_FORMAT_BC4_UNORM ||
            rendererHint == DXGI_FORMAT_BC4_SNORM ||
            rendererHint == DXGI_FORMAT_BC5_TYPELESS ||
            rendererHint == DXGI_FORMAT_BC5_UNORM ||
            rendererHint == DXGI_FORMAT_BC5_SNORM)
        {
            compressedBytesPerBlock = 8;
        }

        // Width and height must be multiples of 4 pixels
        m_uiWidth = (m_uiWidth + 3) & 0xFFFFFFFC;
        m_uiHeight = (m_uiHeight + 3) & 0xFFFFFFFC;
    }

    efd::UInt32 entry = 0;
    for (efd::UInt16 i = 0; i < m_numTextures; i++)
    {
        for (efd::UInt16 j = 0; j < m_levels; j++)
        {
            efd::UInt16 level = j + m_levelsSkipped;
            efd::UInt32 width = pPixels->GetWidth(level);
            efd::UInt32 pitch = width * pPixels->GetPixelStride();
            if (isCompressed)
            {
                // pPixels->GetPixelStride = 0 for compressed formats; need
                // to recalculate and account for non-divisible-by-four pixel
                // counts, such as smaller mipmap levels.
                efd::UInt32 numBlocks = ((width + 3) / 4);

                pitch = numBlocks * compressedBytesPerBlock;
            }

            // Texture dimensions must match.
            EE_ASSERT(pPixels->GetWidth(level, i) == pPixels->GetWidth(level, 0));
            EE_ASSERT(pPixels->GetHeight(level, i) == pPixels->GetHeight(level, 0));
            initialDataArray[entry].pSysMem = pPixels->GetPixels(level, i);
            initialDataArray[entry].SysMemPitch = pitch;

            // MS recommends this, but it's not strictly necessary.
            initialDataArray[entry].SysMemSlicePitch = (UINT)pPixels->GetSizeInBytes(level, i);

            entry++;
        }
    }

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
        miscFlags, 
        initialDataArray);

    EE_STACK_FREE(initialDataArray);

    if (pD3D11Texture == NULL)
    {
        D3D11Error::ReportError(
            D3D11Error::D3D11ERROR_TEXTURE2D_CREATION_FAILED,
            "CreateTexture2D failed during Initialization of texture.");
        return false;
    }

    // Create default resource view
    efd::Bool success = InitializeFromD3D11Resource(pD3D11Texture);
    pD3D11Texture->Release();
    return success;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SourceTextureData::UpdateTexture(const NiPixelData* pPixels)
{
    EE_ASSERT(pPixels);

    D3D11PixelFormat::DetermineDXGIFormat(m_kPixelFormat);

    m_uiWidth = pPixels->GetWidth();
    m_uiHeight = pPixels->GetHeight();
    m_levels = (efd::UInt16)pPixels->GetNumMipmapLevels();

    efd::UInt16 skipLevels = ms_skipLevels;
    if (skipLevels > m_levels - 1)
        skipLevels = m_levels - 1;

    // Adjust (possibly non-power-of-two) dimensions by skipLevels.
    efd::UInt32 i = 0;
    for (; i < skipLevels; i++)
    {
        if (m_uiWidth & 0x1)    // If odd, add 1 so shifted value rounds up.
            m_uiWidth++;
        m_uiWidth >>= 1;
        if (m_uiHeight & 0x1)   // If odd, add 1 so shifted value rounds up.
            m_uiHeight++;
        m_uiHeight >>= 1;
    }

    m_levelsSkipped = skipLevels;

    // Ensure that if we are skipping mipmap levels, then there is at least
    // one mipmap level that is not getting skipped
    EE_ASSERT(ms_skipLevels == 0 || m_levels != 0);

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    EE_ASSERT(pRenderer && pRenderer->GetResourceManager() && pRenderer->GetD3D11Device());

    EE_ASSERT(m_levels != 0 && m_levels < D3D11_MAX_TEXTURE_DIMENSION_2_TO_EXP);

    // BC2 and BC3 have 16 bytes per block, and BC1, BC4, and BC5 have 8 bytes
    efd::UInt32 compressedBitsPerBlock = 16;
    efd::Bool isCompressed = m_kPixelFormat.GetCompressed();
    if (isCompressed)
    {
        efd::UInt32 rendererHint = m_kPixelFormat.GetRendererHint();
        if (rendererHint == DXGI_FORMAT_BC1_TYPELESS ||
            rendererHint == DXGI_FORMAT_BC1_UNORM ||
            rendererHint == DXGI_FORMAT_BC1_UNORM_SRGB ||
            rendererHint == DXGI_FORMAT_BC4_TYPELESS ||
            rendererHint == DXGI_FORMAT_BC4_UNORM ||
            rendererHint == DXGI_FORMAT_BC4_SNORM ||
            rendererHint == DXGI_FORMAT_BC5_TYPELESS ||
            rendererHint == DXGI_FORMAT_BC5_UNORM ||
            rendererHint == DXGI_FORMAT_BC5_SNORM)
        {
            compressedBitsPerBlock = 8;
        }
    }

    EE_ASSERT(m_pD3D11Resource);
    for (i = 0; i < m_levels; i++)
    {
        efd::UInt32 pitch = pPixels->GetWidth(i) * pPixels->GetPixelStride();
        if (isCompressed)
        {
            // pPixels->GetPixelStride = 0 for compressed formats; need to
            // recalculate and account for non-divisible-by-four pixel counts
            pitch = (pPixels->GetWidth(i) + 3) & 0xFFFFFFC;
            if (compressedBitsPerBlock == 8)
                pitch /= 2;
        }

        pRenderer->GetCurrentD3D11DeviceContext()->UpdateSubresource(
            m_pD3D11Resource,
            i, 
            NULL, 
            pPixels->GetPixels(i), 
            pitch, 
            0);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SourceTextureData::UpdatePalette(NiPalette* pNewPalette)
{
    efd::Bool paletteExpand = false;

    // If the Pixel Data has a Palette, We need to check for:
    // 1) New palette attached to data
    // 2) Palette elements changed
    if (pNewPalette)
    {
        efd::UInt32 paletteRevID = pNewPalette->GetRevisionID();

        // We know that the dest is not palettized, so if the palette
        // entries have changed, then we need to re-expand the data
        if (m_paletteRevID != paletteRevID)
            paletteExpand = true;

        m_paletteRevID = paletteRevID;

        // We know that the dest is not palettized, so if the palette
        // itself has changed, then we need to re-expand the data
        if (m_spPalette != pNewPalette)
        {
            m_spPalette = pNewPalette;
            paletteExpand = true;
        }
    }

    return paletteExpand;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11SourceTextureData::LoadTextureFile(const efd::Char* pFilename,
    const NiTexture::FormatPrefs& formatPrefs)
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    if (pRenderer == NULL)
        return false;

    ID3D11Device* pD3D11Device = pRenderer->GetD3D11Device();
    if (pD3D11Device == NULL)
        return false;

    efd::Char standardFilename[efd::EE_MAX_PATH];
    efd::Strcpy(standardFilename, efd::EE_MAX_PATH, pFilename);
    efd::PathUtils::Standardize(standardFilename);

    efd::File* pIstr = efd::File::GetFile(standardFilename, efd::File::READ_ONLY);
    if ((!pIstr) || (!(*pIstr)))
    {
        EE_DELETE pIstr;
        return false;
    }

    const efd::UInt32 bufferLength = pIstr->GetFileSize();
    if (bufferLength == 0)
    {
        EE_DELETE pIstr;
        return false;
    }

    efd::UInt8* pBuffer = EE_ALLOC2(efd::UInt8, bufferLength, efd::MemHint::TEXTURE);
    if (!pBuffer)
    {
        EE_DELETE pIstr;
        return false;
    }

    const efd::UInt32 bytesRead = pIstr->Read(pBuffer, bufferLength);
    EE_DELETE pIstr;
    if (bytesRead != bufferLength)
    {
        EE_FREE(pBuffer);
        return false;
    }

    ID3D11Resource* pResource = NULL;
    HRESULT hr = E_FAIL;

    NiFilename filename(standardFilename);
    const efd::Char* pExtension = filename.GetExt();
    if (pExtension && efd::Stricmp(pExtension, ".dds") == 0)
    {
        hr = DirectX::CreateDDSTextureFromMemory(
            pD3D11Device, pBuffer, bufferLength,
            &pResource, NULL);
    }
    else if (pExtension && efd::Stricmp(pExtension, ".tga") == 0)
    {
        DirectX::TexMetadata metadata;
        DirectX::ScratchImage image;
        hr = DirectX::LoadFromTGAMemory(
            pBuffer, bufferLength, DirectX::TGA_FLAGS_NONE, &metadata, image);
        if (SUCCEEDED(hr))
        {
            hr = DirectX::CreateTexture(pD3D11Device, image.GetImages(),
                image.GetImageCount(), metadata, &pResource);
        }
    }
    else if (pExtension && efd::Stricmp(pExtension, ".hdr") == 0)
    {
        DirectX::TexMetadata metadata;
        DirectX::ScratchImage image;
        hr = DirectX::LoadFromHDRMemory(pBuffer, bufferLength, &metadata, image);
        if (SUCCEEDED(hr))
        {
            hr = DirectX::CreateTexture(pD3D11Device, image.GetImages(),
                image.GetImageCount(), metadata, &pResource);
        }
    }
    else
    {
        ScopedCOMInitialization comInitialization;
        hr = DirectX::CreateWICTextureFromMemory(
            pD3D11Device, pBuffer, bufferLength,
            &pResource, NULL);
    }

    EE_FREE(pBuffer);

    if (FAILED(hr) || pResource == NULL)
        return false;

    DXGI_FORMAT resourceFormat = DXGI_FORMAT_UNKNOWN;
    D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&dimension);
    if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
    {
        ID3D11Texture1D* pTexture = NULL;
        if (SUCCEEDED(pResource->QueryInterface(__uuidof(ID3D11Texture1D),
            reinterpret_cast<void**>(&pTexture))))
        {
            D3D11_TEXTURE1D_DESC desc;
            pTexture->GetDesc(&desc);
            resourceFormat = desc.Format;
            pTexture->Release();
        }
    }
    else if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
        ID3D11Texture2D* pTexture = NULL;
        if (SUCCEEDED(pResource->QueryInterface(__uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(&pTexture))))
        {
            D3D11_TEXTURE2D_DESC desc;
            pTexture->GetDesc(&desc);
            resourceFormat = desc.Format;
            pTexture->Release();
        }
    }
    else if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
    {
        ID3D11Texture3D* pTexture = NULL;
        if (SUCCEEDED(pResource->QueryInterface(__uuidof(ID3D11Texture3D),
            reinterpret_cast<void**>(&pTexture))))
        {
            D3D11_TEXTURE3D_DESC desc;
            pTexture->GetDesc(&desc);
            resourceFormat = desc.Format;
            pTexture->Release();
        }
    }

    if (formatPrefs.m_ePixelLayout == NiTexture::FormatPrefs::BUMPMAP)
    {
        NiPixelFormat pixelFormat;
        D3D11PixelFormat::InitFromDXGIFormat(resourceFormat, pixelFormat);

        NiPixelFormat::Component formatComp;
        NiPixelFormat::Representation formatRep;
        efd::UInt8 bpc;
        efd::Bool isSigned;
        if (!pixelFormat.GetComponent(9, formatComp, formatRep, bpc, isSigned) || !isSigned)
        {
            pResource->Release();
            return false;
        }
    }

    const efd::Bool success = InitializeFromD3D11Resource(pResource);
    pResource->Release();
    return success;
}

//------------------------------------------------------------------------------------------------
