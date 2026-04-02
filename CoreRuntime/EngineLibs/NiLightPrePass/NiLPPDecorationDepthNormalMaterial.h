#pragma once

#ifndef NiLPPDecorationDepthNormalMaterial_H
#define NILPPDecorationDepthNormalMaterial_H

#include "NiFragmentLightPrePass.h"
#include "NiLightPrePassLibType.h"

#include <NiDecorationMaterial.h>


class NILIGHTPREPASS_ENTRY NiLPPDecorationDepthNormalMaterial : public NiDecorationMaterial
{
	NiDeclareRTTI;
public:

	static NiLPPDecorationDepthNormalMaterial* Create();

protected:

	NiLPPDecorationDepthNormalMaterial(const NiFixedString& kName = "NiLPPDecorationDepthNormalMaterial",
		bool bAutoCreateCahces = true);

	enum
	{
		LPPDECORATIONDEPTHNORMALMATERIAL_VERTEX_VERSION = 1,
		LPPDECORATIONDEPTHNORMALMATERIAL_GEOMETRY_VERSION = 1,
		LPPDECORATIONDEPTHNORMALMATERIAL_PIXEL_VERSION = 1
	};

	virtual bool HandlePostLightTextureApplication(Context& kContext,
		NiStandardPixelProgramDescriptor* pkPixelDesc,
		NiMaterialResource*& pkWorldNormal,
		NiMaterialResource* pkViewVector,
		NiMaterialResource*& pkMatOpacity,
		NiMaterialResource*& pkDiffuseAccum,
		NiMaterialResource*& pkSpecularAccum,
		NiMaterialResource* pkGlossiness,
		efd::UInt32& uiTexturesApplied,
		NiMaterialResource** apkUVSets,
		efd::UInt32 uiNumStandardUVs,
		efd::UInt32 uiNumTexEffectUVs);

	virtual bool HandlePixelInputs(Context& kContext,
		NiStandardPixelProgramDescriptor* pkPixelDesc,
		NiMaterialResource*& pkPixelWorldPos,
		NiMaterialResource*& pkPixelWorldNorm,
		NiMaterialResource*& pkPixelWorldBinormal,
		NiMaterialResource*& pkPixelWorldTangent,
		NiMaterialResource*& pkPixelWorldViewVector,
		NiMaterialResource*& pkPixelTangentViewVector);

	virtual bool HandleNormalMap(Context& kContext,
		NiMaterialResource* pkUVSet,
		NormalMapType eType,
		NiMaterialResource*& pkWorldNormal,
		NiMaterialResource* pkWorldBinormal,
		NiMaterialResource* pkWorldTangent);

	virtual bool HandleFinalVertexOutputs(Context& kContext,
		NiStandardVertexProgramDescriptor* pkVertDesc,
		NiMaterialResource* pkWorldPos,
		NiMaterialResource* pkWorldNormal,
		NiMaterialResource* pkWorldReflect,
		NiMaterialResource* pkViewPos,
		NiMaterialResource* pkProjectedPos);

	virtual bool HandleViewProjectionFragment(Context& kContext,
		bool bForceViewPos,
		NiMaterialResource* pkVertWorldPos,
		NiMaterialResource*& pkVertOutProjectedPos,
		NiMaterialResource*& pkVertOutViewPos);

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

	virtual bool GenerateDescriptor(
		const NiRenderObject* pGeometry,
		const NiPropertyState* pState,
		const NiDynamicEffectState* pEffects,
		NiMaterialDescriptor& kMaterialDesc);

	NiMaterialResource* m_pkWorldNormal;
	NiFragmentLightPrePass* m_pkLightPrePass;
	NiFragmentOperations* m_pkOperations;
};

EE_DECLARE_SMART_POINTER(NiLPPDecorationDepthNormalMaterial);

#endif