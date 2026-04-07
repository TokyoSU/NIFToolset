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

#include "NiTerrainPCH.h"

#include "NiSurface.h"
#include "NiTerrainMaterial.h"
#include "NiTerrainUtils.h"

#include <NiBooleanExtraData.h>
#include <NiDevImageConverter.h>
#include <NiSourceTexture.h>

#include <efd/Logger.h>
#include <efd/ecrLogIDs.h>

//--------------------------------------------------------------------------------------------------
static NiPixelData* LoadSlotImage(const NiSurface::TextureSlotEntry* pkSlot)
{
    // If no slot, then no image
    if (!pkSlot)
        return NULL;

    // Attempt to read the image from file
    NiDevImageConverter kImageConverter;
    NiPixelData* pkPixelData = kImageConverter.ReadImageFile(pkSlot->m_kFilePath, 
        NULL);

    if (!pkPixelData)
        return NULL;

    // Make sure this image is decompressed
    const NiPixelFormat kSrcFormat = pkPixelData->GetPixelFormat();
    if (kSrcFormat.GetCompressed())
    {
        // Expand the image format out so we can collage it's channels
        NiPixelFormat kDstFormat = NiPixelFormat::RGBA32;
        NiPixelData* pkDstPixelData = 
            kImageConverter.ConvertPixelData(*pkPixelData, kDstFormat, NULL, false);
        EE_DELETE(pkPixelData);
        
        if (pkDstPixelData)
            pkPixelData = pkDstPixelData;
        else
            return NULL;
    }

    // error checking
    if (!pkPixelData)
    {
        EE_LOG(efd::kGamebryoGeneral0, efd::ILogger::kERR1,
            ("Terrain Surface Compilation failed: Could not read surface material slot image. %s", 
            (const char*)pkSlot->m_kFilePath));
    }

    return pkPixelData;
}

//--------------------------------------------------------------------------------------------------
static NiSourceTexture* CombineBaseMapDistributionMap(const NiSurface* pkSurface)
{
    // Fetch the slots
    const NiSurface::TextureSlotEntry* pkBaseMap = pkSurface->GetTextureSlotEntry(
        NiSurface::SURFACE_MAP_DIFFUSE);
    const NiSurface::TextureSlotEntry* pkDistMap = pkSurface->GetTextureSlotEntry(
        NiSurface::SURFACE_MAP_DISTRIBUTION);

    // Load the images
    NiPixelDataPtr spBaseImage = LoadSlotImage(pkBaseMap);
    NiPixelDataPtr spDistImage = LoadSlotImage(pkDistMap);

    // Build the texture
    return NiTerrainUtils::CombineImageChannelsToTexture(
        spBaseImage, NiPixelFormat::COMP_RED,
        spBaseImage, NiPixelFormat::COMP_GREEN,
        spBaseImage, NiPixelFormat::COMP_BLUE,
        spDistImage, NiPixelFormat::COMP_RED);
}

//--------------------------------------------------------------------------------------------------
static NiSourceTexture* CombineNormalMapParallaxMap(const NiSurface* pkSurface)
{
    // Fetch the slots
    const NiSurface::TextureSlotEntry* pkNormalMap = pkSurface->GetTextureSlotEntry(
        NiSurface::SURFACE_MAP_NORMAL);
    const NiSurface::TextureSlotEntry* pkParallaxMap = pkSurface->GetTextureSlotEntry(
        NiSurface::SURFACE_MAP_PARALLAX);

    // Load the images
    NiPixelDataPtr spNormalImage = LoadSlotImage(pkNormalMap);
    NiPixelDataPtr spParallaxImage = LoadSlotImage(pkParallaxMap);

    // Build the texture
    return NiTerrainUtils::CombineImageChannelsToTexture(
        spNormalImage, NiPixelFormat::COMP_RED,
        spNormalImage, NiPixelFormat::COMP_GREEN,
        spNormalImage, NiPixelFormat::COMP_BLUE,
        spParallaxImage, NiPixelFormat::COMP_RED);
}

//--------------------------------------------------------------------------------------------------
static NiSourceTexture* CombineSpecularMapDetailMap(const NiSurface* pkSurface)
{
    // Fetch the slots
    const NiSurface::TextureSlotEntry* pkSpecularMap = pkSurface->GetTextureSlotEntry(
        NiSurface::SURFACE_MAP_SPECULAR);
    const NiSurface::TextureSlotEntry* pkDetailMap = pkSurface->GetTextureSlotEntry(
        NiSurface::SURFACE_MAP_DETAIL);

    // Load the images
    NiPixelDataPtr spSpecularImage = LoadSlotImage(pkSpecularMap);
    NiPixelDataPtr spDetailImage = LoadSlotImage(pkDetailMap);

    // Build the texture
    return NiTerrainUtils::CombineImageChannelsToTexture(
        spSpecularImage, NiPixelFormat::COMP_RED,
        spSpecularImage, NiPixelFormat::COMP_GREEN,
        spSpecularImage, NiPixelFormat::COMP_BLUE,
        spDetailImage, NiPixelFormat::COMP_RED);
}

//--------------------------------------------------------------------------------------------------
NiTexturingProperty* NiSurface::GenerateSurfaceTextures() const
{
    // If a surface is not resolved, then all texture asset ids have not yet been converted to 
    // file paths for loading.
    if (!IsResolved()) 
        return NULL;

    // Make a new texturing property
    NiTexturingProperty* pkTexProp = NiNew NiTexturingProperty();

    // BaseMap : RGB = BaseMap, A = DistributionMap
    NiSourceTexturePtr spBaseMapTex = CombineBaseMapDistributionMap(this);
    NiTexturingProperty::ShaderMap* pkBaseMap = NiNew NiTexturingProperty::ShaderMap(spBaseMapTex, 
        0, NiTexturingProperty::WRAP_S_WRAP_T, NiTexturingProperty::FILTER_TRILERP, 
        NiTerrainMaterial::BASE_MAP);
    pkTexProp->SetShaderMap(SURFACE_TEX_DIFFUSE_DETAIL, pkBaseMap);

    // NormalMap : RGB = NormalMap, A = ParallaxMap
    NiTexturingProperty::ShaderMap* pkNormalMap = NULL;
    NiSourceTexturePtr spNormalMapTex = CombineNormalMapParallaxMap(this);
    pkNormalMap = NiNew NiTexturingProperty::ShaderMap(spNormalMapTex, 
        0, NiTexturingProperty::WRAP_S_WRAP_T, NiTexturingProperty::FILTER_TRILERP, 
        NiTerrainMaterial::NORMAL_MAP);        
    pkTexProp->SetShaderMap(SURFACE_TEX_NORMAL_PARALLAX, pkNormalMap);

    // SpecularMap : RGB = Specular, A = DetailMap
    NiTexturingProperty::ShaderMap* pkSpecularMap = NULL;
    NiSourceTexturePtr spSpecularMapTex = CombineSpecularMapDetailMap(this);
    pkSpecularMap = NiNew NiTexturingProperty::ShaderMap(spSpecularMapTex, 
        0, NiTexturingProperty::WRAP_S_WRAP_T, NiTexturingProperty::FILTER_TRILERP, 
        NiTerrainMaterial::SPEC_MAP);
    pkTexProp->SetShaderMap(SURFACE_TEX_SPECULAR_DISTRIBUTION, pkSpecularMap);

    return pkTexProp;
}


