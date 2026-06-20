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
#include "NiTerrainPCH.h"

#include "NiFragmentLighting.h"
#include "NiStandardMaterialNodeLibrary.h"
#include "NiPSSMShadowClickGenerator.h"

#include <NiCommonSemantics.h>
#include <NiVertexColorProperty.h>
#include <NiSpecularProperty.h>
#include <NiSpotLight.h>
#include <NiDirectionalLight.h>

//---------------------------------------------------------------------------
NiImplementRTTI(NiFragmentLighting, NiFragment, NiTypeMask::NiFragmentLighting);
//---------------------------------------------------------------------------
NiFragmentLighting::NiFragmentLighting():
    NiFragment(VERTEX_VERSION,GEOMETRY_VERSION,PIXEL_VERSION),
    m_bForcePerPixelLighting(false),
    m_bSaturateShading(false),
    m_bSaturateTextures(false)
{
    // Append the required node libraries
    m_kLibraries.Add(
        NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary());
}
//---------------------------------------------------------------------------
void NiFragmentLighting::FetchDependencies()
{
    // Set the owner as per normal
    NiFragment::FetchDependencies();

    // Fetch any dependant fragments that this fragment needs
    NiFragment::Fetch(m_pkMaterial, m_pkOperations);
}

//--------------------------------------------------------------------------------------------------
NiFragmentLighting::Descriptor::Descriptor():
    uiNumPointLights(0),
    uiNumDirectionalLights(0),
    uiNumSpotLights(0),
    eAmbDiffEmissive(EnumSource::ADE_IGNORE),
    eLightingMode(EnumSource::LIGHTING_E_A_D),
    eApplyMode(EnumSource::APPLY_MODULATE),
    bApplyAmbient(false),
    bApplyEmissive(false),
    bSpecularOn(false),
    bPerVertexForLights(false),
    bPSSMSliceTransitionEnabled(false),
    usPSSMSliceCount(0),
    usPSSMWhichLight(0),
    uiShadowMapBitfield(0),
    usShadowTechnique(0),
    bVertexOnlyLights(false),
    bVertexColors(false)
{
}

//--------------------------------------------------------------------------------------------------
NiFragmentLighting::PixelDescriptor::PixelDescriptor():
    bInputWorldView(false),
    bInputWorldPos(false),
    bInputWorldNormal(false),
    uiNumPointLights(0),
    uiNumDirectionalLights(0),
    uiNumSpotLights(0),
    eAmbDiffEmissive(EnumSource::ADE_IGNORE),
    eLightingMode(EnumSource::LIGHTING_E_A_D),
    eApplyMode(EnumSource::APPLY_MODULATE),
    bApplyAmbient(false),
    bApplyEmissive(false),
    bSpecularOn(false),
    bPerVertexForLights(true),
    bPSSMSliceTransitionEnabled(false),
    usPSSMSliceCount(0),
    usPSSMWhichLight(0),
    uiShadowMapBitfield(0),
    usShadowTechnique(0)
{
}

