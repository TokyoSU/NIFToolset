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

#include "NiDecorationPCH.h"

#include "NiDecorationMaterial.h"

// The following include is here since it is missing from the node library
#include "NiDecorationLibType.h"
#include "NiDecorationMaterialNodeLibrary.h"
#include "NiDecorationGenerator.h"
#include "NiStandardMaterialNodeLibrary.h"
#include "NiDecorationMaterialDescriptor.h"
#include "NiDecorationPixelProgramDescriptor.h"

//------------------------------------------------------------------------------------------------
NiImplementRTTI(NiDecorationMaterial, NiStandardMaterial);
//------------------------------------------------------------------------------------------------
const char* NiDecorationMaterial::FADE_OUTERMINDISTSQR_SHADER_CONSTANT = "g_FadeOuterMinDistSqr";
const char* NiDecorationMaterial::FADE_OUTERMAXDISTSQR_SHADER_CONSTANT = "g_FadeOuterMaxDistSqr";
const char* NiDecorationMaterial::FADE_INNERMINDISTSQR_SHADER_CONSTANT = "g_FadeInnerMinDistSqr";
const char* NiDecorationMaterial::FADE_INNERMAXDISTSQR_SHADER_CONSTANT = "g_FadeInnerMaxDistSqr";
const char* NiDecorationMaterial::DIFFUSE_SATURATION_MULTIPLIER_SHADER_CONSTANT = 
    "g_DiffuseSaturationMultiplier";

//------------------------------------------------------------------------------------------------
NiDecorationMaterial* NiDecorationMaterial::Create()
{
    // Get the material if it exist already
    NiDecorationMaterial* pkMaterial = NiDynamicCast(NiDecorationMaterial, 
        NiMaterial::GetMaterial("NiDecorationMaterial"));

    if (!pkMaterial)
    {
        // Create a new material if we didn't find it
        pkMaterial = NiNew NiDecorationMaterial();
    }

    return pkMaterial;
}

