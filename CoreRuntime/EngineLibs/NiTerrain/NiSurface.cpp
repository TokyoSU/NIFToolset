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
#include "NiSurfacePackage.h"
#include "NiSourceTexture.h"
#include "NiTerrainMaterial.h"
#include "NiTerrainSurfacePackageFile.h"
#include "NiTerrainSurfaceLibrary.h"

using namespace efd;

//--------------------------------------------------------------------------------------------------
NiFixedString * NiSurface::ms_pkSurfaceMapNames = NULL;
const char* NiSurface::CONFIG_EXTRADATA_KEY = "SurfaceConfigData";
NiSurface* NiSurface::ms_pkLoadingSurface = NULL;
NiSurface* NiSurface::ms_pkErrorSurface = NULL;
NiFixedString NiSurface::ms_kErrorSurfaceName = NULL;
NiFixedString NiSurface::ms_kLoadingSurfaceName = NULL;
const char* NiSurface::MISSING_TEXTURE_PATH = "[Failed to resolve URN]";
//--------------------------------------------------------------------------------------------------
void NiSurface::_SDMInit()
{
    ms_pkSurfaceMapNames = NiNew NiFixedString[NUM_SURFACE_MAPS];
    ms_pkSurfaceMapNames[SURFACE_MAP_DIFFUSE] = "BaseMap";
    ms_pkSurfaceMapNames[SURFACE_MAP_DETAIL] = "DetailMap";
    ms_pkSurfaceMapNames[SURFACE_MAP_NORMAL] = "NormalMap";
    ms_pkSurfaceMapNames[SURFACE_MAP_PARALLAX] = "ParallaxMap";
    ms_pkSurfaceMapNames[SURFACE_MAP_DISTRIBUTION] = "DistributionMap";
    ms_pkSurfaceMapNames[SURFACE_MAP_SPECULAR] = "SpecularMap";

    NiSurface::SetErrorSurface(NULL);
    NiSurface::SetLoadingSurface(NULL);
    ms_kErrorSurfaceName = "Error";
    ms_kLoadingSurfaceName = "Loading";
}

//--------------------------------------------------------------------------------------------------
void NiSurface::_SDMShutdown()
{
    NiSurface::SetErrorSurface(NULL);
    NiSurface::SetLoadingSurface(NULL);
    ms_kErrorSurfaceName = NULL;
    ms_kLoadingSurfaceName = NULL;
    NiDelete[] ms_pkSurfaceMapNames;
}

//--------------------------------------------------------------------------------------------------
void NiSurface::SetErrorSurface(NiSurface* pkSurface)
{
    if (ms_pkErrorSurface)
        EE_DELETE ms_pkErrorSurface;
    ms_pkErrorSurface = pkSurface;
}

//--------------------------------------------------------------------------------------------------
void NiSurface::SetLoadingSurface(NiSurface* pkSurface)
{
    if (ms_pkLoadingSurface)
        EE_DELETE ms_pkLoadingSurface;
    ms_pkLoadingSurface = pkSurface;
}

//--------------------------------------------------------------------------------------------------
NiSurface* NiSurface::GetErrorSurface()
{
    NiSurface* pkSurface = ms_pkErrorSurface;
    
    if (!pkSurface)
    {
        // Create a base texture to show on the surface
        NiPixelData* pkPixelData = EE_NEW NiPixelData(1, 1, NiPixelFormat::RGB24);
        efd::UInt8* pucPixels = pkPixelData->GetPixels();
        pucPixels[0] = 255;
        pucPixels[1] = 0;
        pucPixels[2] = 0;
        NiTexture* pkTexture = NiSourceTexture::Create(pkPixelData);
        pkSurface = CreateSpecialSurface(pkTexture);
        pkSurface->SetName(ms_kErrorSurfaceName);
        SetErrorSurface(pkSurface);
    }
    
    return pkSurface;
}

//--------------------------------------------------------------------------------------------------
NiSurface* NiSurface::GetLoadingSurface()
{
    NiSurface* pkSurface = ms_pkLoadingSurface;

    if (!pkSurface)
    {
        // Create a base texture to show on the surface
        NiPixelData* pkPixelData = EE_NEW NiPixelData(1, 1, NiPixelFormat::RGB24);
        efd::UInt8* pucPixels = pkPixelData->GetPixels();
        pucPixels[0] = 0;
        pucPixels[1] = 0;
        pucPixels[2] = 0;
        NiTexture* pkTexture = NiSourceTexture::Create(pkPixelData);
        pkSurface = CreateSpecialSurface(pkTexture);
        pkSurface->SetName(ms_kLoadingSurfaceName);
        SetLoadingSurface(pkSurface);
    }

    return pkSurface;
}

