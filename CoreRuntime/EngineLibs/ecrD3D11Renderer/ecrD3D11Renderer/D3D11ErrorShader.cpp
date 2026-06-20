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
#include "ecrD3D11RendererPCH.h"

#include "D3D11ErrorShader.h"
#include "D3D11Pass.h"
#include "D3D11PixelShader.h"
#include "D3D11Renderer.h"
#include "D3D11ShaderProgramFactory.h"
#include "D3D11VertexShader.h"
#include "D3D11MeshMaterialBinding.h"

using namespace ecr;

NiImplementRTTI(D3D11ErrorShader, D3D11ShaderCore, NiTypeMask::D3D11ErrorShader);

//------------------------------------------------------------------------------------------------
D3D11ErrorShader::D3D11ErrorShader()
{
    // Create the local stages and passes we will use...
    CreateStagesAndPasses();

    SetName("D3D11ErrorShader");

    // This is the best (and only) implementation of this shader
    SetIsBestImplementation(true);
}

//------------------------------------------------------------------------------------------------
D3D11ErrorShader::~D3D11ErrorShader()
{
    m_passArray.RemoveAll();
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ErrorShader::Initialize()
{
    CreateShaderDeclaration();

    CreateShaders();

    return D3D11ShaderCore::Initialize();
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ErrorShader::SetupTransformations(
    const NiRenderCallContext& callContext)
{
    // Create a new, bright random color
    m_materialColor.x = efd::UnitRandom() * 0.5f + 0.5f;
    m_materialColor.y = efd::UnitRandom() * 0.5f + 0.5f;
    m_materialColor.z = efd::UnitRandom() * 0.5f + 0.5f;

    NiTransform kNewWorld = *callContext.m_pkWorld;

    efd::Float32 bound = callContext.m_pkWorldBound->GetRadius();
    efd::Float32 range = 0.1f * bound;
    kNewWorld.m_Translate.x += efd::SymmetricRandom() * range;
    kNewWorld.m_Translate.y += efd::SymmetricRandom() * range;
    kNewWorld.m_Translate.z += efd::SymmetricRandom() * range;

    return D3D11ShaderCore::SetupTransformations(callContext);
}

//------------------------------------------------------------------------------------------------
void D3D11ErrorShader::Do_RenderMeshes(NiVisibleArray* pVisibleArray)
{
    D3D11Renderer* pkRenderer = D3D11Renderer::GetRenderer();

    efd::UInt32 arrayCount = pVisibleArray->GetCount();

    for (efd::UInt32 i = 0; i < arrayCount; i++)
    {
        NiMesh* pMesh = (NiMesh*)&pVisibleArray->GetAt(i);

        const NiMaterialInstance* pMatInst = pMesh->GetActiveMaterialInstance();
        D3D11MeshMaterialBindingPtr spDecl;
        if (!pMatInst)
        {
            pkRenderer->GetShaderAndVertexDecl(pMesh, spDecl);
        }
        else
        {
            spDecl = (D3D11MeshMaterialBinding*)pMatInst->GetVertexDeclarationCache();
        }

        if (!spDecl)
        {
            // No vertex decl. avaliable, mesh can not be rendered.
            // Remove mesh from the visible array since it can not rendered.
            pVisibleArray->RemoveAtAndFill(i);
            i--;
            arrayCount--;

            // Manually complete any outstanding modifiers.
            NiSyncArgs syncArgs;
            syncArgs.m_uiSubmitPoint = NiSyncArgs::SYNC_ANY;
            syncArgs.m_uiCompletePoint = NiSyncArgs::SYNC_RENDER;
            pMesh->CompleteModifiers(&syncArgs);

            NiTimeController::OnPreDisplayIterate(pMesh->GetControllers());
        }

    }

    D3D11ShaderCore::Do_RenderMeshes(pVisibleArray);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ErrorShader::CreateStagesAndPasses()
{
    D3D11PassPtr spPass;
    EE_VERIFY(D3D11Pass::CreateNewPass(spPass));
    EE_ASSERT(spPass);
    m_pPass = spPass;

    m_passArray.SetAtGrow(0, spPass);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ErrorShader::CreateShaderDeclaration()
{
    NiShaderDeclarationPtr spShaderDecl = NiShaderDeclaration::Create(1, 1);
    spShaderDecl->SetEntry(0,
        NiShaderDeclaration::SHADERPARAM_NI_POSITION0,
        NiShaderDeclaration::SPTYPE_FLOAT3, 
        0);

    SetSemanticAdapterTableFromShaderDeclaration(spShaderDecl);

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ErrorShader::CreateShaders()
{
    const efd::Char vertexShaderText[] =
        "float4x4 WorldViewProj : WorldViewProj;\n"
        "float4 MaterialColor;\n"
        "void VSMain(float3 LocalPos : POSITION0,\n"
        "    out float4 ProjPos : SV_POSITION,\n"
        "    out float4 Color : COLOR0)\n"
        "{\n"
        "    ProjPos = mul(float4(LocalPos, 1.0f), WorldViewProj);\n"
        "    Color = MaterialColor;\n"
        "}\n";
    const efd::UInt32 vsBufferSize = sizeof(vertexShaderText);

    const efd::Char pixelShaderText[] =
        "void PSMain(float4 ProjPos : SV_POSITION,\n"
        "    float4 InColor : COLOR0,\n"
        "    out float4 OutColor: SV_Target)\n"
        "{\n"
        "    OutColor = InColor;\n"
        "}\n";
    const efd::UInt32 psBufferSize = sizeof(pixelShaderText);

    ID3DBlob* vsBlob = NULL;
    HRESULT hr = D3D11Renderer::D3D10CreateBlob(vsBufferSize, &vsBlob);
    EE_ASSERT(SUCCEEDED(hr) && vsBlob);
    efd::Memcpy(vsBlob->GetBufferPointer(), 
        vsBlob->GetBufferSize(),
        vertexShaderText, 
        vsBufferSize);

    D3D11VertexShaderPtr spVertexShader;
    D3D11ShaderProgramFactory::CreateVertexShaderFromBlob(
        vsBlob, 
        ".vsh",
        NULL, 
        NULL, 
        "VSMain", 
        "vs_4_0_level_9_1", 
        0, 
        "D3D11ErrorShader_VS",
        false,
        spVertexShader);
    EE_ASSERT(spVertexShader);
    m_pPass->SetVertexShader(spVertexShader);
    vsBlob->Release();

    ID3DBlob* psBlob = NULL;
    hr = D3D11Renderer::D3D10CreateBlob(psBufferSize, &psBlob);
    EE_ASSERT(SUCCEEDED(hr) && vsBlob);
    efd::Memcpy(psBlob->GetBufferPointer(), 
        psBlob->GetBufferSize(),
        pixelShaderText, 
        psBufferSize);

    D3D11PixelShaderPtr spPixelShader;
    D3D11ShaderProgramFactory::CreatePixelShaderFromBlob(
        psBlob, 
        ".psh",
        NULL, 
        NULL, 
        "PSMain", 
        "ps_4_0_level_9_1", 
        0, 
        "D3D11ErrorShader_PS",
        false,
        spPixelShader);
    EE_ASSERT(spPixelShader);
    m_pPass->SetPixelShader(spPixelShader);
    psBlob->Release();

    D3D11ShaderConstantMapPtr spVSMap = EE_NEW D3D11ShaderConstantMap(NiGPUProgram::PROGRAM_VERTEX);
    m_pPass->AddConstantMap(spVSMap);

    // Projection to clip space
    efd::UInt32 definedMatrixFlags =
        NiShaderConstantMapEntry::GetAttributeFlags(
        NiShaderAttributeDesc::ATTRIB_TYPE_MATRIX4) |
        NiShaderConstantMapEntry::SCME_MAP_DEFINED;
    spVSMap->AddEntry("WorldViewProj", definedMatrixFlags, 0, 0, 4, "WorldViewProj");

    // Set Constants
    efd::UInt32 constantPoint4Flags =
        NiShaderConstantMapEntry::GetAttributeFlags(
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4) |
        NiShaderConstantMapEntry::SCME_MAP_CONSTANT;

    m_materialColor.x = 1.0f;
    m_materialColor.y = 1.0f;
    m_materialColor.z = 1.0f;
    m_materialColor.w = 1.0f;
    spVSMap->AddEntry("MaterialColor",
        constantPoint4Flags, 
        0, 
        10, 
        1, 
        "MaterialColor",
        sizeof(m_materialColor), 
        sizeof(float), 
        &m_materialColor, false);

    return true;
}

//------------------------------------------------------------------------------------------------
