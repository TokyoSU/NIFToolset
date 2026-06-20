// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

// Pre-compiled header
#include "NiTerrainPCH.h"

#include "NiTerrainCellShaderData.h"
#include "NiTerrainMaterial.h"
#include "NiTerrainMaterialDescriptor.h"
#include "NiTerrainMaterialNodeLibrary.h"
#include "NiTerrainMaterialPixelDescriptor.h"
#include "NiTerrainMaterialVertexDescriptor.h"
#include "NiTerrainSectorData.h"

#include <NiIntegersExtraData.h>
#include <NiCommonSemantics.h>
#include <NiFogProperty.h>
#include <NiRenderer.h>
#include <NiRenderObject.h>
#include <NiRenderObjectMaterialOption.h>
#include <NiShaderFactory.h>
#include <NiSourceTexture.h>
#include <NiSurface.h>
#include <NiIntegerExtraData.h>
#include <NiBooleanExtraData.h>
#include <NiFloatExtraData.h>

//---------------------------------------------------------------------------
NiImplementRTTI(NiTerrainMaterial, NiFragmentMaterial, NiTypeMask::NiTerrainMaterial);
//---------------------------------------------------------------------------
const NiColor NiTerrainMaterial::ms_kDefaultColor = NiColor(1.0f, 1.0f, 1.0f);
//---------------------------------------------------------------------------
NiTerrainMaterial* NiTerrainMaterial::Create()
{
    // Fetch a previously generated NiTerrainMaterial
    NiTerrainMaterial* pkMaterial = NiDynamicCast(NiTerrainMaterial, 
        NiMaterial::GetMaterial("NiTerrainMaterial"));

    if (!pkMaterial)
    {
        // First instance of a NiTerrainMaterial
        NiMaterialNodeLibrary* pkTerrainNodeLib = 
            NiTerrainMaterialNodeLibrary::CreateMaterialNodeLibrary();
        pkMaterial = NiNew NiTerrainMaterial(pkTerrainNodeLib);

        // Since the terrain material requires that it's cache be created during the 
        // NiFragmentMaterial constructor NiFragmentMaterial::AddReplacementShaders()
        // will be called (which is a no-op). So we need to manually call AddReplacementShaders
        // to force addition of the terrain replacement shaders in the terrain cache.
        pkMaterial->AddReplacementShaders();  

        pkMaterial->AddDefaultFallbacks();
    }

    return pkMaterial;
}
//---------------------------------------------------------------------------
NiTerrainMaterial::NiTerrainMaterial(NiMaterialNodeLibrary* pkMaterialNodeLib,
    bool bAutoCreateCaches, const NiFixedString &kName) :
    NiFragmentMaterial(pkMaterialNodeLib, kName,
    VERTEX_VERSION, GEOMETRY_VERSION, PIXEL_VERSION,
    bAutoCreateCaches),
    m_kMaterialDescriptorName("NiTerrainMaterialDescriptor"),
    m_kVertexShaderDescriptorName("NiTerrainMaterialVertexDescriptor"),
    m_kPixelShaderDescriptorName("NiTerrainMaterialPixelDescriptor")
{
    // Get the shader version we are currently running on. This is used during
    // shade tree construction to drop certain maps (normal and parallax) if we
    // are running on any 2_x shader model.
    NiGPUProgramCache* pkCache =
        m_aspProgramCaches[NiGPUProgram::PROGRAM_PIXEL];
    const NiFixedString& kShaderModel = pkCache->GetShaderProfile();

    // If we are on an SM2_x card do not allow normal and parallax maps even if
    // the profile has enough instructions to support them. This is due to
    // performance issues with SM2_x when normal and parallax maps are enabled for
    // the terrain.
    if (strstr((const char*)kShaderModel, "ps_2_"))
        m_bEnableNPOnSM2 = false;
    else
        m_bEnableNPOnSM2 = true;

    // Set the material for the fragments:
    NiFragment::Fetch(this, m_pkLighting);
    NiFragment::Fetch(this, m_pkOperations);
}

