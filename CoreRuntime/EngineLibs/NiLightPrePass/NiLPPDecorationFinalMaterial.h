#pragma once

#ifndef NiLPPDecorationFinalMaterial_H
#define NiLPPDecorationFinalMaterial_H

#include <NiLightPrePassLibType.h>
#include <NiDecorationMaterial.h>

class NILIGHTPREPASS_ENTRY NiLPPDecorationFinalMaterial : public NiDecorationMaterial
{
	NiDeclareRTTI;
public:
	static NiLPPDecorationFinalMaterial* Create();

protected:

	NiLPPDecorationFinalMaterial(const NiFixedString& kName = "NiLPPDecorationFinalMaterial",
		bool bAutoCreateCaches = true);

	enum
	{
		NILPPDECORATIONFINALMATERIAL_VERTEX_VERSION = 1,
		NILPPDECORATIONFINALMATERIAL_GEOMETRY_VERSION = 1,
		NILPPDECORATIONFINALMATERIAL_PIXEL_VERSION =1
	};

	virtual bool HandleFinalVertexOutputs(
		Context& kContext,
		NiStandardVertexProgramDescriptor* pkVertDesc,
		NiMaterialResource* pkWorldPos,
		NiMaterialResource* pkWorldNormal,
		NiMaterialResource* pkWorldReflect,
		NiMaterialResource* pkViewPos,
		NiMaterialResource* pkProjectedPos);

	virtual bool HandleLighting(
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
		NiMaterialResource*& pAmbientAccum,
		NiMaterialResource*& pDiffuseAccum,
		NiMaterialResource*& pSpecularAccum);

	virtual bool HandleVertexLightingAndMaterials(Context& kContext,
		NiStandardVertexProgramDescriptor* pkVertexDesc,
		NiMaterialResource* pkWorldPos,
		NiMaterialResource* pkWorldNormal,
		NiMaterialResource* pkWorldView);

    virtual bool HandleFinalPixelOutputs(Context& kContext,
        NiStandardPixelProgramDescriptor* pkPixDesc,
        NiMaterialResource* pkDiffuseAccum,
        NiMaterialResource* pkSpecularAccum,
        NiMaterialResource* pkOpacityAccum);

	virtual ReturnCode GenerateShaderDescArray(
		NiMaterialDescriptor* pNiMaterialDescriptor,
		RenderPassDescriptor* pRenderPasses,
		efd::UInt32 maxCount,
		efd::UInt32& countAdded);

	virtual bool GenerateDescriptor(const NiRenderObject* pkMesh,
		const NiPropertyState* pkPropState,
		const NiDynamicEffectState* pkEffectState,
		NiMaterialDescriptor& kMaterialDesc);

	NiFragmentLightPrePass* m_pkLightPrePass;
	NiFragmentLighting* m_pkLighting;
	NiFragmentOperations* m_pkOperations;


};

EE_DECLARE_SMART_POINTER(NiLPPDecorationFinalMaterial);

#endif