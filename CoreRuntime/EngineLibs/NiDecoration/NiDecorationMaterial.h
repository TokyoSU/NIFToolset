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

#ifndef NIDECORATIONMATERIAL_H
#define NIDECORATIONMATERIAL_H

#include <NiStandardMaterial.h>
#include "NiDecorationLibType.h"

class NiDecorationPixelProgramDescriptor;

/**
    A simple extension to NiStandardMaterial, enabling a mesh to fade to transparent as it becomes 
    farther away from the viewport. 
    
    'Screen Door' transparency is used to remove the need for z-sorting of the meshes (which would 
    be required for alpha based transparency). A single channel 8-bit texture containing some form 
    of noise is required to give a smoother appearance to the screen door effect.

    The material also supports over saturation of the base texture, by multiplying the base texture
    samplers color by a scalar shader constant.
 */
class NIDECORATION_ENTRY NiDecorationMaterial : public NiStandardMaterial
{
    NiDeclareRTTI;

public:

    /**
        Parameterized constructor

        @see NiStandardMaterial Constructor
     */ 
    NiDecorationMaterial(bool bAutoCreateCaches = true);

    /// Constants declared for the global shader constants used by the 
    /// material.
    //@{
    static const char* FADE_OUTERMAXDISTSQR_SHADER_CONSTANT;
    static const char* FADE_OUTERMINDISTSQR_SHADER_CONSTANT;
	static const char* FADE_INNERMAXDISTSQR_SHADER_CONSTANT;
	static const char* FADE_INNERMINDISTSQR_SHADER_CONSTANT;
    static const char* DIFFUSE_SATURATION_MULTIPLIER_SHADER_CONSTANT;
    //@}

    /**
        @return NiDecorationMaterial singleton or NULL if failed to create or find valid instance.
     */
    static NiDecorationMaterial* Create();

    /// Version information for this material
    enum
    {
        NIDECORATIONMATERIAL_VERTEX_VERSION = NiStandardMaterial::VERTEX_VERSION,
        NIDECORATIONMATERIAL_PIXEL_VERSION = (NiStandardMaterial::PIXEL_VERSION << 8) + 65,
        NIDECORATIONMATERIAL_GEOMETRY_VERSION = (NiStandardMaterial::GEOMETRY_VERSION << 8) + 65,
    };

    /// Transition type for instance visibility
    enum FADETYPE
    {
        /// No transition
        FADETYPE_NONE,

        /// Screen-door pixel culling
        FADETYPE_NOISE
    };

    /// Descriptor sizes
    enum
    {
        // The number of bytes to use for material descriptor bit field.
        MATERIAL_DESCRIPTOR_BYTE_COUNT = 5,
        // The number of bytes to use for pixel program descriptor bit field.
        PIXEL_PROGRAM_DESCRIPTOR_BYTE_COUNT = 5,
    };

    // Overridden virtual functions inherit base documentation and thus
    // are not documented here.

    /// NiStandardMaterial overrides
    //@{
    virtual bool GenerateDescriptor(const NiRenderObject* pkMesh, 
        const NiPropertyState* pkPropState, 
        const NiDynamicEffectState* pkEffectState,
        NiMaterialDescriptor& kMaterialDesc);    
    virtual NiFragmentMaterial::ReturnCode GenerateShaderDescArray(
        NiMaterialDescriptor* pkMaterialDescriptor, 
        RenderPassDescriptor* pkRenderPasses, 
        unsigned int uiMaxCount, unsigned int& uiCountAdded);
    virtual bool GeneratePixelShadeTree(Context& kContext, NiGPUProgramDescriptor* pkDesc);
    virtual unsigned int GetMaterialDescriptorSize();
    virtual unsigned int GetPixelProgramDescriptorSize();
    virtual bool HandlePixelUVSets(Context& kContext,
        NiStandardPixelProgramDescriptor* pkPixelDesc,  
        NiMaterialResource** ppkUVSets, 
        unsigned int uiMaxUVIndex,
        unsigned int& uiNumStandardUVs, 
        unsigned int& uiDynamicEffectCount);
    virtual bool HandleBaseMap(Context& kContext, NiMaterialResource* pkUVSet, 
        NiMaterialResource*& pkDiffuseColorAccum, NiMaterialResource*& pkOpacity, 
        bool bOpacityOnly);
    //@}

protected:

	/// Protected constructor for derived classes
	NiDecorationMaterial(const NiFixedString& kName,
		unsigned int uiVertextVersion,
		unsigned int uiGeometryVersion,
		unsigned int uiPixelVersion,
		bool bAutoCreateCaches);
};

NiSmartPointer(NiDecorationMaterial);

#endif // NIDECORATIONMATERIAL_H
