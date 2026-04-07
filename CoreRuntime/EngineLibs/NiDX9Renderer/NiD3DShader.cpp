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

#include "NiD3DError.h"
#include "NiD3DPass.h"
#include "NiD3DRenderStateGroup.h"
#include "NiD3DRendererHeaders.h"
#include "NiD3DShader.h"
#include "NiD3DShaderFactory.h"
#include "NiD3DShaderProgramFactory.h"
#include "NiDX9DataStream.h"
#include "NiDX9MeshMaterialBinding.h"
#include "NiMesh.h"


NiD3DShader::DynamicEffectPacker NiD3DShader::ms_apfnDynEffectPackers[
    NiTextureEffect::NUM_COORD_GEN];

NiImplementRTTI(NiD3DShader, NiD3DShaderInterface);

//------------------------------------------------------------------------------------------------
void NiD3DShader::_SDMInit()
{
    ms_apfnDynEffectPackers[NiTextureEffect::WORLD_PARALLEL] =
        &PackWorldParallelEffect;
    ms_apfnDynEffectPackers[NiTextureEffect::WORLD_PERSPECTIVE] =
        &PackWorldPerspectiveEffect;
    ms_apfnDynEffectPackers[NiTextureEffect::SPHERE_MAP] =
        &PackWorldSphereEffect;
    ms_apfnDynEffectPackers[NiTextureEffect::SPECULAR_CUBE_MAP] =
        &PackSpecularCubeEffect;
    ms_apfnDynEffectPackers[NiTextureEffect::DIFFUSE_CUBE_MAP] =
        &PackDiffuseCubeEffect;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::_SDMShutdown()
{

}

//------------------------------------------------------------------------------------------------
NiD3DShader::~NiD3DShader()
{
    if (m_kName.Exists() && m_kName.GetLength() != 0)
    {
        NiD3DShaderFactory* pkShaderFactory =
            NiD3DShaderFactory::GetD3DShaderFactory();

        // Attempt to remove the shader wherever it has been added
        if (this ==
            pkShaderFactory->FindShader(m_kName, GetImplementation()))
        {
            pkShaderFactory->RemoveShader(m_kName, GetImplementation());
        }
    }
    if (m_pkD3DRenderer)
        m_pkD3DRenderer->ReleaseD3DShader(this);

    m_bInitialized = false;
    NiD3DRenderStateGroup::ReleaseRenderStateGroup(m_pkRenderStateGroup);
    m_uiCurrentPass = 0;
    m_uiPassCount = 0;
    for (unsigned int ui = 0; ui < m_kPasses.GetSize(); ui++)
    {
        m_kPasses.SetAt(ui, 0);
    }

    m_kPasses.RemoveAll();
}

//------------------------------------------------------------------------------------------------
bool NiD3DShader::Initialize()
{
    // By default, we don't do anything in this call. (At the current moment)
    // A derived class would want to load any shader files at this point,
    // as well as any other 'miscellaneous' data it requires. This call is
    // provided to allow for shaders to be instantiated during streaming,
    // but not initialized. This prevents the shader from interfering with
    // load times by requiring a random seek during the loading of a nif
    // file.

    return NiD3DShaderInterface::Initialize();
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::Do_RenderMeshes(NiVisibleArray* pkVisibleArray)
{
    if (pkVisibleArray->GetCount() == 0)
        return;

    NiDX9Renderer* pkRenderer = NiDX9Renderer::GetRenderer();

    NiDX9RenderState* pkRenderState = pkRenderer->GetRenderState();
    LPDIRECT3DDEVICE9 pkD3DDevice9 = pkRenderer->GetD3DDevice();

    NIMETRICS_DX9RENDERER_SCOPETIMER(DRAW_TIME_MESH);
    unsigned int uiMetricDPCalls = 0;

    unsigned int uiRet;
    NiRenderCallContext kRCC;

    kRCC.m_uiPass = 0;
    kRCC.m_uiActivePhases =
        NiRenderer::PHASE_PER_SHADER |
        NiRenderer::PHASE_PER_LIGHTSTATE |
        NiRenderer::PHASE_PER_MESH;

    bool bStatesCommitted = false;
    const NiDynamicEffectState* pkPreviousEffectState = NULL;

    for (NiUInt32 uiRenderObjectIndex = 0;
        uiRenderObjectIndex < pkVisibleArray->GetCount();
        uiRenderObjectIndex++)
    {
        NiMesh* pkMesh = NiVerifyStaticCast(NiMesh, &pkVisibleArray->GetAt(uiRenderObjectIndex));

        // Enable the PHASE_PER_LIGHTSTATE phase if the dynamic effect
        // state has changed and the previous states have been
        // commited to the shader constants.
        if (bStatesCommitted && pkPreviousEffectState != pkMesh->GetEffectState())
            kRCC.m_uiActivePhases |= NiRenderer::PHASE_PER_LIGHTSTATE;

        NiSyncArgs kSyncArgs;
        kSyncArgs.m_uiSubmitPoint = NiSyncArgs::SYNC_ANY;
        kSyncArgs.m_uiCompletePoint = NiSyncArgs::SYNC_RENDER;
        pkMesh->CompleteModifiers(&kSyncArgs);

        NiTimeController::OnPreDisplayIterate(pkMesh->GetControllers());

        pkRenderer->SetPropertyState(pkMesh->GetPropertyState());
        pkRenderer->SetEffectState(pkMesh->GetEffectState());

        const NiMaterialInstance* pkMatInst = pkMesh->GetActiveMaterialInstance();
        NiDX9MeshMaterialBindingPtr spVertexDeclaration;
        if (!pkMatInst)
        {
            pkRenderer->GetShaderAndVertexDecl(pkMesh, spVertexDeclaration);
            if (spVertexDeclaration == NULL)
            {
                EE_FAIL("GetShaderAndVertexDecl failed - skipping render");
                continue;
            }
        }
        else
        {
            spVertexDeclaration = (NiDX9MeshMaterialBinding*)pkMatInst->GetVertexDeclarationCache();
        }

        NiTransform kWorld = pkMesh->GetWorldTransform();
        NiBound kWorldBound = pkMesh->GetWorldBound();

        kRCC.m_pkMesh = pkMesh;
        kRCC.m_pkMeshMaterialBinding = ((NiVertexDeclarationCache*)(spVertexDeclaration.data()));
        kRCC.m_pkWorldBound = &kWorldBound;
        kRCC.m_pkEffects = pkRenderer->GetEffectState();
        kRCC.m_pkState = pkRenderer->GetPropertyState();
        kRCC.m_pkWorld = &kWorld;
        kRCC.m_uiPass = 0;

        // Preprocess the pipeline
        uiRet = PreProcessPipeline(kRCC);

        if (uiRet != 0)
        {
            EE_FAIL("PreProcess failed - skipping render");
            continue;
        }

        // Update the pipeline
        uiRet = UpdatePipeline(kRCC);

        // Convert primitive type to geometry type
        D3DPRIMITIVETYPE eGeomType;
        if (!pkRenderer->GetDX9PrimitiveFromNiMeshPrimitiveType(
            pkMesh->GetPrimitiveType(),
            eGeomType))
        {
            continue;
        }
        EE_ASSERT(eGeomType);

        // Get the first vertex buffer
        if (spVertexDeclaration->m_kStreamsToSet.GetSize() == 0)
        {
            NiDX9Renderer::Warning(
                "NiD3DShader::RenderMeshes> "
                "No vertex streams found to render with on"
                "'%s' object, pointer: %x, skipping render.",
                pkMesh->GetName(),
                pkMesh);
            continue;
        }

        bool bIndexed = spVertexDeclaration->m_pkIndexStreamRef != NULL;

        const NiDataStreamRef* pkStreamRef = NULL;
        if (bIndexed)
        {
            pkStreamRef = spVertexDeclaration->m_pkIndexStreamRef;
        }
        else
        {
            pkStreamRef = pkMesh->GetStreamRefAt(
                spVertexDeclaration->m_kStreamsToSet.GetAt(0));
        }

        // Set vertex declaration on device
        pkD3DDevice9->SetVertexDeclaration(spVertexDeclaration->GetD3DDeclaration());

        // Get active submesh count
        NiUInt32 uiSubmeshCount = 0;
        if (pkMesh->GetInstanced())
            uiSubmeshCount = NiInstancingUtilities::GetVisibleSubmeshCount(pkMesh);
        else
            uiSubmeshCount = pkMesh->GetSubmeshCount();

        // Iterate over passes
        unsigned int uiRemainingPasses = FirstPass();
        unsigned int uiTotalPasses = uiRemainingPasses;
        while (uiRemainingPasses != 0)
        {
            // Setup the rendering pass
            uiRet = SetupRenderingPass(kRCC);

            // Set the transformations
            uiRet = SetupTransformations(kRCC);

            // Iterate over submesh regions
            for (NiUInt32 uiSubmesh = 0; uiSubmesh < uiSubmeshCount; ++uiSubmesh)
            {
                kRCC.m_uiSubmesh = uiSubmesh;

                NiDataStream::Region kRegion = pkStreamRef->GetRegionForSubmesh(
                    uiSubmesh);
                unsigned int uiElementCount = pkStreamRef->GetCount(uiSubmesh);
                unsigned int uiPrimitiveCount =
                    pkMesh->GetPrimitiveCountFromElementCount(uiElementCount);

                if (uiPrimitiveCount == 0)
                {
                    // DT33195
                    // A zero primitive submesh is not an error, but it is inefficient if all
                    // submeshes have 0 primitives. We should determine how to detect this
                    // earlier and log appropriately.
                    continue;
                }
                // Set the shader programs
                // This is to give the shader final 'override' authority
                SetupShaderPrograms(kRCC);

                pkRenderState->CommitShaderConstants();

                // The current states are now committed to
                // shader constaints.
                bStatesCommitted = true;

                // Setup the current submesh
                PreRenderSubmesh(kRCC);

                NiUInt32 uiNumVertices;
                if (bIndexed)
                {
                    // Draw indexed
                    uiNumVertices = pkMesh->GetVertexCount(uiSubmesh);

                    pkD3DDevice9->DrawIndexedPrimitive(
                        eGeomType, // Type
                        0, // BaseVertexIndex
                        0, // MinIndex
                        uiNumVertices, // NumVertices
                        kRegion.GetStartIndex(), // StartIndex
                        uiPrimitiveCount); // PrimitiveCount
                }
                else
                {
                    // Draw non-indexed
                    uiNumVertices = kRegion.GetRange();

                    pkD3DDevice9->DrawPrimitive(
                        eGeomType, // Type
                        0, // StartIndex (offset applied at SetStreamSource)
                        uiPrimitiveCount); // PrimitiveCount
                }

                uiMetricDPCalls++;
                NIMETRICS_DX9RENDERER_AGGREGATEVALUE(DRAW_PRIMITIVES,
                    uiPrimitiveCount);
                NIMETRICS_DX9RENDERER_AGGREGATEVALUE(DRAW_VERTS,
                    uiNumVertices);

                PostRenderSubmesh(kRCC);
            }

            // Perform any post-rendering steps
            uiRet = PostRender(kRCC);

            // Inform the shader to move to the next pass
            uiRemainingPasses = NextPass();
            kRCC.m_uiPass++;
        }

        // PostProcess the pipeline
        PostProcessPipeline(kRCC);

        // Only once state changes have been commited to
        // the shader constants can we change phase.
        if (bStatesCommitted)
        {
            // If we have single pass objects we can do per
            // mesh phase rendering. If we have multi pass objects
            // we must do per lightstate and per mesh rendering
            // since every pass after the first pass may overwrite
            // the shader constants set in the previous passes.
            if (uiTotalPasses > 1)
                kRCC.m_uiActivePhases = NiRenderer::PHASE_PER_MESH |
                    NiRenderer::PHASE_PER_LIGHTSTATE;
            else
                kRCC.m_uiActivePhases = NiRenderer::PHASE_PER_MESH;
        }
        pkPreviousEffectState = kRCC.m_pkEffects;
    }

    NIMETRICS_DX9RENDERER_AGGREGATEVALUE(DRAW_SUBMESH_COUNT, uiMetricDPCalls);
}

//------------------------------------------------------------------------------------------------
unsigned int NiD3DShader::FirstPass()
{
    // Open first pass
    m_uiCurrentPass = 0;
    m_spCurrentPass = m_kPasses.GetAt(m_uiCurrentPass);

    return m_uiPassCount;
}

//------------------------------------------------------------------------------------------------
unsigned int NiD3DShader::NextPass()
{
    // Close out the current pass
    if (m_spCurrentPass != NULL)
        m_spCurrentPass->PostProcessRenderingPass(m_uiCurrentPass);

    // Advance to the next pass
    m_uiCurrentPass++;
    if (m_uiCurrentPass == m_uiPassCount)
        return 0;

    m_spCurrentPass = m_kPasses.GetAt(m_uiCurrentPass);

    return (m_uiPassCount - m_uiCurrentPass);
}

//------------------------------------------------------------------------------------------------
unsigned int NiD3DShader::PreProcessPipeline(
    const NiRenderCallContext& kRCC)
{
    // Safety catch - fail if the shader hasn't been initialized
    if (!m_bInitialized)
    {
        return 0xFFFFFFFF;
    }

    if (m_bUsesNiRenderState)
    {
        // Update the render state
        m_pkD3DRenderState->UpdateRenderState(kRCC.m_pkState);
    }

// Xenon handles lights via shaders and does not have a light manager.
#if !defined(_XENON)
    if (m_bUsesNiLightState)
    {
        // Update the light state
        if (m_pkD3DRenderer->GetLightManager())
        {
            m_pkD3DRenderer->GetLightManager()->SetState(
                kRCC.m_pkEffects,
                kRCC.m_pkState->GetTexturing(),
                kRCC.m_pkState->GetVertexColor());
        }
    }
#endif

    // Set the 'global' rendering states
    if (m_pkRenderStateGroup != NULL)
    {
        m_pkRenderStateGroup->SetRenderStates();
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
unsigned int NiD3DShader::UpdatePipeline(const NiRenderCallContext&)
{
    // By default, nothing to do in UpdatePipline
    return 0;
}

//------------------------------------------------------------------------------------------------
unsigned int NiD3DShader::SetupRenderingPass(const NiRenderCallContext& kRCC)
{
    unsigned int uiRet = 0;

    // Setup the current pass
    uiRet = m_spCurrentPass->SetupRenderingPass(kRCC);

    return uiRet;
}

//------------------------------------------------------------------------------------------------
unsigned int NiD3DShader::SetupTransformations(const NiRenderCallContext& kRCC)
{
    if (m_uiCurrentPass == 0)
    {
        m_pkD3DRenderState->SetBoneCount(0);
        m_pkD3DRenderer->SetModelTransform(*kRCC.m_pkWorld);
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
unsigned int NiD3DShader::SetupShaderPrograms(const NiRenderCallContext& kRCC)
{
    unsigned int uiRet = 0;

    // If there is an NiSCMExtraData object on this geometry, we want to
    // reset the iterator to 0 so we can hit that cache when we set
    // attribute constants.
    ResetSCMExtraData(kRCC.m_pkMesh);

    // Setup the shader programs and constants for the current pass.
    // If this is the first pass, then also set the 'global' constants
    // and render states.
    // Set the shader constants
    uiRet = m_spCurrentPass->SetupShaderPrograms(kRCC);

    // Now that the shaders have been set, if there are any 'global'
    // mappings, set them now. This has to occur now since the pixel
    // shader has to be set prior to setting pixel shader constants.
    // This must be done every pass, since using a different shader on
    // a different pass may require constant remapping.
    NiD3DError eErr = NISHADERERR_OK;

    // Pixel shader mappings
    if (m_spPixelConstantMap && m_spCurrentPass->GetPixelShader())
    {
        eErr = m_spPixelConstantMap->SetShaderConstants(
            kRCC, m_spCurrentPass->GetPixelShader(), true);
    }

    // Vertex shader mappings
    if (m_spVertexConstantMap && m_spCurrentPass->GetVertexShader())
    {
        eErr = m_spVertexConstantMap->SetShaderConstants(
            kRCC, m_spCurrentPass->GetVertexShader(), true);
    }

    return uiRet;
}

//------------------------------------------------------------------------------------------------
unsigned int NiD3DShader::PreRenderSubmesh(const NiRenderCallContext& kRCC)
{
    NiMesh* pkMesh = NiVerifyStaticCast(NiMesh, kRCC.m_pkMesh);
    unsigned int uiRet = 0;

    EE_ASSERT(NiRenderer::GetRenderer());
    NiRenderer::GetRenderer()->GetFrameID();

    NiDX9MeshMaterialBinding* pkMMB =
        (NiDX9MeshMaterialBinding*)kRCC.m_pkMeshMaterialBinding;

    //
    // Set Index Stream
    //
    unsigned int uiIndexStreamsSet = 0;
    const NiDataStreamRef* pkIndexStreamRef = pkMMB->m_pkIndexStreamRef;

    if (pkIndexStreamRef)
    {
        NiDX9DataStream* pkIndexStream = NiVerifyStaticCast(NiDX9DataStream,
            pkIndexStreamRef->GetDataStream());

        pkIndexStream->UpdateD3DBuffers();
        m_pkD3DDevice->SetIndices(pkIndexStream->GetIndexBuffer());
        uiIndexStreamsSet++;
    }

    //
    // Set Vertex Streams
    //
    NiTPrimitiveArray<NiUInt16>& kStreamsToSet = pkMMB->m_kStreamsToSet;
    unsigned int uiStreamCount = kStreamsToSet.GetSize();
    unsigned int uiVertexStreamsSet = 0;
    for (unsigned int uiStream = 0;
        uiStream < uiStreamCount;
        uiStream++)
    {
        NiUInt16 uiStreamRefIndex = kStreamsToSet.GetAt(uiStream);
        NiDataStreamRef* pkStreamRef =
            pkMesh->GetStreamRefAt(uiStreamRefIndex);

        NiDX9DataStream* pkStream =
            NiVerifyStaticCast(NiDX9DataStream, pkStreamRef->GetDataStream());

        pkStream->UpdateD3DBuffers();

        // Get the appropriate slot and the offset to that slot

        NiDataStream::Region& kRegion =
            pkStreamRef->GetRegionForSubmesh(kRCC.m_uiSubmesh);

        NiUInt32 uiStride = pkStream->GetStride();
        if (pkStream->GetGPUConstantSingleEntry())
        {
            uiStride = 0;
        }

        NiUInt32 uiRegionOffset = kRegion.GetStartIndex() * uiStride;

        HRESULT hr;
        if (pkMesh->GetInstanced())
        {
            if (!pkStreamRef->IsPerInstance())
            {
                hr = m_pkD3DDevice->SetStreamSourceFreq(
                    uiVertexStreamsSet,
                    D3DSTREAMSOURCE_INDEXEDDATA |
                    NiInstancingUtilities::GetVisibleInstanceCount(pkMesh));
            }
            else
            {
                hr = m_pkD3DDevice->SetStreamSourceFreq(
                    uiVertexStreamsSet,
                    (UINT)(D3DSTREAMSOURCE_INSTANCEDATA | 1));
            }
        }
        else
        {
            hr = m_pkD3DDevice->SetStreamSourceFreq(uiVertexStreamsSet, 1);
        }
        EE_ASSERT(SUCCEEDED(hr));

        hr = m_pkD3DDevice->SetStreamSource(
            uiVertexStreamsSet, // StreamNumber
            pkStream->GetVertexBuffer(), // pStreamData
            pkStream->GetD3DOffset() + uiRegionOffset, // OffsetInBytes
            uiStride); // Stride
        EE_ASSERT(SUCCEEDED(hr));
        uiVertexStreamsSet++;
    }

    NIMETRICS_DX9RENDERER_AGGREGATEVALUE(VERTEX_BUFFER_CHANGES,
        uiVertexStreamsSet + uiIndexStreamsSet);

    return uiRet;
}

//------------------------------------------------------------------------------------------------
unsigned int NiD3DShader::PostRenderSubmesh(const NiRenderCallContext& kRCC)
{
    NiMesh* pkMesh = NiVerifyStaticCast(NiMesh, kRCC.m_pkMesh);
    unsigned int uiRet = 0;

    if (pkMesh->GetInstanced())
    {
        // Reset all the streams stream source frequency back to 1.
        unsigned int uiStreamCount = pkMesh->GetStreamRefCount();
        unsigned int uiVertexStreamsSet = 0;

        for (unsigned int uiStreamRef = 0;
            uiStreamRef < uiStreamCount;
            uiStreamRef++)
        {
            NiDataStreamRef* pkStreamRef = pkMesh->GetStreamRefAt(uiStreamRef);

            NiDX9DataStream* pkStream = NiVerifyStaticCast(NiDX9DataStream,
                pkStreamRef->GetDataStream());

            if (pkStream->GetUsage() == NiDataStream::USAGE_VERTEX)
            {
                if (pkStream->GetAccessMask() & NiDataStream::ACCESS_GPU_READ)
                {
                    HRESULT hr =
                        m_pkD3DDevice->SetStreamSourceFreq(
                        uiVertexStreamsSet, 1);
                    EE_UNUSED_ARG(hr);
                    EE_ASSERT(SUCCEEDED(hr));
                    uiVertexStreamsSet++;
                }
            }
        }
    }

    return uiRet;
}

//------------------------------------------------------------------------------------------------
unsigned int NiD3DShader::PostProcessPipeline(const NiRenderCallContext&)
{
    // Restore the 'global' rendering states
    if (m_pkRenderStateGroup != NULL)
    {
        m_pkRenderStateGroup->RestoreRenderStates();
    }

    if (m_bUsesNiRenderState)
    {
        // Update the render state
        m_pkD3DRenderState->RestoreRenderState();
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::ResetPasses()
{
    for (unsigned int i = 0; i < m_uiPassCount; i++)
        m_kPasses.SetAt(i, 0);

    m_spCurrentPass = NULL;
    m_uiCurrentPass = 0;
    m_uiPassCount = 0;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::InitializePools()
{
    NiD3DPass::InitializePools();
    NiD3DTextureStage::InitializePools();
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::ShutdownPools()
{
    // Needs to be called by Ni***RendererSDM::Shutdown so all passes and
    // stages are destroyed before the pools.
    NiD3DPass::ShutdownPools();
    NiD3DTextureStage::ShutdownPools();
}

//------------------------------------------------------------------------------------------------
bool NiD3DShader::SetupGeometry(NiRenderObject* pkGeometry,
    NiMaterialInstance* pkMaterialInstance)
{
    // Fill this in with the geometry getting assigned 'default' extra
    // data instances the shader expects to see. By default, NiD3DShader
    // simply sets up the NiSCMExtraData so that the engine does not have
    // to call strcmp too many times when rendering.
    SetupSCMExtraData(this, pkGeometry);

    pkMaterialInstance->UpdateSemanticAdapterTable(pkGeometry);

    // Create the vertex declaration
    NiDX9MeshMaterialBindingPtr spMMB = NiDX9MeshMaterialBinding::Create(
        NiVerifyStaticCast(NiMesh, pkGeometry),
        pkMaterialInstance->GetSemanticAdapterTable(),
        m_bConvertBlendIndicesToD3DColor);

    pkMaterialInstance->SetVertexDeclarationCache(
        (NiVertexDeclarationCache)spMMB);

    return spMMB != NULL;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::DestroyRendererData()
{
    for (unsigned int i = 0; i < m_uiPassCount; i++)
    {
        NiD3DPass* pkPass = m_kPasses.GetAt(i);
        if (pkPass)
        {
            NiD3DPixelShader* pkPS = pkPass->GetPixelShader();
            if (pkPS)
                pkPS->DestroyRendererData();

            NiD3DVertexShader* pkVS = pkPass->GetVertexShader();
            if (pkVS)
                pkVS->DestroyRendererData();
        }
    }
    m_bInitialized = false;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::RecreateRendererData()
{
    if (!m_pkD3DRenderer)
    {
        EE_FAIL("NiD3DShader::RecreateRendererData> Invalid renderer!");
        return;
    }

    for (unsigned int i = 0; i < m_uiPassCount; i++)
    {
        NiD3DPass* pkPass = m_kPasses.GetAt(i);
        if (pkPass)
        {
            NiD3DPixelShader* pkPS = pkPass->GetPixelShader();
            if (pkPS)
                pkPS->RecreateRendererData();

            NiD3DVertexShader* pkVS = pkPass->GetVertexShader();
            if (pkVS)
                pkVS->RecreateRendererData();
        }
    }
    m_bInitialized = true;
}

//------------------------------------------------------------------------------------------------
bool NiD3DShader::GetVSPresentAllPasses() const
{
    for (unsigned int i = 0; i < m_kPasses.GetSize(); i++)
    {
        NiD3DPass* pkPass = m_kPasses.GetAt(i);
        if (pkPass->GetVertexShader() == NULL)
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiD3DShader::GetVSPresentAnyPass() const
{
    for (unsigned int i = 0; i < m_kPasses.GetSize(); i++)
    {
        NiD3DPass* pkPass = m_kPasses.GetAt(i);
        if (pkPass->GetVertexShader())
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
bool NiD3DShader::GetPSPresentAllPasses() const
{
    for (unsigned int i = 0; i < m_kPasses.GetSize(); i++)
    {
        NiD3DPass* pkPass = m_kPasses.GetAt(i);
        if (pkPass->GetPixelShader() == NULL)
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
bool NiD3DShader::GetPSPresentAnyPass() const
{
    for (unsigned int i = 0; i < m_kPasses.GetSize(); i++)
    {
        NiD3DPass* pkPass = m_kPasses.GetAt(i);
        if (pkPass->GetPixelShader())
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::SetupSCMExtraData(NiD3DShader* pkD3DShader,
    NiRenderObject* pkGeometry)
{
    // Remove any previous instances of the extra data cache.
    NiShaderConstantMap::RemoveSCMExtraData(pkGeometry);

    // Determine the number of entries and allocate a new instance so that
    // we get a tightly packed array of entries.
    unsigned int uiNumVertexEntries = 0;
    unsigned int uiNumPixelEntries = 0;
    unsigned int uiCount = 0;

    // Establish the number of entries in the global constant map times the
    // number of passes.

    // Vertex Shader Constants
    if (pkD3DShader->m_spVertexConstantMap != 0)
    {
        unsigned int uiCount =
            pkD3DShader->m_spVertexConstantMap->GetEntryCount();
        for (unsigned int ui = 0; ui < uiCount; ui++)
        {
            NiShaderConstantMapEntry* pkEntry =
                pkD3DShader->m_spVertexConstantMap->GetEntryAtIndex(ui);
            if (pkEntry && pkEntry->IsAttribute())
            {
                ++uiNumVertexEntries;
            }
        }
        // If the passes array is not tightly packed, this could cause some
        // waste in the arrays, but that is not likely.
        uiNumVertexEntries *= pkD3DShader->m_kPasses.GetSize();
    }

    // Pixel Shader Constants
    if (pkD3DShader->m_spPixelConstantMap != 0)
    {
        unsigned int uiCount =
            pkD3DShader->m_spPixelConstantMap->GetEntryCount();
        for (unsigned int ui = 0; ui < uiCount; ui++)
        {
            NiShaderConstantMapEntry* pkEntry =
                pkD3DShader->m_spPixelConstantMap->GetEntryAtIndex(ui);
            if (pkEntry && pkEntry->IsAttribute())
            {
                ++uiNumPixelEntries;
            }
        }
        // If the passes array is not tightly packed, this could cause some
        // waste in the arrays, but that is not likely.
        uiNumPixelEntries *= pkD3DShader->m_kPasses.GetSize();
    }

    unsigned int uiPasses;
    unsigned int uiNumPasses = pkD3DShader->m_kPasses.GetSize();
    for (uiPasses = 0; uiPasses < uiNumPasses; uiPasses++)
    {
        NiD3DPass* pkPass = pkD3DShader->m_kPasses.GetAt(uiPasses);
        if (pkPass)
        {
            // Check each pass for constant maps and then increment the entry
            // count if that entry is an attribute type entry.
            NiD3DShaderConstantMap* pkMap = pkPass->GetPixelConstantMap();
            if (pkMap)
            {

                unsigned int uiCount = pkMap->GetEntryCount();
                for (unsigned int ui = 0; ui < uiCount; ui++)
                {
                    NiShaderConstantMapEntry* pkEntry =
                        pkMap->GetEntryAtIndex(ui);
                    if (pkEntry && pkEntry->IsAttribute())
                    {
                        ++uiNumPixelEntries;
                    }
                }
            }

            pkMap = pkPass->GetVertexConstantMap();
            if (pkMap)
            {
                unsigned int uiCount = pkMap->GetEntryCount();
                for (unsigned int ui = 0; ui < uiCount; ui++)
                {
                    NiShaderConstantMapEntry* pkEntry =
                        pkMap->GetEntryAtIndex(ui);
                    if (pkEntry && pkEntry->IsAttribute())
                    {
                        ++uiNumVertexEntries;
                    }
                }
            }
        }
    }

    // Create the extra data cache table if necessary. We don't want to waste
    // time if there are no attribute type constant map entries.
    NiSCMExtraData* pkShaderExtraData = 0;
    if (uiNumVertexEntries > 0 || uiNumPixelEntries > 0)
    {
        pkShaderExtraData = NiShaderConstantMap::CreateSCMExtraData(
            uiNumVertexEntries, 0, uiNumPixelEntries);
    }
    else
    {
        return;
    }

    // Populate the extra data with pointers. Again, we only insert entries
    // into our cache when the entry in the constant map is of the attribute
    // type.
    for (uiPasses = 0; uiPasses < uiNumPasses; uiPasses++)
    {
        NiD3DPass* pkPass = pkD3DShader->m_kPasses.GetAt(uiPasses);
        if (pkPass)
        {
            // Add global constant maps per pass so that we hit them
            // multiple times when iterating through the lists.
            uiCount = 0;
            if (pkD3DShader->m_spPixelConstantMap != 0)
                uiCount = pkD3DShader->m_spPixelConstantMap->GetEntryCount();
            unsigned int uiEntry;
            for (uiEntry = 0; uiEntry < uiCount; uiEntry++)
            {
                NiShaderConstantMapEntry* pkEntry = pkD3DShader->
                    m_spPixelConstantMap->GetEntryAtIndex(uiEntry);
                if (pkEntry && pkEntry->IsAttribute())
                {
                    NiExtraData* pkExtra =
                        pkGeometry->GetExtraData(pkEntry->GetKey());
                    if (pkExtra)
                    {
                        pkShaderExtraData->AddEntry(
                            pkEntry->GetShaderRegister(), 0,
                            NiGPUProgram::PROGRAM_PIXEL, pkExtra, true);
                    }
                }
            }

            uiCount = 0;
            if (pkD3DShader->m_spVertexConstantMap != 0)
                uiCount = pkD3DShader->m_spVertexConstantMap->GetEntryCount();
            for (uiEntry = 0; uiEntry < uiCount; uiEntry++)
            {
                NiShaderConstantMapEntry* pkEntry = pkD3DShader->
                    m_spVertexConstantMap->GetEntryAtIndex(uiEntry);
                if (pkEntry && pkEntry->IsAttribute())
                {
                    NiExtraData* pkExtra =
                        pkGeometry->GetExtraData(pkEntry->GetKey());
                    if (pkExtra)
                    {
                        pkShaderExtraData->AddEntry(
                            pkEntry->GetShaderRegister(),0,
                            NiGPUProgram::PROGRAM_VERTEX, pkExtra, true);
                    }
                }
            }

            // Add per pass pixel shader constants.
            NiD3DShaderConstantMap* pkPixelMap = pkPass->GetPixelConstantMap();
            if (pkPixelMap)
            {
                uiCount = pkPixelMap->GetEntryCount();
                for (unsigned int ui = 0; ui < uiCount; ui++)
                {
                    NiShaderConstantMapEntry* pkEntry =
                        pkPixelMap->GetEntryAtIndex(ui);
                    if (pkEntry && pkEntry->IsAttribute())
                    {
                        NiExtraData* pkExtra =
                            pkGeometry->GetExtraData(pkEntry->GetKey());
                        if (pkExtra)
                        {
                            pkShaderExtraData->AddEntry(
                                pkEntry->GetShaderRegister(),
                                0, NiGPUProgram::PROGRAM_PIXEL,
                                pkExtra, false);
                        }
                    }
                }
            }

            // Add per pass vertex shader constants.
            NiD3DShaderConstantMap* pkVertexMap =
                pkPass->GetVertexConstantMap();
            if (pkVertexMap)
            {
                unsigned int uiCount = pkVertexMap->GetEntryCount();
                for (unsigned int ui = 0; ui < uiCount; ui++)
                {
                    NiShaderConstantMapEntry* pkEntry =
                        pkVertexMap->GetEntryAtIndex(ui);
                    if (pkEntry && pkEntry->IsAttribute())
                    {
                        NiExtraData* pkExtra =
                            pkGeometry->GetExtraData(pkEntry->GetKey());
                        if (pkExtra)
                        {
                            pkShaderExtraData->AddEntry(
                                pkEntry->GetShaderRegister(),
                                0, NiGPUProgram::PROGRAM_VERTEX,
                                pkExtra, false);
                        }
                    }
                }
            }
        }
    }

    // Attach the NiSCMExtraData object which holds our cached values.
    NiShaderConstantMap::SetSCMExtraData(pkGeometry, pkShaderExtraData);
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::PackWorldParallelEffect(const NiMatrix3& kWorldMat,
    const NiPoint3& kWorldTrans, NiD3DTextureStage* pkStage, bool bSave,
    NiD3DRenderer* pkD3DRenderer)
{
    pkStage->SetStageState(D3DTSS_TEXCOORDINDEX,
        D3DTSS_TCI_CAMERASPACEPOSITION, bSave);
    pkStage->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2,
        bSave);

    D3DMATRIX& kMat = pkStage->GetTextureTransformation();

    // kMat._14, kMat._24, kMat._34 and kMat._44 are always 0.0

    // cam matrix = kWorldMat * m_invView
    // D3DMatrices are transposed with respect to NiMatrix3.
    const D3DMATRIX& kInvMat = pkD3DRenderer->GetInvView();

    // cam matrix = kWorldMat * m_invView
    kMat._11 = kWorldMat.GetEntry(0,0) * kInvMat._11 +
        kWorldMat.GetEntry(0,1) * kInvMat._12 +
        kWorldMat.GetEntry(0,2) * kInvMat._13;
    kMat._21 = kWorldMat.GetEntry(0,0) * kInvMat._21 +
        kWorldMat.GetEntry(0,1) * kInvMat._22 +
        kWorldMat.GetEntry(0,2) * kInvMat._23;
    kMat._31 = kWorldMat.GetEntry(0,0) * kInvMat._31 +
        kWorldMat.GetEntry(0,1) * kInvMat._32 +
        kWorldMat.GetEntry(0,2) * kInvMat._33;
    kMat._12 = kWorldMat.GetEntry(1,0) * kInvMat._11 +
        kWorldMat.GetEntry(1,1) * kInvMat._12 +
        kWorldMat.GetEntry(1,2) * kInvMat._13;
    kMat._22 = kWorldMat.GetEntry(1,0) * kInvMat._21 +
        kWorldMat.GetEntry(1,1) * kInvMat._22 +
        kWorldMat.GetEntry(1,2) * kInvMat._23;
    kMat._32 = kWorldMat.GetEntry(1,0) * kInvMat._31 +
        kWorldMat.GetEntry(1,1) * kInvMat._32 +
        kWorldMat.GetEntry(1,2) * kInvMat._33;
    kMat._13 = kWorldMat.GetEntry(2,0) * kInvMat._11 +
        kWorldMat.GetEntry(2,1) * kInvMat._12 +
        kWorldMat.GetEntry(2,2) * kInvMat._13;
    kMat._23 = kWorldMat.GetEntry(2,0) * kInvMat._21 +
        kWorldMat.GetEntry(2,1) * kInvMat._22 +
        kWorldMat.GetEntry(2,2) * kInvMat._23;
    kMat._33 = kWorldMat.GetEntry(2,0) * kInvMat._31 +
        kWorldMat.GetEntry(2,1) * kInvMat._32 +
        kWorldMat.GetEntry(2,2) * kInvMat._33;

    // cam trans = kWorldMat * invViewTrans + kWorldTrans
    kMat._41 = kWorldMat.GetEntry(0,0) * kInvMat._41 +
        kWorldMat.GetEntry(0,1) * kInvMat._42 +
        kWorldMat.GetEntry(0,2) * kInvMat._43 + kWorldTrans.x;
    kMat._42 = kWorldMat.GetEntry(1,0) * kInvMat._41 +
        kWorldMat.GetEntry(1,1) * kInvMat._42 +
        kWorldMat.GetEntry(1,2) * kInvMat._43 + kWorldTrans.y;
    kMat._43 = kWorldMat.GetEntry(2,0) * kInvMat._41 +
        kWorldMat.GetEntry(2,1) * kInvMat._42 +
        kWorldMat.GetEntry(2,2) * kInvMat._43 + kWorldTrans.z;

    kMat._14 = kMat._24 = kMat._34 = kMat._44 = 0.0f;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::PackWorldPerspectiveEffect(const NiMatrix3& kWorldMat,
    const NiPoint3& kWorldTrans, NiD3DTextureStage* pkStage, bool bSave,
    NiD3DRenderer* pkD3DRenderer)
{
    pkStage->SetStageState(D3DTSS_TEXCOORDINDEX,
        D3DTSS_TCI_CAMERASPACEPOSITION, bSave);
    pkStage->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS,
        pkD3DRenderer->GetProjectedTextureFlags(), bSave);

    D3DMATRIX& kMat = pkStage->GetTextureTransformation();

    // kMat._14, kMat._24, kMat._34 and kMat._44 are always 0.0

    // cam matrix = kWorldMat * kInvMat
    // D3DMatrices are transposed with respect to NiMatrix3.
    const D3DMATRIX& kInvMat = pkD3DRenderer->GetInvView();

    kMat._11 = kWorldMat.GetEntry(0,0) * kInvMat._11 +
        kWorldMat.GetEntry(0,1) * kInvMat._12 +
        kWorldMat.GetEntry(0,2) * kInvMat._13;
    kMat._21 = kWorldMat.GetEntry(0,0) * kInvMat._21 +
        kWorldMat.GetEntry(0,1) * kInvMat._22 +
        kWorldMat.GetEntry(0,2) * kInvMat._23;
    kMat._31 = kWorldMat.GetEntry(0,0) * kInvMat._31 +
        kWorldMat.GetEntry(0,1) * kInvMat._32 +
        kWorldMat.GetEntry(0,2) * kInvMat._33;
    kMat._12 = kWorldMat.GetEntry(1,0) * kInvMat._11 +
        kWorldMat.GetEntry(1,1) * kInvMat._12 +
        kWorldMat.GetEntry(1,2) * kInvMat._13;
    kMat._22 = kWorldMat.GetEntry(1,0) * kInvMat._21 +
        kWorldMat.GetEntry(1,1) * kInvMat._22 +
        kWorldMat.GetEntry(1,2) * kInvMat._23;
    kMat._32 = kWorldMat.GetEntry(1,0) * kInvMat._31 +
        kWorldMat.GetEntry(1,1) * kInvMat._32 +
        kWorldMat.GetEntry(1,2) * kInvMat._33;
    kMat._13 = kWorldMat.GetEntry(2,0) * kInvMat._11 +
        kWorldMat.GetEntry(2,1) * kInvMat._12 +
        kWorldMat.GetEntry(2,2) * kInvMat._13;
    kMat._23 = kWorldMat.GetEntry(2,0) * kInvMat._21 +
        kWorldMat.GetEntry(2,1) * kInvMat._22 +
        kWorldMat.GetEntry(2,2) * kInvMat._23;
    kMat._33 = kWorldMat.GetEntry(2,0) * kInvMat._31 +
        kWorldMat.GetEntry(2,1) * kInvMat._32 +
        kWorldMat.GetEntry(2,2) * kInvMat._33;

    // cam trans = kWorldMat * pkInvViewTrans + kWorldTrans

    kMat._41 = kWorldMat.GetEntry(0,0) * kInvMat._41 +
        kWorldMat.GetEntry(0,1) * kInvMat._42 +
        kWorldMat.GetEntry(0,2) * kInvMat._43 + kWorldTrans.x;
    kMat._42 = kWorldMat.GetEntry(1,0) * kInvMat._41 +
        kWorldMat.GetEntry(1,1) * kInvMat._42 +
        kWorldMat.GetEntry(1,2) * kInvMat._43 + kWorldTrans.y;
    kMat._43 = kWorldMat.GetEntry(2,0) * kInvMat._41 +
        kWorldMat.GetEntry(2,1) * kInvMat._42 +
        kWorldMat.GetEntry(2,2) * kInvMat._43 + kWorldTrans.z;

    kMat._14 = kMat._24 = kMat._34 = kMat._44 = 0.0f;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::PackWorldSphereEffect(const NiMatrix3& kWorldMat,
    const NiPoint3& kWorldTrans, NiD3DTextureStage* pkStage, bool bSave,
    NiD3DRenderer* pkD3DRenderer)
{
    pkStage->SetStageState(D3DTSS_TEXCOORDINDEX,
        D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR, bSave);
    pkStage->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2,
        bSave);

    D3DMATRIX& kMat = pkStage->GetTextureTransformation();

    // kMat._14, kMat._24, kMat._34 and kMat._44 are always 0.0

    // cam matrix = kWorldMat * kInvMat
    // D3DMatrices are transposed with respect to NiMatrix3.
    const D3DMATRIX& kInvMat = pkD3DRenderer->GetInvView();

    kMat._11 = kWorldMat.GetEntry(0,0) * kInvMat._11 +
        kWorldMat.GetEntry(0,1) * kInvMat._12 +
        kWorldMat.GetEntry(0,2) * kInvMat._13;
    kMat._21 = kWorldMat.GetEntry(0,0) * kInvMat._21 +
        kWorldMat.GetEntry(0,1) * kInvMat._22 +
        kWorldMat.GetEntry(0,2) * kInvMat._23;
    kMat._31 = kWorldMat.GetEntry(0,0) * kInvMat._31 +
        kWorldMat.GetEntry(0,1) * kInvMat._32 +
        kWorldMat.GetEntry(0,2) * kInvMat._33;
    kMat._12 = kWorldMat.GetEntry(1,0) * kInvMat._11 +
        kWorldMat.GetEntry(1,1) * kInvMat._12 +
        kWorldMat.GetEntry(1,2) * kInvMat._13;
    kMat._22 = kWorldMat.GetEntry(1,0) * kInvMat._21 +
        kWorldMat.GetEntry(1,1) * kInvMat._22 +
        kWorldMat.GetEntry(1,2) * kInvMat._23;
    kMat._32 = kWorldMat.GetEntry(1,0) * kInvMat._31 +
        kWorldMat.GetEntry(1,1) * kInvMat._32 +
        kWorldMat.GetEntry(1,2) * kInvMat._33;
    kMat._13 = kWorldMat.GetEntry(2,0) * kInvMat._11 +
        kWorldMat.GetEntry(2,1) * kInvMat._12 +
        kWorldMat.GetEntry(2,2) * kInvMat._13;
    kMat._23 = kWorldMat.GetEntry(2,0) * kInvMat._21 +
        kWorldMat.GetEntry(2,1) * kInvMat._22 +
        kWorldMat.GetEntry(2,2) * kInvMat._23;
    kMat._33 = kWorldMat.GetEntry(2,0) * kInvMat._31 +
        kWorldMat.GetEntry(2,1) * kInvMat._32 +
        kWorldMat.GetEntry(2,2) * kInvMat._33;

    // cam trans is used directly

    kMat._41 = kWorldTrans.x;
    kMat._42 = kWorldTrans.y;
    kMat._43 = kWorldTrans.z;

    kMat._14 = kMat._24 = kMat._34 = kMat._44 = 0.0f;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::PackCameraSphereEffect(const NiMatrix3&,
    const NiPoint3&, NiD3DTextureStage* pkStage, bool bSave,
    NiD3DRenderer*)
{
    pkStage->SetStageState(D3DTSS_TEXCOORDINDEX,
        D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR, bSave);
    pkStage->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2,
        bSave);

    D3DMATRIX& kMat = pkStage->GetTextureTransformation();

    // This is always the same matrix in camera space
    kMat._11 = kMat._12 = kMat._13 = kMat._21 = kMat._23
        = kMat._32 = kMat._33 = 0.0f;
    kMat._22 = 0.5f;
    kMat._31 = 0.5f;

    kMat._41 = 0.5f;
    kMat._42 = 0.5f;
    kMat._43 = 0.0f;

    kMat._14 = kMat._24 = kMat._34 = kMat._44 = 0.0f;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::PackSpecularCubeEffect(const NiMatrix3& kWorldMat,
    const NiPoint3& kWorldTrans, NiD3DTextureStage* pkStage, bool bSave,
    NiD3DRenderer* pkD3DRenderer)
{
    pkStage->SetStageState(D3DTSS_TEXCOORDINDEX,
        D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR, bSave);
    pkStage->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3,
        bSave);

    D3DMATRIX& kMat = pkStage->GetTextureTransformation();

    // kMat._14, kMat._24, kMat._34 and kMat._44 are always 0.0

    // cam matrix = kWorldMat * kInvMat
    // D3DMatrices are transposed with respect to NiMatrix3.
    const D3DMATRIX& kInvMat = pkD3DRenderer->GetInvView();

    kMat._11 = kWorldMat.GetEntry(0,0) * kInvMat._11 +
        kWorldMat.GetEntry(0,1) * kInvMat._12 +
        kWorldMat.GetEntry(0,2) * kInvMat._13;
    kMat._21 = kWorldMat.GetEntry(0,0) * kInvMat._21 +
        kWorldMat.GetEntry(0,1) * kInvMat._22 +
        kWorldMat.GetEntry(0,2) * kInvMat._23;
    kMat._31 = kWorldMat.GetEntry(0,0) * kInvMat._31 +
        kWorldMat.GetEntry(0,1) * kInvMat._32 +
        kWorldMat.GetEntry(0,2) * kInvMat._33;
    kMat._12 = kWorldMat.GetEntry(1,0) * kInvMat._11 +
        kWorldMat.GetEntry(1,1) * kInvMat._12 +
        kWorldMat.GetEntry(1,2) * kInvMat._13;
    kMat._22 = kWorldMat.GetEntry(1,0) * kInvMat._21 +
        kWorldMat.GetEntry(1,1) * kInvMat._22 +
        kWorldMat.GetEntry(1,2) * kInvMat._23;
    kMat._32 = kWorldMat.GetEntry(1,0) * kInvMat._31 +
        kWorldMat.GetEntry(1,1) * kInvMat._32 +
        kWorldMat.GetEntry(1,2) * kInvMat._33;
    kMat._13 = kWorldMat.GetEntry(2,0) * kInvMat._11 +
        kWorldMat.GetEntry(2,1) * kInvMat._12 +
        kWorldMat.GetEntry(2,2) * kInvMat._13;
    kMat._23 = kWorldMat.GetEntry(2,0) * kInvMat._21 +
        kWorldMat.GetEntry(2,1) * kInvMat._22 +
        kWorldMat.GetEntry(2,2) * kInvMat._23;
    kMat._33 = kWorldMat.GetEntry(2,0) * kInvMat._31 +
        kWorldMat.GetEntry(2,1) * kInvMat._32 +
        kWorldMat.GetEntry(2,2) * kInvMat._33;

    // cam trans is used directly

    kMat._41 = kWorldTrans.x;
    kMat._42 = kWorldTrans.y;
    kMat._43 = kWorldTrans.z;

    kMat._14 = kMat._24 = kMat._34 = kMat._44 = 0.0f;
}

//------------------------------------------------------------------------------------------------
void NiD3DShader::PackDiffuseCubeEffect(const NiMatrix3& kWorldMat,
    const NiPoint3& kWorldTrans, NiD3DTextureStage* pkStage, bool bSave,
    NiD3DRenderer* pkD3DRenderer)
{
    pkStage->SetStageState(D3DTSS_TEXCOORDINDEX,
        D3DTSS_TCI_CAMERASPACENORMAL, bSave);
    pkStage->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3,
        bSave);

    D3DMATRIX& kMat = pkStage->GetTextureTransformation();

    // kMat._14, kMat._24, kMat._34 and kMat._44 are always 0.0

    // cam matrix = kWorldMat * kInvMat
    // D3DMatrices are transposed with respect to NiMatrix3.
    const D3DMATRIX& kInvMat = pkD3DRenderer->GetInvView();

    kMat._11 = kWorldMat.GetEntry(0,0) * kInvMat._11 +
        kWorldMat.GetEntry(0,1) * kInvMat._12 +
        kWorldMat.GetEntry(0,2) * kInvMat._13;
    kMat._21 = kWorldMat.GetEntry(0,0) * kInvMat._21 +
        kWorldMat.GetEntry(0,1) * kInvMat._22 +
        kWorldMat.GetEntry(0,2) * kInvMat._23;
    kMat._31 = kWorldMat.GetEntry(0,0) * kInvMat._31 +
        kWorldMat.GetEntry(0,1) * kInvMat._32 +
        kWorldMat.GetEntry(0,2) * kInvMat._33;
    kMat._12 = kWorldMat.GetEntry(1,0) * kInvMat._11 +
        kWorldMat.GetEntry(1,1) * kInvMat._12 +
        kWorldMat.GetEntry(1,2) * kInvMat._13;
    kMat._22 = kWorldMat.GetEntry(1,0) * kInvMat._21 +
        kWorldMat.GetEntry(1,1) * kInvMat._22 +
        kWorldMat.GetEntry(1,2) * kInvMat._23;
    kMat._32 = kWorldMat.GetEntry(1,0) * kInvMat._31 +
        kWorldMat.GetEntry(1,1) * kInvMat._32 +
        kWorldMat.GetEntry(1,2) * kInvMat._33;
    kMat._13 = kWorldMat.GetEntry(2,0) * kInvMat._11 +
        kWorldMat.GetEntry(2,1) * kInvMat._12 +
        kWorldMat.GetEntry(2,2) * kInvMat._13;
    kMat._23 = kWorldMat.GetEntry(2,0) * kInvMat._21 +
        kWorldMat.GetEntry(2,1) * kInvMat._22 +
        kWorldMat.GetEntry(2,2) * kInvMat._23;
    kMat._33 = kWorldMat.GetEntry(2,0) * kInvMat._31 +
        kWorldMat.GetEntry(2,1) * kInvMat._32 +
        kWorldMat.GetEntry(2,2) * kInvMat._33;

    // cam trans is used directly

    kMat._41 = kWorldTrans.x;
    kMat._42 = kWorldTrans.y;
    kMat._43 = kWorldTrans.z;

    kMat._14 = kMat._24 = kMat._34 = kMat._44 = 0.0f;
}

//------------------------------------------------------------------------------------------------