//------------------------------------------------------------------------------------------------
NiDecorationMaterial::NiDecorationMaterial(bool bAutoCreateCaches)
: NiStandardMaterial("NiDecorationMaterial", NULL, 
                     NIDECORATIONMATERIAL_VERTEX_VERSION, 
                     NIDECORATIONMATERIAL_GEOMETRY_VERSION, 
                     NIDECORATIONMATERIAL_PIXEL_VERSION, bAutoCreateCaches)
{
    m_kLibraries.Add(NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(NiDecorationMaterialNodeLibrary::CreateMaterialNodeLibrary());
    AddDefaultFallbacks();
}

//------------------------------------------------------------------------------------------------
NiDecorationMaterial::NiDecorationMaterial(const NiFixedString& kName, 
	unsigned int uiVertextVersion, 
	unsigned int uiGeometryVersion, 
	unsigned int uiPixelVersion, 
	bool bAutoCreateCaches)
	: NiStandardMaterial(kName, 
	 NULL,
	 uiVertextVersion, 
	 uiGeometryVersion, 
	 uiPixelVersion, 
	 bAutoCreateCaches)
{
	m_kLibraries.Add(NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary());
	m_kLibraries.Add(NiDecorationMaterialNodeLibrary::CreateMaterialNodeLibrary());
	AddDefaultFallbacks();
}

//------------------------------------------------------------------------------------------------
bool NiDecorationMaterial::GenerateDescriptor(const NiRenderObject* pkMesh, 
    const NiPropertyState* pkPropState, 
    const NiDynamicEffectState* pkEffectState,
    NiMaterialDescriptor& kMaterialDesc)
{
    // Standard descriptor entries
    bool bRes = NiStandardMaterial::GenerateDescriptor(pkMesh, pkPropState, 
        pkEffectState, kMaterialDesc);

    if (!bRes)
        return false;

    NiTexturingProperty* pkTexProp = pkPropState->GetTexturing();

    // Decoration specific descriptor entries
    NiDecorationMaterialDescriptor* pkDesc = (NiDecorationMaterialDescriptor*)&kMaterialDesc;

    // Transition Type
    pkDesc->SetFADE_METHOD(FADETYPE_NONE);
    if (pkTexProp != NULL &&
        pkMesh->GetExtraData(FADE_INNERMAXDISTSQR_SHADER_CONSTANT) != NULL &&
        pkMesh->GetExtraData(FADE_INNERMAXDISTSQR_SHADER_CONSTANT) != NULL &&
        pkMesh->GetExtraData(FADE_INNERMAXDISTSQR_SHADER_CONSTANT) != NULL &&
        pkMesh->GetExtraData(FADE_INNERMAXDISTSQR_SHADER_CONSTANT) != NULL)
    {
        if (pkTexProp->GetShaderMap(0) != NULL)
            pkDesc->SetFADE_METHOD(FADETYPE_NOISE);
    }

    // FIXME: Force standard shadowing technique instead of PCF, for performance reasons (since you
    // can't set it per object, only per light).
    NiUInt32 activeTechnique = pkDesc->GetSHADOWTECHNIQUE();
    if (activeTechnique == 1)
        pkDesc->SetSHADOWTECHNIQUE(0);

    return true;
}

//------------------------------------------------------------------------------------------------
NiFragmentMaterial::ReturnCode NiDecorationMaterial::GenerateShaderDescArray(
    NiMaterialDescriptor* pkMaterialDescriptor,
    RenderPassDescriptor* pkRenderPasses, 
    unsigned int uiMaxCount, 
    unsigned int& uiCountAdded)
{
    // Generate the standard material descriptor entries
    NiFragmentMaterial::ReturnCode returnCode = 
        NiStandardMaterial::GenerateShaderDescArray(pkMaterialDescriptor, 
        pkRenderPasses, uiMaxCount, uiCountAdded);

    if (returnCode != NiFragmentMaterial::RC_SUCCESS)
        return returnCode;

    NiDecorationMaterialDescriptor* pkMatlDesc = 
        (NiDecorationMaterialDescriptor*) pkMaterialDescriptor;

    // Decoration specific descriptor entries
    NiStandardVertexProgramDescriptor* pkVertexDesc = 
        (NiStandardVertexProgramDescriptor*) pkRenderPasses[0].m_pkVertexDesc;

    NiDecorationPixelProgramDescriptor* pkPixelDesc = 
        (NiDecorationPixelProgramDescriptor*) pkRenderPasses[0].m_pkPixelDesc;

    // Distance fade
    if (pkMatlDesc->GetFADE_METHOD() == FADETYPE_NOISE)
    {
        pkPixelDesc->SetFADE_NOISE_ENABLED(1);
        pkVertexDesc->SetOUTPUTWORLDPOS(1);
        pkPixelDesc->SetWORLDPOSITION(1);
    }
    else
    {
        pkPixelDesc->SetFADE_NOISE_ENABLED(0);
    }

    return RC_SUCCESS;
}

//------------------------------------------------------------------------------------------------
bool NiDecorationMaterial::GeneratePixelShadeTree(Context& kContext, NiGPUProgramDescriptor* pkDesc)
{
    bool bRes = NiStandardMaterial::GeneratePixelShadeTree(kContext, pkDesc);
    if (!bRes)
        return false;

    NiDecorationPixelProgramDescriptor* pkPixelDesc = (NiDecorationPixelProgramDescriptor*)pkDesc;
    kContext.m_spConfigurator->SetDescription(pkPixelDesc->ToString());

    return true;
}

//------------------------------------------------------------------------------------------------
unsigned int NiDecorationMaterial::GetMaterialDescriptorSize()
{
    return MATERIAL_DESCRIPTOR_BYTE_COUNT;
}

//------------------------------------------------------------------------------------------------
unsigned int NiDecorationMaterial::GetPixelProgramDescriptorSize()
{
    return PIXEL_PROGRAM_DESCRIPTOR_BYTE_COUNT;
}

//------------------------------------------------------------------------------------------------
bool NiDecorationMaterial::HandlePixelUVSets(Context& kContext,
    NiStandardPixelProgramDescriptor* pkPixelDesc,  
    NiMaterialResource** ppkUVSets, 
    unsigned int uiMaxUVIndex,
    unsigned int& uiNumStandardUVs, 
    unsigned int& uiDynamicEffectCount)
{
    bool bRes = NiStandardMaterial::HandlePixelUVSets(kContext,pkPixelDesc, ppkUVSets, uiMaxUVIndex,
        uiNumStandardUVs, uiDynamicEffectCount);
    if (!bRes)
        return false;

    NiDecorationPixelProgramDescriptor* pkDecoPixelDesc = 
        (NiDecorationPixelProgramDescriptor*)pkPixelDesc;

    if (pkDecoPixelDesc->GetFADE_NOISE_ENABLED())
    {
        // Add the early exit alpha
        NiMaterialNode* pkFragment = GetAttachableNodeFromLibrary("DistanceAlpha");
        EE_ASSERT(pkFragment);

        NiMaterialResource* pkCameraPos = AddOutputPredefined(
            kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_EYE_POS);
        EE_ASSERT(pkCameraPos);

        kContext.m_spConfigurator->AddNode(pkFragment);

        NiMaterialResource* pkPixelWorldPos = 
            kContext.m_spInputs->GetOutputResourceByVariableName("WorldPos"); 
        EE_ASSERT(pkPixelWorldPos);
        // Positions used to calculate desired alpha
        kContext.m_spConfigurator->AddBinding(pkPixelWorldPos, "WorldPos", pkFragment);
        kContext.m_spConfigurator->AddBinding(pkCameraPos, 
            pkFragment->GetInputResourceByVariableName("CameraPos"));

        // Minimum and maximum distance thresholds    
        NiMaterialResource* pkFadeOuterMinDistanceSqr = AddOutputAttribute(
            kContext.m_spUniforms, FADE_OUTERMINDISTSQR_SHADER_CONSTANT,
            NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
        EE_ASSERT(pkFadeOuterMinDistanceSqr);
        kContext.m_spConfigurator->AddBinding(pkFadeOuterMinDistanceSqr,
            pkFragment->GetInputResourceByVariableName("FadeOuterMinDistSqr"));

        NiMaterialResource* pkFadeOuterMaxDistanceSqr = AddOutputAttribute(
            kContext.m_spUniforms, FADE_OUTERMAXDISTSQR_SHADER_CONSTANT,
            NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
        EE_ASSERT(pkFadeOuterMaxDistanceSqr);
        kContext.m_spConfigurator->AddBinding(pkFadeOuterMaxDistanceSqr,
            pkFragment->GetInputResourceByVariableName("FadeOuterMaxDistSqr"));

        NiMaterialResource* pkFadeInnerMinDistanceSqr = AddOutputAttribute(
            kContext.m_spUniforms, FADE_INNERMINDISTSQR_SHADER_CONSTANT,
            NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
        EE_ASSERT(pkFadeInnerMinDistanceSqr);
        kContext.m_spConfigurator->AddBinding(pkFadeInnerMinDistanceSqr,
            pkFragment->GetInputResourceByVariableName("FadeInnerMinDistSqr"));

        NiMaterialResource* pkFadeInnerMaxDistanceSqr = AddOutputAttribute(
            kContext.m_spUniforms, FADE_INNERMAXDISTSQR_SHADER_CONSTANT,
            NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
        EE_ASSERT(pkFadeInnerMaxDistanceSqr);
        kContext.m_spConfigurator->AddBinding(pkFadeInnerMaxDistanceSqr,
            pkFragment->GetInputResourceByVariableName("FadeInnerMaxDistSqr"));

        // fly screen alpha texture
        NiFixedString kSamplerName("FadeMask");
        NiMaterialResource* pkSampler = 
            kContext.m_spUniforms->AddOutputResource("sampler2D", "Shader", 
            "", kSamplerName, 1, NiMaterialResource::SOURCE_PREDEFINED, 
            NiShaderAttributeDesc::OT_UNDEFINED, 0);
        EE_ASSERT(pkSampler);

        kContext.m_spConfigurator->AddBinding(pkSampler, 
            pkFragment->GetInputResourceByVariableName("FadeMask"));

        NiMaterialResource* pkTexcoord = kContext.m_spInputs->AddOutputResource("float2", 
            "TexCoord", "", "UVSet0");

        AddOutputAttribute(kContext.m_spOutputs, "UVSet0", 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT2, 1);

        bRes &= kContext.m_spConfigurator->AddBinding(pkTexcoord, "MapUV", pkFragment);
    }

    return bRes;
}

//------------------------------------------------------------------------------------------------
bool NiDecorationMaterial::HandleBaseMap(Context& kContext, NiMaterialResource* pkUVSet, 
    NiMaterialResource*& pkDiffuseColorAccum, 
    NiMaterialResource*& pkOpacity, 
    bool bOpacityOnly)
{
    NiStandardMaterial::HandleBaseMap(kContext, pkUVSet, pkDiffuseColorAccum, pkOpacity, 
        bOpacityOnly);

    // Multiply the diffuse color by a given over-saturation factor
    NiMaterialResource* pkSaturationMultiplier = AddOutputAttribute(kContext.m_spUniforms,
         DIFFUSE_SATURATION_MULTIPLIER_SHADER_CONSTANT,
        NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
    EE_ASSERT(pkSaturationMultiplier);

    return ScaleVector(kContext, pkDiffuseColorAccum, pkSaturationMultiplier, pkDiffuseColorAccum);
}

//------------------------------------------------------------------------------------------------