#include "NiMainPCH.h"
#include "NiExtendedMaterial.h"
#include "NiExtendedMaterialNodeLibrary.h"
#include <NiMaterialNode.h>
#include <NiMaterialResource.h>
#include <NiMaterialConfigurator.h>
#include <NiShaderAttributeDesc.h>
#include <NiStandardPixelProgramDescriptor.h>
#include <NiStandardMaterialDescriptor.h>
#include <NiStandardMaterialNodeLibrary.h>
#include <NiTexturingProperty.h>
#include <NiFloatsExtraData.h>

NiImplementRTTI(NiExtendedMaterial, NiStandardMaterial, NiTypeMask::NiExtendedMaterial);



NiExtendedMaterial* NiExtendedMaterial::Create()
{
    NiMaterial* pkExisting = NiMaterial::GetMaterial("NiExtendedMaterial");

    if (pkExisting)
    {
        NiExtendedMaterial* pkExtended =
            NiDynamicCast(NiExtendedMaterial, pkExisting);

        if (pkExtended)
            return pkExtended;
    }

    NiExtendedMaterial* pkMaterial = NiNew NiExtendedMaterial(true);

    NiRenderer* pkRenderer = NiRenderer::GetRenderer();
    if (pkRenderer)
    {
        if (!pkMaterial->GetProgramCache(NiGPUProgram::PROGRAM_VERTEX) ||
            !pkMaterial->GetProgramCache(NiGPUProgram::PROGRAM_PIXEL))
        {
            pkRenderer->SetDefaultProgramCache(pkMaterial);
        }
    }

    return pkMaterial;
}

NiExtendedMaterial::NiExtendedMaterial(bool bAutoCreateCaches) : NiStandardMaterial(
        "NiExtendedMaterial",
        NiExtendedMaterialNodeLibrary::CreateMaterialNodeLibrary(),
        EXTENDED_VERTEX_VERSION,
        EXTENDED_GEOMETRY_VERSION,
        EXTENDED_PIXEL_VERSION,
        bAutoCreateCaches),
    m_bTerrainEnabled(false),
    m_uiTerrainLayerCount(0)
{
    for (NiUInt32 i = 0; i < MAX_TERRAIN_LAYERS; ++i)
    {
        m_akTerrainLayerData[i] = NiPoint4(
            16.0f,  // scale U
            16.0f,  // scale V
            1.0f,   // weight
            0.0f);  // invert alpha
    }
}

NiExtendedMaterial::~NiExtendedMaterial()
{
}

void NiExtendedMaterial::SetTerrainEnabled(bool bEnabled)
{
    if (m_bTerrainEnabled == bEnabled)
        return;

    m_bTerrainEnabled = bEnabled;
    UnloadShaders();
}

bool NiExtendedMaterial::GetTerrainEnabled() const
{
    return m_bTerrainEnabled;
}

void NiExtendedMaterial::SetTerrainTextureArray(NiSourceTexture* pkTexture)
{
    m_spTerrainTextureArray = pkTexture;
}

NiSourceTexture* NiExtendedMaterial::GetTerrainTextureArray() const
{
    return m_spTerrainTextureArray;
}

void NiExtendedMaterial::SetTerrainAlphaArray(NiSourceTexture* pkTexture)
{
    m_spTerrainAlphaArray = pkTexture;
}

NiSourceTexture* NiExtendedMaterial::GetTerrainAlphaArray() const
{
    return m_spTerrainAlphaArray;
}

void NiExtendedMaterial::SetTerrainLayerCount(NiUInt32 uiLayerCount)
{
    if (uiLayerCount > MAX_TERRAIN_LAYERS)
        uiLayerCount = MAX_TERRAIN_LAYERS;

    if (m_uiTerrainLayerCount == uiLayerCount)
        return;

    m_uiTerrainLayerCount = uiLayerCount;
    UnloadShaders();
}

