// Precompiled Header
#include "NiD3D10RendererPCH.h"

#include "NiD3D10TextureTools.h"

#include <algorithm>
#include <cstring>

namespace NiD3D10TextureTools
{
bool ToWidePath(const char* path, wchar_t* destination, size_t destinationCount)
{
    if (!path || !destination || destinationCount == 0)
        return false;

    destination[0] = L'\0';
    int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
        destination, static_cast<int>(destinationCount));
    if (result == 0)
    {
        result = MultiByteToWideChar(CP_ACP, 0, path, -1,
            destination, static_cast<int>(destinationCount));
    }
    return result != 0;
}

HRESULT CaptureTexture(
    ID3D10Device* device,
    ID3D10Texture2D* source,
    DirectX::ScratchImage& result)
{
    if (!device || !source)
        return E_INVALIDARG;

    D3D10_TEXTURE2D_DESC sourceDesc = {};
    source->GetDesc(&sourceDesc);

    ID3D10Texture2D* copySource = source;
    copySource->AddRef();

    if (sourceDesc.SampleDesc.Count > 1)
    {
        D3D10_TEXTURE2D_DESC resolveDesc = sourceDesc;
        resolveDesc.MipLevels = 1;
        resolveDesc.ArraySize = 1;
        resolveDesc.SampleDesc.Count = 1;
        resolveDesc.SampleDesc.Quality = 0;
        resolveDesc.Usage = D3D10_USAGE_DEFAULT;
        resolveDesc.BindFlags = 0;
        resolveDesc.CPUAccessFlags = 0;
        resolveDesc.MiscFlags = 0;

        ID3D10Texture2D* resolved = NULL;
        HRESULT hr = device->CreateTexture2D(&resolveDesc, NULL, &resolved);
        if (FAILED(hr))
        {
            copySource->Release();
            return hr;
        }

        device->ResolveSubresource(resolved, 0, source, 0, sourceDesc.Format);
        copySource->Release();
        copySource = resolved;
        sourceDesc = resolveDesc;
    }

    D3D10_TEXTURE2D_DESC stagingDesc = sourceDesc;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Usage = D3D10_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;

    ID3D10Texture2D* staging = NULL;
    HRESULT hr = device->CreateTexture2D(&stagingDesc, NULL, &staging);
    if (FAILED(hr))
    {
        copySource->Release();
        return hr;
    }

    device->CopySubresourceRegion(staging, 0, 0, 0, 0, copySource, 0, NULL);
    copySource->Release();

    D3D10_MAPPED_TEXTURE2D mapped = {};
    hr = staging->Map(0, D3D10_MAP_READ, 0, &mapped);
    if (FAILED(hr))
    {
        staging->Release();
        return hr;
    }

    hr = result.Initialize2D(stagingDesc.Format, stagingDesc.Width,
        stagingDesc.Height, 1, 1);
    if (SUCCEEDED(hr))
    {
        const DirectX::Image* image = result.GetImage(0, 0, 0);
        if (!image)
        {
            hr = E_FAIL;
        }
        else
        {
            const size_t rows = image->rowPitch ? image->slicePitch / image->rowPitch : 0;
            const size_t bytesPerRow = std::min<size_t>(image->rowPitch, mapped.RowPitch);
            const unsigned char* sourceBytes = static_cast<const unsigned char*>(mapped.pData);
            unsigned char* destinationBytes = image->pixels;
            for (size_t row = 0; row < rows; ++row)
            {
                std::memcpy(destinationBytes, sourceBytes, bytesPerRow);
                sourceBytes += mapped.RowPitch;
                destinationBytes += image->rowPitch;
            }
        }
    }

    staging->Unmap(0);
    staging->Release();
    return hr;
}

