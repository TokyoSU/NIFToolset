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

#include "NiLPPTerrainFinalMaterial.h"

#include <NiTerrainMaterialDescriptor.h>
#include <NiTerrainMaterialPixelDescriptor.h>
#include <NiTerrainMaterialVertexDescriptor.h>
#include <NiRenderer.h>
#include <NiMaterialFragmentNode.h>
#include <NiTerrainMaterialNodeLibrary.h>
#include <NiMaterialResource.h>

#include "NiLightPrePassMaterialNodeLibrary.h"
#include "NiFragmentOperations.h"
#include "NiFragmentLighting.h"
#include "NiFragmentLightPrePass.h"
#include "NiTerrainCellShaderData.h"

//------------------------------------------------------------------------------------------------
NiImplementRTTI(NiLPPTerrainFinalMaterial, NiTerrainMaterial);
//------------------------------------------------------------------------------------------------
NiLPPTerrainFinalMaterial* NiLPPTerrainFinalMaterial::Create()
{
    // Fetch a previously generated NiTerrainMaterial
    NiLPPTerrainFinalMaterial* pkMaterial = NiDynamicCast(NiLPPTerrainFinalMaterial, 
        NiMaterial::GetMaterial("NiLPPTerrainFinalMaterial"));

    if (!pkMaterial)
    {
        pkMaterial = NiNew NiLPPTerrainFinalMaterial();
    }

    return pkMaterial;
}
//------------------------------------------------------------------------------------------------
NiLPPTerrainFinalMaterial::NiLPPTerrainFinalMaterial(bool bAutoCreateCaches) :
    NiTerrainMaterial(NULL, bAutoCreateCaches, "NiLPPTerrainFinalMaterial")
{
    m_kLibraries.Add(NiTerrainMaterialNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary());

    // Set the material for the fragments:
    NiFragment::Fetch(this, m_pkLightPrePass);

    m_pkLighting->SetForcePerPixelLighting(true);
}

//------------------------------------------------------------------------------------------------
NiLPPTerrainFinalMaterial::NiLPPTerrainFinalMaterial(
    const NiFixedString& kName,
    efd::UInt32 uiVertexVersion,
    efd::UInt32 uiGeometryVersion,
    efd::UInt32 uiPixelVersion,
    bool bAutoCreateCaches)
    : NiTerrainMaterial(NULL, bAutoCreateCaches, kName)
{
    EE_UNUSED_ARG(uiVertexVersion);
    EE_UNUSED_ARG(uiGeometryVersion);
    EE_UNUSED_ARG(uiPixelVersion);

    m_kLibraries.Add(NiTerrainMaterialNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary());

    // Set the material for the fragments:
    NiFragment::Fetch(this, m_pkLightPrePass);

    m_pkLighting->SetForcePerPixelLighting(true);
}

//--------------------------------------------------------------------------------------------------
bool NiLPPTerrainFinalMaterial::HandlePixelLighting(Context& kContext,
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
	if (!m_pkLightPrePass->HandlePixelLighting(kContext,
		pkPixelDesc, 
		pkWorldPos,
		pkWorldView, 
		pkSpecularPower, 
		pkFinalNormal, 
		pkMatEmissive, 
		pkMatDiffuse, 
		pkMatAmbient,
		pkMatSpecular, 
		pkLightSpecularAccum, 
		pkLightDiffuseAccum, 
		pkLightAmbientAccum,
		pkGlossiness,
		pkFinalDiffuse,
		pkFinalSpecular,
		pkSpecularAccum,
		pkDiffuseAccum))
	{
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
bool NiLPPTerrainFinalMaterial::HandleVertexLighting(Context& kContext, 
    NiTerrainMaterialVertexDescriptor* pkVertexDesc, 
    NiMaterialResource* pkWorldPosition, 
    NiMaterialResource* pkWorldNormal, 
    NiMaterialResource* pkWorldView)
{
    // Extract lighting descriptor from the material descriptor
    NiFragmentLighting::VertexDescriptor kLightingVertDesc;
    m_pkLighting->GetDescriptor(pkVertexDesc, kLightingVertDesc);

    if (!m_pkLightPrePass->HandleVertexLightingAndMaterials(kContext, 
        kLightingVertDesc, pkWorldPosition, pkWorldNormal, pkWorldView))
    {
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------------------------------
NiFragmentMaterial::ReturnCode NiLPPTerrainFinalMaterial::GenerateShaderDescArray(
    NiMaterialDescriptor* pNiMaterialDescriptor,
    RenderPassDescriptor* pRenderPasses,
    efd::UInt32 maxCount,
    efd::UInt32& countAdded)
{
    NiFragmentMaterial::ReturnCode eResult = NiTerrainMaterial::GenerateShaderDescArray(
        pNiMaterialDescriptor,
        pRenderPasses,
        maxCount,
        countAdded);

    if (eResult != RC_SUCCESS)
        return eResult;

    NiTerrainMaterialVertexDescriptor* pVertexDesc =
        (NiTerrainMaterialVertexDescriptor*)pRenderPasses[0].m_pkVertexDesc;

    NiTerrainMaterialPixelDescriptor* pPixelDesc =
        (NiTerrainMaterialPixelDescriptor*)pRenderPasses[0].m_pkPixelDesc;

    // disable vertex lighting
    pVertexDesc->SetVERTEXLIGHTSONLY(0);
    pVertexDesc->SetPOINTLIGHTCOUNT(0);
    pVertexDesc->SetDIRLIGHTCOUNT(0);
    pVertexDesc->SetSPOTLIGHTCOUNT(0);
    pVertexDesc->SetSPECULAR(0);
    pVertexDesc->SetOUTPUT_WORLDPOSITION(1);

    pPixelDesc->SetPERVERTEXLIGHTING(0);
    pPixelDesc->SetINPUT_WORLDPOSITION(1);

    return eResult;
}

//------------------------------------------------------------------------------------------------
bool NiLPPTerrainFinalMaterial::GenerateDescriptor(const NiRenderObject* pkMesh,
    const NiPropertyState* pkPropState,
    const NiDynamicEffectState* pkEffectState,
    NiMaterialDescriptor& kMaterialDesc)
{
    bool result =
        NiTerrainMaterial::GenerateDescriptor(pkMesh, pkPropState, pkEffectState, kMaterialDesc);

    NiTerrainMaterialDescriptor* pkDesc = (NiTerrainMaterialDescriptor*)
        &kMaterialDesc;

    // disable vertex lighting
    pkDesc->SetPERVERTEXFORLIGHTS(0);

    return result;
}

//--------------------------------------------------------------------------------------------------
bool NiLPPTerrainFinalMaterial::HandleLighting_Terrain(
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
	if (!m_pkLightPrePass->HandleLighting_Terrain(kContext,
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
		pSpecularAccum,kExtraData))
	{
		return false;
	}
		
	return true;
}

//------------------------------------------------------------------------------------------------