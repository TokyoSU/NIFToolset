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

#include "D3D11Direct3DResource.h"

using namespace ecr;

NiImplementRTTI(D3D11Direct3DResource, NiTexture, NiTypeMask::D3D11Direct3DResource);

//------------------------------------------------------------------------------------------------
D3D11Direct3DResource* D3D11Direct3DResource::Create(NiRenderer* pkRenderer)
{
    if (!pkRenderer)
        return NULL;

    D3D11Direct3DResource* pkThis = EE_NEW D3D11Direct3DResource;

    return pkThis;
}

//------------------------------------------------------------------------------------------------
D3D11Direct3DResource::D3D11Direct3DResource() :
    m_width(0),
    m_height(0),
    m_depth(0),
    m_arrayCount(0)
{
    /* */
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Direct3DResource::GetWidth() const
{
    return m_width;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Direct3DResource::GetHeight() const
{
    return m_height;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Direct3DResource::GetDepth() const
{
    return m_depth;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11Direct3DResource::GetArrayCount() const
{
    return m_arrayCount;
}

//------------------------------------------------------------------------------------------------
