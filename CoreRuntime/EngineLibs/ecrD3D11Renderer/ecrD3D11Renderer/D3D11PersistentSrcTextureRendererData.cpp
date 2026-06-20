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

#include <NiImageConverter.h>

#include "D3D11PersistentSrcTextureRendererData.h"
#include "D3D11Renderer.h"

using namespace ecr;

NiImplementRTTI(D3D11PersistentSrcTextureRendererData,
    NiPersistentSrcTextureRendererData, NiTypeMask::D3D11PersistentSrcTextureRendererData);

//------------------------------------------------------------------------------------------------
D3D11PersistentSrcTextureRendererData::D3D11PersistentSrcTextureRendererData()
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11PersistentSrcTextureRendererData::~D3D11PersistentSrcTextureRendererData()
{
    /* */
}

//------------------------------------------------------------------------------------------------
// streaming
//------------------------------------------------------------------------------------------------
NiImplementCreateObject(D3D11PersistentSrcTextureRendererData);

//------------------------------------------------------------------------------------------------
void D3D11PersistentSrcTextureRendererData::LoadBinary(NiStream& stream)
{
    NiTexture::RendererData::LoadBinary(stream);

    m_kPixelFormat.LoadBinary(stream);

    stream.ReadLinkID();   // m_spPalette
    NiStreamLoadBinary(stream, m_uiMipmapLevels);
    NiStreamLoadBinary(stream, m_uiPixelStride);

    efd::UInt32 widthArray[16];
    efd::UInt32 heightArray[16];
    efd::UInt32 offsetArray[16];

    for (efd::UInt32 i = 0; i < m_uiMipmapLevels; i++)
    {
        NiStreamLoadBinary(stream, widthArray[i]);
        NiStreamLoadBinary(stream, heightArray[i]);
        NiStreamLoadBinary(stream, offsetArray[i]);
    }

    NiStreamLoadBinary(stream, offsetArray[m_uiMipmapLevels]);

    if ((stream.GetFileVersion() >= NiStream::GetVersion(20, 2, 0, 6)))
    {
        m_uiPadOffsetInBytes = 0;
        NiStreamLoadBinary(stream, m_uiPadOffsetInBytes);
    }
    else
    {
        efd::Memcpy(&m_uiPadOffsetInBytes, &offsetArray[m_uiMipmapLevels], 4);
    }

    NiStreamLoadBinary(stream, m_uiFaces);

    if (stream.GetFileVersion() < NiStream::GetVersion(30, 1, 0, 1))
    {
        PlatformID tempPlatformID;
        NiStreamLoadEnum(stream, tempPlatformID);

        switch (tempPlatformID)
        {
        case NI_XENON:
            m_eTargetRenderer = efd::SystemDesc::RENDERER_XBOX360;
            break;
        case NI_PS3:
            m_eTargetRenderer = efd::SystemDesc::RENDERER_PS3;
            break;
        case NI_DX9:
            m_eTargetRenderer = efd::SystemDesc::RENDERER_DX9;
            break;
        case NI_WII:
            m_eTargetRenderer = efd::SystemDesc::RENDERER_WII;
            break;
        case NI_D3D10:
            m_eTargetRenderer = efd::SystemDesc::RENDERER_D3D10;
            break;
        case NI_ANY:
        default:
            m_eTargetRenderer = efd::SystemDesc::RENDERER_GENERIC;
            break;
        }
    }
    else
    {
        NiStreamLoadEnum(stream, m_eTargetRenderer);
    }

    AllocateData(m_uiMipmapLevels, m_uiFaces, offsetArray[m_uiMipmapLevels]);

    efd::UInt32 destSize =  m_uiMipmapLevels << 2;
    efd::Memcpy(m_puiWidth, &widthArray, destSize);
    efd::Memcpy(m_puiHeight, &heightArray, destSize);

    destSize = (m_uiMipmapLevels + 1) << 2;
    efd::Memcpy(m_puiOffsetInBytes, &offsetArray, destSize);

    NiStreamLoadBinary(stream, m_pucPixels, m_puiOffsetInBytes[m_uiMipmapLevels] * m_uiFaces);

    // If in "tool mode streaming" mode, pristine copies of data must be saved
    // to guarantee what will be streamed out matches what has been streamed
    // in.
    if (ms_bToolModeStreaming)
    {
        m_uiPristineMaxOffsetInBytes = m_puiOffsetInBytes[m_uiMipmapLevels];
        m_uiPristinePadOffsetInBytes = m_uiPadOffsetInBytes;
        efd::UInt32 totalPixelMemory = m_uiPristineMaxOffsetInBytes * m_uiFaces;
        m_pucPristinePixels = EE_ALLOC2(efd::UInt8, totalPixelMemory, NiMemHint::TEXTURE);
        EE_ASSERT(m_pucPristinePixels);
        efd::Memcpy(m_pucPristinePixels, m_pucPixels, totalPixelMemory);
    }
}

//------------------------------------------------------------------------------------------------
void D3D11PersistentSrcTextureRendererData::LinkObject(NiStream& kStream)
{
    NiPersistentSrcTextureRendererData::LinkObject(kStream);
}

//------------------------------------------------------------------------------------------------
bool D3D11PersistentSrcTextureRendererData::RegisterStreamables(
    NiStream& kStream)
{
    if (!NiPersistentSrcTextureRendererData::RegisterStreamables(kStream))
        return false;

    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11PersistentSrcTextureRendererData::SaveBinary(NiStream& kStream)
{
    NiPersistentSrcTextureRendererData::SaveBinary(kStream);
}

//------------------------------------------------------------------------------------------------
bool D3D11PersistentSrcTextureRendererData::GetStreamableRTTIName(
    efd::Char* rttiName, 
    efd::UInt32 maxSize) const
{
    return NiPersistentSrcTextureRendererData::GetRTTI()->CopyName(rttiName, maxSize);
}

//------------------------------------------------------------------------------------------------
bool D3D11PersistentSrcTextureRendererData::IsEqual(NiObject* pkObject)
{
    EE_ASSERT(NiIsKindOf(D3D11PersistentSrcTextureRendererData, pkObject));
    if (!NiIsKindOf(D3D11PersistentSrcTextureRendererData, pkObject))
        return false;

    if (!NiPersistentSrcTextureRendererData::IsEqual(pkObject))
        return false;

    return true;
}

//------------------------------------------------------------------------------------------------
void D3D11PersistentSrcTextureRendererData::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiPersistentSrcTextureRendererData::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(
        D3D11PersistentSrcTextureRendererData::ms_RTTI.GetName()));
}

//------------------------------------------------------------------------------------------------
