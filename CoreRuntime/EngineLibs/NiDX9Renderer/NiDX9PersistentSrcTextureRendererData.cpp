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


#include "NiD3DRendererPCH.h"   // Precompiled Header

#include <NiImageConverter.h>

#include "NiDX9PersistentSrcTextureRendererData.h"
#include "NiDX9Renderer.h"

NiImplementRTTI(NiDX9PersistentSrcTextureRendererData,
    NiPersistentSrcTextureRendererData, NiTypeMask::NiDX9PersistentSrcTextureRendererData);

//--------------------------------------------------------------------------------------------------
NiDX9PersistentSrcTextureRendererData::NiDX9PersistentSrcTextureRendererData()
{
}

//--------------------------------------------------------------------------------------------------
NiDX9PersistentSrcTextureRendererData::~NiDX9PersistentSrcTextureRendererData()
{
    // Defer to base class.
}

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiDX9PersistentSrcTextureRendererData);

//--------------------------------------------------------------------------------------------------
void NiDX9PersistentSrcTextureRendererData::LoadBinary(NiStream& kStream)
{
    NiTexture::RendererData::LoadBinary(kStream);

    m_kPixelFormat.LoadBinary(kStream);

    kStream.ReadLinkID();   // m_spPalette
    NiStreamLoadBinary(kStream, m_uiMipmapLevels);
    NiStreamLoadBinary(kStream, m_uiPixelStride);

    unsigned int auiWidth[16], auiHeight[16], auiOffsetInBytes[16];

    for (unsigned int i = 0; i < m_uiMipmapLevels; i++)
    {
        NiStreamLoadBinary(kStream, auiWidth[i]);
        NiStreamLoadBinary(kStream, auiHeight[i]);
        NiStreamLoadBinary(kStream, auiOffsetInBytes[i]);
    }

    NiStreamLoadBinary(kStream, auiOffsetInBytes[m_uiMipmapLevels]);

    if ((kStream.GetFileVersion() >= NiStream::GetVersion(20, 2, 0, 6)))
    {
        m_uiPadOffsetInBytes = 0;
        NiStreamLoadBinary(kStream, m_uiPadOffsetInBytes);
    }
    else
    {
        NiMemcpy(&m_uiPadOffsetInBytes, &auiOffsetInBytes[m_uiMipmapLevels],
            4);
    }

    NiStreamLoadBinary(kStream, m_uiFaces);

    if (kStream.GetFileVersion() < NiStream::GetVersion(30, 1, 0, 1))
    {
        PlatformID eTemp;
        NiStreamLoadEnum(kStream, eTemp);

        switch (eTemp)
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
        NiStreamLoadEnum(kStream, m_eTargetRenderer);
    }

    AllocateData(m_uiMipmapLevels, m_uiFaces,
        auiOffsetInBytes[m_uiMipmapLevels]);

    unsigned int uiDestSize =  m_uiMipmapLevels << 2;
    NiMemcpy(m_puiWidth, &auiWidth, uiDestSize);
    NiMemcpy(m_puiHeight, &auiHeight, uiDestSize);

    uiDestSize = (m_uiMipmapLevels + 1) << 2;
    NiMemcpy(m_puiOffsetInBytes, &auiOffsetInBytes, uiDestSize);

    NiStreamLoadBinary(kStream, m_pucPixels,
        m_puiOffsetInBytes[m_uiMipmapLevels] * m_uiFaces);

    // If in "tool mode streaming" mode, pristine copies of data must be saved
    // to guarantee what will be streamed out matches what has been streamed
    // in.
    if (ms_bToolModeStreaming)
    {
        m_uiPristineMaxOffsetInBytes = m_puiOffsetInBytes[m_uiMipmapLevels];
        m_uiPristinePadOffsetInBytes = m_uiPadOffsetInBytes;
        unsigned int uiTotalPixelMemory =
            m_uiPristineMaxOffsetInBytes * m_uiFaces;
        m_pucPristinePixels = NiAlloc2(unsigned char, uiTotalPixelMemory, NiMemHint::TEXTURE);
        EE_ASSERT(m_pucPristinePixels);
        NiMemcpy(m_pucPristinePixels, m_pucPixels, uiTotalPixelMemory);
    }
}

//--------------------------------------------------------------------------------------------------
void NiDX9PersistentSrcTextureRendererData::LinkObject(NiStream& kStream)
{
    NiPersistentSrcTextureRendererData::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiDX9PersistentSrcTextureRendererData::RegisterStreamables(
    NiStream& kStream)
{
    if (!NiPersistentSrcTextureRendererData::RegisterStreamables(kStream))
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiDX9PersistentSrcTextureRendererData::SaveBinary(NiStream& kStream)
{
    NiPersistentSrcTextureRendererData::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiDX9PersistentSrcTextureRendererData::GetStreamableRTTIName(char* acName,
    unsigned int uiMaxSize) const
{
    return NiPersistentSrcTextureRendererData::GetRTTI()->CopyName(acName,
        uiMaxSize);
}

//--------------------------------------------------------------------------------------------------
bool NiDX9PersistentSrcTextureRendererData::IsEqual(NiObject* pkObject)
{
    EE_ASSERT(NiIsKindOf(NiDX9PersistentSrcTextureRendererData, pkObject));
    if (!NiIsKindOf(NiDX9PersistentSrcTextureRendererData, pkObject))
        return false;

    if (!NiPersistentSrcTextureRendererData::IsEqual(pkObject))
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiDX9PersistentSrcTextureRendererData::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiPersistentSrcTextureRendererData::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(
        NiDX9PersistentSrcTextureRendererData::ms_RTTI.GetName()));
}

//--------------------------------------------------------------------------------------------------
