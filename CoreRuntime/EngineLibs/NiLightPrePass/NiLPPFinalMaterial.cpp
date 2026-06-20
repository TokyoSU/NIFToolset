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

#include "NiLPPFinalMaterial.h"

#include <NiStandardMaterialDescriptor.h>
#include <NiStandardMaterialNodeLibrary.h>
#include <NiStandardPixelProgramDescriptor.h>
#include <NiStandardVertexProgramDescriptor.h>
#include <NiRenderer.h>
#include <NiMaterialFragmentNode.h>
#include <NiMaterialNodeLibrary.h>
#include <NiMaterialResource.h>
#include <NiCodeBlock.h>

#include "NiLightPrePassMaterialNodeLibrary.h"
#include "NiFragmentOperations.h"
#include "NiFragmentLighting.h"
#include "NiFragmentLightPrePass.h"

//------------------------------------------------------------------------------------------------
NiImplementRTTI(NiLPPFinalMaterial, NiStandardMaterial, NiTypeMask::NiLPPFinalMaterial);
//------------------------------------------------------------------------------------------------
NiLPPFinalMaterial* NiLPPFinalMaterial::Create()
{
    // Fetch a previously generated NiTerrainMaterial
    NiLPPFinalMaterial* pkMaterial = NiDynamicCast(NiLPPFinalMaterial, 
        NiMaterial::GetMaterial("NiLPPFinalMaterial"));

    if (!pkMaterial)
    {
        pkMaterial = NiNew NiLPPFinalMaterial();
    }

    return pkMaterial;
}
//------------------------------------------------------------------------------------------------
NiLPPFinalMaterial::NiLPPFinalMaterial(bool bAutoCreateCaches) :
    NiStandardMaterial("NiLPPFinalMaterial", NULL,
                     LPPFinalMaterial_VERTEX_VERSION,
                     LPPFinalMaterial_GEOMETRY_VERSION,
                     LPPFinalMaterial_PIXEL_VERSION, bAutoCreateCaches)
{
    m_bForcePerPixelLighting = true;

    m_kLibraries.Add(NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary());

    // Set the material for the fragments:
    NiFragment::Fetch(this, m_pkLightPrePass);
    NiFragment::Fetch(this, m_pkLighting);
    NiFragment::Fetch(this, m_pkOperations);
}

//------------------------------------------------------------------------------------------------
NiLPPFinalMaterial::NiLPPFinalMaterial(
    const NiFixedString& kName,
    efd::UInt32 uiVertexVersion,
    efd::UInt32 uiGeometryVersion,
    efd::UInt32 uiPixelVersion,
    bool bAutoCreateCaches)
: NiStandardMaterial(kName, NULL, uiVertexVersion, uiGeometryVersion, uiPixelVersion, bAutoCreateCaches)
{
    m_bForcePerPixelLighting = true;

    m_kLibraries.Add(NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary());

    // Set the material for the fragments:
    NiFragment::Fetch(this, m_pkLightPrePass);
    NiFragment::Fetch(this, m_pkLighting);
    NiFragment::Fetch(this, m_pkOperations);
}

//------------------------------------------------------------------------------------------------
bool NiLPPFinalMaterial::HandleFinalVertexOutputs(
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
bool NiLPPFinalMaterial::HandleLighting(
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
bool NiLPPFinalMaterial::HandleVertexLightingAndMaterials(Context& kContext,
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
bool NiLPPFinalMaterial::HandleFinalPixelOutputs(
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

    NiStandardMaterial::HandleFinalPixelOutputs(
        kContext,
        pkPixelDesc,
        pkDiffuseAccum,
        pkSpecularAccum,
        pkOpacityAccum);

    return true;
}

//-------------------------------------------------------------------------------------------------
NiFragmentMaterial::ReturnCode NiLPPFinalMaterial::GenerateShaderDescArray(
    NiMaterialDescriptor* pMaterialDescriptor,
    RenderPassDescriptor* pRenderPasses,
    efd::UInt32 maxCount,
    efd::UInt32& countAdded)
{
    NiFragmentMaterial::ReturnCode eResult = NiStandardMaterial::GenerateShaderDescArray(
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
bool NiLPPFinalMaterial::GenerateDescriptor(const NiRenderObject* pkMesh,
    const NiPropertyState* pkPropState,
    const NiDynamicEffectState* pkEffectState,
    NiMaterialDescriptor& kMaterialDesc)
{
    bool result =
        NiStandardMaterial::GenerateDescriptor(pkMesh, pkPropState, pkEffectState, kMaterialDesc);

	NiStandardMaterialDescriptor* pkDesc = (NiStandardMaterialDescriptor*)
		&kMaterialDesc;

	// disable vertex lighting
	pkDesc->SetPERVERTEXFORLIGHTS(0);

    return result;
}

//------------------------------------------------------------------------------------------------
