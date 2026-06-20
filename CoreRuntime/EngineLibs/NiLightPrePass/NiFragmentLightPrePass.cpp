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
#include "NiLightPrePassPCH.h"

#include "NiFragmentLightPrePass.h"
#include "NiStandardMaterialNodeLibrary.h"

#include "NiLightPrePassMaterialNodeLibrary.h"
#include "NiStandardPixelProgramDescriptor.h"
#include "NiStandardVertexProgramDescriptor.h"
#include "NiTerrainCellShaderData.h"

//--------------------------------------------------------------------------------------------------
NiImplementRTTI(NiFragmentLightPrePass, NiFragment, NiTypeMask::NiFragmentLightPrePass);
//--------------------------------------------------------------------------------------------------
NiFragmentLightPrePass::NiFragmentLightPrePass():
    NiFragment(VERTEX_VERSION,GEOMETRY_VERSION,PIXEL_VERSION)
{
    // Append the required node libraries
    m_kLibraries.Add(
        NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary());
}

//--------------------------------------------------------------------------------------------------
void NiFragmentLightPrePass::FetchDependencies()
{
	// Set the owner as per normal
	NiFragment::FetchDependencies();

	// Fetch any dependant fragments that this fragment needs
	NiFragment::Fetch(m_pkMaterial, m_pkLighting);
	NiFragment::Fetch(m_pkMaterial, m_pkOperations);
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLightPrePass::HandlePixelLighting(Context& kContext,
	NiTerrainMaterialPixelDescriptor* pkPixelDesc, 
	NiMaterialResource* pkWorldPos, 
	NiMaterialResource* pkWorldView, 
	NiMaterialResource* pkSpecularPower, 
	NiMaterialResource*& pkFinalNormal,
	NiMaterialResource* pkMatEmissive, 
	NiMaterialResource* pkMatDiffuse, 
	NiMaterialResource* pkMatAmbient, 
	NiMaterialResource* pkMatSpecular, 
	NiMaterialResource* pkLightSpecularAccum, 
	NiMaterialResource* pkLightDiffuseAccum, 
	NiMaterialResource* pkLightAmbientAccum,
	NiMaterialResource* pkGlossiness, 
	NiMaterialResource* pkFinalDiffuse,
	NiMaterialResource* pkFinalSpecular, 
	NiMaterialResource*& pkSpecularAccum, 
	NiMaterialResource*& pkDiffuseAccum)
{
	// Generate projected position
	NiMaterialResource* pkProjectedPos = NULL;
	NiMaterialResource* pkViewProjMatrix = AddOutputPredefined(
		kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_VIEWPROJ, 4);
	m_pkOperations->TransformPosition(kContext, pkWorldPos, pkViewProjMatrix, pkProjectedPos);
	EE_ASSERT(pkProjectedPos);

	// Extract the lighting descriptor from the pixel descriptor
	NiFragmentLighting::PixelDescriptor kLightingPixelDesc;
	m_pkLighting->GetDescriptor(pkPixelDesc, kLightingPixelDesc);

	NiUInt32 uiPSSMWhichLight = kLightingPixelDesc.usPSSMWhichLight;
	NiUInt32 uiShadowAtlasCells = NiPSSMShadowClickGenerator::
		DecodeDescriptorMaxSliceCount(kLightingPixelDesc.usPSSMSliceCount);

	// Perform the final accumulation of lighting for this pixel
	if (!HandleLighting(kContext, 
		uiShadowAtlasCells,
		uiPSSMWhichLight,
		kLightingPixelDesc.bPSSMSliceTransitionEnabled,
		kLightingPixelDesc.bSpecularOn,
		kLightingPixelDesc.uiNumPointLights,
		kLightingPixelDesc.uiNumDirectionalLights,
		kLightingPixelDesc.uiNumSpotLights,
		kLightingPixelDesc.uiShadowMapBitfield,
		kLightingPixelDesc.usShadowTechnique,
		pkWorldPos,
		pkFinalNormal,
		pkWorldView,
		pkSpecularPower,
		pkProjectedPos,
		pkLightAmbientAccum,
		pkLightDiffuseAccum,
		pkLightSpecularAccum))
	{
		return false;
	}

	// Merge all lighting calculations into specular and diffuse values
	if (!m_pkLighting->HandleColorAccumulation(kContext, kLightingPixelDesc, 
		pkMatEmissive, pkMatDiffuse, pkMatAmbient, pkMatSpecular, 
		pkLightSpecularAccum, pkLightDiffuseAccum, pkLightAmbientAccum,
		pkGlossiness, pkFinalDiffuse, pkFinalSpecular,
		pkSpecularAccum, pkDiffuseAccum))
	{
		return false;
	}

	return true;

}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLightPrePass::HandleFinalPixelOutputs_DepthNormal(Context& kContext,
	NiStandardPixelProgramDescriptor* pkPixelDesc,
	NiMaterialResource* pkDiffuseAccum,
	NiMaterialResource* pkSpecularAccum,
	NiMaterialResource* pkOpacityAccum,
	NiMaterialResource* pkWorldNormal)
{
	// lighting is not used by this shader
	EE_UNUSED_ARG(pkSpecularAccum);
	EE_UNUSED_ARG(pkDiffuseAccum);

	// handle alpha test
	if (pkOpacityAccum && pkPixelDesc->GetALPHATEST() != 0)
	{
		// TODO Alpha tested objects are currently not allowed for
		//      this NiMaterial. Because the depth value outputs to
		//      the alpha color, we need to clip in the shader
		//      and not with the hardware alpha test.
		//      The code below performs the clip in the shader
		//      properly, but we do not yet disable the hardware test.

		m_pkOperations->HandleAlphaTest(kContext, true, pkOpacityAccum, true);
	}

	// Fetch the input
	NiMaterialResource* pkWorldPos = kContext.m_spInputs->AddOutputResource(
		"float4", "TexCoord", "", "WorldPos");

    NiMaterialResource* pkSpecularPower = AddOutputPredefined(kContext.m_spUniforms,
        NiShaderConstantMap::SCM_DEF_MATERIAL_POWER);

	NiMaterialResource* pkDepthNormalOutput = NULL;
	if (!HandlePixelGenerateNormalDepthOutput(kContext, 
		pkWorldNormal, pkWorldPos, pkSpecularPower, pkDepthNormalOutput))
	{
		return false;
	}

	// Bind output 
	NiMaterialResource* pkPixelOutColor = 
		kContext.m_spOutputs->AddInputResource("float4", "Color", "", "Color0");
	kContext.m_spConfigurator->AddBinding(pkDepthNormalOutput, pkPixelOutColor);

	return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLightPrePass::HandleFinalPixelOutputs_Final(Context& kContext,
    NiStandardPixelProgramDescriptor* pkPixelDesc,
    NiMaterialResource* pkDiffuseAccum,
    NiMaterialResource* pkSpecularAccum,
    NiMaterialResource* pkOpacityAccum)
{
    EE_UNUSED_ARG(pkDiffuseAccum);
    EE_UNUSED_ARG(pkSpecularAccum);

    // handle alpha test
    if (pkOpacityAccum && pkPixelDesc->GetALPHATEST() != 0)
    {
        // TODO Alpha tested objects are currently not allowed for
        //      this NiMaterial. Because the depth value outputs to
        //      the alpha color, we need to clip in the shader
        //      and not with the hardware alpha test.
        //      The code below performs the clip in the shader
        //      properly, but we do not yet disable the hardware test.

        m_pkOperations->HandleAlphaTest(kContext, true, pkOpacityAccum, true);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// For terrian materials.
bool NiFragmentLightPrePass::HandleFinalPixelOutputs_DepthNormal(Context& kContext, 
	NiTerrainMaterialPixelDescriptor* pkPixelDesc,
	NiMaterialResource* pkDiffuseAccum,
	NiMaterialResource* pkSpecularAccum,
	NiMaterialResource* pkOpacityAccum, 
	NiMaterialResource* pkGlossiness,
	NiMaterialResource* pkFinalNormal, 
	NiMaterialResource* pkParallaxOffset, 
	NiMaterialResource* pkMorphValue, 
	NiMaterialResource* pkTotalMask,
	NiMaterialResource* pkWorldPos, 
    NiMaterialResource* pkSpecularPower)
{
	// lighting is not used by this shader
	EE_UNUSED_ARG(pkPixelDesc);
	EE_UNUSED_ARG(pkDiffuseAccum);
	EE_UNUSED_ARG(pkSpecularAccum);
	EE_UNUSED_ARG(pkOpacityAccum);
	EE_UNUSED_ARG(pkGlossiness);
	// pkFinalNormal
	EE_UNUSED_ARG(pkParallaxOffset);
	EE_UNUSED_ARG(pkMorphValue);
	EE_UNUSED_ARG(pkTotalMask);
	// pkWorldPos

	// If this assertion is hit then make sure to set the appropriate flags
	// on the descriptors to output world pos from the vert shader
	EE_ASSERT(pkWorldPos);

	NiMaterialResource* pkDepthNormalOutput = NULL;
	if (!HandlePixelGenerateNormalDepthOutput(kContext, 
		pkFinalNormal, pkWorldPos, pkSpecularPower, pkDepthNormalOutput))
	{
		return false;
	}

	// Bind output 
	NiMaterialResource* pkPixelOutColor = 
		kContext.m_spOutputs->AddInputResource("float4", "Color", "", "Color0");
	kContext.m_spConfigurator->AddBinding(pkDepthNormalOutput, pkPixelOutColor);

	return true;

}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLightPrePass::OverrideHandleFinalVertexOutputs(Context& kContext, 
	NiStandardVertexProgramDescriptor* pkVertDesc, 
	NiMaterialResource* pkWorldPos, 
	NiMaterialResource* pkWorldNormal, 
	NiMaterialResource* pkWorldReflect, 
	NiMaterialResource* pkViewPos, 
	NiMaterialResource* pkProjectedPos)
{
	EE_UNUSED_ARG(pkWorldPos);
	EE_UNUSED_ARG(pkWorldNormal);
	EE_UNUSED_ARG(pkWorldReflect);
	EE_UNUSED_ARG(pkViewPos);

	// Add projected position as a texture coordinate
	if (!HandleVertexPositionPassThrough(kContext, pkProjectedPos))
	{
		return false;
	}

	// sometimes we still need to pass vertex colors
	// because we always render per pixel lighting in this shader, so we need these values to come
	// out.
	NiStandardMaterial::AmbDiffEmissiveEnum eAmbDiffEmissive = 
		(NiStandardMaterial::AmbDiffEmissiveEnum)pkVertDesc->GetAMBDIFFEMISSIVE();

	NiStandardMaterial::LightingModeEnum eLightingMode = 
		(NiStandardMaterial::LightingModeEnum)pkVertDesc->GetLIGHTINGMODE();

	if (eAmbDiffEmissive == NiStandardMaterial::ADE_EMISSIVE ||
		(eAmbDiffEmissive == NiStandardMaterial::ADE_AMB_DIFF && eLightingMode == NiStandardMaterial::LIGHTING_E_A_D))
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

	return true;
}


//--------------------------------------------------------------------------------------------------
bool NiFragmentLightPrePass::HandleVertexPositionPassThrough(Context& kContext, 
    NiMaterialResource* pkProjectedPos)
{
    // Add projected position as a texture coordinate
    NiMaterialResource* pkVertOutProjTexCoord =
        kContext.m_spOutputs->AddInputResource("float4", "TexCoord", "World",
        "PosProjectedPassThrough");

    NiMaterialNode* pkSplitterNode = GetAttachableNodeFromLibrary("PositionPassThrough");
    EE_ASSERT(pkSplitterNode);
    kContext.m_spConfigurator->AddNode(pkSplitterNode);
    kContext.m_spConfigurator->AddBinding(pkProjectedPos,
        pkSplitterNode->GetInputResourceByVariableName("Input"));
    kContext.m_spConfigurator->AddBinding(
        pkSplitterNode->GetOutputResourceByVariableName("Output"), pkVertOutProjTexCoord);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLightPrePass::HandlePixelGenerateNormalDepthOutput(Context& kContext,
    NiMaterialResource* pkWorldNormal, 
    NiMaterialResource* pkWorldPos, 
    NiMaterialResource* pkSpecularPower,
    NiMaterialResource*& pkDepthNormalOutput)
{
    // Fetch the view matrix
    NiMaterialResource* pkViewMatrix = AddOutputPredefined(
        kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_VIEW, 4);

    NiMaterialResource* pkDepthScale = AddOutputGlobal(
        kContext.m_spUniforms, "g_fDepthScale", 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);

    // create the packed depth fragment
    NiMaterialNode* pkDepthFragment = GetAttachableNodeFromLibrary("LPPDepthNormal");
    EE_ASSERT(pkDepthFragment);
    kContext.m_spConfigurator->AddNode(pkDepthFragment);

    // Inputs
    kContext.m_spConfigurator->AddBinding(pkWorldNormal,
        pkDepthFragment->GetInputResourceByVariableName("WorldNormal"));
    kContext.m_spConfigurator->AddBinding(pkWorldPos,
        pkDepthFragment->GetInputResourceByVariableName("Position"));
    kContext.m_spConfigurator->AddBinding(pkViewMatrix,
        pkDepthFragment->GetInputResourceByVariableName("ViewMatrix"));
    kContext.m_spConfigurator->AddBinding(pkDepthScale,
        pkDepthFragment->GetInputResourceByVariableName("DepthScale"));
    kContext.m_spConfigurator->AddBinding(pkSpecularPower,
        pkDepthFragment->GetInputResourceByVariableName("SpecularPower"));

    // Outputs
    pkDepthNormalOutput = pkDepthFragment->GetOutputResourceByVariableName("Output");

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLightPrePass::HandleVertexLightingAndMaterials(Context& kContext,
    VertexDescriptor& kVertexDesc,
    NiMaterialResource* pkWorldPos,
    NiMaterialResource* pkWorldNormal,
    NiMaterialResource* pkWorldView)
{
    EE_UNUSED_ARG(kContext);
    EE_UNUSED_ARG(kVertexDesc);
    EE_UNUSED_ARG(pkWorldPos);
    EE_UNUSED_ARG(pkWorldNormal);
    EE_UNUSED_ARG(pkWorldView);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLightPrePass::HandleGenerateScreenUV(Context& kContext, 
    NiMaterialResource* pProjPos, NiMaterialResource*& pScreenUV)
{
    NiMaterialNode* pScreenUVNode = GetAttachableNodeFromLibrary("LPPScreenUV");
    EE_ASSERT(pScreenUVNode);
    kContext.m_spConfigurator->AddNode(pScreenUVNode);

    // Fetch inputs
    NiMaterialResource* pkDepthScale = AddOutputGlobal(kContext.m_spUniforms, "g_fDepthScale", 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);

    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pProjPos, "Input", pScreenUVNode);
    kContext.m_spConfigurator->AddBinding(pkDepthScale, "DepthScale", pScreenUVNode);

    // Bind Outputs
    pScreenUV = pScreenUVNode->GetOutputResourceByVariableName("Output");

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLightPrePass::HandleLighting_Terrain(
	Context& kContext,
	efd::UInt32 uiShadowAtlasCells,
	efd::UInt32 uiPSSMWhichLight,
	bool bSliceTransitions,
	bool bSpecular,
	efd::UInt32 uiNumPoint,
	efd::UInt32 uiNumDirectional,
	efd::UInt32 uiNumSpot,
	efd::UInt32 uiShadowBitfield,
	efd::UInt32 uiShadowTechnique,
	NiMaterialResource* pWorldPos,
	NiMaterialResource* pWorldNorm,
	NiMaterialResource* pViewVector,
	NiMaterialResource* pSpecularPower,
	NiMaterialResource* pProjectedPosition,
	NiMaterialResource*& pAmbientAccum,
	NiMaterialResource*& pDiffuseAccum,
	NiMaterialResource*& pSpecularAccum,
	NiFragmentLighting::ExtraLightingData kExtraData)
{
	NiMaterialNode* pSampleLightNode = 
		GetAttachableNodeFromLibrary("ReconstructLightingFromTextureForTerrain");

	// Perform the final accumulation of lighting for this pixel
	if (!HandleLighting(kContext, 
		uiShadowAtlasCells,
		uiPSSMWhichLight,
		bSliceTransitions,
		bSpecular,
		uiNumPoint,
		uiNumDirectional,
		uiNumSpot,
		uiShadowBitfield,
		uiShadowTechnique,
		pWorldPos,
		pWorldNorm,
		pViewVector,
		pSpecularPower,
		pProjectedPosition,
		pAmbientAccum,
		pDiffuseAccum,
		pSpecularAccum,
		pSampleLightNode))
	{
		return false;
	}

	// Bind all terrain inputs
	{
		NiTerrainMaterial::TerrainLightingData* pkSpecularData = 
			(NiTerrainMaterial::TerrainLightingData*)kExtraData;
		EE_ASSERT(pkSpecularData->m_uiSize = sizeof(NiTerrainMaterial::TerrainLightingData));

		NiMaterialResource* pkTrueConstant = 
			m_pkOperations->GenerateShaderConstant(kContext, true);
		NiMaterialResource* pkFalseConstant = 
			m_pkOperations->GenerateShaderConstant(kContext, false);
		for (efd::UInt32 uiLayer = 0; uiLayer < NiTerrainMaterial::TerrainLightingData::MAX_LAYERS; ++uiLayer)
		{
			// Specular Enabled
			char acEnabled[255];
			NiSnprintf(acEnabled, 255, NI_TRUNCATE, "layer%dEnabled", uiLayer);
			m_pkOperations->OptionalBind(kContext,
				pkSpecularData->m_abLayerSpecularEnabled[uiLayer] ? pkTrueConstant : pkFalseConstant, 
				acEnabled, pSampleLightNode);

			// Specular Color
			char acSpecular[255];
			NiSnprintf(acSpecular, 255, NI_TRUNCATE, "layer%dSpecular", uiLayer);
			m_pkOperations->OptionalBind(kContext,
				pkSpecularData->m_apkLayerSpecular[uiLayer], 
				acSpecular, pSampleLightNode);
		}

		m_pkOperations->OptionalBind(kContext, pkSpecularData->m_pkLayerMaskValues, 
			"maskValues", pSampleLightNode);
		m_pkOperations->OptionalBind(kContext, pkSpecularData->m_pkLayerSpecularPowers,
			"layerSpecularPowers", pSampleLightNode);
		m_pkOperations->OptionalBind(kContext, pkSpecularData->m_pkLayerSpecularIntensities, 
			"layerSpecularIntensities", pSampleLightNode);

		// Add the low detail specular data
		NiMaterialResource* pkLowDetailSpecularData = AddOutputAttribute(kContext.m_spUniforms, 
			NiTerrainCellShaderData::LOWDETAIL_SPECULAR_SHADER_CONSTANT, 
			NiShaderAttributeDesc::ATTRIB_TYPE_POINT2);

		NiMaterialResource* pkLowDetailSpecularPower = NULL;
		NiMaterialResource* pkLowDetailSpecularIntensity = NULL;
		m_pkOperations->ExtractChannel(
			kContext, pkLowDetailSpecularData, 0, pkLowDetailSpecularPower);
		m_pkOperations->ExtractChannel(
			kContext, pkLowDetailSpecularData, 1, pkLowDetailSpecularIntensity);
		EE_ASSERT(pkLowDetailSpecularPower);
		EE_ASSERT(pkLowDetailSpecularIntensity);

		m_pkOperations->OptionalBind(kContext, pkSpecularData->m_pkLowDetailSpecular, 
			"lowDetailSpecular", pSampleLightNode);
		m_pkOperations->OptionalBind(kContext, pkLowDetailSpecularPower, 
			"lowDetailSpecularPower", pSampleLightNode);
		m_pkOperations->OptionalBind(kContext, pkLowDetailSpecularIntensity, 
			"lowDetailSpecularIntensity", pSampleLightNode);
	}

	return true;

}

//--------------------------------------------------------------------------------------------------
bool NiFragmentLightPrePass::HandleLighting(
    Context& kContext,
    efd::UInt32 uiShadowAtlasCells,
    efd::UInt32 uiPSSMWhichLight,
    bool bSliceTransitions,
    bool bSpecular,
    efd::UInt32 uiNumPoint,
    efd::UInt32 uiNumDirectional,
    efd::UInt32 uiNumSpot,
    efd::UInt32 uiShadowBitfield,
    efd::UInt32 uiShadowTechnique,
    NiMaterialResource* pWorldPos,
    NiMaterialResource* pWorldNorm,
    NiMaterialResource* pViewVector,
    NiMaterialResource* pSpecularPower,
    NiMaterialResource* pProjectedPosition,
    NiMaterialResource*& pAmbientAccum,
    NiMaterialResource*& pDiffuseAccum,
    NiMaterialResource*& pSpecularAccum,
    NiMaterialNode* pSampleLightNode)
{
    EE_UNUSED_ARG(uiShadowAtlasCells);
    EE_UNUSED_ARG(uiPSSMWhichLight);
    EE_UNUSED_ARG(bSliceTransitions);
    // bool bSpecular
    EE_UNUSED_ARG(uiNumPoint);
    EE_UNUSED_ARG(uiNumDirectional);
    EE_UNUSED_ARG(uiNumSpot);
    EE_UNUSED_ARG(uiShadowBitfield);
    EE_UNUSED_ARG(uiShadowTechnique);
    EE_UNUSED_ARG(pWorldPos);
    EE_UNUSED_ARG(pWorldNorm);
    EE_UNUSED_ARG(pViewVector);
    // NiMaterialResource* pSpecularPower
    EE_UNUSED_ARG(pAmbientAccum);
    // NiMaterialResource*& pDiffuseAccum
    // NiMaterialResource*& pSpecularAccum

    // Bind the screen UV
    NiMaterialResource* pScreenUV = NULL;
    if (!HandleGenerateScreenUV(kContext, pProjectedPosition, pScreenUV))
    {
        return false;
    }

    // Create lighting node
    if (!pSampleLightNode)
        pSampleLightNode = GetAttachableNodeFromLibrary("ReconstructLightingFromTexture");
    kContext.m_spConfigurator->AddNode(pSampleLightNode);

    // get screen space UV coordinates from vertex shader
    kContext.m_spConfigurator->AddBinding(pScreenUV, "ScreenUV", pSampleLightNode);

    if ( bSpecular )
    {
        kContext.m_spConfigurator->AddBinding(
            pSpecularPower,
            pSampleLightNode->GetInputResourceByVariableName("SpecPower"));
    }

    // Use normal map semantic as lighting sampler
    NiMaterialResource* pLightTextureSampler = NULL;
    {
        NiFixedString kSamplerName;
        efd::UInt32 uiOccurance = 0;
        if (!NiStandardMaterial::GetTextureNameFromTextureEnum(
            NiStandardMaterial::MAP_LPP_LBUFFER, kSamplerName, uiOccurance))
        {
            return false;
        }

        pLightTextureSampler = InsertTextureSampler(kContext,
            kSamplerName, NiStandardMaterial::TEXTURE_SAMPLER_2D, uiOccurance);
    }
    EE_ASSERT(pLightTextureSampler);
    kContext.m_spConfigurator->AddBinding(
        pLightTextureSampler,
        pSampleLightNode->GetInputResourceByVariableName("SamplerID"));

    // Attach the outputs to Diffuse and Specular lighting accumulators
    pDiffuseAccum = pSampleLightNode->GetOutputResourceByVariableName("OutputDiffuse");
    if (bSpecular)
    {
        pSpecularAccum = pSampleLightNode->GetOutputResourceByVariableName("OutputSpecular");
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
