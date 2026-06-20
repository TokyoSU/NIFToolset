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
#include "NiD3DRendererPCH.h"

#include <NiDX9Renderer.h>
#include "NiDX9ErrorShader.h"

NiImplementRTTI(NiDX9ErrorShader, NiD3DShader, NiTypeMask::NiDX9ErrorShader);

//--------------------------------------------------------------------------------------------------
NiDX9ErrorShader::NiDX9ErrorShader()
{
    //  Create the local stages and passes we will use...
    CreateStagesAndPasses();

    SetName("NiDX9ErrorShader");

    // This is the best (and only) implementation of this shader
    SetIsBestImplementation(true);
}

//--------------------------------------------------------------------------------------------------
NiDX9ErrorShader::~NiDX9ErrorShader()
{
    m_kPasses.RemoveAll();
}

//--------------------------------------------------------------------------------------------------
bool NiDX9ErrorShader::Initialize()
{
    if (m_bInitialized)
        return true;

    if (NiD3DShader::Initialize())
    {
        CreateShaderDeclaration();
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiDX9ErrorShader::UpdatePipeline(const NiRenderCallContext& kRCC)
{
    // Create a new, bright random color
    unsigned char ucR = (unsigned char)(NiUnitRandom() * 127) + 128;
    unsigned char ucG = (unsigned char)(NiUnitRandom() * 127) + 128;
    unsigned char ucB = (unsigned char)(NiUnitRandom() * 127) + 128;

    D3DCOLOR kColor = D3DCOLOR_ARGB(255, ucR, ucG, ucB);

    m_pkPass->SetRenderState(D3DRS_TEXTUREFACTOR, kColor, true);

    m_pkPass->SetRenderState(D3DRS_SPECULARENABLE, false, true);
    m_pkPass->SetRenderState(D3DRS_ALPHABLENDENABLE, false, true);
    m_pkPass->SetRenderState(D3DRS_ALPHATESTENABLE, false, true);
    m_pkPass->SetRenderState(D3DRS_STENCILENABLE, false, true);

    return NiD3DShader::UpdatePipeline(kRCC);
}

//--------------------------------------------------------------------------------------------------
unsigned int NiDX9ErrorShader::SetupTransformations(
    const NiRenderCallContext& kRCC)
{
    // Add random jitter to world transform
    EE_ASSERT(kRCC.m_pkWorld);
    NiTransform kNewWorld = *kRCC.m_pkWorld;

    EE_ASSERT(kRCC.m_pkWorldBound);
    float fBound = kRCC.m_pkWorldBound->GetRadius();
    float fRange = 0.1f * fBound;
    kNewWorld.m_Translate.x += NiSymmetricRandom() * fRange;
    kNewWorld.m_Translate.y += NiSymmetricRandom() * fRange;
    kNewWorld.m_Translate.z += NiSymmetricRandom() * fRange;

    NiRenderCallContext kNewRCC(kRCC);
    kNewRCC.m_pkWorld = &kNewWorld;
    return NiD3DShader::SetupTransformations(kNewRCC);
}

//--------------------------------------------------------------------------------------------------
void NiDX9ErrorShader::Do_RenderMeshes(NiVisibleArray* pkVisibleArray)
{
    NiDX9Renderer* pkRenderer = NiDX9Renderer::GetRenderer();

    NiUInt32 uiCount = pkVisibleArray->GetCount();

    for (NiUInt32 ui = 0; ui < uiCount; ui++)
    {
        NiMesh* pkMesh = (NiMesh*)&pkVisibleArray->GetAt(ui);

        const NiMaterialInstance* pkMatInst = pkMesh->GetActiveMaterialInstance();
        NiDX9MeshMaterialBindingPtr spDecl = NULL;
        if (!pkMatInst)
        {
            pkRenderer->GetShaderAndVertexDecl(pkMesh, spDecl);
        }
        else
        {
            spDecl = (NiDX9MeshMaterialBinding*)
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

    NiD3DShader::Do_RenderMeshes(pkVisibleArray);
}

//--------------------------------------------------------------------------------------------------
bool NiDX9ErrorShader::CreateStagesAndPasses()
{
    NiD3DPassPtr spPass = NiD3DPass::CreateNewPass();
    EE_ASSERT(spPass);
    m_pkPass = spPass;

    NiD3DTextureStagePtr spStage = NiD3DTextureStage::CreateNewStage();
    EE_ASSERT(spStage);
    NiD3DTextureStage* pkStage = spStage;

    pkStage->SetStage(0);
    pkStage->SetTexture(0);
    pkStage->SetStageState(D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    pkStage->SetStageState(D3DTSS_COLORARG1, D3DTA_TFACTOR);

    pkStage->SetStageState(D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pkStage->SetStageState(D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
    pkStage->SetStageState(D3DTSS_TEXCOORDINDEX, 0);

    pkStage->SetSamplerState(NiD3DRenderState::NISAMP_ADDRESSU,
        D3DTADDRESS_CLAMP);
    pkStage->SetSamplerState(NiD3DRenderState::NISAMP_ADDRESSV,
        D3DTADDRESS_CLAMP);

    pkStage->SetSamplerState(NiD3DRenderState::NISAMP_MAGFILTER,
        D3DTEXF_LINEAR);
    pkStage->SetSamplerState(NiD3DRenderState::NISAMP_MINFILTER,
        D3DTEXF_LINEAR);
    pkStage->SetSamplerState(NiD3DRenderState::NISAMP_MIPFILTER,
        D3DTEXF_NONE);

    pkStage->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

    spPass->AppendStage(pkStage);
    m_kPasses.SetAt(0, spPass);
    m_uiPassCount = 1;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiDX9ErrorShader::CreateShaderDeclaration()
{
    NiShaderDeclarationPtr spShaderDecl = NiShaderDeclaration::Create(1, 1);
    spShaderDecl->SetEntry(0,
        NiShaderDeclaration::SHADERPARAM_NI_POSITION0,
        NiShaderDeclaration::SPTYPE_FLOAT3, 0);

    SetSemanticAdapterTableFromShaderDeclaration(spShaderDecl);

    return true;
}

//--------------------------------------------------------------------------------------------------
