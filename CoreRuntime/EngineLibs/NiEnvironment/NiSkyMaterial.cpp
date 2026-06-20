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

#include "NiEnvironmentPCH.h"

#include "NiSkyMaterial.h"
#include "NiSkyMaterialDescriptor.h"
#include "NiSkyMaterialPixelDescriptor.h"
#include "NiSkyMaterialVertexDescriptor.h"
#include "NiAtmosphere.h"

#include <NiSkyMaterialNodeLibrary.h>
#include <NiAtmosphereMaterialNodeLibrary.h>

#include <NiCommonSemantics.h>
#include <NiFogProperty.h>
#include <NiRenderer.h>
#include <NiRenderObject.h>
#include <NiRenderObjectMaterialOption.h>
#include <NiShaderFactory.h>
#include <NiIntegerExtraData.h>
#include <NiFloatExtraData.h>
#include <NiColorExtraData.h>

//---------------------------------------------------------------------------
const char* NiSkyMaterial::ED_ATMOSPHERICCALCMODE = 
    "g_AtmosphericCalculationMode";
const char* NiSkyMaterial::ED_STAGECONFIGURATION =
    "Configuration";

const char* NiSkyMaterial::SC_ATMOSPHERICSCATTERINGCONST = 
    "g_AtmosphericScatteringConsts";
const char* NiSkyMaterial::SC_RGBINVWAVELENGTH4 = 
    "g_RGBInvWavelength4";
const char* NiSkyMaterial::SC_HDREXPOSURE = 
    "g_HDRExposure";
const char* NiSkyMaterial::SC_UPVECTOR = 
    "g_UpVector";
const char* NiSkyMaterial::SC_PLANETDIMENSIONS = 
    "g_PlanetDimensions";
const char* NiSkyMaterial::SC_SCALEDEPTH = 
    "g_AtmosphericScaleDepth";
const char* NiSkyMaterial::SC_FRAMEDATA = 
    "g_FrameData";
const char* NiSkyMaterial::SC_NUMSAMPLESINT = 
    "g_iNumSamples";
const char* NiSkyMaterial::SC_NUMSAMPLESFLOAT = 
    "g_fNumSamples";
const char* NiSkyMaterial::SC_STAGEMODIFIEREXPONENT = 
    "ModifierExponent";
const char* NiSkyMaterial::SC_STAGEMODIFIERHORIZONBIAS =
    "ModifierHorizonBias";
const char* NiSkyMaterial::SC_STAGEMODIFIERCONSTANT =
    "ModifierConstant";
const char* NiSkyMaterial::SC_STAGEGRADIENTHORIZON = 
    "GradientHorizonColor";
const char* NiSkyMaterial::SC_STAGEGRADIENTZENITH = 
    "GradientZenithColor";
const char* NiSkyMaterial::SC_STAGEGRADIENTEXPONENT = 
    "GradientExponent";
const char* NiSkyMaterial::SC_STAGEGRADIENTHORIZONBIAS =
    "GradientHorizonBias";
const char* NiSkyMaterial::SC_STAGESKYBOXORIENTATION =
    "SkyboxOrientation";
//---------------------------------------------------------------------------
NiImplementRTTI(NiSkyMaterial, NiFragmentMaterial, NiTypeMask::NiSkyMaterial);

