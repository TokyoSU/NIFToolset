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

#pragma once
#ifndef NiLPPTerrainFinalMaterial_H
#define NiLPPTerrainFinalMaterial_H

#include "NiLightPrePassLibType.h"
#include <NiTerrainMaterial.h>

class NiFragmentLightPrePass;
class NiFragmentLighting;
class NiFragmentOperations;

/**
    This NiMaterial renders the terrain taking the lighting buffer
    and uses this for lighting instead of performing the lighting calculations.
 */
class NILIGHTPREPASS_ENTRY NiLPPTerrainFinalMaterial : public NiTerrainMaterial
{
    NiDeclareRTTI;

public:
    /// Create the material or fetch it if it has already been created
    static NiLPPTerrainFinalMaterial* Create();
    
protected:
    /// Constructor (to be called by Create)
    NiLPPTerrainFinalMaterial(bool bAutoCreateCaches = true);
    /// Constructor for derived classes
    NiLPPTerrainFinalMaterial(
        const NiFixedString& kName,
        efd::UInt32 uiVertexVersion,
        efd::UInt32 uiGeometryVersion,
        efd::UInt32 uiPixelVersion,
        bool bAutoCreateCaches);

    enum
    {
        LPPFinalMaterial_VERTEX_VERSION = 1,
        LPPFinalMaterial_GEOMETRY_VERSION = 1,
        LPPFinalMaterial_PIXEL_VERSION = 3
    };

    // overridden virtuals

    bool HandleVertexLighting(Context& kContext, 
        NiTerrainMaterialVertexDescriptor* pkVertexDesc, 
        NiMaterialResource* pkWorldPosition, 
        NiMaterialResource* pkWorldNormal, 
        NiMaterialResource* pkWorldView);

    bool HandlePixelLighting(Context& kContext,
        NiTerrainMaterialPixelDescriptor* pkPixelDesc, NiMaterialResource* pkWorldPos, 
        NiMaterialResource* pkWorldView, NiMaterialResource* pkSpecularPower, 
        NiMaterialResource*& pkFinalNormal, NiMaterialResource* pkMatEmissive, 
        NiMaterialResource* pkMatDiffuse, NiMaterialResource* pkMatAmbient, 
        NiMaterialResource* pkMatSpecular, NiMaterialResource* pkLightSpecularAccum, 
        NiMaterialResource* pkLightDiffuseAccum, NiMaterialResource* pkLightAmbientAccum,
        NiMaterialResource* pkGlossiness, NiMaterialResource* pkFinalDiffuse,
        NiMaterialResource* pkFinalSpecular, 
        NiMaterialResource*& pkSpecularAccum, NiMaterialResource*& pkDiffuseAccum);

    virtual ReturnCode GenerateShaderDescArray(
        NiMaterialDescriptor* pNiMaterialDescriptor,
        RenderPassDescriptor* pRenderPasses,
        efd::UInt32 maxCount,
        efd::UInt32& countAdded);

    virtual bool GenerateDescriptor(const NiRenderObject* pkMesh,
        const NiPropertyState* pkPropState,
        const NiDynamicEffectState* pkEffectState,
        NiMaterialDescriptor& kMaterialDesc);

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

    NiFragmentLightPrePass* m_pkLightPrePass;
};

EE_DECLARE_SMART_POINTER(NiLPPTerrainFinalMaterial);

#endif  // #ifndef NiLPPDEPTHNORMALMATERIAL_H
