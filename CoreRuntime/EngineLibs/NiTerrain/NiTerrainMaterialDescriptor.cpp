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

#include "NiTerrainMaterialDescriptor.h"
#include "NiTerrainMaterial.h"
#include "NiTerrainCellShaderData.h"

//--------------------------------------------------------------------------------------------------
NiTerrainMaterialDescriptor::NiTerrainMaterialDescriptor() :
    NiMaterialDescriptor(NiTerrainMaterial::MATERIAL_DESCRIPTOR_INDEX_COUNT)
{
    // Configure default parameters for the material for use on any mesh
    SetUSE_LOWDETAILDIFFUSE(0);
    SetUSE_LOWDETAILNORMAL(0);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterialDescriptor::SetLayerCaps(NiUInt32 uiLayerIndex, bool bEnableDiffuseMap,
    bool bEnableNormalMap, bool bEnableParallaxMap, bool bEnableDetailMap, 
    bool bEnableDistributionMap, bool bEnableSpecularMap)
{
    switch (uiLayerIndex)
    {
    case 0:
        SetLAYER0_BASEMAP_ENABLED(bEnableDiffuseMap);
        SetLAYER0_NORMALMAP_ENABLED(bEnableNormalMap);
        SetLAYER0_PARALLAXMAP_ENABLED(bEnableParallaxMap);
        SetLAYER0_DETAILMAP_ENABLED(bEnableDetailMap);
        SetLAYER0_DISTMAP_ENABLED(bEnableDistributionMap);
        SetLAYER0_SPECULARMAP_ENABLED(bEnableSpecularMap);
        break;

    case 1:
        SetLAYER1_BASEMAP_ENABLED(bEnableDiffuseMap);
        SetLAYER1_NORMALMAP_ENABLED(bEnableNormalMap);
        SetLAYER1_PARALLAXMAP_ENABLED(bEnableParallaxMap);
        SetLAYER1_DETAILMAP_ENABLED(bEnableDetailMap);
        SetLAYER1_DISTMAP_ENABLED(bEnableDistributionMap);
        SetLAYER1_SPECULARMAP_ENABLED(bEnableSpecularMap);
        break;

    case 2:
        SetLAYER2_BASEMAP_ENABLED(bEnableDiffuseMap);
        SetLAYER2_NORMALMAP_ENABLED(bEnableNormalMap);
        SetLAYER2_PARALLAXMAP_ENABLED(bEnableParallaxMap);
        SetLAYER2_DETAILMAP_ENABLED(bEnableDetailMap);
        SetLAYER2_DISTMAP_ENABLED(bEnableDistributionMap);
        SetLAYER2_SPECULARMAP_ENABLED(bEnableSpecularMap);
        break;

    case 3:
        SetLAYER3_BASEMAP_ENABLED(bEnableDiffuseMap);
        SetLAYER3_NORMALMAP_ENABLED(bEnableNormalMap);
        SetLAYER3_PARALLAXMAP_ENABLED(bEnableParallaxMap);
        SetLAYER3_DETAILMAP_ENABLED(bEnableDetailMap);
        SetLAYER3_DISTMAP_ENABLED(bEnableDistributionMap);
        SetLAYER3_SPECULARMAP_ENABLED(bEnableSpecularMap);
        break;

    default:
        EE_FAIL("Unsupported layer index.");
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterialDescriptor::HasNormalMapping() const
{
    bool bResult = false;

    bResult |= GetLAYER0_NORMALMAP_ENABLED() == 1;
    bResult |= GetLAYER1_NORMALMAP_ENABLED() == 1;
    bResult |= GetLAYER2_NORMALMAP_ENABLED() == 1;
    bResult |= GetLAYER3_NORMALMAP_ENABLED() == 1;
    bResult |= GetUSE_LOWDETAILNORMAL() == 1;
    bResult |= GetDEBUG_MODE() == NiTerrainCellShaderData::DEBUG_SHOW_NORMALS;

    return bResult;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterialDescriptor::HasParallaxMapping() const
{
    bool bResult = false;

    bResult |= GetLAYER0_PARALLAXMAP_ENABLED() == 1;
    bResult |= GetLAYER1_PARALLAXMAP_ENABLED() == 1;
    bResult |= GetLAYER2_PARALLAXMAP_ENABLED() == 1;
    bResult |= GetLAYER3_PARALLAXMAP_ENABLED() == 1;

    return bResult;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterialDescriptor::HasSpecularMapping() const
{
    bool bResult = false;

    bResult |= GetLAYER0_SPECULARMAP_ENABLED() == 1;
    bResult |= GetLAYER1_SPECULARMAP_ENABLED() == 1;
    bResult |= GetLAYER2_SPECULARMAP_ENABLED() == 1;
    bResult |= GetLAYER3_SPECULARMAP_ENABLED() == 1;

    return bResult;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterialDescriptor::RequiresViewVector()
{
    bool bLayer0Req = (GetLAYER0_PARALLAXMAP_ENABLED() == 1) || 
        (GetLAYER0_SPECULARMAP_ENABLED() == 1);

    bool bLayer1Req = (GetLAYER1_PARALLAXMAP_ENABLED() == 1) || 
        (GetLAYER1_SPECULARMAP_ENABLED() == 1);

    bool bLayer2Req = (GetLAYER2_PARALLAXMAP_ENABLED() == 1) || 
        (GetLAYER2_SPECULARMAP_ENABLED() == 1);

    bool bLayer3Req = (GetLAYER3_PARALLAXMAP_ENABLED() == 1) || 
        (GetLAYER3_SPECULARMAP_ENABLED() == 1);
    
    return bLayer0Req || bLayer1Req || bLayer2Req || bLayer3Req;
}
