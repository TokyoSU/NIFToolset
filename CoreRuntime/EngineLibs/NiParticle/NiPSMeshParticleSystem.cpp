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
#include "NiParticlePCH.h"

#include "NiPSMeshParticleSystem.h"
#include "NiPSSimulatorGeneralStep.h"
#include "NiPSSimulatorForcesStep.h"
#include "NiPSSimulatorCollidersStep.h"
#include "NiPSSimulatorFinalStep.h"
#include "NiPSSimulatorMeshAlignStep.h"
#include "NiPSFlagsHelpers.h"
#include "NiPSCommonSemantics.h"

#include <NiMeshUpdateProcess.h>

NiImplementRTTI(NiPSMeshParticleSystem, NiPSParticleSystem, NiTypeMask::NiPSMeshParticleSystem);

//--------------------------------------------------------------------------------------------------
NiPSMeshParticleSystem* NiPSMeshParticleSystem::Create(
    const NiUInt32 uiMaxNumParticles,
    const AlignMethod eNormalMethod,
    const NiPoint3& kNormalDirection,
    const AlignMethod eUpMethod,
    const NiPoint3& kUpDirection,
    const bool bHasLivingSpawner,
    const bool bHasColors,
    const bool bHasRotations,
    const bool bWorldSpace,
    const bool bDynamicBounds,
    const bool bAttachMeshModifiers,
    const NiUInt16 usNumGenerations,
    const NiUInt32 uiPoolSize,
    const bool bAutoFillPools,
    const float fScaleAmount,
    const float fScaleRest,
    const float fScaleLimit)
{
    // Create simulator.
    NiPSSimulator* pkSimulator = NiNew NiPSSimulator();

    // Add simulation steps.
    pkSimulator->AddStep(NiNew NiPSSimulatorGeneralStep());
    pkSimulator->AddStep(NiNew NiPSSimulatorForcesStep());
    pkSimulator->AddStep(NiNew NiPSSimulatorCollidersStep());
    pkSimulator->AddStep(NiNew NiPSSimulatorFinalStep());
    pkSimulator->AddStep(NiNew NiPSSimulatorMeshAlignStep(fScaleAmount, fScaleRest, fScaleLimit));

    // Create bound updater, if requested.
    NiPSBoundUpdater* pkBoundUpdater = NULL;
    if (bDynamicBounds)
    {
        pkBoundUpdater = NiNew NiPSBoundUpdater();
    }

    // Create particle system.
    NiPSMeshParticleSystem* pkSystem = NiNew NiPSMeshParticleSystem(
        pkSimulator,
        uiMaxNumParticles,
        eNormalMethod,
        kNormalDirection,
        eUpMethod,
        kUpDirection,
        bHasLivingSpawner,
        bHasColors,
        bHasRotations,
        bHasRotations,
        bWorldSpace,
        pkBoundUpdater,
        usNumGenerations,
        uiPoolSize,
        bAutoFillPools);

    // Add simulator.
    pkSystem->AddModifier(pkSimulator, bAttachMeshModifiers);

    return pkSystem;
}

//--------------------------------------------------------------------------------------------------
NiPSMeshParticleSystem::NiPSMeshParticleSystem(
    NiPSSimulator* pkSimulator,
    const NiUInt32 uiMaxNumParticles,
    const AlignMethod eNormalMethod,
    const NiPoint3& kNormalDirection,
    const AlignMethod eUpMethod,
    const NiPoint3& kUpDirection,
    const bool bHasLivingSpawner,
    const bool bHasColors,
    const bool bHasRotations,
    const bool bHasRotationAxes,
    const bool bWorldSpace,
    NiPSBoundUpdater* pkBoundUpdater,
    const NiUInt16 usNumGenerations,
    const NiUInt32 uiPoolSize,
    const bool bAutoFillPools) :
    NiPSParticleSystem(
        pkSimulator,
        uiMaxNumParticles,
        eNormalMethod,
        kNormalDirection,
        eUpMethod,
        kUpDirection,
        bHasLivingSpawner,
        bHasColors,
        bHasRotations,
        bHasRotationAxes,
        false,
        bWorldSpace,
        false,
        pkBoundUpdater),
    m_kPools(usNumGenerations),
    m_kMasterParticles(usNumGenerations),
    m_uiPoolSize(uiPoolSize),
    m_bAutoFillPools(bAutoFillPools)
{
    // Create the particle container node.
    m_spParticleContainer = NiNew NiNode(m_uiMaxNumParticles);

    // Allocate the transformations array.
    m_pkRotations = NiAlignedAlloc(NiPoint3, m_uiMaxNumParticles * 3, NIPSKERNEL_ALIGNMENT);
    m_pfScales = NiAlignedAlloc(float, m_uiMaxNumParticles, NIPSKERNEL_ALIGNMENT);

    // Set the pool size.
    if (m_uiPoolSize == (NiUInt32) DEFAULT_POOL_SIZE)
    {
        m_uiPoolSize = m_uiMaxNumParticles;
    }

    // Create the individual pools for each generation.
    for (NiUInt16 us = 0; us < usNumGenerations; ++us)
    {
        m_kPools.SetAt(us, NiNew NiAVObjectArray(m_uiPoolSize));
    }
}

