// GAMEBASE USA LLC PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Gamebase USA LLC and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2011 Gamebase USA LLC.
//      All Rights Reserved.
//
// Gamebase USA LLC, Research Triangle Park, NC 27709
// http://www.gamebryo.com

#include "NiLightPrePassPCH.h"

#include "NiLPPLightMaterial.h"

#include <NiStandardMaterialDescriptor.h>
#include <NiStandardMaterialNodeLibrary.h>
#include <NiStandardPixelProgramDescriptor.h>
#include <NiStandardVertexProgramDescriptor.h>
#include <NiRenderer.h>
#include "NiLightPrePassMaterialNodeLibrary.h"
#include <NiMaterialFragmentNode.h>
#include <NiMaterialNodeLibrary.h>
#include <NiMaterialResource.h>
#include <NiCodeBlock.h>
#include <NiShadowManager.h>
#include <NiPSSMShadowClickGenerator.h>

//------------------------------------------------------------------------------------------------
NiImplementRTTI(NiLPPLightMaterial, NiStandardMaterial, NiTypeMask::NiLPPLightMaterial);
//------------------------------------------------------------------------------------------------
// macros to reduce boilerplate

#define ADDNODE(node) \
    kContext.m_spConfigurator->AddNode(node);

#define BINDINPUT(resource,node,variable) \
    kContext.m_spConfigurator->AddBinding(resource, \
        node->GetInputResourceByVariableName(variable));

#define GETOUTPUT(node,variable) \
    node->GetOutputResourceByVariableName(variable);

//------------------------------------------------------------------------------------------------
NiLPPLightMaterial* NiLPPLightMaterial::Create()
{
    // Fetch a previously generated NiTerrainMaterial
    NiLPPLightMaterial* pkMaterial = NiDynamicCast(NiLPPLightMaterial, 
        NiMaterial::GetMaterial("NiLPPLightMaterial"));

    if (!pkMaterial)
    {
        pkMaterial = NiNew NiLPPLightMaterial();
    }

    return pkMaterial;
}

//------------------------------------------------------------------------------------------------
NiLPPLightMaterial::NiLPPLightMaterial(bool bAutoCreateCaches)
    : NiStandardMaterial("NiLPPLightMaterial", NULL,
                     LPPLightMaterial_VERTEX_VERSION,
                     LPPLightMaterial_GEOMETRY_VERSION,
                     LPPLightMaterial_PIXEL_VERSION, bAutoCreateCaches)
{
    m_bForcePerPixelLighting = true;
    m_kLibraries.Add(NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary());
}

//------------------------------------------------------------------------------------------------
bool NiLPPLightMaterial::SetupTransformPipeline(Context& kContext,
    NiMaterialResource* pkVertOutProjPos,
    NiStandardVertexProgramDescriptor* pkVertDesc, bool bForceView,
    bool bForceViewPos, NiMaterialResource*& pkWorldPos,
    NiMaterialResource*& pkViewPos, NiMaterialResource*& pkProjectedPos,
    NiMaterialResource*& pkWorldNormal, NiMaterialResource*& pkWorldView)
{
    // Force the view position to be generated
    bForceViewPos = true;

    return NiStandardMaterial::SetupTransformPipeline(kContext, pkVertOutProjPos, pkVertDesc,
        bForceView, bForceViewPos,
        pkWorldPos, pkViewPos, pkProjectedPos, pkWorldNormal, pkWorldView);
}

