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

#include "NiLPPDepthNormalMaterial.h"

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
#include "NiFragmentLightPrePass.h"


NiImplementRTTI(NiLPPDepthNormalMaterial, NiStandardMaterial, NiTypeMask::NiLPPDepthNormalMaterial);
//------------------------------------------------------------------------------------------------
NiLPPDepthNormalMaterial* NiLPPDepthNormalMaterial::Create()
{
    // Fetch a previously generated NiTerrainMaterial
    NiLPPDepthNormalMaterial* pkMaterial = NiDynamicCast(NiLPPDepthNormalMaterial, 
        NiMaterial::GetMaterial("NiLPPDepthNormalMaterial"));

    if (!pkMaterial)
    {
        pkMaterial = NiNew NiLPPDepthNormalMaterial();
    }

    return pkMaterial;
}

//------------------------------------------------------------------------------------------------
NiLPPDepthNormalMaterial::NiLPPDepthNormalMaterial(bool bAutoCreateCaches)
: NiStandardMaterial("NiLPPDepthNormalMaterial", NULL,
                     LPPDEPTHNORMALMATERIAL_VERTEX_VERSION,
                     LPPDEPTHNORMALMATERIAL_GEOMETRY_VERSION,
                     LPPDEPTHNORMALMATERIAL_PIXEL_VERSION, bAutoCreateCaches)
                     , m_pkWorldNormal(NULL)
{
    m_kLibraries.Add(NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary());

    // Set the material for the fragments:
    NiFragment::Fetch(this, m_pkLightPrePass);
    NiFragment::Fetch(this, m_pkOperations);
}

//-------------------------------------------------------------------------------------------------
NiLPPDepthNormalMaterial::NiLPPDepthNormalMaterial(
    const NiFixedString& kName,
    efd::UInt32 uiVertexVersion,
    efd::UInt32 uiGeometryVersion,
    efd::UInt32 uiPixelVersion,
    bool bAutoCreateCaches)
: NiStandardMaterial(kName, NULL, uiVertexVersion, uiGeometryVersion, uiPixelVersion, bAutoCreateCaches)
, m_pkWorldNormal(NULL)
{
    m_kLibraries.Add(NiStandardMaterialNodeLibrary::CreateMaterialNodeLibrary());
    m_kLibraries.Add(NiLightPrePassMaterialNodeLibrary::CreateMaterialNodeLibrary());

    // Set the material for the fragments:
    NiFragment::Fetch(this, m_pkLightPrePass);
    NiFragment::Fetch(this, m_pkOperations);
}

