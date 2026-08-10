#pragma once
#ifndef EE_D3D11TEXTURETOOLS_H
#define EE_D3D11TEXTURETOOLS_H

#include "D3D11Headers.h"

namespace ecr
{
namespace D3D11TextureTools
{
    bool ToWidePath(const char* path, wchar_t* destination, size_t destinationCount);

    HRESULT CopyTextureRegion(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        ID3D11Texture2D* source,
        ID3D11Texture2D* destination,
        const D3D11_BOX* sourceBox,
        const D3D11_BOX* destinationBox,
        DirectX::TEX_FILTER_FLAGS filter);
}
}

#endif
