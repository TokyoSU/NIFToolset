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

#include "D3D11Direct3DResourceData.h"
#include "D3D11Direct3DResource.h"
#include "D3D11Renderer.h"

using namespace ecr;

//------------------------------------------------------------------------------------------------
D3D11Direct3DResourceData* D3D11Direct3DResourceData::Create(
    D3D11Direct3DResource* pTexture, 
    ID3D11Resource* pD3D11Texture,
    ID3D11ShaderResourceView* pResourceView)
{
    D3D11Direct3DResourceData* pThis = EE_NEW D3D11Direct3DResourceData(pTexture);

    if (pThis == NULL)
        return NULL;

    efd::Bool success = pThis->InitializeFromD3D11Resource(pD3D11Texture, pResourceView);
    if (success == false)
    {
        EE_DELETE pThis;
        return NULL;
    }

    pTexture->SetWidth(pThis->GetWidth());
    pTexture->SetHeight(pThis->GetHeight());
    pTexture->SetDepth(pThis->GetDepth());
    pTexture->SetArrayCount(pThis->GetArrayCount());

    pThis->m_pkTexture->SetRendererData(pThis);

    return pThis;
}

//------------------------------------------------------------------------------------------------
D3D11Direct3DResourceData::D3D11Direct3DResourceData(
    D3D11Direct3DResource* pTexture) :
    D3D11TextureData(pTexture)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11Direct3DResourceData::~D3D11Direct3DResourceData()
{
    /* */
}

//------------------------------------------------------------------------------------------------