//--------------------------------------------------------------------------------------------------
NiFragmentLighting::VertexDescriptor::VertexDescriptor():
    bOutputWorldView(false),
    bOutputWorldPos(false),
    bOutputWorldNormal(false),
    uiNumPointLights(0),
    uiNumDirectionalLights(0),
    uiNumSpotLights(0),
    eAmbDiffEmissive(EnumSource::ADE_IGNORE),
    eLightingMode(EnumSource::LIGHTING_E_A_D),
    eApplyMode(EnumSource::APPLY_MODULATE),
    bSpecularOn(false),
    bVertexOnlyLights(true),
    bVertexColors(false)
{
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::GetTextureNameFromTextureEnum(TextureMap eMap,
    NiFixedString& kString, NiUInt32& uiOccurance)
{
    switch (eMap)
    {
    case EnumSource::MAP_MAX:
    default:
        EE_FAIL("Could not find map!\n");
        return false;
    case EnumSource::MAP_DIRSHADOW00:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_DIR);
        uiOccurance = 0;
        return true;
    case EnumSource::MAP_DIRSHADOW01:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_DIR);
        uiOccurance = 1;
        return true;
    case EnumSource::MAP_DIRSHADOW02:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_DIR);
        uiOccurance = 2;
        return true;
    case EnumSource::MAP_DIRSHADOW03:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_DIR);
        uiOccurance = 3;
        return true;
    case EnumSource::MAP_DIRSHADOW04:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_DIR);
        uiOccurance = 4;
        return true;
    case EnumSource::MAP_DIRSHADOW05:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_DIR);
        uiOccurance = 5;
        return true;
    case EnumSource::MAP_DIRSHADOW06:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_DIR);
        uiOccurance = 6;
        return true;
    case EnumSource::MAP_DIRSHADOW07:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_DIR);
        uiOccurance = 7;
        return true;
    case EnumSource::MAP_POINTSHADOW00:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_POINT);
        uiOccurance = 0;
        return true;
    case EnumSource::MAP_POINTSHADOW01:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_POINT);
        uiOccurance = 1;
        return true;
    case EnumSource::MAP_POINTSHADOW02:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_POINT);
        uiOccurance = 2;
        return true;
    case EnumSource::MAP_POINTSHADOW03:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_POINT);
        uiOccurance = 3;
        return true;
    case EnumSource::MAP_POINTSHADOW04:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_POINT);
        uiOccurance = 4;
        return true;
    case EnumSource::MAP_POINTSHADOW05:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_POINT);
        uiOccurance = 5;
        return true;
    case EnumSource::MAP_POINTSHADOW06:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_POINT);
        uiOccurance = 6;
        return true;
    case EnumSource::MAP_POINTSHADOW07:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_POINT);
        uiOccurance = 7;
        return true;
    case EnumSource::MAP_SPOTSHADOW00:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_SPOT);
        uiOccurance = 0;
        return true;
    case EnumSource::MAP_SPOTSHADOW01:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_SPOT);
        uiOccurance = 1;
        return true;
    case EnumSource::MAP_SPOTSHADOW02:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_SPOT);
        uiOccurance = 2;
        return true;
    case EnumSource::MAP_SPOTSHADOW03:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_SPOT);
        uiOccurance = 3;
        return true;
    case EnumSource::MAP_SPOTSHADOW04:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_SPOT);
        uiOccurance = 4;
        return true;
    case EnumSource::MAP_SPOTSHADOW05:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_SPOT);
        uiOccurance = 5;
        return true;
    case EnumSource::MAP_SPOTSHADOW06:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_SPOT);
        uiOccurance = 6;
        return true;
    case EnumSource::MAP_SPOTSHADOW07:
        kString = NiShadowMap::GetMapNameFromType(
            NiStandardMaterial::LIGHT_SPOT);
        uiOccurance = 7;
        return true;

    case EnumSource::MAP_SHADOWMAP_NOISE_GREYSCALE:
        kString = NiNoiseTexture::GetMapNameFromType(EnumSource::NOISE_GREYSCALE);
        uiOccurance = 0;
        return true;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::GenerateDescriptor(const NiRenderObject* pkMesh,
    const NiPropertyState* pkPropState,
    const NiDynamicEffectState* pkEffectState,
    Descriptor& kDesc, bool bNormals, bool bParallaxMapping,
    bool bNormalMapping)
{
    if (!pkPropState)
    {
        EE_FAIL("Could not find property state! Try calling"
            " UpdateProperties.\n");
        return false;
    }

    // Handle vertex colors
    bool bVertexColors = false;
    SetVertexColorDescriptor(kDesc, pkMesh, bVertexColors);
    bool bSpecularEnabled = false;

    if (pkPropState)
    {
        SetVertexColorPropertyDescriptor(kDesc, pkMesh, pkPropState,
            bVertexColors);
        SetSpecularPropertyDescriptor(kDesc, pkMesh, pkPropState,
            bSpecularEnabled);
        SetMaterialPropertyDescriptor(kDesc, pkMesh, pkPropState);
    }

    if (pkEffectState)
    {
        SetLightsDescriptor(kDesc, pkMesh, pkEffectState);
    }

    bool bDynamicLighting = kDesc.uiNumPointLights != 0 ||
        kDesc.uiNumSpotLights != 0 ||
        kDesc.uiNumDirectionalLights != 0;

    NiUInt32 uiShadowMapsForLight = kDesc.uiShadowMapBitfield;

    // Only force per pixel lighting if a dynamic light exist.
    bool bUsePerPixelLighting = bDynamicLighting && m_bForcePerPixelLighting;

    if (uiShadowMapsForLight || bUsePerPixelLighting ||
        (bDynamicLighting && (bParallaxMapping ||
        bNormalMapping)))
    {
        kDesc.bPerVertexForLights = false;
    }
    else
    {
        // Even if no lights actually exist, VS will handle all lighting
        kDesc.bPerVertexForLights = true;
    }

    // If there are no normals, disable effects that require normals
    if (!bNormals)
    {
        kDesc.uiNumPointLights = 0;
        kDesc.uiNumDirectionalLights = 0;
        kDesc.uiNumSpotLights = 0;
    }

    return true;
}
//---------------------------------------------------------------------------
bool NiFragmentLighting::SetupPackingRequirements(Descriptor& kDescriptor, 
    NiShaderDeclaration* pkShaderDecl, unsigned int& uiEntryCount)
{
    if (kDescriptor.bVertexColors)
    {
        pkShaderDecl->SetEntry(uiEntryCount++, 
            NiShaderDeclaration::SHADERPARAM_NI_COLOR,
            NiShaderDeclaration::SPTYPE_UBYTECOLOR);
    }

    return true;
}
//---------------------------------------------------------------------------
bool NiFragmentLighting::GenerateShaderDescArray(
        Descriptor& kDescriptor,
        PixelDescriptor& kPixelDesc, VertexDescriptor& kVertexDesc,
        NiUInt32 uiPassIndex, NiUInt32& uiRequiredPassCount)
{
    // Only support a single pass of lighting
    if (uiPassIndex != 0)
        return false;

    AmbDiffEmissiveEnum eAmbDiffEmissive = kDescriptor.eAmbDiffEmissive;
    kPixelDesc.eAmbDiffEmissive = eAmbDiffEmissive;

    LightingModeEnum eLightingMode = kDescriptor.eLightingMode;
    kPixelDesc.eLightingMode = eLightingMode;

    bool bPerVertexForLights = kDescriptor.bPerVertexForLights;
    kPixelDesc.bPerVertexForLights = bPerVertexForLights;

    ApplyMode eApplyMode = kDescriptor.eApplyMode;
    kPixelDesc.eApplyMode = eApplyMode;

    if (NiShadowManager::GetShadowManager() &&
        NiShadowManager::GetActive())
    {
        NiUInt32 uiShadowMapsForLight = kDescriptor.uiShadowMapBitfield;
        kPixelDesc.uiShadowMapBitfield = uiShadowMapsForLight;

        NiUInt32 uiShadowTechniqueSlot = kDescriptor.usShadowTechnique;
        NiShadowTechnique* pkShadowTechnique =
            NiShadowManager::GetActiveShadowTechnique(
            (NiUInt16)uiShadowTechniqueSlot);
        kPixelDesc.usShadowTechnique = pkShadowTechnique->GetTechniqueID();

        // Number of shadow map atlas splits
        bool bPSSMSliceTransitions = kDescriptor.bPSSMSliceTransitionEnabled;
        NiUInt16 usPSSMSliceCount = kDescriptor.usPSSMSliceCount;
        NiUInt16 usPSSMWhichLight = kDescriptor.usPSSMWhichLight;
        kPixelDesc.bPSSMSliceTransitionEnabled = bPSSMSliceTransitions;
        kPixelDesc.usPSSMSliceCount = usPSSMSliceCount;
        kPixelDesc.usPSSMWhichLight = usPSSMWhichLight;
    }
    else
    {
        kPixelDesc.usShadowTechnique = 0;
    }

    NiUInt32 uiDirLightCount = kDescriptor.uiNumDirectionalLights;
    NiUInt32 uiSpotLightCount = kDescriptor.uiNumSpotLights;
    NiUInt32 uiPointLightCount = kDescriptor.uiNumPointLights;
    NiUInt32 uiShadowMapForLight = kDescriptor.uiShadowMapBitfield;
    bool bSpecular = kDescriptor.bSpecularOn;

    NiUInt32 uiNumLights = uiDirLightCount + uiSpotLightCount +
        uiPointLightCount;

    // If the apply mode is REPLACE, then no lighting takes place
    if (eApplyMode == EnumSource::APPLY_REPLACE)
    {
        uiNumLights = 0;
        uiDirLightCount = 0;
        uiSpotLightCount = 0;
        uiPointLightCount = 0;
        bSpecular = false;
        bPerVertexForLights = true;
        kPixelDesc.bPerVertexForLights = bPerVertexForLights;
    }

    // Wait to set specular until after the apply mode has been taken into
    // consideration.
    kPixelDesc.bSpecularOn = bSpecular;

    // If per-pixel lighting
    if (!bPerVertexForLights)
    {
        kPixelDesc.uiNumPointLights = uiPointLightCount;
        kPixelDesc.uiNumSpotLights = uiSpotLightCount;
        kPixelDesc.uiNumDirectionalLights = uiDirLightCount;
        kPixelDesc.uiShadowMapBitfield = uiShadowMapForLight;
        kPixelDesc.bApplyAmbient = true;
        kPixelDesc.bApplyEmissive = true;

        if (bSpecular)
        {
            kVertexDesc.bOutputWorldView = true;
            kPixelDesc.bInputWorldView = true;
        }

        if (uiNumLights != 0)
        {
            kVertexDesc.bOutputWorldPos = true;
            kVertexDesc.bOutputWorldNormal = true;

            kPixelDesc.bInputWorldNormal = true;
            kPixelDesc.bInputWorldPos = true;

            kVertexDesc.bVertexOnlyLights = false;
        }
        else
        {
            kVertexDesc.bVertexOnlyLights = true;
            kPixelDesc.bPerVertexForLights = true;
        }
    }
    else
    {
        kVertexDesc.uiNumPointLights = uiPointLightCount;
        kVertexDesc.uiNumSpotLights = uiSpotLightCount;
        kVertexDesc.uiNumDirectionalLights = uiDirLightCount;
        kVertexDesc.bSpecularOn = bSpecular;
        kPixelDesc.bApplyAmbient = false;
        kPixelDesc.bApplyEmissive = false;
        kVertexDesc.bVertexOnlyLights = true;
    }

    EE_ASSERT(kVertexDesc.bVertexOnlyLights ==
        kPixelDesc.bPerVertexForLights);

    kVertexDesc.eAmbDiffEmissive = eAmbDiffEmissive;
    kVertexDesc.eLightingMode = eLightingMode;
    kVertexDesc.eApplyMode = eApplyMode;

    uiRequiredPassCount = 1;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::SetVertexColorDescriptor(
    Descriptor& kDesc,
    const NiRenderObject* pkMesh, bool& bVertexColors)
{
    bVertexColors = pkMesh->ContainsData(NiCommonSemantics::COLOR());
    kDesc.bVertexColors = bVertexColors;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::SetVertexColorPropertyDescriptor(
    Descriptor& kDesc,
    const NiRenderObject*, const NiPropertyState* pkPropState,
    bool bVertexColors)
{
    NiVertexColorProperty* pkVCProp = pkPropState->GetVertexColor();
    if (pkVCProp && bVertexColors)
    {
        if (pkVCProp->GetSourceMode() ==
            NiVertexColorProperty::SOURCE_AMB_DIFF)
        {
            kDesc.eAmbDiffEmissive = (NiStandardMaterial::ADE_AMB_DIFF);
        }
        else if (pkVCProp->GetSourceMode() ==
            NiVertexColorProperty::SOURCE_EMISSIVE)
        {
            kDesc.eAmbDiffEmissive = (NiStandardMaterial::ADE_EMISSIVE);
        }
        else if (pkVCProp->GetSourceMode() ==
            NiVertexColorProperty::SOURCE_IGNORE)
        {
            kDesc.eAmbDiffEmissive = (NiStandardMaterial::ADE_IGNORE);
        }
        else
        {
            EE_FAIL("Should never get here!");
        }

    }
    else // Always src_ignore if no vertex colors
    {
        kDesc.eAmbDiffEmissive = (NiStandardMaterial::ADE_IGNORE);
    }

    if (pkVCProp)
    {
        kDesc.eLightingMode = LightingModeEnum(pkVCProp->GetLightingMode());
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::SetSpecularPropertyDescriptor(
    Descriptor& kDesc,
    const NiRenderObject*, const NiPropertyState* pkPropState,
    bool& bSpecularEnabled)
{
    NiSpecularProperty* pkSpecProp = pkPropState->GetSpecular();

    bSpecularEnabled = false;
    if (pkSpecProp)
    {
        bSpecularEnabled = pkSpecProp->GetSpecular();
        kDesc.bSpecularOn = bSpecularEnabled;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::SetMaterialPropertyDescriptor(
    Descriptor&,
    const NiRenderObject*, const NiPropertyState*)
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::SetLightsDescriptor(
    Descriptor& kDesc,
    const NiRenderObject* pkMesh,
    const NiDynamicEffectState* pkEffectState)
{
    if (kDesc.eApplyMode == NiStandardMaterial::APPLY_MODULATE)
    {
        NiUInt32 uiLightCount = 0;

        // Add lights in the order Point, Directional, Spot. This is
        // required because all of NiStandardMaterial assumes lights have
        // been added in this order.

        // Add Point Lights
        NiDynEffectStateIter kLightIter = pkEffectState->GetLightHeadPos();
        while (kLightIter != NULL)
        {
            NiLight* pkLight = pkEffectState->GetNextLight(kLightIter);
            if (pkLight &&
                (pkLight->GetEffectType() ==
                NiDynamicEffect::POINT_LIGHT ||
                pkLight->GetEffectType() ==
                NiDynamicEffect::SHADOWPOINT_LIGHT))
            {
                AddLight(kDesc, pkLight, uiLightCount, pkMesh);
                uiLightCount++;
            }
        }

        // Add Directional Lights
        kLightIter = pkEffectState->GetLightHeadPos();
        while (kLightIter != NULL)
        {
            NiLight* pkLight = pkEffectState->GetNextLight(kLightIter);
            if (pkLight &&
                (pkLight->GetEffectType() == NiDynamicEffect::DIR_LIGHT ||
                pkLight->GetEffectType() ==
                NiDynamicEffect::SHADOWDIR_LIGHT))
            {
                AddLight(kDesc, pkLight, uiLightCount, pkMesh);
                uiLightCount++;
            }
        }

        // Add Spot Lights
        kLightIter = pkEffectState->GetLightHeadPos();
        while (kLightIter != NULL)
        {
            NiLight* pkLight = pkEffectState->GetNextLight(kLightIter);
            if (pkLight &&
                (pkLight->GetEffectType() == NiDynamicEffect::SPOT_LIGHT ||
                pkLight->GetEffectType() ==
                NiDynamicEffect::SHADOWSPOT_LIGHT))
            {
                AddLight(kDesc, pkLight, uiLightCount, pkMesh);
                uiLightCount++;
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::AddLight(Descriptor& kDesc,
    NiLight* pkLight, NiUInt32 uiWhichLight,
    const NiRenderObject* pkGeometry)
{
    if (uiWhichLight < STANDARD_PIPE_MAX_LIGHTS)
    {
        bool bFound = false;
        if (NiIsKindOf(NiSpotLight, pkLight))
        {
            kDesc.uiNumSpotLights = kDesc.uiNumSpotLights + 1;
            bFound = true;
        }
        else if (NiIsKindOf(NiPointLight, pkLight))
        {
            kDesc.uiNumPointLights = kDesc.uiNumPointLights + 1;
            bFound = true;
        }
        else if (NiIsKindOf(NiDirectionalLight, pkLight))
        {
            kDesc.uiNumDirectionalLights = kDesc.uiNumDirectionalLights + 1;
            bFound = true;
        }

        NiShadowGenerator* pkShadowGen = pkLight->GetShadowGenerator();
        if (bFound && pkShadowGen && NiShadowManager::GetShadowManager() &&
            NiShadowManager::GetActive())
        {
            if (!pkShadowGen->GetActive())
                return true;

            if (pkShadowGen->IsUnaffectedReceiverNode(pkGeometry))
                return true;

            bool bFoundShadowMap = false;
            const NiUInt32 uiShadowMapCount =
                pkShadowGen->GetShadowMapCount();
            for (NiUInt32 uiShadowMap = 0; uiShadowMap < uiShadowMapCount;
                uiShadowMap++)
            {
                if (pkShadowGen->GetOwnedShadowMap(uiShadowMap))
                {
                    bFoundShadowMap = true;
                    break;
                }
            }
            if (!bFoundShadowMap)
            {
                return true;
            }

            NiShadowTechnique* pkTechnique = pkShadowGen->GetShadowTechnique();

            if (pkTechnique)
            {
                NiUInt16 usActiveSlot =
                    pkTechnique->GetActiveTechniqueSlot();

                // If this assert is hit it means the light is assigned a
                // NiShadowTechnique that is not one of the active
                // NiShadowTechniques.
                EE_ASSERT (usActiveSlot <
                    NiShadowManager::MAX_ACTIVE_SHADOWTECHNIQUES);

                SetLightInfo(kDesc, uiWhichLight, true);

                // Only use the assigned shadow technique from the light if the
                // NiShadowTechnique has a higher priority than the one already
                // applied. Note: NiShadowTechnique priority is defined by the
                // slot the NiShadowTechnique is assigned to.
                if (usActiveSlot > kDesc.usShadowTechnique)
                    kDesc.usShadowTechnique = (usActiveSlot);

                // Get PSSM Info
                NiShadowClickGenerator* pkActive =
                    NiShadowManager::GetActiveShadowClickGenerator();
                NiPSSMShadowClickGenerator* pkPSSM = NiDynamicCast(
                    NiPSSMShadowClickGenerator, pkActive);

                if (pkPSSM && pkPSSM->LightSupportsPSSM(pkShadowGen, pkLight))
                {
                    NiPSSMConfiguration* pkConfig = pkPSSM->GetConfiguration(
                        pkShadowGen);

                    NiUInt32 uiEncodedSlices = NiPSSMShadowClickGenerator::
                        EncodeDescriptorSliceCount(pkConfig->GetNumSlices());

                    kDesc.usPSSMSliceCount = (NiUInt16)uiEncodedSlices;
                    kDesc.usPSSMWhichLight = (NiUInt16)uiWhichLight;

                    if (uiEncodedSlices != 0 &&
                        pkConfig->GetSliceTransitionEnabled())
                    {
                        kDesc.bPSSMSliceTransitionEnabled = true;
                    }
                    else
                    {
                        kDesc.bPSSMSliceTransitionEnabled = false;
                    }
                }
            }
        }

        return bFound;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::GetLightType(Descriptor& kDesc,
    NiUInt32 uiWhichLight, NiStandardMaterial::LightType& eLightType)
{
    NiUInt32 uiPointLights = kDesc.uiNumPointLights;
    NiUInt32 uiDirLights = kDesc.uiNumDirectionalLights;
    NiUInt32 uiSpotLights = kDesc.uiNumSpotLights;

    if (uiWhichLight > uiPointLights + uiDirLights + uiSpotLights)
    {
        return false;
    }
    else if (uiWhichLight > uiPointLights + uiDirLights)
    {
        eLightType = NiStandardMaterial::LIGHT_SPOT;
        return true;
    }
    else if (uiWhichLight > uiPointLights)
    {
        eLightType = NiStandardMaterial::LIGHT_DIR;
        return true;
    }
    else
    {
        eLightType = NiStandardMaterial::LIGHT_POINT;
        return true;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::GetLightInfo(Descriptor& kDesc,
    NiUInt32 uiWhichLight, bool& bShadowed)
{
    NiUInt32 uiPointLights = kDesc.uiNumPointLights;
    NiUInt32 uiDirLights = kDesc.uiNumDirectionalLights;
    NiUInt32 uiSpotLights = kDesc.uiNumSpotLights;

    if (uiWhichLight > uiPointLights + uiDirLights + uiSpotLights)
    {
       return false;
    }

    NiUInt32 uiShadowBits = kDesc.uiShadowMapBitfield;
    bShadowed = NiTGetBit< NiUInt32 >(uiShadowBits, 1 << uiWhichLight);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiFragmentLighting::SetLightInfo(Descriptor& kDesc,
    NiUInt32 uiWhichLight, bool bShadowed)
{
    NiUInt32 uiShadowBits = kDesc.uiShadowMapBitfield;
    NiTSetBit< NiUInt32 >(uiShadowBits, bShadowed,
        1 << uiWhichLight);

    kDesc.uiShadowMapBitfield = (uiShadowBits);
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::HandlePixelMaterialInitialValues(Context& kContext,
    PixelDescriptor& kPixelDesc,
    NiMaterialResource*& pkMatDiffuse,
    NiMaterialResource*& pkMatSpecular,
    NiMaterialResource*& pkSpecularPower,
    NiMaterialResource*&,
    NiMaterialResource*& pkMatAmbient,
    NiMaterialResource*& pkMatEmissive,
    NiMaterialResource*&,
    NiMaterialResource*&,
    NiMaterialResource*&,
    NiMaterialResource*&,
    NiMaterialResource*& pkOpacityAccum,
    NiMaterialResource*& pkLightDiffuseAccum,
    NiMaterialResource*& pkLightSpecularAccum,
    NiMaterialResource*& pkLightAmbientAccum)
{
    NiUInt32 uiPointLightCount = kPixelDesc.uiNumPointLights;
    NiUInt32 uiDirLightCount = kPixelDesc.uiNumDirectionalLights;
    NiUInt32 uiSpotLightCount = kPixelDesc.uiNumSpotLights;
    NiUInt32 uiPixelLightCount =
        uiPointLightCount + uiDirLightCount + uiSpotLightCount;
    bool bSpecular = kPixelDesc.bSpecularOn;

    AmbDiffEmissiveEnum eAmbDiffEmissive = kPixelDesc.eAmbDiffEmissive;
    LightingModeEnum eLightingMode = kPixelDesc.eLightingMode;
    bool bPerVertexLighting = kPixelDesc.bPerVertexForLights;

    if (kPixelDesc.eApplyMode != EnumSource::APPLY_REPLACE)
    {
        if (bPerVertexLighting)
        {
            // Vertex lights only or mix of pixel and vertex lights
            pkLightDiffuseAccum = kContext.m_spInputs->AddOutputResource(
                "float4", "TexCoord", "", "DiffuseAccum");

            if (!m_pkOperations->SplitColorAndOpacity(kContext, 
                pkLightDiffuseAccum, pkLightDiffuseAccum, pkOpacityAccum))
            {
                return false;
            }

            if (uiPixelLightCount != 0)

            {
                pkLightAmbientAccum = kContext.m_spInputs->AddOutputResource(
                    "float4", "TexCoord", "", "AmbientAccum");
            }

            if (bSpecular)
            {
                pkLightSpecularAccum = kContext.m_spInputs->AddOutputResource(
                    "float3", "TexCoord", "", "SpecularAccum");
            }
        }
        else
        {
            bool bApplyAmbient = kPixelDesc.bApplyAmbient;
            bool bApplyEmissive = kPixelDesc.bApplyEmissive;

            // Pixel lights only
            if (bApplyAmbient)
            {
                pkLightAmbientAccum = AddOutputPredefined(
                    kContext.m_spUniforms,
                    NiShaderConstantMap::SCM_DEF_AMBIENTLIGHT);
            }

            if (!HandleInitialSpecAmbDiffEmissiveColor(kContext, bSpecular,
                eAmbDiffEmissive, eLightingMode, pkMatDiffuse, pkMatSpecular,
                pkSpecularPower, pkMatAmbient, pkMatEmissive, pkOpacityAccum))
            {
                return false;
            }

            if (!bApplyEmissive)
            {
                pkMatEmissive = NULL;
            }
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::HandleVertexLightingAndMaterials(Context& kContext,
    VertexDescriptor& kVertexDesc,
    NiMaterialResource* pkWorldPos,
    NiMaterialResource* pkWorldNormal,
    NiMaterialResource* pkWorldView)
{
    NiUInt32 uiPointLights = kVertexDesc.uiNumPointLights;
    NiUInt32 uiDirLights = kVertexDesc.uiNumDirectionalLights;
    NiUInt32 uiSpotLights = kVertexDesc.uiNumSpotLights;
    bool bSpecularOn = kVertexDesc.bSpecularOn;

    NiUInt32 uiNumPerVertexLights = uiPointLights + uiDirLights +
        uiSpotLights;

    bool bVertexOnlyLights = kVertexDesc.bVertexOnlyLights;

    AmbDiffEmissiveEnum eAmbDiffEmissive = kVertexDesc.eAmbDiffEmissive;
    LightingModeEnum eLightingMode = kVertexDesc.eLightingMode;
    ApplyMode eApplyMode = kVertexDesc.eApplyMode;

    if (eApplyMode != EnumSource::APPLY_REPLACE)
    {
        if (uiNumPerVertexLights != 0 || bVertexOnlyLights)
        {
            NiMaterialResource* pkMatDiffuse = NULL;
            NiMaterialResource* pkMatSpecular = NULL;
            NiMaterialResource* pkSpecularPower = NULL;
            NiMaterialResource* pkMatAmbient = NULL;
            NiMaterialResource* pkMatEmissive = NULL;
            NiMaterialResource* pkOpacityAccum = NULL;

            if (bVertexOnlyLights)
            {
                if (!HandleInitialSpecAmbDiffEmissiveColor(kContext,
                    bSpecularOn, eAmbDiffEmissive, eLightingMode,
                    pkMatDiffuse, pkMatSpecular, pkSpecularPower,
                    pkMatAmbient, pkMatEmissive, pkOpacityAccum))
                {
                    return false;
                }
            }

            NiMaterialResource* pkDiffuseAccum = NULL;
            NiMaterialResource* pkSpecularAccum = NULL;
            NiMaterialResource* pkAmbientAccum = NULL;

            if (eLightingMode == EnumSource::LIGHTING_E_A_D)
            {
                pkAmbientAccum = AddOutputPredefined(
                    kContext.m_spUniforms,
                    NiShaderConstantMap::SCM_DEF_AMBIENTLIGHT);

                if (pkWorldNormal &&
                    !m_pkOperations->NormalizeVector(kContext,
                        pkWorldNormal))
                {
                    return false;
                }

                if (pkWorldView && 
                    !m_pkOperations->NormalizeVector(kContext, 
                        pkWorldView))
                {
                    return false;
                }

                // Vertex lighting does not dynamic support shadow maps,
                // so no shadow map atlas can exist.
                NiUInt32 uiShadowAtlasCells = 0;
                NiUInt32 uiPSSMWhichLight = 0;
                bool bPSSMSliceTransitions = false;

                if (!HandleLighting(kContext, uiShadowAtlasCells,
                    uiPSSMWhichLight, bPSSMSliceTransitions, bSpecularOn,
                    uiPointLights, uiDirLights, uiSpotLights, 0, 0,
                    pkWorldPos, pkWorldNormal, pkWorldView, pkSpecularPower,
                    pkAmbientAccum, pkDiffuseAccum, pkSpecularAccum))
                {
                    return false;
                }
            }

            if (bVertexOnlyLights)
            {
                // Only vertex lights are used
                NiMaterialResource* pkDiffuseCoeff = NULL;
                NiMaterialResource* pkSpecularCoeff = NULL;

                if (!HandleShadingCoefficients(kContext,
                    m_bSaturateShading,
                    pkMatEmissive,
                    pkMatDiffuse, pkMatAmbient, pkMatSpecular,
                    pkSpecularAccum, pkDiffuseAccum, pkAmbientAccum,
                    pkDiffuseCoeff, pkSpecularCoeff))
                {
                    return false;
                }

                EE_ASSERT(pkDiffuseCoeff && pkSpecularCoeff);

                NiMaterialNode* pkDiffuseCombineNode =
                    GetAttachableNodeFromLibrary("CompositeFinalRGBAColor");
                kContext.m_spConfigurator->AddNode(pkDiffuseCombineNode);

                kContext.m_spConfigurator->AddBinding(pkDiffuseCoeff,
                    "FinalColor", pkDiffuseCombineNode);
                if (pkOpacityAccum)
                {
                    kContext.m_spConfigurator->AddBinding(pkOpacityAccum,
                        "FinalOpacity", pkDiffuseCombineNode);
                }

                NiMaterialResource* pkDiffuseColorAlpha =
                    pkDiffuseCombineNode->GetOutputResourceByVariableName(
                    "OutputColor");

                NiMaterialResource* pkVertOut =
                    kContext.m_spOutputs->AddInputResource(
                    pkDiffuseColorAlpha->GetType(), "TexCoord",
                    pkDiffuseColorAlpha->GetLabel(), "DiffuseAccum");

                kContext.m_spConfigurator->AddBinding(pkDiffuseColorAlpha,
                    pkVertOut);

                if (bSpecularOn)
                {
                    NiMaterialResource* pkVertOutRes =
                        kContext.m_spOutputs->AddInputResource(
                        pkSpecularCoeff->GetType(), "TexCoord",
                        pkSpecularCoeff->GetLabel(), "SpecularAccum");

                    kContext.m_spConfigurator->AddBinding(pkSpecularCoeff,
                        pkVertOutRes);
                }
            }
            else
            {
                // Both pixel and vertex lights are used
                // Gouraud calculations to be done in PS
                EE_ASSERT(pkDiffuseAccum && pkAmbientAccum);
                EE_ASSERT(!bSpecularOn || pkSpecularAccum);

                if (pkDiffuseAccum)
                {
                    NiMaterialResource* pkVertOut =
                        kContext.m_spOutputs->AddInputResource(
                        pkDiffuseAccum->GetType(), "TexCoord",
                        pkDiffuseAccum->GetLabel(), "DiffuseAccum");

                    kContext.m_spConfigurator->AddBinding(pkDiffuseAccum,
                        pkVertOut);
                }

                if (pkAmbientAccum)
                {
                    NiMaterialResource* pkVertOut =
                        kContext.m_spOutputs->AddInputResource(
                        pkDiffuseAccum->GetType(), "TexCoord",
                        pkDiffuseAccum->GetLabel(), "AmbientAccum");

                    kContext.m_spConfigurator->AddBinding(pkAmbientAccum,
                        pkVertOut);
                }

                if (pkSpecularAccum)
                {
                    NiMaterialResource* pkVertOut =
                        kContext.m_spOutputs->AddInputResource(
                        pkSpecularAccum->GetType(), "TexCoord",
                        pkSpecularAccum->GetLabel(), "SpecularAccum");

                    kContext.m_spConfigurator->AddBinding(pkSpecularAccum,
                        pkVertOut);
                }
            }
        }
        else
        {
            // Only pixel lights are used
            bool bVertexColors = kVertexDesc.bVertexColors;

            if (bVertexColors && (eAmbDiffEmissive ==
                EnumSource::ADE_EMISSIVE ||
                (eAmbDiffEmissive == EnumSource::ADE_AMB_DIFF &&
                eLightingMode == EnumSource::LIGHTING_E_A_D)))
            {
                NiMaterialResource* pkVertIn =
                    kContext.m_spInputs->AddOutputResource(
                    "float4", "Color", "", "VertexColors");

                NiMaterialResource* pkVertOut =
                    kContext.m_spOutputs->AddInputResource(
                    pkVertIn->GetType(), pkVertIn->GetSemantic(),
                    pkVertIn->GetLabel(), "VertexColors");

                kContext.m_spConfigurator->AddBinding(pkVertIn, pkVertOut);
            }
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::HandleColorAccumulation(Context& kContext,
    PixelDescriptor& kPixelDesc,
    NiMaterialResource* pkMatEmissive,
    NiMaterialResource* pkMatDiffuse,
    NiMaterialResource* pkMatAmbient,
    NiMaterialResource* pkMatSpecular,
    NiMaterialResource* pkLightSpecularAccum,
    NiMaterialResource* pkLightDiffuseAccum,
    NiMaterialResource* pkLightAmbientAccum,
    NiMaterialResource* pkGlossiness,
    NiMaterialResource* pkTexDiffuseAccum,
    NiMaterialResource*,
    NiMaterialResource*& pkSpecularAccum,
    NiMaterialResource*& pkDiffuseAccum)
{
    NiMaterialResource* pkDiffuseCoeff = NULL;
    NiMaterialResource* pkSpecularCoeff = NULL;
    LightingModeEnum eLightingMode = kPixelDesc.eLightingMode;

    // APPLY_REPLACE only uses the texture's color, all lighting, vertex,
    // and material colors are ignored.
    if (kPixelDesc.eApplyMode == EnumSource::APPLY_REPLACE)
    {
        pkDiffuseCoeff = NULL;
    }
    else
    {
        // If vertex lighting was done and there are no pixel lights, then
        // the Gouraud calculations have already been done
        NiUInt32 uiPerPixelLights = kPixelDesc.uiNumPointLights +
            kPixelDesc.uiNumDirectionalLights +
            kPixelDesc.uiNumSpotLights;

        if (kPixelDesc.bPerVertexForLights &&
            uiPerPixelLights == 0)
        {
            pkDiffuseCoeff = pkLightDiffuseAccum;
            pkSpecularCoeff = pkLightSpecularAccum;
        }
        else
        {
            if (eLightingMode == EnumSource::LIGHTING_E)
            {
                pkLightDiffuseAccum = NULL;
                pkLightAmbientAccum = NULL;
            }

            if (!HandleShadingCoefficients(kContext,
                m_bSaturateShading,
                pkMatEmissive,
                pkMatDiffuse, pkMatAmbient, pkMatSpecular,
                pkLightSpecularAccum, pkLightDiffuseAccum,
                pkLightAmbientAccum, pkDiffuseCoeff, pkSpecularCoeff))
            {
                return false;
            }
        }
    }

    if (pkDiffuseCoeff && pkTexDiffuseAccum)
    {
        if (!m_pkOperations->MultiplyVector(kContext, 
            pkDiffuseCoeff, pkTexDiffuseAccum, pkDiffuseAccum))
        {
            return false;
        }

        if (m_bSaturateTextures && 
            !m_pkOperations->SaturateVector(kContext, 
            pkDiffuseAccum, pkDiffuseAccum))
        {
            return false;
        }
    }
    else if (pkTexDiffuseAccum)
    {
        pkDiffuseAccum = pkTexDiffuseAccum;
    }
    else if (pkDiffuseCoeff)
    {
        pkDiffuseAccum = pkDiffuseCoeff;
    }
    else
    {
        return false;
    }

    pkSpecularAccum = pkSpecularCoeff;

    if (pkGlossiness && pkSpecularAccum)
    {
        if (!m_pkOperations->ScaleVector(kContext, 
            pkSpecularAccum, pkGlossiness, pkSpecularAccum))
        {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::HandleInitialSpecAmbDiffEmissiveColor(
    Context& kContext, bool bSpecularOn, AmbDiffEmissiveEnum eADF,
    LightingModeEnum eLightingMode, NiMaterialResource*& pkDiffuseColorRes,
    NiMaterialResource*& pkSpecularColorRes,
    NiMaterialResource*& pkSpecularPowerRes,
    NiMaterialResource*& pkAmbientColorRes,
    NiMaterialResource*& pkEmissiveColorRes,
    NiMaterialResource*& pkOpacityRes)
{
    pkSpecularColorRes = NULL;
    pkDiffuseColorRes = NULL;
    pkAmbientColorRes = NULL;
    pkEmissiveColorRes = NULL;
    pkSpecularPowerRes = NULL;
    pkOpacityRes = NULL;

    if (bSpecularOn)
    {
        pkSpecularColorRes = AddOutputPredefined(
            kContext.m_spUniforms,
            NiShaderConstantMap::SCM_DEF_MATERIAL_SPECULAR);

        pkSpecularPowerRes = AddOutputPredefined(
            kContext.m_spUniforms,
            NiShaderConstantMap::SCM_DEF_MATERIAL_POWER);
    }

    bool bVertexColors = (eADF == EnumSource::ADE_EMISSIVE ||
        (eADF == EnumSource::ADE_AMB_DIFF &&
        eLightingMode == EnumSource::LIGHTING_E_A_D));
    NiMaterialResource* pkVertexColorRes = NULL;
    if (bVertexColors)
    {
        pkVertexColorRes = kContext.m_spInputs->AddOutputResource("float4",
            "Color", "", "VertexColors");
    }

    switch (eADF)
    {
        case EnumSource::ADE_AMB_DIFF:
            {
                pkEmissiveColorRes = AddOutputPredefined(
                    kContext.m_spUniforms,
                    NiShaderConstantMap::SCM_DEF_MATERIAL_EMISSIVE);
                if (eLightingMode == EnumSource::LIGHTING_E_A_D)
                {
                    pkDiffuseColorRes = pkVertexColorRes;
                    pkAmbientColorRes = pkVertexColorRes;
                }
            }
            break;
        case EnumSource::ADE_EMISSIVE:
            {
                pkEmissiveColorRes = pkVertexColorRes;
                if (eLightingMode == EnumSource::LIGHTING_E_A_D)
                {
                    pkDiffuseColorRes = AddOutputPredefined(
                        kContext.m_spUniforms,
                        NiShaderConstantMap::SCM_DEF_MATERIAL_DIFFUSE);
                    pkAmbientColorRes = AddOutputPredefined(
                        kContext.m_spUniforms,
                        NiShaderConstantMap::SCM_DEF_MATERIAL_AMBIENT);
                }
            }
            break;
        case EnumSource::ADE_IGNORE:
            {
                pkEmissiveColorRes = AddOutputPredefined(
                    kContext.m_spUniforms,
                    NiShaderConstantMap::SCM_DEF_MATERIAL_EMISSIVE);

                if (eLightingMode == EnumSource::LIGHTING_E_A_D)
                {
                    pkDiffuseColorRes = AddOutputPredefined(
                        kContext.m_spUniforms,
                        NiShaderConstantMap::SCM_DEF_MATERIAL_DIFFUSE);
                    pkAmbientColorRes = AddOutputPredefined(
                        kContext.m_spUniforms,
                        NiShaderConstantMap::SCM_DEF_MATERIAL_AMBIENT);
                }
            }
            break;
        default:
            EE_FAIL("Unknown AmbientDiffuseEmissive Enum value");
            break;
    }

    if (pkDiffuseColorRes == NULL)
    {
        // If no diffuse color is set (because the lighting mode is
        // LIGHTING_E), then alpha should come from the emissive component.
        if (!m_pkOperations->SplitColorAndOpacity(kContext, pkEmissiveColorRes, 
            pkEmissiveColorRes, pkOpacityRes))
        {
            return false;
        }
    }
    else
    {
        if (!m_pkOperations->SplitColorAndOpacity(kContext, pkDiffuseColorRes, 
            pkDiffuseColorRes, pkOpacityRes))
        {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::HandleLighting(Context& kContext,
    PixelDescriptor& kPixelDesc,
    NiMaterialResource* pkWorldPos, NiMaterialResource* pkWorldNorm,
    NiMaterialResource* pkViewVector,
    NiMaterialResource* pkSpecularPower,
    NiMaterialResource*& pkAmbientAccum,
    NiMaterialResource*& pkDiffuseAccum,
    NiMaterialResource*& pkSpecularAccum, ExtraLightingData kExtraData)
{
    // Extract the number of shadowmap atlas splits
    NiUInt32 uiPSSMWhichLight = kPixelDesc.usPSSMWhichLight;
    NiUInt32 uiShadowAtlasCells = NiPSSMShadowClickGenerator::
        DecodeDescriptorMaxSliceCount(kPixelDesc.usPSSMSliceCount);

    return HandleLighting(kContext,
        uiShadowAtlasCells, uiPSSMWhichLight,
        kPixelDesc.bPSSMSliceTransitionEnabled,
        kPixelDesc.bSpecularOn,
        kPixelDesc.uiNumPointLights,
        kPixelDesc.uiNumDirectionalLights,
        kPixelDesc.uiNumSpotLights,
        kPixelDesc.uiShadowMapBitfield,
        kPixelDesc.usShadowTechnique,
        pkWorldPos, pkWorldNorm,
        pkViewVector, pkSpecularPower, pkAmbientAccum, pkDiffuseAccum,
        pkSpecularAccum, kExtraData);
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::HandleLighting(
    Context& kContext, NiUInt32 uiShadowAtlasCells, NiUInt32 uiPSSMWhichLight,
    bool bSliceTransitions,
    bool bSpecular, NiUInt32 uiNumPoint, NiUInt32 uiNumDirectional,
    NiUInt32 uiNumSpot, NiUInt32 uiShadowBitfield,
    NiUInt32 uiShadowTechnique, NiMaterialResource* pkWorldPos,
    NiMaterialResource* pkWorldNorm, NiMaterialResource* pkViewVector,
    NiMaterialResource* pkSpecularPower, NiMaterialResource*& pkAmbientAccum,
    NiMaterialResource*& pkDiffuseAccum, NiMaterialResource*& pkSpecularAccum, 
    ExtraLightingData kExtraData)
{
    NiUInt32 uiLight = 0;
    NiMaterialResource* pkShadow = NULL;

    NiUInt32 uiLightTypeCount = 0;
    NiUInt32 uiShadowLightTypeCount = 0;
    for (NiUInt32 ui = 0; ui < uiNumPoint; ui++)
    {
        pkShadow = NULL;
        NiUInt32 uiMask = NiTGetBitMask<NiUInt32>(uiLight, 1);
        bool bShadow = NiTGetBit<NiUInt32>(uiShadowBitfield, uiMask);
        if (bShadow && NiShadowManager::GetShadowManager())
        {
            // PSSM Is only enabled for directional lights.
            const NiUInt32 uiPSSMNumSlices = 1;
            const bool bPSSMTransitions = false;

            if (!HandleShadow(kContext, uiPSSMNumSlices, bPSSMTransitions,
                uiLight, EnumSource::LIGHT_POINT,
                uiShadowLightTypeCount, pkWorldPos, pkWorldNorm,
                uiShadowTechnique, pkShadow))
            {
                return false;
            }

            if (!HandleLight(kContext, uiLight,
                EnumSource::LIGHT_POINT,
                uiShadowLightTypeCount, pkWorldPos, pkWorldNorm, pkViewVector,
                pkSpecularPower, pkAmbientAccum, pkDiffuseAccum,
                pkSpecularAccum, bSpecular, pkShadow, kExtraData))
            {
                return false;
            }

            uiShadowLightTypeCount++;
        }
        else
        {
            if (!HandleLight(kContext, uiLight, EnumSource::LIGHT_POINT,
                uiLightTypeCount,
                pkWorldPos, pkWorldNorm, pkViewVector, pkSpecularPower,
                pkAmbientAccum, pkDiffuseAccum, pkSpecularAccum, bSpecular,
                pkShadow, kExtraData))
            {
                return false;
            }
            uiLightTypeCount++;
        }


        uiLight++;
    }

    uiLightTypeCount = uiShadowLightTypeCount = 0;
    for (NiUInt32 ui = 0; ui < uiNumDirectional; ui++)
    {
        pkShadow = NULL;
        NiUInt32 uiMask = NiTGetBitMask<NiUInt32>(uiLight, 1);
        bool bShadow = NiTGetBit<NiUInt32>(uiShadowBitfield, uiMask);
        if (bShadow && NiShadowManager::GetShadowManager())
        {
            NiUInt32 uiPSSMNumSlices = 1;
            if (uiPSSMWhichLight == uiShadowLightTypeCount)
                uiPSSMNumSlices = uiShadowAtlasCells;

            if (!HandleShadow(kContext, uiShadowAtlasCells,
                bSliceTransitions, uiLight, EnumSource::LIGHT_DIR,
                uiShadowLightTypeCount, pkWorldPos, pkWorldNorm,
                uiShadowTechnique, pkShadow))
            {
                return false;
            }

            if (!HandleLight(kContext, uiLight, EnumSource::LIGHT_DIR,
                uiShadowLightTypeCount, pkWorldPos, pkWorldNorm, pkViewVector,
                pkSpecularPower, pkAmbientAccum, pkDiffuseAccum,
                pkSpecularAccum, bSpecular, pkShadow, kExtraData))
            {
                return false;
            }
            uiShadowLightTypeCount++;
        }
        else
        {
            if (!HandleLight(kContext, uiLight, EnumSource::LIGHT_DIR,
                uiLightTypeCount, pkWorldPos, pkWorldNorm, pkViewVector,
                pkSpecularPower, pkAmbientAccum, pkDiffuseAccum,
                pkSpecularAccum, bSpecular, pkShadow, kExtraData))
            {
                return false;
            }
            uiLightTypeCount++;
        }
        uiLight++;
    }

    uiLightTypeCount = uiShadowLightTypeCount = 0;
    for (NiUInt32 ui = 0; ui < uiNumSpot; ui++)
    {
        pkShadow = NULL;
        NiUInt32 uiMask = NiTGetBitMask<NiUInt32>(uiLight, 1);
        bool bShadow = NiTGetBit<NiUInt32>(uiShadowBitfield, uiMask);
        if (bShadow && NiShadowManager::GetShadowManager())
        {
            // PSSM Is only enabled for directional lights.
            const NiUInt32 uiPSSMNumSlices = 1;
            const bool bPSSMTransitions = false;

            if (!HandleShadow(kContext, uiPSSMNumSlices, bPSSMTransitions,
                uiLight, EnumSource::LIGHT_SPOT,
                uiShadowLightTypeCount, pkWorldPos, pkWorldNorm,
                uiShadowTechnique, pkShadow))
            {
                return false;
            }

            if (!HandleLight(kContext, uiLight, EnumSource::LIGHT_SPOT,
                uiShadowLightTypeCount, pkWorldPos, pkWorldNorm, pkViewVector,
                pkSpecularPower, pkAmbientAccum, pkDiffuseAccum,
                pkSpecularAccum, bSpecular, pkShadow, kExtraData))
            {
                return false;
            }
            uiShadowLightTypeCount++;
        }
        else
        {
            if (!HandleLight(kContext, uiLight, EnumSource::LIGHT_SPOT,
                uiLightTypeCount,
                pkWorldPos, pkWorldNorm, pkViewVector, pkSpecularPower,
                pkAmbientAccum, pkDiffuseAccum, pkSpecularAccum, bSpecular,
                pkShadow, kExtraData))
            {
                return false;
            }
            uiLightTypeCount++;
        }
        uiLight++;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::HandleShadow(
    Context& kContext, NiUInt32 uiShadowAtlasCells, bool bSliceTransitions,
    NiUInt32, LightType eType, NiUInt32 uiLightByType,
    NiMaterialResource* pkWorldPos, NiMaterialResource*,
    NiUInt32 uiShadowTechnique, NiMaterialResource*& pkShadow)
{
    char acValue[256];

    NiShadowTechnique* pkShadowTechnique =
        NiShadowManager::GetKnownShadowTechnique(
        (NiUInt16)uiShadowTechnique);

    if (!pkShadowTechnique)
        return false;

    NiMaterialNode* pkShadowNode = GetAttachableNodeFromLibrary(
            pkShadowTechnique->GetReadFragmentName(eType));
    kContext.m_spConfigurator->AddNode(pkShadowNode);
    EE_ASSERT(pkShadowNode);

    // WorldPos
    kContext.m_spConfigurator->AddBinding(pkWorldPos,
        pkShadowNode->GetInputResourceByVariableName("WorldPos"));

    // LightType
    NiSprintf(acValue, sizeof(acValue), "(%d)", (NiUInt32) eType);
    kContext.m_spConfigurator->AddBinding(
        kContext.m_spStatics->AddOutputConstant("int", acValue),
        pkShadowNode->GetInputResourceByVariableName("LightType"));

    NiShaderAttributeDesc::ObjectType eObjType =
        NiShaderAttributeDesc::OT_EFFECT_GENERALLIGHT;
    const char* pcVariableModifier = NULL;
    switch (eType)
    {
    case EnumSource::LIGHT_DIR:
        eObjType = NiShaderAttributeDesc::OT_EFFECT_SHADOWDIRECTIONALLIGHT;
        pcVariableModifier = "ShadowDir";
        break;
    case EnumSource::LIGHT_POINT:
        eObjType = NiShaderAttributeDesc::OT_EFFECT_SHADOWPOINTLIGHT;
        pcVariableModifier = "ShadowPoint";
        break;
    case EnumSource::LIGHT_SPOT:
        eObjType = NiShaderAttributeDesc::OT_EFFECT_SHADOWSPOTLIGHT;
        pcVariableModifier = "ShadowSpot";
        break;
    default:
        EE_FAIL("Unknown enumeration");
        break;
    }

    // LightDirection
    if (eType == EnumSource::LIGHT_SPOT)
    {
        NiMaterialResource* pkLightDir = AddOutputObject(
            kContext.m_spUniforms,
            NiShaderConstantMap::SCM_OBJ_WORLDDIRECTION, eObjType,
            uiLightByType, pcVariableModifier);
        kContext.m_spConfigurator->AddBinding(pkLightDir,
            pkShadowNode->GetInputResourceByVariableName("LightDirection"));
    }

    NiMaterialResource* pkLightPos =
        pkShadowNode->GetInputResourceByVariableName("LightPos");
    if (pkLightPos)
    {
        // LightPos
        NiMaterialResource* pkLightPosObj = AddOutputObject(
            kContext.m_spUniforms,NiShaderConstantMap::SCM_OBJ_WORLDPOSITION,
            eObjType, uiLightByType, pcVariableModifier);
        kContext.m_spConfigurator->AddBinding(pkLightPosObj, pkLightPos);
    }

    // ShadowMapSize
    NiMaterialResource* pkInputResource =
        pkShadowNode->GetInputResourceByVariableName("ShadowMapSize");
    if (pkInputResource)
    {
        NiMaterialResource* pkShadowMapSize = AddOutputObject(
            kContext.m_spUniforms,
            NiShaderConstantMap::SCM_OBJ_SHADOWMAPTEXSIZE, eObjType,
            uiLightByType, pcVariableModifier);
        kContext.m_spConfigurator->AddBinding(pkShadowMapSize,
            pkInputResource);
    }

    // ShadowBias
    pkInputResource =
        pkShadowNode->GetInputResourceByVariableName("ShadowBias");
    if (pkInputResource)
    {
        NiMaterialResource* pkShadowBias = AddOutputObject(
            kContext.m_spUniforms,
            NiShaderConstantMap::SCM_OBJ_SHADOWBIAS, eObjType,
            uiLightByType, pcVariableModifier);
        kContext.m_spConfigurator->AddBinding(pkShadowBias,
            pkInputResource);
    }

    // ShadowVSMPower
    pkInputResource =
        pkShadowNode->GetInputResourceByVariableName("ShadowVSMPowerEpsilon");
    if (pkInputResource)
    {
        NiMaterialResource* pkShadowVSMPowerEp = AddOutputObject(
            kContext.m_spUniforms,
            NiShaderConstantMap::SCM_OBJ_SHADOW_VSM_POWER_EPSILON, eObjType,
            uiLightByType, pcVariableModifier);
        kContext.m_spConfigurator->AddBinding(pkShadowVSMPowerEp,
            pkInputResource);
    }

    // ShadowMap
    NiFixedString kSamplerName;
    TextureMap eMap = EnumSource::MAP_DIRSHADOW00;

    EE_ASSERT(uiLightByType < 8 && "Too many shadow lights");
    switch (eType)
    {
    case EnumSource::LIGHT_DIR:
        eMap = (NiFragmentLighting::TextureMap)
            (EnumSource::MAP_DIRSHADOW00 + uiLightByType);
        break;
    case EnumSource::LIGHT_SPOT:
        eMap = (NiFragmentLighting::TextureMap)
            (EnumSource::MAP_SPOTSHADOW00 + uiLightByType);
        break;
    case EnumSource::LIGHT_POINT:
        eMap = (NiFragmentLighting::TextureMap)
            (EnumSource::MAP_POINTSHADOW00 + uiLightByType);
        break;
    default:
        EE_FAIL("Invalid many light type.");
    }

    if (!GetTextureNameFromTextureEnum(eMap, kSamplerName, uiLightByType))
    {
        return false;
    }

    NiMaterialResource* pkSamplerRes;
    if (eType == EnumSource::LIGHT_POINT &&
        pkShadowTechnique->GetUseCubeMapForPointLight())
    {
        pkSamplerRes = InsertTextureSampler(kContext,
            kSamplerName, EnumSource::TEXTURE_SAMPLER_CUBE, uiLightByType,
            eObjType);
    }
    else if (eType == EnumSource::LIGHT_DIR && uiShadowAtlasCells > 1)
    {
        // The shadow map has been split using PSSM.
        pkSamplerRes = InsertTextureSampler(kContext,
            kSamplerName, EnumSource::TEXTURE_SAMPLER_2D, uiLightByType,
            eObjType);

        //
        // Camera Distance Fragment
        //

        NiMaterialNode* pkPSSMCamDistanceNode = NULL;
        if (bSliceTransitions)
        {
            NiUInt32 uiNoiseOccurance;
            bool bTextureNameExists = GetTextureNameFromTextureEnum(
                EnumSource::MAP_SHADOWMAP_NOISE_GREYSCALE, kSamplerName, 
                uiNoiseOccurance);
            EE_ASSERT(bTextureNameExists);

            if (bTextureNameExists)
            {
                pkPSSMCamDistanceNode = GetAttachableNodeFromLibrary(
                    "PSSMCamDistanceWithTransition");
                EE_ASSERT(pkPSSMCamDistanceNode);
                kContext.m_spConfigurator->AddNode(pkPSSMCamDistanceNode);

                // Outputs
                NiMaterialResource* pkTransitionMatrixOutput = AddOutputObject(
                    kContext.m_spUniforms,
                    NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_TRANSITIONMATRIX,
                    eObjType, uiLightByType, pcVariableModifier, 1);
                NiMaterialResource* pkNoiseSamplerOutput = InsertTextureSampler(
                    kContext,
                    NiNoiseTexture::GetMapNameFromType(
                        EnumSource::NOISE_GREYSCALE), 
                    EnumSource::TEXTURE_SAMPLER_2D, uiNoiseOccurance, 
                    NiShaderAttributeDesc::OT_EFFECT_PSSMSLICENOISEMASK);
                NiMaterialResource* pkTransitionSizeOutput = AddOutputObject(
                    kContext.m_spUniforms,
                    NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_TRANSITIONSIZE,
                    eObjType, uiLightByType, pcVariableModifier, 1);

                // Inputs
                NiMaterialResource* pkTransitionMatrixInput =
                    pkPSSMCamDistanceNode->GetInputResourceByVariableName(
                    "TransitionViewProj");
                NiMaterialResource* pkNoiseSamplerInput =
                    pkPSSMCamDistanceNode->GetInputResourceByVariableName(
                    "TransitionNoise");
                NiMaterialResource* pkTransitionSizeInput =
                    pkPSSMCamDistanceNode->GetInputResourceByVariableName(
                    "TransitionSize");

                // Bindings
                kContext.m_spConfigurator->AddBinding(pkTransitionMatrixOutput,
                    pkTransitionMatrixInput);
                kContext.m_spConfigurator->AddBinding(pkNoiseSamplerOutput,
                    pkNoiseSamplerInput);
                kContext.m_spConfigurator->AddBinding(pkTransitionSizeOutput,
                    pkTransitionSizeInput);
            }
        }

        if (!pkPSSMCamDistanceNode)
        {
            pkPSSMCamDistanceNode = GetAttachableNodeFromLibrary(
                "PSSMCamDistance");

            EE_ASSERT(pkPSSMCamDistanceNode);
            kContext.m_spConfigurator->AddNode(pkPSSMCamDistanceNode);
        }

        // Bindings shared by both the transition and regular camera distance
        // fragment variations
        NiMaterialResource* pkEyeDirectionOutput = AddOutputPredefined(
            kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_EYE_DIR);
        NiMaterialResource* pkEyePosOutput = AddOutputPredefined(
            kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_EYE_POS);

        kContext.m_spConfigurator->AddBinding(pkWorldPos,
            pkPSSMCamDistanceNode->GetInputResourceByVariableName(
            "WorldPos"));
        kContext.m_spConfigurator->AddBinding(pkEyeDirectionOutput,
            pkPSSMCamDistanceNode->GetInputResourceByVariableName(
            "EyeDirection"));
        kContext.m_spConfigurator->AddBinding(pkEyePosOutput,
            pkPSSMCamDistanceNode->GetInputResourceByVariableName(
            "EyePos"));

        //
        // ChooseSlice fragment(s)
        //

        // ceil(float(uiNumFragments) / 4.0f);
        NiUInt32 uiNumFragments = uiShadowAtlasCells >> 2;
        if (uiShadowAtlasCells % 4 != 0)
            uiNumFragments++;
        EE_ASSERT(uiNumFragments > 0);

        NiMaterialNode* pkChooseSliceFragment;
        NiMaterialNode* pkAddFragment;
        NiMaterialResource* pkPreviousSliceFragmentOutput = NULL;
        NiMaterialResource* pkCamDistanceOutput =
            pkPSSMCamDistanceNode->GetOutputResourceByVariableName(
            "CamDistance");

        NiString kSliceDistanceOutputString;
        bool bDistancesArray = (uiNumFragments > 1);
        for (NiUInt32 ui = 0; ui < uiNumFragments; ++ui)
        {
            // We use a different fragment when the slice distances is an array,
            // so that the [] operator works in the shader.
            if (bDistancesArray)
            {
                pkChooseSliceFragment = GetAttachableNodeFromLibrary(
                    "PSSMChooseSliceArray");
            }
            else
            {
                pkChooseSliceFragment = GetAttachableNodeFromLibrary(
                    "PSSMChooseSlice");
            }
            EE_ASSERT(pkChooseSliceFragment);
            kContext.m_spConfigurator->AddNode(pkChooseSliceFragment);

            // Outputs
            kSliceDistanceOutputString.Format("(%d)", ui);
            NiMaterialResource* pkSliceDistanceIndexOutput =
                kContext.m_spStatics->AddOutputConstant(
                "int", (const char*)kSliceDistanceOutputString);
            NiMaterialResource* pkSplitDistancesOutput = AddOutputObject(
                kContext.m_spUniforms,
                NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_SPLITDISTANCES,
                eObjType, uiLightByType, pcVariableModifier, uiNumFragments);

            // Inputs
            NiMaterialResource* pkSliceDistancesIndexInput =
                pkChooseSliceFragment->GetInputResourceByVariableName(
                "SplitDistancesIndex");
            NiMaterialResource* pkCamDistanceInput =
                pkChooseSliceFragment->GetInputResourceByVariableName(
                "CamDistance");
            NiMaterialResource* pkSliceDistancesInput =
                pkChooseSliceFragment->GetInputResourceByVariableName(
                "SplitDistances");
            pkSliceDistancesInput->SetCount(uiNumFragments);

            // Results
            NiMaterialResource* pkSliceFragmentResult =
                pkChooseSliceFragment->GetOutputResourceByVariableName(
                "SliceToUse");

            // Bindings
            kContext.m_spConfigurator->AddBinding(pkCamDistanceOutput,
                pkCamDistanceInput);
            kContext.m_spConfigurator->AddBinding(pkSplitDistancesOutput,
                pkSliceDistancesInput);
            if (bDistancesArray)
            {
                kContext.m_spConfigurator->AddBinding(pkSliceDistanceIndexOutput,
                    pkSliceDistancesIndexInput);
            }

            // First fragment is a special case, since it does rely on the
            // previous iterations result
            if (pkPreviousSliceFragmentOutput != NULL)
            {
                pkAddFragment = GetAttachableNodeFromLibrary("AddFloat");
                EE_ASSERT(pkAddFragment);
                kContext.m_spConfigurator->AddNode(pkAddFragment);

                // Outputs
                NiMaterialResource* pkV1Output = pkPreviousSliceFragmentOutput;
                NiMaterialResource* pkV2Output = pkSliceFragmentResult;

                // Inputs
                NiMaterialResource* pkV1Input =
                    pkAddFragment->GetInputResourceByVariableName("V1");
                NiMaterialResource* pkV2Input =
                    pkAddFragment->GetInputResourceByVariableName("V2");

                // Results
                pkPreviousSliceFragmentOutput =
                    pkAddFragment->GetOutputResourceByVariableName("Output");

                // Bindings
                kContext.m_spConfigurator->AddBinding(pkV1Output, pkV1Input);
                kContext.m_spConfigurator->AddBinding(pkV2Output, pkV2Input);
            }
            else
            {
                pkPreviousSliceFragmentOutput = pkSliceFragmentResult;
            }
        }

        //
        // Transform fragment
        //

        NiMaterialNode* pkPSSMTransformationNode = NULL;
        pkPSSMTransformationNode = GetAttachableNodeFromLibrary(
            "PSSMTransform");
        EE_ASSERT(pkPSSMTransformationNode);
        kContext.m_spConfigurator->AddNode(pkPSSMTransformationNode);

        // Outputs
        NiMaterialResource* pkPSSMMatricesOutput = AddOutputObject(
            kContext.m_spUniforms,
            NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_SPLITMATRICES, eObjType,
            uiLightByType, pcVariableModifier, uiShadowAtlasCells);
        NiMaterialResource* pkPSSMViewportsOutput = AddOutputObject(
            kContext.m_spUniforms,
            NiShaderConstantMap::SCM_OBJ_SHADOW_PSSM_ATLASVIEWPORTS, eObjType,
            uiLightByType, pcVariableModifier, uiShadowAtlasCells);

        // Inputs
        NiMaterialResource* pkTransformMatricesInput =
            pkPSSMTransformationNode->GetInputResourceByVariableName(
            "SplitMatrices");
        pkTransformMatricesInput->SetCount(uiShadowAtlasCells);
        NiMaterialResource* pkTransformViewportsInput =
            pkPSSMTransformationNode->GetInputResourceByVariableName(
            "ShadowMapViewports");
        pkTransformViewportsInput->SetCount(uiShadowAtlasCells);
        NiMaterialResource* pkTransformSliceToUseInput =
            pkPSSMTransformationNode->GetInputResourceByVariableName(
            "SliceToUse");
        NiMaterialResource* pkSliceCountInput =
            pkPSSMTransformationNode->GetInputResourceByVariableName(
            "SliceCount");

        // Results
        NiMaterialResource* pkTransformWorldToLightProjMatResult =
            pkPSSMTransformationNode->GetOutputResourceByVariableName(
            "WorldToLightProjMat");
        NiMaterialResource* pkTransformViewportResult =
            pkPSSMTransformationNode->GetOutputResourceByVariableName(
            "ShadowMapViewport");

        // Bindings
        kContext.m_spConfigurator->AddBinding(pkPSSMMatricesOutput,
            pkTransformMatricesInput);
        kContext.m_spConfigurator->AddBinding(pkPSSMViewportsOutput,
            pkTransformViewportsInput);
        kContext.m_spConfigurator->AddBinding(pkPreviousSliceFragmentOutput,
            pkTransformSliceToUseInput);

        NiString kSliceCountOutputString;
        kSliceCountOutputString.Format("(%d)", uiShadowAtlasCells);
        NiMaterialResource* pkSliceCountOutput =
            kContext.m_spStatics->AddOutputConstant(
            "int", (const char*)kSliceCountOutputString);
        kContext.m_spConfigurator->AddBinding(pkSliceCountOutput,
            pkSliceCountInput);

        //
        // Shadow fragment
        //

        // Inputs
        NiMaterialResource* pkShadowWorldToLightProjMatInput =
            pkShadowNode->GetInputResourceByVariableName(
            "WorldToLightProjMat");
        NiMaterialResource* pkShadowMapViewportInput =
            pkShadowNode->GetInputResourceByVariableName(
            "ShadowMapViewport");

        // Bindings
        kContext.m_spConfigurator->AddBinding(
            pkTransformWorldToLightProjMatResult,
            pkShadowWorldToLightProjMatInput);
        kContext.m_spConfigurator->AddBinding(pkTransformViewportResult,
            pkShadowMapViewportInput);
    }
    else
    {
        pkSamplerRes = InsertTextureSampler(kContext,
            kSamplerName, EnumSource::TEXTURE_SAMPLER_2D, uiLightByType,
            eObjType);

        NiMaterialResource* pkV2LProj = AddOutputObject(
            kContext.m_spUniforms,
            NiShaderConstantMap::SCM_OBJ_WORLDTOSHADOWMAPMATRIX, eObjType,
            uiLightByType, pcVariableModifier);

        kContext.m_spConfigurator->AddBinding(pkV2LProj,
            pkShadowNode->GetInputResourceByVariableName(
            "WorldToLightProjMat"));
    }

    kContext.m_spConfigurator->AddBinding(pkSamplerRes,
        pkShadowNode->GetInputResourceByVariableName("ShadowMap"));

    pkShadow = pkShadowNode->GetOutputResourceByVariableName(
        "ShadowOut");

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::HandleLight(Context& kContext, NiUInt32,
    LightType eType, NiUInt32 uiLightByType,
    NiMaterialResource* pkWorldPos, NiMaterialResource* pkWorldNorm,
    NiMaterialResource* pkViewVector, NiMaterialResource* pkSpecularPower,
    NiMaterialResource*& pkAmbientAccum, NiMaterialResource*& pkDiffuseAccum,
    NiMaterialResource*& pkSpecularAccum, bool bSpecular,
    NiMaterialResource* pkShadow, ExtraLightingData kExtraData)
{
    EE_UNUSED_ARG(kExtraData);

    NiMaterialNode* pkLightNode = GetAttachableNodeFromLibrary(
        "Light");
    kContext.m_spConfigurator->AddNode(pkLightNode);

    kContext.m_spConfigurator->AddBinding(pkWorldPos,
        pkLightNode->GetInputResourceByVariableName("WorldPos"));
    kContext.m_spConfigurator->AddBinding(pkWorldNorm,
        pkLightNode->GetInputResourceByVariableName("WorldNrm"));

    if (pkAmbientAccum)
    {
        kContext.m_spConfigurator->AddBinding(pkAmbientAccum,
            pkLightNode->GetInputResourceByVariableName("AmbientAccum"));
    }

    if (pkDiffuseAccum)
    {
        kContext.m_spConfigurator->AddBinding(pkDiffuseAccum,
            pkLightNode->GetInputResourceByVariableName("DiffuseAccum"));
    }

    if (pkSpecularAccum && bSpecular)
    {
        kContext.m_spConfigurator->AddBinding(pkSpecularAccum,
            pkLightNode->GetInputResourceByVariableName("SpecularAccum"));
    }

    kContext.m_spConfigurator->AddBinding(
        kContext.m_spStatics->AddOutputConstant(
        "bool", bSpecular ? "(true)" : "(false)"),
        pkLightNode->GetInputResourceByVariableName("SpecularEnable"));

    if (pkShadow)
    {
        kContext.m_spConfigurator->AddBinding(
            pkShadow, pkLightNode->GetInputResourceByVariableName("Shadow"));
    }

    char acValue[32];
    NiSprintf(acValue, 32, "(%d)", (NiUInt32) eType);
    kContext.m_spConfigurator->AddBinding(
        kContext.m_spStatics->AddOutputConstant("int", acValue),
        pkLightNode->GetInputResourceByVariableName("LightType"));
    NiShaderAttributeDesc::ObjectType eObjType =
        NiShaderAttributeDesc::OT_EFFECT_GENERALLIGHT;
    const char* pcVariableModifier = NULL;
    switch (eType)
    {
        case EnumSource::LIGHT_DIR:
            if (pkShadow)
            {
                eObjType =
                    NiShaderAttributeDesc::OT_EFFECT_SHADOWDIRECTIONALLIGHT;
                pcVariableModifier = "ShadowDir";
            }
            else
            {
                eObjType = NiShaderAttributeDesc::OT_EFFECT_DIRECTIONALLIGHT;
                pcVariableModifier = "Dir";
            }
            break;
        case EnumSource::LIGHT_POINT:
            if (pkShadow)
            {
                eObjType = NiShaderAttributeDesc::OT_EFFECT_SHADOWPOINTLIGHT;
                pcVariableModifier = "ShadowPoint";
            }
            else
            {
                eObjType = NiShaderAttributeDesc::OT_EFFECT_POINTLIGHT;
                pcVariableModifier = "Point";
            }
            break;
        case EnumSource::LIGHT_SPOT:
            if (pkShadow)
            {
                eObjType = NiShaderAttributeDesc::OT_EFFECT_SHADOWSPOTLIGHT;
                pcVariableModifier = "ShadowSpot";
            }
            else
            {
                eObjType = NiShaderAttributeDesc::OT_EFFECT_SPOTLIGHT;
                pcVariableModifier = "Spot";
            }

            break;
        default:
            EE_FAIL("Unknown enumeration");
            break;
    }

    NiMaterialResource* pkRes = AddOutputObject(
        kContext.m_spUniforms,
        NiShaderConstantMap::SCM_OBJ_AMBIENT, eObjType, uiLightByType,
        pcVariableModifier);

    kContext.m_spConfigurator->AddBinding(pkRes,
        pkLightNode->GetInputResourceByVariableName("LightAmbient"));

    pkRes = AddOutputObject(kContext.m_spUniforms,
        NiShaderConstantMap::SCM_OBJ_DIFFUSE, eObjType, uiLightByType,
        pcVariableModifier);

    kContext.m_spConfigurator->AddBinding(pkRes,
        pkLightNode->GetInputResourceByVariableName("LightDiffuse"));

    pkRes = AddOutputObject(kContext.m_spUniforms,
        NiShaderConstantMap::SCM_OBJ_SPECULAR, eObjType, uiLightByType,
        pcVariableModifier);
    kContext.m_spConfigurator->AddBinding(pkRes,
        pkLightNode->GetInputResourceByVariableName("LightSpecular"));

    pkRes = AddOutputObject(kContext.m_spUniforms,
        NiShaderConstantMap::SCM_OBJ_WORLDPOSITION, eObjType, uiLightByType,
        pcVariableModifier);
    kContext.m_spConfigurator->AddBinding(pkRes,
        pkLightNode->GetInputResourceByVariableName("LightPos"));

    if (eType != EnumSource::LIGHT_DIR) // LIGHT_SPOT or LIGHT_POINT
    {
        pkRes = AddOutputObject(kContext.m_spUniforms,
            NiShaderConstantMap::SCM_OBJ_ATTENUATION, eObjType, uiLightByType,
            pcVariableModifier);
        kContext.m_spConfigurator->AddBinding(pkRes,
            pkLightNode->GetInputResourceByVariableName("LightAttenuation"));
    }

    if (eType != EnumSource::LIGHT_POINT) // LIGHT_SPOT or LIGHT_DIR
    {
        pkRes = AddOutputObject(kContext.m_spUniforms,
            NiShaderConstantMap::SCM_OBJ_WORLDDIRECTION, eObjType,
            uiLightByType, pcVariableModifier);
        kContext.m_spConfigurator->AddBinding(pkRes,
            pkLightNode->GetInputResourceByVariableName("LightDirection"));
    }

    if (eType == EnumSource::LIGHT_SPOT)
    {
        pkRes = AddOutputObject(kContext.m_spUniforms,
            NiShaderConstantMap::SCM_OBJ_SPOTATTENUATION, eObjType,
            uiLightByType, pcVariableModifier);
        kContext.m_spConfigurator->AddBinding(pkRes,
            pkLightNode->GetInputResourceByVariableName(
            "LightSpotAttenuation"));
    }

    if (bSpecular)
    {
        if (pkViewVector)
        {
            kContext.m_spConfigurator->AddBinding(pkViewVector,
                pkLightNode->GetInputResourceByVariableName(
                "WorldViewVector"));
        }

        if (pkSpecularPower)
        {
            kContext.m_spConfigurator->AddBinding(pkSpecularPower,
                pkLightNode->GetInputResourceByVariableName(
                "SpecularPower"));
        }

    }

    pkAmbientAccum = pkLightNode->GetOutputResourceByVariableName(
        "AmbientAccumOut");

    pkDiffuseAccum = pkLightNode->GetOutputResourceByVariableName(
        "DiffuseAccumOut");

    if (bSpecular)
    {
        pkSpecularAccum = pkLightNode->GetOutputResourceByVariableName(
            "SpecularAccumOut");
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLighting::HandleShadingCoefficients(Context& kContext,
    bool bSaturateShading,
    NiMaterialResource* pkMatEmissive, NiMaterialResource* pkMatDiffuse,
    NiMaterialResource* pkMatAmbient,  NiMaterialResource* pkMatSpecular,
    NiMaterialResource* pkLightSpecularAccum,
    NiMaterialResource* pkLightDiffuseAccum,
    NiMaterialResource* pkLightAmbientAccum,
    NiMaterialResource*& pkDiffuseCoeff, NiMaterialResource*& pkSpecularCoeff)
{
    NiMaterialNode* pkShadeNode =
        GetAttachableNodeFromLibrary("ComputeShadingCoefficients");
    kContext.m_spConfigurator->AddNode(pkShadeNode);

    if (pkMatDiffuse == NULL)
    {
        pkMatDiffuse = kContext.m_spStatics->AddOutputConstant(
            "float3","(1.0, 1.0, 1.0)");
    }
    kContext.m_spConfigurator->AddBinding(pkMatDiffuse,
        pkShadeNode->GetInputResourceByVariableName(
        "MatDiffuse"));

    if (pkMatSpecular == NULL)
    {
        pkMatSpecular = kContext.m_spStatics->AddOutputConstant(
            "float3","(1.0, 1.0, 1.0)");
    }
    kContext.m_spConfigurator->AddBinding(pkMatSpecular,
        pkShadeNode->GetInputResourceByVariableName(
        "MatSpecular"));

    if (pkMatAmbient == NULL)
    {
        pkMatAmbient = kContext.m_spStatics->AddOutputConstant(
            "float3","(1.0, 1.0, 1.0)");
    }
    kContext.m_spConfigurator->AddBinding(pkMatAmbient,
        pkShadeNode->GetInputResourceByVariableName(
        "MatAmbient"));

    if (pkMatEmissive == NULL)
    {
        pkMatEmissive = kContext.m_spStatics->AddOutputConstant(
            "float3","(0.0, 0.0, 0.0)");
    }
    kContext.m_spConfigurator->AddBinding(pkMatEmissive,
        pkShadeNode->GetInputResourceByVariableName(
        "MatEmissive"));

    if (pkLightDiffuseAccum)
    {
         kContext.m_spConfigurator->AddBinding(pkLightDiffuseAccum,
            pkShadeNode->GetInputResourceByVariableName(
            "LightDiffuseAccum"));
    }

    if (pkLightSpecularAccum)
    {
        kContext.m_spConfigurator->AddBinding(pkLightSpecularAccum,
            pkShadeNode->GetInputResourceByVariableName(
            "LightSpecularAccum"));
    }

    if (pkLightAmbientAccum)
    {
        kContext.m_spConfigurator->AddBinding(pkLightAmbientAccum,
            pkShadeNode->GetInputResourceByVariableName(
            "LightAmbientAccum"));
    }

    kContext.m_spConfigurator->AddBinding(
        kContext.m_spStatics->AddOutputConstant("bool", bSaturateShading ?
            "(true)" : "(false)"),  "Saturate", pkShadeNode);

    pkDiffuseCoeff =
        pkShadeNode->GetOutputResourceByVariableName("Diffuse");
    pkSpecularCoeff =
        pkShadeNode->GetOutputResourceByVariableName("Specular");

    return true;
}

//--------------------------------------------------------------------------------------------------