NiUInt32 NiExtendedMaterial::GetTerrainLayerCount() const
{
    return m_uiTerrainLayerCount;
}

void NiExtendedMaterial::SetTerrainLayer(
    NiUInt32 uiLayer,
    float fScaleU,
    float fScaleV,
    float fWeight,
    bool bInvertAlpha)
{
    if (uiLayer >= MAX_TERRAIN_LAYERS)
        return;

    m_akTerrainLayerData[uiLayer] = NiPoint4(
        fScaleU,
        fScaleV,
        fWeight,
        bInvertAlpha ? 1.0f : 0.0f);
}

bool NiExtendedMaterial::GenerateDescriptor(
    const NiRenderObject* pkGeometry,
    const NiPropertyState* pkState,
    const NiDynamicEffectState* pkEffects,
    NiMaterialDescriptor& kMaterialDesc)
{
    if (!NiStandardMaterial::GenerateDescriptor(
        pkGeometry,
        pkState,
        pkEffects,
        kMaterialDesc))
    {
        return false;
    }

    NiStandardMaterialDescriptor* pkDesc =
        static_cast<NiStandardMaterialDescriptor*>(&kMaterialDesc);

    if (m_bTerrainEnabled)
    {
        pkDesc->SetUSERDEFINED00(1);
        pkDesc->SetBASEMAPCOUNT(0);
        pkDesc->SetDETAILMAPCOUNT(0);
        pkDesc->SetDARKMAPCOUNT(0);
        pkDesc->SetGLOWMAPCOUNT(0);
        pkDesc->SetAPPLYMODE(NiStandardMaterial::APPLY_MODULATE);
    }
    else
    {
        pkDesc->SetUSERDEFINED00(0);
    }

    return true;
}

