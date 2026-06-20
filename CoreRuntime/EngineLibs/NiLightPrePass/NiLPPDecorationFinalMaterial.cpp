#include "NiLightPrePassPCH.h"
#include "NiLPPDecorationFinalMaterial.h"

#include <NiStandardVertexProgramDescriptor.h>
#include <NiStandardPixelProgramDescriptor.h>
#include <NiDecorationMaterialDescriptor.h>
#include "NiLightPrePassMaterialNodeLibrary.h"

using namespace efd;

//------------------------------------------------------------------------------------------------
NiImplementRTTI(NiLPPDecorationFinalMaterial, NiDecorationMaterial, NiTypeMask::NiLPPDecorationFinalMaterial);
//------------------------------------------------------------------------------------------------
NiLPPDecorationFinalMaterial* NiLPPDecorationFinalMaterial::Create()
{
	NiLPPDecorationFinalMaterial* pkMaterial = NiDynamicCast(NiLPPDecorationFinalMaterial,
		NiMaterial::GetMaterial("NiLPPDecorationFinalMaterial"));

	if (!pkMaterial)
	{
		pkMaterial = NiNew NiLPPDecorationFinalMaterial();
	}

	return pkMaterial;
	
}

//------------------------------------------------------------------------------------------------
NiLPPDecorationFinalMaterial::NiLPPDecorationFinalMaterial(const NiFixedString& kName,
	bool bAutoCreateCaches)
	: NiDecorationMaterial(kName, 
	NILPPDECORATIONFINALMATERIAL_VERTEX_VERSION,
	NILPPDECORATIONFINALMATERIAL_GEOMETRY_VERSION,
	NILPPDECORATIONFINALMATERIAL_PIXEL_VERSION,
	bAutoCreateCaches)
{

	m_kLibraries.Add(NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary());

	NiFragment::Fetch(this, m_pkLightPrePass);
	NiFragment::Fetch(this, m_pkLighting);
	NiFragment::Fetch(this, m_pkOperations);
}

//------------------------------------------------------------------------------------------------
bool NiLPPDecorationFinalMaterial::HandleFinalVertexOutputs(
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

	if (!m_pkLightPrePass->OverrideHandleFinalVertexOutputs(kContext, 
		pkVertDesc, 
		pkWorldPos, 
		pkWorldNormal, 
		pkWorldReflect, 
		pkViewPos, 
		pkProjectedPos))
	{
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
bool NiLPPDecorationFinalMaterial::HandleLighting(Context& kContext,
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
	NiMaterialResource*& pSpecularAccum)
{

	NiMaterialResource* pProjectedPosition = kContext.m_spInputs->AddOutputResource(
		"float4", "TexCoord", "World", "PosProjectedPassThrough");

	if (!m_pkLightPrePass->HandleLighting(kContext, 
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
		pSpecularAccum))
	{
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
bool NiLPPDecorationFinalMaterial::HandleVertexLightingAndMaterials(Context& kContext,
	NiStandardVertexProgramDescriptor* pkVertexDesc,
	NiMaterialResource* pkWorldPos,
	NiMaterialResource* pkWorldNormal,
	NiMaterialResource* pkWorldView)
{
	NiFragmentLighting::VertexDescriptor vertDesc;
	m_pkLighting->GetDescriptor(pkVertexDesc, vertDesc);

	if (!m_pkLightPrePass->HandleVertexLightingAndMaterials(kContext, 
		vertDesc, pkWorldPos, pkWorldNormal, pkWorldView))
	{
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------------------------
bool NiLPPDecorationFinalMaterial::HandleFinalPixelOutputs(
    Context& kContext,
    NiStandardPixelProgramDescriptor* pkPixelDesc,
    NiMaterialResource* pkDiffuseAccum,
    NiMaterialResource* pkSpecularAccum,
    NiMaterialResource* pkOpacityAccum)
{
    if (!m_pkLightPrePass->HandleFinalPixelOutputs_Final(kContext, 
        pkPixelDesc, 
        pkDiffuseAccum,
        pkSpecularAccum,
        pkOpacityAccum))
    {
        return false;
    }

    NiDecorationMaterial::HandleFinalPixelOutputs(
        kContext,
        pkPixelDesc,
        pkDiffuseAccum,
        pkSpecularAccum,
        pkOpacityAccum);

    return true;
}


//-------------------------------------------------------------------------------------------------
NiFragmentMaterial::ReturnCode NiLPPDecorationFinalMaterial::GenerateShaderDescArray(
	NiMaterialDescriptor* pMaterialDescriptor,
	RenderPassDescriptor* pRenderPasses,
	efd::UInt32 maxCount,
	efd::UInt32& countAdded)
{
	NiFragmentMaterial::ReturnCode eResult = NiDecorationMaterial::GenerateShaderDescArray(
		pMaterialDescriptor,
		pRenderPasses,
		maxCount,
		countAdded);

	if (eResult != RC_SUCCESS)
		return eResult;

    NiStandardMaterialDescriptor* pkMatlDesc =
        (NiStandardMaterialDescriptor*) pMaterialDescriptor;

	NiStandardVertexProgramDescriptor* pVertexDesc =
		(NiStandardVertexProgramDescriptor*)pRenderPasses[0].m_pkVertexDesc;

	NiStandardPixelProgramDescriptor* pPixelDesc =
		(NiStandardPixelProgramDescriptor*)pRenderPasses[0].m_pkPixelDesc;

	// disable vertex lighting
	pVertexDesc->SetVERTEXLIGHTSONLY(0);
	pPixelDesc->SetPERVERTEXLIGHTING(0);

	pVertexDesc->SetPOINTLIGHTCOUNT(0);
	pVertexDesc->SetDIRLIGHTCOUNT(0);
	pVertexDesc->SetSPOTLIGHTCOUNT(0);
	pVertexDesc->SetSPECULAR(0);

    // Disable the alpha hardware test if there is one:
    if (pkMatlDesc->GetALPHATEST())
    {
        pRenderPasses->m_bAlphaTestOverride = true;
        pRenderPasses->m_bAlphaTest = false;
    }

	return eResult;
}

//------------------------------------------------------------------------------------------------
bool NiLPPDecorationFinalMaterial::GenerateDescriptor(const NiRenderObject* pkMesh,
	const NiPropertyState* pkPropState,
	const NiDynamicEffectState* pkEffectState,
	NiMaterialDescriptor& kMaterialDesc)
{
	bool result =
		NiDecorationMaterial::GenerateDescriptor(pkMesh, pkPropState, pkEffectState, kMaterialDesc);

	NiDecorationMaterialDescriptor* pkDesc = (NiDecorationMaterialDescriptor*)
		&kMaterialDesc;

	// disable vertex lighting
	pkDesc->SetPERVERTEXFORLIGHTS(0);

	return result;
}

//------------------------------------------------------------------------------------------------
