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
#ifndef NiLPPFinalMaterial_H
#define NiLPPFinalMaterial_H

#include "NiLightPrePassLibType.h"
#include <NiStandardMaterial.h>

class NiFragmentLightPrePass;
class NiFragmentLighting;
class NiFragmentOperations;

/**
    This NiMaterial renders an object taking the lighting buffer
    and uses this for lighting instead of performing the lighting calculations.
 */
class NILIGHTPREPASS_ENTRY NiLPPFinalMaterial : public NiStandardMaterial
{
    NiDeclareRTTI;

public:
    static NiLPPFinalMaterial* Create();
    
protected:
    NiLPPFinalMaterial(bool bAutoCreateCaches = true);
    // constructor for derived classes
    NiLPPFinalMaterial(
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

    virtual bool HandleFinalPixelOutputs(
        Context& kContext,
        NiStandardPixelProgramDescriptor* pkPixelDesc,
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

EE_DECLARE_SMART_POINTER(NiLPPFinalMaterial);

#endif  // #ifndef NiLPPDEPTHNORMALMATERIAL_H
