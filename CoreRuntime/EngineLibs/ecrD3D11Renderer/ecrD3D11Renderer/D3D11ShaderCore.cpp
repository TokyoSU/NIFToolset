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

#include "D3D11ShaderCore.h"
#include "D3D11DataStream.h"
#include "D3D11Error.h"
#include "D3D11ComputeShader.h"
#include "D3D11DomainShader.h"
#include "D3D11GeometryShader.h"
#include "D3D11HullShader.h"
#include "D3D11MeshMaterialBinding.h"
#include "D3D11Pass.h"
#include "D3D11PixelFormat.h"
#include "D3D11PixelShader.h"
#include "D3D11Renderer.h"
#include "D3D11RenderStateManager.h"
#include "D3D11RenderStateGroup.h"
#include "D3D11ShaderConstantManager.h"
#include "D3D11ShaderConstantMap.h"
#include "D3D11VertexShader.h"

#include <NiLight.h>
#include <NiPSSMConfiguration.h>
#include <NiPSSMShadowClickGenerator.h>
#include <NiShaderAttributeDesc.h>
#include <NiShaderConstantMap.h>
#include <NiShaderError.h>
#include <NiShadowGenerator.h>
#include <NiShadowMap.h>
#include <NiSCMExtraData.h>
#include <NiTextureEffect.h>
#include <NiTextureStage.h>

using namespace ecr;

NiImplementRTTI(D3D11ShaderCore, D3D11ShaderInterface, NiTypeMask::D3D11ShaderCore);

