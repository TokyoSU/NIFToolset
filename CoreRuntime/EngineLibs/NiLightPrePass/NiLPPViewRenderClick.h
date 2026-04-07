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
#ifndef NiLPPViewRenderClick_H
#define NiLPPViewRenderClick_H

#include "NiLightPrePassLibType.h"
#include <NiViewRenderClick.h>
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

#include "NiLPPLightRenderer.h"

/**
    Performs rendering of Ni3DRenderViews with a light prepass.
    This has four stages:
    1. G-Buffer pass: storing depth and normal information.
    2. Lighting pass: accumulates lighting in screen-space.
    3. Finish pass: renders objects with lighting from 2.
    4. Forward pass: renders objects exempt from LPP (e.g. alpha blended objects)

    The user may create a GBuffer and a Lighting buffer manually. Alternatively,
    the user may allow LPP to create it's own buffers based on the assigned output
    render target.
 */
class NILIGHTPREPASS_ENTRY NiLPPViewRenderClick : public NiViewRenderClick
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:
    /// Constructor
    NiLPPViewRenderClick(bool bInitializeSwaps = true);
    /// Destructor
    ~NiLPPViewRenderClick();

    /**
        DebugMode enumeration to adjust the output from LPP.
        The various values will make LPP output it's different stages
        to the output render target. 
    */
    enum DebugMode
    {
        /// Output the final result of LPP with no modifications.
        DEBUG_NONE,

        /// Output the content stored in the GBuffer to the Output render target.
        DEBUG_GBUFFER,

        /// Output the content stored in the LBuffer to the Output render target.
        DEBUG_LIGHTING,

        /// Render LPP normally, but do not render the objects that are exempt from it.
        DEBUG_NOFORWARD,

        /// Render LPP normally, and additionally overlay the geometry used to render 
        /// lights in wireframe.
        DEBUG_LIGHTS
    };

	/**
		RenderMode enumeration to adjust the output from LPP.
		The different values change when the forward rendering occurs
	*/
	enum RenderMode
	{
		/// Run the forward rendering after LPP Final
		FORWARD_LAST,

		/// Run forward rendering interleaved with LPP Final
		FORWARD_INTERLEAVED
	};

    /**
        Set the debug mode of the LPP render click. This allows debugging of the process
        used to render a sceen using light pre pass techniques.
    
        @param mode The mode to use.
    */
    void SetDebugMode(DebugMode mode);

	/**
		Set the Render Mode of the LPP render click.

		@param mode the mode to use
	*/
	void SetRenderMode(RenderMode mode);

    /**
        Set a user specified RGBA buffer to contain Normal and Depth information
    
        @param pkTarget The render target to render to the texture
        @param pkTexture The texture
    */
    void SetGBuffer(NiRenderTargetGroup* pkTarget, NiRenderedTexture* pkTexture);

    /**
        Set a user specified RGBA buffer to contain the lighting precalculation.
        It is recommended that this buffer be 16bit if possible.

        @param pkTarget The render target to render to the texture
        @param pkTexture The texture
    */ 
    void SetLightingBuffer(NiRenderTargetGroup* pkTarget, NiRenderedTexture* pkTexture);

#if defined(EE_PLATFORM_XBOX360) && defined(XBOX360_LPP_TILING)

    /**
        Set a user specified resolvable depth buffer to use with the 360 when using tiling.
        
        @param pkBuffer The buffer
    */ 
    void SetGBufferDepth(ResolvableDepthStencilBuffer* pkBuffer);

#endif

    /// @see NiViewRenderClick
    virtual efd::UInt32 GetNumObjectsDrawn() const;
    /// @see NiViewRenderClick
    virtual efd::Float32 GetCullTime() const;
    /// @see NiViewRenderClick
    virtual efd::Float32 GetRenderTime() const;

protected:
    // Derived from NiViewRenderClick
    virtual void PerformRendering(unsigned int uiFrameID);

    // Internal render functions
    void CreateBuffers();
    void InitializeShaderConstants();
    void ShutdownShaderConstants();
    void GenerateLists(unsigned int uiFrameID);
    void GenerateLights();
    void UseTarget(NiRenderTargetGroup* pkTarget, efd::UInt32 uiClearMode);
    void RenderPasses(); 

    // Render Passes
    void RenderGBuffer();
    void RenderLighting();
    void DebugLighting();
	void RenderLPP(Ni3DRenderView* pView, efd::UInt32 viewIndex);
	void RenderForward(Ni3DRenderView* pView, efd::UInt32 viewIndex);

    // Configuration data
    DebugMode m_eDebug;
	RenderMode m_eRenderMode;
    bool m_bFirstRender;
    efd::Float32 m_fDepthRange;
    efd::Float32 m_fDepthBias;

    // Render target/texture information
    NiRenderTargetGroupPtr m_spTargetGBuffer;
    NiRenderTargetGroupPtr m_spTargetLighting;
    NiRenderedTexturePtr m_spTextureGBuffer;
    NiRenderedTexturePtr m_spTextureLighting;

    // Material smart pointers and swap processors
    NiMaterialPtr m_spDepthNormalMaterial;
    NiMaterialPtr m_spFinalMaterial;
    NiMaterialSwapProcessorPtr m_spSwapG;
    NiMaterialSwapProcessorPtr m_spSwapF;

    // Render Pass temporary information and helper objects
    NiRenderer* m_pkRenderer;
	
    //NiVisibleArray m_kListLPP;
    typedef efd::vector<NiVisibleArray*> VisibleArrayVector;
	VisibleArrayVector m_kViewForwardArrays;
	VisibleArrayVector m_kViewLPPArrays;
    //NiVisibleArray m_kListForward;
    NiTPointerList<NiLight*> m_kLights;
    NiVisibleArray m_kVisibleOutput;
    NiLPPLightRenderer m_kLightRenderer;
    NiAlphaAccumulator m_kAccumulator;

    // Statistic tracking
    efd::UInt32 m_iDraws;
    efd::Float32 m_fCullTime;
    efd::Float32 m_fRenderTime;
	efd::UInt32 GetLPPDraws() const;
	efd::UInt32 GetForwardDraws() const;
    
    // Static shader constant strings
    NiFixedString m_kSCDepthScale;

    // XBox 360 variables
#if defined(EE_PLATFORM_XBOX360) && defined(XBOX360_LPP_TILING)
    ResolvableDepthStencilBuffer* m_spGBufferDepth;
#endif
    
};

EE_DECLARE_SMART_POINTER(NiLPPViewRenderClick);

#endif // NiLPPViewRenderClick_H
