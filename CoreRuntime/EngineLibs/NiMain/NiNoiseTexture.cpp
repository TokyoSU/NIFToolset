// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#include "NiMainPCH.h"
#include "NiNoiseTexture.h"
//---------------------------------------------------------------------------

NiImplementRTTI(NiNoiseTexture, NiSourceTexture);
NiFixedString NiNoiseTexture::ms_akMapFromTypeName[
    NiStandardMaterial::NOISE_MAX];

//---------------------------------------------------------------------------
NiNoiseTexture::NiNoiseTexture()
{
}
//---------------------------------------------------------------------------
void NiNoiseTexture::_SDMInit()
{
    ms_akMapFromTypeName[NiStandardMaterial::NOISE_GREYSCALE] = 
        "GreyscaleNoise";
}
//---------------------------------------------------------------------------
void NiNoiseTexture::_SDMShutdown()
{
    ms_akMapFromTypeName[NiStandardMaterial::NOISE_GREYSCALE] = NULL;
}
//---------------------------------------------------------------------------
NiNoiseTexture* NiNoiseTexture::Create(NoiseType eNoiseType,
    NiUInt32 uiTextureSize, NiUInt32 uiRandSeed)
{
    // Create the pixel data
    NiPixelFormat kPixelFormat(
        NiPixelFormat::FORMAT_ONE_CHANNEL,
        NiPixelFormat::COMP_INTENSITY,
        NiPixelFormat::REP_NORM_INT,
        8);

    NiPixelData* pkPixelData = NiNew NiPixelData(
        uiTextureSize, uiTextureSize, kPixelFormat);

    if (!pkPixelData)
        return NULL;

    // Create the noise
    unsigned char* pucPixels = pkPixelData->GetPixels();
    NiUInt32 uiNumPixels = uiTextureSize * uiTextureSize;

    switch (eNoiseType)
    {
    default:
    case NT_RAND:
        {
            NiSrand(uiRandSeed);
            for (NiUInt32 ui = 0; ui < uiNumPixels; ++ui)
            {
                NiUInt32 uiRand = NiRand();
                *pucPixels = (unsigned char)(uiRand >> 7 & 255);
                pucPixels++;
            }
        }
    }

    // Texture format prefs
    NiTexture::FormatPrefs kPrefs;
    kPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::NONE;
    kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::SINGLE_COLOR_8;

    // Create the actual texture
    NiNoiseTexture* pkNoiseTexture = NiNew NiNoiseTexture;
    pkNoiseTexture->SetNoiseType(eNoiseType);
    pkNoiseTexture->m_spSrcPixelData = pkPixelData;
    pkNoiseTexture->m_kFormatPrefs = kPrefs;

    // Load texture to vid card?
    if (ms_bPreload && !pkNoiseTexture->CreateRendererData())
    {
        NiDelete pkNoiseTexture;
        return 0;
    }
    else
    {
        return pkNoiseTexture;
    }
}
//---------------------------------------------------------------------------