//------------------------------------------------------------------------------------------------
void D3D11ShaderCore::_SDMInit()
{
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderCore::_SDMShutdown()
{
}

//------------------------------------------------------------------------------------------------
D3D11ShaderCore::D3D11ShaderCore() :
    m_currentPassID(0),
    m_pUAVSlots(NULL),
    m_pCurrentPass(NULL),
    m_resetIndexUAVSlots(0),
    m_boneMatrixRegisters(4),
    m_transposeBones(false),
    m_worldSpaceBones(false),
    m_usesGBRenderState(false)
{
    /* */
}

//------------------------------------------------------------------------------------------------
D3D11ShaderCore::~D3D11ShaderCore()
{
    EE_DELETE m_pUAVSlots;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderCore::IsInitialized()
{
    return m_bInitialized;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderCore::Initialize()
{
    const efd::UInt32 passCount = m_passArray.GetSize();
    for (efd::UInt32 i = 0; i < passCount; i++)
    {
        D3D11Pass* pPass = m_passArray.GetAt(i);
        if (pPass)
        {
            // Failure cases should produce an error in ExposeShaderParameters
            if (pPass->ExposeShaderParameters(this) == false)
                return false;
        }
    }
    return D3D11ShaderInterface::Initialize();
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderCore::InvokeShader()
{
    Do_InvokeShader();
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderCore::Do_RenderMeshes(NiVisibleArray* pVisibleArray)
{
    if (pVisibleArray->GetCount() == 0)
        return;

    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    ID3D11DeviceContext* pD3D11DeviceContext = pRenderer->GetCurrentD3D11DeviceContext();

    D3D11RenderStateManager* pRenderStateManager = pRenderer->GetRenderStateManager();

    D3D11ShaderConstantManager* pShaderConstantManager = pRenderer->GetShaderConstantManager();

    efd::UInt32 returnVal = 0;
    NiRenderCallContext callContext;
    NiTransform worldTransform;
    NiBound worldBound;
    NiMesh* pMesh = NULL;

    callContext.m_uiPass = 0;
    callContext.m_uiActivePhases =
        NiRenderer::PHASE_PER_SHADER |
        NiRenderer::PHASE_PER_LIGHTSTATE |
        NiRenderer::PHASE_PER_MESH;

    efd::Bool statesCommitted = false;
    const NiDynamicEffectState* pPreviousEffectState = NULL;

    for (efd::UInt32 renderObjectIndex = 0;
        renderObjectIndex < pVisibleArray->GetCount();
        renderObjectIndex++)
    {
        pMesh = NiVerifyStaticCast(NiMesh, &pVisibleArray->GetAt(renderObjectIndex));

        // Enable the PHASE_PER_LIGHTSTATE phase if the dynamic effect
        // state has changed and the previous states have been
        // committed to the shader constants.
        if (pPreviousEffectState != pMesh->GetEffectState())
        {
            callContext.m_uiActivePhases |= NiRenderer::PHASE_PER_LIGHTSTATE;
            statesCommitted = false;
        }

        NiSyncArgs syncArgs;
        syncArgs.m_uiSubmitPoint = NiSyncArgs::SYNC_ANY;
        syncArgs.m_uiCompletePoint = NiSyncArgs::SYNC_RENDER;
        pMesh->CompleteModifiers(&syncArgs);

        NiTimeController::OnPreDisplayIterate(pMesh->GetControllers());

        pRenderer->SetPropertyState(pMesh->GetPropertyState());
        pRenderer->SetEffectState(pMesh->GetEffectState());

        const NiMaterialInstance* pMatInst = pMesh->GetActiveMaterialInstance();
        D3D11MeshMaterialBindingPtr spMMB;
        if (!pMatInst)
        {
            pRenderer->GetShaderAndVertexDecl(pMesh, spMMB);
            if (!spMMB)
            {
                // GetShaderAndVertexDecl failed - skipping render
                D3D11Error::ReportWarning(
                    "GetShaderAndVertexDecl failed in "
                    __FUNCTION__
                    " on shader '%s', implementation %d on mesh '%s, pointer: 0x%08X; "
                    "skipping render", 
                    GetName(),
                    m_uiImplementation,
                    pMesh->GetName(),
                    pMesh);
                continue;
            }
        }
        else
        {
            spMMB = (D3D11MeshMaterialBinding*)pMatInst->GetVertexDeclarationCache();
        }

        worldTransform = pMesh->GetWorldTransform();
        worldBound = pMesh->GetWorldBound();

        callContext.m_pkMesh = pMesh;
        callContext.m_pkMeshMaterialBinding = (NiVertexDeclarationCache*)spMMB.data();
        callContext.m_pkWorldBound = &worldBound;
        callContext.m_pkEffects = pRenderer->GetEffectState();
        callContext.m_pkState = pRenderer->GetPropertyState();
        callContext.m_pkWorld = &worldTransform;
        callContext.m_uiPass = 0;

        // Preprocess the pipeline
        returnVal = PreProcessPipeline(callContext);
        if (returnVal != 0)
        {
            D3D11Error::ReportWarning(
                "D3D11ShaderCore::PreProcessPipeline failed with return value %d in "
                __FUNCTION__
                " on shader '%s', implementation %d on mesh '%s, pointer: 0x%08X; "
                "skipping render.",
                returnVal,
                GetName(),
                m_uiImplementation,
                pMesh->GetName(),
                pMesh);
            return;
        }

        // Update the pipeline
        returnVal = UpdatePipeline(callContext);
        if (returnVal != 0)
        {
            D3D11Error::ReportWarning(
                "D3D11ShaderCore::UpdatePipeline failed with return value %d in "
                __FUNCTION__
                " on shader '%s', implementation %d on mesh '%s, pointer: 0x%08X; "
                "skipping render.",
                returnVal,
                GetName(),
                m_uiImplementation,
                pMesh->GetName(),
                pMesh);
            return;
        }


        // Get the index buffer, if there is one
        efd::Bool drawAuto = pMesh->GetInputDataIsFromStreamOut();
        efd::Bool isIndexed;

        const NiDataStreamRef* pStreamRef = pMesh->GetFirstStreamRef(
            NiDataStream::USAGE_VERTEX_INDEX, 
            NiDataStream::ACCESS_GPU_READ);

        if (drawAuto)
        {
            isIndexed = false;
        }
        else if (pStreamRef)
        {
            isIndexed = true;
        }
        else
        {
            // No index buffer
            isIndexed = false;
            pStreamRef = pMesh->GetFirstUsageVertexPerVertexStreamRef();

            if (pStreamRef == NULL)
            {
                D3D11Error::ReportWarning(
                    "No vertex streams found to render with in "
                    __FUNCTION__
                    " on shader '%s', implementation %d on mesh '%s, pointer: 0x%08X; "
                    "skipping render.",
                    returnVal,
                    GetName(),
                    m_uiImplementation,
                    pMesh->GetName(),
                    pMesh);
                return;
            }
        }

        // Query for instancing
        efd::UInt32 numInstances = 
            (pMesh->GetInstanced() ? NiInstancingUtilities::GetVisibleInstanceCount(pMesh) : 1);

        // Get active submesh count
        efd::UInt32 submeshCount = 0;
        if (pMesh->GetInstanced())
            submeshCount = NiInstancingUtilities::GetVisibleSubmeshCount(pMesh);
        else
            submeshCount = pMesh->GetSubmeshCount();

        // Iterate over passes
        efd::UInt32 remainingPasses = FirstPass();
        efd::UInt32 totalPasses = remainingPasses;
        while (remainingPasses != 0)
        {
            // Setup the rendering pass
            returnVal = SetupRenderingPass(callContext);

            if (returnVal != 0)
            {
                D3D11Error::ReportWarning(
                    "D3D11ShaderCore::SetupRenderingPass failed with return value %d in "
                    __FUNCTION__
                    " on pass %d of shader '%s', implementation %d on mesh '%s', pointer: 0x%08X; "
                    "skipping render.",
                    returnVal,
                    callContext.m_uiPass,
                    GetName(),
                    m_uiImplementation,
                    pMesh->GetName(),
                    pMesh);
                continue;
            }

            if (drawAuto && submeshCount > 1)
            {
                D3D11Error::ReportWarning(
                    "DrawAuto not implemented with submeshes in "
                    __FUNCTION__
                    " on pass %d of shader '%s', implementation %d on mesh '%s', pointer: 0x%08X; "
                    "skipping pass.",
                    returnVal,
                    callContext.m_uiPass,
                    GetName(),
                    m_uiImplementation,
                    pMesh->GetName(),
                    pMesh);
                continue;
            }

            // Bind shader programs once per pass.
            // These do not depend on the submesh.
            EE_ASSERT(m_pCurrentPass);
            returnVal = m_pCurrentPass->ApplyShaderPrograms(callContext);
            if (returnVal != 0)
            {
                D3D11Error::ReportWarning(
                    "D3D11Pass::ApplyShaderPrograms failed with return value %d in "
                    __FUNCTION__
                    " on pass %d of shader '%s', implementation %d on mesh '%s', pointer: 0x%08X; "
                    "skipping pass.",
                    returnVal,
                    callContext.m_uiPass,
                    GetName(),
                    m_uiImplementation,
                    pMesh->GetName(),
                    pMesh);

                remainingPasses = NextPass();
                callContext.m_uiPass++;
                continue;
            }

            // Iterate over submesh regions
            for (efd::UInt32 submesh = 0; submesh < submeshCount; ++submesh)
            {
                callContext.m_uiSubmesh = submesh;

                efd::UInt32 elementCount = 0;
                efd::UInt32 primitiveCount = 0;
                if (!drawAuto)
                {
                    elementCount = pStreamRef->GetCount(submesh);
                    primitiveCount =
                        pMesh->GetPrimitiveCountFromElementCount(elementCount);
                }

                if (primitiveCount == 0 && !drawAuto)
                {
                    // DT33195
                    // A zero primitive submesh is not an error, but it is inefficient if all
                    // submeshes have 0 primitives. We should determine how to detect this 
                    // earlier and log appropriately.
                    continue;
                }

                // Set the transformations
                returnVal = SetupTransformations(callContext);
                if (returnVal != 0)
                {
                    D3D11Error::ReportWarning(
                        "D3D11ShaderCore::SetupTransformations failed with return value %d in "
                        __FUNCTION__
                        " on pass %d of shader '%s', implementation %d on mesh '%s', "
                        "pointer: 0x%08X, submesh %d; skipping pass.",
                        returnVal,
                        callContext.m_uiPass,
                        GetName(),
                        m_uiImplementation,
                        pMesh->GetName(),
                        pMesh,
                        submesh);
                    continue;
                }

                // Update shader constants per submesh.
                // Do NOT rebind shader programs here; they were already bound once per pass.
                ResetSCMExtraData(callContext.m_pkMesh);

                EE_ASSERT(m_pCurrentPass);
                returnVal = m_pCurrentPass->ApplyShaderConstants(callContext);
                if (returnVal != 0)
                {
                    D3D11Error::ReportWarning(
                        "D3D11Pass::ApplyShaderConstants failed with return value %d in "
                        __FUNCTION__
                        " on pass %d of shader '%s', implementation %d on mesh '%s', "
                        "pointer: 0x%08X, submesh %d; skipping submesh.",
                        returnVal,
                        callContext.m_uiPass,
                        GetName(),
                        m_uiImplementation,
                        pMesh->GetName(),
                        pMesh,
                        submesh);
                    continue;
                }

                // Setup the current submesh
                returnVal = PreRenderSubmesh(callContext);
                if (returnVal != 0)
                {
                    D3D11Error::ReportWarning(
                        "D3D11ShaderCore::PreRenderSubmesh failed with return value %d in "
                        __FUNCTION__
                        " on pass %d of shader '%s', implementation %d on mesh '%s', "
                        "pointer: 0x%08X, submesh %d; skipping pass.",
                        returnVal,
                        callContext.m_uiPass,
                        GetName(),
                        m_uiImplementation,
                        pMesh->GetName(),
                        pMesh,
                        submesh);
                    continue;
                }

                // Apply accumulated state, but only for first submesh
                if (submesh == 0)
                {
                    pRenderStateManager->ApplyCurrentState(GetStateBlockMask(callContext));
                }

                // Apply constants
                pShaderConstantManager->ApplyShaderConstants();

                // The current states are now committed to
                // shader constants.
                statesCommitted = true;

                // When drawing, don't use index offsets - byte offsets are
                // specified when setting the streams in
                // NiShader::PreRenderSubmesh

                if (drawAuto)
                {
                    if (numInstances > 1)
                    {
                        D3D11Error::ReportWarning(
                            "DrawAuto not implemented with instances in "
                            __FUNCTION__
                            " on pass %d of shader '%s', implementation %d on mesh '%s', "
                            "pointer: 0x%08X, submesh %d; skipping pass.",
                            returnVal,
                            callContext.m_uiPass,
                            GetName(),
                            m_uiImplementation,
                            pMesh->GetName(),
                            pMesh,
                            submesh);
                    }
                    else if (isIndexed)
                    {
                        D3D11Error::ReportWarning(
                            "DrawAuto does not support indexed draw in "
                            __FUNCTION__
                            " on pass %d of shader '%s', implementation %d on mesh '%s', "
                            "pointer: 0x%08X, submesh %d; skipping pass.",
                            returnVal,
                            callContext.m_uiPass,
                            GetName(),
                            m_uiImplementation,
                            pMesh->GetName(),
                            pMesh,
                            submesh);
                    }
                    else
                    {
                        pD3D11DeviceContext->DrawAuto();
                    }
                }
                else if (isIndexed)
                {
                    if (numInstances > 1)
                    {
                        pD3D11DeviceContext->DrawIndexedInstanced(
                            elementCount,
                            numInstances,
                            0,  // StartIndexLocation
                            0,  // BaseVertexLocation
                            0); // StartInstanceLocation
                    }
                    else
                    {
                        pD3D11DeviceContext->DrawIndexed(
                            elementCount,
                            0,  // StartIndexLocation
                            0); // BaseVertexLocation
                    }
                }
                else
                {
                    if (numInstances > 1)
                    {
                        pD3D11DeviceContext->DrawInstanced(
                            elementCount,
                            numInstances,
                            0,  // StartVertexLocation
                            0); // StartInstanceLocation
                    }
                    else
                    {
                        pD3D11DeviceContext->Draw(
                            elementCount,
                            0); // StartVertexLocation
                    }
                }

                returnVal = PostRenderSubmesh(callContext);
                if (returnVal != 0)
                {
                    D3D11Error::ReportWarning(
                        "D3D11ShaderCore::PostRenderSubmesh failed with return value %d in "
                        __FUNCTION__
                        " on pass %d of shader '%s', implementation %d on mesh '%s', "
                        "pointer: 0x%08X, submesh %d.",
                        returnVal,
                        callContext.m_uiPass,
                        GetName(),
                        m_uiImplementation,
                        pMesh->GetName(),
                        pMesh,
                        submesh);
                    continue;
                }
            }

            // Perform any post-rendering steps
            returnVal = PostRender(callContext);
            if (returnVal != 0)
            {
                D3D11Error::ReportWarning(
                    "D3D11ShaderCore::PostRender failed with return value %d in "
                    __FUNCTION__
                    " on pass %d of shader '%s', implementation %d on mesh '%s', "
                    "pointer: 0x%08X.",
                    returnVal,
                    callContext.m_uiPass,
                    GetName(),
                    m_uiImplementation,
                    pMesh->GetName(),
                    pMesh);
                continue;
            }

            // Inform the shader to move to the next pass
            remainingPasses = NextPass();
            callContext.m_uiPass++;
        }
        // PostProcess the pipeline
        returnVal = PostProcessPipeline(callContext);
        if (returnVal != 0)
        {
            D3D11Error::ReportWarning(
                "D3D11ShaderCore::PostProcessPipeline failed with return value %d in "
                __FUNCTION__
                " on shader '%s', implementation %d on mesh '%s, pointer: 0x%08X; "
                "skipping render.",
                returnVal,
                GetName(),
                m_uiImplementation,
                pMesh->GetName(),
                pMesh);
            return;
        }

        // Only once state changes have been committed to
        // the shader constants can we change phase.
        if (statesCommitted)
        {
            // If we have single pass objects we can do per
            // mesh phase rendering. If we have multi pass objects
            // we must do per lightstate, per shader and per mesh rendering
            // since every pass after the first pass may overwrite
            // the shader constants set in the previous passes.
            if (totalPasses > 1)
            {
                callContext.m_uiActivePhases =
                    NiRenderer::PHASE_PER_SHADER |
                    NiRenderer::PHASE_PER_LIGHTSTATE |
                    NiRenderer::PHASE_PER_MESH;
            }
            else
            {
                callContext.m_uiActivePhases = NiRenderer::PHASE_PER_MESH;
            }
        }

        pPreviousEffectState = callContext.m_pkEffects;
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderCore::Do_InvokeShader()
{
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    ID3D11DeviceContext* pD3D11DeviceContext = pRenderer->GetCurrentD3D11DeviceContext();

    D3D11ShaderConstantManager* pShaderConstantManager = pRenderer->GetShaderConstantManager();

    efd::UInt32 returnVal = 0;
    NiRenderCallContext callContext;

    callContext.m_uiPass = 0;
    callContext.m_uiActivePhases =
        NiRenderer::PHASE_PER_SHADER |
        NiRenderer::PHASE_PER_LIGHTSTATE |
        NiRenderer::PHASE_PER_MESH;

    // No properties or effects...
    pRenderer->SetPropertyState(NULL);
    pRenderer->SetEffectState(NULL);

    callContext.m_pkMesh = NULL;
    callContext.m_pkWorldBound = NULL;
    callContext.m_pkEffects = NULL;
    callContext.m_pkState = NULL;
    callContext.m_pkWorld = NULL;
    callContext.m_pkMeshMaterialBinding = NULL;
    callContext.m_uiPass = 0;
    callContext.m_uiSubmesh = 0;

    // Preprocess the pipeline
    returnVal = PreProcessPipeline(callContext);
    if (returnVal != 0)
    {
        D3D11Error::ReportWarning(
            "D3D11ShaderCore::PreProcessPipeline failed with return value %d in "
            __FUNCTION__
            " on shader '%s', implementation %d; "
            "skipping dispatch.",
            returnVal,
            GetName(),
            m_uiImplementation);
        return;
    }

    // Update the pipeline
    returnVal = UpdatePipeline(callContext);
    if (returnVal != 0)
    {
        D3D11Error::ReportWarning(
            "D3D11ShaderCore::UpdatePipeline failed with return value %d in "
            __FUNCTION__
            " on shader '%s', implementation %d; "
            "skipping dispatch.",
            returnVal,
            GetName(),
            m_uiImplementation);
        return;
    }

    // Iterate over passes
    efd::UInt32 remainingPasses = FirstPass();
    while (remainingPasses != 0)
    {
        // Ensure that this is a compute pass
        EE_ASSERT(m_pCurrentPass->GetComputeShader() != NULL);

        // Setup the rendering pass
        returnVal = SetupRenderingPass(callContext);

        if (returnVal != 0)
        {
            D3D11Error::ReportWarning(
                "D3D11ShaderCore::SetupRenderingPass failed with return value %d in "
                __FUNCTION__
                " on pass %d of shader '%s', implementation %d; "
                "skipping dispatch.",
                returnVal,
                callContext.m_uiPass,
                GetName(),
                m_uiImplementation);
            continue;
        }

        // Don't need to call SetupTransformations - no mesh transforms to set

        // Set the shader programs
        // This is to give the shader final 'override' authority
        returnVal = SetupShaderPrograms(callContext);
        if (returnVal != 0)
        {
            D3D11Error::ReportWarning(
                "D3D11ShaderCore::SetupShaderPrograms failed with return value %d in "
                __FUNCTION__
                " on pass %d of shader '%s', implementation %d; "
                "skipping dispatch.",
                returnVal,
                callContext.m_uiPass,
                GetName(),
                m_uiImplementation);
            continue;
        }

        // Don't need to call PreRenderSubmesh because no IA data needs to be set

        // Apply constants
        pShaderConstantManager->ApplyShaderConstants();

        // Dispatch
        pD3D11DeviceContext->Dispatch(
            m_pCurrentPass->GetComputeThreadGroupCountX(),
            m_pCurrentPass->GetComputeThreadGroupCountY(),
            m_pCurrentPass->GetComputeThreadGroupCountZ());

        returnVal = PostRenderSubmesh(callContext);
        if (returnVal != 0)
        {
            D3D11Error::ReportWarning(
                "D3D11ShaderCore::PostRenderSubmesh failed with return value %d in "
                __FUNCTION__
                " on pass %d of shader '%s', implementation %d; "
                "skipping dispatch.",
                returnVal,
                callContext.m_uiPass,
                GetName(),
                m_uiImplementation);
            continue;
        }

        // Perform any post-rendering steps
        returnVal = PostRender(callContext);
        if (returnVal != 0)
        {
            D3D11Error::ReportWarning(
                "D3D11ShaderCore::PostRender failed with return value %d in "
                __FUNCTION__
                " on pass %d of shader '%s', implementation %d; "
                "skipping dispatch.",
                returnVal,
                callContext.m_uiPass,
                GetName(),
                m_uiImplementation);
            continue;
        }

        // Inform the shader to move to the next pass
        remainingPasses = NextPass();
        callContext.m_uiPass++;
    }
    // PostProcess the pipeline
    returnVal = PostProcessPipeline(callContext);
    if (returnVal != 0)
    {
        D3D11Error::ReportWarning(
            "D3D11ShaderCore::PostProcessPipeline failed with return value %d in "
            __FUNCTION__
            " on shader '%s', implementation %d; "
            "skipping dispatch.",
            returnVal,
            GetName(),
            m_uiImplementation);
        return;
    }
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderCore::PreProcessPipeline(const NiRenderCallContext& callContext)
{
    if (!m_bInitialized)
    {
        // Safety catch - fail if the shader hasn't been initialized
        return EE_UINT32_MAX;
    }

    EE_ASSERT(D3D11Renderer::GetRenderer());

    D3D11RenderStateManager* pRenderState = D3D11Renderer::GetRenderer()->GetRenderStateManager();

    EE_ASSERT(pRenderState);

    if (m_usesGBRenderState)
        pRenderState->ApplyProperties(callContext.m_pkState);

    if (m_spRenderStateGroup)
        pRenderState->ApplyRenderStateGroup(m_spRenderStateGroup);

    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderCore::FirstPass()
{
    m_currentPassID = 0;
    efd::UInt32 passCount = m_passArray.GetSize();
    if (m_currentPassID < passCount)
        m_pCurrentPass = m_passArray.GetAt(m_currentPassID);
    else
        m_pCurrentPass = NULL;

    return passCount;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderCore::SetupRenderingPass(const NiRenderCallContext& callContext)
{
    EE_ASSERT(m_pCurrentPass);
    EE_ASSERT(callContext.m_uiPass == m_currentPassID);

    efd::Bool isComputePass = (m_pCurrentPass->GetComputeShader() != NULL);
    if (!isComputePass)
    {
        //
        // Set up Stream Output
        //
        //   This must be done here (not in PreRenderSubmesh()) because the
        //   clearing of the stream out targets (if enabled) should only happen
        //   once per MESH, not once per submesh. (Just in case we ever add
        //   support for submeshes + Stream Output.)
        D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
        ID3D11DeviceContext* pDeviceContext = pRenderer->GetCurrentD3D11DeviceContext();
        NiStreamOutSettings& soSettings = m_pCurrentPass->GetStreamOutSettings();
        efd::UInt32 streamTargetCount = soSettings.GetStreamOutTargetCount();
        if (streamTargetCount == 0)
        {
            pDeviceContext->SOSetTargets(0, NULL, NULL);
        }
        else
        {
            NiMesh* pMesh = NiVerifyStaticCast(NiMesh, callContext.m_pkMesh);
            efd::UInt32 meshOutputStreamCount =
                pMesh->GetCurrentMaterialOutputStreamRefCount();

            // Append or Clear?
            efd::Bool appendStream = soSettings.GetStreamOutAppend();
            if (pMesh->CheckIfActiveMaterialStreamOutBuffersNeedCleared())
                appendStream = false;
            if (!appendStream)  // flag the mesh that we're doing the clear.
                pMesh->OnActiveMaterialStreamOutBuffersCleared();

            ID3D11Buffer* soTargetArray[D3D11_SO_BUFFER_SLOT_COUNT];
            efd::UInt32 offsetArray[D3D11_SO_BUFFER_SLOT_COUNT];
            for (efd::UInt32 i = 0; i < streamTargetCount; i++)
            {
                // offset: 0 = write to stream start; -1 = appendStream
                offsetArray[i] = appendStream ? -1 : 0;

                // Find the correct stream, by name.
                soTargetArray[i] = NULL;
                const NiFixedString& streamName = soSettings.GetStreamOutTarget(i);
                for (efd::UInt32 j = 0; j < meshOutputStreamCount; j++)
                {
                    const NiDataStreamRef* pStreamRef = 
                        pMesh->GetCurrentMaterialOutputStreamRefAt(j);
                    const NiFixedString& streamNameFromMaterial = 
                        pMesh->GetCurrentMaterialOutputStreamRefNameAt(j);
                    if (streamName == streamNameFromMaterial)
                    {
                        // We've now matched desired output stream by name, to an
                        // output stream on the mesh with that name as its
                        // semantic.
                        D3D11DataStream* pStream = NiVerifyStaticCast(
                            D3D11DataStream, 
                            pStreamRef->GetDataStream());
                        EE_ASSERT(0 != (pStream->GetUsage() | NiDataStream::USAGE_VERTEX));

                        soTargetArray[i] = pStream->GetBuffer();
                        if (!soTargetArray[i])
                        {
                            pStream->UpdateD3D11Buffers();
                            soTargetArray[i] = pStream->GetBuffer();
                        }
                        EE_ASSERT(soTargetArray[i]);

                        break;
                    }
                }
                if (!soTargetArray[i])
                {
                    D3D11Error::ReportWarning(
                        "%s: Failed to bind one or more Stream Output targets.\n",
                        __FUNCTION__);
                    return EE_UINT32_MAX;
                }
            }
            pDeviceContext->SOSetTargets(streamTargetCount, soTargetArray, offsetArray);
        }
    }

    return m_pCurrentPass->SetupRenderingPass(this);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderCore::SetupTransformations(const NiRenderCallContext& callContext)
{
    EE_ASSERT(D3D11Renderer::GetRenderer());
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();

    if (m_currentPassID == 0)
        pRenderer->SetModelTransform(*callContext.m_pkWorld);

    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderCore::SetupShaderPrograms(const NiRenderCallContext& callContext)
{
    // If there is an NiSCMExtraData object on this geometry, we want to
    // reset the iterator to 0 so we can hit that cache when we set
    // attribute constants.
    ResetSCMExtraData(callContext.m_pkMesh);

    // Setup the shader programs and constants for the current pass.
    // If this is the first pass, then also set the 'global' constants
    // and render states.
    // Set the shader constants
    EE_ASSERT(m_pCurrentPass);

    m_pCurrentPass->ApplyShaderPrograms(callContext);
    return m_pCurrentPass->ApplyShaderConstants(callContext);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderCore::PreRenderSubmesh(const NiRenderCallContext& callContext)
{
    NiMesh* pMesh = NiVerifyStaticCast(NiMesh, callContext.m_pkMesh);
    D3D11Renderer* pRenderer = D3D11Renderer::GetRenderer();
    ID3D11DeviceContext* pDeviceContext = pRenderer->GetCurrentD3D11DeviceContext();
    D3D11DeviceState* pDeviceState = pRenderer->GetDeviceState();

    D3D11MeshMaterialBinding* pMMB = (D3D11MeshMaterialBinding*)callContext.m_pkMeshMaterialBinding;

    efd::Bool drawAuto = pMesh->GetInputDataIsFromStreamOut();

    //
    // Set Index Stream
    //
    efd::UInt32 indexStreamsSet = 0;
    const NiDataStreamRef* pIndexStreamRef = pMMB->GetIndexStreamRef();
    efd::Bool adjacency = false;

    if (pIndexStreamRef)
    {
        D3D11DataStream* pIndexStream = NiVerifyStaticCast(
            D3D11DataStream,
            pIndexStreamRef->GetDataStream());

        pIndexStream->UpdateD3D11Buffers();

        // Require GPU Read
        if ((pIndexStream->GetAccessMask() & NiDataStream::ACCESS_GPU_READ) == 0)
        {
            D3D11Error::ReportWarning(
                "Attempt to use non-GPU-readable index buffer in "
                __FUNCTION__
                " on mesh '%s, pointer: 0x%08X; "
                "skipping render.",
                pMesh->GetName(),
                pMesh);

            return EE_UINT32_MAX;
        }

        // Only element 0 allowed for index buffers
        const NiDataStreamElement& dsElem = pIndexStream->GetElementDescAt(0);

        DXGI_FORMAT ibFormat = D3D11PixelFormat::DetermineDXGIFormat(dsElem.GetFormat());

        if (ibFormat == DXGI_FORMAT_UNKNOWN ||
            !pRenderer->DoesFormatSupportFlag(ibFormat, D3D11_FORMAT_SUPPORT_IA_INDEX_BUFFER))
        {
            D3D11Error::ReportWarning(
                "Attempt to use unsupported index buffer format %s in "
                __FUNCTION__
                " on mesh '%s, pointer: 0x%08X; "
                "skipping render.",
                D3D11PixelFormat::GetFormatName(ibFormat),
                pMesh->GetName(),
                pMesh);

            return EE_UINT32_MAX;
        }

        // Get the appropriate slot and the offset to that slot
        const NiDataStream::Region& region =
            pIndexStreamRef->GetRegionForSubmesh(callContext.m_uiSubmesh);

        const efd::UInt32 stride = pIndexStream->GetStride();
        const efd::UInt32 regionOffset = region.GetStartIndex() * stride;
        pDeviceState->IASetIndexBuffer(
            pIndexStream->GetBuffer(),
            ibFormat,
            regionOffset);
        indexStreamsSet++;
    }

    //
    // Set Vertex Streams
    //
    ID3D11Buffer* vertexBufferArray[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    efd::UInt32 vbStrideArray[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    efd::UInt32 vbOffsetArray[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    efd::UInt32 numVertexStreams = 0;

    const efd::UInt16* pStreamsToSet = pMMB->GetStreamsToSetArray();
    efd::UInt32 streamCount = pMMB->GetLastValidStream() + 1;
    EE_ASSERT(streamCount < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
    for (efd::UInt32 i = 0; i < streamCount; i++)
    {
        efd::UInt16 streamRefIndex = pStreamsToSet[i];
        NiDataStreamRef* pStreamRef = pMesh->GetStreamRefAt(streamRefIndex);

        D3D11DataStream* pStream = NiVerifyStaticCast(D3D11DataStream, pStreamRef->GetDataStream());
        EE_ASSERT((pStream->GetUsage() == NiDataStream::USAGE_VERTEX) ||
            (pStream->GetUsage() == NiDataStream::USAGE_DISPLAYLIST));

        pStream->UpdateD3D11Buffers();

        // Get the appropriate slot and the offset to that slot
        efd::UInt32 stride = pStream->GetStride();
        if (pStream->GetGPUConstantSingleEntry())
        {
            stride = 0;
        }

        efd::UInt32 regionOffset = 0;
        if (!drawAuto)
        {
            NiDataStream::Region& region = pStreamRef->GetRegionForSubmesh(callContext.m_uiSubmesh);
            regionOffset = region.GetStartIndex() * stride;
        }

        // Require GPU Read
        if ((pStream->GetAccessMask() & NiDataStream::ACCESS_GPU_READ) == 0)
            continue;

        vertexBufferArray[numVertexStreams] = pStream->GetBuffer();
        vbStrideArray[numVertexStreams] = stride;
        vbOffsetArray[numVertexStreams] = regionOffset;
        numVertexStreams++;
    }

    pDeviceState->IASetVertexBuffers(0,
        numVertexStreams,
        vertexBufferArray,
        vbStrideArray,
        vbOffsetArray);

    //
    // Set topology
    //
    D3D11_PRIMITIVE_TOPOLOGY topology =
        D3D11Renderer::GetD3D11TopologyFromPrimitiveType(
        pMesh->GetPrimitiveType(), adjacency);

    // Currently, only triangle patches are supported.
    if (m_pCurrentPass->GetHullShader())
        topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;

    if (topology == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
    {
        D3D11Error::ReportWarning(
            "Attempt to use undefined topology in "
            __FUNCTION__
            " on mesh '%s, pointer: 0x%08X; "
            "skipping render.",
            pMesh->GetName(),
            pMesh);

        return EE_UINT32_MAX;
    }
    pDeviceState->IASetPrimitiveTopology(topology);

    //
    // Set input layout
    //

    // Update input layout with current pass's input signature
    ID3DBlob* pByteCode = m_pCurrentPass->GetInputSignature();
    EE_ASSERT(pByteCode);
    pMMB->UpdateInputLayout(pByteCode->GetBufferPointer(),
        (efd::UInt32)pByteCode->GetBufferSize());

    pDeviceState->IASetInputLayout(pMMB->GetInputLayout());

    return 0;
}

//------------------------------------------------------------------------------------------------
efd::UInt32 D3D11ShaderCore::NextPass()
{
    // Close out the current pass
    if (m_pCurrentPass != NULL)
        m_pCurrentPass->PostProcessRenderingPass(m_currentPassID);

    m_currentPassID++;
    efd::UInt32 passCount = m_passArray.GetSize();
    efd::UInt32 remainingPasses = 0;
    if (m_currentPassID < passCount)
    {
        m_pCurrentPass = m_passArray.GetAt(m_currentPassID);
        remainingPasses = passCount - m_currentPassID;
    }
    else
    {
        m_pCurrentPass = NULL;
    }

    return remainingPasses;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderCore::SetupGeometry(
    NiRenderObject* pMesh,
    NiMaterialInstance* pMaterialInstance)
{
    // Fill this in with the geometry getting assigned 'default' extra
    // data instances the shader expects to see. By default, NiD3DShader
    // simply sets up the NiSCMExtraData so that the engine does not have
    // to call strcmp too many times when rendering.
    SetupSCMExtraData(this, pMesh);

    pMaterialInstance->UpdateSemanticAdapterTable(pMesh);

    // Create the vertex declaration
    D3D11MeshMaterialBindingPtr spMMB = D3D11MeshMaterialBinding::Create(
        NiVerifyStaticCast(NiMesh, pMesh),
        pMaterialInstance->GetSemanticAdapterTable());

    pMaterialInstance->SetVertexDeclarationCache((NiVertexDeclarationCache)spMMB);

    return (spMMB != NULL);
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderCore::SetupSCMExtraData(D3D11ShaderCore* pShader, NiRenderObject* pMesh)
{
    // DT33847: Re-enable NiSCMExtraData usage
#if 0
    // Remove any previous instances of the extra data cache.
    NiShaderConstantMap::RemoveSCMExtraData(pMesh);

    // Determine the number of entries and allocate a new instance so that
    // we get a tightly packed array of entries.

    // If the passes array is not tightly packed, this could cause some
    // waste in the arrays, but that is not likely.
    const efd::UInt32 numPasses = pShader->m_passArray.GetSize();
    efd::UInt32 entryArray[NiGPUProgram::PROGRAM_MAX];
    memset(entryArray, 0, sizeof(entryArray));

    efd::Bool entryFound = false;

    efd::UInt32 pass;
    for (pass = 0; pass < numPasses; pass++)
    {
        D3D11Pass* pPass = pShader->m_passArray.GetAt(pass);
        if (pPass)
        {
            // Check each pass for constant maps and then increment the entry
            // count if that entry is an attribute type entry.
            const efd::UInt32 perPassMapCount = pPass->GetConstantMapCount();
            for (efd::UInt32 i = 0; i < perPassMapCount; i++)
            {
                D3D11ShaderConstantMap* pMap = pPass->GetConstantMap(i);
                if (pMap)
                {
                    efd::UInt32 entryCount = pMap->GetEntryCount();
                    NiGPUProgram::ProgramType programType = pMap->GetProgramType();
                    for (efd::UInt32 j = 0; j < entryCount; j++)
                    {
                        NiShaderConstantMapEntry* pEntry = pMap->GetEntryAtIndex(j);
                        if (pEntry && pEntry->IsAttribute())
                        {
                            ++entryArray[programType];
                            entryFound = true;
                        }
                    }
                }
            }
        }
    }

    // Create the extra data cache table if necessary. We don't want to waste
    // time if there are no attribute type constant map entries.
    NiSCMExtraData* pShaderExtraData = 0;
    if (entryFound)
    {
        pShaderExtraData = NiShaderConstantMap::CreateSCMExtraData(
            entryArray[NiGPUProgram::PROGRAM_VERTEX], 
            entryArray[NiGPUProgram::PROGRAM_GEOMETRY], 
            entryArray[NiGPUProgram::PROGRAM_PIXEL]);
    }
    else
    {
        return;
    }

    // Populate the extra data with pointers. Again, we only insert entries
    // into our cache when the entry in the constant map is of the attribute
    // type.
    for (pass = 0; pass < numPasses; pass++)
    {
        D3D11Pass* pPass = pShader->m_passArray.GetAt(pass);
        if (pPass)
        {
            // Add per pass shader constants.
            const efd::UInt32 perPassMapCount = pPass->GetConstantMapCount();
            for (efd::UInt32 i = 0; i < perPassMapCount; i++)
            {
                D3D11ShaderConstantMap* pMap = pPass->GetConstantMap(i);
                if (pMap)
                {
                    const efd::UInt32 entryCount = pMap->GetEntryCount();
                    for (efd::UInt32 j = 0; j < entryCount; j++)
                    {
                        NiShaderConstantMapEntry* pEntry = pMap->GetEntryAtIndex(j);
                        if (pEntry && pEntry->IsAttribute())
                        {
                            NiExtraData* pExtra = pMesh->GetExtraData(pEntry->GetKey());
                            if (pExtra)
                            {
                                pShaderExtraData->AddEntry(
                                    pEntry->GetShaderRegister(),
                                    0, 
                                    pMap->GetProgramType(),
                                    pExtra, 
                                    false);
                            }
                        }
                    }
                }
            }
        }
    }

    // Attach the NiSCMExtraData object which holds our cached values.
    NiShaderConstantMap::SetSCMExtraData(pMesh, pShaderExtraData);
#else
    EE_UNUSED_ARG(pShader);
    EE_UNUSED_ARG(pMesh);
#endif
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderCore::PrepareResource(
    const NiRenderCallContext& callContext,
    efd::UInt8 shaderResourceID,
    D3D11Pass* pPass,
    efd::Bool updateSampler)
{
    EE_ASSERT(pPass != 0 && shaderResourceID < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

    NiTexturingProperty::ClampMode clampMode = NiTexturingProperty::CLAMP_S_CLAMP_T;
    NiTexturingProperty::FilterMode filterMode = NiTexturingProperty::FILTER_NEAREST;
    efd::UInt16 maxAnisotropy = 1;

    efd::FixedString resourceName;

    efd::Bool success = pPass->ObtainAndCacheResource(
        shaderResourceID, 
        callContext, 
        resourceName,
        clampMode,
        filterMode,
        maxAnisotropy);

    if (!success)
        return;

    if (updateSampler)
    {
        // UpdateSampler will only be true generally for fragment shaders, in which
        // the sampler and the texture share the same name.
        efd::FixedString samplerName = resourceName;
        
        D3D11RenderStateGroup* pRSGroup = pPass->GetRenderStateGroup();
        if (pRSGroup == NULL)
        {
            pRSGroup = EE_NEW D3D11RenderStateGroup;
            pPass->SetRenderStateGroup(pRSGroup);
        }

        D3D11RenderStateGroup::Sampler* pSampler = pRSGroup->GetSampler(samplerName);

        pSampler->SetFilter(
            D3D11RenderStateManager::ConvertGbFilterModeToD3D11Filter(filterMode));

        pSampler->SetMaxAnisotropy(maxAnisotropy);

        efd::Bool mipmapEnable = 
            D3D11RenderStateManager::ConvertGbFilterModeToMipmapEnable(filterMode);

        float maxLOD = (mipmapEnable ? D3D11_FLOAT32_MAX : 0.0f);

        // MaxLOD must be FLT_MAX when feature level 9 HW is in use
        // DT33836 How to disable mipmapping then?
        if (D3D11Renderer::GetRenderer()->GetFeatureLevel() < D3D_FEATURE_LEVEL_10_0)
            maxLOD = D3D11_FLOAT32_MAX;

        pSampler->SetMaxLOD(maxLOD);

        pSampler->SetAddressU(
            D3D11RenderStateManager::ConvertGbClampModeToD3D11AddressU(clampMode));

        pSampler->SetAddressV(
            D3D11RenderStateManager::ConvertGbClampModeToD3D11AddressV(clampMode));
    }
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderCore::ResetSCMExtraData(NiRenderObject* pMesh)
{
    // DT33847: Re-enable NiSCMExtraData usage
#if 0
    if (pMesh)
    {
        NiSCMExtraData* pShaderData = NiShaderConstantMap::GetSCMExtraData(pMesh);
        if (pShaderData)
            pShaderData->Reset();
    }
#else
    EE_UNUSED_ARG(pMesh);
#endif
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderCore::DestroyRendererData()
{
    const efd::UInt32 passCount = m_passArray.GetSize();
    for (efd::UInt32 i = 0; i < passCount; i++)
    {
        D3D11Pass* pPass = m_passArray.GetAt(i);
        if (pPass)
        {
            D3D11VertexShader* pVS = pPass->GetVertexShader();
            if (pVS)
                pVS->DestroyRendererData();

            D3D11HullShader* pHS = pPass->GetHullShader();
            if (pHS)
                pHS->DestroyRendererData();

            D3D11DomainShader* pDS = pPass->GetDomainShader();
            if (pDS)
                pDS->DestroyRendererData();

            D3D11GeometryShader* pGS = pPass->GetGeometryShader();
            if (pGS)
                pGS->DestroyRendererData();

            D3D11PixelShader* pPS = pPass->GetPixelShader();
            if (pPS)
                pPS->DestroyRendererData();

            D3D11ComputeShader* pCS = pPass->GetComputeShader();
            if (pCS)
                pCS->DestroyRendererData();
        }
    }
    m_bInitialized = false;
}

//------------------------------------------------------------------------------------------------
void D3D11ShaderCore::RecreateRendererData()
{
    if (D3D11Renderer::GetRenderer() == NULL)
    {
        D3D11Error::ReportWarning(
            "Attempt recreate renderer data in "
            __FUNCTION__
            " without a valid renderer.");
        return;
    }

    const efd::UInt32 passCount = m_passArray.GetSize();
    for (efd::UInt32 i = 0; i < passCount; i++)
    {
        D3D11Pass* pPass = m_passArray.GetAt(i);
        if (pPass)
        {
            D3D11VertexShader* pVS = pPass->GetVertexShader();
            if (pVS)
                pVS->RecreateRendererData();

            D3D11HullShader* pHS = pPass->GetHullShader();
            if (pHS)
                pHS->RecreateRendererData();

            D3D11DomainShader* pDS = pPass->GetDomainShader();
            if (pDS)
                pDS->RecreateRendererData();

            D3D11GeometryShader* pGS = pPass->GetGeometryShader();
            if (pGS)
                pGS->RecreateRendererData();

            D3D11PixelShader* pPS = pPass->GetPixelShader();
            if (pPS)
                pPS->RecreateRendererData();

            D3D11ComputeShader* pCS = pPass->GetComputeShader();
            if (pCS)
                pCS->RecreateRendererData();
        }
    }
    m_bInitialized = true;
}

//------------------------------------------------------------------------------------------------
UAVSlot* D3D11ShaderCore::AddUAVSlot(efd::FixedString& name)
{
    UAVSlot* pUAVSlot = GetUAVSlot(name);
    if (pUAVSlot == NULL)
    {
        pUAVSlot = EE_NEW UAVSlot(name);
        pUAVSlot->m_pNext = m_pUAVSlots;
        m_pUAVSlots = pUAVSlot;

    }
    return pUAVSlot;
}

//------------------------------------------------------------------------------------------------
UAVSlot* D3D11ShaderCore::FindUAVSlot(
    efd::FixedString& name, 
    UAVSlot*& pUAVSlotBefore) const
{
    pUAVSlotBefore = NULL;
    UAVSlot* pUAVSlot = m_pUAVSlots;
    while (pUAVSlot)
    {
        if (pUAVSlot->m_name == name)
        {
            return pUAVSlot;
        }
        pUAVSlotBefore = pUAVSlot;
        pUAVSlot = pUAVSlot->m_pNext;
    }

    pUAVSlotBefore = NULL;
    return NULL;
}

//------------------------------------------------------------------------------------------------
UAVSlot* D3D11ShaderCore::GetUAVSlot(efd::FixedString& UAVSlotName) const
{
    UAVSlot* pUAVSlotBefore = NULL;
    return FindUAVSlot(UAVSlotName, pUAVSlotBefore);
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderCore::RemoveUAVSlot(
    efd::FixedString& UAVSlotName)
{
    UAVSlot* pUAVSlotBefore = NULL;
    UAVSlot* pRemoveUAVSlot = FindUAVSlot(UAVSlotName, pUAVSlotBefore);
    if (pRemoveUAVSlot == NULL)
        return false;

    if (pUAVSlotBefore)
        pUAVSlotBefore->m_pNext = pRemoveUAVSlot->m_pNext;
    else
        m_pUAVSlots = pRemoveUAVSlot->m_pNext;

    pRemoveUAVSlot->m_pNext = NULL;

    EE_DELETE pRemoveUAVSlot;

    // Bump reset count because UAVSlot pointers held elsewhere may be invalid now.
    m_resetIndexUAVSlots++;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderCore::RemoveUAVSlot(UAVSlot* pUAVSlot)
{
    UAVSlot* pUAVSlotBefore = NULL;
    UAVSlot* pRemoveUAVSlot = FindUAVSlot(
        pUAVSlot->m_name, 
        pUAVSlotBefore);
    if (pRemoveUAVSlot == NULL)
        return false;

    // It should never be the case that multiple UAVSlots with the same name/program type exist.
    EE_ASSERT(pRemoveUAVSlot == pUAVSlot);

    if (pUAVSlotBefore)
        pUAVSlotBefore->m_pNext = pRemoveUAVSlot->m_pNext;
    else
        m_pUAVSlots = pRemoveUAVSlot->m_pNext;

    pRemoveUAVSlot->m_pNext = NULL;

    EE_DELETE pRemoveUAVSlot;

    // Bump reset count because UAVSlot pointers held elsewhere may be invalid now.
    m_resetIndexUAVSlots++;

    return true;
}

//------------------------------------------------------------------------------------------------
efd::Bool D3D11ShaderCore::ObtainTexture(
    efd::UInt32 gbMapFlags,
    efd::UInt16 objectTextureFlags,
    const NiRenderCallContext& callContext,
    NiTexture*& pTexture,
    NiTexturingProperty::ClampMode& clampMode,
    NiTexturingProperty::FilterMode& filterMode,
    efd::UInt16& maxAnisotropy)
{
    EE_ASSERT(callContext.m_pkState != NULL);
    const NiTexturingProperty* pTexProp = callContext.m_pkState->GetTexturing();
    const NiDynamicEffectState* pEffects = callContext.m_pkEffects;
    NiMesh* pMesh = NiVerifyStaticCast(NiMesh, callContext.m_pkMesh);

    if (objectTextureFlags == NiTextureStage::TSOTF_IGNORE)
    {
        if (gbMapFlags != NiTextureStage::TSTF_IGNORE)
        {
            const NiTexturingProperty::Map* pMap = NULL;
            EE_ASSERT(pTexProp);
            if ((gbMapFlags & NiTextureStage::TSTF_MAP_MASK) == 0)
            {
                switch (gbMapFlags & NiTextureStage::TSTF_NDL_TYPEMASK)
                {
                case NiTextureStage::TSTF_NONE:
                    break;
                case NiTextureStage::TSTF_NDL_BASE:
                    pMap = pTexProp->GetBaseMap();
                    break;
                case NiTextureStage::TSTF_NDL_DARK:
                    pMap = pTexProp->GetDarkMap();
                    break;
                case NiTextureStage::TSTF_NDL_DETAIL:
                    pMap = pTexProp->GetDetailMap();
                    break;
                case NiTextureStage::TSTF_NDL_GLOSS:
                    pMap = pTexProp->GetGlossMap();
                    break;
                case NiTextureStage::TSTF_NDL_GLOW:
                    pMap = pTexProp->GetGlowMap();
                    break;
                case NiTextureStage::TSTF_NDL_BUMP:
                    pMap = pTexProp->GetBumpMap();
                    break;
                case NiTextureStage::TSTF_NDL_NORMAL:
                    pMap = pTexProp->GetNormalMap();
                    break;
                case NiTextureStage::TSTF_NDL_PARALLAX:
                    pMap = pTexProp->GetParallaxMap();
                    break;

                }
            }
            else if ((gbMapFlags & NiTextureStage::TSTF_MAP_MASK) ==
                NiTextureStage::TSTF_MAP_DECAL)
            {
                efd::UInt32 index = gbMapFlags & NiTextureStage::TSTF_INDEX_MASK;
                if (index < pTexProp->GetDecalArrayCount())
                {
                    pMap = pTexProp->GetDecalMap(index);
                }
            }
            else if ((gbMapFlags & NiTextureStage::TSTF_MAP_MASK) ==
                NiTextureStage::TSTF_MAP_SHADER)
            {
                efd::UInt32 index = gbMapFlags & NiTextureStage::TSTF_INDEX_MASK;
                if (index < pTexProp->GetShaderArrayCount())
                {
                    pMap = pTexProp->GetShaderMap(index);
                }
            }

            if (pMap)
            {
                pTexture = pMap->GetTexture();
                clampMode = pMap->GetClampMode();
                filterMode = pMap->GetFilterMode();
                maxAnisotropy = pMap->GetMaxAnisotropy();
                
                // Note that the texture coordinate index is ignored, since
                // that value can only be specified in the pixel shader.
            }
        }
    }
    else if (pEffects)
    {
        NiTextureEffect* pTextureEffect = NULL;
        efd::UInt16 objectIndex = 
            static_cast<efd::UInt16>(objectTextureFlags & NiTextureStage::TSOTF_INDEX_MASK);
        switch ((objectTextureFlags & NiTextureStage::TSOTF_TYPE_MASK) >>
            NiTextureStage::TSOTF_TYPE_SHIFT)
        {
        case NiShaderAttributeDesc::OT_EFFECT_SHADOWDIRECTIONALLIGHT:
        case NiShaderAttributeDesc::OT_EFFECT_DIRSHADOWMAP:
        {
            NiLight* pLight = (NiLight*)NiShaderConstantMap::GetDynamicEffectForObject(
                pEffects,
                NiShaderAttributeDesc::OT_EFFECT_SHADOWDIRECTIONALLIGHT,
                objectIndex);

            EE_ASSERT(pLight);

            NiShadowGenerator* pGenerator = pLight->GetShadowGenerator();
            EE_ASSERT(pGenerator);

            NiShadowMap* pShadowMap = pGenerator->RetrieveShadowMap(
                NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, 
                pMesh);
            EE_ASSERT(pShadowMap);

            pTexture = pShadowMap->GetTexture();
            clampMode = pShadowMap->GetClampMode();
            filterMode = pShadowMap->GetFilterMode();
            maxAnisotropy = pShadowMap->GetMaxAnisotropy();
            break;
        }
        case NiShaderAttributeDesc::OT_EFFECT_SHADOWPOINTLIGHT:
        case NiShaderAttributeDesc::OT_EFFECT_POINTSHADOWMAP:
        {
            NiLight* pLight = (NiLight*)NiShaderConstantMap::GetDynamicEffectForObject(
                pEffects, 
                NiShaderAttributeDesc::OT_EFFECT_SHADOWPOINTLIGHT,
                objectIndex);
            EE_ASSERT(pLight);

            NiShadowGenerator* pGenerator = pLight->GetShadowGenerator();
            EE_ASSERT(pGenerator);

            NiShadowMap* pShadowMap = pGenerator->RetrieveShadowMap(
                NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, 
                pMesh);
            EE_ASSERT(pShadowMap);

            pTexture = pShadowMap->GetTexture();
            clampMode = pShadowMap->GetClampMode();
            filterMode = pShadowMap->GetFilterMode();
            maxAnisotropy = pShadowMap->GetMaxAnisotropy();
            break;
        }
        case NiShaderAttributeDesc::OT_EFFECT_SHADOWSPOTLIGHT:
        case NiShaderAttributeDesc::OT_EFFECT_SPOTSHADOWMAP:
        {
            NiLight* pLight = (NiLight*)
                NiShaderConstantMap::GetDynamicEffectForObject(
                pEffects, 
                NiShaderAttributeDesc::OT_EFFECT_SHADOWSPOTLIGHT,
                objectIndex);
            EE_ASSERT(pLight);

            NiShadowGenerator* pGenerator = pLight->GetShadowGenerator();
            EE_ASSERT(pGenerator);

            NiShadowMap* pShadowMap = pGenerator->RetrieveShadowMap(
                NiShadowGenerator::AUTO_DETERMINE_SM_INDEX,
                pMesh);
            EE_ASSERT(pShadowMap);

            pTexture = pShadowMap->GetTexture();
            clampMode = pShadowMap->GetClampMode();
            filterMode = pShadowMap->GetFilterMode();
            maxAnisotropy = pShadowMap->GetMaxAnisotropy();
            break;
        }
        case NiShaderAttributeDesc::OT_EFFECT_ENVIRONMENTMAP:
            pTextureEffect = pEffects->GetEnvironmentMap();
            break;
        case NiShaderAttributeDesc::OT_EFFECT_PROJECTEDSHADOWMAP:
        {
            efd::UInt16 index = 0;
            NiDynEffectStateIter iter = pEffects->GetProjShadowHeadPos();
            while (iter)
            {
                NiTextureEffect* pProjShadow = pEffects->GetNextProjShadow(iter);
                if (pProjShadow && pProjShadow->GetSwitch() == true)
                {
                    if (index++ == objectIndex)
                    {
                        pTextureEffect = pProjShadow;
                        break;
                    }
                }
            }
            break;
        }
        case NiShaderAttributeDesc::OT_EFFECT_PROJECTEDLIGHTMAP:
        {
            efd::UInt16 index = 0;
            NiDynEffectStateIter iter = pEffects->GetProjLightHeadPos();
            while (iter)
            {
                NiTextureEffect* pProjLight = pEffects->GetNextProjLight(iter);
                if (pProjLight && pProjLight->GetSwitch() == true)
                {
                    if (index++ == objectIndex)
                    {
                        pTextureEffect = pProjLight;
                        break;
                    }
                }
            }
            break;
        }
        case NiShaderAttributeDesc::OT_EFFECT_FOGMAP:
            pTextureEffect = pEffects->GetFogMap();
            break;
        case NiShaderAttributeDesc::OT_EFFECT_PSSMSLICENOISEMASK:
            {
                NiPSSMShadowClickGenerator* pPSSMGenerator = (NiPSSMShadowClickGenerator*)
                    NiShadowManager::GetActiveShadowClickGenerator();
                EE_ASSERT(pPSSMGenerator);
                EE_ASSERT(NiIsKindOf(NiPSSMShadowClickGenerator,
                    NiShadowManager::GetActiveShadowClickGenerator()));

#if defined(EE_ASSERTS_ARE_ENABLED)
                NiLight* pLight = (NiLight*)NiShaderConstantMap::GetDynamicEffectForObject(
                    pEffects,
                    NiShaderAttributeDesc::OT_EFFECT_SHADOWDIRECTIONALLIGHT,
                    objectIndex);
                EE_ASSERT(pLight);

                NiPSSMConfiguration* pConfig = pPSSMGenerator->GetConfiguration(
                    pLight->GetShadowGenerator());
                EE_ASSERT(pConfig);

                EE_ASSERT(pConfig->GetSliceTransitionEnabled() &&
                    "Remember to call UpdateEffects on all receivers.");
#endif

                NiTexturingProperty::Map* pNoiseMap = pPSSMGenerator->GetNoiseTextureMap();
                if (pNoiseMap)
                {
                    pTexture = pNoiseMap->GetTexture();
                    clampMode = (NiTexturingProperty::ClampMode)pNoiseMap->GetClampMode();
                    filterMode = (NiTexturingProperty::FilterMode)pNoiseMap->GetFilterMode();
                }
                break;
            }
        case NiShaderAttributeDesc::OT_EFFECT_LPP_GBUFFER:
        {
            pTexture = NiTextureEffect::GetLPPGBuffer();
            clampMode = NiTexturingProperty::CLAMP_S_CLAMP_T;
            filterMode = NiTexturingProperty::FILTER_BILERP;
            break;
        }
        case NiShaderAttributeDesc::OT_EFFECT_LPP_LBUFFER:
        {
            pTexture = NiTextureEffect::GetLPPLBuffer();
            clampMode = NiTexturingProperty::CLAMP_S_CLAMP_T;
            filterMode = NiTexturingProperty::FILTER_BILERP;
            break;
        }
        default:
            // This assertion is hit when the object type is not one
            // that has a texture.
            EE_ASSERT(false);
            break;
        }

        if (pTextureEffect && pTextureEffect->GetSwitch())
        {
            pTexture = pTextureEffect->GetEffectTexture();
            clampMode = pTextureEffect->GetTextureClamp();
            filterMode = pTextureEffect->GetTextureFilter();
            maxAnisotropy = pTextureEffect->GetMaxAnisotropy();
        }
    }
    return (pTexture != NULL);
}

//------------------------------------------------------------------------------------------------
UAVSlot::UAVSlot(efd::FixedString name) :
    m_name(name),
    m_pNext(NULL)
{
    /* */
}

//------------------------------------------------------------------------------------------------
UAVSlot::~UAVSlot()
{
    EE_DELETE m_pNext;
}

//------------------------------------------------------------------------------------------------