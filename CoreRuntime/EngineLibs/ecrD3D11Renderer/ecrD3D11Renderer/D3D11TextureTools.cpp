// Precompiled Header
#include "ecrD3D11RendererPCH.h"

#include "D3D11TextureTools.h"

#include <algorithm>
#include <cstring>

namespace ecr
{
namespace D3D11TextureTools
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

HRESULT CopyTextureRegion(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    ID3D11Texture2D* source,
    ID3D11Texture2D* destination,
    const D3D11_BOX* sourceBox,
    const D3D11_BOX* destinationBox,
    DirectX::TEX_FILTER_FLAGS filter)
{
    if (!device || !context || !source || !destination)
        return E_INVALIDARG;

    D3D11_TEXTURE2D_DESC sourceDesc = {};
    D3D11_TEXTURE2D_DESC destinationDesc = {};
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
        D3D11_BOX box = { srcLeft, srcTop, 0, srcRight, srcBottom, 1 };
        context->CopySubresourceRegion(destination, 0, dstLeft, dstTop, 0,
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
    HRESULT hr = DirectX::CaptureTexture(device, context, source, captured);
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

    D3D11_BOX uploadBox = { dstLeft, dstTop, 0, dstRight, dstBottom, 1 };
    context->UpdateSubresource(destination, 0, &uploadBox,
        uploadImage->pixels, static_cast<UINT>(uploadImage->rowPitch),
        static_cast<UINT>(uploadImage->slicePitch));
    return S_OK;
}
}
}
