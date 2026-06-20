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

#include "NiLPPTerrainDepthNormalMaterial.h"

#include <NiTerrainMaterialDescriptor.h>
#include <NiTerrainMaterialPixelDescriptor.h>
#include <NiTerrainMaterialVertexDescriptor.h>
#include <NiRenderer.h>
#include <NiMaterialFragmentNode.h>
#include <NiTerrainMaterialNodeLibrary.h>
#include <NiMaterialNodeLibrary.h>
#include <NiMaterialResource.h>

#include "NiLightPrePassMaterialNodeLibrary.h"
#include "NiFragmentOperations.h"
#include "NiFragmentLightPrePass.h"


NiImplementRTTI(NiLPPTerrainDepthNormalMaterial, NiTerrainMaterial, NiTypeMask::NiLPPTerrainDepthNormalMaterial);
//------------------------------------------------------------------------------------------------
NiLPPTerrainDepthNormalMaterial* NiLPPTerrainDepthNormalMaterial::Create()
{
    // Fetch a previously generated NiTerrainMaterial
    NiLPPTerrainDepthNormalMaterial* pkMaterial = NiDynamicCast(NiLPPTerrainDepthNormalMaterial, 
        NiMaterial::GetMaterial("NiLPPTerrainDepthNormalMaterial"));

    if (!pkMaterial)
    {
        pkMaterial = NiNew NiLPPTerrainDepthNormalMaterial();
    }

    return pkMaterial;
}

//------------------------------------------------------------------------------------------------
NiLPPTerrainDepthNormalMaterial::NiLPPTerrainDepthNormalMaterial(bool bAutoCreateCaches)
    : NiTerrainMaterial(NULL, bAutoCreateCaches, "NiLPPTerrainDepthNormalMaterial")
{
    m_kLibraries.Add(NiTerrainMaterialNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary());

    // Set the material for the fragments:
    NiFragment::Fetch(this, m_pkLightPrePass);
}

//-------------------------------------------------------------------------------------------------
NiLPPTerrainDepthNormalMaterial::NiLPPTerrainDepthNormalMaterial(
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
}

//------------------------------------------------------------------------------------------------
bool NiLPPTerrainDepthNormalMaterial::HandleFinalPixelOutputs(Context& kContext, 
    NiTerrainMaterialPixelDescriptor* pkPixelDesc,
    NiMaterialResource* pkDiffuseAccum,
    NiMaterialResource* pkSpecularAccum,
    NiMaterialResource* pkOpacityAccum, NiMaterialResource* pkGlossiness,
    NiMaterialResource* pkFinalNormal, NiMaterialResource* pkParallaxOffset, 
    NiMaterialResource* pkMorphValue, NiMaterialResource* pkTotalMask,
    NiMaterialResource* pkWorldPos, NiMaterialResource* pkSpecularPower)
{
	if (!m_pkLightPrePass->HandleFinalPixelOutputs_DepthNormal(kContext,
		pkPixelDesc,
		pkDiffuseAccum, 
		pkSpecularAccum, 
		pkOpacityAccum, 
		pkGlossiness, 
		pkFinalNormal, 
		pkParallaxOffset, 
		pkMorphValue, 
		pkTotalMask, 
		pkWorldPos,
        pkSpecularPower))
	{
		return false;
	}

	return true;
}

//-------------------------------------------------------------------------------------------------
NiFragmentMaterial::ReturnCode NiLPPTerrainDepthNormalMaterial::GenerateShaderDescArray(
    NiMaterialDescriptor* pMaterialDescriptor,
    RenderPassDescriptor* pRenderPasses,
    efd::UInt32 maxCount,
    efd::UInt32& countAdded)
{
    NiFragmentMaterial::ReturnCode eResult = NiTerrainMaterial::GenerateShaderDescArray(
        pMaterialDescriptor,
        pRenderPasses,
        maxCount,
        countAdded);

    if (eResult != RC_SUCCESS)
        return eResult;

    // Munge the descriptors to remove most traces of maps, etc...
    NiTerrainMaterialVertexDescriptor* pVertexDesc =
        (NiTerrainMaterialVertexDescriptor*)pRenderPasses[0].m_pkVertexDesc;

    NiTerrainMaterialPixelDescriptor* pPixelDesc =
        (NiTerrainMaterialPixelDescriptor*)pRenderPasses[0].m_pkPixelDesc;

    pVertexDesc->SetPOINTLIGHTCOUNT(0);
    pVertexDesc->SetDIRLIGHTCOUNT(0);
    pVertexDesc->SetSPOTLIGHTCOUNT(0);
    pVertexDesc->SetVERTEXLIGHTSONLY(0);
    pVertexDesc->SetOUTPUT_WORLDNBT(1);
    pVertexDesc->SetOUTPUT_WORLDVIEW(1);
    pVertexDesc->SetOUTPUT_WORLDNORMAL(1);
    pVertexDesc->SetOUTPUT_WORLDPOSITION(1);

    pPixelDesc->SetPOINTLIGHTCOUNT(0);
    pPixelDesc->SetDIRLIGHTCOUNT(0);
    pPixelDesc->SetSPOTLIGHTCOUNT(0);
    pPixelDesc->SetPERVERTEXLIGHTING(0);
    pPixelDesc->SetINPUT_WORLDNBT(1);
    pPixelDesc->SetINPUT_WORLDVIEW(1);
    pPixelDesc->SetINPUT_WORLDNORMAL(1);
    pPixelDesc->SetINPUT_WORLDPOSITION(1);

    return eResult;
}

//-------------------------------------------------------------------------------------------------
bool NiLPPTerrainDepthNormalMaterial::GenerateDescriptor(
    const NiRenderObject* pGeometry,
    const NiPropertyState* pState,
    const NiDynamicEffectState* pEffects,
    NiMaterialDescriptor& kMaterialDesc)
{
    bool success = NiTerrainMaterial::GenerateDescriptor(
        pGeometry,
        pState,
        pEffects,
        kMaterialDesc);

    if (!success)
        return false;

    NiTerrainMaterialDescriptor* pkDesc = (NiTerrainMaterialDescriptor*)&kMaterialDesc;

    // Force various things to be computed per-pixel
    pkDesc->SetPERVERTEXFORLIGHTS(0);
    pkDesc->SetDIRLIGHTCOUNT(0);
    pkDesc->SetSPOTLIGHTCOUNT(0);
    pkDesc->SetPOINTLIGHTCOUNT(0);

    return true;
}

//------------------------------------------------------------------------------------------------