//---------------------------------------------------------------------------
NiSkyMaterial::NiSkyMaterial(NiMaterialNodeLibrary* pkMaterialNodeLib,
    bool bAutoCreateCaches, const NiFixedString &kName) : NiFragmentMaterial(
    pkMaterialNodeLib, 
    kName, 
    VERTEX_VERSION, GEOMETRY_VERSION, PIXEL_VERSION, 
    bAutoCreateCaches),
    m_kMaterialDescriptorName("NiSkyMaterialDescriptor"),
    m_kVertexShaderDescriptorName("NiSkyMaterialVertexDescriptor"),
    m_kPixelShaderDescriptorName("NiSkyMaterialPixelDescriptor")
{   
    m_kLibraries.Add(
        NiSkyMaterialNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(
        NiAtmosphereMaterialNodeLibrary::CreateMaterialNodeLibrary());
}

//---------------------------------------------------------------------------
NiSkyMaterial::~NiSkyMaterial()
{
}

//---------------------------------------------------------------------------
NiSkyMaterial* NiSkyMaterial::Create()
{
    // Get the material if it exist already
    NiSkyMaterial* pkMaterial = NiDynamicCast(NiSkyMaterial, 
        NiMaterial::GetMaterial("NiSkyMaterial"));

    if (!pkMaterial)
    {
        // Create a new material if we didn't find it
        pkMaterial = NiNew NiSkyMaterial(NULL);
        pkMaterial->AddDefaultFallbacks();
    }

    return pkMaterial;
}

//---------------------------------------------------------------------------
NiShader* NiSkyMaterial::CreateShader(NiMaterialDescriptor* pkDesc)
{
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    if (pkRenderer == NULL)
        return NULL;

    return pkRenderer->GetFragmentShader(pkDesc);
}

//---------------------------------------------------------------------------
bool NiSkyMaterial::SetupPackingRequirements(NiShader* pkShader,
    NiMaterialDescriptor* pkMaterialDescriptor,
    RenderPassDescriptor*, NiUInt32)
{
    EE_UNUSED_ARG(pkMaterialDescriptor);

    NiUInt32 uiStreamCount = 1;    
    NiShaderDeclarationPtr spShaderDecl = 
        NiShaderDeclaration::Create(15, uiStreamCount);

    if (!spShaderDecl)
    {
        EE_FAIL("Invalid shader declaration");
        return false;
    }
    
    NiUInt32 uiEntryCount = 0;
    
    // Handle position and normal streams.
    spShaderDecl->SetEntry(uiEntryCount++, 
        NiShaderDeclaration::SHADERPARAM_NI_POSITION0,
        NiShaderDeclaration::SPTYPE_FLOAT3);

    spShaderDecl->SetEntry(uiEntryCount++,
        NiShaderDeclaration::SHADERPARAM_NI_NORMAL,
        NiShaderDeclaration::SPTYPE_FLOAT3);

    pkShader->SetSemanticAdapterTableFromShaderDeclaration(spShaderDecl);
    
    return true;
}

//---------------------------------------------------------------------------
bool NiSkyMaterial::GenerateDescriptor(const NiRenderObject* pkGeometry, 
    const NiPropertyState* pkPropState, 
    const NiDynamicEffectState* pkEffects,
    NiMaterialDescriptor& kMaterialDesc)
{
    if (!pkPropState)
    {
        EE_FAIL("Could not find property state! Try calling"
            " UpdateProperties.\n");
     
        return false;
    }
    
    NiSkyMaterialDescriptor* pkDesc = (NiSkyMaterialDescriptor*)
        &kMaterialDesc;    
    pkDesc->m_kIdentifier = m_kMaterialDescriptorName;

    // Make sure the sky material is being applied to the proper geometry.
    if (pkGeometry->RequiresMaterialOption(
        NiRenderObjectMaterialOption::TRANSFORM_SKINNED()))
    {
        EE_FAIL("Cannot apply sky material to skinned geometry.\n");
        return false;
    }

    if (pkGeometry->RequiresMaterialOption(
        NiRenderObjectMaterialOption::TRANSFORM_INSTANCED()))
    {
        EE_FAIL("Cannot apply sky material to instanced geometry.\n");
        return false;
    }

    if (pkGeometry->RequiresMaterialOption(
        NiRenderObjectMaterialOption::MORPHING()))
    {
        EE_FAIL("Cannot apply sky material to morphed geometry.\n");
        return false;
    }
    
    // Initialize the dependency flags
    bool bNeedSunlight = false;

    // Select the type of atmospheric scattering calculations to be performed
    NiIntegerExtraData* pkExtraMode = NiDynamicCast(NiIntegerExtraData,
        pkGeometry->GetExtraData(NiSkyMaterial::ED_ATMOSPHERICCALCMODE));

    if (pkExtraMode)
    {
        NiUInt32 uiAtmosCalcMode = pkExtraMode->GetValue();
        pkDesc->SetATMOSPHERE_CALC_MODE(uiAtmosCalcMode);

        bNeedSunlight = uiAtmosCalcMode != AtmosphericCalcMode::NONE;
    }
    else
    {
        pkDesc->SetATMOSPHERE_CALC_MODE(AtmosphericCalcMode::NONE);
    }

    // Get the first directional light on this mesh and use it as the sun
    // not required if we aren't doing scattering though 
    if (pkEffects && bNeedSunlight)
    {
        bool bFoundSun = false;

        // Grab the first directional light if it exists and treat it as the 
        // sunlight.
        NiDynEffectStateIter kLightIter = pkEffects->GetLightHeadPos();
        while (kLightIter != NULL)
        {
            NiLight* pkLight = pkEffects->GetNextLight(kLightIter);
            if (pkLight && 
                (pkLight->GetEffectType() == NiDynamicEffect::DIR_LIGHT ||
                 pkLight->GetEffectType() == NiDynamicEffect::SHADOWDIR_LIGHT))
            {
                bFoundSun = true;
                break;
            }
        }

        if (!bFoundSun)
        {
            EE_FAIL("The atmospheric sky mesh must be affected by at least 1 "
                "directional light.");
            return false;
        }
    }

    // Iterate over all the possible blend stages and check for configurations
    NiTexturingProperty *pkTexProp = pkPropState->GetTexturing();
    for (NiUInt32 uiStageIndex = 0; uiStageIndex < NUM_BLEND_STAGES; 
        ++uiStageIndex)
    {
        NiUInt32 uiModifierSource = ModifierSource::DISABLED;
        NiUInt32 uiBlendMethod = BlendMethod::CUSTOM;
        NiUInt32 uiColorMap = ColorMap::CUSTOM;

        // Fetch the stage configuration from an extra data
        GetBlendStageConfiguration(pkGeometry, uiStageIndex, 
            uiColorMap, uiModifierSource, uiBlendMethod);

        // Decide upon the type of color map used in this stage
        {
            // Check for a cube map
            if (uiColorMap == ColorMap::SKYBOX && 
                !(pkTexProp && pkTexProp->GetShaderMap(uiStageIndex)))
            {
                return false;
            }

            // Check for gradient constants
            if (uiColorMap == ColorMap::GRADIENT)
            {
                NiFixedString kExponentName = 
                    GenerateConstantName(uiStageIndex, 
                    SC_STAGEGRADIENTEXPONENT);
                NiFixedString kHorizonBiasName = 
                    GenerateConstantName(uiStageIndex, 
                    SC_STAGEGRADIENTHORIZONBIAS);
                NiFixedString kHorizonColorName = 
                    GenerateConstantName(uiStageIndex, 
                    SC_STAGEGRADIENTHORIZON);
                NiFixedString kZenithColorName = 
                    GenerateConstantName(uiStageIndex, 
                    SC_STAGEGRADIENTZENITH);

                if (!(pkGeometry->GetExtraData(kExponentName) && 
                    pkGeometry->GetExtraData(kHorizonBiasName) &&
                    pkGeometry->GetExtraData(kHorizonColorName) &&
                    pkGeometry->GetExtraData(kZenithColorName)))
                {
                    return false;
                }
            }
            
            // Check for fog constants
            if (uiColorMap == ColorMap::FOG && 
                !pkPropState->GetFog())
            {
                return false;
            }
        }

        // Decide upon the type of modifier used in this stage
        {
            // Check for a shader constant modifier
            if (uiModifierSource == ModifierSource::CONSTANT)
            {
                NiFixedString kConstantName = 
                    GenerateConstantName(uiStageIndex, 
                    SC_STAGEMODIFIERCONSTANT);
                
                if (!(pkGeometry->GetExtraData(kConstantName)))
                {
                    return false;
                }
            }

            // Check for horizon bias constants
            if (uiModifierSource == ModifierSource::HORIZONBIAS)
            {
                NiFixedString kExponentName = 
                    GenerateConstantName(uiStageIndex, 
                    SC_STAGEMODIFIEREXPONENT);
                NiFixedString kHorizonBiasName = 
                    GenerateConstantName(uiStageIndex, 
                    SC_STAGEMODIFIERHORIZONBIAS);

                if (!(pkGeometry->GetExtraData(kExponentName) && 
                    pkGeometry->GetExtraData(kHorizonBiasName)))
                {
                    return false;
                }
            }
        }

        SetStageConfiguration(pkDesc, uiStageIndex, uiColorMap, 
            uiModifierSource, uiBlendMethod);
    }

    return true;
}

//---------------------------------------------------------------------------
void NiSkyMaterial::SetBlendStageConfiguration(NiAVObject* pkObject, 
    NiUInt32 uiStageIndex, NiUInt32 uiColorMap, NiUInt32 uiModifierSource, 
    NiUInt32 uiBlendMethod) const
{
    NiUInt32 uiConfiguration = 0;
    uiConfiguration += uiColorMap << 16;
    uiConfiguration += uiModifierSource << 8;
    uiConfiguration += uiBlendMethod;

    NiFixedString kStageConfiguration = GenerateConstantName(uiStageIndex,
        ED_STAGECONFIGURATION);
    NiIntegerExtraData* pkExtraData = NiDynamicCast(NiIntegerExtraData,
        pkObject->GetExtraData(kStageConfiguration));
    if (!pkExtraData)
    {
        pkExtraData = NiNew NiIntegerExtraData(0);
        pkObject->AddExtraData(kStageConfiguration,pkExtraData);
    }
        
    pkExtraData->SetValue(uiConfiguration);
}

//---------------------------------------------------------------------------
bool NiSkyMaterial::GetBlendStageConfiguration(const NiAVObject* pkObject, 
    NiUInt32 uiStageIndex, NiUInt32& kColorMap, NiUInt32& kModifierSource, 
    NiUInt32& kBlendMethod) const
{
    NiFixedString kStageConfiguration = GenerateConstantName(uiStageIndex,
        ED_STAGECONFIGURATION);
    NiIntegerExtraData* pkExtraData = NiDynamicCast(NiIntegerExtraData,
        pkObject->GetExtraData(kStageConfiguration));
    if (!pkExtraData)
        return false;
    
    NiUInt32 uiConfiguration = pkExtraData->GetValue();

    kColorMap = (uiConfiguration & 0x00FF0000) >> 16;
    kModifierSource = (uiConfiguration & 0x0000FF00) >> 8;
    kBlendMethod = (uiConfiguration & 0x000000FF);

    return true;
}

//---------------------------------------------------------------------------
void NiSkyMaterial::DisableBlendStage(NiAVObject* pkObject, 
    NiUInt32 uiStageIndex)
{
    NiFixedString kStageConfiguration = GenerateConstantName(uiStageIndex,
        ED_STAGECONFIGURATION);
    pkObject->RemoveExtraData(kStageConfiguration);
}

//---------------------------------------------------------------------------
void NiSkyMaterial::SetStageConfiguration(NiSkyMaterialDescriptor* pkDesc,
    NiUInt32 uiStageIndex, NiUInt32 uiColorMap, NiUInt32 uiModifierSource, 
    NiUInt32 uiBlendMethod)
{
    switch (uiStageIndex)
    {
    case 0:
        pkDesc->SetSTAGE0_MODIFIER_SELECTION(uiModifierSource);
        pkDesc->SetSTAGE0_COLORMAP_SELECTION(uiColorMap);
        pkDesc->SetSTAGE0_BLENDMODE_SELECTION(uiBlendMethod);
        break;
    case 1:
        pkDesc->SetSTAGE1_MODIFIER_SELECTION(uiModifierSource);
        pkDesc->SetSTAGE1_COLORMAP_SELECTION(uiColorMap);
        pkDesc->SetSTAGE1_BLENDMODE_SELECTION(uiBlendMethod);
        break;
    case 2:
        pkDesc->SetSTAGE2_MODIFIER_SELECTION(uiModifierSource);
        pkDesc->SetSTAGE2_COLORMAP_SELECTION(uiColorMap);
        pkDesc->SetSTAGE2_BLENDMODE_SELECTION(uiBlendMethod);
        break;
    case 3:
        pkDesc->SetSTAGE3_MODIFIER_SELECTION(uiModifierSource);
        pkDesc->SetSTAGE3_COLORMAP_SELECTION(uiColorMap);
        pkDesc->SetSTAGE3_BLENDMODE_SELECTION(uiBlendMethod);
        break;
    case 4:
        pkDesc->SetSTAGE4_MODIFIER_SELECTION(uiModifierSource);
        pkDesc->SetSTAGE4_COLORMAP_SELECTION(uiColorMap);
        pkDesc->SetSTAGE4_BLENDMODE_SELECTION(uiBlendMethod);
        break;
    default:
        EE_FAIL("Setting config of an unsupported blend stage");
    }
}

//---------------------------------------------------------------------------
void NiSkyMaterial::SetAtmosphericCalcMode(NiAVObject* pkObject, 
    NiUInt32 uiCalcMode)
{
    NiIntegerExtraData* pkExtraData = NiDynamicCast(NiIntegerExtraData,
        pkObject->GetExtraData(ED_ATMOSPHERICCALCMODE));
    if (!pkExtraData)
    {
        pkExtraData = NiNew NiIntegerExtraData(0);
        pkObject->AddExtraData(ED_ATMOSPHERICCALCMODE, pkExtraData);
    }

    pkExtraData->SetValue(uiCalcMode);
}

//---------------------------------------------------------------------------
void NiSkyMaterial::SetModifierHorizonBiasValues(NiAVObject* pkObject,
    NiUInt32 uiStageIndex, float fExponent, float fHorizonBias)
{
    NiFixedString kExponent = GenerateConstantName(uiStageIndex, 
        SC_STAGEMODIFIEREXPONENT);
    NiFixedString kHorizonBias = GenerateConstantName(uiStageIndex, 
        SC_STAGEMODIFIERHORIZONBIAS);
    NiFloatExtraData* pkExponent = NiDynamicCast(NiFloatExtraData,
        pkObject->GetExtraData(kExponent));
    NiFloatExtraData* pkHorizonBias = NiDynamicCast(NiFloatExtraData,
        pkObject->GetExtraData(kHorizonBias));

    if (!pkExponent)
    {
        pkExponent = NiNew NiFloatExtraData(0);
        pkObject->AddExtraData(kExponent, pkExponent);
    }
    if (!pkHorizonBias)
    {
        pkHorizonBias = NiNew NiFloatExtraData(0);
        pkObject->AddExtraData(kHorizonBias, pkHorizonBias);
    }

    pkExponent->SetValue(fExponent);
    pkHorizonBias->SetValue(fHorizonBias);
}

//---------------------------------------------------------------------------
void NiSkyMaterial::SetModifierValue(NiAVObject* pkObject, 
    NiUInt32 uiStageIndex, float fModifier)
{
    NiFixedString kModifierName = GenerateConstantName(uiStageIndex, 
        SC_STAGEMODIFIERCONSTANT);
    NiFloatExtraData* pkExtraData = NiDynamicCast(NiFloatExtraData,
        pkObject->GetExtraData(kModifierName));
    if (!pkExtraData)
    {
        pkExtraData = NiNew NiFloatExtraData(0);
        pkObject->AddExtraData(kModifierName, pkExtraData);
    }

    pkExtraData->SetValue(fModifier);
}

//---------------------------------------------------------------------------
void NiSkyMaterial::SetGradientValues(NiAVObject* pkObject, 
    NiUInt32 uiStageIndex, float fExponent, float fHorizonBias, 
    const NiColorA& kHorizonColor, const NiColorA& kZenithColor)
{
    NiFixedString kExponent = GenerateConstantName(uiStageIndex, 
        SC_STAGEGRADIENTEXPONENT);
    NiFixedString kHorizonBias = GenerateConstantName(uiStageIndex, 
        SC_STAGEGRADIENTHORIZONBIAS);
    NiFloatExtraData* pkExponent = NiDynamicCast(NiFloatExtraData,
        pkObject->GetExtraData(kExponent));
    NiFloatExtraData* pkHorizonBias = NiDynamicCast(NiFloatExtraData,
        pkObject->GetExtraData(kHorizonBias));

    NiFixedString kHorizon = GenerateConstantName(uiStageIndex, 
        SC_STAGEGRADIENTHORIZON);
    NiFixedString kZenith = GenerateConstantName(uiStageIndex, 
        SC_STAGEGRADIENTZENITH);
    NiColorExtraData* pkHorizonColor = NiDynamicCast(NiColorExtraData,
        pkObject->GetExtraData(kHorizon));
    NiColorExtraData* pkZenithColor = NiDynamicCast(NiColorExtraData,
        pkObject->GetExtraData(kZenith));

    if (!pkExponent)
    {
        pkExponent = NiNew NiFloatExtraData(0);
        pkObject->AddExtraData(kExponent, pkExponent);
    }
    if (!pkHorizonBias)
    {
        pkHorizonBias = NiNew NiFloatExtraData(0);
        pkObject->AddExtraData(kHorizonBias, pkHorizonBias);
    }
    if (!pkHorizonColor)
    {
        pkHorizonColor = NiNew NiColorExtraData(NiColor::WHITE);
        pkObject->AddExtraData(kHorizon, pkHorizonColor);
    }
    if (!pkZenithColor)
    {
        pkZenithColor = NiNew NiColorExtraData(NiColor::WHITE);
        pkObject->AddExtraData(kZenith, pkZenithColor);
    }

    pkExponent->SetValue(fExponent);
    pkHorizonBias->SetValue(fHorizonBias);
    pkHorizonColor->SetValue(kHorizonColor);
    pkZenithColor->SetValue(kZenithColor);
}

//---------------------------------------------------------------------------
void NiSkyMaterial::SetOrientedSkyboxValues(NiAVObject* pkObject, 
    NiUInt32 uiStageIndex, const NiMatrix3& kOrientation)
{
    NiFixedString kOrientationName = GenerateConstantName(uiStageIndex, 
        SC_STAGESKYBOXORIENTATION);

    NiFloatsExtraData* pkOrientation = 
        (NiFloatsExtraData*)pkObject->GetExtraData(kOrientationName);
    if (!pkOrientation)
    {
        const float afIdentity[] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};

        pkOrientation = NiNew NiFloatsExtraData(16, afIdentity);
        pkOrientation->SetName(kOrientationName);
        pkObject->AddExtraData(pkOrientation);
    }

    // Manually set each value to avoid the re-allocation in the 
    // NiFloatsExtraData::SetArray
    EE_VERIFY(pkOrientation->SetValue(0, kOrientation.GetEntry(0,0)));
    EE_VERIFY(pkOrientation->SetValue(1, kOrientation.GetEntry(0,1)));
    EE_VERIFY(pkOrientation->SetValue(2, kOrientation.GetEntry(0,2)));

    EE_VERIFY(pkOrientation->SetValue(4, kOrientation.GetEntry(1,0)));
    EE_VERIFY(pkOrientation->SetValue(5, kOrientation.GetEntry(1,1)));
    EE_VERIFY(pkOrientation->SetValue(6, kOrientation.GetEntry(1,2)));

    EE_VERIFY(pkOrientation->SetValue(8, kOrientation.GetEntry(2,0)));
    EE_VERIFY(pkOrientation->SetValue(9, kOrientation.GetEntry(2,1)));
    EE_VERIFY(pkOrientation->SetValue(10, kOrientation.GetEntry(2,2)));
}

