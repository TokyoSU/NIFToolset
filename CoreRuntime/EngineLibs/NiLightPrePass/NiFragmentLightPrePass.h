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

#ifndef NiFragmentLightPrePass_H
#define NiFragmentLightPrePass_H

#include "NiFragment.h"
#include "NiFragmentLighting.h"
#include "NiTerrainMaterialPixelDescriptor.h"
#include <NiTerrainMaterial.h>
/**
    This class is another helper NiFragment class used to provide basic vector operation
    functionality to any materials that reference it. 
*/
class NILIGHTPREPASS_ENTRY NiFragmentLightPrePass : public NiFragment
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:

    typedef NiFragmentLighting::VertexDescriptor VertexDescriptor;

    /// Constructor
    NiFragmentLightPrePass();

	void FetchDependencies();

    /// An enum storing the set of shader versions for this material
    enum ShaderVersion
    {
        /// The current vertex version. Adjusting this invalidates the vertex
        /// cache and forces new shaders to be generated.
        VERTEX_VERSION = 1,
        /// The current geometry version. Adjusting this invalidates the
        /// geometry cache and forces new shaders to be generated.
        GEOMETRY_VERSION = 0,
        /// The current pixel version. Adjusting this invalidates the pixel
        /// cache and forces new shaders to be generated.
        PIXEL_VERSION = 1,
    };

    /// Override StandardMaterial's pixel lighting functonality
    /// with a texture lookup into the L Buffer.
    bool HandlePixelLighting(Context& kContext,
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
		NiMaterialResource*& pkDiffuseAccum);

    /// Override StandardMaterial's final pixel outputs for
    /// outputing to the G Buffer
	bool HandleFinalPixelOutputs_DepthNormal(Context& kContext,
		NiStandardPixelProgramDescriptor* pkPixelDesc,
		NiMaterialResource* pkDiffuseAccum,
		NiMaterialResource* pkSpecularAccum,
		NiMaterialResource* pkOpacityAccum,
		NiMaterialResource* pkWorldNormal);

    bool HandleFinalPixelOutputs_Final(Context& kContext,
        NiStandardPixelProgramDescriptor* pkPixelDesc,
        NiMaterialResource* pkDiffuseAccum,
        NiMaterialResource* pkSpecularAccum,
        NiMaterialResource* pkOpacityAccum);

    /// Override NiTerrainMaterial's final pixel outputs
    /// for outputing to the G Buffer
	bool HandleFinalPixelOutputs_DepthNormal(Context& kContext, 
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
        NiMaterialResource* pkSpecularPower);

    /// Override StandardMaterials's vertex outputs to include
    /// the ProjectedPosition in a texCoord. 
	bool OverrideHandleFinalVertexOutputs(Context& kContext,
		NiStandardVertexProgramDescriptor* pkVertDesc,
		NiMaterialResource* pkWorldPos,
		NiMaterialResource* pkWorldNormal,
		NiMaterialResource* pkWorldReflect,
		NiMaterialResource* pkViewPos,
		NiMaterialResource* pkProjectedPos);

    /// Handle adding the projected position pass through to a shader
    bool HandleVertexPositionPassThrough(Context& kContext, NiMaterialResource* pkProjectedPos);

    /// Handle the generation of a color to store the given normal and depth
    bool HandlePixelGenerateNormalDepthOutput(Context& kContext, NiMaterialResource* pkWorldNormal, 
        NiMaterialResource* pkWorldPos, NiMaterialResource* pkSpecularPower,
        NiMaterialResource*& pkDepthNormalOutput);

    /// Override the handling of vertex lighting on the shader
    bool HandleVertexLightingAndMaterials(Context& kContext,
        VertexDescriptor& kVertexDesc,
        NiMaterialResource* pkWorldPos,
        NiMaterialResource* pkWorldNormal,
        NiMaterialResource* pkWorldView);

    /// Handle lighting for terrain
	bool HandleLighting_Terrain(
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
		NiFragmentLighting::ExtraLightingData kExtraData);

    /// Handle lighting for standard materials
    bool HandleLighting(
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
        NiMaterialNode* pSampleLightNode = NULL);

    /// Handle generation of screen UV coords based on the projected position
    bool HandleGenerateScreenUV(Context& kContext, 
        NiMaterialResource* pProjPos, NiMaterialResource*& pScreenUV);

protected:
	NiFragmentLighting* m_pkLighting;
	NiFragmentOperations* m_pkOperations;
};

#endif  //#ifndef NiFragmentLightPrePass_H