//--------------------------------------------------------------------------------------------------
NiPSMeshParticleSystem::NiPSMeshParticleSystem() :
    m_pkRotations(NULL),
    m_pfScales(NULL),
    m_uiPoolSize(0),
    m_bAutoFillPools(false)
{
}

//--------------------------------------------------------------------------------------------------
NiPSMeshParticleSystem::~NiPSMeshParticleSystem()
{
    // Delete the transforms
    NiAlignedFree(m_pkRotations);
    NiAlignedFree(m_pfScales);

    // Delete each pool.
    const NiUInt32 uiPoolsCount = m_kPools.GetSize();
    for (NiUInt32 ui = 0; ui < uiPoolsCount; ++ui)
    {
        NiDelete m_kPools.GetAt(ui);
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::ResetParticleSystem(const float fNewLastTime)
{
    // Completes tasks.
    m_pkSimulator->ResetSimulator(this);

    // Remove each particle, adding the removed particle back to the pool for
    // its generation.
    for (NiUInt32 uiParticle = 0; uiParticle < m_uiNumParticles; ++uiParticle)
    {
        NiAVObjectPtr spRemovedParticle = m_spParticleContainer->DetachChildAt(
            uiParticle);

        NiUInt16 usGeneration = NiPSFlagsHelpers::GetGeneration(
            m_puiFlags[uiParticle]);
        if (usGeneration >= GetNumGenerations())
        {
            usGeneration = GetNumGenerations() - 1;
        }
        if (m_kPools.GetAt(usGeneration)->GetSize() <
            m_kPools.GetAt(usGeneration)->GetAllocatedSize())
        {
            m_kPools.GetAt(usGeneration)->Add(spRemovedParticle);
        }
    }

    // Call the base class version of this function to actually reset the
    // particle counts.
    NiPSParticleSystem::ResetParticleSystem(fNewLastTime);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::FillPools()
{
    const NiUInt32 uiPoolsCount = m_kPools.GetSize();
    for (NiUInt32 ui = 0; ui < uiPoolsCount; ++ui)
    {
        // A master particle must be specified for each generation prior to
        // this function being called.
        NiAVObject* pkMasterParticle = m_kMasterParticles.GetAt(ui);
        EE_ASSERT(pkMasterParticle);

        NiAVObjectArray* pkPool = m_kPools.GetAt(ui);
        EE_ASSERT(pkPool);

        const NiUInt32 uiPoolSize = pkPool->GetAllocatedSize();
        for (NiUInt32 uj = 0; uj < uiPoolSize; ++uj)
        {
            if (!pkPool->GetAt(uj))
            {
                pkPool->SetAt(uj, (NiAVObject*) pkMasterParticle->Clone());
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::InitializeParticle(NiUInt32 uiNewParticle)
{
    // Get particle generation.
    NiUInt16 usPoolGeneration = 0;
    NiUInt32 uiFlags = m_puiFlags[uiNewParticle];
    if (!uiFlags && !m_bPreRPIParticleSystem)
    {
        // Randomly select the first generation (mesh) to be emitted
        NiUInt16 usGeneration = (NiUInt16)NiRand() % GetNumGenerations();
        NiPSFlagsHelpers::SetGeneration(m_puiFlags[uiNewParticle], usGeneration);
        usPoolGeneration = usGeneration;
    }
    else
    {
        usPoolGeneration = NiPSFlagsHelpers::GetGeneration(uiFlags);
        if (usPoolGeneration >= GetNumGenerations())
        {
            usPoolGeneration = GetNumGenerations() - 1;
        }
    }

    // Try to pull from the pool.
    NiAVObjectPtr spClonedParticle =
        m_kPools.GetAt(usPoolGeneration)->RemoveEnd();
    if (!spClonedParticle)
    {
        // Get master particle. It is a configuration error if a master
        // particle does not exist for this generation.
        NiAVObject* pkMasterParticle =
            m_kMasterParticles.GetAt(usPoolGeneration);
        EE_ASSERT(pkMasterParticle);

        // If no particles in pool, clone one.
        spClonedParticle = (NiAVObject*) pkMasterParticle->Clone();
    }

    // Initialize particle and add to container.
    spClonedParticle->SetAppCulled(true);
    NiTimeController::StartAnimations(spClonedParticle, 0.0f);
    m_spParticleContainer->SetAt(uiNewParticle, spClonedParticle);
    spClonedParticle->UpdatePropertiesDownward(m_spPropertyState);
    spClonedParticle->UpdateEffectsDownward(m_spEffectState);

    // Set the size correctly. The emitter does not have enough information to
    // set this and it must be set before the first particle mesh update.
    NiPSSimulatorGeneralStep* pkGeneralStep = (NiPSSimulatorGeneralStep*)
        m_pkSimulator->GetSimulatorStepByType(
        &NiPSSimulatorGeneralStep::ms_RTTI);
    if (pkGeneralStep)
    {
        pkGeneralStep->InitializeNewParticle(this, uiNewParticle);
    }

    /// Set scale and rotation so they are correct for first update. Always prime the
    /// rotation because, if mesh alignment is used, the pre-existing rotation values
    /// may be needed.
    NiPSSimulatorMeshAlignStep* pkMeshAlignStep = (NiPSSimulatorMeshAlignStep*)
        m_pkSimulator->GetSimulatorStepByType(&NiPSSimulatorMeshAlignStep::ms_RTTI);
    spClonedParticle->SetRotate(NiMatrix3::IDENTITY);

    if (pkMeshAlignStep)
    {
        pkMeshAlignStep->InitializeNewParticle(this, uiNewParticle);
    }
    else
    {
        m_pkRotations[uiNewParticle * 3] = NiPoint3::UNIT_X;
        m_pkRotations[uiNewParticle * 3 + 1] = NiPoint3::UNIT_Y;
        m_pkRotations[uiNewParticle * 3 + 2] = NiPoint3::UNIT_Z;
        m_pfScales[uiNewParticle] = 1.0f;
    }
    
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::InitializeSpawnedParticle(NiUInt32 uiNewParticle)
{
    InitializeParticle(uiNewParticle);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::RemoveParticle(NiUInt32 uiIndexToRemove)
{
    NiUInt32 uiFinalIndex = m_uiNumParticles - 1;

    // Get particle generation.
    NiUInt32 uiFlags = m_puiFlags[uiIndexToRemove];
    NiUInt16 usGeneration = NiPSFlagsHelpers::GetGeneration(uiFlags);

    // Copy the transform data.
    m_pkRotations[uiIndexToRemove * 3] = m_pkRotations[uiFinalIndex * 3];
    m_pkRotations[uiIndexToRemove * 3 + 1] = m_pkRotations[uiFinalIndex * 3 + 1];
    m_pkRotations[uiIndexToRemove * 3 + 2] = m_pkRotations[uiFinalIndex * 3 + 2];
    m_pfScales[uiIndexToRemove] = m_pfScales[uiFinalIndex];

    NiPSParticleSystem::RemoveParticle(uiIndexToRemove);

    NiAVObjectPtr spRemovedParticle;
    if (uiIndexToRemove == uiFinalIndex)
    {
        spRemovedParticle = m_spParticleContainer->DetachChildAt(
            uiFinalIndex);
    }
    else
    {
        NiAVObjectPtr spLastParticle = m_spParticleContainer->DetachChildAt(
            uiFinalIndex);
        spRemovedParticle = m_spParticleContainer->DetachChildAt(
            uiIndexToRemove);
        m_spParticleContainer->SetAt(uiIndexToRemove, spLastParticle);
    }

    // We can be removing a NULL mesh when called from ResolveAddedParticles.
    if (!spRemovedParticle)
        return;

    // Add the removed particle back to the pool. It will be deleted if the
    // pool is full.
    if (usGeneration >= GetNumGenerations())
    {
        usGeneration = GetNumGenerations() - 1;
    }
    if (m_kPools.GetAt(usGeneration)->GetSize() <
        m_kPools.GetAt(usGeneration)->GetAllocatedSize())
    {
        m_kPools.GetAt(usGeneration)->Add(spRemovedParticle);
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::ResolveAddedParticles(const bool bUpdateBound)
{
    for (NiUInt32 ui = m_uiAddedParticlesBase;
        ui < m_uiAddedParticlesBase + m_uiNumAddedParticles; ++ui)
    {
        NiAVObject* pkParticle = m_spParticleContainer->GetAt(ui);
        if (pkParticle)
        {
            pkParticle->SetAppCulled(false);
        }
    }

    NiPSParticleSystem::ResolveAddedParticles(bUpdateBound);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::PostUpdate(NiUpdateProcess& kUpdate)
{
    NiPSParticleSystem::PostUpdate(kUpdate);

    if (m_uiNumParticles == 0)
    {
        return;
    }

    float fCurrentTime = kUpdate.GetTime();

    for (NiUInt32 ui = 0; ui < m_uiNumParticles; ++ui)
    {
        // Get particle.
        NiAVObject* pkParticle = m_spParticleContainer->GetAt(ui);
        EE_ASSERT(pkParticle);

        // Update the position and scale of the particle.
        pkParticle->SetTranslate(m_pkPositions[ui]);
        pkParticle->SetScale(m_pfScales[ui]);

        // Rotation must check for cases in which the alignment step could not
        // resolve the orientation. Such cases are marked by some zero columns in
        // the rotation matrix.
        NiMatrix3 kRotate(
            m_pkRotations[3 * ui],
            m_pkRotations[3 * ui + 1],
            m_pkRotations[3 * ui + 2]);
        if (m_pkRotations[3 * ui].x == 0.0f &&
            m_pkRotations[3 * ui].y == 0.0f &&
            m_pkRotations[3 * ui].z == 0.0f)
        {
            // We expect non-zero in the third column
            NiPoint3 kExistingNormal;
            pkParticle->GetRotate().GetCol(0, kExistingNormal);
            NiPoint3 kRight = m_pkRotations[3 * ui + 2].UnitCross(kExistingNormal);
            NiPoint3 kUp = kExistingNormal.Cross(kRight);
            kRotate.SetCol(0, kExistingNormal);
            kRotate.SetCol(1, kRight);
            kRotate.SetCol(2, kUp);
        }
        else if (m_pkRotations[3 * ui + 2].x == 0.0f &&
            m_pkRotations[3 * ui + 2].y == 0.0f &&
            m_pkRotations[3 * ui + 2].z == 0.0f)
        {
            // We expect non-zero in the first column
            NiPoint3 kExistingUp;
            pkParticle->GetRotate().GetCol(2, kExistingUp);
            NiPoint3 kRight = kExistingUp.UnitCross(m_pkRotations[3 * ui]);
            NiPoint3 kUp = m_pkRotations[3 * ui].Cross(kRight);
            kRotate.SetCol(1, kRight);
            kRotate.SetCol(2, kUp);
        }
        pkParticle->SetRotate(kRotate);

        // Update the particle with its age.
        kUpdate.SetTime(m_pfAges[ui]);
        pkParticle->UpdateDownwardPass(kUpdate);
    }

    // Set the time back to current
    kUpdate.SetTime(fCurrentTime);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::UpdatePropertiesDownward(
    NiPropertyState* pkParentState)
{
    NiPSParticleSystem::UpdatePropertiesDownward(pkParentState);

    m_spParticleContainer->UpdatePropertiesDownward(m_spPropertyState);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::UpdateEffectsDownward(
    NiDynamicEffectState* pkParentState)
{
    NiPSParticleSystem::UpdateEffectsDownward(pkParentState);

    m_spParticleContainer->UpdateEffectsDownward(m_spEffectState);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::OnVisible(NiCullingProcess& kCuller)
{
    NiPSParticleSystem::OnVisible(kCuller);

    m_spParticleContainer->OnVisible(kCuller);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::SetSelectiveUpdateFlags(
    bool& bSelectiveUpdate,
    bool bSelectiveUpdateTransforms,
    bool& bRigid)
{
    NiPSParticleSystem::SetSelectiveUpdateFlags(bSelectiveUpdate,
        bSelectiveUpdateTransforms, bRigid);

    m_spParticleContainer->SetSelectiveUpdateFlags(bSelectiveUpdate, true,
        bRigid);
    bRigid = false;
    m_spParticleContainer->SetSelectiveUpdateRigid(bRigid);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::RenderImmediate(NiRenderer*)
{
    // Since this object does not contain any renderable geometry, we will
    // not call RenderMesh here. RenderImmediate will be called separately on
    // all active particles, so they do not need to be handled here.

    // The mesh modifiers should be completed no matter what.
    NiSyncArgs kSyncArgs;
    kSyncArgs.m_uiSubmitPoint = NiSyncArgs::SYNC_ANY;
    kSyncArgs.m_uiCompletePoint = NiSyncArgs::SYNC_RENDER;
    CompleteModifiers(&kSyncArgs);

    if (m_uiNumParticles > 0)
    {
        // Only call OnPreDisplay for time controllers if some particles exist.
        NiTimeController::OnPreDisplayIterate(GetControllers());
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::UpdateWorldData()
{
    NiPSParticleSystem::UpdateWorldData();

    m_spParticleContainer->SetRotate(m_kWorld.m_Rotate);
    m_spParticleContainer->SetTranslate(m_kWorld.m_Translate);
    m_spParticleContainer->SetScale(m_kWorld.m_fScale);
    m_spParticleContainer->UpdateWorldData();
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::UpdateWorldBound()
{
    // Here the local bound represents the full world bound, so we direclty assign
    // the local bound to the world bound.
    m_kWorldBound = m_kBound;
    m_spParticleContainer->SetWorldBound(m_kWorldBound);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::PurgeRendererData(NiRenderer* pkRenderer)
{
    NiPSParticleSystem::PurgeRendererData(pkRenderer);

    // Purge particles.
    m_spParticleContainer->PurgeRendererData(pkRenderer);

    // Purge pools.
    NiUInt32 uiPoolsCount = m_kPools.GetSize();
    for (NiUInt32 ui = 0; ui < uiPoolsCount; ++ui)
    {
        NiAVObjectArray* pkPool = m_kPools.GetAt(ui);

        const NiUInt32 uiPoolSize = pkPool->GetSize();
        for (NiUInt32 uj = 0; uj < uiPoolSize; ++uj)
        {
            if (pkPool->GetAt(uj))
                pkPool->GetAt(uj)->PurgeRendererData(pkRenderer);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::RetrieveMeshSet(
    NiTPrimitiveSet<NiMesh*>& kMeshSet)
{
    NiPSParticleSystem::RetrieveMeshSet(kMeshSet);

    // Process master particles.
    const NiUInt32 uiMasterParticlesCount = m_kMasterParticles.GetSize();
    for (NiUInt32 ui = 0; ui < uiMasterParticlesCount; ++ui)
    {
        NiAVObject* pkMasterParticle = m_kMasterParticles.GetAt(ui);
        if (pkMasterParticle)
        {
            RecursiveRetrieveMeshSet(pkMasterParticle, kMeshSet);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::RecursiveRetrieveMeshSet(
    NiAVObject* pkObject,
    NiTPrimitiveSet<NiMesh*>& kMeshSet)
{
    if (NiIsKindOf(NiMesh, pkObject))
    {
        ((NiMesh*) pkObject)->RetrieveMeshSet(kMeshSet);
    }
    else if (NiIsKindOf(NiNode, pkObject))
    {
        NiNode* pkNode = (NiNode*) pkObject;
        const NiUInt32 uiChildCount = pkNode->GetArrayCount();
        for (NiUInt32 ui = 0; ui < uiChildCount; ++ui)
        {
            NiAVObject* pkChild = pkNode->GetAt(ui);
            if (pkChild)
            {
                RecursiveRetrieveMeshSet(pkChild, kMeshSet);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSMeshParticleSystem);

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::CopyMembers(
    NiPSMeshParticleSystem* pkDest,
    NiCloningProcess& kCloning)
{
    NiPSParticleSystem::CopyMembers(pkDest, kCloning);

    const NiUInt32 uiMasterParticlesCount = m_kMasterParticles.GetSize();
    pkDest->m_kMasterParticles.SetSize(uiMasterParticlesCount);
    for (NiUInt32 ui = 0; ui < uiMasterParticlesCount; ++ui)
    {
        // It is a configuration error if a master particle does not exist for
        // all generations.
        NiAVObject* pkMasterParticle = m_kMasterParticles.GetAt(ui);
        EE_ASSERT(pkMasterParticle);

        pkDest->m_kMasterParticles.SetAt(ui, (NiAVObject*)
            pkMasterParticle->CreateClone(kCloning));
    }

    // Allocate the space for transforms
    pkDest->m_pkRotations = NiAlignedAlloc(NiPoint3, m_uiMaxNumParticles * 3, NIPSKERNEL_ALIGNMENT);
    pkDest->m_pfScales = NiAlignedAlloc(float, m_uiMaxNumParticles, NIPSKERNEL_ALIGNMENT);
    NiMemcpy(pkDest->m_pkRotations, m_uiMaxNumParticles * 3 * sizeof(NiPoint3),
            m_pkRotations, m_uiNumParticles * sizeof(NiPoint3));
    NiMemcpy(pkDest->m_pfScales, m_uiMaxNumParticles * sizeof(float),
            m_pfScales, m_uiNumParticles * sizeof(float));

    pkDest->m_uiPoolSize = m_uiPoolSize;
    pkDest->m_bAutoFillPools = m_bAutoFillPools;

    // Create pools.
    const NiUInt32 uiPoolsCount = m_kPools.GetSize();
    pkDest->m_kPools.SetSize(uiPoolsCount);
    for (NiUInt32 ui = 0; ui < uiPoolsCount; ++ui)
    {
        NiAVObjectArray* pkOldPool = m_kPools.GetAt(ui);
        EE_ASSERT(pkOldPool);

        const NiUInt32 uiPoolSize = pkOldPool->GetAllocatedSize();
        NiAVObjectArray* pkNewPool = NiNew NiAVObjectArray(uiPoolSize);
        pkDest->m_kPools.SetAt(ui, pkNewPool);

        if (m_bAutoFillPools)
        {
            for (NiUInt32 uj = 0; uj < uiPoolSize; ++uj)
            {
                if (pkOldPool->GetAt(uj))
                {
                    pkNewPool->SetAt(uj, (NiAVObject*) pkOldPool->GetAt(uj)
                        ->CreateClone(kCloning));
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::ProcessClone(NiCloningProcess& kCloning)
{
    NiPSParticleSystem::ProcessClone(kCloning);

    NiObject* pkObject = NULL;
    EE_VERIFY(kCloning.m_pkCloneMap->GetAt(this, pkObject));
    NiPSMeshParticleSystem* pkDest = (NiPSMeshParticleSystem*) pkObject;

    // Create the particle container node.
    pkDest->m_spParticleContainer = NiNew NiNode(m_uiMaxNumParticles);

    const NiUInt32 uiMasterParticlesCount = m_kMasterParticles.GetSize();
    for (NiUInt32 ui = 0; ui < uiMasterParticlesCount; ++ui)
    {
        // It is a configuration error if a master particle does not exist for
        // all generations.
        NiAVObject* pkMasterParticle = m_kMasterParticles.GetAt(ui);
        EE_ASSERT(pkMasterParticle);

        pkMasterParticle->ProcessClone(kCloning);
    }

    // Auto-fill pools.
    if (m_bAutoFillPools)
    {
        const NiUInt32 uiPoolsCount = m_kPools.GetSize();
        for (NiUInt32 ui = 0; ui < uiPoolsCount; ++ui)
        {
            NiAVObjectArray* pkPool = m_kPools.GetAt(ui);
            EE_ASSERT(pkPool);

            const NiUInt32 uiPoolSize = pkPool->GetAllocatedSize();
            for (NiUInt32 uj = 0; uj < uiPoolSize; ++uj)
            {
                if (pkPool->GetAt(uj))
                {
                    pkPool->GetAt(uj)->ProcessClone(kCloning);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSMeshParticleSystem);

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::LoadBinary(NiStream& kStream)
{
    NiPSParticleSystem::LoadBinary(kStream);

    NiUInt32 uiNumGenerations;
    NiStreamLoadBinary(kStream, uiNumGenerations);
    m_kMasterParticles.SetSize(uiNumGenerations);
    for (NiUInt32 ui = 0; ui < uiNumGenerations; ++ui)
    {
        m_kMasterParticles.SetAt(ui, (NiAVObject*) kStream.ResolveLinkID());
    }

    NiStreamLoadBinary(kStream, m_uiPoolSize);

    NiBool bValue;
    NiStreamLoadBinary(kStream, bValue);
    m_bAutoFillPools = NIBOOL_IS_TRUE(bValue);

    // Create the individual pools for each generation.
    m_kPools.SetSize(uiNumGenerations);
    for (NiUInt32 ui = 0; ui < uiNumGenerations; ++ui)
    {
        m_kPools.SetAt(ui, NiNew NiAVObjectArray(m_uiPoolSize));
    }

    // Allocate the space for transforms
    m_pkRotations = NiAlignedAlloc(NiPoint3, m_uiMaxNumParticles * 3, NIPSKERNEL_ALIGNMENT);
    m_pfScales = NiAlignedAlloc(float, m_uiMaxNumParticles, NIPSKERNEL_ALIGNMENT);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::LinkObject(NiStream& kStream)
{
    NiPSParticleSystem::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::PostLinkObject(NiStream& kStream)
{
    NiPSParticleSystem::PostLinkObject(kStream);

    m_spParticleContainer = NiNew NiNode(m_uiMaxNumParticles);

    // Auto-fill pools.
    if (m_bAutoFillPools)
    {
        FillPools();
    }
}

//--------------------------------------------------------------------------------------------------
bool NiPSMeshParticleSystem::RegisterStreamables(NiStream& kStream)
{
    if (!NiPSParticleSystem::RegisterStreamables(kStream))
    {
        return false;
    }

    const NiUInt32 uiMasterParticlesCount = m_kMasterParticles.GetSize();
    for (NiUInt32 ui = 0; ui < uiMasterParticlesCount; ++ui)
    {
        // It is a configuration error if a master particle does not exist for
        // all generations.
        NiAVObject* pkMasterParticle = m_kMasterParticles.GetAt(ui);
        EE_ASSERT(pkMasterParticle);

        pkMasterParticle->RegisterStreamables(kStream);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::SaveBinary(NiStream& kStream)
{
    NiPSParticleSystem::SaveBinary(kStream);

    const NiUInt32 uiNumGenerations = m_kMasterParticles.GetSize();
    NiStreamSaveBinary(kStream, uiNumGenerations);
    for (NiUInt32 ui = 0; ui < uiNumGenerations; ++ui)
    {
        // It is a configuration error if a master particle does not exist for
        // all generations.
        NiAVObject* pkMasterParticle = m_kMasterParticles.GetAt(ui);
        EE_ASSERT(pkMasterParticle);

        kStream.SaveLinkID(pkMasterParticle);
    }

    NiStreamSaveBinary(kStream, m_uiPoolSize);

    NiStreamSaveBinary(kStream, NiBool(m_bAutoFillPools));
}

//--------------------------------------------------------------------------------------------------
bool NiPSMeshParticleSystem::IsEqual(NiObject* pkObject)
{
    if (!NiPSParticleSystem::IsEqual(pkObject))
    {
        return false;
    }

    NiPSMeshParticleSystem* pkDest = (NiPSMeshParticleSystem*) pkObject;

    if (pkDest->m_kMasterParticles.GetSize() != m_kMasterParticles.GetSize())
    {
        return false;
    }

    const NiUInt32 uiMasterParticlesCount =
        pkDest->m_kMasterParticles.GetSize();
    for (NiUInt32 ui = 0; ui < uiMasterParticlesCount; ++ui)
    {
        NiAVObject* pkDestMasterParticle =
            pkDest->m_kMasterParticles.GetAt(ui);
        NiAVObject* pkSrcMasterParticle = m_kMasterParticles.GetAt(ui);
        if ((pkDestMasterParticle && !pkSrcMasterParticle) ||
            (!pkDestMasterParticle && pkSrcMasterParticle) ||
            (pkDestMasterParticle &&
                !pkDestMasterParticle->IsEqual(pkSrcMasterParticle)))
        {
            return false;
        }
    }

    if (pkDest->m_uiPoolSize != m_uiPoolSize ||
        pkDest->m_bAutoFillPools != m_bAutoFillPools)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiPSMeshParticleSystem::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiPSParticleSystem::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSMeshParticleSystem::ms_RTTI
        .GetName()));

    pkStrings->Add(NiGetViewerString("NumGenerations", m_kPools.GetSize()));
    pkStrings->Add(NiGetViewerString("PoolSize", m_uiPoolSize));
    pkStrings->Add(NiGetViewerString("AutoFillPools", m_bAutoFillPools));
}

//--------------------------------------------------------------------------------------------------
