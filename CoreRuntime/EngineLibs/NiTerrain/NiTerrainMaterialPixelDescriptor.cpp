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

#include <NiTerrainPCH.h>

#include "NiTerrainMaterialPixelDescriptor.h"
#include "NiTerrainMaterialDescriptor.h"
#include "NiTerrainMaterial.h"

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterialPixelDescriptor::
    SetLayerCapabilitiesFromMaterialDescriptor(
        NiTerrainMaterialDescriptor* pkMaterialDesc, NiUInt32 uiLayer)
{
    NiUInt32 uiValueInMaterial;

    switch (uiLayer)
    {
    case 0:
        uiValueInMaterial = pkMaterialDesc->GetLAYER0_BASEMAP_ENABLED();
        SetLAYER0_BASEMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER0_NORMALMAP_ENABLED();
        SetLAYER0_NORMALMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER0_PARALLAXMAP_ENABLED();
        SetLAYER0_PARALLAXMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER0_DISTMAP_ENABLED();
        SetLAYER0_DISTMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER0_DETAILMAP_ENABLED();
        SetLAYER0_DETAILMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER0_SPECULARMAP_ENABLED();
        SetLAYER0_SPECULARMAP_ENABLED(uiValueInMaterial);

        break;

    case 1:
        uiValueInMaterial = pkMaterialDesc->GetLAYER1_BASEMAP_ENABLED();
        SetLAYER1_BASEMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER1_NORMALMAP_ENABLED();
        SetLAYER1_NORMALMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER1_PARALLAXMAP_ENABLED();
        SetLAYER1_PARALLAXMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER1_DISTMAP_ENABLED();
        SetLAYER1_DISTMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER1_DETAILMAP_ENABLED();
        SetLAYER1_DETAILMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER1_SPECULARMAP_ENABLED();
        SetLAYER1_SPECULARMAP_ENABLED(uiValueInMaterial);
        break;

    case 2:
        uiValueInMaterial = pkMaterialDesc->GetLAYER2_BASEMAP_ENABLED();
        SetLAYER2_BASEMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER2_NORMALMAP_ENABLED();
        SetLAYER2_NORMALMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER2_PARALLAXMAP_ENABLED();
        SetLAYER2_PARALLAXMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER2_DISTMAP_ENABLED();
        SetLAYER2_DISTMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER2_DETAILMAP_ENABLED();
        SetLAYER2_DETAILMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER2_SPECULARMAP_ENABLED();
        SetLAYER2_SPECULARMAP_ENABLED(uiValueInMaterial);
        break;

    case 3:
        uiValueInMaterial = pkMaterialDesc->GetLAYER3_BASEMAP_ENABLED();
        SetLAYER3_BASEMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER3_NORMALMAP_ENABLED();
        SetLAYER3_NORMALMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER3_PARALLAXMAP_ENABLED();
        SetLAYER3_PARALLAXMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER3_DISTMAP_ENABLED();
        SetLAYER3_DISTMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER3_DETAILMAP_ENABLED();
        SetLAYER3_DETAILMAP_ENABLED(uiValueInMaterial);

        uiValueInMaterial = pkMaterialDesc->GetLAYER3_SPECULARMAP_ENABLED();
        SetLAYER3_SPECULARMAP_ENABLED(uiValueInMaterial);
        break;

    default:
        EE_FAIL("Layer index exceeded max number of supported layers.");
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterialPixelDescriptor::SupportsBaseMap(NiUInt32 uiLayerIndex)
{
    switch (uiLayerIndex)
    {
    case 0:
        return GetLAYER0_BASEMAP_ENABLED() == 1;

    case 1:
        return GetLAYER1_BASEMAP_ENABLED() == 1;

    case 2:
        return GetLAYER2_BASEMAP_ENABLED() == 1;

    case 3:
        return GetLAYER3_BASEMAP_ENABLED() == 1;

    default:
        EE_FAIL("Unsupported layer index.");
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterialPixelDescriptor::SetSupportsBaseMap(NiUInt32 uiLayerIndex, bool bEnabled)
{
    switch (uiLayerIndex)
    {
    case 0:
        SetLAYER0_BASEMAP_ENABLED(bEnabled);
        break;

    case 1:
        SetLAYER1_BASEMAP_ENABLED(bEnabled);
        break;

    case 2:
        SetLAYER2_BASEMAP_ENABLED(bEnabled);
        break;

    case 3:
        SetLAYER3_BASEMAP_ENABLED(bEnabled);
        break;

    default:
        EE_FAIL("Unsupported layer index.");
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterialPixelDescriptor::SupportsDetailMap(NiUInt32 uiLayerIndex)
{
    switch (uiLayerIndex)
    {
    case 0:
        return GetLAYER0_DETAILMAP_ENABLED() == 1;

    case 1:
        return GetLAYER1_DETAILMAP_ENABLED() == 1;

    case 2:
        return GetLAYER2_DETAILMAP_ENABLED() == 1;

    case 3:
        return GetLAYER3_DETAILMAP_ENABLED() == 1;

    default:
        EE_FAIL("Unsupported layer index.");
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterialPixelDescriptor::SupportsNormalMap(NiUInt32 uiLayerIndex)
{
    switch (uiLayerIndex)
    {
    case 0:
        return GetLAYER0_NORMALMAP_ENABLED() == 1;

    case 1:
        return GetLAYER1_NORMALMAP_ENABLED() == 1;

    case 2:
        return GetLAYER2_NORMALMAP_ENABLED() == 1;

    case 3:
        return GetLAYER3_NORMALMAP_ENABLED() == 1;

    default:
        EE_FAIL("Unsupported layer index.");
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterialPixelDescriptor::SetNormalMapEnabled(NiUInt32 uiLayerIndex, bool bEnabled)
{
    switch (uiLayerIndex)
    {
    case 0:
        SetLAYER0_NORMALMAP_ENABLED(bEnabled);
        break;

    case 1:
        SetLAYER1_NORMALMAP_ENABLED(bEnabled);
        break;

    case 2:
        SetLAYER2_NORMALMAP_ENABLED(bEnabled);
        break;

    case 3:
        SetLAYER3_NORMALMAP_ENABLED(bEnabled);
        break;

    default:
        EE_FAIL("Unsupported layer index.");
        break;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterialPixelDescriptor::SupportsDistributionMap(NiUInt32 uiLayerIndex)
{
    switch (uiLayerIndex)
    {
    case 0:
        return GetLAYER0_DISTMAP_ENABLED() == 1;

    case 1:
        return GetLAYER1_DISTMAP_ENABLED() == 1;

    case 2:
        return GetLAYER2_DISTMAP_ENABLED() == 1;

    case 3:
        return GetLAYER3_DISTMAP_ENABLED() == 1;

    default:
        EE_FAIL("Unsupported layer index.");
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterialPixelDescriptor::SetSupportsDistributionMap(NiUInt32 uiLayerIndex, 
    bool bEnabled)
{
    switch (uiLayerIndex)
    {
    case 0:
        SetLAYER0_DISTMAP_ENABLED(bEnabled);
        break;

    case 1:
        SetLAYER1_DISTMAP_ENABLED(bEnabled);
        break;

    case 2:
        SetLAYER2_DISTMAP_ENABLED(bEnabled);
        break;

    case 3:
        SetLAYER3_DISTMAP_ENABLED(bEnabled);
        break;

    default:
        EE_FAIL("Unsupported layer index.");
        break;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterialPixelDescriptor::SupportsParallaxMap(NiUInt32 uiLayerIndex)
{
    switch (uiLayerIndex)
    {
    case 0:
        return GetLAYER0_PARALLAXMAP_ENABLED() == 1;

    case 1:
        return GetLAYER1_PARALLAXMAP_ENABLED() == 1;

    case 2:
        return GetLAYER2_PARALLAXMAP_ENABLED() == 1;

    case 3:
        return GetLAYER3_PARALLAXMAP_ENABLED() == 1;

    default:
        EE_FAIL("Unsupported layer index.");
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterialPixelDescriptor::SetParallaxMapEnabled(NiUInt32 uiLayerIndex, bool bEnabled)
{
    switch (uiLayerIndex)
    {
    case 0:
        SetLAYER0_PARALLAXMAP_ENABLED(bEnabled);
        break;

    case 1:
        SetLAYER1_PARALLAXMAP_ENABLED(bEnabled);
        break;

    case 2:
        SetLAYER2_PARALLAXMAP_ENABLED(bEnabled);
        break;

    case 3:
        SetLAYER3_PARALLAXMAP_ENABLED(bEnabled);
        break;

    default:
        EE_FAIL("Unsupported layer index.");
        break;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterialPixelDescriptor::SupportsSpecularMap(NiUInt32 uiLayerIndex)
{
    switch (uiLayerIndex)
    {
    case 0:
        return GetLAYER0_SPECULARMAP_ENABLED() == 1;

    case 1:
        return GetLAYER1_SPECULARMAP_ENABLED() == 1;

    case 2:
        return GetLAYER2_SPECULARMAP_ENABLED() == 1;

    case 3:
        return GetLAYER3_SPECULARMAP_ENABLED() == 1;

    default:
        EE_FAIL("Unsupported layer index.");
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterialPixelDescriptor::SetSupportsSpecularMap(NiUInt32 uiLayerIndex, bool bEnabled)
{
    switch (uiLayerIndex)
    {
    case 0:
        SetLAYER0_SPECULARMAP_ENABLED(bEnabled);
        break;

    case 1:
        SetLAYER1_SPECULARMAP_ENABLED(bEnabled);
        break;

    case 2:
        SetLAYER2_SPECULARMAP_ENABLED(bEnabled);
        break;

    case 3:
        SetLAYER3_SPECULARMAP_ENABLED(bEnabled);
        break;

    default:
        EE_FAIL("Unsupported layer index.");
        break;
    }
}

//--------------------------------------------------------------------------------------------------
NiString NiTerrainMaterialPixelDescriptor::ToString()
{
    NiString kResult;

    ToStringDEBUG_MODE(kResult, false);
    ToStringRENDER_MODE(kResult, false);
    ToStringNUM_LAYERS(kResult, false);
    ToStringUSE_LOWDETAILDIFFUSE(kResult, false);
    ToStringUSE_LOWDETAILNORMAL(kResult, false);

    kResult += "\n";
    ToStringSPOTLIGHTCOUNT(kResult, true);

    kResult += "\n";
    ToStringSHADOWMAPFORLIGHT(kResult, true);

    kResult += "\n";
    ToStringLAYER3_BASEMAP_ENABLED(kResult, true);

    return kResult;
}