bool NiExtendedMaterial::HandlePreLightTextureApplication(
    Context& kContext,
    NiStandardPixelProgramDescriptor* pkPixelDesc,
    NiMaterialResource*& pkWorldPos,
    NiMaterialResource*& pkWorldNormal,
    NiMaterialResource*& pkWorldBinormal,
    NiMaterialResource*& pkWorldTangent,
    NiMaterialResource*& pkWorldViewVector,
    NiMaterialResource*& pkTangentViewVector,
    NiMaterialResource*& pkMatDiffuseColor,
    NiMaterialResource*& pkMatSpecularColor,
    NiMaterialResource*& pkMatSpecularPower,
    NiMaterialResource*& pkMatGlossiness,
    NiMaterialResource*& pkMatAmbientColor,
    NiMaterialResource*& pkMatEmissiveColor,
    NiMaterialResource*& pkOpacityAccum,
    NiMaterialResource*& pkAmbientLightAccum,
    NiMaterialResource*& pkDiffuseLightAccum,
    NiMaterialResource*& pkSpecularLightAccum,
    NiMaterialResource*& pkDiffuseTexAccum,
    NiMaterialResource*& pkSpecularTexAccum,
    unsigned int& uiTexturesApplied,
    NiMaterialResource** apkUVSets,
    unsigned int uiNumStandardUVs,
    unsigned int uiNumTexEffectUVs)
{
    NiOutputDebugString("NiExtendedMaterial::HandlePreLightTextureApplication entered\n");
    if (!NiStandardMaterial::HandlePreLightTextureApplication(
        kContext,
        pkPixelDesc,
        pkWorldPos,
        pkWorldNormal,
        pkWorldBinormal,
        pkWorldTangent,
        pkWorldViewVector,
        pkTangentViewVector,
        pkMatDiffuseColor,
        pkMatSpecularColor,
        pkMatSpecularPower,
        pkMatGlossiness,
        pkMatAmbientColor,
        pkMatEmissiveColor,
        pkOpacityAccum,
        pkAmbientLightAccum,
        pkDiffuseLightAccum,
        pkSpecularLightAccum,
        pkDiffuseTexAccum,
        pkSpecularTexAccum,
        uiTexturesApplied,
        apkUVSets,
        uiNumStandardUVs,
        uiNumTexEffectUVs))
    {
        return false;
    }

    if (!m_bTerrainEnabled) {
        NiOutputDebugString("NiExtendedMaterial: terrain disabled\n");
        return true;
    }

    if (!apkUVSets || uiNumStandardUVs == 0 || !apkUVSets[0])
    {
        NiOutputDebugString("NiExtendedMaterial: missing UVSet0\n");
        return false;
    }

    NiMaterialNode* pkTerrainNode =
        GetAttachableNodeFromLibrary("TerrainSplatTextureArray");

    if (!pkTerrainNode)
    {
        NiOutputDebugString("NiExtendedMaterial: TerrainSplatTextureArray node not found\n");
        return false;
    }

    NiOutputDebugString("NiExtendedMaterial: TerrainSplatTextureArray node found\n");
    kContext.m_spConfigurator->AddNode(pkTerrainNode);

    NiMaterialResource* pkUV = apkUVSets[0];

    NiMaterialResource* pkDiffuseArray =
        AddTerrainTextureArrayResource(kContext, "DiffuseArray", 0);

    NiMaterialResource* pkAlphaArray =
        AddTerrainTextureArrayResource(kContext, "AlphaArray", 1);

    NiMaterialResource* pkDiffuseSampler =
        AddTerrainSamplerStateResource(kContext, "DiffuseArraySampler");

    NiMaterialResource* pkAlphaSampler =
        AddTerrainSamplerStateResource(kContext, "AlphaArraySampler");

    NiMaterialResource* pkTerrainInfo = AddOutputAttribute(
        kContext.m_spUniforms,
        "TerrainInfo",
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4);

    NiMaterialResource* pkLayerData = AddOutputAttribute(
        kContext.m_spUniforms,
        "TerrainLayerData",
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4,
        MAX_TERRAIN_LAYERS);

    if (!pkDiffuseArray || !pkAlphaArray ||
        !pkDiffuseSampler || !pkAlphaSampler ||
        !pkTerrainInfo || !pkLayerData)
    {
        return false;
    }

    bool bSuccess = true;

    bSuccess &= kContext.m_spConfigurator->AddBinding(
        pkUV,
        pkTerrainNode->GetInputResourceByVariableName("UV"));

    bSuccess &= kContext.m_spConfigurator->AddBinding(
        pkDiffuseArray,
        pkTerrainNode->GetInputResourceByVariableName("DiffuseArray"));

    bSuccess &= kContext.m_spConfigurator->AddBinding(
        pkDiffuseSampler,
        pkTerrainNode->GetInputResourceByVariableName("DiffuseArraySampler"));

    bSuccess &= kContext.m_spConfigurator->AddBinding(
        pkAlphaArray,
        pkTerrainNode->GetInputResourceByVariableName("AlphaArray"));

    bSuccess &= kContext.m_spConfigurator->AddBinding(
        pkAlphaSampler,
        pkTerrainNode->GetInputResourceByVariableName("AlphaArraySampler"));

    bSuccess &= kContext.m_spConfigurator->AddBinding(
        pkTerrainInfo,
        pkTerrainNode->GetInputResourceByVariableName("TerrainInfo"));

    bSuccess &= kContext.m_spConfigurator->AddBinding(
        pkLayerData,
        pkTerrainNode->GetInputResourceByVariableName("LayerData"));

    if (!bSuccess)
        return false;

    NiMaterialResource* pkTerrainColor =
        pkTerrainNode->GetOutputResourceByVariableName("ColorOut");

    if (!pkTerrainColor)
        return false;

    pkDiffuseTexAccum = pkTerrainColor;
    uiTexturesApplied += 2;

    NiOutputDebugString("NiExtendedMaterial: terrain diffuse override applied\n");
    return true;
}

void NiExtendedMaterial::AddReplacementShaders()
{
	// No replacement shaders are added for this material.
}
