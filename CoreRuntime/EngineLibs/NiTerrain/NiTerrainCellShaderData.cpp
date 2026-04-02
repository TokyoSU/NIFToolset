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

#include "NiTerrainPCH.h"
#include "NiTerrainCellShaderData.h"

#include <NiFloatsExtraData.h>
#include <NiFloatExtraData.h>
#include <NiIntegerExtraData.h>
#include <NiMesh.h>
#include "NiTerrain.h"

//---------------------------------------------------------------------------
const char* NiTerrainCellShaderData::STITCHINGINFO_SHADER_CONSTANT = "g_StitchingInfo";
const char* NiTerrainCellShaderData::LODTHRESHOLD_SHADER_CONSTANT = "g_LODThreshold";
const char* NiTerrainCellShaderData::LODMORPHDISTANCE_SHADER_CONSTANT = "g_MorphDistance";
const char* NiTerrainCellShaderData::MORPHMODE_SHADER_CONSTANT = "g_MorphMode";

const char* NiTerrainCellShaderData::LOWDETAIL_TEXTURE_SIZES_SHADER_CONSTANT =
    "g_LowDetailTextureSizes";
const char* NiTerrainCellShaderData::LOWDETAIL_TEXTURE_OFFSET_SHADER_CONSTANT = 
    "g_LowDetailTextureOffset";
const char* NiTerrainCellShaderData::LOWDETAIL_TEXTURE_SCALE_SHADER_CONSTANT = 
    "g_LowDetailTextureScale";
const char* NiTerrainCellShaderData::LOWDETAIL_SPECULAR_SHADER_CONSTANT = 
    "g_LowDetailTextureSpecularConstants";

const char* NiTerrainCellShaderData::BLENDMAP_SCALE_SHADER_CONSTANT = "g_BlendMapScale";
const char* NiTerrainCellShaderData::BLENDMAP_OFFSET_SHADER_CONSTANT = "g_BlendMapOffset";
const char* NiTerrainCellShaderData::LAYER_SCALE_SHADER_CONSTANT = "g_LayerScale";

const char* NiTerrainCellShaderData::DEBUGMODE_SHADER_CONSTANT = "g_TerrainDebugMode";
const char* NiTerrainCellShaderData::RENDERMODE_SHADER_CONSTANT = "g_TerrainRenderMode";

const char* NiTerrainCellShaderData::ADJUSTED_EYE_POSITION = "g_AdjustedEyePos";

const char* NiTerrainCellShaderData::DISTRIBUTION_RAMP_SHADER_CONSTANT = "g_DistRamp";
const char* NiTerrainCellShaderData::PARALLAX_MAP_STRENGTH_SHADER_CONSTANT = "g_ParallaxStrength";
const char* NiTerrainCellShaderData::SPECULAR_POWER_SHADER_CONSTANT = "g_SpecPower";
const char* NiTerrainCellShaderData::SPECULAR_INTENSITY_SHADER_CONSTANT = "g_SpecIntensity";
const char* NiTerrainCellShaderData::LAYER_DETAIL_TEXTURE_SCALE_SHADER_CONSTANT = 
    "g_DetailMapScale";

