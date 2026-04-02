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
#ifndef NiLPPTerrainDepthNormalMaterial_H
#define NiLPPTerrainDepthNormalMaterial_H

#include "NiLightPrePassLibType.h"
#include <NiTerrainMaterial.h>

class NiFragmentLightPrePass;
class NiFragmentOperations;

/**
    This NiMaterial renders the depth and normal of an object in camera-relative space.
 */
class NILIGHTPREPASS_ENTRY NiLPPTerrainDepthNormalMaterial : public NiTerrainMaterial
{
    NiDeclareRTTI;

public:
    static NiLPPTerrainDepthNormalMaterial* Create();

    NiLPPTerrainDepthNormalMaterial(bool bAutoCreateCaches = true);

protected:
    
    // constructor for derived classes
    NiLPPTerrainDepthNormalMaterial(
        const NiFixedString& kName,
        efd::UInt32 uiVertexVersion,
        efd::UInt32 uiGeometryVersion,
        efd::UInt32 uiPixelVersion,
        bool bAutoCreateCaches);

    enum
    {
        LPPDEPTHNORMALMATERIAL_VERTEX_VERSION = 1,
        LPPDEPTHNORMALMATERIAL_GEOMETRY_VERSION = 1,
        LPPDEPTHNORMALMATERIAL_PIXEL_VERSION = 2
    };

    // overridden virtuals

    bool HandleFinalPixelOutputs(Context& kContext, 
        NiTerrainMaterialPixelDescriptor* pkPixelDesc,
        NiMaterialResource* pkDiffuseAccum,
        NiMaterialResource* pkSpecularAccum,
        NiMaterialResource* pkOpacityAccum, NiMaterialResource* pkGlossiness,
        NiMaterialResource* pkFinalNormal, NiMaterialResource* pkParallaxOffset, 
        NiMaterialResource* pkMorphValue, NiMaterialResource* pkTotalMask,
        NiMaterialResource* pkWorldPos, NiMaterialResource* pkSpecularPower);

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
};

EE_DECLARE_SMART_POINTER(NiLPPTerrainDepthNormalMaterial);

#endif  // #ifndef NiLPPTerrainDepthNormalMaterial_H