//--------------------------------------------------------------------------------------------------
NiTerrainMaterial::~NiTerrainMaterial()
{
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::GenerateDescriptor(const NiRenderObject* pkGeometry, 
    const NiPropertyState* pkPropState, 
    const NiDynamicEffectState* pkEffects,
    NiMaterialDescriptor& kMaterialDesc)
{
    // Initialize the descriptor
    NiTerrainMaterialDescriptor* pkDesc = (NiTerrainMaterialDescriptor*)
        &kMaterialDesc;    
    pkDesc->m_kIdentifier = m_kMaterialDescriptorName;

    // Detect any incompatibilities in this geometry
    if (!DetectIncompatibleObject(pkGeometry, pkPropState, pkEffects))
    {
        return false;
    }
    
    // Detect the format of the geometry streams
    bool bSupportsMorphing;
    bool bCompressed;
    bool bTangentsAvailable;
    bool bNormalsAvailable;
    NiUInt32 uiNumTexCoords;
    if (!DetectStreamFormat(pkGeometry, bNormalsAvailable, bTangentsAvailable, 
        bSupportsMorphing, bCompressed, uiNumTexCoords))
    {
        return false;
    }
    pkDesc->SetINPUT_NORMALS((bNormalsAvailable) ? 1 : 0); 
    pkDesc->SetINPUT_TANGENTS((bTangentsAvailable) ? 1 : 0);    
    pkDesc->SetINPUT_MORPHING(bSupportsMorphing ? 1 : 0);
    pkDesc->SetINPUT_LIGHTING_COMPRESSED(bCompressed ? 1 : 0);

    // Detect any settings determined by extra data
    NiUInt32 uiMorphMode;
    if (!DetectExtraData(pkGeometry, uiMorphMode))
        return false;
    else
        pkDesc->SetMORPH_MODE(uiMorphMode);

    // Handle property states...
    if (pkPropState)
    {
        // Handle fog support.
        NiFogProperty* pkFogProp = pkPropState->GetFog();
        if (pkFogProp && pkFogProp->GetFog())
        {
            switch (pkFogProp->GetFogFunction())
            {
            case NiFogProperty::FOG_Z_LINEAR:
                pkDesc->SetFOGTYPE(FOG_LINEAR);
                break;

            case NiFogProperty::FOG_RANGE_SQ:
                pkDesc->SetFOGTYPE(FOG_SQUARED);
                break;

            default:
                pkDesc->SetFOGTYPE(FOG_NONE);
            }
        } 

        // Handle texture properties.
        NiTexturingProperty* pkTexProp = pkPropState->GetTexturing();
        if (!DetectSurfaceLayerProperties(pkDesc, pkTexProp))
            return false;

        // Handle the low detail textures.
        pkDesc->SetUSE_LOWDETAILDIFFUSE(0);
        if (pkTexProp && pkTexProp->GetBaseMap())
        {
            pkDesc->SetUSE_LOWDETAILDIFFUSE(1);
        }
        pkDesc->SetUSE_LOWDETAILNORMAL(0);
        if (pkTexProp && pkTexProp->GetNormalMap())
        {
            pkDesc->SetUSE_LOWDETAILNORMAL(1);
        }
    }

    // Modify the descriptor to comply with any platform restrictions
    if (!DetectPlatformRestrictions(pkDesc, pkGeometry))
        return false;

    // Generate the lighting part of the descriptor
    bool bParallaxMapping = pkDesc->HasParallaxMapping();
    bool bNormalMapping = pkDesc->HasNormalMapping() || 
        pkDesc->GetUSE_LOWDETAILNORMAL() != NULL; // Low detail normal mapping enabled on SM2
    // Low detail always has specular enabled (unless platform doesn't support it)
    bool bSpecularMapping = true; 

    NiFragmentLighting::Descriptor kLightingDesc;
    m_pkLighting->GenerateDescriptor(pkGeometry, pkPropState, pkEffects, 
        kLightingDesc, true, bParallaxMapping, bNormalMapping);

    // Make sure we apply specular mapping if we support it
    kLightingDesc.bSpecularOn |= bSpecularMapping;
    m_pkLighting->SetDescriptor(pkDesc, kLightingDesc);

    // Modify the generated descriptor to be relevant to the selected render
    // mode
    if (!DetectRenderMode(pkDesc, pkGeometry))
        return false;

    // Modify the generated descriptor to handle any debug configuration
    // that has been selected
    if (!DetectDebugConfiguration(pkDesc, pkGeometry))
        return false;

    return true;
}
//---------------------------------------------------------------------------
bool NiTerrainMaterial::DetectIncompatibleObject(
    const NiRenderObject* pkGeometry, const NiPropertyState* pkPropState, 
    const NiDynamicEffectState* pkEffects)
{
    EE_UNUSED_ARG(pkEffects);

    if (!pkPropState)
    {
        EE_FAIL("Could not find property state! Try calling"
            " UpdateProperties.\n");
        return false;
    }

    // Make sure the terrain material is being applied to the proper geometry.
    if (pkGeometry->RequiresMaterialOption(NiRenderObjectMaterialOption::TRANSFORM_SKINNED()))
    {
        EE_FAIL("Cannot apply terrain material to skinned geometry.\n");
        return false;
    }

    if (pkGeometry->RequiresMaterialOption(NiRenderObjectMaterialOption::TRANSFORM_INSTANCED()))
    {
        EE_FAIL("Cannot apply terrain material to instanced geometry.\n");
        return false;
    }

    if (pkGeometry->RequiresMaterialOption(NiRenderObjectMaterialOption::MORPHING()))
    {
        EE_FAIL("Cannot apply terrain material to morphed geometry.\n");
        return false;
    }

    // Count up the number of texture coordinate sets on the geometry.
    NiUInt32 uiNumTexCoordSets = pkGeometry->GetSemanticCount(NiCommonSemantics::TEXCOORD());
    if (uiNumTexCoordSets == 0)
    {
        EE_FAIL("Terrain geometry must have at least one set of texture "
            "coordinates.");
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------
bool NiTerrainMaterial::DetectStreamFormat(const NiRenderObject* pkGeometry, 
    bool& bNormalsAvailable, bool& bTangentsAvailable, 
    bool& bSupportsMorphing, bool& bCompressed, NiUInt32& uiNumTexCoords)
{
    // Default return values for these settings
    bSupportsMorphing = false;
    bCompressed = false;
    bNormalsAvailable = false;
    bTangentsAvailable = false;
    uiNumTexCoords = 0;

    // Detect if normals are present
    bNormalsAvailable = 
        pkGeometry->ContainsData(NiCommonSemantics::NORMAL());

    // Detect the number of texture coordinates on this object
    uiNumTexCoords = pkGeometry->GetSemanticCount(
        NiCommonSemantics::TEXCOORD());

    // Attempt conversion of this object to a mesh
    NiMesh* pkMesh = NiDynamicCast(NiMesh, pkGeometry);
    if (pkMesh)
    {
        bSupportsMorphing = true;
        bCompressed = false;
        NiDataStreamElement kCurElement;
        NiDataStreamRef* pkRef;

        bSupportsMorphing &= pkMesh->FindStreamRefAndElementBySemantic(
            NiCommonSemantics::POSITION(), 
            0, NiDataStreamElement::F_UNKNOWN, pkRef, kCurElement);
        bSupportsMorphing &= kCurElement.GetComponentCount() == 4;

        pkMesh->FindStreamRefAndElementBySemantic(
            NiCommonSemantics::NORMAL(), 
            0, NiDataStreamElement::F_UNKNOWN, pkRef, kCurElement);
        bCompressed |= kCurElement.GetComponentCount()== 2;

        // Tangents are optional:
        if (pkMesh->FindStreamRefAndElementBySemantic(
            NiCommonSemantics::TANGENT(), 
            0, NiDataStreamElement::F_UNKNOWN, pkRef, kCurElement))
        {
            bTangentsAvailable = true;
            bCompressed |= kCurElement.GetComponentCount()== 2;
        }
    }
    else
    {
        // Check the object for tangents available somewhere.
        for (NiUInt32 uiSemanticIndex = 0; uiSemanticIndex < 8; uiSemanticIndex++)
        {
            if (pkGeometry->ContainsData(NiCommonSemantics::TANGENT(), 
                uiSemanticIndex))
            {
                bTangentsAvailable = true;
                break;
            }
        }
    }

    return true;
}
//---------------------------------------------------------------------------
bool NiTerrainMaterial::DetectExtraData(const NiRenderObject* pkGeometry, NiUInt32& uiMorphMode)
{
    // Default values for these settings
    uiMorphMode = 0;

    // Detect the morph mode selected for this object (2D or 3D)
    NiIntegerExtraData* pkMorphMode = NiDynamicCast(NiIntegerExtraData,
        pkGeometry->GetExtraData(NiTerrainCellShaderData::MORPHMODE_SHADER_CONSTANT));
    if (pkMorphMode)
    {
        NiUInt32 uiMode = pkMorphMode->GetValue();
        if (uiMode & NiTerrainSectorData::LOD_MORPH_ENABLE)
        {
            uiMorphMode = 
                (uiMode & (~NiTerrainSectorData::LOD_MORPH_ENABLE)) + 1;
        }
    }
    else
    {
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------
bool NiTerrainMaterial::DetectSurfaceLayerProperties(NiTerrainMaterialDescriptor* pkDesc,
    const NiTexturingProperty* pkTexProp)
{
    if (pkTexProp && pkTexProp->GetShaderMapCount())
    {  
        NiIntegersExtraData* pkLayerInfo = NiDynamicCast(NiIntegersExtraData, 
            pkTexProp->GetExtraDataAt(0));
        EE_ASSERT(pkLayerInfo);

        NiUInt32 uiNumSurfaces = (NiUInt32)pkLayerInfo->GetValue(0);
        pkDesc->SetNUM_LAYERS(uiNumSurfaces);
        for (NiUInt32 ui = 0; ui < uiNumSurfaces; ui++)
        {
            NiInt32 iLayerCaps = pkLayerInfo->GetValue(ui + 1);
            pkDesc->SetLayerCaps(ui, 
                (iLayerCaps & NiSurface::SURFACE_CAPS_DIFFUSE) != 0,
                (iLayerCaps & NiSurface::SURFACE_CAPS_NORMAL) != 0, 
                (iLayerCaps & NiSurface::SURFACE_CAPS_PARALLAX) != 0, 
                (iLayerCaps & NiSurface::SURFACE_CAPS_DETAIL) != 0,
                (iLayerCaps & NiSurface::SURFACE_CAPS_DISTRIBUTION) != 0,
                (iLayerCaps & NiSurface::SURFACE_CAPS_SPECULAR) != 0);
        }

#ifdef EE_ASSERTS_ARE_ENABLED
        // Run through the available textures and verify that they are in their correct positions
        for (efd::UInt32 uiIndex = 0; uiIndex < pkTexProp->GetShaderMapCount(); ++uiIndex)
        {
            const NiTexturingProperty::ShaderMap* pkMap = pkTexProp->GetShaderMap(uiIndex);
            efd::UInt32 uiMapID = pkMap->GetID();

            if (uiIndex == 0)
            {
                EE_ASSERT(uiMapID == NiTerrainMaterial::BLEND_MAP);
            }
            else
            {
                efd::UInt32 uiExpectedMapClass = (uiIndex - 1) % 3;
                switch(uiExpectedMapClass)
                {
                    case 0:
                        EE_ASSERT(uiMapID == NiTerrainMaterial::BASE_MAP);
                        break;
                    case 1:
                        EE_ASSERT(uiMapID == NiTerrainMaterial::NORMAL_MAP);
                        break;
                    case 2:
                        EE_ASSERT(uiMapID == NiTerrainMaterial::SPEC_MAP);
                        break;
                    default:
                        EE_FAIL("TerrainMaterial - Unexpected map ID");
                }
            }
        }
#endif
    }

    return true;
}

//---------------------------------------------------------------------------
bool NiTerrainMaterial::DetectPlatformRestrictions(NiTerrainMaterialDescriptor* pkDesc,
    const NiRenderObject* pkGeometry)
{
    EE_UNUSED_ARG(pkGeometry);
    if (!m_bEnableNPOnSM2)
    { // Shader model 2 does not support terrain with normal/parallax mapping
        
        // Therefore we don't need any tangents input
        pkDesc->SetINPUT_TANGENTS(0);

        // Disable the parallax mapping routines
        pkDesc->SetLAYER0_PARALLAXMAP_ENABLED(false);
        pkDesc->SetLAYER1_PARALLAXMAP_ENABLED(false);
        pkDesc->SetLAYER2_PARALLAXMAP_ENABLED(false);
        pkDesc->SetLAYER3_PARALLAXMAP_ENABLED(false);

        // Disable the normal mapping routines
        pkDesc->SetLAYER0_NORMALMAP_ENABLED(false);
        pkDesc->SetLAYER1_NORMALMAP_ENABLED(false);
        pkDesc->SetLAYER2_NORMALMAP_ENABLED(false);
        pkDesc->SetLAYER3_NORMALMAP_ENABLED(false);
    }   

    return true;
}

//---------------------------------------------------------------------------
bool NiTerrainMaterial::DetectDebugConfiguration(
    NiTerrainMaterialDescriptor* pkDesc,
    const NiRenderObject* pkGeometry)
{
    // Default debug values
    NiUInt32 uiDebugMode = NiTerrainCellShaderData::DEBUG_OFF;

    // Detect the debug mode selected for this object
    NiIntegerExtraData* pkDebugMode = NiDynamicCast(NiIntegerExtraData,
        pkGeometry->GetExtraData(NiTerrainCellShaderData::DEBUGMODE_SHADER_CONSTANT));
    if (pkDebugMode)
    {
        uiDebugMode = pkDebugMode->GetValue();
    }

    // Select forced settings for particular debug modes
    // Disable lighting for all modes (they look better)
    efd::UInt32 uiShowDebugMode = (uiDebugMode & ~NiTerrainCellShaderData::DEBUG_FLAG_MASK);
    if (uiShowDebugMode != NiTerrainCellShaderData::DEBUG_OFF && 
        uiShowDebugMode != NiTerrainCellShaderData::DEBUG_SHOW_NORMALS && 
        uiShowDebugMode != NiTerrainCellShaderData::DEBUG_SHOW_GLOSSINESS)
    {
        uiDebugMode |= NiTerrainCellShaderData::DEBUG_DISABLE_LIGHTING;
    }
    
    // Apply these debug values
    pkDesc->SetDEBUG_MODE(uiShowDebugMode);
    if (uiDebugMode & NiTerrainCellShaderData::DEBUG_DISABLE_LIGHTING)
    {
        pkDesc->SetAPPLYMODE(NiStandardMaterial::APPLY_REPLACE);
        pkDesc->SetPERVERTEXFORLIGHTS(1);
        pkDesc->SetDIRLIGHTCOUNT(0);
        pkDesc->SetSPOTLIGHTCOUNT(0);
        pkDesc->SetPOINTLIGHTCOUNT(0);
    }

    if (uiDebugMode & NiTerrainCellShaderData::DEBUG_DISABLE_NORMAL_MAPS)
    {
        pkDesc->SetLAYER0_NORMALMAP_ENABLED(false);
        pkDesc->SetLAYER1_NORMALMAP_ENABLED(false);
        pkDesc->SetLAYER2_NORMALMAP_ENABLED(false);
        pkDesc->SetLAYER3_NORMALMAP_ENABLED(false);
        pkDesc->SetUSE_LOWDETAILNORMAL(false);
    }

    if (uiDebugMode & NiTerrainCellShaderData::DEBUG_DISABLE_PARALLAX_MAPS)
    {
        pkDesc->SetLAYER0_PARALLAXMAP_ENABLED(false);
        pkDesc->SetLAYER1_PARALLAXMAP_ENABLED(false);
        pkDesc->SetLAYER2_PARALLAXMAP_ENABLED(false);
        pkDesc->SetLAYER3_PARALLAXMAP_ENABLED(false);
    }

    if (uiDebugMode & NiTerrainCellShaderData::DEBUG_DISABLE_HIGH_DETAIL)
    {
        if (pkDesc->GetRENDER_MODE() != NiTerrainCellShaderData::BAKE_DIFFUSE)
        {
            pkDesc->SetNUM_LAYERS(0);
            pkDesc->SetBLENDMAP_COUNT(0);
        }
    }

    if (uiDebugMode & NiTerrainCellShaderData::DEBUG_DISABLE_BASE_NORMAL_MAP)
    {
        pkDesc->SetUSE_LOWDETAILNORMAL(0);
    }

    if (uiDebugMode & NiTerrainCellShaderData::DEBUG_DISABLE_SPECULAR_MAPS)
    {
        pkDesc->SetLAYER0_SPECULARMAP_ENABLED(false);
        pkDesc->SetLAYER1_SPECULARMAP_ENABLED(false);
        pkDesc->SetLAYER2_SPECULARMAP_ENABLED(false);
        pkDesc->SetLAYER3_SPECULARMAP_ENABLED(false);
        pkDesc->SetSPECULAR(0);
    }

    if (uiDebugMode & NiTerrainCellShaderData::DEBUG_DISABLE_DETAIL_MAPS)
    {
        pkDesc->SetLAYER0_DETAILMAP_ENABLED(false);
        pkDesc->SetLAYER1_DETAILMAP_ENABLED(false);
        pkDesc->SetLAYER2_DETAILMAP_ENABLED(false);
        pkDesc->SetLAYER3_DETAILMAP_ENABLED(false);
    }

    if (uiDebugMode & NiTerrainCellShaderData::DEBUG_DISABLE_DISTRIBUTION_MASKS)
    {
        pkDesc->SetLAYER0_DISTMAP_ENABLED(false);
        pkDesc->SetLAYER1_DISTMAP_ENABLED(false);
        pkDesc->SetLAYER2_DISTMAP_ENABLED(false);
        pkDesc->SetLAYER3_DISTMAP_ENABLED(false);
    }

    return true;
}
//---------------------------------------------------------------------------
bool NiTerrainMaterial::DetectRenderMode(NiTerrainMaterialDescriptor* pkDesc,
    const NiRenderObject* pkGeometry)
{
    // Default render mode
    NiUInt32 uiRenderMode = NiTerrainCellShaderData::RENDER_DISPLAY;

    // Detect the render mode selected for this object
    NiIntegerExtraData* pkRenderMode = NiDynamicCast(NiIntegerExtraData,
        pkGeometry->GetExtraData(NiTerrainCellShaderData::RENDERMODE_SHADER_CONSTANT));
    if (pkRenderMode)
    {
        uiRenderMode = pkRenderMode->GetValue();
    }

    // Make any required changes to the descriptors
    pkDesc->SetRENDER_MODE(uiRenderMode);
    if (uiRenderMode == NiTerrainCellShaderData::BAKE_DIFFUSE)
    {
        // We still need lighting routines for highlights
        pkDesc->SetAPPLYMODE(NiStandardMaterial::APPLY_MODULATE);
        pkDesc->SetPERVERTEXFORLIGHTS(1);

        // Disable the parallax mapping routines
        pkDesc->SetLAYER0_PARALLAXMAP_ENABLED(false);
        pkDesc->SetLAYER1_PARALLAXMAP_ENABLED(false);
        pkDesc->SetLAYER2_PARALLAXMAP_ENABLED(false);
        pkDesc->SetLAYER3_PARALLAXMAP_ENABLED(false);

        // Disable the normal mapping routines
        pkDesc->SetLAYER0_NORMALMAP_ENABLED(false);
        pkDesc->SetLAYER1_NORMALMAP_ENABLED(false);
        pkDesc->SetLAYER2_NORMALMAP_ENABLED(false);
        pkDesc->SetLAYER3_NORMALMAP_ENABLED(false);
    }
    else if (uiRenderMode == NiTerrainCellShaderData::PULSE_UNPAINTED)
    {
        NiFloatExtraData* pkTime = NiDynamicCast(NiFloatExtraData,
            pkGeometry->GetExtraData(NiShaderConstantMap::GetTimeExtraDataName()));
        if (!pkTime)
            pkDesc->SetRENDER_MODE(NiTerrainCellShaderData::RENDER_DISPLAY);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::SetupPackingRequirements(NiShader* pkShader,
     NiMaterialDescriptor* pkMaterialDescriptor,
     NiFragmentMaterial::RenderPassDescriptor*, 
     NiUInt32)
{
    NiTerrainMaterialDescriptor* pkMaterialDesc = 
        (NiTerrainMaterialDescriptor*)pkMaterialDescriptor;

    NiUInt32 uiStreamCount = 1;
    NiShaderDeclarationPtr spShaderDecl = 
        NiShaderDeclaration::Create(15, uiStreamCount);

    if (!spShaderDecl)
    {
        EE_ASSERT(!"Invalid shader declaration.");
        return false;
    }

    NiUInt32 uiEntryCount = 0;

    // Select the appropriate stream size depending on morphing
    bool bMorphStreams = pkMaterialDesc->GetINPUT_MORPHING() != 0;
    bool bCompressedStreams = pkMaterialDesc->GetINPUT_LIGHTING_COMPRESSED() != 0;

    NiShaderDeclaration::ShaderParameterType kPositionElementType = 
        NiShaderDeclaration::SPTYPE_FLOAT3;
    if (bMorphStreams)
    {
        kPositionElementType = NiShaderDeclaration::SPTYPE_FLOAT4;
    }

    // Handle position stream.
    spShaderDecl->SetEntry(uiEntryCount++, 
        NiShaderDeclaration::SHADERPARAM_NI_POSITION0,
        kPositionElementType);

    NiShaderDeclaration::ShaderParameterType kNTElementType = 
        NiShaderDeclaration::SPTYPE_FLOAT3;
    if (bCompressedStreams)
    {
        kNTElementType = NiShaderDeclaration::SPTYPE_FLOAT2;
    }

    // Handle normal and tangent stream.
    if (pkMaterialDesc->GetINPUT_NORMALS())
    {
        spShaderDecl->SetEntry(uiEntryCount++,
            NiShaderDeclaration::SHADERPARAM_NI_NORMAL,
            kNTElementType);

        // Check for per-vertex tangents and add a stream if they are present.
        if (pkMaterialDesc->GetINPUT_TANGENTS())
        {
            spShaderDecl->SetEntry(uiEntryCount++, 
                NiShaderDeclaration::SHADERPARAM_NI_TANGENT,
                kNTElementType);
        }
    }

    // Setup any lighting streams
    NiFragmentLighting::Descriptor kLightingDesc;
    m_pkLighting->GetDescriptor(pkMaterialDesc, kLightingDesc);
    m_pkLighting->SetupPackingRequirements(kLightingDesc, 
        spShaderDecl, uiEntryCount);

    // Add streams for each texture coordinate set that the material 
    // requires.
    NiUInt32 uiTexCoordSetCount = 1;
    for (NiUInt32 ui = 0; ui < uiTexCoordSetCount; ui++)
    {
        spShaderDecl->SetEntry(uiEntryCount++, 
            (NiShaderDeclaration::ShaderParameter)
            (NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0 + ui),
            NiShaderDeclaration::SPTYPE_FLOAT2);
    }

    pkShader->SetSemanticAdapterTableFromShaderDeclaration(spShaderDecl);
    return true;
}
//---------------------------------------------------------------------------
NiShader* NiTerrainMaterial::CreateShader(NiMaterialDescriptor* pkDesc)
{
    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    if (pkRenderer == NULL)
        return nullptr;

    return pkRenderer->GetFragmentShader(pkDesc);
}
//---------------------------------------------------------------------------
NiFragmentMaterial::ReturnCode NiTerrainMaterial::GenerateShaderDescArray(
    NiMaterialDescriptor* pkMaterialDescriptor,
    RenderPassDescriptor* pkRenderPasses,
    NiUInt32 uiMaxCount,
    NiUInt32& uiCountAdded)
{
    EE_UNUSED_ARG(uiMaxCount);
    EE_ASSERT(uiMaxCount != 0);
    uiCountAdded = 0;

    if (pkMaterialDescriptor->m_kIdentifier != m_kMaterialDescriptorName)
        return RC_INVALID_MATERIAL;

    // Setup the first pass.
    pkRenderPasses[0].m_bUsesNiRenderState = true;
    pkRenderPasses[0].m_bResetObjectOffsets = true;

    NiTerrainMaterialDescriptor* pkMaterialDesc =
        (NiTerrainMaterialDescriptor*)pkMaterialDescriptor;

    // Initialize the descriptors
    NiTerrainMaterialVertexDescriptor* pkVertexDesc =
        (NiTerrainMaterialVertexDescriptor*)pkRenderPasses[0].m_pkVertexDesc;
    pkVertexDesc->m_kIdentifier = m_kVertexShaderDescriptorName;

    NiTerrainMaterialPixelDescriptor* pkPixelDesc =
        (NiTerrainMaterialPixelDescriptor*)pkRenderPasses[0].m_pkPixelDesc;
    pkPixelDesc->m_kIdentifier = m_kPixelShaderDescriptorName;

    // Begin setting the values
    pkVertexDesc->SetINPUT_NORMALS(pkMaterialDesc->GetINPUT_NORMALS());  
    bool bHasTangents = (pkMaterialDesc->GetINPUT_TANGENTS() != 0);    
    pkVertexDesc->SetINPUT_TANGENTS(bHasTangents);
    pkVertexDesc->SetFOGTYPE(pkMaterialDesc->GetFOGTYPE());
    pkPixelDesc->SetFOGTYPE(pkMaterialDesc->GetFOGTYPE());

    bool bOutputWorldView = pkMaterialDesc->RequiresViewVector();
    pkVertexDesc->SetOUTPUT_WORLDVIEW(bOutputWorldView);
    pkPixelDesc->SetINPUT_WORLDVIEW(bOutputWorldView);

    // Copy the Stream format descriptors:
    pkVertexDesc->SetINPUT_LIGHTING_COMPRESSED(pkMaterialDesc->GetINPUT_LIGHTING_COMPRESSED());
    pkVertexDesc->SetINPUT_MORPHING(pkMaterialDesc->GetINPUT_MORPHING());
    pkVertexDesc->SetMORPH_MODE(pkMaterialDesc->GetMORPH_MODE());

    // If there are per-vertex tangents, then we want to output a tangent frame
    // per vertex with the bi-normal computed in the vertex shader. If there
    // are no tangents, we send across a per-vertex normal and compute the
    // tangent frame in the pixel shader.

    bool bOutputWorldNBT = 
        pkMaterialDesc->HasNormalMapping() || pkMaterialDesc->HasParallaxMapping();
    pkVertexDesc->SetOUTPUT_WORLDNBT(bOutputWorldNBT);
    pkPixelDesc->SetINPUT_WORLDNBT(bOutputWorldNBT);

    // Now inspect the material descriptor for information regarding the layers
    // to render for the block.
    NiUInt32 uiNumLayers = pkMaterialDesc->GetNUM_LAYERS();
    pkPixelDesc->SetNUM_LAYERS(uiNumLayers);
    pkVertexDesc->SetNUM_LAYERS(uiNumLayers);

    NiUInt32 uiNumBlendMaps = pkMaterialDesc->GetBLENDMAP_COUNT();
    pkPixelDesc->SetBLENDMAP_COUNT(uiNumBlendMaps);

    for (NiUInt32 ui = 0; ui < uiNumLayers; ui++)
        pkPixelDesc->SetLayerCapabilitiesFromMaterialDescriptor(pkMaterialDesc, ui);
   
    // Split the lighting descriptor into vertex and pixel halves.
    NiFragmentLighting::Descriptor kLightingDesc;
    m_pkLighting->GetDescriptor(pkMaterialDesc, kLightingDesc);

    NiFragmentLighting::VertexDescriptor kLightingVertDesc;
    NiFragmentLighting::PixelDescriptor kLightingPixelDesc;

    NiUInt32 uiRequiredLightingPasses = 0;
    m_pkLighting->GenerateShaderDescArray(kLightingDesc, kLightingPixelDesc, 
        kLightingVertDesc, 0, uiRequiredLightingPasses);
    EE_ASSERT(uiRequiredLightingPasses == 1);

    m_pkLighting->SetDescriptor(pkPixelDesc, kLightingPixelDesc);
    m_pkLighting->SetDescriptor(pkVertexDesc, kLightingVertDesc);

    // Make adjustments to the descriptors to include the
    // required parameters for lighting in the pixel shader

    // Pixel Descriptor lighting adjustments
    if (kLightingPixelDesc.bInputWorldView)
        pkPixelDesc->SetINPUT_WORLDVIEW(1);
    if (kLightingPixelDesc.bInputWorldPos)
        pkPixelDesc->SetINPUT_WORLDPOSITION(1);
    if (kLightingPixelDesc.bInputWorldNormal)
        pkPixelDesc->SetINPUT_WORLDNORMAL(1);

    // Vertex Descriptor lighting adjustments
    if (kLightingVertDesc.bOutputWorldView)
        pkVertexDesc->SetOUTPUT_WORLDVIEW(1);
    if (kLightingVertDesc.bOutputWorldPos)
        pkVertexDesc->SetOUTPUT_WORLDPOSITION(1);
    if (kLightingVertDesc.bOutputWorldNormal)
        pkVertexDesc->SetOUTPUT_WORLDNORMAL(1);

    // set the appropriate byte to show we have base maps
    pkPixelDesc->SetUSE_LOWDETAILDIFFUSE(pkMaterialDesc->GetUSE_LOWDETAILDIFFUSE());
    pkPixelDesc->SetUSE_LOWDETAILNORMAL(pkMaterialDesc->GetUSE_LOWDETAILNORMAL());

    // Copy the debug mode information
    pkPixelDesc->SetDEBUG_MODE(pkMaterialDesc->GetDEBUG_MODE());

    // copy the render information
    pkPixelDesc->SetRENDER_MODE(pkMaterialDesc->GetRENDER_MODE());
    pkVertexDesc->SetRENDER_MODE(pkMaterialDesc->GetRENDER_MODE());

    // Sanity check (otherwise encoding/decoding interpolators will be a mess)
    EE_ASSERT(pkPixelDesc->GetINPUT_WORLDPOSITION() == pkVertexDesc->GetOUTPUT_WORLDPOSITION());
    EE_ASSERT(pkPixelDesc->GetINPUT_WORLDVIEW() == pkVertexDesc->GetOUTPUT_WORLDVIEW());
    EE_ASSERT(pkPixelDesc->GetINPUT_WORLDNBT() == pkVertexDesc->GetOUTPUT_WORLDNBT());
    EE_ASSERT(pkPixelDesc->GetINPUT_WORLDNORMAL() == pkVertexDesc->GetOUTPUT_WORLDNORMAL());

    uiCountAdded++;
    return NiFragmentMaterial::RC_SUCCESS;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::GenerateVertexShadeTree(Context& kContext,
    NiGPUProgramDescriptor* pkDesc)
{
    EE_ASSERT(pkDesc->m_kIdentifier == "NiTerrainMaterialVertexDescriptor");
    NiTerrainMaterialVertexDescriptor* pkVertexDesc =
        (NiTerrainMaterialVertexDescriptor*)pkDesc;

    // Set the description of this shader
    kContext.m_spConfigurator->SetDescription(pkVertexDesc->ToString());

    // Add vertex in, vertex out, constants, and uniforms 
    // (default configuration nodes for the shader)
    if (!AddDefaultMaterialNodes(kContext, pkDesc,
        NiGPUProgram::PROGRAM_VERTEX))
    {
        return false;
    }

    // Fetch the vertex inputs from the vertex streams
    NiMaterialResource* pkRawPosition = NULL;
    NiMaterialResource* pkRawNormal = NULL;
    NiMaterialResource* pkRawTangent = NULL;
    if (!HandleVertexInputs(kContext, pkVertexDesc,
        pkRawPosition, pkRawNormal, pkRawTangent))
    {
        return false;
    }

    // First split the position, normal, and possibly tangent into a source and
    // a destination morph.
    NiMaterialResource* pkPosHigh = NULL;
    NiMaterialResource* pkPosLow = NULL;
    NiMaterialResource* pkNormal = NULL;
    NiMaterialResource* pkTangent = NULL;
    HandleInputDataSplits(kContext, pkVertexDesc, 
        pkRawPosition, pkRawNormal, pkRawTangent,
        pkPosHigh, pkPosLow, pkNormal, pkTangent);

    // Calculate the morph value for the vertex attributes.
    NiMaterialResource* pkMorphValue = NULL;
    HandleCalculateMorphValue(kContext, pkVertexDesc, pkMorphValue);

    // Morph the position according to the morph value
    NiMaterialResource* pkMorphPos = NULL;
    HandleMorphStreams(kContext, pkMorphValue, pkPosHigh, pkPosLow,  
        pkMorphPos);

    // Lerp the high and low positions and send them through the view
    // projection matrix. Also, output the interpolated world position of the
    // vertex as well.
    NiMaterialResource* pkWorldPos = NULL;
    NiMaterialResource* pkViewPos = NULL;
    NiMaterialResource* pkProjPos = NULL;
    NiMaterialResource* pkWorldNormal = NULL;
    NiMaterialResource* pkWorldBinormal = NULL;
    NiMaterialResource* pkWorldTangent = NULL;
    SetupTransformPipeline(kContext, pkVertexDesc, pkMorphPos, pkNormal, 
        pkTangent, pkWorldPos, pkProjPos, pkViewPos, pkWorldNormal, 
        pkWorldBinormal, pkWorldTangent);

    // Handle any fog calculations
    NiMaterialResource* pkFogValue = NULL;
    Fog eFogType = (Fog)pkVertexDesc->GetFOGTYPE();
    HandleCalculateFog(kContext, eFogType, pkViewPos, pkFogValue);

    // Handle the view vector fragment and connect it to the vertex out if 
    // there are any specular maps available.
    NiMaterialResource* pkWorldView = NULL;
    HandleCalculateWorldView(kContext, pkVertexDesc, pkWorldPos, pkWorldView);

    // Output the required values out of those available.
    if (!HandleVertexOutputs(kContext, pkVertexDesc, pkProjPos, pkWorldPos, 
        pkWorldNormal, pkWorldBinormal, pkWorldTangent, pkWorldView, 
        pkMorphValue, pkFogValue))
    {
        return false;
    }

    // Handle the standard lighting pipeline
    if (!HandleVertexLighting(kContext, pkVertexDesc, pkWorldPos, 
        pkWorldNormal, pkWorldView))
    {
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------
bool NiTerrainMaterial::HandleVertexInputs(Context& kContext,
    NiTerrainMaterialVertexDescriptor* pkVertexDesc,
    NiMaterialResource*& pkInputPosition, 
    NiMaterialResource*& pkInputNormal, 
    NiMaterialResource*& pkInputTangent)
{
    EE_ASSERT(pkVertexDesc);

    // Configuration values defining what is stored in the streams
    bool bMorphingDataPresent = pkVertexDesc->GetINPUT_MORPHING() != 0;
    bool bNormalsPresent = pkVertexDesc->GetINPUT_NORMALS() != 0;
    bool bTangentsPresent = pkVertexDesc->GetINPUT_TANGENTS() != 0;
    bool bCompressedData = pkVertexDesc->GetINPUT_LIGHTING_COMPRESSED() != 0;
    
    // Select the appropriate type of value to fetch from the position stream
    NiFixedString kPositionStreamType = "float3";
    if (bMorphingDataPresent)
        kPositionStreamType = "float4";
    pkInputPosition = kContext.m_spInputs->AddOutputResource(
        kPositionStreamType, "Position", "Local", "vPosition");

    if (bNormalsPresent)
    {
        // Select the appropriate type of value to fetch from the normal stream
        NiFixedString kNormalStreamType = "float3";
        if (bCompressedData)
            kNormalStreamType = "float2";

        pkInputNormal = kContext.m_spInputs->AddOutputResource(
            kNormalStreamType, "Normal", "Local", "vNormal");

        // Fetch the tangents if they are available/required
        if (bTangentsPresent)
        {
            pkInputTangent = kContext.m_spInputs->AddOutputResource(
                kNormalStreamType, "Tangent", "Local", "vTangent");    
        }
    }
    else
    {
        // Fetch the normal and tangent of the terrain itself. 
        pkInputNormal = m_pkOperations->GenerateShaderConstant(kContext, NiPoint3::UNIT_Z);

        if (bTangentsPresent)
        {
            pkInputTangent = m_pkOperations->GenerateShaderConstant(kContext, NiPoint3::UNIT_X);
        }
    }

    EE_ASSERT(pkInputPosition && pkInputNormal);
    return true;
}
//---------------------------------------------------------------------------
void NiTerrainMaterial::HandleInputDataSplits(
    NiFragmentMaterial::Context& kContext, 
    NiTerrainMaterialVertexDescriptor* pkVertexDesc,
    NiMaterialResource* pkRawPos, NiMaterialResource* pkRawNormal,
    NiMaterialResource* pkRawTangent,
    NiMaterialResource*& pkPosHigh, NiMaterialResource*& pkPosLow,
    NiMaterialResource*& pkNormal, NiMaterialResource*& pkTangent)
{
    EE_ASSERT(pkVertexDesc);

    // Configuration values defining what is stored in the streams
    bool bMorphingDataPresent = pkVertexDesc->GetINPUT_MORPHING() != 0;
    bool bTangentsPresent = pkVertexDesc->GetINPUT_TANGENTS() != 0;
    bool bCompressedData = pkVertexDesc->GetINPUT_LIGHTING_COMPRESSED() != 0;
    
    if (bMorphingDataPresent)
    {
        // Handle the split positions fragment.
        NiMaterialNode* pkSplitPositionFrag = GetAttachableNodeFromLibrary(
            "SplitPosition");
        EE_ASSERT(pkSplitPositionFrag);
        kContext.m_spConfigurator->AddNode(pkSplitPositionFrag);        

        // Bind Inputs
        kContext.m_spConfigurator->AddBinding(pkRawPos,
            "CombinedPosition", pkSplitPositionFrag);

        // Bind outputs
        pkPosHigh = pkSplitPositionFrag->GetOutputResourceByVariableName(
            "PositionHigh");
        pkPosLow = pkSplitPositionFrag->GetOutputResourceByVariableName(
            "PositionLow");
        EE_ASSERT(pkPosHigh && pkPosLow);
    }
    else
    {
        // If there is no morphing data present then we need to copy the data 
        // to use in following calculations
        pkPosHigh = pkRawPos;
        pkPosLow = pkRawPos;
    }

    if (bCompressedData)
    {
        HandleDecompressNormal(kContext, pkRawNormal, pkRawNormal);
        if (bTangentsPresent)
        {
            HandleDecompressTangent(kContext, pkRawTangent, pkRawTangent);
        }
    }
    
    pkNormal = pkRawNormal;
    pkTangent = pkRawTangent;
}
//---------------------------------------------------------------------------
void NiTerrainMaterial::HandleSplitFloat4ToFloat2(Context& kContext,
        NiMaterialResource* pkFloat4, NiMaterialResource*& pkFirstHalf,
        NiMaterialResource*& pkSecondHalf)
{
    EE_ASSERT(pkFloat4);

    // Create the splitting fragment
    NiMaterialNode* pkSplitFrag = GetAttachableNodeFromLibrary(
        "SplitFloat4ToFloat2");       
    EE_ASSERT(pkSplitFrag);
    kContext.m_spConfigurator->AddNode(pkSplitFrag);

    // Bind inputs
    kContext.m_spConfigurator->AddBinding(pkFloat4, 
        "Combined", pkSplitFrag);

    // Bind outputs
    pkFirstHalf = pkSplitFrag->GetOutputResourceByVariableName("FirstHalf");
    pkSecondHalf = pkSplitFrag->GetOutputResourceByVariableName("SecondHalf");

    EE_ASSERT(pkFirstHalf && pkSecondHalf);
}
//---------------------------------------------------------------------------
void NiTerrainMaterial::HandleDecompressNormal(Context& kContext,
        NiMaterialResource* pkCompressed, NiMaterialResource*& pkNormal)
{
    EE_ASSERT(pkCompressed);

    // Create the splitting fragment
    NiMaterialNode* pkDecomFrag = GetAttachableNodeFromLibrary(
        "DecompressNormal");       
    EE_ASSERT(pkDecomFrag);
    kContext.m_spConfigurator->AddNode(pkDecomFrag);

    // Bind inputs
    kContext.m_spConfigurator->AddBinding(pkCompressed, 
        "Compressed", pkDecomFrag);

    // Bind outputs
    pkNormal = pkDecomFrag->GetOutputResourceByVariableName("Normal");

    EE_ASSERT(pkNormal);
}
//---------------------------------------------------------------------------
void NiTerrainMaterial::HandleDecompressTangent(Context& kContext,
    NiMaterialResource* pkCompressed, NiMaterialResource*& pkTangent)
{
    EE_ASSERT(pkCompressed);

    // Create the splitting fragment
    NiMaterialNode* pkDecomFrag = GetAttachableNodeFromLibrary(
        "DecompressTangent");       
    EE_ASSERT(pkDecomFrag);
    kContext.m_spConfigurator->AddNode(pkDecomFrag);

    // Bind inputs
    kContext.m_spConfigurator->AddBinding(pkCompressed, 
        "Compressed", pkDecomFrag);

    // Bind outputs
    pkTangent = pkDecomFrag->GetOutputResourceByVariableName("Tangent");

    EE_ASSERT(pkTangent);
}
//---------------------------------------------------------------------------
void NiTerrainMaterial::HandleMorphStreams(Context& kContext,
    NiMaterialResource* pkMorphValue,
    NiMaterialResource* pkPosHigh, NiMaterialResource* pkPosLow, 
    NiMaterialResource*& pkFinalPos)
{
    EE_ASSERT (pkMorphValue);
    EE_ASSERT (pkPosHigh && pkPosLow);
    {
        m_pkOperations->LerpVector(kContext, pkPosHigh, pkPosLow, 
            pkMorphValue, pkFinalPos);
        EE_ASSERT(pkFinalPos);
    }
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterial::HandleCalculateMorphValue(
    NiFragmentMaterial::Context& kContext,
    NiTerrainMaterialVertexDescriptor* pkDesc, 
    NiMaterialResource*& pkMorphValueOutput)
{
    NiMaterialNode* pkMorphFrag = NULL;
    NiMaterialResource* pkStitchingRes = NULL;
    switch (pkDesc->GetMORPH_MODE())
    {
        case 3: // 3D Morphing
        {
            pkMorphFrag = GetAttachableNodeFromLibrary(
                "Calculate3DVertexMorph");
        }
        break;
        
        case 2: // 2.5D Morphing
        {
            pkMorphFrag = GetAttachableNodeFromLibrary(
                "Calculate25DVertexMorph");
        }
        break;
        
        case 1: // 2D Morphing
        {
            pkMorphFrag = GetAttachableNodeFromLibrary(
                "Calculate2DVertexMorph");
        }
        break;

        case 0: // No Morphing
        default:
        {
            pkMorphFrag = GetAttachableNodeFromLibrary(
                "CalculateNoVertexMorph");
            pkStitchingRes = AddOutputAttribute(
                kContext.m_spUniforms, NiTerrainCellShaderData::STITCHINGINFO_SHADER_CONSTANT,
                NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);
            EE_ASSERT(pkStitchingRes);
        }
    }
    EE_ASSERT(pkMorphFrag);
    kContext.m_spConfigurator->AddNode(pkMorphFrag);

    NiMaterialResource* pkLODThresholdDistance = AddOutputAttribute(
        kContext.m_spUniforms, NiTerrainCellShaderData::LODTHRESHOLD_SHADER_CONSTANT,
        NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
    EE_ASSERT(pkLODThresholdDistance);

    NiMaterialResource* pkLODMorphDistance = AddOutputAttribute(
        kContext.m_spUniforms, NiTerrainCellShaderData::LODMORPHDISTANCE_SHADER_CONSTANT,
        NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT);
    EE_ASSERT(pkLODMorphDistance);

    NiMaterialResource* pkCameraPos = AddOutputAttribute(
        kContext.m_spUniforms, NiTerrainCellShaderData::ADJUSTED_EYE_POSITION,
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT3);
    EE_ASSERT(pkCameraPos);

    if (pkStitchingRes)
    {
        kContext.m_spConfigurator->AddBinding(pkStitchingRes,
            pkMorphFrag->GetInputResourceByVariableName("StitchingInfo"));
    }
    kContext.m_spConfigurator->AddBinding(pkLODThresholdDistance,
        pkMorphFrag->GetInputResourceByVariableName("LODThresholdDistance"));
    kContext.m_spConfigurator->AddBinding(pkLODMorphDistance,
        pkMorphFrag->GetInputResourceByVariableName("LODMorphDistance"));
    kContext.m_spConfigurator->AddBinding(pkCameraPos,
        pkMorphFrag->GetInputResourceByVariableName("TerrainCameraPos"));

    kContext.m_spConfigurator->AddBinding(
        kContext.m_spInputs->GetOutputResourceByVariableName("vPosition"),
        pkMorphFrag->GetInputResourceByVariableName("LocalPos"));
    
    pkMorphValueOutput = 
        pkMorphFrag->GetOutputResourceByVariableName("LODMorphValue");  
    EE_ASSERT(pkMorphValueOutput);
}

//---------------------------------------------------------------------------
void NiTerrainMaterial::SetupTransformPipeline(
    Context& kContext, NiTerrainMaterialVertexDescriptor* pkVertexDesc,
    NiMaterialResource* pkModelPosition,
    NiMaterialResource* pkModelNormal,
    NiMaterialResource* pkModelTangent,
    NiMaterialResource*& pkWorldPos,
    NiMaterialResource*& pkProjPos,
    NiMaterialResource*& pkViewPos,
    NiMaterialResource*& pkWorldNormal,
    NiMaterialResource*& pkWorldBinormal,
    NiMaterialResource*& pkWorldTangent)
{   
    EE_ASSERT(pkModelPosition && pkVertexDesc);

    // Fetch the WORLD matrix
    NiMaterialResource* pkWorldMatrix = AddOutputPredefined(
        kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_WORLD, 4);
    EE_ASSERT(pkWorldMatrix);
    
    // Transform the position into world space
    m_pkOperations->TransformPosition(kContext, pkModelPosition, pkWorldMatrix, 
        pkWorldPos);

    // Decide upon whether we need the view position or not.
    // (if we don't then skip straight to projected position)
    bool bCalculateViewPos = pkVertexDesc->GetFOGTYPE() != FOG_NONE;
    if (bCalculateViewPos)
    {
        // Fetch the VIEW matrix
        NiMaterialResource* pkViewMatrix = AddOutputPredefined(
            kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_VIEW, 4);
        EE_ASSERT(pkViewMatrix);

        // Transform to view space
        m_pkOperations->TransformPosition(kContext, 
            pkWorldPos, pkViewMatrix, pkViewPos);

        // Fetch the PROJECTION matrix
        NiMaterialResource* pkProjMatrix = AddOutputPredefined(
            kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_PROJ, 4);
        EE_ASSERT(pkProjMatrix);

        // Transform to clip space
        m_pkOperations->TransformPosition(kContext, 
            pkViewPos, pkProjMatrix, pkProjPos);
    }
    else
    {
        // Fetch the VIEWPROJECTION matrix
        NiMaterialResource* pkViewProjMatrix = AddOutputPredefined(
            kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_VIEWPROJ, 4);
        EE_ASSERT(pkViewProjMatrix);

        // Transform to clip space
        m_pkOperations->TransformPosition(kContext, 
            pkWorldPos, pkViewProjMatrix, pkProjPos);
    }

    // Transform normals. Also generate a bi-normal if tangents are present.
    TransformNBTs(kContext, pkVertexDesc, 
        pkModelNormal, pkModelTangent, pkWorldMatrix, 
        pkWorldNormal, pkWorldBinormal, pkWorldTangent);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterial::TransformNBTs(
    NiFragmentMaterial::Context& kContext,
    NiTerrainMaterialVertexDescriptor* pkDesc,
    NiMaterialResource* pkNormal, NiMaterialResource* pkTangent, 
    NiMaterialResource* pkWorldMatrix, NiMaterialResource*& pkWorldNormal,
    NiMaterialResource*& pkWorldBinormal, NiMaterialResource*& pkWorldTangent)
{
    EE_ASSERT(pkWorldMatrix);

    EE_ASSERT(pkNormal);
    {
        // Transform Normal
        m_pkOperations->TransformDirection(kContext, pkNormal, 
            pkWorldMatrix, pkWorldNormal);
    }
    
    // Handle tangents if present. If we do have tangents generate a 
    // binormal as well for a complete tangent frame.
    if (pkDesc->GetOUTPUT_WORLDNBT())
    {
        if (!pkTangent)
        {
            // Generate the tangent based on the normal
            NiMaterialResource* pkUnitYVector = 
                m_pkOperations->GenerateShaderConstant(kContext, NiPoint3::UNIT_Y);
            
            // Cross the Normal with the Y to get the tangent
            m_pkOperations->CrossVector(kContext, pkUnitYVector, pkNormal, pkTangent);
        }

        // Transform Tangent
        EE_ASSERT(pkTangent);
        m_pkOperations->TransformDirection(kContext, pkTangent, 
            pkWorldMatrix, pkWorldTangent);

        // Construct the bi-normal to complete the tangent frame.
        NiMaterialNode* pkCalcBinormal = GetAttachableNodeFromLibrary(
            "CalculateBinormal");
        kContext.m_spConfigurator->AddNode(pkCalcBinormal);

        // Bind Inputs
        kContext.m_spConfigurator->AddBinding(pkWorldNormal,
            pkCalcBinormal->GetInputResourceByVariableName("WorldNormal"));
        kContext.m_spConfigurator->AddBinding(pkWorldTangent,
            pkCalcBinormal->GetInputResourceByVariableName("WorldTangent"));

        // Bind outputs
        pkWorldBinormal = 
            pkCalcBinormal->GetOutputResourceByVariableName("WorldBinormal");
        
        EE_ASSERT(pkWorldBinormal && pkWorldTangent);
    }
}

//---------------------------------------------------------------------------
void NiTerrainMaterial::HandleCalculateFog(Context& kContext, Fog eFogType,
    NiMaterialResource* pkViewPos, NiMaterialResource*& pkFogValue)
{
    if (eFogType == FOG_NONE)
        return;

    EE_ASSERT(pkViewPos != NULL);

    // Bind fog resources
    NiMaterialNode* pkFogNode = GetAttachableNodeFromLibrary(
        "CalculateFog");
    kContext.m_spConfigurator->AddNode(pkFogNode);

    pkFogValue = pkFogNode->GetOutputResourceByVariableName("FogOut");

    NiUInt32 uiFogType = 0;
    bool bFogRange = false;
    if (eFogType == FOG_LINEAR)
    {
        // NiFogProperty::FOG_Z_LINEAR
        uiFogType = 3; // 3 == linear
        bFogRange = false;
    }
    else if (eFogType == FOG_SQUARED)
    {
        // NiFogProperty::FOG_RANGE_SQ
        uiFogType = 3; // 3 == linear
        bFogRange = true;
    }

    char acValue[32];
    NiSprintf(acValue, 32, "(%d)", uiFogType);
    kContext.m_spConfigurator->AddBinding(
        kContext.m_spStatics->AddOutputConstant("int", acValue),
        pkFogNode->GetInputResourceByVariableName("FogType"));

    kContext.m_spConfigurator->AddBinding(
        kContext.m_spStatics->AddOutputConstant("bool",
        bFogRange ? "(true)" : "(false)"),
        pkFogNode->GetInputResourceByVariableName("FogRange"));

    NiMaterialResource* pkFogDensity = AddOutputPredefined(
        kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_FOG_DENSITY);
    kContext.m_spConfigurator->AddBinding(pkFogDensity,
        pkFogNode->GetInputResourceByVariableName("FogDensity"));

    NiMaterialResource* pkFogNearFar = AddOutputPredefined(
        kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_FOG_NEARFAR);
    kContext.m_spConfigurator->AddBinding(pkFogNearFar,
        pkFogNode->GetInputResourceByVariableName("FogStartEnd"));

    kContext.m_spConfigurator->AddBinding(pkViewPos,
        pkFogNode->GetInputResourceByVariableName("ViewPosition"));
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterial::HandleCalculateWorldView(Context& kContext, 
    NiTerrainMaterialVertexDescriptor* pkDesc, NiMaterialResource* pkWorldPos, 
    NiMaterialResource*& pkViewVector)
{
    EE_UNUSED_ARG(pkDesc);
    EE_ASSERT(pkWorldPos);

    NiMaterialNode* pkCalcViewVectorFrag = GetAttachableNodeFromLibrary(
        "CalculateViewVector");
    kContext.m_spConfigurator->AddNode(pkCalcViewVectorFrag);

    NiMaterialResource* pkCameraPos = AddOutputPredefined(
        kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_EYE_POS);
    EE_ASSERT(pkCameraPos);

    kContext.m_spConfigurator->AddBinding(pkCameraPos,
        pkCalcViewVectorFrag->GetInputResourceByVariableName("CameraPos"));

    kContext.m_spConfigurator->AddBinding(pkWorldPos,
        pkCalcViewVectorFrag->GetInputResourceByVariableName("WorldPos"));

    pkViewVector = pkCalcViewVectorFrag->GetOutputResourceByVariableName(
        "WorldViewVector");
    EE_ASSERT(pkViewVector);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::HandleVertexLighting(Context& kContext, 
    NiTerrainMaterialVertexDescriptor* pkVertexDesc, 
    NiMaterialResource* pkWorldPosition, 
    NiMaterialResource* pkWorldNormal, 
    NiMaterialResource* pkWorldView)
{
    // Extract lighting descriptor from the material descriptor
    NiFragmentLighting::VertexDescriptor kLightingVertDesc;
    m_pkLighting->GetDescriptor(pkVertexDesc, kLightingVertDesc);

    // Generate standard lighting calculations for this vertex
    if (!m_pkLighting->HandleVertexLightingAndMaterials(kContext, 
        kLightingVertDesc, pkWorldPosition, pkWorldNormal, pkWorldView))
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterial::HandleGenerateTextureCoordinates(Context& kContext, 
    NiMaterialResource*& pkMaskUV, NiMaterialResource*& pkLayer0UV,
    NiMaterialResource*& pkLayer1UV, NiMaterialResource*& pkLayer2UV,
    NiMaterialResource*& pkLayer3UV, NiMaterialResource*& pkLowDetailDiffuseUV,
    NiMaterialResource*& pkLowDetailNormalUV)
{
    // Fetch the resources required (constants and inputs)
    NiMaterialResource* pkUVIn = kContext.m_spInputs->AddOutputResource("float2", "TexCoord", 
        "", "UVSet0");
    EE_ASSERT(pkUVIn);

    NiMaterialResource* pkBlendMapScale = AddOutputAttribute(kContext.m_spUniforms,
        NiTerrainCellShaderData::BLENDMAP_SCALE_SHADER_CONSTANT, 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT2);
    EE_ASSERT(pkBlendMapScale);

    NiMaterialResource* pkBlendMapOffset = AddOutputAttribute(kContext.m_spUniforms,
        NiTerrainCellShaderData::BLENDMAP_OFFSET_SHADER_CONSTANT,
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT2);
    EE_ASSERT(pkBlendMapOffset);

    NiMaterialResource* pkLayerScale = AddOutputAttribute(kContext.m_spUniforms,
        NiTerrainCellShaderData::LAYER_SCALE_SHADER_CONSTANT,
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);
    EE_ASSERT(pkLayerScale);

    NiMaterialResource* pkLowResMapScale = AddOutputAttribute(kContext.m_spUniforms,
        NiTerrainCellShaderData::LOWDETAIL_TEXTURE_SCALE_SHADER_CONSTANT,
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT2);
    EE_ASSERT(pkLowResMapScale);

    NiMaterialResource* pkLowResMapOffset = AddOutputAttribute(kContext.m_spUniforms,
        NiTerrainCellShaderData::LOWDETAIL_TEXTURE_OFFSET_SHADER_CONSTANT,
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT2);
    EE_ASSERT(pkLowResMapOffset);

    NiMaterialResource* pkLowResMapSizes = AddOutputAttribute(kContext.m_spUniforms,
        NiTerrainCellShaderData::LOWDETAIL_TEXTURE_SIZES_SHADER_CONSTANT,
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT2);

    // Fetch the material node:
    NiMaterialNode* pkGenTextureCoordinatesFrag = GetAttachableNodeFromLibrary(
        "GenerateTextureCoordinates");
    EE_ASSERT(pkGenTextureCoordinatesFrag);
    kContext.m_spConfigurator->AddNode(pkGenTextureCoordinatesFrag);

    // Bind Inputs:
    kContext.m_spConfigurator->AddBinding(pkUVIn, 
        "uvIn", pkGenTextureCoordinatesFrag);
    kContext.m_spConfigurator->AddBinding(pkLayerScale, 
        "layerScale", pkGenTextureCoordinatesFrag);
    kContext.m_spConfigurator->AddBinding(pkBlendMapScale, 
        "blendMapScale", pkGenTextureCoordinatesFrag);
    kContext.m_spConfigurator->AddBinding(pkBlendMapOffset, 
        "blendMapOffset", pkGenTextureCoordinatesFrag);
    kContext.m_spConfigurator->AddBinding(pkLowResMapScale, 
        "lowResMapScale", pkGenTextureCoordinatesFrag);
    kContext.m_spConfigurator->AddBinding(pkLowResMapOffset, 
        "lowResMapOffset", pkGenTextureCoordinatesFrag);
    kContext.m_spConfigurator->AddBinding(pkLowResMapSizes, 
        "lowResMapSize", pkGenTextureCoordinatesFrag);

    // Bind Outputs:
    pkMaskUV = pkGenTextureCoordinatesFrag->GetOutputResourceByVariableName("maskUV");
    pkLayer0UV = pkGenTextureCoordinatesFrag->GetOutputResourceByVariableName("layer0UV");
    pkLayer1UV = pkGenTextureCoordinatesFrag->GetOutputResourceByVariableName("layer1UV");
    pkLayer2UV = pkGenTextureCoordinatesFrag->GetOutputResourceByVariableName("layer2UV");
    pkLayer3UV = pkGenTextureCoordinatesFrag->GetOutputResourceByVariableName("layer3UV");
    pkLowDetailDiffuseUV = 
        pkGenTextureCoordinatesFrag->GetOutputResourceByVariableName("lowDetailDiffuseUV");
    pkLowDetailNormalUV = 
        pkGenTextureCoordinatesFrag->GetOutputResourceByVariableName("lowDetailNormalUV");

    EE_ASSERT(pkMaskUV);
    EE_ASSERT(pkLayer0UV);
    EE_ASSERT(pkLayer1UV);
    EE_ASSERT(pkLayer2UV);
    EE_ASSERT(pkLayer3UV);
    EE_ASSERT(pkLowDetailDiffuseUV);
    EE_ASSERT(pkLowDetailNormalUV);
}

//--------------------------------------------------------------------------------------------------
efd::UInt32 NiTerrainMaterial::HandleEncodeVertexInterpolators(Context& kContext, 
    NiTerrainMaterialVertexDescriptor* pkDesc, NiMaterialResource* pkMaskUV, 
    NiMaterialResource* pkLayer0UV, NiMaterialResource* pkLayer1UV, NiMaterialResource* pkLayer2UV,
    NiMaterialResource* pkLayer3UV, NiMaterialResource* pkLowDetailDiffuseUV,
    NiMaterialResource* pkLowDetailNormalUV, NiMaterialResource* pkWorldPosition, 
    NiMaterialResource* pkWorldNormal, NiMaterialResource* pkWorldBinormal, 
    NiMaterialResource* pkWorldTangent, NiMaterialResource* pkWorldView, 
    NiMaterialResource* pkMorphValue, NiMaterialResource*& pkTexCoord0, 
    NiMaterialResource*& pkTexCoord1, NiMaterialResource*& pkTexCoord2, 
    NiMaterialResource*& pkTexCoord3, NiMaterialResource*& pkTexCoord4, 
    NiMaterialResource*& pkTexCoord5, NiMaterialResource*& pkTexCoord6, 
    NiMaterialResource*& pkTexCoord7)
{
    // Pack the remaining interpolators and add them to the vertex output. Note, we are always 
    // sending uv's for each layer. The overhead for interpolator usage is minimal. 
    // We could only add interpolators based on how many layers are needed but the complications 
    // aren't worth the savings.

    NiMaterialNode* pkEncodeInterpolatorsFrag = 
        GetAttachableNodeFromLibrary("EncodeInterpolators");
    EE_ASSERT(pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddNode(pkEncodeInterpolatorsFrag);

    // Bind the inputs
    kContext.m_spConfigurator->AddBinding(pkMaskUV, "maskUV", pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(pkLayer0UV, "layer0UV", pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(pkLayer1UV, "layer1UV", pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(pkLayer2UV, "layer2UV", pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(pkLayer3UV, "layer3UV", pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(pkLowDetailDiffuseUV, "lowDetailDiffuseUV", 
        pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(pkLowDetailNormalUV, "lowDetailNormalUV", 
        pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(pkMorphValue, "morphValue", pkEncodeInterpolatorsFrag);

    // Bind the flags to control the encoding
    bool bOutputWorldNormal = pkDesc->GetOUTPUT_WORLDNORMAL() != 0;
    bool bOutputWorldPosition = pkDesc->GetOUTPUT_WORLDPOSITION() != 0;
    bool bOutputWorldView = pkDesc->GetOUTPUT_WORLDVIEW() != 0;
    bool bOutputWorldNBT = pkDesc->GetOUTPUT_WORLDNBT() != 0;
    kContext.m_spConfigurator->AddBinding(
        m_pkOperations->GenerateShaderConstant(kContext, bOutputWorldNormal),
        "bOutputWorldNormal", pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(
        m_pkOperations->GenerateShaderConstant(kContext, bOutputWorldPosition),
        "bOutputWorldPosition", pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(
        m_pkOperations->GenerateShaderConstant(kContext, bOutputWorldView),
        "bOutputWorldView", pkEncodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(
        m_pkOperations->GenerateShaderConstant(kContext, bOutputWorldNBT),
        "bOutputWorldNBT", pkEncodeInterpolatorsFrag);

    // Optional Bindings (the passed in values may be NULL)
    m_pkOperations->OptionalBind(kContext,
        pkWorldView, "worldViewIn", pkEncodeInterpolatorsFrag);
    m_pkOperations->OptionalBind(kContext,
        pkWorldPosition, "worldPosIn", pkEncodeInterpolatorsFrag);
    m_pkOperations->OptionalBind(kContext,
        pkWorldNormal, "worldNormalIn", pkEncodeInterpolatorsFrag);
    m_pkOperations->OptionalBind(kContext,
        pkWorldTangent, "worldTangentIn", pkEncodeInterpolatorsFrag);
    m_pkOperations->OptionalBind(kContext,
        pkWorldBinormal, "worldBinormalIn", pkEncodeInterpolatorsFrag);

    // Fetch the outputs:
    pkTexCoord0 = pkEncodeInterpolatorsFrag->GetOutputResourceByVariableName("texcoord0");
    pkTexCoord1 = pkEncodeInterpolatorsFrag->GetOutputResourceByVariableName("texcoord1");
    pkTexCoord2 = pkEncodeInterpolatorsFrag->GetOutputResourceByVariableName("texcoord2");
    pkTexCoord3 = pkEncodeInterpolatorsFrag->GetOutputResourceByVariableName("texcoord3");
    pkTexCoord4 = pkEncodeInterpolatorsFrag->GetOutputResourceByVariableName("texcoord4");
    pkTexCoord5 = pkEncodeInterpolatorsFrag->GetOutputResourceByVariableName("texcoord5");
    pkTexCoord6 = pkEncodeInterpolatorsFrag->GetOutputResourceByVariableName("texcoord6");
    pkTexCoord7 = pkEncodeInterpolatorsFrag->GetOutputResourceByVariableName("texcoord7");

    return CalculateNumEncodedInterpolators(bOutputWorldNormal, bOutputWorldPosition, 
        bOutputWorldView, bOutputWorldNBT);
}

//--------------------------------------------------------------------------------------------------
efd::UInt32 NiTerrainMaterial::CalculateNumEncodedInterpolators(bool bIncludeNormal, 
    bool bIncludePosition, bool bIncludeView, bool bIncludeNBT)
{
    // Calculate the number of interpolators that will be used:
    efd::UInt32 uiNumInterpolators = 3;
    if (!bIncludeNBT && bIncludeNormal)
    {
        uiNumInterpolators += 2;
        if (bIncludeView || bIncludePosition)
            uiNumInterpolators++;
    }
    else
    {
        if (bIncludeNBT)
        {
            uiNumInterpolators += 3;
        }
        else
        {
            uiNumInterpolators += 1;
        }
        if (bIncludeView)
            uiNumInterpolators++;
        if (bIncludePosition)
            uiNumInterpolators++;
    }
    EE_ASSERT(uiNumInterpolators >= 4 && uiNumInterpolators <= 8);

    return uiNumInterpolators;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::HandleVertexOutputs(Context& kContext,
    NiTerrainMaterialVertexDescriptor* pkVertexDesc, 
    NiMaterialResource* pkProjPosition,
    NiMaterialResource* pkWorldPosition, NiMaterialResource* pkWorldNormal,
    NiMaterialResource* pkWorldBinormal, NiMaterialResource* pkWorldTangent,
    NiMaterialResource* pkWorldView, NiMaterialResource* pkMorphValue,
    NiMaterialResource* pkFogValue)
{
    EE_UNUSED_ARG(pkVertexDesc);

    // Output the position
    NiMaterialResource* pkProjPosOut = kContext.m_spOutputs->AddInputResource(
        "float4", "Position", "World", "PosProjected");
    kContext.m_spConfigurator->AddBinding(pkProjPosition, 
        pkProjPosOut);

    // Generate the texture coordinates:
    NiMaterialResource* pkMaskUV = NULL; 
    NiMaterialResource* pkLayer0UV = NULL; 
    NiMaterialResource* pkLayer1UV = NULL; 
    NiMaterialResource* pkLayer2UV = NULL; 
    NiMaterialResource* pkLayer3UV = NULL;  
    NiMaterialResource* pkLowDetailDiffuseUV = NULL; 
    NiMaterialResource* pkLowDetailNormalUV = NULL; 

    HandleGenerateTextureCoordinates(kContext, pkMaskUV, pkLayer0UV, pkLayer1UV, pkLayer2UV, 
        pkLayer3UV, pkLowDetailDiffuseUV, pkLowDetailNormalUV);

    // Generate the interpolator outputs
    NiMaterialResource* pkTexCoords[8] = {NULL};
    efd::UInt32 uiNumInterpolators = 
        HandleEncodeVertexInterpolators(kContext, pkVertexDesc, pkMaskUV, pkLayer0UV, pkLayer1UV, 
        pkLayer2UV, pkLayer3UV, pkLowDetailDiffuseUV, pkLowDetailNormalUV, 
        pkWorldPosition, pkWorldNormal, pkWorldBinormal, pkWorldTangent, pkWorldView, pkMorphValue,
        pkTexCoords[0], pkTexCoords[1], pkTexCoords[2], pkTexCoords[3], pkTexCoords[4], 
        pkTexCoords[5], pkTexCoords[6], pkTexCoords[7]);
    
    // Output all the encoded coordinate interpolators
    for (efd::UInt32 uiIndex = 0; uiIndex < uiNumInterpolators; ++uiIndex)
    {
        NiString kInterpolatorName;
        kInterpolatorName.Format("Encoded_TexCoord%d", uiIndex);
        NiMaterialResource* pkInterpolator = 
            kContext.m_spOutputs->AddInputResource("float4", "TexCoord", "", 
            (const char*)kInterpolatorName);
        kContext.m_spConfigurator->AddBinding(pkTexCoords[uiIndex], pkInterpolator);
    }

    // Output the fog value
    if (pkFogValue)
    {
        NiMaterialResource* pkFogOut = kContext.m_spOutputs->AddInputResource(
            "float", "Fog", "", "FogOut");
        kContext.m_spConfigurator->AddBinding(pkFogValue, pkFogOut);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::GeneratePixelShadeTree(Context& kContext,
    NiGPUProgramDescriptor* pkDesc)
{
    EE_ASSERT(pkDesc->m_kIdentifier == m_kPixelShaderDescriptorName);
    NiTerrainMaterialPixelDescriptor* pkPixelDesc =
        (NiTerrainMaterialPixelDescriptor*)pkDesc;

    kContext.m_spConfigurator->SetDescription(pkPixelDesc->ToString());

    // Add pixel in, pixel out, constants, and uniforms
    if (!AddDefaultMaterialNodes(kContext, pkDesc,
        NiGPUProgram::PROGRAM_PIXEL))
    {
        return false;
    }

    NiMaterialResource* pkMorphValue = NULL;
    NiMaterialResource* pkMaskValues = NULL;
    NiMaterialResource* pkLayerUV[4] = { NULL, NULL, NULL, NULL};
    NiMaterialResource* pkLowResDiffuseUV = NULL;
    NiMaterialResource* pkLowResNormalUV = NULL;
    NiMaterialResource* pkWorldView = NULL;
    NiMaterialResource* pkWorldViewTS = NULL;
    NiMaterialResource* pkWorldNormal = NULL;
    NiMaterialResource* pkWorldPos = NULL;
    NiMaterialResource* pkNBTMatrix = NULL;
    if (!HandlePixelInputs(kContext, pkPixelDesc, pkMorphValue, pkMaskValues,
        pkLayerUV[0], pkLayerUV[1], pkLayerUV[2], pkLayerUV[3], pkLowResDiffuseUV,
        pkLowResNormalUV, pkWorldView, pkWorldViewTS, pkWorldPos, pkWorldNormal, pkNBTMatrix))
    {
        return false;
    }

    NiMaterialResource* pkMatDiffuse = NULL;
    NiMaterialResource* pkMatSpecular = NULL;
    NiMaterialResource* pkSpecularPower = NULL;
    NiMaterialResource* pkGlossiness = NULL;
    NiMaterialResource* pkMatAmbient = NULL;
    NiMaterialResource* pkMatEmissive = NULL;

    NiMaterialResource* pkTexDiffuseAccum = NULL;
    NiMaterialResource* pkTexSpecularAccum = NULL;

    NiMaterialResource* pkDiffuseAccum = NULL;
    NiMaterialResource* pkSpecularAccum = NULL;
    NiMaterialResource* pkOpacityAccum = NULL;

    NiMaterialResource* pkLightDiffuseAccum = NULL;
    NiMaterialResource* pkLightSpecularAccum = NULL;
    NiMaterialResource* pkLightAmbientAccum = NULL;

    // Apply initial values before accumulation of color
    if (!HandlePixelInitialValues(kContext, pkPixelDesc,
        pkMatDiffuse, pkMatSpecular, pkSpecularPower,
        pkGlossiness, pkMatAmbient, pkMatEmissive, pkTexDiffuseAccum,
        pkTexSpecularAccum, pkDiffuseAccum, pkSpecularAccum, pkOpacityAccum,
        pkLightDiffuseAccum, pkLightSpecularAccum, pkLightAmbientAccum))
    {
        return false;
    }

    // Handle all the texture layers and produce a final diffuse color, final normal, and final 
    // specular color.
    NiMaterialResource* pkFinalLayerDiffuseColor = NULL;
    NiMaterialResource* pkFinalLayerNormal = NULL;
    NiMaterialResource* pkFinalLayerParallaxAccum = NULL;
    NiMaterialResource* pkFinalLayerSpecularPower = 
        m_pkOperations->GenerateShaderConstant(kContext, NiPoint4(100,100,100,100));
    NiMaterialResource* pkFinalLayerGlossiness =  
        m_pkOperations->GenerateShaderConstant(kContext, float(0.0f));
    NiMaterialResource* pkFinalLayerSpecular = 
        m_pkOperations->GenerateShaderConstant(kContext, NiPoint3::ZERO);
    NiMaterialResource* pkTotalMask = NULL;
    if (!HandleSurfaceLayers(kContext, pkPixelDesc, pkMaskValues, pkWorldViewTS, 
        pkLayerUV, pkNBTMatrix, pkWorldNormal, pkFinalLayerDiffuseColor, 
        pkFinalLayerNormal, pkFinalLayerSpecular, pkFinalLayerParallaxAccum, pkFinalLayerGlossiness, 
        pkTotalMask, pkFinalLayerSpecularPower))
    {
        return false;
    }

    // Add the low detail texture sampling to the shader
    NiMaterialResource* pkFinalDiffuseColor = NULL;
    NiMaterialResource* pkFinalNormal = NULL;
    NiMaterialResource* pkFinalSpecular = NULL;
    NiMaterialResource* pkFinalGlossiness = NULL;
    NiMaterialResource* pkFinalSpecularPower = NULL;
    {
        // Fetch the low detail map values (default colors otherwise)
        NiMaterialResource* pkLowDetailDiffuseColor = NULL;
        NiMaterialResource* pkLowDetailSpecular = NULL;
        NiMaterialResource* pkLowDetailGlossiness = NULL;
        NiMaterialResource* pkLowDetailSpecularPower = NULL;
        HandleSampleLowDetailDiffuse(kContext, pkPixelDesc, pkLowResDiffuseUV, 
            pkLowDetailDiffuseColor, pkLowDetailSpecular, pkLowDetailGlossiness,
            pkLowDetailSpecularPower);
        NiMaterialResource* pkLowDetailNormal = NULL;
        HandleSampleLowDetailNormal(kContext, pkPixelDesc, pkLowResNormalUV,
            pkLowDetailNormal);

        // We now need to morph from the layered surfaces to the
        // texture according to distance and to the morphing threshold
        HandleTextureMorphing(kContext, pkPixelDesc,
            pkLowDetailDiffuseColor, pkFinalLayerDiffuseColor, 
            pkWorldNormal, pkLowDetailNormal, pkFinalLayerNormal, 
            pkLowDetailGlossiness, pkLowDetailSpecular, pkLowDetailSpecularPower,
            pkFinalLayerGlossiness, pkFinalLayerSpecular, pkFinalLayerSpecularPower,
            pkMorphValue, 
            pkFinalDiffuseColor, pkFinalSpecular, pkFinalNormal, pkFinalGlossiness,
            pkFinalSpecularPower);
    }
    EE_ASSERT(pkFinalDiffuseColor);
    
    // Handle Lighting contribution
    if (!HandlePixelLighting(kContext, pkPixelDesc, pkWorldPos,
        pkWorldView, pkFinalSpecularPower, pkFinalNormal,
        pkMatEmissive, pkMatDiffuse, pkMatAmbient, pkFinalSpecular,
        pkLightSpecularAccum, pkLightDiffuseAccum, pkLightAmbientAccum,
        pkFinalGlossiness, pkFinalDiffuseColor, pkFinalSpecular,
        pkSpecularAccum, pkDiffuseAccum))
    {
        return false;
    }

    if (!HandleFinalPixelOutputs(kContext, pkPixelDesc, pkDiffuseAccum,
        pkSpecularAccum, pkOpacityAccum, pkFinalGlossiness, pkFinalNormal, 
        pkFinalLayerParallaxAccum, pkMorphValue, pkTotalMask, pkWorldPos, 
        pkFinalSpecularPower))
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::HandlePixelInputs(Context& kContext, 
    NiTerrainMaterialPixelDescriptor* pkPixDesc, NiMaterialResource*& pkMorphValue, 
    NiMaterialResource*& pkMaskValues, NiMaterialResource*& pkLayer0UV, 
    NiMaterialResource*& pkLayer1UV, NiMaterialResource*& pkLayer2UV, 
    NiMaterialResource*& pkLayer3UV, NiMaterialResource*& pkLowResDiffuseUV, 
    NiMaterialResource*& pkLowResNormalUV, NiMaterialResource*& pkWorldView, 
    NiMaterialResource*& pkWorldViewTS, NiMaterialResource*& pkWorldPos, 
    NiMaterialResource*& pkWorldNormal, NiMaterialResource*& pkNBTMatrix)
{
    // Setup projected position input
    kContext.m_spInputs->AddOutputResource("float4", "Position", "World",  "PosProjected");

    // Figure out what decoding fragment to use
    NiMaterialNode* pkDecodeInterpolatorsFrag = 
            GetAttachableNodeFromLibrary("DecodeInterpolators");
    EE_ASSERT(pkDecodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddNode(pkDecodeInterpolatorsFrag);

    // Bind the flags to control the encoding
    bool bInputWorldNormal = pkPixDesc->GetINPUT_WORLDNORMAL() != 0;
    bool bInputWorldPosition = pkPixDesc->GetINPUT_WORLDPOSITION() != 0;
    bool bInputWorldView = pkPixDesc->GetINPUT_WORLDVIEW() != 0;
    bool bInputWorldNBT = pkPixDesc->GetINPUT_WORLDNBT() != 0;
    kContext.m_spConfigurator->AddBinding(
        m_pkOperations->GenerateShaderConstant(kContext, bInputWorldNormal),
        "bInputWorldNormal", pkDecodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(
        m_pkOperations->GenerateShaderConstant(kContext, bInputWorldPosition),
        "bInputWorldPosition", pkDecodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(
        m_pkOperations->GenerateShaderConstant(kContext, bInputWorldView),
        "bInputWorldView", pkDecodeInterpolatorsFrag);
    kContext.m_spConfigurator->AddBinding(
        m_pkOperations->GenerateShaderConstant(kContext, bInputWorldNBT),
        "bInputWorldNBT", pkDecodeInterpolatorsFrag);

    // Calculate the number of interpolators that will be used:
    efd::UInt32 uiNumInterpolators = CalculateNumEncodedInterpolators(bInputWorldNormal, 
        bInputWorldPosition, bInputWorldView, bInputWorldNBT);

    // Input all the encoded coordinate interpolators
    for (efd::UInt32 uiIndex = 0; uiIndex < uiNumInterpolators; ++uiIndex)
    {
        // Get the input for the fragment
        NiString kInputName;
        kInputName.Format("texcoord%d", uiIndex);
        NiMaterialResource* pkInput = 
            pkDecodeInterpolatorsFrag->GetInputResourceByVariableName((const char*)kInputName);

        NiString kInterpolatorName;
        kInterpolatorName.Format("Encoded_TexCoord%d", uiIndex);
        NiMaterialResource* pkInterpolator = 
            kContext.m_spInputs->AddOutputResource("float4", "TexCoord", "", 
            (const char*)kInterpolatorName);
        kContext.m_spConfigurator->AddBinding(pkInterpolator, pkInput);
    }

    // Bind the other input variables
    if (pkPixDesc->GetNUM_LAYERS())
    {
        // The decode fragment also samples the blend mask texture as well so pass in a sampler.
        NiMaterialResource* pkBlendMapSampler = AddTextureSampler(kContext, 
            GenerateSamplerName("BlendMask", 0), 0);
        EE_ASSERT(pkBlendMapSampler);

        kContext.m_spConfigurator->AddBinding(pkBlendMapSampler, 
            pkDecodeInterpolatorsFrag->GetInputResourceByVariableName("layerMaskSampler"));
        kContext.m_spConfigurator->AddBinding(
            m_pkOperations->GenerateShaderConstant(kContext, true), 
            "sampleBlendMask", pkDecodeInterpolatorsFrag);
    }
    else
    {
        // Bind a dummy texture sampler that will get compiled out.
        kContext.m_spConfigurator->AddBinding(
            GetDummySampler(kContext), 
            pkDecodeInterpolatorsFrag->GetInputResourceByVariableName("layerMaskSampler"));
    }

    // Get the decoded output: (Compulsory outputs)
    pkMorphValue = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("morphValue");
    EE_ASSERT(pkMorphValue);
    pkMaskValues = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("maskValues");
    EE_ASSERT(pkMaskValues);
    pkLayer0UV = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("layer0UV");
    EE_ASSERT(pkLayer0UV);
    pkLayer1UV = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("layer1UV");
    EE_ASSERT(pkLayer1UV);
    pkLayer2UV = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("layer2UV");
    EE_ASSERT(pkLayer2UV);
    pkLayer3UV = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("layer3UV");
    EE_ASSERT(pkLayer3UV);
    pkLowResDiffuseUV = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName(
        "lowResDiffuseUV");
    EE_ASSERT(pkLowResDiffuseUV);
    pkLowResNormalUV = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("lowResNormalUV");
    EE_ASSERT(pkLowResNormalUV);

    // Get the decoded output: (Optional outputs)
    pkWorldView = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("worldViewOut");
    EE_ASSERT(pkWorldView);
    pkWorldViewTS = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("worldViewTSOut");
    EE_ASSERT(pkWorldViewTS);
    pkWorldPos = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("worldPosOut");
    EE_ASSERT(pkWorldPos);
    pkWorldNormal = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("worldNormalOut");
    EE_ASSERT(pkWorldNormal);
    pkNBTMatrix = pkDecodeInterpolatorsFrag->GetOutputResourceByVariableName("nbtMatrix");
    EE_ASSERT(pkNBTMatrix);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::HandlePixelInitialValues(Context& kContext,
    NiTerrainMaterialPixelDescriptor* pkPixelDesc,
    NiMaterialResource*& pkMatDiffuse, NiMaterialResource*& pkMatSpecular,
    NiMaterialResource*& pkSpecularPower, NiMaterialResource*& pkGlossiness,
    NiMaterialResource*& pkMatAmbient, NiMaterialResource*& pkMatEmissive,
    NiMaterialResource*& pkTexDiffuseAccum,
    NiMaterialResource*& pkTexSpecularAccum,
    NiMaterialResource*& pkDiffuseAccum, NiMaterialResource*& pkSpecularAccum,
    NiMaterialResource*& pkOpacityAccum,
    NiMaterialResource*& pkLightDiffuseAccum,
    NiMaterialResource*& pkLightSpecularAccum,
    NiMaterialResource*& pkLightAmbientAccum)
{
    // Extract the lighting descriptor from the pixel descriptor
    NiFragmentLighting::PixelDescriptor kLightingPixelDesc;
    m_pkLighting->GetDescriptor(pkPixelDesc, kLightingPixelDesc);

    // Generate initial values for pixel shader constants based on property
    // settings
    if (!m_pkLighting->HandlePixelMaterialInitialValues(kContext, 
        kLightingPixelDesc, 
        pkMatDiffuse, pkMatSpecular, pkSpecularPower,
        pkGlossiness, pkMatAmbient, pkMatEmissive, pkTexDiffuseAccum,
        pkTexSpecularAccum, pkDiffuseAccum, pkSpecularAccum, pkOpacityAccum,
        pkLightDiffuseAccum, pkLightSpecularAccum, pkLightAmbientAccum))
    {
        return false;
    }

    // Override the default material specular color
    pkMatSpecular = 
        m_pkOperations->GenerateShaderConstant(kContext, NiPoint3(1.0f, 1.0f, 1.0f));

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::HandleSurfaceLayers(Context& kContext, 
    NiTerrainMaterialPixelDescriptor* pkPixelDesc, 
    NiMaterialResource*& pkMaskValues, NiMaterialResource* pkWorldViewTS, 
    NiMaterialResource* pkLayerUV[4], 
    NiMaterialResource* pkNBTMatrix, NiMaterialResource* pkWorldNormal, 
    NiMaterialResource*& pkFinalLayerColor, NiMaterialResource*& pkFinalLayerNormal, 
    NiMaterialResource*& pkFinalLayerSpecular, NiMaterialResource*& pkFinalLayerParallax,
    NiMaterialResource*& pkFinalLayerGlossiness, NiMaterialResource*& pkTotalMask,
    NiMaterialResource*& pkFinalSpecularPower)
{
    EE_UNUSED_ARG(pkWorldNormal);
    NiUInt32 uiLayerCount = pkPixelDesc->GetNUM_LAYERS();
    if (!uiLayerCount)
    {
        return true;
    }

    // Before processing the layers we may need to modify each layer's texture coordinates based
    // on any parallax maps influencing the pixel.
    if (!HandleParallaxMaps(kContext, pkPixelDesc, pkMaskValues, pkLayerUV, pkWorldViewTS,
        pkFinalLayerParallax))
        return false;

    NiMaterialResource* pkLayerColors[4] = {NULL, NULL, NULL, NULL};
    NiMaterialResource* pkLayerNormals[4] = {NULL, NULL, NULL, NULL};
    NiMaterialResource* pkLayerSpec[4] = {NULL, NULL, NULL, NULL};
    NiMaterialResource* pkTrueConstant = m_pkOperations->GenerateShaderConstant(kContext, true);
    NiMaterialResource* pkFalseConstant = m_pkOperations->GenerateShaderConstant(kContext, false);

    // Set index to start at 2 since we have the dummy sampler and the blend mask first.
    NiUInt32 uiShaderMapOffset = 1;
    for (NiUInt32 ui = 0; ui < uiLayerCount; ui++)
    {
        NiMaterialNode* pkComputeLayerColorNode = GetAttachableNodeFromLibrary(
            "CalculateLayerColor");
        kContext.m_spConfigurator->AddNode(pkComputeLayerColorNode);

        kContext.m_spConfigurator->AddBinding(
            m_pkOperations->GenerateShaderConstant(kContext, (efd::SInt32)ui),
            "layerIndex", pkComputeLayerColorNode);

        kContext.m_spConfigurator->AddBinding(pkLayerUV[ui], "layerUV", pkComputeLayerColorNode);
        kContext.m_spConfigurator->AddBinding(pkMaskValues, "layerMaskIn", pkComputeLayerColorNode);
        kContext.m_spConfigurator->AddBinding(pkNBTMatrix, "nbtMat", pkComputeLayerColorNode);

        NiMaterialResource* pkDistRamp = AddOutputAttribute(kContext.m_spUniforms, 
            NiTerrainCellShaderData::DISTRIBUTION_RAMP_SHADER_CONSTANT, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);
        EE_ASSERT(pkDistRamp);
        kContext.m_spConfigurator->AddBinding(pkDistRamp, "dRamp", pkComputeLayerColorNode);

        NiMaterialResource* pkDetailMapScale = AddOutputAttribute(kContext.m_spUniforms, 
            NiTerrainCellShaderData::LAYER_DETAIL_TEXTURE_SCALE_SHADER_CONSTANT, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);
        EE_ASSERT(pkDetailMapScale);
        kContext.m_spConfigurator->AddBinding(pkDetailMapScale, "detailMapScale", 
            pkComputeLayerColorNode);

        bool bEnableBaseMap = pkPixelDesc->SupportsBaseMap(ui);
        bool bEnableDetailMap = pkPixelDesc->SupportsDetailMap(ui);
        bool bEnableNormalMap = pkPixelDesc->SupportsNormalMap(ui);
        bool bEnableParallaxMap = pkPixelDesc->SupportsParallaxMap(ui);
        bool bEnableSpecMap = pkPixelDesc->SupportsSpecularMap(ui);
        bool bEnableDistMap = pkPixelDesc->SupportsDistributionMap(ui);

        // Base map
        kContext.m_spConfigurator->AddBinding(
            bEnableBaseMap == true ? pkTrueConstant : pkFalseConstant, 
            "enableBaseMap", pkComputeLayerColorNode);

        // Distribution map
        kContext.m_spConfigurator->AddBinding(
            bEnableDistMap == true ? pkTrueConstant : pkFalseConstant, 
            "enableDistributionMap", pkComputeLayerColorNode);

        // Normal map
        kContext.m_spConfigurator->AddBinding(
            bEnableNormalMap == true ? pkTrueConstant : pkFalseConstant, 
            "enableNormalMap", pkComputeLayerColorNode);

        // Specular map
        kContext.m_spConfigurator->AddBinding(
            bEnableSpecMap == true ? pkTrueConstant : pkFalseConstant, 
            "enableSpecMap", pkComputeLayerColorNode);

        // Detail map
        kContext.m_spConfigurator->AddBinding(
            bEnableDetailMap == true ? pkTrueConstant : pkFalseConstant, 
            "enableDetailMap", pkComputeLayerColorNode);

        // Add texture samplers for the layer.
        if (bEnableBaseMap || bEnableDistMap)
        {
            efd::UInt32 uiShaderMapIndex = NiSurface::SURFACE_TEX_DIFFUSE_DETAIL;
            uiShaderMapIndex += uiShaderMapOffset;
            NiMaterialResource* pkTexture = AddTextureSampler(kContext, 
                GenerateSamplerName("LayerBaseMap", ui), uiShaderMapIndex);
            kContext.m_spConfigurator->AddBinding(pkTexture, "baseMap", pkComputeLayerColorNode);
        }
        else
        {
            kContext.m_spConfigurator->AddBinding(
                GetDummySampler(kContext), 
                "baseMap", pkComputeLayerColorNode);
        }

        if (bEnableNormalMap || bEnableParallaxMap)
        {
            efd::UInt32 uiShaderMapIndex = NiSurface::SURFACE_TEX_NORMAL_PARALLAX;
            uiShaderMapIndex += uiShaderMapOffset;
            NiMaterialResource* pkTexture = AddTextureSampler(kContext, 
                GenerateSamplerName("LayerNormalMap", ui), uiShaderMapIndex);
            kContext.m_spConfigurator->AddBinding(pkTexture, "normalMap", pkComputeLayerColorNode);
        }
        else
        {
            kContext.m_spConfigurator->AddBinding(
                GetDummySampler(kContext), 
                "normalMap", pkComputeLayerColorNode);
        }

        if (bEnableSpecMap || bEnableDetailMap)
        {
            efd::UInt32 uiShaderMapIndex = NiSurface::SURFACE_TEX_SPECULAR_DISTRIBUTION;
            uiShaderMapIndex += uiShaderMapOffset;
            NiMaterialResource* pkTexture = AddTextureSampler(kContext, 
                GenerateSamplerName("LayerSpecMap", ui), uiShaderMapIndex);
            kContext.m_spConfigurator->AddBinding(pkTexture, "specMap", pkComputeLayerColorNode);
        }
        else
        {
            kContext.m_spConfigurator->AddBinding(
                GetDummySampler(kContext), 
                "specMap", pkComputeLayerColorNode);
        }
        uiShaderMapOffset += NiSurface::NUM_SURFACE_TEXTURES;

        // Get all of the layer outputs...
        pkLayerColors[ui] = 
            pkComputeLayerColorNode->GetOutputResourceByVariableName("layerColor");
        pkLayerNormals[ui] = 
            pkComputeLayerColorNode->GetOutputResourceByVariableName("layerNormal");
        pkLayerSpec[ui] = 
            pkComputeLayerColorNode->GetOutputResourceByVariableName("layerSpecular");
        pkMaskValues = 
            pkComputeLayerColorNode->GetOutputResourceByVariableName("layerMaskOut");
    }

    // Now that we have all layers calculated merge them together.
    NiMaterialNode* pkCombineLayersNode = GetAttachableNodeFromLibrary("CombineAllLayers");
    kContext.m_spConfigurator->AddNode(pkCombineLayersNode);

    // Handle mask values
    if (pkMaskValues)
        kContext.m_spConfigurator->AddBinding(pkMaskValues, "maskValues", pkCombineLayersNode);
    
    // Handle default values
    kContext.m_spConfigurator->AddBinding(
        m_pkOperations->GenerateShaderConstant(kContext, ms_kDefaultColor), 
        "defaultColor", pkCombineLayersNode);
    kContext.m_spConfigurator->AddBinding(
        pkWorldNormal, 
        "defaultNormal", pkCombineLayersNode);
    kContext.m_spConfigurator->AddBinding(
        m_pkOperations->GenerateShaderConstant(kContext, NiPoint3::ZERO), 
        "defaultSpec", pkCombineLayersNode);

    for (NiUInt32 ui = 0; ui < NiTerrainMaterial::MAX_LAYERS_PER_CELL; ui++)
    {
        char acName[255];
        NiSnprintf(acName, 255, NI_TRUNCATE, "layer%dColor", ui);
        if (pkLayerColors[ui])
            kContext.m_spConfigurator->AddBinding(pkLayerColors[ui], acName, pkCombineLayersNode);

        NiSnprintf(acName, 255, NI_TRUNCATE, "layer%dNormal", ui);
        if (pkLayerNormals[ui])
            kContext.m_spConfigurator->AddBinding(pkLayerNormals[ui], acName, pkCombineLayersNode);

        NiSnprintf(acName, 255, NI_TRUNCATE, "layer%dSpec", ui);
        if (pkLayerSpec[ui])
            kContext.m_spConfigurator->AddBinding(pkLayerSpec[ui], acName, pkCombineLayersNode);
    }

    // Bind the outputs
    pkFinalLayerColor = pkCombineLayersNode->GetOutputResourceByVariableName("finalColor");
    EE_ASSERT(pkFinalLayerColor);

    pkFinalLayerNormal = pkCombineLayersNode->GetOutputResourceByVariableName("finalNormal");
    EE_ASSERT(pkFinalLayerNormal);

    pkFinalLayerSpecular = pkCombineLayersNode->GetOutputResourceByVariableName("finalSpecular");
    EE_ASSERT(pkFinalLayerSpecular);

    pkMaskValues = pkCombineLayersNode->GetOutputResourceByVariableName("maskValuesOut");
    EE_ASSERT(pkMaskValues);

    pkTotalMask = pkCombineLayersNode->GetOutputResourceByVariableName("finalMaskSum");
    EE_ASSERT(pkTotalMask);

    // Calculate the glossiness from the final specular
    m_pkOperations->ColorToLuminance(kContext, pkFinalLayerSpecular, pkFinalLayerGlossiness);
    EE_ASSERT(pkFinalLayerGlossiness);

    // Blend the specular powers
    {
        NiMaterialNode* pkCombineSpecularNode = GetAttachableNodeFromLibrary("CombineSpecularPowers");
        kContext.m_spConfigurator->AddNode(pkCombineSpecularNode);

        NiMaterialResource* pkLayerSpecularPowers = AddOutputAttribute(kContext.m_spUniforms, 
            NiTerrainCellShaderData::SPECULAR_POWER_SHADER_CONSTANT, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);
        NiMaterialResource* pkLayerSpecularIntensities = AddOutputAttribute(kContext.m_spUniforms, 
            NiTerrainCellShaderData::SPECULAR_INTENSITY_SHADER_CONSTANT, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);

        // Bind the inputs
        if (pkMaskValues)
            kContext.m_spConfigurator->AddBinding(pkMaskValues, "maskValues", pkCombineSpecularNode);

        kContext.m_spConfigurator->AddBinding(pkLayerSpecularPowers, "layerSpecularPowers", 
            pkCombineSpecularNode);
        kContext.m_spConfigurator->AddBinding(pkLayerSpecularIntensities, "layerSpecularIntensities", 
            pkCombineSpecularNode);

        for (efd::UInt32 uiLayer = 0; uiLayer < MAX_LAYERS_PER_CELL; ++uiLayer)
        {
            char acName[255];
            NiSnprintf(acName, 255, NI_TRUNCATE, "layer%dEnabled", uiLayer);
            
            kContext.m_spConfigurator->AddBinding(
                pkPixelDesc->SupportsSpecularMap(uiLayer) == true ? pkTrueConstant : pkFalseConstant, 
                acName, pkCombineSpecularNode);
        }

        // Bind the outputs
        pkFinalSpecularPower = pkCombineSpecularNode->GetOutputResourceByVariableName("finalSpecularPower");
        EE_ASSERT(pkFinalSpecularPower);

        NiMaterialResource* pkFinalSpecularIntensity = 
            pkCombineSpecularNode->GetOutputResourceByVariableName("finalSpecularIntensity");
        EE_ASSERT(pkFinalSpecularIntensity);

        // Now lerp the final glossiness with the intensity
        m_pkOperations->ScaleVector(kContext, pkFinalLayerGlossiness, pkFinalSpecularIntensity, 
            pkFinalLayerGlossiness);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::HandleParallaxMaps(Context& kContext, 
    NiTerrainMaterialPixelDescriptor* pkPixelDesc, NiMaterialResource* pkMaskValues, 
    NiMaterialResource** pkLayerUV, NiMaterialResource* pkWorldViewTS, 
    NiMaterialResource*& pkParallaxAccum)
{   
    const efd::UInt32 uiMaxNumLayers = NiTerrainMaterial::MAX_LAYERS_PER_CELL;

    // Generate common values
    NiMaterialResource* pkTrueConstant = m_pkOperations->GenerateShaderConstant(kContext, true);
    NiMaterialResource* pkFalseConstant = m_pkOperations->GenerateShaderConstant(kContext, false);

    // Bind the parameters into the parallax mapping node
    NiMaterialNode* pkParallaxNode = GetAttachableNodeFromLibrary("CalculateTotalParallax");
    EE_ASSERT(pkParallaxNode);
    kContext.m_spConfigurator->AddNode(pkParallaxNode);

    // Bind the tangent space view vector
    EE_ASSERT(pkWorldViewTS);
    kContext.m_spConfigurator->AddBinding(pkWorldViewTS, "worldViewTS", pkParallaxNode);

    // Bind the mask values
    EE_ASSERT(pkMaskValues);
    kContext.m_spConfigurator->AddBinding(pkMaskValues, "maskValues", pkParallaxNode);

    // Bind the parallax strengths
    NiMaterialResource* pkParallaxStrengths = AddOutputAttribute(kContext.m_spUniforms, 
        NiTerrainCellShaderData::PARALLAX_MAP_STRENGTH_SHADER_CONSTANT, 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);
    EE_ASSERT(pkParallaxStrengths);
    kContext.m_spConfigurator->AddBinding(pkParallaxStrengths, "strength", pkParallaxNode);

    // Bind the layer UV's and which layers are enabled
    for (efd::UInt32 uiLayer = 0; uiLayer < uiMaxNumLayers; ++uiLayer)
    {
        // Layer UV
        char acName[255];
        NiSnprintf(acName, 255, NI_TRUNCATE, "layerUV%dIn", uiLayer);
        if (pkLayerUV[uiLayer])
            kContext.m_spConfigurator->AddBinding(pkLayerUV[uiLayer], acName, pkParallaxNode);

        // Layer parallax enabled?
        bool bParallaxEnabled = pkPixelDesc->SupportsParallaxMap(uiLayer);
        char acEnabled[255];
        NiSnprintf(acEnabled, 255, NI_TRUNCATE, "layer%dEnabled", uiLayer);
        kContext.m_spConfigurator->AddBinding(
            (bParallaxEnabled ? pkTrueConstant : pkFalseConstant),
            acEnabled, pkParallaxNode);

        // Layer parallax map
        char acMap[255];
        NiSnprintf(acMap, 255, NI_TRUNCATE, "pMap%d", uiLayer);
        NiMaterialResource* pkMapResource = NULL;
        if (bParallaxEnabled)
        {
            efd::UInt32 uiShaderMapIndex = NiSurface::SURFACE_TEX_NORMAL_PARALLAX;
            uiShaderMapIndex += uiLayer * NiSurface::NUM_SURFACE_TEXTURES + 1;
            pkMapResource = AddTextureSampler(kContext, 
                GenerateSamplerName("LayerNormalMap", uiLayer), uiShaderMapIndex);
        }
        else
        {
            pkMapResource = GetDummySampler(kContext);
        }
        EE_ASSERT(pkMapResource);
        kContext.m_spConfigurator->AddBinding(pkMapResource, acMap, pkParallaxNode);
    }

    // Bind the outputs
    for (efd::UInt32 uiLayer = 0; uiLayer < uiMaxNumLayers; ++uiLayer)
    {
        char acName[255];
        NiSnprintf(acName, 255, NI_TRUNCATE, "layerUV%dOut", uiLayer);
        pkLayerUV[uiLayer] = pkParallaxNode->GetOutputResourceByVariableName(acName);
        EE_ASSERT(pkLayerUV[uiLayer]);
    }
    pkParallaxAccum = pkParallaxNode->GetOutputResourceByVariableName("totalParallax");
    EE_ASSERT(pkParallaxAccum);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::HandlePixelLighting(Context& kContext,
    NiTerrainMaterialPixelDescriptor* pkPixelDesc, NiMaterialResource* pkWorldPos, 
    NiMaterialResource* pkWorldView, NiMaterialResource* pkSpecularPower, 
    NiMaterialResource*& pkFinalNormal, NiMaterialResource* pkMatEmissive, 
    NiMaterialResource* pkMatDiffuse, NiMaterialResource* pkMatAmbient, 
    NiMaterialResource* pkMatSpecular, NiMaterialResource* pkLightSpecularAccum, 
    NiMaterialResource* pkLightDiffuseAccum, NiMaterialResource* pkLightAmbientAccum,
    NiMaterialResource* pkGlossiness, NiMaterialResource* pkFinalDiffuse,
    NiMaterialResource* pkFinalSpecular, 
    NiMaterialResource*& pkSpecularAccum, NiMaterialResource*& pkDiffuseAccum)
{
    // Extract the lighting descriptor from the pixel descriptor
    NiFragmentLighting::PixelDescriptor kLightingPixelDesc;
    m_pkLighting->GetDescriptor(pkPixelDesc, kLightingPixelDesc);

    // Perform the final accumulation of lighting for this pixel
    if (!m_pkLighting->HandleLighting(kContext, 
        kLightingPixelDesc,
        pkWorldPos, pkFinalNormal,
        pkWorldView, pkSpecularPower, pkLightAmbientAccum,
        pkLightDiffuseAccum, pkLightSpecularAccum, NULL))
    {
        return false;
    }

    // Merge all lighting calculations into specular and diffuse values
    if (!m_pkLighting->HandleColorAccumulation(kContext, kLightingPixelDesc, 
        pkMatEmissive, pkMatDiffuse, pkMatAmbient, pkMatSpecular, 
        pkLightSpecularAccum, pkLightDiffuseAccum, pkLightAmbientAccum,
        pkGlossiness, pkFinalDiffuse, pkFinalSpecular,
        pkSpecularAccum, pkDiffuseAccum))
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::NiTerrainFragmentLighting::HandleLight(Context& kContext, NiUInt32 uiLight,
    LightType eType, NiUInt32 uiLightByType,
    NiMaterialResource* pkWorldPos,
    NiMaterialResource* pkWorldNorm, NiMaterialResource* pkViewVector,
    NiMaterialResource* pkSpecularPower,
    NiMaterialResource*& pkAmbientAccum,
    NiMaterialResource*& pkDiffuseAccum,
    NiMaterialResource*& pkSpecularAccum,
    bool bSpecular, NiMaterialResource* pkShadow, 
    ExtraLightingData kExtraData)
{
    return NiFragmentLighting::HandleLight(kContext, uiLight, eType, uiLightByType,
        pkWorldPos, pkWorldNorm, pkViewVector, pkSpecularPower, pkAmbientAccum, pkDiffuseAccum, 
        pkSpecularAccum, bSpecular, pkShadow, kExtraData);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::HandleFinalPixelOutputs(Context& kContext,
    NiTerrainMaterialPixelDescriptor* pkPixelDesc,
    NiMaterialResource* pkDiffuseAccum,
    NiMaterialResource* pkSpecularAccum,
    NiMaterialResource* pkOpacityAccum, NiMaterialResource* pkGlossiness,
    NiMaterialResource* pkFinalNormal, NiMaterialResource* pkParallaxOffset,
    NiMaterialResource* pkMorphValue, NiMaterialResource* pkTotalMask,
    NiMaterialResource* pkWorldPos, NiMaterialResource* pkSpecularPower)
{
    EE_UNUSED_ARG(pkWorldPos); // Useful for overridden materials
    EE_UNUSED_ARG(pkSpecularPower); // Useful for overridden materials

    // Add output color resource
    NiMaterialResource* pkPixelOutColor =
        kContext.m_spOutputs->AddInputResource("float4", "Color", "",
        "Color0");

    NiMaterialResource* pkFinalColor = NULL;
    NiMaterialNode* pkNode = GetAttachableNodeFromLibrary(
        "CompositeFinalRGBColor");
    kContext.m_spConfigurator->AddNode(pkNode);

    if (pkDiffuseAccum)
    {
        kContext.m_spConfigurator->AddBinding(pkDiffuseAccum,
            "DiffuseColor", pkNode);
    }

    if (pkSpecularAccum)
    {
        kContext.m_spConfigurator->AddBinding(pkSpecularAccum,
            "SpecularColor", pkNode);
    }

    pkFinalColor = pkNode->GetOutputResourceByVariableName("OutputColor");

    if (!HandleApplyFog(kContext, pkPixelDesc, pkFinalColor, pkFinalColor))
        return false;

    // Output a pulsing effect if render mode is pulse
    if (pkPixelDesc->GetRENDER_MODE() == NiTerrainCellShaderData::PULSE_UNPAINTED)
    {
        if (!HandlePulseUnpainted(kContext, pkFinalColor, pkTotalMask, pkFinalColor))
        {
            return false;
        }
    }

    if (!HandlePixelDebuging(kContext, pkPixelDesc, pkFinalNormal, 
        pkParallaxOffset, pkMorphValue, pkTotalMask, pkGlossiness, pkFinalColor))
    {
        return false;
    }

    // Output the specular in the alpha channel when baking diffuse
    if (pkPixelDesc->GetRENDER_MODE() == NiTerrainCellShaderData::BAKE_DIFFUSE)
    {
        if (pkGlossiness)
            pkOpacityAccum = pkGlossiness;
    }

    pkNode = GetAttachableNodeFromLibrary(
        "CompositeFinalRGBAColor");
    EE_ASSERT(pkNode);
    kContext.m_spConfigurator->AddNode(pkNode);

    kContext.m_spConfigurator->AddBinding(pkFinalColor, "FinalColor", pkNode);
    if (pkOpacityAccum)
    {
        m_pkOperations->HandleAlphaTest(kContext, 
            false, 
            pkOpacityAccum);

        kContext.m_spConfigurator->AddBinding(pkOpacityAccum,
            "FinalOpacity", pkNode);
    }

    kContext.m_spConfigurator->AddBinding("OutputColor", pkNode,
        pkPixelOutColor);

    return true;
}

//---------------------------------------------------------------------------
bool NiTerrainMaterial::HandlePulseUnpainted(Context& kContext, NiMaterialResource* pkFinalColor,
    NiMaterialResource* pkTotalMask, NiMaterialResource*& pkOutputColor)
{
    if (pkTotalMask)
    {
        NiMaterialResource* pkYellowColor = 
            m_pkOperations->GenerateShaderConstant(kContext, NiColor(1.0f, 1.0f, 0.0f));
        NiMaterialResource* pkRedColor = 
            m_pkOperations->GenerateShaderConstant(kContext, NiColor(1.0f, 0.0f, 0.0f));
        NiMaterialResource* pkTime = AddOutputPredefined(
            kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_CONSTS_TIME);

        NiMaterialNode* pkPulseNode = GetAttachableNodeFromLibrary("PulseUnpaintedArea");
        kContext.m_spConfigurator->AddNode(pkPulseNode);

        kContext.m_spConfigurator->AddBinding(pkYellowColor,
            pkPulseNode->GetInputResourceByVariableName("LowColor"));
        kContext.m_spConfigurator->AddBinding(pkRedColor,
            pkPulseNode->GetInputResourceByVariableName("HighColor"));
        kContext.m_spConfigurator->AddBinding(pkFinalColor,
            pkPulseNode->GetInputResourceByVariableName("NormalColor"));
        kContext.m_spConfigurator->AddBinding(pkTotalMask,
            pkPulseNode->GetInputResourceByVariableName("TotalMask"));
        kContext.m_spConfigurator->AddBinding(pkTime,
            pkPulseNode->GetInputResourceByVariableName("Time"));

        pkOutputColor = pkPulseNode->GetOutputResourceByVariableName("OutputColor");
    }
    
    return true;
}

//---------------------------------------------------------------------------
bool NiTerrainMaterial::HandlePixelDebuging(Context& kContext,
    NiTerrainMaterialPixelDescriptor* pkPixelDesc,
    NiMaterialResource* pkFinalNormal, NiMaterialResource* pkParallaxOffset,
    NiMaterialResource* pkMorphValue, NiMaterialResource* pkTotalMask, 
    NiMaterialResource* pkGlossiness, NiMaterialResource*& pkFinalColor)
{
    EE_UNUSED_ARG(pkParallaxOffset);

    // Modify the output based on debug configuration
    switch(pkPixelDesc->GetDEBUG_MODE())
    {
    case NiTerrainCellShaderData::DEBUG_SHOW_NORMALS:
        pkFinalColor = pkFinalNormal;
        break;

    case NiTerrainCellShaderData::DEBUG_SHOW_MORPH_VALUE:
        pkFinalColor = pkMorphValue;
        break;

    case NiTerrainCellShaderData::DEBUG_SHOW_NUM_SURFACES:
        if (pkPixelDesc->GetNUM_LAYERS())
        {
            float fUsedLayers = float(pkPixelDesc->GetNUM_LAYERS()) / 
                float(4);
            NiMaterialResource* pkGreenColor = 
                m_pkOperations->GenerateShaderConstant(kContext, NiColor(0.0f, 1.0f, 0.0f));
            NiMaterialResource* pkRedColor = 
                m_pkOperations->GenerateShaderConstant(kContext, NiColor(1.0f, 0.0f, 0.0f));
            NiString kNumSurfaceStr;
            kNumSurfaceStr.Format("(%f)", fUsedLayers);
            NiMaterialResource* pkNumSurfaces =
                kContext.m_spStatics->AddOutputConstant("float", 
                (const char*)kNumSurfaceStr);
            m_pkOperations->LerpVector(kContext, pkGreenColor, pkRedColor,
                pkNumSurfaces, pkFinalColor);
        }
        break;

    case NiTerrainCellShaderData::DEBUG_SHOW_UNPAINTED_AREA:
        if (pkPixelDesc->GetNUM_LAYERS())
        {
            NiMaterialResource* pkGreyColor = 
                m_pkOperations->GenerateShaderConstant(kContext, NiColor(0.5f, 0.5f, 0.5f));
            NiMaterialResource* pkRedColor = 
                m_pkOperations->GenerateShaderConstant(kContext, NiColor(1.0f, 0.0f, 0.0f));
            m_pkOperations->LerpVector(kContext, pkRedColor, pkGreyColor,
                pkTotalMask, pkFinalColor);
        }
        break;
    case NiTerrainCellShaderData::DEBUG_SHOW_GLOSSINESS:
        if (pkGlossiness)
        {
            pkFinalColor = pkGlossiness;
        }
        break;
    }

    // Make sure the final color is a float3
    m_pkOperations->TypeCast(kContext, "float3", pkFinalColor, pkFinalColor);
    EE_ASSERT(pkFinalColor);

    return true;
}
//---------------------------------------------------------------------------
bool NiTerrainMaterial::HandleApplyFog(Context& kContext, 
    NiTerrainMaterialPixelDescriptor* pkPixelDesc,
    NiMaterialResource* pkUnfoggedColor,
    NiMaterialResource*& pkFogOutput)
{
    // At this time, all other platforms require this fog calculation.
    if (pkPixelDesc->GetFOGTYPE() != FOG_NONE)
    {
        if (kContext.m_spConfigurator->GetPlatformString() == "DX9")
        {
            // DX9 uses HLSL with varying shader targets
            // Fog should only be applied by the pixel shader in
            // SM 3.0 or greater
            NiGPUProgramCache* pkCache =
                m_aspProgramCaches[NiGPUProgram::PROGRAM_PIXEL];
            NiFixedString kShaderTarget = pkCache->GetShaderProfile();

            // In SM 2.0, fog is applied automatically by the hardware,
            // so we simply return true and do not apply fog in the
            // pixel shader.
            if (strstr(kShaderTarget, "ps_2_"))
            {
                return true;
            }
        }

        NiMaterialNode* pkFogNode =
            GetAttachableNodeFromLibrary("ApplyFog");
        kContext.m_spConfigurator->AddNode(pkFogNode);

        NiMaterialResource* pkFogResource =
            kContext.m_spInputs->AddOutputResource("float", "Fog", "",
            "FogDepth");

        kContext.m_spConfigurator->AddBinding(
            pkFogResource,
            pkFogNode->GetInputResourceByVariableName("FogAmount"));

        kContext.m_spConfigurator->AddBinding(pkUnfoggedColor,
            pkFogNode->GetInputResourceByVariableName("UnfoggedColor"));

        NiMaterialResource* pkFogColor = AddOutputPredefined(
            kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_FOG_COLOR);
        kContext.m_spConfigurator->AddBinding(pkFogColor,
            pkFogNode->GetInputResourceByVariableName("FogColor"));

        pkFogOutput =
            pkFogNode->GetOutputResourceByVariableName("FoggedColor");
    }
    else
    {
        pkFogOutput = pkUnfoggedColor;
    }

    return true;
}

//---------------------------------------------------------------------------
void NiTerrainMaterial::HandleSampleLowDetailDiffuse(Context& kContext, 
    NiTerrainMaterialPixelDescriptor* pkPixelDesc, NiMaterialResource* pkUV, 
    NiMaterialResource*& pkLowDetailDiffuseColor, NiMaterialResource*& pkLowDetailSpecularColor,
    NiMaterialResource*& pkLowDetailGlossiness, NiMaterialResource*& pkLowDetailSpecularPower)
{
    if (pkPixelDesc->GetUSE_LOWDETAILDIFFUSE() == 0)
        return;
    EE_ASSERT(pkUV);
    
    // Find the appropriate sampler
    NiFixedString kSamplerName = NiTexturingProperty::GetMapNameFromID(
        NiTexturingProperty::BASE_INDEX);
    NiMaterialResource* pkSampler = 
        kContext.m_spUniforms->GetInputResourceByVariableName(kSamplerName);
    if (!pkSampler)
    {
        pkSampler = kContext.m_spUniforms->AddOutputResource(
            "sampler2D", kSamplerName, "",
            kSamplerName, 1, NiMaterialResource::SOURCE_PREDEFINED, 
            NiShaderAttributeDesc::OT_UNDEFINED, 0);
    }
    EE_ASSERT(pkSampler);

    // Fetch shader node
    NiMaterialNode* pkSampleBaseMap = GetAttachableNodeFromLibrary(
        "SampleLowDetailDiffuseMap");
    EE_ASSERT(pkSampleBaseMap);
    kContext.m_spConfigurator->AddNode(pkSampleBaseMap);

    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkUV,
        pkSampleBaseMap->GetInputResourceByVariableName("UV"));
    kContext.m_spConfigurator->AddBinding(pkSampler,
        pkSampleBaseMap->GetInputResourceByVariableName("Sampler"));

    // Bind Outputs
    pkLowDetailDiffuseColor = pkSampleBaseMap->GetOutputResourceByVariableName("Diffuse");
    EE_ASSERT(pkLowDetailDiffuseColor);

    if (pkPixelDesc->GetSPECULAR() == 1)
    {
        // Fetch the low detail specular configuration
        NiMaterialResource* pkLowDetailSpecularData = AddOutputAttribute(kContext.m_spUniforms, 
            NiTerrainCellShaderData::LOWDETAIL_SPECULAR_SHADER_CONSTANT, 
            NiShaderAttributeDesc::ATTRIB_TYPE_POINT2);

        NiMaterialNode* pkLowDetailSpecDataBreakoutNode = GetAttachableNodeFromLibrary(
            "BreakoutLowDetailSpecularConfig");
        kContext.m_spConfigurator->AddNode(pkLowDetailSpecDataBreakoutNode);

        // Bind inputs:
        kContext.m_spConfigurator->AddBinding(pkLowDetailSpecularData,
            "LowDetailSpecularData", pkLowDetailSpecDataBreakoutNode);

        // Bind outputs:
        pkLowDetailSpecularPower = 
            pkLowDetailSpecDataBreakoutNode->GetOutputResourceByVariableName(
                "lowDetailSpecularPower");
        NiMaterialResource* pkLowDetailSpecularIntensity = 
            pkLowDetailSpecDataBreakoutNode->GetOutputResourceByVariableName(
                "lowDetailSpecularIntensity");
        EE_ASSERT(pkLowDetailSpecularPower);
        EE_ASSERT(pkLowDetailSpecularIntensity);
        
        pkLowDetailSpecularColor = pkLowDetailDiffuseColor;
        pkLowDetailGlossiness = pkSampleBaseMap->GetOutputResourceByVariableName("Glossiness");
        
        // Scale glossiness by intensity
        m_pkOperations->ScaleVector(kContext, pkLowDetailGlossiness, pkLowDetailSpecularIntensity, 
            pkLowDetailGlossiness);
    }
    else
    {
        pkLowDetailSpecularColor = 
            m_pkOperations->GenerateShaderConstant(kContext, NiPoint3::ZERO);
        pkLowDetailGlossiness = 
            m_pkOperations->GenerateShaderConstant(kContext, float(0.0f));
        pkLowDetailSpecularPower = 
            m_pkOperations->GenerateShaderConstant(kContext, NiPoint4(100,100,100,100));
    }
    EE_ASSERT(pkLowDetailGlossiness);
    EE_ASSERT(pkLowDetailSpecularPower);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterial::HandleSampleLowDetailNormal(Context& kContext, 
    NiTerrainMaterialPixelDescriptor* pkPixelDesc, NiMaterialResource* pkUV,
    NiMaterialResource*& pkLowDetailNormal)
{
    // If there is no base normal map then do not attempt to fetch the normal 
    // from the texture
    if (pkPixelDesc->GetUSE_LOWDETAILNORMAL() == 0)
        return;
    EE_ASSERT(pkUV);

    // Locate the appropriate sampler
    NiFixedString kSamplerName = NiTexturingProperty::GetMapNameFromID(
        NiTexturingProperty::NORMAL_INDEX);
    NiMaterialResource* pkSampler = 
        kContext.m_spUniforms->AddOutputResource(
            "sampler2D", kSamplerName, "",
            kSamplerName, 1, NiMaterialResource::SOURCE_PREDEFINED, 
            NiShaderAttributeDesc::OT_UNDEFINED, 0);
    
    // Locate the world transform
    NiMaterialResource* pkWorldMatrix = AddOutputPredefined(
        kContext.m_spUniforms, NiShaderConstantMap::SCM_DEF_WORLD, 4);
    EE_ASSERT(pkWorldMatrix);

    // Fetch the sampling node
    NiMaterialNode* pkSampleBaseNormalMap = GetAttachableNodeFromLibrary(
        "SampleLowDetailNormalMap");
    EE_ASSERT(pkSampleBaseNormalMap);
    kContext.m_spConfigurator->AddNode(pkSampleBaseNormalMap);

    // Bind Inputs
    kContext.m_spConfigurator->AddBinding(pkUV, 
        "UV", pkSampleBaseNormalMap);
    kContext.m_spConfigurator->AddBinding(pkSampler, 
        "Sampler", pkSampleBaseNormalMap);
    kContext.m_spConfigurator->AddBinding(pkWorldMatrix, 
        "WorldMatrix", pkSampleBaseNormalMap);

    // Bind Outputs
    pkLowDetailNormal = pkSampleBaseNormalMap->GetOutputResourceByVariableName("Output");    
    EE_ASSERT(pkLowDetailNormal);
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterial::HandleTextureMorphing(Context& kContext, 
    NiTerrainMaterialPixelDescriptor* pkPixelDesc,
    NiMaterialResource* pkLowDetailDiffuseColor, 
    NiMaterialResource* pkBlendDiffuseAccum,
    NiMaterialResource* pkWorldNormal,        
    NiMaterialResource* pkLowDetailNormalValue,
    NiMaterialResource* pkBlendNormalAccum,
    NiMaterialResource* pkLowDetailGlossiness,
    NiMaterialResource* pkLowDetailSpecular,
    NiMaterialResource* pkLowDetailSpecularPower,
    NiMaterialResource* pkBlendGlossiness,
    NiMaterialResource* pkBlendSpecular,
    NiMaterialResource* pkBlendSpecularPower,
    NiMaterialResource* pkMorphValue,
    NiMaterialResource*& pkDiffuseColor,
    NiMaterialResource*& pkSpecularColor,
    NiMaterialResource*& pkNormal,
    NiMaterialResource*& pkGlossiness,
    NiMaterialResource*& pkSpecularPower)
{
    EE_UNUSED_ARG(pkPixelDesc);
    
    // Morph the low detail diffuse texture with high detail if required
    if (pkLowDetailDiffuseColor && pkBlendDiffuseAccum)
    {
        // Blending between high and low detail
        m_pkOperations->LerpVector(kContext, pkBlendDiffuseAccum, pkLowDetailDiffuseColor, 
            pkMorphValue, pkDiffuseColor);
    }
    else if (pkLowDetailDiffuseColor)
    {
        // Rendering low detail cells
        pkDiffuseColor = pkLowDetailDiffuseColor;
    }
    else if (pkBlendDiffuseAccum)
    {
        // Rendering low detail diffuse texture out of high detail cells
        pkDiffuseColor = pkBlendDiffuseAccum;
    }
    else
    {
        // No maps assigned, use default
        pkDiffuseColor = m_pkOperations->GenerateShaderConstant(kContext, ms_kDefaultColor);
    }
    EE_ASSERT(pkDiffuseColor);

    // Morph the normals with high detail normals if required
    if (!pkLowDetailNormalValue)
    {
        EE_ASSERT(!pkPixelDesc->GetUSE_LOWDETAILNORMAL());
        pkLowDetailNormalValue = pkWorldNormal;
    }

    if (pkBlendNormalAccum)
    {
        m_pkOperations->LerpVector(kContext, pkBlendNormalAccum, pkLowDetailNormalValue, 
            pkMorphValue, pkNormal);
    }
    else 
    {
        pkNormal = pkLowDetailNormalValue;
    }
    EE_ASSERT(pkNormal);

    // Morph the glossiness
    if (!pkLowDetailGlossiness)
        pkLowDetailGlossiness = pkBlendGlossiness;
    if (!pkLowDetailGlossiness)
        pkLowDetailGlossiness = m_pkOperations->GenerateShaderConstant(kContext, float(0.0f));
    
    // Morph the specularity
    if (!pkLowDetailSpecular)
    {
        pkLowDetailSpecular = pkBlendSpecular;
        pkLowDetailSpecularPower = pkBlendSpecularPower;
    }
    if (!pkLowDetailSpecular)
    {
        pkLowDetailSpecular = m_pkOperations->GenerateShaderConstant(kContext, NiPoint3::ZERO);
        pkLowDetailSpecularPower = m_pkOperations->GenerateShaderConstant(kContext, 
            NiPoint4(100,100,100,100));
    }

    if (pkPixelDesc->GetNUM_LAYERS())
    {
        // Lerp the glossiness value
        m_pkOperations->LerpVector(kContext, pkBlendGlossiness, 
            pkLowDetailGlossiness, pkMorphValue, 
            pkGlossiness);

        // Lerp the low detail specular power
        m_pkOperations->LerpVector(kContext, pkBlendSpecularPower,
            pkLowDetailSpecularPower, pkMorphValue,
            pkSpecularPower);

        // Lerp the low detail specular value out to 0
        m_pkOperations->LerpVector(kContext, pkBlendSpecular, 
            pkLowDetailSpecular, pkMorphValue, 
            pkSpecularColor);
    }
    else
    {
        // Set the low detail glossiness
        pkGlossiness = pkLowDetailGlossiness;

        // Specular power
        pkSpecularPower = pkLowDetailSpecularPower;

        // Specular color
        pkSpecularColor = pkLowDetailSpecular;
    }
    EE_ASSERT(pkGlossiness);
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiTerrainMaterial::VerifyShaderPrograms(NiGPUProgram* pkVertexShader,
    NiGPUProgram*, NiGPUProgram* pkPixelShader)
{
    NiUInt32 uiReturnCode = RC_SUCCESS;
    if (pkVertexShader == NULL)
        uiReturnCode |= RC_COMPILE_FAILURE_VERTEX;
    if (pkPixelShader == NULL)
        uiReturnCode |= RC_COMPILE_FAILURE_PIXEL;
    // No need to check geometry shader

    return uiReturnCode;
}

//---------------------------------------------------------------------------
void NiTerrainMaterial::AddDefaultFallbacks()
{
    AddShaderFallbackFunc(SplitPerPixelLights);
    AddShaderFallbackFunc(DropPSSMShadowMaps);
    AddShaderFallbackFunc(DropPSSMShadowMapsThenSplitPerPixelLights);
    AddShaderFallbackFunc(DropShadowMaps);    
    AddShaderFallbackFunc(DropShadowMapsThenSplitPerPixelLights);
    AddShaderFallbackFunc(SplitPerVertexLights);
    AddShaderFallbackFunc(DropNormalParallaxMap);
    AddShaderFallbackFunc(DropNormalParallaxMapThenSplitLights);
    AddShaderFallbackFunc(
        DropPSSMShadowMapsThenNormalParallaxMapThenSplitLights);
    AddShaderFallbackFunc(
        DropShadowMapsThenDropNormalParallaxMapThenSplitLights);
    AddShaderFallbackFunc(UseTerrainReplacementShader);

    NiFragmentMaterial::AddDefaultFallbacks();
}

//---------------------------------------------------------------------------
bool NiTerrainMaterial::SplitPerPixelLights(
    NiMaterialDescriptor*, ReturnCode eFailedRC,
    unsigned int uiFailedPass, RenderPassDescriptor* pkRenderPasses,
    unsigned int uiMaxCount, unsigned int& uiCount,
    unsigned int&)
{
    return NiFragmentLighting::Fallbacks<
        NiTerrainMaterialDescriptor,
        NiTerrainMaterialVertexDescriptor,
        NiTerrainMaterialPixelDescriptor>::SplitPerPixelLights(
            eFailedRC, uiFailedPass, pkRenderPasses, uiMaxCount, uiCount);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::DropPSSMShadowMaps(
    NiMaterialDescriptor* pkMaterialDescriptor, ReturnCode eFailedRC,
    unsigned int uiFailedPass, RenderPassDescriptor* pkRenderPasses,
    unsigned int uiMaxCount, unsigned int& uiCount,
    unsigned int& uiFunctionData)
{
    EE_UNUSED_ARG(pkMaterialDescriptor);
    EE_UNUSED_ARG(uiCount);
    EE_UNUSED_ARG(uiMaxCount);
    EE_UNUSED_ARG(uiFunctionData);

    return NiFragmentLighting::Fallbacks<
        NiTerrainMaterialDescriptor,
        NiTerrainMaterialVertexDescriptor,
        NiTerrainMaterialPixelDescriptor>::DropPSSMShadowMaps(
            eFailedRC, uiFailedPass, pkRenderPasses);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::DropPSSMShadowMapsThenSplitPerPixelLights(
    NiMaterialDescriptor* pkMaterialDescriptor, ReturnCode eFailedRC,
    unsigned int uiFailedPass, RenderPassDescriptor* pkRenderPasses,
    unsigned int uiMaxCount, unsigned int& uiCount,
    unsigned int& uiFunctionData)
{
    // The first time this function is encountered, uiFunctionData should be 0
    if (uiFunctionData == 0)
    {
        uiFunctionData = 1;
        // If DropPSSMShadowMaps returns false (meaning it can't do anything)
        // then there's no point continuing this fallback either.
        return DropPSSMShadowMaps(pkMaterialDescriptor, eFailedRC,
            uiFailedPass, pkRenderPasses, uiMaxCount,
            uiCount, uiFunctionData);
    }

    // In subsequent iterations, attempt to split up the per-pixel lights
    return SplitPerPixelLights(pkMaterialDescriptor, eFailedRC,
        uiFailedPass, pkRenderPasses, uiMaxCount, uiCount, uiFunctionData);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::DropShadowMaps(
    NiMaterialDescriptor* pkMaterialDescriptor, ReturnCode eFailedRC,
    unsigned int uiFailedPass, RenderPassDescriptor* pkRenderPasses,
    unsigned int uiMaxCount, unsigned int& uiCount,
    unsigned int& uiFunctionData)
{
    EE_UNUSED_ARG(pkMaterialDescriptor);
    EE_UNUSED_ARG(uiCount);
    EE_UNUSED_ARG(uiMaxCount);
    EE_UNUSED_ARG(uiFunctionData);

    return NiFragmentLighting::Fallbacks<
        NiTerrainMaterialDescriptor,
        NiTerrainMaterialVertexDescriptor,
        NiTerrainMaterialPixelDescriptor>::DropShadowMaps(
            eFailedRC, uiFailedPass, pkRenderPasses);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::DropShadowMapsThenSplitPerPixelLights(
    NiMaterialDescriptor* pkMaterialDescriptor, ReturnCode eFailedRC,
    unsigned int uiFailedPass, RenderPassDescriptor* pkRenderPasses,
    unsigned int uiMaxCount, unsigned int& uiCount,
    unsigned int& uiFunctionData)
{
    // Attempt to remove the shadow maps,
    // and then split up the per-pixel lights

    // The first time this function is encountered, uiFunctionData should be 0
    if (uiFunctionData == 0)
    {
        uiFunctionData = 1;
        // If DropShadowMaps returns false (meaning it can't do anything)
        // then there's no point continuing this fallback either.
        return DropShadowMaps(pkMaterialDescriptor, eFailedRC,
            uiFailedPass, pkRenderPasses, uiMaxCount,
            uiCount, uiFunctionData);
    }

    // In subsequent iterations, attempt to split up the per-pixel lights
    return SplitPerPixelLights(pkMaterialDescriptor, eFailedRC,
        uiFailedPass, pkRenderPasses, uiMaxCount, uiCount, uiFunctionData);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::SplitPerVertexLights(
    NiMaterialDescriptor*, ReturnCode eFailedRC,
    unsigned int uiFailedPass, RenderPassDescriptor* pkRenderPasses,
    unsigned int uiMaxCount, unsigned int& uiCount,
    unsigned int&)
{
    return NiFragmentLighting::Fallbacks<
        NiTerrainMaterialDescriptor,
        NiTerrainMaterialVertexDescriptor,
        NiTerrainMaterialPixelDescriptor>::SplitPerVertexLights(
            eFailedRC, uiFailedPass, pkRenderPasses, uiMaxCount, uiCount);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::DropNormalParallaxMapThenSplitLights(
    NiMaterialDescriptor* pkMaterialDescriptor, ReturnCode eFailedRC,
    unsigned int uiFailedPass, RenderPassDescriptor* pkRenderPasses,
    unsigned int uiMaxCount, unsigned int& uiCount,
    unsigned int& uiFunctionData)
{
    // Attempt to remove the parallax map,
    // and then split up the per-pixel lights

    // The first time this function is encountered, uiFunctionData should be 0
    if (uiFunctionData == 0)
    {
        uiFunctionData = 1;
        // If DropParallaxMap returns false (meaning it can't do anything)
        // then there's no point continuing this fallback either.
        return DropNormalParallaxMap(pkMaterialDescriptor, eFailedRC,
            uiFailedPass, pkRenderPasses, uiMaxCount,
            uiCount, uiFunctionData);
    }

    // In subsequent iterations, attempt to split up the per-pixel lights.
    return SplitPerPixelLights(pkMaterialDescriptor, eFailedRC,
        uiFailedPass, pkRenderPasses, uiMaxCount, uiCount, uiFunctionData);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::DropPSSMShadowMapsThenNormalParallaxMapThenSplitLights(
    NiMaterialDescriptor* pkMaterialDescriptor, ReturnCode eFailedRC,
    unsigned int uiFailedPass, RenderPassDescriptor* pkRenderPasses,
    unsigned int uiMaxCount, unsigned int& uiCount,
    unsigned int& uiFunctionData)
{
    // Attempt to remove the PSSM enabled shadow maps,
    // and then remove the parallax map,
    // and then split up the per-pixel lights

    // The first time this function is encountered, uiFunctionData should be 0
    if (uiFunctionData == 0)
    {
        uiFunctionData = 1;
        // If DropPSSMShadowMaps returns false (meaning it can't do anything)
        // then there's no point continuing this fallback either.
        return DropPSSMShadowMaps(pkMaterialDescriptor, eFailedRC,
            uiFailedPass, pkRenderPasses, uiMaxCount,
            uiCount, uiFunctionData);
    }

    // The second time this function is encountered, uiFunctionData should be 1
    if (uiFunctionData == 1)
    {
        uiFunctionData = 2;
        // If DropParallaxMap returns false (meaning it can't do anything)
        // then there's no point continuing this fallback either.
        return DropNormalParallaxMap(pkMaterialDescriptor, eFailedRC,
            uiFailedPass, pkRenderPasses, uiMaxCount,
            uiCount, uiFunctionData);
    }

    // In subsequent iterations, attempt to split up the per-pixel lights
    return SplitPerPixelLights(pkMaterialDescriptor, eFailedRC,
        uiFailedPass, pkRenderPasses, uiMaxCount, uiCount, uiFunctionData);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::DropShadowMapsThenDropNormalParallaxMapThenSplitLights(
    NiMaterialDescriptor* pkMaterialDescriptor, ReturnCode eFailedRC,
    unsigned int uiFailedPass, RenderPassDescriptor* pkRenderPasses,
    unsigned int uiMaxCount, unsigned int& uiCount,
    unsigned int& uiFunctionData)
{
    // Attempt to remove the shadow maps,
    // and then remove the parallax map,
    // and then split up the per-pixel lights

    // The first time this function is encountered, uiFunctionData should be 0
    if (uiFunctionData == 0)
    {
        uiFunctionData = 1;
        // If DropShadowMaps returns false (meaning it can't do anything)
        // then there's no point continuing this fallback either.
        return DropShadowMaps(pkMaterialDescriptor, eFailedRC,
            uiFailedPass, pkRenderPasses, uiMaxCount,
            uiCount, uiFunctionData);
    }

    // The second time this function is encountered, uiFunctionData should be
    // 1
    if (uiFunctionData == 1)
    {
        uiFunctionData = 2;
        // If DropParallaxMap returns false (meaning it can't do anything)
        // then there's no point continuing this fallback either.
        return DropNormalParallaxMap(pkMaterialDescriptor, eFailedRC,
            uiFailedPass, pkRenderPasses, uiMaxCount,
            uiCount, uiFunctionData);
    }

    // In subsequent iterations, attempt to split up the per-pixel lights
    return SplitPerPixelLights(pkMaterialDescriptor, eFailedRC,
        uiFailedPass, pkRenderPasses, uiMaxCount, uiCount, uiFunctionData);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::DropNormalParallaxMap(
    NiMaterialDescriptor*, ReturnCode eFailedRC,
    unsigned int uiFailedPass, RenderPassDescriptor* pkRenderPasses,
    unsigned int, unsigned int&, unsigned int&)
{
    // This function can only deal with failed pixel or vertex shader compiles
    if ((eFailedRC & RC_COMPILE_FAILURE_PIXEL) == 0 &&
        (eFailedRC & RC_COMPILE_FAILURE_VERTEX) == 0)
    {
        return false;
    }

    NiTerrainMaterialPixelDescriptor* pkInvalidPixelDesc =
        reinterpret_cast<NiTerrainMaterialPixelDescriptor*>(
        pkRenderPasses[uiFailedPass].m_pkPixelDesc);

    NiTerrainMaterialVertexDescriptor* pkInvalidVertexDesc =
        reinterpret_cast<NiTerrainMaterialVertexDescriptor*>(
        pkRenderPasses[uiFailedPass].m_pkVertexDesc);

    bool bRemovedMaps = false;

    unsigned int uiNumLayers = pkInvalidPixelDesc->GetNUM_LAYERS();
    for (unsigned int uiCurrentLayer = 0; uiCurrentLayer < uiNumLayers;
        ++uiCurrentLayer)
    {
        if (pkInvalidPixelDesc->SupportsNormalMap(uiCurrentLayer) ||
            pkInvalidPixelDesc->SupportsParallaxMap(uiCurrentLayer))
        {
            pkInvalidPixelDesc->SetNormalMapEnabled(uiCurrentLayer, false);
            pkInvalidPixelDesc->SetParallaxMapEnabled(uiCurrentLayer, false);

            bRemovedMaps = true;
        }
    }

    if (bRemovedMaps)
    {
        // If we have removed all the normal/parallax maps, we no longer
        // need world view or tangents
        pkInvalidPixelDesc->SetINPUT_WORLDVIEW(false);
        pkInvalidVertexDesc->SetOUTPUT_WORLDVIEW(false);

        pkInvalidPixelDesc->SetINPUT_WORLDNBT(false);
        pkInvalidVertexDesc->SetINPUT_TANGENTS(false);
    }

    return bRemovedMaps;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiTerrainMaterial::GetMaterialDescriptorSize()
{
    return MATERIAL_DESCRIPTOR_INDEX_COUNT;
}

//---------------------------------------------------------------------------
unsigned int NiTerrainMaterial::GetVertexProgramDescriptorSize()
{
    return VERTEX_PROGRAM_DESCRIPTOR_INDEX_COUNT;
}

//---------------------------------------------------------------------------
unsigned int NiTerrainMaterial::GetGeometryProgramDescriptorSize()
{
    return 0;
}

//---------------------------------------------------------------------------
unsigned int NiTerrainMaterial::GetPixelProgramDescriptorSize()
{
    return PIXEL_PROGRAM_DESCRIPTOR_INDEX_COUNT;
}

//--------------------------------------------------------------------------------------------------
void NiTerrainMaterial::AddReplacementShaders()
{
    // Create a terrain material descriptor
    NiTerrainMaterialDescriptor* pkTerrainMatDesc = NiNew NiTerrainMaterialDescriptor();
    pkTerrainMatDesc->m_kIdentifier = "NiTerrainMaterialDescriptor";

    // Configure default lighting config
    NiFragmentLighting::Descriptor kLightingDesc;
    kLightingDesc.uiNumDirectionalLights = 1;
    NiFragmentLighting::SetDescriptor(pkTerrainMatDesc, kLightingDesc);

    // Create the shader and insert it into the shader cache.
    PrecacheGPUPrograms(pkTerrainMatDesc);
}

//--------------------------------------------------------------------------------------------------
bool NiTerrainMaterial::UseTerrainReplacementShader(
    NiMaterialDescriptor* /*pkMaterialDescriptor*/,
    ReturnCode eFailedRC,
    unsigned int uiFailedPass,
    RenderPassDescriptor* pkRenderPasses,
    unsigned int uiMaxCount,
    unsigned int& uiCount,
    unsigned int& uiFunctionData)
{
    EE_UNUSED_ARG(uiCount);
    EE_UNUSED_ARG(uiMaxCount);
    EE_UNUSED_ARG(eFailedRC);
    
    // Only use the replacement shaders when shader cache is locked. Since the shader cache
    // will ideally be unlocked durring most of an application's development cycle, this
    // prevents the replacement shader being used and 'masking' shader compilation issues.
    if (!NiFragmentMaterial::ms_bLockProgramCache)
        return false;

    // This fall back should only ever be used once.  
    if (uiFunctionData)
        return false;

    uiFunctionData = 1;
    NiOutputDebugString("Using a NiTerrainMaterial replacement shader.\n");

    {
        // Clear out the material descriptors
        NiTerrainMaterialPixelDescriptor* pkInvalidPixelDesc =
            (NiTerrainMaterialPixelDescriptor*)pkRenderPasses[uiFailedPass].m_pkPixelDesc;
        NiTerrainMaterialVertexDescriptor* pkInvalidVertexDesc =
            (NiTerrainMaterialVertexDescriptor*)pkRenderPasses[uiFailedPass].m_pkVertexDesc;

        memset(pkInvalidPixelDesc->m_pkBitArray, 0, 
            pkInvalidPixelDesc->m_uiIntCount * sizeof(int) );
        memset(pkInvalidVertexDesc->m_pkBitArray, 0, 
            pkInvalidVertexDesc->m_uiIntCount * sizeof(int) );
    }

    NiTerrainMaterialDescriptor* pkReplaceMatDesc = NiNew NiTerrainMaterialDescriptor();
    pkReplaceMatDesc->m_kIdentifier = "NiTerrainMaterialDescriptor";

    NiFragmentLighting::Descriptor kLightingDesc;
    kLightingDesc.uiNumDirectionalLights = 1;
    NiFragmentLighting::SetDescriptor(pkReplaceMatDesc, kLightingDesc);

    NiTerrainMaterial* pkMaterial = NiDynamicCast(NiTerrainMaterial,
        NiMaterial::GetMaterial("NiTerrainMaterial"));
    EE_ASSERT(pkMaterial);

    NiUInt32 uiPassCount = 0;
    pkMaterial->GenerateShaderDescArray(pkReplaceMatDesc, pkRenderPasses, 1, uiPassCount);

    NiDelete pkReplaceMatDesc;

    return true;
}

//--------------------------------------------------------------------------------------------------
NiTerrainMaterial::NiTerrainFragmentLighting::NiTerrainFragmentLighting():
    NiFragmentLighting()
{
    // Append the required node libraries
    m_kLibraries.Add(NiTerrainMaterialNodeLibrary::CreateMaterialNodeLibrary());
}

//--------------------------------------------------------------------------------------------------
NiTerrainMaterial::TerrainLightingData::TerrainLightingData()
{
    m_pkLayerSpecularIntensities = NULL;
    m_pkLayerSpecularPowers = NULL;
    m_pkLayerMaskValues = NULL;
    m_pkLowDetailSpecular = NULL;

    for (efd::UInt32 uiLayer = 0; uiLayer < MAX_LAYERS; ++uiLayer)
    {
        m_abLayerSpecularEnabled[uiLayer] = NULL;
        m_apkLayerSpecular[uiLayer] = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