//---------------------------------------------------------------------------
NiFragmentMaterial::ReturnCode NiSkyMaterial::GenerateShaderDescArray(
    NiMaterialDescriptor* pkMaterialDescriptor,
    RenderPassDescriptor* pkRenderPasses,
    NiUInt32 uiMaxCount,
    NiUInt32& uiCountAdded)
{
    EE_UNUSED_ARG(uiMaxCount);
    EE_ASSERT(uiMaxCount != 0);
    uiCountAdded = 0;

    if (pkMaterialDescriptor->m_kIdentifier != m_kMaterialDescriptorName)
        return NiFragmentMaterial::RC_INVALID_MATERIAL;

    // Setup the first pass.
    pkRenderPasses[0].m_bUsesNiRenderState = true;
    pkRenderPasses[0].m_bResetObjectOffsets = true;

    NiSkyMaterialDescriptor* pkMaterialDesc = (NiSkyMaterialDescriptor*)
        pkMaterialDescriptor;

    NiSkyMaterialVertexDescriptor* pkVertexDesc =
        (NiSkyMaterialVertexDescriptor*)pkRenderPasses[0].m_pkVertexDesc;
    pkVertexDesc->m_kIdentifier = m_kVertexShaderDescriptorName;

    NiSkyMaterialPixelDescriptor* pkPixelDesc = 
        (NiSkyMaterialPixelDescriptor*)pkRenderPasses[0].m_pkPixelDesc;
    pkPixelDesc->m_kIdentifier = m_kPixelShaderDescriptorName;

    // Set pixel descriptor
    pkPixelDesc->SetATMOSPHERE_CALC_MODE(
        pkMaterialDesc->GetATMOSPHERE_CALC_MODE());

    pkPixelDesc->SetSTAGE0_BLENDMODE_SELECTION(
        pkMaterialDesc->GetSTAGE0_BLENDMODE_SELECTION());
    pkPixelDesc->SetSTAGE0_COLORMAP_SELECTION(
        pkMaterialDesc->GetSTAGE0_COLORMAP_SELECTION());
    pkPixelDesc->SetSTAGE0_MODIFIER_SELECTION(
        pkMaterialDesc->GetSTAGE0_MODIFIER_SELECTION());

    pkPixelDesc->SetSTAGE1_BLENDMODE_SELECTION(
        pkMaterialDesc->GetSTAGE1_BLENDMODE_SELECTION());
    pkPixelDesc->SetSTAGE1_COLORMAP_SELECTION(
        pkMaterialDesc->GetSTAGE1_COLORMAP_SELECTION());
    pkPixelDesc->SetSTAGE1_MODIFIER_SELECTION(
        pkMaterialDesc->GetSTAGE1_MODIFIER_SELECTION());

    pkPixelDesc->SetSTAGE2_BLENDMODE_SELECTION(
        pkMaterialDesc->GetSTAGE2_BLENDMODE_SELECTION());
    pkPixelDesc->SetSTAGE2_COLORMAP_SELECTION(
        pkMaterialDesc->GetSTAGE2_COLORMAP_SELECTION());
    pkPixelDesc->SetSTAGE2_MODIFIER_SELECTION(
        pkMaterialDesc->GetSTAGE2_MODIFIER_SELECTION());

    pkPixelDesc->SetSTAGE3_BLENDMODE_SELECTION(
        pkMaterialDesc->GetSTAGE3_BLENDMODE_SELECTION());
    pkPixelDesc->SetSTAGE3_COLORMAP_SELECTION(
        pkMaterialDesc->GetSTAGE3_COLORMAP_SELECTION());
    pkPixelDesc->SetSTAGE3_MODIFIER_SELECTION(
        pkMaterialDesc->GetSTAGE3_MODIFIER_SELECTION());

    pkPixelDesc->SetSTAGE4_BLENDMODE_SELECTION(
        pkMaterialDesc->GetSTAGE4_BLENDMODE_SELECTION());
    pkPixelDesc->SetSTAGE4_COLORMAP_SELECTION(
        pkMaterialDesc->GetSTAGE4_COLORMAP_SELECTION());
    pkPixelDesc->SetSTAGE4_MODIFIER_SELECTION(
        pkMaterialDesc->GetSTAGE4_MODIFIER_SELECTION());

    // Set the vertex descriptor
    pkVertexDesc->SetATMOSPHERE_CALC_MODE(
        pkMaterialDesc->GetATMOSPHERE_CALC_MODE());
    pkVertexDesc->SetUSES_NORMALS(pkPixelDesc->InputNormals());

    uiCountAdded++;
    return NiFragmentMaterial::RC_SUCCESS;
}