//------------------------------------------------------------------------------------------------
bool NiLPPLightMaterial::HandleFinalVertexOutputs(
    Context& kContext,
    NiStandardVertexProgramDescriptor* pkVertDesc,
    NiMaterialResource* pkWorldPos,
    NiMaterialResource* pkWorldNormal,
    NiMaterialResource* pkWorldReflect,
    NiMaterialResource* pkViewPos,
    NiMaterialResource* pkProjectedPos)
{
    if (!NiStandardMaterial::HandleFinalVertexOutputs(kContext, pkVertDesc,
        pkWorldPos, pkWorldNormal, pkWorldReflect, pkViewPos, pkProjectedPos))
    {
        return false;
    }

    // add projected position as a texture coordinate
    NiMaterialResource* pkVertOutProjTexCoord =
        kContext.m_spOutputs->AddInputResource("float4", "TexCoord", "World",
        "PassThrough");

    NiMaterialNode* pkSplitterNode = GetAttachableNodeFromLibrary("PositionPassThrough");
    kContext.m_spConfigurator->AddNode(pkSplitterNode);
    kContext.m_spConfigurator->AddBinding(pkProjectedPos,
        pkSplitterNode->GetInputResourceByVariableName("Input"));
    kContext.m_spConfigurator->AddBinding(
        pkSplitterNode->GetOutputResourceByVariableName("Output"), pkVertOutProjTexCoord);

    // add view pos
    NiMaterialResource* pkViewPosTexCoord =
        kContext.m_spOutputs->AddInputResource("float4", "TexCoord", "View",
        "ViewPos");
    kContext.m_spConfigurator->AddBinding(
        pkViewPos, pkViewPosTexCoord);

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiLPPLightMaterial::GeneratePixelShadeTree(Context& kContext,
    NiGPUProgramDescriptor* pkDesc)
{
    // 1. Setup from NiStandardMaterial

    EE_ASSERT(pkDesc->m_kIdentifier == "NiStandardPixelProgramDescriptor");
    NiStandardPixelProgramDescriptor* pkPixelDesc =
        (NiStandardPixelProgramDescriptor*)pkDesc;

    kContext.m_spConfigurator->SetDescription(pkPixelDesc->ToString());

    // Add pixel in, pixel out, constants, and uniforms
    if (!AddDefaultMaterialNodes(kContext, pkDesc,
        NiGPUProgram::PROGRAM_PIXEL))
    {
        return false;
    }

    // 2. Get Normal and Position from G-Buffer
    NiMaterialResource* pkPixelWorldPos = NULL;
    NiMaterialResource* pkPixelWorldNorm = NULL;
    NiMaterialResource* pkPixelWorldViewVector = NULL;
    NiMaterialResource* pkPixelSpecularPower = NULL;
    if( !HandlePixelGBuffer(kContext, pkPixelWorldPos, pkPixelWorldNorm, pkPixelWorldViewVector,
        pkPixelSpecularPower) )
    {
        return false;
    }

    // 3. Handle Lighting

    efd::UInt32 uiNiPointLightCount = pkPixelDesc->GetPOINTLIGHTCOUNT();
    efd::UInt32 uiDirLightCount = pkPixelDesc->GetDIRLIGHTCOUNT();
    efd::UInt32 uiNiSpotLightCount = pkPixelDesc->GetSPOTLIGHTCOUNT();
    //efd::UInt32 uiPixelLightCount =
    //    uiNiPointLightCount + uiDirLightCount + uiNiSpotLightCount;
    //EE_ASSERT(uiPixelLightCount <= 1); // only one light should be attached to a volume

    //bool bSpecular = pkPixelDesc->GetSPECULAR() ? true : false;
    bool bSpecular = true; // specular comes from light, not NiMaterial description

    //NiMaterialResource* pkMatDiffuse = NULL;
    //NiMaterialResource* pkMatSpecular = NULL;
    NiMaterialResource* pkSpecularPower = pkPixelSpecularPower; // defaults to 1 if NULL
    //NiMaterialResource* pkGlossiness = NULL;
    //NiMaterialResource* pkMatAmbient = NULL;
    //NiMaterialResource* pkMatEmissive = NULL;

    //NiMaterialResource* pkTexDiffuseAccum = NULL;
    //NiMaterialResource* pkTexSpecularAccum = NULL;

    //NiMaterialResource* pkDiffuseAccum = NULL;
    //NiMaterialResource* pkSpecularAccum = NULL;
    //NiMaterialResource* pkOpacityAccum = NULL;

    NiMaterialResource* pkLightDiffuseAccum = NULL;
    NiMaterialResource* pkLightSpecularAccum = NULL;
    NiMaterialResource* pkLightAmbientAccum = NULL;

    //LightingModeEnum eLightingMode = LIGHTING_E_A_D;

    efd::UInt32 uiShadowMapForLight = pkPixelDesc->GetSHADOWMAPFORLIGHT();
    efd::UInt32 uiShadowTechnqiue = pkPixelDesc->GetSHADOWTECHNIQUE();

    // Extract the number of shadowmap atlas splits
    efd::UInt32 uiPSSMWhichLight = pkPixelDesc->GetPSSMWHICHLIGHT();
    efd::UInt32 uiShadowAtlasCells = NiPSSMShadowClickGenerator::
        DecodeDescriptorMaxSliceCount(pkPixelDesc->GetPSSMSLICECOUNT());

    bool bSliceTransitions = pkPixelDesc->GetPSSMSLICETRANSITIONSENABLED() == 1;

    if (!HandleLighting(kContext, uiShadowAtlasCells, uiPSSMWhichLight,
        bSliceTransitions, bSpecular, uiNiPointLightCount,
        uiDirLightCount, uiNiSpotLightCount, uiShadowMapForLight,
        uiShadowTechnqiue, pkPixelWorldPos, pkPixelWorldNorm,
        pkPixelWorldViewVector, pkSpecularPower, pkLightAmbientAccum,
        pkLightDiffuseAccum, pkLightSpecularAccum))
    {
        return false;
    }

    // 4. Final Output

    if (!HandleFinalPixelOutputs(kContext, pkPixelDesc, pkLightDiffuseAccum,
        pkLightSpecularAccum, pkLightAmbientAccum, NULL))
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiLPPLightMaterial::HandlePixelGBuffer(
    Context& kContext,
    NiMaterialResource*& pkPosition,
    NiMaterialResource*& pkNormal,
    NiMaterialResource*& pkWorldView,
    NiMaterialResource*& pkSpecularPower)
{
    // get the passed through projected world position
    NiMaterialResource* pkPassThrough = kContext.m_spInputs->AddOutputResource(
        "float4", "TexCoord", "World", "PassThrough");

    // float2 g_fPosScale; // scale of texcoord to x,y position
    // float4 g_fScaleControl; // depth buffer scale/bias, half pixel offset
    NiMaterialResource* pkPosScale = AddOutputGlobal(kContext.m_spUniforms, "g_fPosScale", 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT2);
    NiMaterialResource* pkDepthScale = AddOutputGlobal(kContext.m_spUniforms, "g_fDepthScale", 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);
    NiMaterialResource* pkProjectionSwitch = AddOutputGlobal(kContext.m_spUniforms, 
        "g_fProjectionPersOrtho", 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT2);

    // TODO want to eventually make all the calculations camera-relative
    //      need to convert light values to camera relative (g_fLightPos, etc.)
    // float3 g_fCamR;
    // float3 g_fCamU;
    // float3 g_fCamD;
    // float3 g_fCamPos;
    NiMaterialResource* pkCamR = AddOutputGlobal(kContext.m_spUniforms, "g_fCamR", 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT3);
    NiMaterialResource* pkCamU = AddOutputGlobal(kContext.m_spUniforms, "g_fCamU", 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT3);
    NiMaterialResource* pkCamD = AddOutputGlobal(kContext.m_spUniforms, "g_fCamD", 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT3);
    NiMaterialResource* pkCamPos = AddOutputGlobal(kContext.m_spUniforms, "g_fCamPos", 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT3);

    // float2 pkUV = screen space UV to look up texture
    NiMaterialResource* pkUV = NULL;
    {
        NiMaterialNode* pkFrag = GetAttachableNodeFromLibrary("LPPScreenUV");
        ADDNODE(pkFrag);
        BINDINPUT(pkPassThrough, pkFrag, "Input");
        BINDINPUT(pkDepthScale, pkFrag, "DepthScale");
        pkUV = GETOUTPUT(pkFrag,"Output");
    }

    // float4 pkGBuffer = sample from GBuffer texture using screen space UV
    NiMaterialResource* pkGBuffer = NULL;
    {
        NiFixedString kSamplerName;
        efd::UInt32 uiOccurance = 0;
        if (!GetTextureNameFromTextureEnum(MAP_BASE,
            kSamplerName, uiOccurance))
        {
            return false;
        }
        NiMaterialResource* pkSampler = InsertTextureSampler(kContext,
            kSamplerName, TEXTURE_SAMPLER_2D, uiOccurance);

        NiMaterialNode* pkFrag = GetAttachableNodeFromLibrary("TextureRGBASample");
        ADDNODE(pkFrag);
        BINDINPUT(pkUV, pkFrag, "TexCoord");
        BINDINPUT(pkSampler, pkFrag, "Sampler");
        pkGBuffer = GETOUTPUT(pkFrag, "ColorOut");
    }

    // output 1
    // float3 pkNormal = world space normal calculated from GBuffer
    {
        NiMaterialNode* pkFrag = GetAttachableNodeFromLibrary("LPPNormal");
        ADDNODE(pkFrag);
        BINDINPUT(pkGBuffer, pkFrag, "GBuffer");
        BINDINPUT(pkCamR, pkFrag, "CamR");
        BINDINPUT(pkCamU, pkFrag, "CamU");
        BINDINPUT(pkCamD, pkFrag, "CamD");
        pkNormal = GETOUTPUT(pkFrag, "Output");
    }

    // output 2
    // float3 pkPosition = world space position calculated from GBuffer and screen UV
    {
        NiMaterialResource* pkInvViewMatrix = AddOutputPredefined(
            kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_INVVIEW, 4);

        NiMaterialNode* pkFrag = GetAttachableNodeFromLibrary("LPPPos");
        ADDNODE(pkFrag);
        BINDINPUT(pkGBuffer, pkFrag, "GBuffer");
        BINDINPUT(pkUV, pkFrag, "ScreenUV");
        BINDINPUT(pkProjectionSwitch, pkFrag, "ProjectionSwitch");
        BINDINPUT(pkInvViewMatrix, pkFrag, "InvViewMatrix");
        BINDINPUT(pkDepthScale, pkFrag, "DepthScale");
        BINDINPUT(pkPosScale, pkFrag, "PosScale");
        BINDINPUT(pkCamPos, pkFrag, "CamPos");
        pkPosition = GETOUTPUT(pkFrag, "Output");
        pkWorldView = GETOUTPUT(pkFrag, "WorldView");
        pkSpecularPower = GETOUTPUT(pkFrag, "SpecularPower");
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiLPPLightMaterial::HandleFinalPixelOutputs(
    Context& kContext,
    NiStandardPixelProgramDescriptor* pkPixelDesc,
    NiMaterialResource* pkDiffuseAccum,
    NiMaterialResource* pkSpecularAccum,
	NiMaterialResource* pkAmbientAccum,
    NiMaterialResource* pkOpacityAccum)
{
    EE_UNUSED_ARG(pkPixelDesc);

    // Global contants

    // pkDiffuseAccum = Diffuse light contribution
    // pkSpecularAccum = Specular light contribution
    // pkOpacityAccum = Additional quadratic range attenuation

    // float4 pkFinal = final shader output
    NiMaterialResource* pkFinal = NULL;
    {
        NiMaterialNode* pkFrag = GetAttachableNodeFromLibrary("LPPFinal");
        ADDNODE(pkFrag);
        if (pkDiffuseAccum)
        {
            BINDINPUT(pkDiffuseAccum, pkFrag, "Diffuse");
        }
        if (pkSpecularAccum)
        {
            BINDINPUT(pkSpecularAccum, pkFrag, "Specular");
        }
		if (pkAmbientAccum)
		{
			BINDINPUT(pkAmbientAccum, pkFrag, "Ambient");
		}
        if (pkOpacityAccum)
        {
            BINDINPUT(pkOpacityAccum, pkFrag, "Atten");
        }
        pkFinal = GETOUTPUT(pkFrag, "Output");
    }

    // create output
    NiMaterialResource* pkPixelOutColor = kContext.m_spOutputs->
        AddInputResource("float4", "Color", "", "Color0");
    kContext.m_spConfigurator->AddBinding(pkFinal, pkPixelOutColor);

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiLPPLightMaterial::GenerateDescriptor(
    const NiRenderObject* pkMesh,
    const NiPropertyState* pkPropState,
    const NiDynamicEffectState* pkEffectState,
    NiMaterialDescriptor& kMaterialDesc)
{
    // This is mostly copied from NiStandardMaterial::GenerateDescriptor
    // but disabling the code that disabled per-pixel lighting when no normals exist.

    if (!pkPropState)
    {
        EE_FAIL("Could not find property state! Try calling"
            " UpdateProperties.\n");
        return false;
    }

    NiStandardMaterialDescriptor* pkDesc = (NiStandardMaterialDescriptor*)
        &kMaterialDesc;
    pkDesc->m_kIdentifier = m_kDescriptorName;

    // Handle transform
    bool bHardwareSkinned = false;
    pkDesc->SetTransformDescriptor(pkMesh, bHardwareSkinned);

    // Handle normals
    bool bNBTs = false;
    bool bNormals = false;
    pkDesc->SetNBTDescriptor(pkMesh, bHardwareSkinned,  bNormals, bNBTs);

    // Handle vertex colors
    bool bVertexColors = false;
    pkDesc->SetVertexColorDescriptor(pkMesh, bVertexColors);

    pkDesc->SetINPUTUVCOUNT(0);

    bool bSpecularEnabled = false;

    if (pkPropState)
    {
        pkDesc->SetVertexColorPropertyDescriptor(pkMesh, pkPropState,
            bVertexColors);
        pkDesc->SetSpecularPropertyDescriptor(pkMesh, pkPropState,
            bSpecularEnabled);
        pkDesc->SetMaterialPropertyDescriptor(pkMesh, pkPropState);
        pkDesc->SetFogPropertyDescriptor(pkMesh, pkPropState);
        pkDesc->SetAlphaPropertyDescriptor(pkMesh, pkPropState);

        // create active map array
        const NiTexturingProperty::Map* apkActiveMaps[
            STANDARD_PIPE_MAX_TEXTURE_MAPS];

        // Reset active maps to zero
        memset(apkActiveMaps, 0, sizeof(NiTexturingProperty::Map*) *
            STANDARD_PIPE_MAX_TEXTURE_MAPS);

        pkDesc->SetTexuringPropertyDescriptor(pkMesh, pkPropState,
            pkEffectState, apkActiveMaps, STANDARD_PIPE_MAX_TEXTURE_MAPS,
            STANDARD_PIPE_MAX_DECAL_MAPS, STANDARD_PIPE_MAX_SHADER_MAPS,
            bSpecularEnabled);
    }

    pkDesc->SetLightsDescriptor(pkMesh, pkEffectState);

    if (pkEffectState)
    {
        pkDesc->SetEnvMapDescriptor(pkMesh, pkEffectState);
        pkDesc->SetProjLightMapDescriptor(pkMesh, pkEffectState);
        pkDesc->SetProjShadowMapDescriptor(pkMesh, pkEffectState);
    }

    bool bDynamicLighting = pkDesc->GetPOINTLIGHTCOUNT() != 0 ||
        pkDesc->GetSPOTLIGHTCOUNT() != 0 ||
        pkDesc->GetDIRLIGHTCOUNT() != 0;

    efd::UInt32 uiShadowMapsForLight = pkDesc->GetSHADOWMAPFORLIGHT();

    // Only force per pixel lighting if a dynamic light exist.
    bool bUsePerPixelLighting = bDynamicLighting && m_bForcePerPixelLighting;

    if (uiShadowMapsForLight || bUsePerPixelLighting ||
        (bDynamicLighting && (pkDesc->GetPARALLAXMAPCOUNT() != 0 ||
        pkDesc->GetNORMALMAPCOUNT() != 0)))
    {
        pkDesc->SetPERVERTEXFORLIGHTS(0);
    }
    else
    {
        // Even if no lights actually exist, VS will handle all lighting
        pkDesc->SetPERVERTEXFORLIGHTS(1);
    }

    // If there are no normals, disable effects that require normals
    //if (pkDesc->GetNORMAL() == NORMAL_NONE)
    //{
    //    pkDesc->SetPOINTLIGHTCOUNT(0);
    //    pkDesc->SetDIRLIGHTCOUNT(0);
    //    pkDesc->SetSPOTLIGHTCOUNT(0);
    //
    //    NiStandardMaterial::TexEffectType eTexEffect =
    //        (NiStandardMaterial::TexEffectType) pkDesc->GetENVMAPTYPE();
    //    
    //    if (eTexEffect == NiStandardMaterial::TEXEFFECT_SPHERICAL ||
    //        eTexEffect == NiStandardMaterial::TEXEFFECT_SPECULAR_CUBE ||
    //        eTexEffect == NiStandardMaterial::TEXEFFECT_DIFFUSE_CUBE)
    //    {
    //        pkDesc->SetENVMAPTYPE(NiStandardMaterial::TEXEFFECT_NONE);
    //    }
    //}

    return true;
}

//--------------------------------------------------------------------------------------------------
NiFragmentMaterial::ReturnCode NiLPPLightMaterial::GenerateShaderDescArray(
    NiMaterialDescriptor* pkMaterialDescriptor,
    RenderPassDescriptor* pkRenderPasses,
    efd::UInt32 uiMaxCount,
    efd::UInt32& uiCountAdded)
{
    // This is mostly copied from NiStandardMaterial::GenerateShaderDescArray
    // but disabling the code that disabled per-pixel lighting when no normals exist.

    EE_UNUSED_ARG(uiMaxCount);
    EE_ASSERT(uiMaxCount != 0);
    uiCountAdded = 0;

    if (pkMaterialDescriptor->m_kIdentifier != "NiStandardMaterialDescriptor")
        return RC_INVALID_MATERIAL;

    // Make sure that we're using the Gamebryo render state on the first pass.
    pkRenderPasses[0].m_bUsesNiRenderState = true;

    // Reset all object offsets for the first pass.
    pkRenderPasses[0].m_bResetObjectOffsets = true;

    NiStandardMaterialDescriptor* pkMatlDesc =
        (NiStandardMaterialDescriptor*) pkMaterialDescriptor;

    // Uncomment these lines to get a human-readable version of the
    // NiMaterial description
    // String kDescString = pkMatlDesc->ToString();

    NiStandardVertexProgramDescriptor* pkVertexDesc =
        (NiStandardVertexProgramDescriptor*) pkRenderPasses[0].m_pkVertexDesc;
    pkVertexDesc->m_kIdentifier = "NiStandardVertexProgramDescriptor";

    NiStandardPixelProgramDescriptor* pkPixelDesc =
        (NiStandardPixelProgramDescriptor*) pkRenderPasses[0].m_pkPixelDesc;
    pkPixelDesc->m_kIdentifier = "NiStandardPixelProgramDescriptor";

#if defined(EE_ASSERTS_ARE_ENABLED)
    NormalType eNormalType = (NormalType)pkMatlDesc->GetNORMAL();

    //bool bHasNormal = eNormalType != NORMAL_NONE;
    bool bHasNBT = eNormalType == NORMAL_NBT;
#endif

    // Pixel Desc
    efd::UInt32 uiApplyMode = pkMatlDesc->GetAPPLYMODE();
    pkPixelDesc->SetAPPLYMODE(uiApplyMode);

    efd::UInt32 uiNormalMapType = pkMatlDesc->GetNORMALMAPTYPE();
    pkPixelDesc->SetNORMALMAPTYPE(uiNormalMapType);

    efd::UInt32 uiParallaxCount = pkMatlDesc->GetPARALLAXMAPCOUNT();
    pkPixelDesc->SetPARALLAXMAPCOUNT(uiParallaxCount);

    efd::UInt32 uiBaseCount = pkMatlDesc->GetBASEMAPCOUNT();
    pkPixelDesc->SetBASEMAPCOUNT(uiBaseCount);

    efd::UInt32 uiNormalMapCount = pkMatlDesc->GetNORMALMAPCOUNT();
    pkPixelDesc->SetNORMALMAPCOUNT(uiNormalMapCount);

    efd::UInt32 uiDarkMapCount = pkMatlDesc->GetDARKMAPCOUNT();
    pkPixelDesc->SetDARKMAPCOUNT(uiDarkMapCount);

    efd::UInt32 uiDetailMapCount = pkMatlDesc->GetDETAILMAPCOUNT();
    pkPixelDesc->SetDETAILMAPCOUNT(uiDetailMapCount);

    efd::UInt32 uiBumpMapCount = pkMatlDesc->GetBUMPMAPCOUNT();
    pkPixelDesc->SetBUMPMAPCOUNT(uiBumpMapCount);

    efd::UInt32 uiGlossMapCount = pkMatlDesc->GetGLOSSMAPCOUNT();
    pkPixelDesc->SetGLOSSMAPCOUNT(uiGlossMapCount);

    efd::UInt32 uiGlowMapCount = pkMatlDesc->GetGLOWMAPCOUNT();
    pkPixelDesc->SetGLOWMAPCOUNT(uiGlowMapCount);

    efd::UInt32 uiCustomMap00Count = pkMatlDesc->GetCUSTOMMAP00COUNT();
    pkPixelDesc->SetCUSTOMMAP00COUNT(uiCustomMap00Count);

    efd::UInt32 uiCustomMap01Count = pkMatlDesc->GetCUSTOMMAP01COUNT();
    pkPixelDesc->SetCUSTOMMAP01COUNT(uiCustomMap01Count);

    efd::UInt32 uiCustomMap02Count = pkMatlDesc->GetCUSTOMMAP02COUNT();
    pkPixelDesc->SetCUSTOMMAP02COUNT(uiCustomMap02Count);

    efd::UInt32 uiCustomMap03Count = pkMatlDesc->GetCUSTOMMAP03COUNT();
    pkPixelDesc->SetCUSTOMMAP03COUNT(uiCustomMap03Count);

    efd::UInt32 uiCustomMap04Count = pkMatlDesc->GetCUSTOMMAP04COUNT();
    pkPixelDesc->SetCUSTOMMAP04COUNT(uiCustomMap04Count);

    efd::UInt32 uiDecalMapCount = pkMatlDesc->GetDECALMAPCOUNT();
    pkPixelDesc->SetDECALMAPCOUNT(uiDecalMapCount);

    efd::UInt32 uiProjLightMapCount = pkMatlDesc->GetPROJLIGHTMAPCOUNT();
    pkPixelDesc->SetPROJLIGHTMAPCOUNT(uiProjLightMapCount);

    efd::UInt32 uiProjShadowMapCount = pkMatlDesc->GetPROJSHADOWMAPCOUNT();
    pkPixelDesc->SetPROJSHADOWMAPCOUNT(uiProjShadowMapCount);

    efd::UInt32 uiProjShadowMapTypes = pkMatlDesc->GetPROJSHADOWMAPTYPES();
    pkPixelDesc->SetPROJSHADOWMAPTYPES(uiProjShadowMapTypes);

    efd::UInt32 uiProjLightMapTypes = pkMatlDesc->GetPROJLIGHTMAPTYPES();
    pkPixelDesc->SetPROJLIGHTMAPTYPES(uiProjLightMapTypes);

    efd::UInt32 uiProjShadowMapClipped =
        pkMatlDesc->GetPROJSHADOWMAPCLIPPED();
    pkPixelDesc->SetPROJSHADOWMAPCLIPPED(uiProjShadowMapClipped);

    efd::UInt32 uiProjLightMapClipped = pkMatlDesc->GetPROJLIGHTMAPCLIPPED();
    pkPixelDesc->SetPROJLIGHTMAPCLIPPED(uiProjLightMapClipped);

    efd::UInt32 uiAmbDiffEmissive = pkMatlDesc->GetAMBDIFFEMISSIVE();
    pkPixelDesc->SetAMBDIFFEMISSIVE(uiAmbDiffEmissive);

    efd::UInt32 uiLightingMode = pkMatlDesc->GetLIGHTINGMODE();
    pkPixelDesc->SetLIGHTINGMODE(uiLightingMode);

    efd::UInt32 uiPerVertexForLights = pkMatlDesc->GetPERVERTEXFORLIGHTS();
    pkPixelDesc->SetPERVERTEXLIGHTING(uiPerVertexForLights);

    efd::UInt32 uiFogType = pkMatlDesc->GetFOGTYPE();
    pkPixelDesc->SetFOGENABLED(uiFogType != FOG_NONE);

    efd::UInt32 uiAlphaTest = pkMatlDesc->GetALPHATEST();
    pkPixelDesc->SetALPHATEST(uiAlphaTest);

    if (NiShadowManager::GetShadowManager() &&
        NiShadowManager::GetActive())
    {
        efd::UInt32 uiShadowMapsForLight =
            pkMatlDesc->GetSHADOWMAPFORLIGHT();
        pkPixelDesc->SetSHADOWMAPFORLIGHT(uiShadowMapsForLight);

        efd::UInt32 uiShadowTechniqueSlot = pkMatlDesc->GetSHADOWTECHNIQUE();
        NiShadowTechnique* pkShadowTechnique =
            NiShadowManager::GetActiveShadowTechnique(
            (unsigned short)uiShadowTechniqueSlot);

        pkPixelDesc->SetSHADOWTECHNIQUE(pkShadowTechnique->GetTechniqueID());

        // Number of shadow map atlas splits
        efd::UInt32 uiPSSMWhichLight = pkMatlDesc->GetPSSMWHICHLIGHT();
        efd::UInt32 uiPSSMSliceCount = pkMatlDesc->GetPSSMSLICECOUNT();
        efd::UInt32 uiPSSMSliceTransitionsEnabled =
            pkMatlDesc->GetPSSMSLICETRANSITIONSENABLED();

        pkPixelDesc->SetPSSMWHICHLIGHT(uiPSSMWhichLight);
        pkPixelDesc->SetPSSMSLICECOUNT(uiPSSMSliceCount);
        pkPixelDesc->SetPSSMSLICETRANSITIONSENABLED(
            uiPSSMSliceTransitionsEnabled);
    }
    else
    {
        pkPixelDesc->SetSHADOWTECHNIQUE(0);
    }

    efd::UInt32 uiDirLightCount = pkMatlDesc->GetDIRLIGHTCOUNT();
    efd::UInt32 uiNiSpotLightCount = pkMatlDesc->GetSPOTLIGHTCOUNT();
    efd::UInt32 uiNiPointLightCount = pkMatlDesc->GetPOINTLIGHTCOUNT();
    efd::UInt32 uiShadowMapForLight = pkMatlDesc->GetSHADOWMAPFORLIGHT();
    efd::UInt32 uiSpecular = pkMatlDesc->GetSPECULAR();

    efd::UInt32 uiNumLights = uiDirLightCount + uiNiSpotLightCount +
        uiNiPointLightCount;

    // If the apply mode is REPLACE, then no lighting takes place
    if (uiApplyMode == APPLY_REPLACE)
    {
        uiNumLights = 0;
        uiDirLightCount = 0;
        uiNiSpotLightCount = 0;
        uiNiPointLightCount = 0;
        uiSpecular = 0;
        uiPerVertexForLights = 1;
    }

    // Wait to set specular until after the apply mode has been taken into
    // consideration.
    pkPixelDesc->SetSPECULAR(uiSpecular);

    // If per-pixel lighting
    if (uiPerVertexForLights == 0)
    {
        pkPixelDesc->SetPOINTLIGHTCOUNT(uiNiPointLightCount);
        pkPixelDesc->SetSPOTLIGHTCOUNT(uiNiSpotLightCount);
        pkPixelDesc->SetDIRLIGHTCOUNT(uiDirLightCount);
        pkPixelDesc->SetSHADOWMAPFORLIGHT(uiShadowMapForLight);
        pkPixelDesc->SetAPPLYAMBIENT(true);
        pkPixelDesc->SetAPPLYEMISSIVE(true);

        if (uiSpecular != 0)
        {
            //pkVertexDesc->SetOUTPUTWORLDVIEW(uiSpecular);
            //pkPixelDesc->SetWORLDVIEW(1);
        }

        if (uiNumLights != 0)
        {
            //pkVertexDesc->SetOUTPUTWORLDPOS(1);

            //EE_ASSERT(bHasNormal);
            //pkVertexDesc->SetOUTPUTWORLDNBT(1);

            //pkPixelDesc->SetWORLDNORMAL(1);
            //pkPixelDesc->SetWORLDPOSITION(1);

            pkVertexDesc->SetVERTEXLIGHTSONLY(0);
        }
        else
        {
            pkVertexDesc->SetVERTEXLIGHTSONLY(1);
            pkPixelDesc->SetPERVERTEXLIGHTING(1);
        }

    }
    else
    {
        pkVertexDesc->SetPOINTLIGHTCOUNT(uiNiPointLightCount);

        pkVertexDesc->SetSPOTLIGHTCOUNT(uiNiSpotLightCount);

        pkVertexDesc->SetDIRLIGHTCOUNT(uiDirLightCount);

        pkVertexDesc->SetSPECULAR(uiSpecular);

        pkPixelDesc->SetAPPLYAMBIENT(false);

        pkPixelDesc->SetAPPLYEMISSIVE(false);

        pkVertexDesc->SetVERTEXLIGHTSONLY(1);

        pkPixelDesc->SetPERVERTEXLIGHTING(1);
    }

    EE_ASSERT(pkVertexDesc->GetVERTEXLIGHTSONLY() ==
        pkPixelDesc->GetPERVERTEXLIGHTING());

    // Vertex Desc
    efd::UInt32 uiTransform = pkMatlDesc->GetTRANSFORM();
    pkVertexDesc->SetTRANSFORM(uiTransform);

    efd::UInt32 uiNormal = pkMatlDesc->GetNORMAL();

    if (uiNormal == NORMAL_NBT &&
        !pkMatlDesc->GetNORMALMAPCOUNT() &&
        !pkMatlDesc->GetPARALLAXMAPCOUNT())
    {
        // Only use full NBT frame if normal mapping is being used.
        uiNormal = NORMAL_ONLY;
    }
    pkVertexDesc->SetNORMAL(uiNormal);

    pkVertexDesc->SetFOGTYPE(uiFogType);

    efd::UInt32 uiVertexColors = pkMatlDesc->GetVERTEXCOLORS();
    pkVertexDesc->SetVERTEXCOLORS(uiVertexColors);

    pkVertexDesc->SetPROJLIGHTMAPCOUNT(uiProjLightMapCount);
    pkVertexDesc->SetPROJSHADOWMAPCOUNT(uiProjShadowMapCount);

    efd::UInt32 uiEnvMapType = pkMatlDesc->GetENVMAPTYPE();

    // If the normal map exists, we want the per-pixel normal to
    // affect the environment map.
    if (uiNormalMapCount != 0)
    {
        EE_ASSERT(bHasNBT);
        //pkVertexDesc->SetOUTPUTWORLDNBT(1);
        //pkPixelDesc->SetWORLDNBT(1);

        pkVertexDesc->SetENVMAPTYPE(TEXEFFECT_NONE);

        pkPixelDesc->SetENVMAPTYPE(uiEnvMapType);

        if (uiEnvMapType == TEXEFFECT_SPECULAR_CUBE ||
            uiEnvMapType == TEXEFFECT_SPHERICAL)
        {
            pkVertexDesc->SetOUTPUTWORLDVIEW(1);
            pkPixelDesc->SetWORLDVIEW(1);
        }
    }
    else
    {
        pkVertexDesc->SetENVMAPTYPE(uiEnvMapType);
        pkPixelDesc->SetENVMAPTYPE(uiEnvMapType);
    }

    if (uiParallaxCount != 0)
    {
        EE_ASSERT(bHasNBT);
        pkVertexDesc->SetOUTPUTTANGENTVIEW(1);
        pkVertexDesc->SetOUTPUTWORLDNBT(1);
        pkPixelDesc->SetWORLDNBT(1);
    }

    if (pkPixelDesc->GetPROJLIGHTMAPCLIPPED() != 0 ||
        pkPixelDesc->GetPROJSHADOWMAPCLIPPED() != 0)
    {
        pkVertexDesc->SetOUTPUTWORLDPOS(1);
        pkPixelDesc->SetWORLDPOSITION(1);
    }

    efd::UInt32 auiUVSets[STANDARD_PIPE_MAX_TEXTURE_MAPS];
    memset(auiUVSets, UINT_MAX, sizeof(auiUVSets));

    TexGenOutput aeTexGenOutputs[STANDARD_PIPE_MAX_TEXTURE_MAPS];
    memset(aeTexGenOutputs, 0, sizeof(aeTexGenOutputs));

    efd::UInt32 uiTextureCount = pkMatlDesc->GetStandardTextureCount();
    EE_ASSERT(uiTextureCount <= STANDARD_PIPE_MAX_TEXTURE_MAPS);

    for (efd::UInt32 ui = 0; ui < uiTextureCount; ui++)
    {
        pkMatlDesc->GetTextureUsage(ui, auiUVSets[ui], aeTexGenOutputs[ui]);
    }

    AssignTextureCoordinates(auiUVSets, aeTexGenOutputs, uiTextureCount,
        pkVertexDesc, pkPixelDesc);

    pkVertexDesc->SetAMBDIFFEMISSIVE(uiAmbDiffEmissive);
    pkVertexDesc->SetLIGHTINGMODE(uiLightingMode);
    pkVertexDesc->SetAPPLYMODE(uiApplyMode);

    uiCountAdded++;
    return RC_SUCCESS;
}

//------------------------------------------------------------------------------------------------