//-------------------------------------------------------------------------------------------------
bool NiLPPDepthNormalMaterial::HandlePostLightTextureApplication(
    Context& kContext,
    NiStandardPixelProgramDescriptor* pPixelDesc,
    NiMaterialResource*& pWorldNormal,
    NiMaterialResource* pWorldView,
    NiMaterialResource*& pMatOpacity,
    NiMaterialResource*& pDiffuseAccum,
    NiMaterialResource*& pSpecularAccum,
    NiMaterialResource* pGlossiness,
    efd::UInt32& numTexturesApplied,
    NiMaterialResource** apkUVSets,
    efd::UInt32 numStandardUVs,
    efd::UInt32 numTexEffectUVs)
{
    if (!NiStandardMaterial::HandlePostLightTextureApplication(
        kContext,
        pPixelDesc,
        pWorldNormal,
        pWorldView,
        pMatOpacity,
        pDiffuseAccum,
        pSpecularAccum,
        pGlossiness,
        numTexturesApplied,
        apkUVSets,
        numStandardUVs,
        numTexEffectUVs))
    {
        return false;
    }

    // Copy out the world normal and store it for later use
    m_pkWorldNormal = pWorldNormal;

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiLPPDepthNormalMaterial::HandlePixelInputs(
    Context& kContext,
    NiStandardPixelProgramDescriptor* pkPixelDesc,
    NiMaterialResource*& pkPixelWorldPos,
    NiMaterialResource*& pkPixelWorldNorm,
    NiMaterialResource*& pkPixelWorldBinormal,
    NiMaterialResource*& pkPixelWorldTangent,
    NiMaterialResource*& pkPixelWorldViewVector,
    NiMaterialResource*& pkPixelTangentViewVector)
{
    if (!NiStandardMaterial::HandlePixelInputs(kContext, pkPixelDesc,
        pkPixelWorldPos, pkPixelWorldNorm, pkPixelWorldBinormal,
        pkPixelWorldTangent, pkPixelWorldViewVector,
        pkPixelTangentViewVector))
    {
        return false;
    }

	

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiLPPDepthNormalMaterial::HandleNormalMap(
    Context& kContext,
    NiMaterialResource* pkUVSet,
    NormalMapType eType,
    NiMaterialResource*& pkWorldNormal,
    NiMaterialResource* pkWorldBinormal,
    NiMaterialResource* pkWorldTangent)
{
    if (!NiStandardMaterial::HandleNormalMap(kContext, pkUVSet, eType,
        pkWorldNormal, pkWorldBinormal, pkWorldTangent))
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiLPPDepthNormalMaterial::HandleFinalVertexOutputs(
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

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiLPPDepthNormalMaterial::HandleViewProjectionFragment(
    Context& kContext,
    bool bForceViewPos,
    NiMaterialResource* pkVertWorldPos,
    NiMaterialResource*& pkVertOutProjectedPos,
    NiMaterialResource*& pkVertOutViewPos)
{
    if (!NiStandardMaterial::HandleViewProjectionFragment(kContext,
        bForceViewPos, pkVertWorldPos, pkVertOutProjectedPos,
        pkVertOutViewPos))
    {
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------------------------
bool NiLPPDepthNormalMaterial::HandleFinalPixelOutputs(
    Context& kContext,
    NiStandardPixelProgramDescriptor* pkPixelDesc,
    NiMaterialResource* pkDiffuseAccum,
    NiMaterialResource* pkSpecularAccum,
    NiMaterialResource* pkOpacityAccum)
{
    if (!m_pkLightPrePass->HandleFinalPixelOutputs_DepthNormal(kContext, 
		pkPixelDesc, 
		pkDiffuseAccum,
		pkSpecularAccum,
		pkOpacityAccum,
		m_pkWorldNormal))
	{
		return false;
	}

	m_pkWorldNormal = NULL;

    return true;
}

//-------------------------------------------------------------------------------------------------
NiFragmentMaterial::ReturnCode NiLPPDepthNormalMaterial::GenerateShaderDescArray(
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

    // Munge the descriptors to remove most traces of maps, etc...
    NiStandardMaterialDescriptor* pkMatlDesc =
        (NiStandardMaterialDescriptor*) pMaterialDescriptor;

    NiStandardVertexProgramDescriptor* pVertexDesc =
        (NiStandardVertexProgramDescriptor*)pRenderPasses[0].m_pkVertexDesc;

    NiStandardPixelProgramDescriptor* pPixelDesc =
        (NiStandardPixelProgramDescriptor*)pRenderPasses[0].m_pkPixelDesc;

    pVertexDesc->SetPOINTLIGHTCOUNT(0);
    pVertexDesc->SetDIRLIGHTCOUNT(0);
    pVertexDesc->SetSPOTLIGHTCOUNT(0);
    pVertexDesc->SetVERTEXLIGHTSONLY(0);
    pVertexDesc->SetOUTPUTWORLDNBT(1);
    pVertexDesc->SetOUTPUTWORLDVIEW(0);
    pVertexDesc->SetOUTPUTWORLDPOS(1);

    pPixelDesc->SetPOINTLIGHTCOUNT(0);
    pPixelDesc->SetDIRLIGHTCOUNT(0);
    pPixelDesc->SetSPOTLIGHTCOUNT(0);
    pPixelDesc->SetPERVERTEXLIGHTING(0);
    pPixelDesc->SetWORLDNORMAL(1);
    pPixelDesc->SetWORLDVIEW(0);
    pPixelDesc->SetWORLDPOSITION(1);

    // Disable the alpha hardware test if there is one:
    if (pkMatlDesc->GetALPHATEST())
    {
        pRenderPasses->m_bAlphaTestOverride = true;
        pRenderPasses->m_bAlphaTest = false;
    }

    return eResult;
}

//-------------------------------------------------------------------------------------------------
bool NiLPPDepthNormalMaterial::GenerateDescriptor(
    const NiRenderObject* pGeometry,
    const NiPropertyState* pState,
    const NiDynamicEffectState* pEffects,
    NiMaterialDescriptor& kMaterialDesc)
{
    bool success = NiStandardMaterial::GenerateDescriptor(
        pGeometry,
        pState,
        pEffects,
        kMaterialDesc);

    if (!success)
        return false;

    NiStandardMaterialDescriptor* pDesc = (NiStandardMaterialDescriptor*)&kMaterialDesc;

    // Force various things to be computed per-pixel
    pDesc->SetPERVERTEXFORLIGHTS(0);

    efd::SInt32 activeMaps = 0;

    // Get rid of all of the maps except for the normal maps
    activeMaps += pDesc->GetNORMALMAPCOUNT();

    // Might need the base map if alpha testing is enabled
    if (pDesc->GetALPHATEST() == 0)
        pDesc->SetBASEMAPCOUNT(0);
    else
        activeMaps += pDesc->GetBASEMAPCOUNT();

    pDesc->SetPARALLAXMAPCOUNT(0);
    pDesc->SetDARKMAPCOUNT(0);
    pDesc->SetDETAILMAPCOUNT(0);
    pDesc->SetBUMPMAPCOUNT(0);
    pDesc->SetGLOSSMAPCOUNT(0);
    pDesc->SetGLOWMAPCOUNT(0);
    pDesc->SetCUSTOMMAP00COUNT(0);
    pDesc->SetCUSTOMMAP01COUNT(0);
    pDesc->SetCUSTOMMAP02COUNT(0);
    pDesc->SetCUSTOMMAP03COUNT(0);
    pDesc->SetCUSTOMMAP04COUNT(0);
    pDesc->SetDECALMAPCOUNT(0);
    pDesc->SetPROJLIGHTMAPCOUNT(0);
    pDesc->SetPROJSHADOWMAPCOUNT(0);

    pDesc->SetENVMAPTYPE(TEXEFFECT_NONE);

    if (activeMaps == 0)
    {
        pDesc->SetINPUTUVCOUNT(0);
    }
    else
    {
        // TODO could have an alpha tested NiMaterial
        //      that also has a normal map and uses separate UVs;
        //      for this we would need an INPUTUVCOUNT of 2.
        //      Figure out how to account for this.
        pDesc->SetINPUTUVCOUNT(1);
    }

    return true;
}

//------------------------------------------------------------------------------------------------
