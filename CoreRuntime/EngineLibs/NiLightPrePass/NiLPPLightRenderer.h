// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2010 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#pragma once
#ifndef NiLPPLightRenderer_H
#define NiLPPLightRenderer_H

#include "NiLightPrePassLibType.h"
#include <NiImmediateModeAdapter.h>
#include <NiImmediateModeMacro.h>
#include <efd/MemObject.h>
#include <NiMaterial.h>
#include <NiTexturingProperty.h>
#include <NiZBufferProperty.h>
#include <NiStencilProperty.h>
#include <efd/Color.h>
#include <efd/Foundation.h>
#include <NiMesh.h>
#include <NiNode.h>

class NiLight;
class NiTexture;
class NiCamera;
class NiRenderer;
class NiNode;
EE_DECLARE_SMART_POINTER(NiNode);

/**
    This class handles the rendering of light volumes. It is used by the NiLPPViewRenderClick.

    SetCamera should be called before beginning rendering of light volumes.
    RenderLight takes an NiLight, generates the appropriate volume, and renders it.
    DebugLight is like RenderLight but instead renders the volume in wireframe.
 */
class NILIGHTPREPASS_ENTRY NiLPPLightRenderer : public efd::MemObject
{
public:
    /// Constructor
    NiLPPLightRenderer();
    /// Destructor
    ~NiLPPLightRenderer();

    /**
        Render a single light into the light accumulation buffer.
        NOTE: Call set camera each frame before using render light
    
        @param pkLight The light to accumulate
        @param pkGBufferTexture The GBuffer texture
        @param pkRenderer The renderer
    */
    void RenderLight(
        NiLight* pkLight,
        NiTexture* pkGBufferTexture,
        NiRenderer* pkRenderer);

    /**
        Render a single light's volume as a wireframe mesh.
        Used for visualizing light volumes (use after the lighting pass)
    
        @param pkLight The light to render
        @param pkRenderer The renderer
    */
    void DebugLight(
        const NiLight* pkLight,
        NiRenderer* pkRenderer);

    /**
        Set the camera being used in this scene that the lights are rendered into
    
        @param pkCamera The camera
    */
    void SetCamera(NiCamera* pkCamera);

    /**
        Get the maximum range of a light given the scene setup (far plane value).
        
        @return The maximum range a light can have in the scene.
    */
    efd::Float32 GetMaxRange() const;

protected:

    /**
        A structure to describe the relevant parameters for the rendering
        of a particular light. 
    */
    struct LightData
    {
        efd::Float32 m_range;
        efd::Float32 m_falloff;
        efd::Point3 m_position;
        efd::Point3 m_direction;
        efd::Point3 m_cone;
        bool m_bDirectional;
        bool m_bSpot;
        bool m_bShadow;
        NiTransform m_kTransform;
        efd::Float32 m_shapeLength;
        efd::Float32 m_shapeRadius;
        efd::Float32 m_shapeSinAngle;
        efd::Float32 m_shapeCosAngle;
        bool m_bValid;

        LightData(const NiLight* pkLight, efd::Float32 fMaxRange);
    };

    // Testresult enumeration to describe the position of the camera relative to the light's
    // volume.
    enum TestResult {
        /// The light's volume isn't visible by the light
        TEST_CULL,
        
        /// The light's volume is entirely infront of the camera's near plane
        ///  - render front side of light volume geometry (depth <=)
        TEST_FRONT,
        
        /// The light's volume is encapsulating the camera's near plane
        ///  - render back side of light volume geometry (depth >=)
        TEST_BACK,

        /// The light's volume is intersecting the camera's near plane
        ///  - render front and back of shape (intersection)
        TEST_INTERSECT,

        /// The light's volume is affecting the entire screen
        TEST_FULLSCREEN
    };

    /// Generate the WorldSpace transform for a unit quad to place it infront of the camera
    void GenerateFullscreenUnitQuadTransform(NiTransform& transform);

    /// Test a light's volume against the camera and 
    /// determine how it's light volume should be rendered.
    TestResult TestLight(const LightData &L);

    /// Initialize shader constants
    void InitializeShaderConstants();
    /// Shutdown shader constants
    void ShutdownShaderConstants();

    // Data used for the current rendering pass
    NiCamera* m_pkCamera;
    NiTransform m_kUnitQuadWSTransform;
    NiFrustumPlanes m_kFrustumPlanes;

    // Light rendering meshes and their materials
    NiMaterialPtr m_spLightMaterial;
    NiMeshPtr m_spScreenMesh;
    NiMeshPtr m_spSphereMesh;
    NiMeshPtr m_spConeMesh;
    NiNodePtr m_spParentNode;
    
    // Shader constant strings cached here to avoid strcmps during rendering
    NiFixedString m_kSCPosScale;
    NiFixedString m_kSCProjSwitch;
    NiFixedString m_kSCCamR;
    NiFixedString m_kSCCamU;
    NiFixedString m_kSCCamD;
    NiFixedString m_kSCCamPos;
    NiFixedString m_kSCLightPos;
    NiFixedString m_kSCLightRangeQuad;

    // Static constants to modify the generation of light volumes
	static const efd::Float32 FDEG_TO_RAD;
	static const efd::UInt32 SHAPE_SLICES;
	static const efd::Float32 SHAPE_MARGIN;
};

#endif // NiLPPLIGHTRENDER_H
