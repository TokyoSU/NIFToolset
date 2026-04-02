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
#ifndef NILPPMATERIALSWAPPROCESSOR_H
#define NILPPMATERIALSWAPPROCESSOR_H

#include "NiLightPrePassLibType.h"
#include <NiRenderStep.h>
#include <NiRenderTargetGroup.h>
#include <NiRenderedTexture.h>
#include <Ni3DRenderView.h>
#include <NiMaterial.h>
#include <NiMeshScreenElements.h>
#include <NiTexturingProperty.h>
#include <NiLight.h>
#include <NiAlphaAccumulator.h>
#include <NiMaterialSwapProcessor.h>

#if defined(EE_PLATFORM_XBOX360)
#include <XenonNiRenderer.h>
// define this if using tiled surfaces
//#define XBOX360_LPP_TILING
#endif

/**
    Light Pre Pass Swap Processor for configuring objects to be drawn to the GBuffer
    during the LPP Gbuffer pass. 

    Stencil properties are temporarily modified, and the appropriate GBuffer swap material
    activated on the render objects. 
*/
class NILIGHTPREPASS_ENTRY NiLPPMaterialSwapProcessorG : public NiMaterialSwapProcessor
{
    NiDeclareRTTI;

public:
    // Constructor 
    NiLPPMaterialSwapProcessorG();

    /// @see NiMaterialSwapProcessor
    virtual void PreRenderProcessList(const NiVisibleArray* pkInput, 
        NiVisibleArray& kOutput, void* pvExtraData);

protected:
    NiTPrimitiveArray<unsigned short> kStencilFlags;
    NiTPrimitiveArray<unsigned int> kStencilRefs;
    NiTPrimitiveArray<unsigned int> kStencilMasks;
    NiTPrimitiveArray<unsigned int> kStencilWriteMasks;
};

/**
    Light Pre Pass Swap Processor for configuring objects to be drawn to the output buffer
    during the LPP output pass. 

    The appropriate LPP final swap material is temporarily activated on the render objects. 
*/
class NILIGHTPREPASS_ENTRY NiLPPMaterialSwapProcessorF : public NiMaterialSwapProcessor
{
    NiDeclareRTTI;

public:
    // Constructor 
    NiLPPMaterialSwapProcessorF();

    /// @see NiMaterialSwapProcessor
    virtual void PreRenderProcessList(const NiVisibleArray* pkInput, 
        NiVisibleArray& kOutput, void* pvExtraData);
};


#endif // NILPPMATERIALSWAPPROCESSOR_H
