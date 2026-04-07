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
#include "NiMainPCH.h"
#include "NiShadowGenerator.h"
#include "NiShadowTechnique.h"

NiImplementRootRTTI(NiShadowTechnique);

//--------------------------------------------------------------------------------------------------
NiShadowTechnique::NiShadowTechnique(const NiFixedString& kName,
    const NiFixedString kReadFragmentName,
    const NiFixedString kWriteFragmentName, bool, bool)
{
    NiShadowTechnique(kName,
        kReadFragmentName, kReadFragmentName, kReadFragmentName,
        kWriteFragmentName, kWriteFragmentName, kWriteFragmentName,
        m_bUseCubeMapForPointLight, m_bWriteBatchable);
}

//--------------------------------------------------------------------------------------------------
NiShadowTechnique::NiShadowTechnique(const NiFixedString& kName,
    const NiFixedString kDirReadFragmentName,
    const NiFixedString kPointReadFragmentName,
    const NiFixedString kSpotReadFragmentName,
    const NiFixedString kDirWriteFragmentName,
    const NiFixedString kPointWriteFragmentName,
    const NiFixedString kSpotWriteFragmentName,
    bool bUseCubeMapForPointLight,
    bool bWriteBatchable) :
    m_uiActiveTechniqueSlot(INVALID_SHADOWTECHNIQUE_ID),
    m_usTechniqueID(INVALID_SHADOWTECHNIQUE_ID),
    m_bWriteBatchable(bWriteBatchable),
    m_bUseCubeMapForPointLight(bUseCubeMapForPointLight),
    m_uiGuardBandSize(0)
{
    for (unsigned int i = 0; i < NiStandardMaterial::LIGHT_MAX; ++i)
    {
        m_apkDepthFormats[i] = 0;
        m_aeFilterModes[i] = NiTexturingProperty::FILTER_NEAREST;
        m_aeClampModes[i] = NiTexturingProperty::CLAMP_S_CLAMP_T;
        m_akFormatPrefs[i].m_eMipMapped = NiTexture::FormatPrefs::NO;
    }
    
    // Gamebryo currently only supports hardware accelerated shadow maps on
    // Xbox 360 & PS3 for spot and directional lights.
#if defined(EE_PLATFORM_PS3) || defined(EE_PLATFORM_XBOX360)
    m_akFormatPrefs[NiStandardMaterial::LIGHT_DIR].m_ePixelLayout = 
        NiTexture::FormatPrefs::DEPTH_24_X8;
    m_akFormatPrefs[NiStandardMaterial::LIGHT_POINT].m_ePixelLayout = 
        NiTexture::FormatPrefs::SINGLE_COLOR_32;        
    m_akFormatPrefs[NiStandardMaterial::LIGHT_SPOT].m_ePixelLayout = 
        NiTexture::FormatPrefs::DEPTH_24_X8;
#else
    m_akFormatPrefs[NiStandardMaterial::LIGHT_DIR].m_ePixelLayout = 
        NiTexture::FormatPrefs::SINGLE_COLOR_32;
    m_akFormatPrefs[NiStandardMaterial::LIGHT_POINT].m_ePixelLayout = 
        NiTexture::FormatPrefs::SINGLE_COLOR_32;
    m_akFormatPrefs[NiStandardMaterial::LIGHT_SPOT].m_ePixelLayout = 
        NiTexture::FormatPrefs::SINGLE_COLOR_32;
#endif



    m_kName = kName;

    for (int iIndex = 0; iIndex < NiStandardMaterial::LIGHT_MAX * 2; iIndex++)
    {
        m_afDefaultDepthBias[iIndex] = 0.0f;
    }

    m_kReadFragmentName[NiStandardMaterial::LIGHT_DIR] =
        kDirReadFragmentName;
    m_kReadFragmentName[NiStandardMaterial::LIGHT_POINT] =
        kPointReadFragmentName;
    m_kReadFragmentName[NiStandardMaterial::LIGHT_SPOT] =
        kSpotReadFragmentName;

    m_kWriteFragmentName[NiStandardMaterial::LIGHT_DIR] =
        kDirWriteFragmentName;
    m_kWriteFragmentName[NiStandardMaterial::LIGHT_POINT] =
        kPointWriteFragmentName;
    m_kWriteFragmentName[NiStandardMaterial::LIGHT_SPOT] =
        kSpotWriteFragmentName;
}

//--------------------------------------------------------------------------------------------------
void NiShadowTechnique::SetActiveTechniqueSlot(unsigned short usSlot)
{
    // Ensure that the active shadow technique slot we are telling this
    // NiShadowTechique it is assigned to, is really the slot it is assigned.
    EE_ASSERT(NiShadowManager::GetActiveShadowTechnique(usSlot) == this);

    m_uiActiveTechniqueSlot = usSlot;
}

//--------------------------------------------------------------------------------------------------
