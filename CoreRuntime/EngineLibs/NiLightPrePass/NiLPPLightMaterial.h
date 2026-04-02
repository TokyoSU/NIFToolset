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
#ifndef NiLPPLightMaterial_H
#define NiLPPLightMaterial_H

#include "NiLightPrePassLibType.h"
#include <NiStandardMaterial.h>

/**
    This NiMaterial renders light volumes to the lighting buffer.
    Additive alpha should be used so that light accumulates.
 */
class NILIGHTPREPASS_ENTRY NiLPPLightMaterial : public NiStandardMaterial
{
    NiDeclareRTTI;

public:
    static NiLPPLightMaterial* Create();

    NiLPPLightMaterial(bool bAutoCreateCaches = true);
protected:
    enum
    {
        LPPLightMaterial_VERTEX_VERSION = 2,
        LPPLightMaterial_GEOMETRY_VERSION = 1,
        LPPLightMaterial_PIXEL_VERSION = 4
    };

    virtual bool SetupTransformPipeline(Context& kContext,
        NiMaterialResource* pkVertOutProjPos,
        NiStandardVertexProgramDescriptor* pkVertDesc, bool bForceView,
        bool bForceViewPos, NiMaterialResource*& pkWorldPos,
        NiMaterialResource*& pkViewPos, NiMaterialResource*& pkProjectedPos,
        NiMaterialResource*& pkWorldNormal, NiMaterialResource*& pkWorldView);

    virtual bool HandleFinalVertexOutputs(Context& kContext,
        NiStandardVertexProgramDescriptor* pkVertDesc,
        NiMaterialResource* pkWorldPos,
        NiMaterialResource* pkWorldNormal,
        NiMaterialResource* pkWorldReflect,
        NiMaterialResource* pkViewPos,
        NiMaterialResource* pkProjectedPos);

    virtual bool GeneratePixelShadeTree(Context& kContext,
        NiGPUProgramDescriptor* pkDesc);

    bool HandlePixelGBuffer(Context& kContext,
        NiMaterialResource*& pkPosition,
        NiMaterialResource*& pkNormal,
        NiMaterialResource*& pkWorldView,
        NiMaterialResource*& pkSpecularPower);

    virtual bool HandleFinalPixelOutputs(Context& kContext,
        NiStandardPixelProgramDescriptor* pkPixDesc,
        NiMaterialResource* pkDiffuseAccum,
        NiMaterialResource* pkSpecularAccum,
		NiMaterialResource* pkAmbientAccum,
        NiMaterialResource* pkOpacityAccum);

    virtual bool GenerateDescriptor(
        const NiRenderObject* pGeometry,
        const NiPropertyState* pState,
        const NiDynamicEffectState* pEffects,
        NiMaterialDescriptor& kMaterialDesc);

    virtual ReturnCode GenerateShaderDescArray(
        NiMaterialDescriptor* pkMaterialDescriptor,
        RenderPassDescriptor* pkRenderPasses,
        efd::UInt32 uiMaxCount,
        efd::UInt32& uiCountAdded);
};

EE_DECLARE_SMART_POINTER(NiLPPLightMaterial);

#endif  // #ifndef NiLPPLightMaterial_H
