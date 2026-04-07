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

// Precompiled Header
#include "NiD3D10RendererPCH.h"
#pragma message("post pch")

#include "NiD3D10ErrorShader.h"
#pragma message("posta pch")
#include "NiD3D10PixelShader.h"
#pragma message("postb pch")
#include "NiD3D10Renderer.h"
#pragma message("postc pch")
#include "NiD3D10ShaderProgramFactory.h"
#pragma message("postd pch")
#include "NiD3D10VertexShader.h"
#pragma message("poste pch")
#include "NiD3D10MeshMaterialBinding.h"

NiImplementRTTI(NiD3D10ErrorShader, NiD3D10Shader);
#pragma message("postf pch")

//--------------------------------------------------------------------------------------------------
NiD3D10ErrorShader::NiD3D10ErrorShader()
{
    // Create the local stages and passes we will use...
    CreateStagesAndPasses();

    SetName("NiD3D10ErrorShader");

    // This is the best (and only) implementation of this shader
    SetIsBestImplementation(TRUE);
}

//--------------------------------------------------------------------------------------------------
NiD3D10ErrorShader::~NiD3D10ErrorShader()
{
    m_kPasses.RemoveAll();
}

//--------------------------------------------------------------------------------------------------
bool NiD3D10ErrorShader::Initialize()
{
    if (NiD3D10Shader::Initialize())
    {
        CreateShaderDeclaration();

        CreateShaders();

        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiD3D10ErrorShader::SetupTransformations(
    const NiRenderCallContext& kRCC)
{
    // Create a new, bright random color
    m_kMaterialColor.x = NiUnitRandom() * 0.5f + 0.5f;
    m_kMaterialColor.y = NiUnitRandom() * 0.5f + 0.5f;
    m_kMaterialColor.z = NiUnitRandom() * 0.5f + 0.5f;


    NiTransform kNewWorld = *kRCC.m_pkWorld;

    float fBound = kRCC.m_pkWorldBound->GetRadius();
    float fRange = 0.1f * fBound;
    kNewWorld.m_Translate.x += NiSymmetricRandom() * fRange;
    kNewWorld.m_Translate.y += NiSymmetricRandom() * fRange;
    kNewWorld.m_Translate.z += NiSymmetricRandom() * fRange;

    return NiD3D10Shader::SetupTransformations(kRCC);
}

//--------------------------------------------------------------------------------------------------
void NiD3D10ErrorShader::Do_RenderMeshes(NiVisibleArray* pkVisibleArray)
{
    NiD3D10Renderer* pkRenderer = NiD3D10Renderer::GetRenderer();

    NiUInt32 uiCount = pkVisibleArray->GetCount();

    for (NiUInt32 ui = 0; ui < uiCount; ui++)
    {
        NiMesh* pkMesh = (NiMesh*)&pkVisibleArray->GetAt(ui);

        const NiMaterialInstance* pkMatInst = pkMesh->GetActiveMaterialInstance();
        NiD3D10MeshMaterialBindingPtr spDecl = NULL;
        if (!pkMatInst)
        {
            pkRenderer->GetShaderAndVertexDecl(pkMesh, spDecl);
        }
        else
        {
            spDecl = (NiD3D10MeshMaterialBinding*)
                pkMatInst->GetVertexDeclarationCache();
        }

        if (!spDecl)
        {
            // No vertex decl. avaliable, mesh can not be rendered.
            // Remove mesh from the visible array since it can not rendered.
            pkVisibleArray->RemoveAtAndFill(ui);
            ui--;
            uiCount--;

            // Manually complete any outstanding modifiers.
            NiSyncArgs kSyncArgs;
            kSyncArgs.m_uiSubmitPoint = NiSyncArgs::SYNC_ANY;
            kSyncArgs.m_uiCompletePoint = NiSyncArgs::SYNC_RENDER;
            pkMesh->CompleteModifiers(&kSyncArgs);

            NiTimeController::OnPreDisplayIterate(pkMesh->GetControllers());
        }

    }

    NiD3D10Shader::Do_RenderMeshes(pkVisibleArray);
}

//--------------------------------------------------------------------------------------------------
bool NiD3D10ErrorShader::CreateStagesAndPasses()
{
    NiD3D10PassPtr spPass;
    EE_VERIFY(NiD3D10Pass::CreateNewPass(spPass));
    EE_ASSERT(spPass);
    m_pkPass = spPass;

    m_kPasses.SetAtGrow(0, spPass);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiD3D10ErrorShader::CreateShaderDeclaration()
{
    NiShaderDeclarationPtr spShaderDecl = NiShaderDeclaration::Create(1, 1);
    spShaderDecl->SetEntry(0,
        NiShaderDeclaration::SHADERPARAM_NI_POSITION0,
        NiShaderDeclaration::SPTYPE_FLOAT3, 0);

    SetSemanticAdapterTableFromShaderDeclaration(spShaderDecl);

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiD3D10ErrorShader::CreateShaders()
{
    const char acVertexShader[] =
        "row_major float4x4 WorldViewProj : WorldViewProj;\n"
        "float4 MaterialColor;\n"
        "void VSMain(float3 LocalPos : POSITION0,\n"
        "    out float4 ProjPos : SV_POSITION,\n"
        "    out float4 Color : COLOR0)\n"
        "{\n"
        "    ProjPos = mul(float4(LocalPos, 1.0f), WorldViewProj);\n"
        "    Color = MaterialColor;\n"
        "}\n";
    const unsigned int uiVSBufferSize = sizeof(acVertexShader);

    const char acPixelShader[] =
        "void PSMain(float4 ProjPos : SV_POSITION,\n"
        "    float4 InColor : COLOR0,\n"
        "    out float4 OutColor: SV_Target)\n"
        "{\n"
        "    OutColor = InColor;\n"
        "}\n";
    const unsigned int uiPSBufferSize = sizeof(acPixelShader);

    ID3D10Blob* pkVSBlob = NULL;
    HRESULT hr = NiD3D10Renderer::D3D10CreateBlob(uiVSBufferSize, &pkVSBlob);
    EE_ASSERT(SUCCEEDED(hr) && pkVSBlob);
    NiMemcpy(pkVSBlob->GetBufferPointer(), pkVSBlob->GetBufferSize(),
        acVertexShader, uiVSBufferSize);

    NiD3D10VertexShaderPtr spVertexShader;
    NiD3D10ShaderProgramFactory::CreateVertexShaderFromBlob(pkVSBlob, ".vsh",
        NULL, NULL, "VSMain", "vs_4_0", 0, "NiD3D10ErrorShader_VS",
        spVertexShader);
    EE_ASSERT(spVertexShader);
    m_pkPass->SetVertexShader(spVertexShader);
    pkVSBlob->Release();

    ID3D10Blob* pkPSBlob = NULL;
    hr = NiD3D10Renderer::D3D10CreateBlob(uiPSBufferSize, &pkPSBlob);
    EE_ASSERT(SUCCEEDED(hr) && pkVSBlob);
    NiMemcpy(pkPSBlob->GetBufferPointer(), pkPSBlob->GetBufferSize(),
        acPixelShader, uiPSBufferSize);

    NiD3D10PixelShaderPtr spPixelShader;
    NiD3D10ShaderProgramFactory::CreatePixelShaderFromBlob(pkPSBlob, ".psh",
        NULL, NULL, "PSMain", "ps_4_0", 0, "NiD3D10ErrorShader_PS",
        spPixelShader);
    EE_ASSERT(spPixelShader);
    m_pkPass->SetPixelShader(spPixelShader);
    pkPSBlob->Release();

    NiD3D10ShaderConstantMapPtr spVSMap =
        NiNew NiD3D10ShaderConstantMap(NiGPUProgram::PROGRAM_VERTEX);
    m_pkPass->SetVertexConstantMap(0, spVSMap);

    // Projection to clip space
    unsigned int uiDefinedMatrixFlags =
        NiShaderConstantMapEntry::GetAttributeFlags(
        NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4) |
        NiShaderConstantMapEntry::SCME_MAP_DEFINED;
    spVSMap->AddEntry("WorldViewProj", uiDefinedMatrixFlags, 0, 0, 4,
        "WorldViewProj");

    // Set Constants
    unsigned int uiConstantPoint4Flags =
        NiShaderConstantMapEntry::GetAttributeFlags(
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4) |
        NiShaderConstantMapEntry::SCME_MAP_CONSTANT;

    m_kMaterialColor = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
    spVSMap->AddEntry("MaterialColor",
        uiConstantPoint4Flags, 0, 10, 1, "MaterialColor",
        sizeof(m_kMaterialColor), sizeof(float), &m_kMaterialColor, false);

    return true;
}

//--------------------------------------------------------------------------------------------------