//--------------------------------------------------------------------------------------------------
NiSurface* NiSurface::CreateSpecialSurface(NiTexture* pkBaseTexture)
{
    // Create an error surface
    NiSurface* pkSurface = EE_NEW NiSurface();
    NiTexturingProperty* pkTexProp = EE_NEW NiTexturingProperty();
    pkSurface->m_spTexProp = pkTexProp;

    // Setup the error surface's textures
    NiTexturingProperty::ShaderMap* pkShaderMap = EE_NEW NiTexturingProperty::ShaderMap(pkBaseTexture, 
        0, NiTexturingProperty::WRAP_S_WRAP_T, NiTexturingProperty::FILTER_TRILERP, 
        NiTerrainMaterial::BASE_MAP);
    pkTexProp->SetShaderMap(SURFACE_TEX_DIFFUSE_DETAIL, pkShaderMap);

    pkShaderMap = NiNew NiTexturingProperty::ShaderMap(NULL, 
        0, NiTexturingProperty::WRAP_S_WRAP_T, NiTexturingProperty::FILTER_TRILERP, 
        NiTerrainMaterial::NORMAL_MAP);     
    pkTexProp->SetShaderMap(SURFACE_TEX_NORMAL_PARALLAX, pkShaderMap);

    pkShaderMap = NiNew NiTexturingProperty::ShaderMap(NULL, 
        0, NiTexturingProperty::WRAP_S_WRAP_T, NiTexturingProperty::FILTER_TRILERP, 
        NiTerrainMaterial::SPEC_MAP);     
    pkTexProp->SetShaderMap(SURFACE_TEX_SPECULAR_DISTRIBUTION, pkShaderMap);

    return pkSurface;
}

//--------------------------------------------------------------------------------------------------
NiSurface::NiSurface() :
    m_kMetaData(),
    m_kName(""),
    m_pkPackage(NULL),
    m_fTextureTiling(1.0f),
    m_fDetailTextureTiling(4.0f),
    m_fRotation(0.0f),
    m_fParallaxStrength(0.05f),
    m_fDistributionMaskStrength(0.5f),
    m_fSpecularPower(1.0f),
    m_fSpecularIntensity(0.5f),
    m_bIsResolved(false),
    m_bCompiled(false),
    m_bIsActive(false)
{ 
    for (efd::SInt32 i = 0; i < NUM_SURFACE_MAPS; i++)
        m_apkTextureSlots[i] = NULL;
}

//--------------------------------------------------------------------------------------------------
NiSurface::~NiSurface()
{
    m_spTexProp = NULL;
    for (efd::SInt32 i = 0; i < NUM_SURFACE_MAPS; i++)
    {
        if (m_apkTextureSlots[i])
            EE_DELETE m_apkTextureSlots[i];
    }
}

