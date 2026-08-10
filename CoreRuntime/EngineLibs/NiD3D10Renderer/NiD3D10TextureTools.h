#pragma once
#ifndef NID3D10TEXTURETOOLS_H
#define NID3D10TEXTURETOOLS_H

#include "NiD3D10Headers.h"

namespace NiD3D10TextureTools
{
    bool ToWidePath(const char* path, wchar_t* destination, size_t destinationCount);

    HRESULT CaptureTexture(
        ID3D10Device* device,
        ID3D10Texture2D* source,
        DirectX::ScratchImage& result);

    HRESULT CopyTextureRegion(
        ID3D10Device* device,
        ID3D10Texture2D* source,
        ID3D10Texture2D* destination,
        const D3D10_BOX* sourceBox,
        const D3D10_BOX* destinationBox,
        DirectX::TEX_FILTER_FLAGS filter);
}

#endif