//---------------------------------------------------------------------------
NiTerrainCellShaderData::NiTerrainCellShaderData(NiTerrain* pkTerrain)
{
    // Setup the default shader data
    m_kStitchingInfo = NiPoint4::ZERO;
    m_kAdjustedEyePos = NiPoint3::ZERO;
    m_fMorphDistance = 0.0f;
    m_fMorphThreshold = 0.0f;
    m_kLowDetailTextureSize = NiPoint2(1024.0f, 1024.0f);
    m_kLowDetailTextureOffset = NiPoint2::ZERO;
    m_kLowDetailTextureScale = NiPoint2::ZERO;
    m_kLayerScale = NiPoint4(1.0f, 1.0f, 1.0f, 1.0f);
    m_kBlendMapOffset = NiPoint2(0.0f, 0.0f);
    m_kBlendMapScale = NiPoint2(1.0f, 1.0f);
    m_uiMorphMode = 0;
    
    m_kDistributionRamp = NiPoint4::ZERO;
    m_kParallaxStrength = NiPoint4(0.05f, 0.05f, 0.05f, 0.05f);
    m_kSpecularPower = NiPoint4::ZERO;
    m_kSpecularIntensity = NiPoint4::ZERO;
    m_kDetailTextureScale = NiPoint4::ZERO;

    // Fetch the terrain wide shader datapkTerrain->GetShaderData;
    m_pkDebugModeData = pkTerrain->GetExtraData(DEBUGMODE_SHADER_CONSTANT);
    m_pkRenderModeData = pkTerrain->GetExtraData(RENDERMODE_SHADER_CONSTANT);
    m_pkLowDetailSpecularData = pkTerrain->GetExtraData(LOWDETAIL_SPECULAR_SHADER_CONSTANT);
    m_pkTimeData = pkTerrain->GetExtraData(NiShaderConstantMap::GetTimeExtraDataName());
}
//---------------------------------------------------------------------------
void NiTerrainCellShaderData::InitializeShaderData(NiMesh* pkMesh)
{
    EE_ASSERT(pkMesh);
    float afDummyValues[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    // LOD information.
    pkMesh->AddExtraData(STITCHINGINFO_SHADER_CONSTANT, 
        NiNew NiFloatsExtraData(4, &afDummyValues[0]));

    pkMesh->AddExtraData(LODMORPHDISTANCE_SHADER_CONSTANT,
        NiNew NiFloatExtraData(m_fMorphDistance));

    pkMesh->AddExtraData(LODTHRESHOLD_SHADER_CONSTANT,
        NiNew NiFloatExtraData(m_fMorphThreshold));

    pkMesh->AddExtraData(MORPHMODE_SHADER_CONSTANT,
        NiNew NiIntegerExtraData(m_uiMorphMode));

    pkMesh->AddExtraData(ADJUSTED_EYE_POSITION,
        NiNew NiFloatsExtraData(3, &afDummyValues[0]));

    // Texture layer information
    pkMesh->AddExtraData(BLENDMAP_OFFSET_SHADER_CONSTANT, 
        NiNew NiFloatsExtraData(2, &afDummyValues[0]));

    pkMesh->AddExtraData(BLENDMAP_SCALE_SHADER_CONSTANT, 
        NiNew NiFloatsExtraData(2, &afDummyValues[0]));

    pkMesh->AddExtraData(LAYER_SCALE_SHADER_CONSTANT,
        NiNew NiFloatsExtraData(4, &afDummyValues[0]));

    // Low detail texture information.
    pkMesh->AddExtraData(LOWDETAIL_TEXTURE_SIZES_SHADER_CONSTANT,
        NiNew NiFloatsExtraData(2, &afDummyValues[0]));

    pkMesh->AddExtraData(LOWDETAIL_TEXTURE_OFFSET_SHADER_CONSTANT,
        NiNew NiFloatsExtraData(2, &afDummyValues[0]));

    pkMesh->AddExtraData(LOWDETAIL_TEXTURE_SCALE_SHADER_CONSTANT,
        NiNew NiFloatsExtraData(2, &afDummyValues[0]));

    pkMesh->AddExtraData(DISTRIBUTION_RAMP_SHADER_CONSTANT,
        NiNew NiFloatsExtraData(4, &afDummyValues[0]));

    pkMesh->AddExtraData(PARALLAX_MAP_STRENGTH_SHADER_CONSTANT,
        NiNew NiFloatsExtraData(4, &afDummyValues[0]));

    pkMesh->AddExtraData(SPECULAR_POWER_SHADER_CONSTANT,
        NiNew NiFloatsExtraData(4, &afDummyValues[0]));

    pkMesh->AddExtraData(SPECULAR_INTENSITY_SHADER_CONSTANT,
        NiNew NiFloatsExtraData(4, &afDummyValues[0]));

    pkMesh->AddExtraData(LAYER_DETAIL_TEXTURE_SCALE_SHADER_CONSTANT,
        NiNew NiFloatsExtraData(4, &afDummyValues[0]));

    // Terrain wide shader information.
    if (m_pkDebugModeData)
        pkMesh->AddExtraData(DEBUGMODE_SHADER_CONSTANT, m_pkDebugModeData);
    if (m_pkRenderModeData)
        pkMesh->AddExtraData(RENDERMODE_SHADER_CONSTANT, m_pkRenderModeData);
    if (m_pkLowDetailSpecularData)
        pkMesh->AddExtraData(LOWDETAIL_SPECULAR_SHADER_CONSTANT, m_pkLowDetailSpecularData);
    if (m_pkTimeData)
        pkMesh->AddExtraData(NiShaderConstantMap::GetTimeExtraDataName(), m_pkTimeData);
}
//---------------------------------------------------------------------------
void NiTerrainCellShaderData::UpdateShaderData(NiMesh* pkMesh)
{
    EE_ASSERT(pkMesh);

    // Lod information.
    NiFloatsExtraData* pkFloatExtraData = NiDynamicCast(NiFloatsExtraData, 
        pkMesh->GetExtraData(STITCHINGINFO_SHADER_CONSTANT));
    EE_ASSERT(pkFloatExtraData);
    pkFloatExtraData->SetValue(0, m_kStitchingInfo.X());
    pkFloatExtraData->SetValue(1, m_kStitchingInfo.Y());
    pkFloatExtraData->SetValue(2, m_kStitchingInfo.Z());
    pkFloatExtraData->SetValue(3, m_kStitchingInfo.W());

    NiFloatExtraData* pkSingleFloatExtraData = NiDynamicCast(NiFloatExtraData, 
        pkMesh->GetExtraData(LODMORPHDISTANCE_SHADER_CONSTANT));
    EE_ASSERT(pkSingleFloatExtraData);
    pkSingleFloatExtraData->SetValue(m_fMorphDistance);

    pkSingleFloatExtraData = NiDynamicCast(NiFloatExtraData, 
        pkMesh->GetExtraData(LODTHRESHOLD_SHADER_CONSTANT));
    EE_ASSERT(pkSingleFloatExtraData);
    pkSingleFloatExtraData->SetValue(m_fMorphThreshold);

    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData, 
        pkMesh->GetExtraData(ADJUSTED_EYE_POSITION));
    EE_ASSERT(pkFloatExtraData);
    pkFloatExtraData->SetValue(0, m_kAdjustedEyePos.x);
    pkFloatExtraData->SetValue(1, m_kAdjustedEyePos.y);
    pkFloatExtraData->SetValue(2, m_kAdjustedEyePos.z);

    NiIntegerExtraData* pkIntegerData = NiDynamicCast(NiIntegerExtraData, 
        pkMesh->GetExtraData(MORPHMODE_SHADER_CONSTANT));
    EE_ASSERT(pkIntegerData);
    pkIntegerData->SetValue(m_uiMorphMode);

    // Low detail texture information.
    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData, 
        pkMesh->GetExtraData(LOWDETAIL_TEXTURE_SIZES_SHADER_CONSTANT));
    EE_ASSERT(pkFloatExtraData);
    pkFloatExtraData->SetValue(0, m_kLowDetailTextureSize.x);
    pkFloatExtraData->SetValue(1, m_kLowDetailTextureSize.y);

    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData, 
        pkMesh->GetExtraData(LOWDETAIL_TEXTURE_OFFSET_SHADER_CONSTANT));
    EE_ASSERT(pkFloatExtraData);
    pkFloatExtraData->SetValue(0, m_kLowDetailTextureOffset.x);
    pkFloatExtraData->SetValue(1, m_kLowDetailTextureOffset.y);

    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData, 
        pkMesh->GetExtraData(LOWDETAIL_TEXTURE_SCALE_SHADER_CONSTANT));
    EE_ASSERT(pkFloatExtraData);
    pkFloatExtraData->SetValue(0, m_kLowDetailTextureScale.x);
    pkFloatExtraData->SetValue(1, m_kLowDetailTextureScale.y);

    // Layer information.
    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData,
        pkMesh->GetExtraData(BLENDMAP_SCALE_SHADER_CONSTANT));
    EE_ASSERT(pkFloatExtraData);
    pkFloatExtraData->SetValue(0, m_kBlendMapScale.x);
    pkFloatExtraData->SetValue(1, m_kBlendMapScale.y);

    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData,
        pkMesh->GetExtraData(BLENDMAP_OFFSET_SHADER_CONSTANT));
    EE_ASSERT(pkFloatExtraData);
    pkFloatExtraData->SetValue(0, m_kBlendMapOffset.x);
    pkFloatExtraData->SetValue(1, m_kBlendMapOffset.y);

    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData,
        pkMesh->GetExtraData(LAYER_SCALE_SHADER_CONSTANT));
    EE_ASSERT(pkFloatExtraData);
    pkFloatExtraData->SetValue(0, m_kLayerScale.X());
    pkFloatExtraData->SetValue(1, m_kLayerScale.Y());
    pkFloatExtraData->SetValue(2, m_kLayerScale.Z());
    pkFloatExtraData->SetValue(3, m_kLayerScale.W());

    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData,
        pkMesh->GetExtraData(DISTRIBUTION_RAMP_SHADER_CONSTANT));
    pkFloatExtraData->SetValue(0, m_kDistributionRamp.X());
    pkFloatExtraData->SetValue(1, m_kDistributionRamp.Y());
    pkFloatExtraData->SetValue(2, m_kDistributionRamp.Z());
    pkFloatExtraData->SetValue(3, m_kDistributionRamp.W());

    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData,
        pkMesh->GetExtraData(PARALLAX_MAP_STRENGTH_SHADER_CONSTANT));
    pkFloatExtraData->SetValue(0, m_kParallaxStrength.X());
    pkFloatExtraData->SetValue(1, m_kParallaxStrength.Y());
    pkFloatExtraData->SetValue(2, m_kParallaxStrength.Z());
    pkFloatExtraData->SetValue(3, m_kParallaxStrength.W());

    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData,
        pkMesh->GetExtraData(SPECULAR_POWER_SHADER_CONSTANT));
    pkFloatExtraData->SetValue(0, m_kSpecularPower.X());
    pkFloatExtraData->SetValue(1, m_kSpecularPower.Y());
    pkFloatExtraData->SetValue(2, m_kSpecularPower.Z());
    pkFloatExtraData->SetValue(3, m_kSpecularPower.W());

    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData,
        pkMesh->GetExtraData(SPECULAR_INTENSITY_SHADER_CONSTANT));
    pkFloatExtraData->SetValue(0, m_kSpecularIntensity.X());
    pkFloatExtraData->SetValue(1, m_kSpecularIntensity.Y());
    pkFloatExtraData->SetValue(2, m_kSpecularIntensity.Z());
    pkFloatExtraData->SetValue(3, m_kSpecularIntensity.W());

    pkFloatExtraData = NiDynamicCast(NiFloatsExtraData,
        pkMesh->GetExtraData(LAYER_DETAIL_TEXTURE_SCALE_SHADER_CONSTANT));
    pkFloatExtraData->SetValue(0, m_kDetailTextureScale.X());
    pkFloatExtraData->SetValue(1, m_kDetailTextureScale.Y());
    pkFloatExtraData->SetValue(2, m_kDetailTextureScale.Z());
    pkFloatExtraData->SetValue(3, m_kDetailTextureScale.W());
}