HRESULT CopyTextureRegion(
    ID3D10Device* device,
    ID3D10Texture2D* source,
    ID3D10Texture2D* destination,
    const D3D10_BOX* sourceBox,
    const D3D10_BOX* destinationBox,
    DirectX::TEX_FILTER_FLAGS filter)
{
    if (!device || !source || !destination)
        return E_INVALIDARG;

    D3D10_TEXTURE2D_DESC sourceDesc = {};
    D3D10_TEXTURE2D_DESC destinationDesc = {};
    source->GetDesc(&sourceDesc);
    destination->GetDesc(&destinationDesc);

    if (sourceDesc.Format != destinationDesc.Format)
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    const UINT srcLeft = sourceBox ? sourceBox->left : 0;
    const UINT srcTop = sourceBox ? sourceBox->top : 0;
    const UINT srcRight = sourceBox ? sourceBox->right : sourceDesc.Width;
    const UINT srcBottom = sourceBox ? sourceBox->bottom : sourceDesc.Height;
    const UINT dstLeft = destinationBox ? destinationBox->left : 0;
    const UINT dstTop = destinationBox ? destinationBox->top : 0;
    const UINT dstRight = destinationBox ? destinationBox->right : destinationDesc.Width;
    const UINT dstBottom = destinationBox ? destinationBox->bottom : destinationDesc.Height;

    if (srcRight <= srcLeft || srcBottom <= srcTop ||
        dstRight <= dstLeft || dstBottom <= dstTop)
        return E_INVALIDARG;

    if (srcRight > sourceDesc.Width || srcBottom > sourceDesc.Height ||
        dstRight > destinationDesc.Width || dstBottom > destinationDesc.Height)
        return E_INVALIDARG;

    const UINT sourceWidth = srcRight - srcLeft;
    const UINT sourceHeight = srcBottom - srcTop;
    const UINT destinationWidth = dstRight - dstLeft;
    const UINT destinationHeight = dstBottom - dstTop;

    if (sourceWidth == destinationWidth && sourceHeight == destinationHeight)
    {
        D3D10_BOX box = { srcLeft, srcTop, 0, srcRight, srcBottom, 1 };
        device->CopySubresourceRegion(destination, 0, dstLeft, dstTop, 0,
            source, 0, &box);
        return S_OK;
    }

    if (destinationDesc.SampleDesc.Count > 1 ||
        DirectX::IsCompressed(sourceDesc.Format) ||
        DirectX::IsPlanar(sourceDesc.Format) ||
        DirectX::IsDepthStencil(sourceDesc.Format))
    {
        return E_NOTIMPL;
    }

    DirectX::ScratchImage captured;
    HRESULT hr = CaptureTexture(device, source, captured);
    if (FAILED(hr))
        return hr;

    const DirectX::Image* sourceImage = captured.GetImage(0, 0, 0);
    if (!sourceImage)
        return E_FAIL;

    DirectX::ScratchImage cropped;
    hr = cropped.Initialize2D(sourceImage->format, sourceWidth, sourceHeight, 1, 1);
    if (FAILED(hr))
        return hr;

    const DirectX::Image* croppedImage = cropped.GetImage(0, 0, 0);
    if (!croppedImage)
        return E_FAIL;

    const DirectX::Rect sourceRect(srcLeft, srcTop, sourceWidth, sourceHeight);
    hr = DirectX::CopyRectangle(*sourceImage, sourceRect, *croppedImage,
        DirectX::TEX_FILTER_POINT, 0, 0);
    if (FAILED(hr))
        return hr;

    const DirectX::Image* uploadImage = croppedImage;
    DirectX::ScratchImage resized;
    if (sourceWidth != destinationWidth || sourceHeight != destinationHeight)
    {
        hr = DirectX::Resize(*croppedImage, destinationWidth, destinationHeight,
            filter, resized);
        if (FAILED(hr))
            return hr;
        uploadImage = resized.GetImage(0, 0, 0);
        if (!uploadImage)
            return E_FAIL;
    }

    D3D10_BOX uploadBox = { dstLeft, dstTop, 0, dstRight, dstBottom, 1 };
    device->UpdateSubresource(destination, 0, &uploadBox,
        uploadImage->pixels, static_cast<UINT>(uploadImage->rowPitch),
        static_cast<UINT>(uploadImage->slicePitch));
    return S_OK;
}
}