//---------------------------------------------------------------------------
bool NiSkyMaterial::GenerateVertexShadeTree(Context& kContext, 
    NiGPUProgramDescriptor* pkDesc)
{
    EE_ASSERT(pkDesc->m_kIdentifier == "NiSkyMaterialVertexDescriptor");
    NiSkyMaterialVertexDescriptor* pkVertexDesc = 
        (NiSkyMaterialVertexDescriptor*)pkDesc;

    kContext.m_spConfigurator->SetDescription(pkVertexDesc->ToString());

    // Add vertex in, vertex out, constants, and uniforms
    if (!AddDefaultMaterialNodes(kContext, pkDesc, 
        NiGPUProgram::PROGRAM_VERTEX))
    {
        return false;
    }

    // Create the main vertex shader output
    NiMaterialResource* pkVertOutProjPos = 
        kContext.m_spOutputs->AddInputResource("float4", "Position",
        "World", "WorldProj");
    
    // Work out transformation pipeline
    NiMaterialResource* pkWorldPos = NULL;
    NiMaterialResource* pkWorldView = NULL;
    NiMaterialResource* pkVertProjPos = NULL;
    SetupTransformPipeline(kContext, pkVertProjPos, pkVertexDesc, 
        pkWorldPos, pkWorldView);

    // Destroy Depth Information to stop far clipping plane from cutting sky
    bool bDestroyDepthInformation = true;
    if (bDestroyDepthInformation)
    {
        HandleDestroyDepthInformation(kContext, pkVertProjPos, pkVertProjPos);
    }
    kContext.m_spConfigurator->AddBinding(pkVertProjPos, pkVertOutProjPos);

    // Handle Atmospheric Scattering
    if (!HandleVertexAtmosphericScattering(kContext, pkVertexDesc, 
        pkWorldView))
    {
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
bool NiSkyMaterial::GenerateGeometryShadeTree(Context&, 
    NiGPUProgramDescriptor*)
{
    // Geometry fragments not supported.
    return true;
}

//---------------------------------------------------------------------------
bool NiSkyMaterial::GeneratePixelShadeTree(Context& kContext,
    NiGPUProgramDescriptor* pkDesc)
{
    EE_ASSERT(pkDesc->m_kIdentifier == "NiSkyMaterialPixelDescriptor");
    NiSkyMaterialPixelDescriptor* pkPixelDesc = 
        (NiSkyMaterialPixelDescriptor*)pkDesc;

    kContext.m_spConfigurator->SetDescription(pkPixelDesc->ToString());

    // Add vertex in, vertex out, constants, and uniforms
    if (!AddDefaultMaterialNodes(kContext, pkPixelDesc, 
        NiGPUProgram::PROGRAM_PIXEL))
    {
        return false;
    }
      
    // Create the final pixel out color.
    NiMaterialResource* pkPixelOutColor = 
        kContext.m_spOutputs->AddInputResource("float4", "Color", "", 
        "Color0"); 
    NiMaterialResource* pkCurrentColor = 
        kContext.m_spStatics->AddOutputConstant("float4", "(0, 0, 0, 1)");

    // Setup Pixel shader inputs (in the correct order)
    NiMaterialResource* pkWorldView = NULL;
    NiMaterialResource* pkNormals = NULL;
    if (!SetupPixelInputs(kContext, pkPixelDesc, pkWorldView, pkNormals))
    {
        return false;
    }

    // Handle Atmospheric Scattering
    if (!HandlePixelAtmosphericScattering(kContext, pkPixelDesc, 
        pkCurrentColor, pkWorldView))
    {
        return false;
    }

    // Handle blending the supplied texture maps
    if (!HandleBlendStages(kContext, pkPixelDesc, pkCurrentColor, pkNormals,
        pkWorldView))
    {
        return false;
    }

    // Bind the resulting color to the output
    kContext.m_spConfigurator->AddBinding(pkCurrentColor, pkPixelOutColor);

    return true;
}

//---------------------------------------------------------------------------
NiUInt32 NiSkyMaterial::VerifyShaderPrograms(NiGPUProgram* pkVertexShader,
    NiGPUProgram* pkGeometryShader, NiGPUProgram* pkPixelShader)
{
    NiUInt32 uiReturnCode = RC_SUCCESS;
    if (pkVertexShader == NULL)
        uiReturnCode |= RC_COMPILE_FAILURE_VERTEX;
    if (pkPixelShader == NULL)
        uiReturnCode |= RC_COMPILE_FAILURE_PIXEL;
    // No need to check geometry shader (2nd argument).
    EE_UNUSED_ARG(pkGeometryShader);

    return uiReturnCode;
}

//---------------------------------------------------------------------------
bool NiSkyMaterial::SetupTransformPipeline(Context& kContext,
    NiMaterialResource*& pkVertOutProjPos,
    NiSkyMaterialVertexDescriptor* pkVertDesc, 
    NiMaterialResource*& pkWorldPos, NiMaterialResource*& pkWorldView)
{
    // Initialize the outputs
    pkWorldPos = NULL;

    // Fetch the required inputs for the following operations:
    NiMaterialResource* pkLocalPos = kContext.m_spInputs->
        AddOutputResource("float3", "Position", "Local", "Position", 1);
    NiMaterialResource* pkWorldMatrix = AddOutputPredefined(
        kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_WORLD, 4);
    NiMaterialResource* pkWVPMatix = AddOutputPredefined(
        kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_WORLDVIEWPROJ, 4);
    
    // Get the world position
    if (!HandlePositionTransform(kContext, pkLocalPos, pkWorldMatrix, 
        pkWorldPos))
    {
        return false;
    }
   
    // Get the world normals
    if (pkVertDesc->OutputNormals())
    {
        NiMaterialResource* pkLocalNormal = kContext.m_spInputs->
            AddOutputResource("float3", "Normal", "Local", "Normal", 1);
        NiMaterialResource* pkWorldNormal = NULL;

        if (!HandleDirectionTransform(kContext, pkLocalNormal, pkWorldMatrix,
            pkWorldNormal))
        {
            return false;
        }

        NiMaterialResource* pkVertOutNormal = 
            kContext.m_spOutputs->AddInputResource("float3", "TexCoord", 
            "World", "Normal");
        kContext.m_spConfigurator->AddBinding(pkWorldNormal, pkVertOutNormal);
    }

    // Calculate WorldView and ViewDistance in vertex shader
    if (pkVertDesc->OutputWorldView())
    {        
        if (!HandleViewVector(kContext, pkWorldPos, pkWorldView))
        {
            return false;
        }
    }

    // Get the projected position
    if (!HandlePositionTransform(kContext, pkLocalPos, pkWVPMatix, 
        pkVertOutProjPos))
    {
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------
bool NiSkyMaterial::HandlePositionTransform(Context& kContext, 
    NiMaterialResource* pkOriginalPos, NiMaterialResource* pkTransformMatrix, 
    NiMaterialResource*& pkTransformPos)
{
    EE_ASSERT(pkOriginalPos);
    EE_ASSERT(pkTransformMatrix);

    pkTransformPos = NULL;
    
    // Convert the initial local coordinate from the vertex stream into a 
    // world coordinate for later use in the pipeline.

    NiMaterialNode* pkTransformPosFrag = GetAttachableNodeFromLibrary(
        "TransformPosition");
    EE_ASSERT(pkTransformPosFrag);
    kContext.m_spConfigurator->AddNode(pkTransformPosFrag);
    
    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkOriginalPos,
        "OriginalPos", pkTransformPosFrag);
    kContext.m_spConfigurator->AddBinding(pkTransformMatrix,
        "TransformMatrix", pkTransformPosFrag);

    // Bind Outputs
    pkTransformPos = pkTransformPosFrag->GetOutputResourceByVariableName(
        "TransformPos");

    return true;
}

//---------------------------------------------------------------------------
bool NiSkyMaterial::HandleViewVector(Context& kContext,
    NiMaterialResource* pkWorldPos, NiMaterialResource*& pkWorldView)
{
    EE_ASSERT(pkWorldPos);

    // Grab a copy of the camera position from the shader constants
    NiMaterialResource* pkCameraPos = AddOutputPredefined(
        kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_EYE_POS);

    // Instantiate the view vector calculation node
    NiMaterialNode* pkCalcViewVectorFrag = GetAttachableNodeFromLibrary(
        "CalcViewVector");
    EE_ASSERT(pkCalcViewVectorFrag);
    kContext.m_spConfigurator->AddNode(pkCalcViewVectorFrag);
    
    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkCameraPos,
        "CameraPos", pkCalcViewVectorFrag);
    kContext.m_spConfigurator->AddBinding(pkWorldPos,
        "WorldPos", pkCalcViewVectorFrag);

    // Bind Outputs
    pkWorldView = pkCalcViewVectorFrag->GetOutputResourceByVariableName(
        "ViewVector");
    EE_ASSERT(pkWorldView);

    // Bind the world view vector as an output to the pixel shader.
    NiMaterialResource* pkVertOutWorldView = 
        kContext.m_spOutputs->AddInputResource("float3", "TexCoord", 
        "World", "WorldView");
    kContext.m_spConfigurator->AddBinding(pkWorldView, pkVertOutWorldView);

    return true;
}

//----------------------------------------------------------------------------
bool NiSkyMaterial::HandleDirectionTransform(Context& kContext, 
    NiMaterialResource* pkOriginalDir, NiMaterialResource* pkTransformMatrix, 
    NiMaterialResource*& pkTransformDir)
{
    EE_ASSERT(pkOriginalDir);
    EE_ASSERT(pkTransformMatrix);

    pkTransformDir = NULL;
    
    // Convert the initial local coordinate from the vertex stream into a 
    // world coordinate for later use in the pipeline.

    NiMaterialNode* pkTransformDirFrag = GetAttachableNodeFromLibrary(
        "TransformDirection");
    EE_ASSERT(pkTransformDirFrag);
    kContext.m_spConfigurator->AddNode(pkTransformDirFrag);
    
    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkOriginalDir,
        "OriginalDir", pkTransformDirFrag);
    kContext.m_spConfigurator->AddBinding(pkTransformMatrix,
        "TransformMatrix", pkTransformDirFrag);

    // Bind Outputs
    pkTransformDir = pkTransformDirFrag->GetOutputResourceByVariableName(
        "TransformDir");

    return true;
}

//----------------------------------------------------------------------------
void NiSkyMaterial::HandleDestroyDepthInformation(Context& kContext, 
    NiMaterialResource* pkVertProjPos, NiMaterialResource*& pkVertProjPosOut)
{
    EE_ASSERT(pkVertProjPos);

    // Destroy the depth information for this vertex to prevent clipping
    NiMaterialNode* pkDestroyDepthInfo = GetAttachableNodeFromLibrary(
        "DestroyDepthInformation");
    kContext.m_spConfigurator->AddNode(pkDestroyDepthInfo);
    EE_ASSERT(pkDestroyDepthInfo);

    // Bind the Inputs
    kContext.m_spConfigurator->AddBinding(pkVertProjPos,
        pkDestroyDepthInfo->GetInputResourceByVariableName("posProjectedIn"));

    // Bind the Outputs
    pkVertProjPosOut = pkDestroyDepthInfo->GetOutputResourceByVariableName(
        "posProjectedOut");
    EE_ASSERT(pkVertProjPosOut);
}

//----------------------------------------------------------------------------
bool NiSkyMaterial::SetupPixelInputs(Context& kContext, 
    NiSkyMaterialPixelDescriptor* pkPixelDesc, 
    NiMaterialResource*& pkWorldView, NiMaterialResource*& pkNormals)
{
    kContext.m_spInputs->AddOutputResource("float4", "Position", "World", 
        "WorldProjPos");
    
    if (pkPixelDesc->InputNormals())
    {
        pkNormals = kContext.m_spInputs->AddOutputResource("float3", 
            "TexCoord", "World", "Normal");
        EE_ASSERT(pkNormals);
    }

    if (pkPixelDesc->InputWorldView())
    {
        pkWorldView = kContext.m_spInputs->AddOutputResource("float3", 
            "TexCoord", "World", "WorldView");
        EE_ASSERT(pkWorldView);
    }
    
    return true;
}

//----------------------------------------------------------------------------
bool NiSkyMaterial::HandleVertexAtmosphericScattering(Context& kContext,
    NiSkyMaterialVertexDescriptor* pkVertDesc, NiMaterialResource* pkWorldView)
{
    NiUInt32 uiAtmosphericCalcMode = pkVertDesc->GetATMOSPHERE_CALC_MODE();
    if (uiAtmosphericCalcMode == AtmosphericCalcMode::NONE ||
        uiAtmosphericCalcMode == AtmosphericCalcMode::GPU_PS) return true;

    // Check inputs
    EE_ASSERT(pkWorldView);

    // Perform scattering calculations or pass through results from app
    NiMaterialResource* pkRayleighScattering = NULL;
    NiMaterialResource* pkMieScattering = NULL;
    if (uiAtmosphericCalcMode == AtmosphericCalcMode::GPU_VS)
    {
        // Calculate scattering here in the vertex shader
        NiMaterialResource* pkHDRExposure = NULL;
        NiMaterialResource* pkPhaseConstant = NULL;
        NiMaterialResource* pkPhaseConstant2 = NULL;
        NiMaterialResource* pkCameraHeight = NULL;
        NiMaterialResource* pkCameraHeight2 = NULL;
        NiMaterialResource* pkScale = NULL;
        NiMaterialResource* pkScaleDepth = NULL;
        NiMaterialResource* pkScaleOverScaleDepth = NULL;
        NiMaterialResource* pkSunSizeMultiplier = NULL;
        NiMaterialResource* pkOuterRadius = NULL;
        NiMaterialResource* pkOuterRadius2 = NULL;
        NiMaterialResource* pkInnerRadius = NULL;
        NiMaterialResource* pkInnerRadius2 = NULL;
        NiMaterialResource* pkKm4PI = NULL;
        NiMaterialResource* pkKr4PI = NULL;
        NiMaterialResource* pkKmESun = NULL;
        NiMaterialResource* pkKrESun = NULL;
        NiMaterialResource* pkRGBInvWavelength4 = NULL;
        NiMaterialResource* pkUpVector = NULL;
        NiMaterialResource* pkfNumSamples = NULL;
        NiMaterialResource* pkiNumSamples = NULL;

        // Setup the required constants
        if (!SetupAtmosphericConstants(kContext, &pkHDRExposure,
            &pkPhaseConstant, &pkPhaseConstant2, &pkCameraHeight, 
            &pkCameraHeight2, &pkScale, &pkScaleDepth, &pkScaleOverScaleDepth,
            &pkSunSizeMultiplier, &pkOuterRadius, &pkOuterRadius2, 
            &pkInnerRadius, &pkInnerRadius2, &pkKm4PI, &pkKr4PI, &pkKmESun, 
            &pkKrESun, &pkRGBInvWavelength4, &pkUpVector, &pkfNumSamples, 
            &pkiNumSamples))
        {
            return false;
        }

        // Perform scattering calculation
        if (!HandleAtmosphericScatteringCalculation(kContext,
            // Calculation parameters
            pkWorldView, 
            // Shader constants
            pkPhaseConstant, pkPhaseConstant2, pkCameraHeight, pkCameraHeight2,
            pkScale, pkScaleDepth, pkScaleOverScaleDepth, pkSunSizeMultiplier,
            pkOuterRadius, pkOuterRadius2, pkInnerRadius, pkInnerRadius2,
            pkKm4PI, pkKr4PI, pkKmESun, pkKrESun,
            pkRGBInvWavelength4, pkUpVector, pkfNumSamples, pkiNumSamples,
            // Calculation Outputs
            pkRayleighScattering, pkMieScattering))
        {
            return false;
        }

    }
    else if (uiAtmosphericCalcMode == AtmosphericCalcMode::CPU)
    {
        // Pass through the results of the calculations from CPU
        EE_FAIL("CPU Atmospheric calculations not supported yet");
    }
    EE_ASSERT(pkRayleighScattering);
    EE_ASSERT(pkMieScattering);

    // Bind vertex shader outputs
    NiMaterialResource* pkVertOutRayleigh = 
        kContext.m_spOutputs->AddInputResource("float3", "TexCoord", 
        "World", "RayleighScattering");
    NiMaterialResource* pkVertOutMie = 
        kContext.m_spOutputs->AddInputResource("float3", "TexCoord", 
        "World", "MieScattering");

    kContext.m_spConfigurator->AddBinding(pkRayleighScattering, 
        pkVertOutRayleigh);
    kContext.m_spConfigurator->AddBinding(pkMieScattering, 
        pkVertOutMie);

    return true;
}

//----------------------------------------------------------------------------
bool NiSkyMaterial::HandlePixelAtmosphericScattering(Context& kContext,
    NiSkyMaterialPixelDescriptor* pkPixelDesc,
    NiMaterialResource*& pkCurrentColor, NiMaterialResource* pkWorldView)
{
    NiUInt32 uiAtmosphericCalcMode = pkPixelDesc->GetATMOSPHERE_CALC_MODE();
    if (uiAtmosphericCalcMode == AtmosphericCalcMode::NONE) return true;

    // Grab references to the relevant shader constants
    NiMaterialResource* pkHDRExposure = NULL;
    NiMaterialResource* pkPhaseConstant = NULL;
    NiMaterialResource* pkPhaseConstant2 = NULL;
    NiMaterialResource* pkCameraHeight = NULL;
    NiMaterialResource* pkCameraHeight2 = NULL;
    NiMaterialResource* pkScale = NULL;
    NiMaterialResource* pkScaleDepth = NULL;
    NiMaterialResource* pkScaleOverScaleDepth = NULL;
    NiMaterialResource* pkSunSizeMultiplier = NULL;

    if (!SetupAtmosphericConstants(kContext, &pkHDRExposure,
        &pkPhaseConstant, &pkPhaseConstant2, &pkCameraHeight, &pkCameraHeight2,
        &pkScale, &pkScaleDepth, &pkScaleOverScaleDepth, &pkSunSizeMultiplier))
    {
        return false;
    }

    // Check references to the required parameters for atmosphere calculations
    EE_ASSERT(pkWorldView);        

    // Fetch or calculate scattering results
    NiMaterialResource* pkRayleighScattering = NULL;
    NiMaterialResource* pkMieScattering = NULL;
    if (uiAtmosphericCalcMode == AtmosphericCalcMode::GPU_PS)
    {
        // Perform scattering calculations here in the pixel shader
        // Setup the rest of the variables
        NiMaterialResource* pkOuterRadius = NULL;
        NiMaterialResource* pkOuterRadius2 = NULL;
        NiMaterialResource* pkInnerRadius = NULL;
        NiMaterialResource* pkInnerRadius2 = NULL;
        NiMaterialResource* pkKm4PI = NULL;
        NiMaterialResource* pkKr4PI = NULL;
        NiMaterialResource* pkKmESun = NULL;
        NiMaterialResource* pkKrESun = NULL;
        NiMaterialResource* pkRGBInvWavelength4 = NULL;
        NiMaterialResource* pkUpVector = NULL;
        NiMaterialResource* pkfNumSamples = NULL;
        NiMaterialResource* pkiNumSamples = NULL;

        // Setup the rest of the constants
        if (!SetupAtmosphericConstants(kContext, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
            &pkOuterRadius, &pkOuterRadius2, &pkInnerRadius, &pkInnerRadius2,
            &pkKm4PI, &pkKr4PI, &pkKmESun, &pkKrESun, 
            &pkRGBInvWavelength4, &pkUpVector, &pkfNumSamples, &pkiNumSamples))
        {
            return false;
        }

        // Perform scattering calculation
        if (!HandleAtmosphericScatteringCalculation(kContext,
            // Calculation parameters
            pkWorldView, 
            // Shader constants
            pkPhaseConstant, pkPhaseConstant2, pkCameraHeight, pkCameraHeight2,
            pkScale, pkScaleDepth, pkScaleOverScaleDepth, pkSunSizeMultiplier,
            pkOuterRadius, pkOuterRadius2, pkInnerRadius, pkInnerRadius2,
            pkKm4PI, pkKr4PI, pkKmESun, pkKrESun,
            pkRGBInvWavelength4, pkUpVector, pkfNumSamples, pkiNumSamples,
            // Calculation Outputs
            pkRayleighScattering, pkMieScattering))
        {
            return false;
        }
    }
    else
    {
        // Collect the calculations output or passed on from the vertex shader
        pkRayleighScattering = kContext.m_spInputs->AddOutputResource("float3", 
            "TexCoord", "World", "RayleighScattering");
        EE_ASSERT(pkWorldView);

        pkMieScattering = kContext.m_spInputs->AddOutputResource("float3", 
            "TexCoord", "World", "MieScattering");
        EE_ASSERT(pkMieScattering);
    }
    EE_ASSERT(pkRayleighScattering);
    EE_ASSERT(pkMieScattering);

    // Perform final scattering calculation using phase functions
    if (!HandleAtmosphericColoring(kContext, 
        pkWorldView, pkRayleighScattering, pkMieScattering, pkPhaseConstant, 
        pkPhaseConstant2, pkSunSizeMultiplier, pkHDRExposure, pkCurrentColor))
    {
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------
bool NiSkyMaterial::HandleAtmosphericScatteringCalculation(Context& kContext,
    NiMaterialResource* pkWorldView, NiMaterialResource* pkPhaseConstant, 
    NiMaterialResource* pkPhaseConstant2, NiMaterialResource* pkCameraHeight, 
    NiMaterialResource* pkCameraHeight2, NiMaterialResource* pkScale, 
    NiMaterialResource* pkScaleDepth, 
    NiMaterialResource* pkScaleOverScaleDepth, 
    NiMaterialResource* pkSunSizeMultiplier, NiMaterialResource* pkOuterRadius,
    NiMaterialResource* pkOuterRadius2, NiMaterialResource* pkInnerRadius, 
    NiMaterialResource* pkInnerRadius2, NiMaterialResource* pkKm4PI, 
    NiMaterialResource* pkKr4PI, NiMaterialResource* pkKmESun, 
    NiMaterialResource* pkKrESun, NiMaterialResource* pkRGBInvWavelength4, 
    NiMaterialResource* pkUpVector, NiMaterialResource* pkfNumSamples, 
    NiMaterialResource* pkiNumSamples, 
    NiMaterialResource*& pkRayleighScattering, 
    NiMaterialResource*& pkMieScattering)
{
    EE_UNUSED_ARG(pkCameraHeight);

    // Check required inputs
    EE_ASSERT(pkWorldView);
    EE_ASSERT(pkPhaseConstant);
    EE_ASSERT(pkPhaseConstant2);
    EE_ASSERT(pkCameraHeight);
    EE_ASSERT(pkCameraHeight2);
    EE_ASSERT(pkScale);
    EE_ASSERT(pkScaleDepth);
    EE_ASSERT(pkScaleOverScaleDepth);
    EE_ASSERT(pkSunSizeMultiplier);
    EE_ASSERT(pkOuterRadius);
    EE_ASSERT(pkOuterRadius2);
    EE_ASSERT(pkInnerRadius);
    EE_ASSERT(pkInnerRadius2);
    EE_ASSERT(pkKm4PI);
    EE_ASSERT(pkKr4PI);
    EE_ASSERT(pkKmESun);
    EE_ASSERT(pkKrESun);
    EE_ASSERT(pkRGBInvWavelength4);
    EE_ASSERT(pkUpVector);
    EE_ASSERT(pkfNumSamples);
    EE_ASSERT(pkiNumSamples);

    // Fetch a resource to the direction of the sun
    NiMaterialResource* pkSunDirection = 
        AddOutputObject(kContext.m_spUniforms, 
        NiShaderConstantMap::SCM_OBJ_WORLDDIRECTION, 
        NiShaderAttributeDesc::OT_EFFECT_DIRECTIONALLIGHT, 0, "Dir");

    // Calculate the distance to the edge of the atmosphere
    NiMaterialNode* pkCalcAtmospherePathFrag = 
        GetAttachableNodeFromLibrary("CalcPathThroughAtmosphere");
    EE_ASSERT(pkCalcAtmospherePathFrag);
    kContext.m_spConfigurator->AddNode(pkCalcAtmospherePathFrag);

    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkWorldView,
        "WorldView", pkCalcAtmospherePathFrag);
    kContext.m_spConfigurator->AddBinding(pkUpVector,
        "upVector", pkCalcAtmospherePathFrag);
    kContext.m_spConfigurator->AddBinding(pkOuterRadius,
        "RA", pkCalcAtmospherePathFrag);
    kContext.m_spConfigurator->AddBinding(pkOuterRadius2,
        "RA2", pkCalcAtmospherePathFrag);
    kContext.m_spConfigurator->AddBinding(pkInnerRadius,
        "RE", pkCalcAtmospherePathFrag);
    kContext.m_spConfigurator->AddBinding(pkInnerRadius2,
        "RE2", pkCalcAtmospherePathFrag);

    // Bind Outputs
    NiMaterialResource *pkCameraPos = pkCalcAtmospherePathFrag->
        GetOutputResourceByVariableName("CameraPos");
    NiMaterialResource *pkPathCameraHeight = pkCalcAtmospherePathFrag->
        GetOutputResourceByVariableName("fCameraHeight");
    NiMaterialResource *pkNear = pkCalcAtmospherePathFrag->
        GetOutputResourceByVariableName("fNear");
    NiMaterialResource *pkFar = pkCalcAtmospherePathFrag->
        GetOutputResourceByVariableName("fFar");

    EE_ASSERT(pkCameraPos);
    EE_ASSERT(pkPathCameraHeight);
    EE_ASSERT(pkNear);
    EE_ASSERT(pkFar);

    // Calculate the scattering over this path
    NiMaterialNode* pkCalcScatteringValuesFrag = 
        GetAttachableNodeFromLibrary("CalcAtmosphericScatteringValues");
    EE_ASSERT(pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddNode(pkCalcScatteringValuesFrag);

    // Bind Parameters
    kContext.m_spConfigurator->AddBinding(pkCameraPos,
        "CameraPos", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkWorldView,
        "WorldView", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkNear,
        "fNear", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkFar,
        "fFar", pkCalcScatteringValuesFrag);
    
    // Bind Constants
    kContext.m_spConfigurator->AddBinding(pkPhaseConstant,
        "fG", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkPhaseConstant2,
        "fG2", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkPathCameraHeight,
        "fCameraHeight", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkCameraHeight2,
        "fCameraHeight2", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkScale,
        "fScale", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkScaleDepth,
        "fScaleDepth", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkScaleOverScaleDepth,
        "fScaleOverScaleDepth", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkSunSizeMultiplier,
        "fSunSizeMultiplier", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkOuterRadius,
        "fOuterRadius", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkOuterRadius2,
        "fOuterRadius2", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkInnerRadius,
        "fInnerRadius", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkInnerRadius2,
        "fInnerRadius2", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkKm4PI,
        "fKm4PI", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkKr4PI,
        "fKr4PI", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkKmESun,
        "fKmESun", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkKrESun,
        "fKrESun", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkiNumSamples,
        "nSamples", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkfNumSamples,
        "fSamples", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkRGBInvWavelength4,
        "RGBInvWavelength4", pkCalcScatteringValuesFrag);
    kContext.m_spConfigurator->AddBinding(pkSunDirection,
        "SunDirection", pkCalcScatteringValuesFrag);

    // Bind Outputs
    pkRayleighScattering = pkCalcScatteringValuesFrag->
        GetOutputResourceByVariableName("RayleighRGBScattering");
    pkMieScattering = pkCalcScatteringValuesFrag->
        GetOutputResourceByVariableName("MieRGBScattering");

    EE_ASSERT(pkRayleighScattering);
    EE_ASSERT(pkMieScattering);

    // Attach a fake node instance of CalcOpticalDepth so that it may be
    // called from within the loop in CalcAtmosphericScatteringValues
    {
        NiMaterialNode* pkFakeCalcOpticalDepth = 
            GetAttachableNodeFromLibrary("CalcOpticalDepth");
        EE_ASSERT(pkFakeCalcOpticalDepth);
        kContext.m_spConfigurator->AddNode(pkFakeCalcOpticalDepth);

        // Bind fake inputs
        kContext.m_spConfigurator->AddBinding(pkScale,
            "fCos", pkFakeCalcOpticalDepth);
        kContext.m_spConfigurator->AddBinding(pkScale,
            "fScaleDepth", pkFakeCalcOpticalDepth);
        kContext.m_spConfigurator->AddBinding(pkScale,
            "fScaleOverScaleDepth", pkFakeCalcOpticalDepth);
        kContext.m_spConfigurator->AddBinding(pkScale,
            "fHeightFromSurface", pkFakeCalcOpticalDepth);

        // Bind fake outputs
        kContext.m_spConfigurator->AddBinding(
            "depth", pkFakeCalcOpticalDepth,
            "FakeDepthValue", pkCalcScatteringValuesFrag);
    }

    return true;
}

//----------------------------------------------------------------------------
bool NiSkyMaterial::HandleAtmosphericColoring(Context& kContext, 
        NiMaterialResource* pkWorldView, 
        NiMaterialResource* pkRayleighScattering, 
        NiMaterialResource* pkMieScattering, 
        NiMaterialResource* pkPhaseConstant, 
        NiMaterialResource* pkPhaseConstant2, 
        NiMaterialResource* pkSunSizeMultiplier, 
        NiMaterialResource* pkHDRExposure, 
        NiMaterialResource*& pkOutputColor)
{
    // Check Parameters
    EE_ASSERT(pkWorldView);
    EE_ASSERT(pkRayleighScattering);
    EE_ASSERT(pkMieScattering);
    EE_ASSERT(pkPhaseConstant);
    EE_ASSERT(pkPhaseConstant2);
    EE_ASSERT(pkSunSizeMultiplier);
    EE_ASSERT(pkHDRExposure);

    // Fetch a resource to the direction of the sun
    NiMaterialResource* pkSunDirection = 
        AddOutputObject(kContext.m_spUniforms, 
        NiShaderConstantMap::SCM_OBJ_WORLDDIRECTION, 
        NiShaderAttributeDesc::OT_EFFECT_DIRECTIONALLIGHT, 0, "Dir");

    // Calculate the required sky color based on the scattering
    NiMaterialNode* pkSkyColorFrag = 
        GetAttachableNodeFromLibrary("CalcAtmosphericColor");
    EE_ASSERT(pkSkyColorFrag);
    kContext.m_spConfigurator->AddNode(pkSkyColorFrag);

    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkWorldView,
        "WorldView", pkSkyColorFrag);
    kContext.m_spConfigurator->AddBinding(pkSunDirection,
        "SunDirection", pkSkyColorFrag);
    kContext.m_spConfigurator->AddBinding(pkRayleighScattering,
        "RayleighRGBScattering", pkSkyColorFrag);
    kContext.m_spConfigurator->AddBinding(pkMieScattering,
        "MieRGBScattering", pkSkyColorFrag);
    kContext.m_spConfigurator->AddBinding(pkPhaseConstant,
        "g", pkSkyColorFrag);
    kContext.m_spConfigurator->AddBinding(pkPhaseConstant2,
        "g2", pkSkyColorFrag);
    kContext.m_spConfigurator->AddBinding(pkSunSizeMultiplier,
        "fSunSizeMultiplier", pkSkyColorFrag);

    // Bind Outputs
    pkOutputColor = pkSkyColorFrag->GetOutputResourceByVariableName("result");
    EE_ASSERT(pkOutputColor);

    // Perform HDR Tone mapping on the output color
    NiMaterialNode* pkHDRToneMapFrag = 
        GetAttachableNodeFromLibrary("HDRToneMapping");
    EE_ASSERT(pkHDRToneMapFrag);
    kContext.m_spConfigurator->AddNode(pkHDRToneMapFrag);

    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkOutputColor,
        "inColor", pkHDRToneMapFrag);
    kContext.m_spConfigurator->AddBinding(pkHDRExposure,
        "HDRExposure", pkHDRToneMapFrag);

    // Bind Outputs
    pkOutputColor = pkHDRToneMapFrag->
        GetOutputResourceByVariableName("outColor");
    EE_ASSERT(pkOutputColor);

    return true;
}

//----------------------------------------------------------------------------
bool NiSkyMaterial::SetupAtmosphericConstants(Context& kContext,
    NiMaterialResource** ppkHDRExposure,
    NiMaterialResource** ppkPhaseConstant,
    NiMaterialResource** ppkPhaseConstant2,
    NiMaterialResource** ppkCameraHeight,
    NiMaterialResource** ppkCameraHeight2,
    NiMaterialResource** ppkScale,
    NiMaterialResource** ppkScaleDepth,
    NiMaterialResource** ppkScaleOverScaleDepth,
    NiMaterialResource** ppkSunSizeMultiplier,
    NiMaterialResource** ppkOuterRadius,
    NiMaterialResource** ppkOuterRadius2,
    NiMaterialResource** ppkInnerRadius,
    NiMaterialResource** ppkInnerRadius2,
    NiMaterialResource** ppkKm4PI,
    NiMaterialResource** ppkKr4PI,
    NiMaterialResource** ppkKmESun,
    NiMaterialResource** ppkKrESun,
    NiMaterialResource** ppkRGBInvWavelength4,
    NiMaterialResource** ppkUpVector,
    NiMaterialResource** ppkfNumSamples,
    NiMaterialResource** ppkiNumSamples
    )
{
    // Fetch the HDR Exposure constant
    if (ppkHDRExposure)
    {
        *ppkHDRExposure = AddOutputAttribute(
            kContext.m_spUniforms, SC_HDREXPOSURE, 
            NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
        EE_ASSERT(*ppkHDRExposure);
    }

    // Split frame data
    if (ppkPhaseConstant && ppkPhaseConstant2 && ppkCameraHeight && 
        ppkCameraHeight2)
    {
        // Acquire frame data constant
        NiMaterialResource* pkFrameData = AddOutputAttribute(
            kContext.m_spUniforms, SC_FRAMEDATA, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);

        // Fetch the node to do the split
        NiMaterialNode* pkSplitFrameDataFrag = 
            GetAttachableNodeFromLibrary("SplitFrameData");
        EE_ASSERT(pkSplitFrameDataFrag);
        kContext.m_spConfigurator->AddNode(pkSplitFrameDataFrag);

        // Assign inputs
        kContext.m_spConfigurator->AddBinding(pkFrameData, 
            "scFrameData", pkSplitFrameDataFrag);

        // Assign outputs
        *ppkPhaseConstant = pkSplitFrameDataFrag->
            GetOutputResourceByVariableName("fPhaseConstant");
        *ppkPhaseConstant2 = pkSplitFrameDataFrag->
            GetOutputResourceByVariableName("fPhaseConstant2");
        *ppkCameraHeight = pkSplitFrameDataFrag->
            GetOutputResourceByVariableName("fCameraHeight");
        *ppkCameraHeight2 = pkSplitFrameDataFrag->
            GetOutputResourceByVariableName("fCameraHeight2");

        // Make sure these variables were mapped
        EE_ASSERT(*ppkPhaseConstant);
        EE_ASSERT(*ppkPhaseConstant2);
        EE_ASSERT(*ppkCameraHeight);
        EE_ASSERT(*ppkCameraHeight2);
    }

    // Split atmospheric scale depth
    if (ppkScale && ppkScaleDepth && ppkScaleOverScaleDepth && 
        ppkSunSizeMultiplier)
    {
        // Acquire scale depth constant
        NiMaterialResource* pkScaleDepth = AddOutputAttribute(
            kContext.m_spUniforms, SC_SCALEDEPTH, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);

        // Fetch the node to do the split
        NiMaterialNode* pkSplitScaleDepthFrag = 
            GetAttachableNodeFromLibrary("SplitAtmosphericScaleDepth");
        EE_ASSERT(pkSplitScaleDepthFrag);
        kContext.m_spConfigurator->AddNode(pkSplitScaleDepthFrag);

        // Assign inputs
        kContext.m_spConfigurator->AddBinding(pkScaleDepth, 
            "scAtmosphericScaleDepth", pkSplitScaleDepthFrag);

        // Assign outputs
        *ppkScale = pkSplitScaleDepthFrag->
            GetOutputResourceByVariableName("fScale");
        *ppkScaleDepth = pkSplitScaleDepthFrag->
            GetOutputResourceByVariableName("fScaleDepth");
        *ppkScaleOverScaleDepth = pkSplitScaleDepthFrag->
            GetOutputResourceByVariableName("fScaleOverScaleDepth");
        *ppkSunSizeMultiplier = pkSplitScaleDepthFrag->
            GetOutputResourceByVariableName("fSunSizeMultiplier");

        // Make sure these variables were mapped
        EE_ASSERT(*ppkScale);
        EE_ASSERT(*ppkScaleDepth);
        EE_ASSERT(*ppkScaleOverScaleDepth);
        EE_ASSERT(*ppkSunSizeMultiplier);
    }

    // Split planet dimensions
    if (ppkOuterRadius && ppkOuterRadius2 && ppkInnerRadius && 
        ppkInnerRadius2)
    {
        // Acquire frame data constant
        NiMaterialResource* pkPlanetDimensions = AddOutputAttribute(
            kContext.m_spUniforms, SC_PLANETDIMENSIONS, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);

        // Fetch the node to do the split
        NiMaterialNode* pkSplitPlanetDimensionsFrag = 
            GetAttachableNodeFromLibrary("SplitPlanetDimensions");
        EE_ASSERT(pkSplitPlanetDimensionsFrag);
        kContext.m_spConfigurator->AddNode(pkSplitPlanetDimensionsFrag);

        // Assign inputs
        kContext.m_spConfigurator->AddBinding(pkPlanetDimensions, 
            "scPlanetDimensions", pkSplitPlanetDimensionsFrag);

        // Assign outputs
        *ppkOuterRadius = pkSplitPlanetDimensionsFrag->
            GetOutputResourceByVariableName("fOuterRadius");
        *ppkOuterRadius2 = pkSplitPlanetDimensionsFrag->
            GetOutputResourceByVariableName("fOuterRadius2");
        *ppkInnerRadius = pkSplitPlanetDimensionsFrag->
            GetOutputResourceByVariableName("fInnerRadius");
        *ppkInnerRadius2 = pkSplitPlanetDimensionsFrag->
            GetOutputResourceByVariableName("fInnerRadius2");

        // Make sure these variables were mapped
        EE_ASSERT(*ppkOuterRadius);
        EE_ASSERT(*ppkOuterRadius2);
        EE_ASSERT(*ppkInnerRadius);
        EE_ASSERT(*ppkInnerRadius2);
    }

    // Split scattering constants
    if (ppkKm4PI && ppkKr4PI && ppkKmESun && ppkKrESun)
    {
        // Acquire frame data constant
        NiMaterialResource* pkScatteringConsts = AddOutputAttribute(
            kContext.m_spUniforms, SC_ATMOSPHERICSCATTERINGCONST, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);

        // Fetch the node to do the split
        NiMaterialNode* pkSplitScatteringConstFrag = 
            GetAttachableNodeFromLibrary("SplitAtmosphericScatteringConsts");
        EE_ASSERT(pkSplitScatteringConstFrag);
        kContext.m_spConfigurator->AddNode(pkSplitScatteringConstFrag);

        // Assign inputs
        kContext.m_spConfigurator->AddBinding(pkScatteringConsts, 
            "scAtmosphericScatteringConsts", pkSplitScatteringConstFrag);

        // Assign outputs
        *ppkKm4PI = pkSplitScatteringConstFrag->
            GetOutputResourceByVariableName("fKm4PI");
        *ppkKr4PI = pkSplitScatteringConstFrag->
            GetOutputResourceByVariableName("fKr4PI");
        *ppkKmESun = pkSplitScatteringConstFrag->
            GetOutputResourceByVariableName("fKmESun");
        *ppkKrESun = pkSplitScatteringConstFrag->
            GetOutputResourceByVariableName("fKrESun");

        // Make sure these variables were mapped
        EE_ASSERT(*ppkKm4PI);
        EE_ASSERT(*ppkKr4PI);
        EE_ASSERT(*ppkKmESun);
        EE_ASSERT(*ppkKrESun);
    }

    // Fetch the RGBInvWavelength constant
    if (ppkRGBInvWavelength4)
    {
        *ppkRGBInvWavelength4 = AddOutputAttribute(
            kContext.m_spUniforms, SC_RGBINVWAVELENGTH4, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT3);
        EE_ASSERT(*ppkRGBInvWavelength4);
    }

    // Fetch the up vector
    if (ppkUpVector)
    {
        *ppkUpVector = AddOutputAttribute(
            kContext.m_spUniforms, SC_UPVECTOR, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT3);
        EE_ASSERT(*ppkUpVector);
    }

    // Fetch the number of samples as a float
    if (ppkfNumSamples)
    {
        *ppkfNumSamples = AddOutputAttribute(
            kContext.m_spUniforms, SC_NUMSAMPLESFLOAT, 
            NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
        EE_ASSERT(*ppkfNumSamples);
    }

    // Fetch the number of samples as a integer
    if (ppkiNumSamples)
    {
        *ppkiNumSamples = AddOutputAttribute(
            kContext.m_spUniforms, SC_NUMSAMPLESINT, 
            NiShaderAttributeDesc::ATTRIB_TYPE_UNSIGNEDINT);
        EE_ASSERT(*ppkiNumSamples);
    }

    return true;
}

//---------------------------------------------------------------------------
bool NiSkyMaterial::HandleBlendStages(Context& kContext, 
    NiSkyMaterialPixelDescriptor* pkPixelDesc,
    NiMaterialResource*& pkCurrentColor, NiMaterialResource* pkNormal,
    NiMaterialResource* pkWorldView)
{
    for (NiUInt32 uiStageIndex = 0; uiStageIndex < NUM_BLEND_STAGES; 
        ++uiStageIndex)
    {
        EE_ASSERT(pkCurrentColor);

        NiUInt32 uiColorMap;
        NiUInt32 uiModifierSource;
        NiUInt32 uiBlendMethod;

        // Fetch the settings for this stage:
        pkPixelDesc->GetStageConfiguration(uiStageIndex, uiColorMap, 
            uiModifierSource, uiBlendMethod);
        if (uiModifierSource == ModifierSource::DISABLED)
            continue;

        // Handle selection of the color to blend with
        NiMaterialResource* pkSampleColor = SetupSampleColorMap(kContext, 
            ColorMap::Value(uiColorMap), uiStageIndex, pkPixelDesc, 
            pkCurrentColor, pkNormal, pkWorldView);

        // Calculate a modifier value
        NiMaterialResource* pkModifier = NULL;
        pkModifier = SetupCalcModifier(kContext, 
            ModifierSource::Value(uiModifierSource), uiStageIndex, pkPixelDesc,
            pkCurrentColor, pkSampleColor, pkNormal, pkWorldView);

        // Apply a blend method:
        NiMaterialNode* pkBlendFrag = SetupBlendMethod(kContext, 
            BlendMethod::Value(uiBlendMethod), uiStageIndex, pkPixelDesc, 
            pkCurrentColor, pkNormal, pkWorldView);

        // Assign standard inputs
        kContext.m_spConfigurator->AddBinding(pkCurrentColor,
            "ValueA", pkBlendFrag);
        kContext.m_spConfigurator->AddBinding(pkSampleColor,
            "ValueB", pkBlendFrag);

        // Assign optional multiplier
        NiMaterialResource* pkModifierInput = pkBlendFrag->
            GetInputResourceByVariableName("Modifier");
        if (pkModifier && pkModifierInput)
        {
            kContext.m_spConfigurator->AddBinding(pkModifier,
                pkModifierInput);
        }
        
        // Assign standard outputs
        NiMaterialResource* pkBlendColor = pkBlendFrag->
            GetOutputResourceByVariableName("OutValue");

        pkCurrentColor = pkBlendColor;
    }

    return true;
}

//---------------------------------------------------------------------------
NiMaterialResource* NiSkyMaterial::SetupSampleColorMap(Context& kContext,
    ColorMap::Value kColorMap, NiUInt32 uiStageIndex,
    NiSkyMaterialPixelDescriptor* pkPixelDesc, 
    NiMaterialResource*& pkCurrentColor, NiMaterialResource* pkNormal,
    NiMaterialResource* pkWorldView)
{
    NiMaterialResource* pkOutColor = NULL;

    switch (kColorMap)
    {
    case ColorMap::SKYBOX:
        {
            // Create the appropriate sampler
            NiFixedString kSamplerName = GenerateConstantName(uiStageIndex, 
                "SkyCubeMap");
            NiMaterialResource* pkSampler = AddTextureSampler(kContext, 
                kSamplerName, uiStageIndex);
            EE_ASSERT(pkSampler);
            EE_ASSERT(pkNormal);

            // Fetch the node to do sampling
            NiMaterialNode* pkSampleCubeMapFrag = 
                GetAttachableNodeFromLibrary("SampleCubeMap");
            EE_ASSERT(pkSampleCubeMapFrag);
            kContext.m_spConfigurator->AddNode(pkSampleCubeMapFrag);

            // Assign inputs
            kContext.m_spConfigurator->AddBinding(pkSampler,
                "Sampler", pkSampleCubeMapFrag);
            kContext.m_spConfigurator->AddBinding(pkNormal,
                "SamplePoint", pkSampleCubeMapFrag);
            
            // Assign outputs
            pkOutColor = pkSampleCubeMapFrag->
                GetOutputResourceByVariableName("OutColor");
        }
        break;

    case ColorMap::ORIENTED_SKYBOX:
        {
            // Create the appropriate sampler
            NiFixedString kSamplerName = GenerateConstantName(uiStageIndex, 
                "SkyCubeMap");
            NiFixedString kUpVectorName = GenerateConstantName(uiStageIndex, 
                SC_STAGESKYBOXORIENTATION);

            NiMaterialResource* pkSampler = AddTextureSampler(kContext, 
                kSamplerName, uiStageIndex);
            EE_ASSERT(pkSampler);
            NiMaterialResource* pkTransform = AddOutputAttribute(
                kContext.m_spUniforms, kUpVectorName, 
                NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4);
            EE_ASSERT(pkTransform);

            // Rotate the normal
            NiMaterialResource* pkTransformedNormal = NULL;
            NiMaterialNode* pkTransformFrag =
                GetAttachableNodeFromLibrary("TransformDirection");
            EE_ASSERT(pkTransformFrag);
            kContext.m_spConfigurator->AddNode(pkTransformFrag);

            kContext.m_spConfigurator->AddBinding(pkNormal,
                "OriginalDir", pkTransformFrag);
            kContext.m_spConfigurator->AddBinding(pkTransform,
                "TransformMatrix", pkTransformFrag);
            pkTransformedNormal = pkTransformFrag->
                GetOutputResourceByVariableName("TransformDir");

            // Fetch the node to do sampling
            NiMaterialNode* pkSampleCubeMapFrag = 
                GetAttachableNodeFromLibrary("SampleCubeMap");
            EE_ASSERT(pkSampleCubeMapFrag);
            kContext.m_spConfigurator->AddNode(pkSampleCubeMapFrag);

            // Assign inputs
            kContext.m_spConfigurator->AddBinding(pkSampler,
                "Sampler", pkSampleCubeMapFrag);
            kContext.m_spConfigurator->AddBinding(pkTransformedNormal,
                "SamplePoint", pkSampleCubeMapFrag);

            // Assign outputs
            pkOutColor = pkSampleCubeMapFrag->
                GetOutputResourceByVariableName("OutColor");
        }
        break;

    case ColorMap::GRADIENT:
        {
            // Acquire the necessary parameters
            NiMaterialResource* pkUpVector = AddOutputAttribute(
                kContext.m_spUniforms, SC_UPVECTOR, 
                NiShaderAttributeDesc::ATTRIB_TYPE_POINT3);
            NiFixedString kExponentName = 
                GenerateConstantName(uiStageIndex, 
                SC_STAGEGRADIENTEXPONENT);
            NiMaterialResource* pkExponent = AddOutputAttribute(
                kContext.m_spUniforms, kExponentName, 
                NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
            NiFixedString kHorizonBiasName = 
                GenerateConstantName(uiStageIndex, 
                SC_STAGEGRADIENTHORIZONBIAS);
            NiMaterialResource* pkHorizonBias = AddOutputAttribute(
                kContext.m_spUniforms, kHorizonBiasName, 
                NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);

            // Perform the interpolator calculation
            NiMaterialResource* pkInterpolation = HandleCalculateHorizonBias(
                kContext, pkNormal, pkUpVector, pkExponent, pkHorizonBias);

            // Fetch the two colors to blend:
            NiFixedString kHorizonColorName = 
                GenerateConstantName(uiStageIndex, SC_STAGEGRADIENTHORIZON);
            NiMaterialResource* pkHorizonColor = AddOutputAttribute(
                kContext.m_spUniforms, kHorizonColorName, 
                NiShaderAttributeDesc::ATTRIB_TYPE_COLOR);
            NiFixedString kZenithColorName = 
                GenerateConstantName(uiStageIndex, SC_STAGEGRADIENTZENITH);
            NiMaterialResource* pkZenithColor = AddOutputAttribute(
                kContext.m_spUniforms, kZenithColorName, 
                NiShaderAttributeDesc::ATTRIB_TYPE_COLOR);

            // Blend the two colors together:
            NiMaterialNode* pkGradientFrag = 
                GetAttachableNodeFromLibrary("BlendValueInterpolate");
            EE_ASSERT(pkGradientFrag);
            kContext.m_spConfigurator->AddNode(pkGradientFrag);

            // Bind the inputs
            kContext.m_spConfigurator->AddBinding(pkHorizonColor,
                "ValueA", pkGradientFrag);
            kContext.m_spConfigurator->AddBinding(pkZenithColor,
                "ValueB", pkGradientFrag);
            kContext.m_spConfigurator->AddBinding(pkInterpolation,
                "Modifier", pkGradientFrag);

            // Bind the output
            pkOutColor = pkGradientFrag->GetOutputResourceByVariableName("OutValue");
        }
        break;
    case ColorMap::FOG:
        {
            pkOutColor = AddOutputPredefined(
                kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_FOG_COLOR);
        }
        break;
    case ColorMap::CUSTOM:
    default:
        {
            pkOutColor = HandleCustomColorMap(kContext, uiStageIndex, 
                pkPixelDesc, pkCurrentColor, pkNormal, pkWorldView);
        }
        break;
    }

    EE_ASSERT(pkOutColor);
    return pkOutColor;
}

//---------------------------------------------------------------------------
NiMaterialResource* NiSkyMaterial::HandleCustomColorMap(Context& kContext,
    NiUInt32 uiStageIndex, NiSkyMaterialPixelDescriptor* pkPixelDesc, 
    NiMaterialResource*& pkCurrentColor, NiMaterialResource* pkNormal,
    NiMaterialResource* pkWorldView)
{
    // This function was intentionally implemented blank
    EE_FAIL("Custom color maps are not supported - must subclass to add"
        " this functionality");

    EE_UNUSED_ARG(kContext);
    EE_UNUSED_ARG(uiStageIndex);
    EE_UNUSED_ARG(pkPixelDesc);
    EE_UNUSED_ARG(pkCurrentColor);
    EE_UNUSED_ARG(pkNormal);
    EE_UNUSED_ARG(pkWorldView);

    return NULL;
}

//---------------------------------------------------------------------------
NiMaterialResource* NiSkyMaterial::SetupCalcModifier(Context& kContext,
    ModifierSource::Value kModifierSource, NiUInt32 uiStageIndex, 
    NiSkyMaterialPixelDescriptor* pkPixelDesc, 
    NiMaterialResource*& pkCurrentColor, NiMaterialResource* pkSampleColor,
    NiMaterialResource* pkNormal, NiMaterialResource* pkWorldView)
{
    NiMaterialResource* pkModifier = NULL;

    switch (kModifierSource)
    {
    case ModifierSource::DEFAULT:
        {
            // Force the use of the default modifier values 
            // this may allow shader optimisation by the compiler.
            pkModifier = NULL;
        }
        break;
    case ModifierSource::ALPHA:
        {
            // Fetch the alpha extraction node
            NiMaterialNode* pkExtractAlpha = 
                GetAttachableNodeFromLibrary("ExtractAlphaValue");
            EE_ASSERT(pkExtractAlpha);
            kContext.m_spConfigurator->AddNode(pkExtractAlpha);
    
            // Bind input
            kContext.m_spConfigurator->AddBinding(pkSampleColor,
                "Value", pkExtractAlpha);

            // Bind output
            pkModifier = pkExtractAlpha->
                GetOutputResourceByVariableName("OutValue");
        }
        break;
    case ModifierSource::CONSTANT:
        {
            NiFixedString kConstantName = 
                GenerateConstantName(uiStageIndex, SC_STAGEMODIFIERCONSTANT);
            pkModifier = AddOutputAttribute(
                kContext.m_spUniforms, kConstantName, 
                NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
        }
        break;
    case ModifierSource::HORIZONBIAS:
        {
            // Acquire the necessary parameters
            NiMaterialResource* pkUpVector = AddOutputAttribute(
                kContext.m_spUniforms, SC_UPVECTOR, 
                NiShaderAttributeDesc::ATTRIB_TYPE_POINT3);
            NiFixedString kExponentName = 
                GenerateConstantName(uiStageIndex, SC_STAGEMODIFIEREXPONENT);
            NiMaterialResource* pkExponent = AddOutputAttribute(
                kContext.m_spUniforms, kExponentName, 
                NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
            NiFixedString kHorizonBiasName = 
                GenerateConstantName(uiStageIndex, 
                SC_STAGEMODIFIERHORIZONBIAS);
            NiMaterialResource* pkHorizonBias = AddOutputAttribute(
                kContext.m_spUniforms, kHorizonBiasName, 
                NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);

            // Perform the calculation
            pkModifier = HandleCalculateHorizonBias(kContext, pkNormal,
                pkUpVector, pkExponent, pkHorizonBias);
        }
        break;
    case ModifierSource::CUSTOM:
    default:
        {
            pkModifier = HandleCustomModifier(kContext, uiStageIndex, 
                pkPixelDesc, pkCurrentColor, pkSampleColor, pkNormal, pkWorldView);
        }
        break;
    }

    return pkModifier;
}

//---------------------------------------------------------------------------
NiMaterialResource* NiSkyMaterial::HandleCustomModifier(Context& kContext,
    NiUInt32 uiStageIndex, NiSkyMaterialPixelDescriptor* pkPixelDesc, 
    NiMaterialResource*& pkCurrentColor, NiMaterialResource* pkSampleColor,
    NiMaterialResource* pkNormal, NiMaterialResource* pkWorldView)
{
    // This function was intentionally implemented blank
    EE_FAIL("Custom modifiers are not supported - must subclass to add"
        " this functionality");

    EE_UNUSED_ARG(kContext);
    EE_UNUSED_ARG(uiStageIndex);
    EE_UNUSED_ARG(pkPixelDesc);
    EE_UNUSED_ARG(pkCurrentColor);
    EE_UNUSED_ARG(pkSampleColor);
    EE_UNUSED_ARG(pkNormal);
    EE_UNUSED_ARG(pkWorldView);

    return NULL;
}

//---------------------------------------------------------------------------
NiMaterialNode* NiSkyMaterial::SetupBlendMethod(Context& kContext,
    BlendMethod::Value kBlendMode, NiUInt32 uiStageIndex,
    NiSkyMaterialPixelDescriptor* pkPixelDesc, 
    NiMaterialResource*& pkCurrentColor, NiMaterialResource* pkNormal, 
    NiMaterialResource* pkWorldView)
{
    // Fetch the node to do sampling
    NiMaterialNode* pkBlendFrag = NULL;

    switch(kBlendMode)
    {
    case BlendMethod::MULTIPLY:
        {
            pkBlendFrag = GetAttachableNodeFromLibrary("BlendValueMultiply");
        }
        break;
    case BlendMethod::ADD:
        {
            pkBlendFrag = GetAttachableNodeFromLibrary("BlendValueAdd");
        }
        break;
    case BlendMethod::INTERPOLATE:
        {
            pkBlendFrag = GetAttachableNodeFromLibrary("BlendValueInterpolate");
        }
        break;
    case BlendMethod::CUSTOM:
    default:
        {
            pkBlendFrag = HandleCustomBlendMethod(kContext, uiStageIndex,
                pkPixelDesc, pkCurrentColor, pkNormal, pkWorldView);
        }
        break;
    }

    EE_ASSERT(pkBlendFrag);
    kContext.m_spConfigurator->AddNode(pkBlendFrag);

    return pkBlendFrag;
}

//---------------------------------------------------------------------------
NiMaterialNode* NiSkyMaterial::HandleCustomBlendMethod(
    NiFragmentMaterial::Context& kContext,
    NiUInt32 uiStageIndex, NiSkyMaterialPixelDescriptor* pkPixelDesc, 
    NiMaterialResource*& pkCurrentColor, NiMaterialResource* pkNormal,
    NiMaterialResource* pkWorldView)
{
    // This function was intentionally implemented blank
    EE_FAIL("Custom Blend methods are not supported - must subclass to add"
        " this functionality");

    EE_UNUSED_ARG(kContext);
    EE_UNUSED_ARG(uiStageIndex);
    EE_UNUSED_ARG(pkPixelDesc);
    EE_UNUSED_ARG(pkCurrentColor);
    EE_UNUSED_ARG(pkNormal);
    EE_UNUSED_ARG(pkWorldView);

    return NULL;
}

//---------------------------------------------------------------------------
NiMaterialResource* NiSkyMaterial::HandleCalculateHorizonBias(
    Context& kContext, NiMaterialResource* pkNormal, 
    NiMaterialResource* pkUpVector, NiMaterialResource* pkExponent, 
    NiMaterialResource* pkHorizonBias)
{
    EE_ASSERT(pkUpVector);
    EE_ASSERT(pkNormal);
    EE_ASSERT(pkExponent);
    EE_ASSERT(pkHorizonBias);

    // Get the node to perform the calculation
    NiMaterialNode* pkHorizonBiasFrag = 
        GetAttachableNodeFromLibrary("CalcHorizonBias");
    EE_ASSERT(pkHorizonBiasFrag);
    kContext.m_spConfigurator->AddNode(pkHorizonBiasFrag);

    // Bind inputs
    kContext.m_spConfigurator->AddBinding(pkNormal,
        "Normal", pkHorizonBiasFrag);
    kContext.m_spConfigurator->AddBinding(pkUpVector,
        "UpVector", pkHorizonBiasFrag);
    kContext.m_spConfigurator->AddBinding(pkExponent,
        "Exponent", pkHorizonBiasFrag);
    kContext.m_spConfigurator->AddBinding(pkHorizonBias,
        "HorizonBias", pkHorizonBiasFrag);

    // return output
    return pkHorizonBiasFrag->GetOutputResourceByVariableName("OutValue");
}

//---------------------------------------------------------------------------
NiFixedString NiSkyMaterial::GenerateConstantName(
    NiUInt32 uiStageIndex, const NiFixedString& baseName) const
{
    char acResult[64];    
    NiSprintf(acResult, 64, "g_Stage%d%s", (NiInt32)uiStageIndex, 
        (const char*)baseName);
    return acResult;
}

//---------------------------------------------------------------------------
NiMaterialResource* NiSkyMaterial::AddTextureSampler(
    NiFragmentMaterial::Context& kContext,
    const NiFixedString& kSamplerName, NiUInt32 uiOccurance)
{
    NiMaterialResource* pkSampler = 
        kContext.m_spUniforms->GetInputResourceByVariableName(kSamplerName);

    if (pkSampler)
    {
        return pkSampler;
    }
    else
    {
        return kContext.m_spUniforms->AddOutputResource("samplerCUBE", "Shader", 
            "", kSamplerName, 1, NiMaterialResource::SOURCE_PREDEFINED, 
            NiShaderAttributeDesc::OT_UNDEFINED, uiOccurance);
    }
}

//---------------------------------------------------------------------------
void NiSkyMaterial::AddDefaultFallbacks()
{
    NiFragmentMaterial::AddDefaultFallbacks();
}

//---------------------------------------------------------------------------
