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
#ifndef NiLPPDEPTHNORMALMATERIAL_H
#define NiLPPDEPTHNORMALMATERIAL_H

#include "NiLightPrePassLibType.h"
#include <NiStandardMaterial.h>

class NiFragmentLightPrePass;
class NiFragmentOperations;

/**
    This NiMaterial renders the depth and normal of an object in camera-relative space.
 */
class NILIGHTPREPASS_ENTRY NiLPPDepthNormalMaterial : public NiStandardMaterial
{
    NiDeclareRTTI;

public:
    static NiLPPDepthNormalMaterial* Create();

    NiLPPDepthNormalMaterial(bool bAutoCreateCaches = true);

protected:
    
    // constructor for derived classes
    NiLPPDepthNormalMaterial(
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

EE_DECLARE_SMART_POINTER(NiLPPDepthNormalMaterial);

#endif  // #ifndef NiLPPDEPTHNORMALMATERIAL_H