//--------------------------------------------------------------------------------------------------
const char* NiSurface::GetTextureSlotName(SurfaceMapID eMapID)
{
    EE_ASSERT(eMapID < NUM_SURFACE_MAPS);
    return ms_pkSurfaceMapNames[eMapID];
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiSurface::GetSurfaceCaps() const
{
    NiUInt32 uiResult = 0;

    if (m_apkTextureSlots[SURFACE_MAP_DIFFUSE])
        uiResult |= SURFACE_CAPS_DIFFUSE;
    if (m_apkTextureSlots[SURFACE_MAP_DETAIL])
        uiResult |= SURFACE_CAPS_DETAIL;
    if (m_apkTextureSlots[SURFACE_MAP_NORMAL])
        uiResult |= SURFACE_CAPS_NORMAL;
    if (m_apkTextureSlots[SURFACE_MAP_PARALLAX])
        uiResult |= SURFACE_CAPS_PARALLAX;
    if (m_apkTextureSlots[SURFACE_MAP_DISTRIBUTION])
        uiResult |= SURFACE_CAPS_DISTRIBUTION;
    if (m_apkTextureSlots[SURFACE_MAP_SPECULAR])
        uiResult |= SURFACE_CAPS_SPECULAR;

    return uiResult;
}

//--------------------------------------------------------------------------------------------------
NiTexturingProperty* NiSurface::MergeSurfaces(NiTexturingProperty* pkTexProp, 
    const NiSurface* pkSurface) const
{
    EE_ASSERT(pkSurface);
    EE_ASSERT(pkTexProp);

    // Make sure our texturing property is ready
    const_cast<NiSurface*>(pkSurface)->PrepareForUse();

    // Fetch the texturing property
    NiTexturingProperty* pkTexPropToMerge = pkSurface->GetTexturingProperty();
    EE_ASSERT(pkTexPropToMerge);
    
    // Add the texturing propery to the existing one
    efd::UInt32 uiShaderMapCount = pkTexPropToMerge->GetShaderMapCount();
    EE_ASSERT(uiShaderMapCount == NUM_SURFACE_TEXTURES);
    EE_ASSERT(((pkTexProp->GetShaderMapCount() - 1) % 3) == 0);
    for (efd::UInt32 ui = 0; ui < uiShaderMapCount; ui++)
    {
        NiTexturingProperty::ShaderMap* pkMap = pkTexPropToMerge->GetShaderMap(ui);
        EE_ASSERT(pkMap);

        NiTexturingProperty::ShaderMap* pkCloneMap = EE_NEW NiTexturingProperty::ShaderMap();
        *pkCloneMap = *pkMap;
        pkTexProp->SetShaderMap(pkTexProp->GetShaderMapCount(), pkCloneMap);
    }

    return pkTexProp;
}

//--------------------------------------------------------------------------------------------------
void NiSurface::PrepareForUse()
{
    // Check if one already exists
    NiTexturingProperty* pkTexPropToMerge = GetTexturingProperty();
    if (pkTexPropToMerge)
        return;
    
    // Attempt to load from a precompiled texture file
    LoadCachedTextures();

    // Check if that worked
    pkTexPropToMerge = GetTexturingProperty();
    if (pkTexPropToMerge)
        return;

    // Attempt to compile the surface textures now
    CompileSurface();
}

//--------------------------------------------------------------------------------------------------
void NiSurface::LoadCachedTextures()
{
    // Check if this surface belongs to a proper package
    NiSurfacePackage* pkPackage = GetPackage();
    if (!pkPackage)
        return;
    
    // Fetch the library the package belongs to
    NiTerrainSurfaceLibrary* pkLibrary = GetPackage()->GetLibrary();
    if (!pkLibrary)
        return;

    // Attempt to open the host package's file
    NiTerrainSurfacePackageFile* pkFile = pkLibrary->OpenPackageFile(
        pkPackage->GetFilename(), false);
    if (!pkFile)
        return;

    // Request that only the compiled textures be loaded from disk
    pkFile->Precache(NiTerrainSurfacePackageFile::DataField::COMPILED_TEXTURES);
    
    // Read the configuration:
    efd::utf8string kPackageName;
    efd::UInt32 uiIteration = 0;
    pkFile->ReadPackageConfig(kPackageName, uiIteration);
    if (uiIteration != m_pkPackage->GetIteration())
    {
        pkFile->Close();
        NiDelete pkFile;
        return;
    }
    
    // Read a specific surface's set of textures from the file
    NiTexturePtr aspTexures[NUM_SURFACE_TEXTURES];
    if (pkFile->ReadSurfaceCompiledTextures((const char*)GetName(), aspTexures, 
        NUM_SURFACE_TEXTURES))
    {
        // Clear the old texture property.
        NiTexturingProperty* pkTexProp = NiNew NiTexturingProperty();

        for (efd::UInt32 uiTex = 0; uiTex < NUM_SURFACE_TEXTURES; ++uiTex)
        {
            // Remove the compiled texture's pixel data from memory:
            NiSourceTexture* pkSourceTexture = NiDynamicCast(NiSourceTexture, aspTexures[uiTex]);
            if (pkSourceTexture)
                pkSourceTexture->DestroyAppPixelData();
        }        

        // BaseMap : RGB = BaseMap, A = DistributionMap
        NiTexturingProperty::ShaderMap* pkBaseMap = NiNew NiTexturingProperty::ShaderMap(
            aspTexures[SURFACE_TEX_DIFFUSE_DETAIL],
            0, NiTexturingProperty::WRAP_S_WRAP_T, NiTexturingProperty::FILTER_TRILERP, 
            NiTerrainMaterial::BASE_MAP);
        pkTexProp->SetShaderMap(SURFACE_TEX_DIFFUSE_DETAIL, pkBaseMap);

        // NormalMap : RGB = NormalMap, A = ParallaxMap
        NiTexturingProperty::ShaderMap* pkNormalMap = NULL;
        pkNormalMap = NiNew NiTexturingProperty::ShaderMap(
            aspTexures[SURFACE_TEX_NORMAL_PARALLAX], 
            0, NiTexturingProperty::WRAP_S_WRAP_T, NiTexturingProperty::FILTER_TRILERP, 
            NiTerrainMaterial::NORMAL_MAP);        
        pkTexProp->SetShaderMap(SURFACE_TEX_NORMAL_PARALLAX, pkNormalMap);

        // SpecularMap : RGB = Specular, A = DetailMap
        NiTexturingProperty::ShaderMap* pkSpecularMap = NULL;
        pkSpecularMap = NiNew NiTexturingProperty::ShaderMap(
            aspTexures[SURFACE_TEX_SPECULAR_DISTRIBUTION], 
            0, NiTexturingProperty::WRAP_S_WRAP_T, NiTexturingProperty::FILTER_TRILERP, 
            NiTerrainMaterial::SPEC_MAP);
        pkTexProp->SetShaderMap(SURFACE_TEX_SPECULAR_DISTRIBUTION, pkSpecularMap);

        SetTexturingProperty(pkTexProp);
    }

    // Cleanup
    pkFile->Close();
    NiDelete pkFile;
}

//--------------------------------------------------------------------------------------------------
void NiSurface::CompileSurface()
{
    m_bCompiled = false;
    m_spTexProp = GenerateSurfaceTextures();
    m_bCompiled = true;
}

//--------------------------------------------------------------------------------------------------
