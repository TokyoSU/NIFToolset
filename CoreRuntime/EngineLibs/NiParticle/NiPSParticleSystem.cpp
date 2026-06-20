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

#include "NiPSParticleSystem.h"
#include <NiVersion.h>
#include "NiPSCommonSemantics.h"
#include "NiPSKernelDefinitions.h"
#include "NiPSSimulatorGeneralStep.h"
#include "NiPSSimulatorForcesStep.h"
#include "NiPSSimulatorCollidersStep.h"
#include "NiPSSimulatorFinalStep.h"
#include "NiPSFacingQuadGenerator.h"
#include "NiPSFlagsHelpers.h"
#include "NiPSMetrics.h"

NiImplementRTTI(NiPSParticleSystem, NiMesh, NiTypeMask::NiPSParticleSystem);

const NiUInt32 NiPSParticleSystem::INVALID_PARTICLE = (NiUInt32) -1;

//--------------------------------------------------------------------------------------------------
// The following copyright notice may not be removed.
static char EmergentCopyright[] EE_UNUSED =
    "Copyright (c) 1996-2009 Emergent Game Technologies.";
//--------------------------------------------------------------------------------------------------
static char acGamebryoVersion[] EE_UNUSED =
    GAMEBRYO_MODULE_VERSION_STRING(NiParticle);
//--------------------------------------------------------------------------------------------------
NiPSParticleSystem* NiPSParticleSystem::Create(
    const NiUInt32 uiMaxNumParticles,
    const AlignMethod eNormalMethod,
    const NiPoint3& kNormalDirection,
    const AlignMethod eUpMethod,
    const NiPoint3& kUpDirection,
    const bool bHasLivingSpawner,
    const bool bHasColors,
    const bool bHasRotations,
    const bool bHasAnimatedTextures,
    const bool bWorldSpace,
    const bool bDynamicBounds,
    const bool bCreateDefaultGenerator,
    const bool bAttachMeshModifiers,
    const bool bSorted)
{
    // Create simulator.
    NiPSSimulator* pkSimulator = NiNew NiPSSimulator();

    // Add simulation steps.
    pkSimulator->AddStep(NiNew NiPSSimulatorGeneralStep());
    pkSimulator->AddStep(NiNew NiPSSimulatorForcesStep());
    pkSimulator->AddStep(NiNew NiPSSimulatorCollidersStep());
    pkSimulator->AddStep(NiNew NiPSSimulatorFinalStep());

    // Create bound updater, if requested.
    NiPSBoundUpdater* pkBoundUpdater = NULL;
    if (bDynamicBounds)
    {
        pkBoundUpdater = NiNew NiPSBoundUpdater();
    }

    // Create particle system.
    NiPSParticleSystem* pkSystem = NiNew NiPSParticleSystem(
        pkSimulator,
        uiMaxNumParticles,
        eNormalMethod,
        kNormalDirection,
        eUpMethod,
        kUpDirection,
        bHasLivingSpawner,
        bHasColors,
        bHasRotations,
        false,
        bHasAnimatedTextures,
        bWorldSpace,
        bSorted,
        pkBoundUpdater);

    // Add simulator.
    pkSystem->AddModifier(pkSimulator, bAttachMeshModifiers);

    // Create and add default generator, if requested.
    if (bCreateDefaultGenerator)
    {
        pkSystem->AddModifier(NiNew NiPSFacingQuadGenerator(),
            bAttachMeshModifiers);
    }

    return pkSystem;
}

//--------------------------------------------------------------------------------------------------
NiPSParticleSystem::NiPSParticleSystem() :
    m_pkSimulator(NULL),
    m_pkDeathSpawner(NULL),
    m_pkLivingSpawner(NULL),
    m_fLastTime(-NI_INFINITY),
    m_uiMaxNumParticles(0),
    m_uiNumParticles(0),
    m_uiAddedParticlesBase(0),
    m_uiNumAddedParticles(0),
    m_pkPositions(NULL),
    m_pkVelocities(NULL),
    m_pfAges(NULL),
    m_pfLifeSpans(NULL),
    m_pfLastUpdateTimes(NULL),
    m_pfNextSpawnTimes(NULL),
    m_puiFlags(NULL),
    m_pfInitialSizes(NULL),
    m_pfSizes(NULL),
    m_pkColors(NULL),
    m_pfInitialRotationAngles(NULL),
    m_pfRotationAngles(NULL),
    m_pfRotationSpeeds(NULL),
    m_pkRotationAxes(NULL),
    m_pfVariance(NULL),
    m_eNormalMethod(ALIGN_CAMERA),
    m_kNormalDirection(NiPoint3::ZERO),
    m_eUpMethod(ALIGN_CAMERA),
    m_kUpDirection(NiPoint3::ZERO),
    m_fPreviousWorldScale(1.0f),
    m_bWorldSpace(true),
    m_bSorted(false),
    m_bCompleteSimulation(false),
    m_bScaleSet(false),
    m_ucNumSpawnRateKeys(0),
    m_pkSpawnRateKeys(NULL),
    m_bPreRPIParticleSystem(false)
{
}

//--------------------------------------------------------------------------------------------------
NiPSParticleSystem::NiPSParticleSystem(
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
    const bool bHasAnimatedTextures,
    const bool bWorldSpace,
    const bool bSorted,
    NiPSBoundUpdater* pkBoundUpdater) :
    m_pkSimulator(pkSimulator),
    m_spBoundUpdater(pkBoundUpdater),
    m_pkDeathSpawner(NULL),
    m_pkLivingSpawner(NULL),
    m_fLastTime(-NI_INFINITY),
    m_uiMaxNumParticles(uiMaxNumParticles),
    m_uiNumParticles(0),
    m_uiAddedParticlesBase(0),
    m_uiNumAddedParticles(0),
    m_pkPositions(NULL),
    m_pkVelocities(NULL),
    m_pfAges(NULL),
    m_pfLifeSpans(NULL),
    m_pfLastUpdateTimes(NULL),
    m_pfNextSpawnTimes(NULL),
    m_puiFlags(NULL),
    m_pfInitialSizes(NULL),
    m_pfSizes(NULL),
    m_pkColors(NULL),
    m_pfInitialRotationAngles(NULL),
    m_pfRotationAngles(NULL),
    m_pfRotationSpeeds(NULL),
    m_pkRotationAxes(NULL),
    m_pfVariance(NULL),
    m_eNormalMethod(eNormalMethod),
    m_kNormalDirection(kNormalDirection),
    m_eUpMethod(eUpMethod),
    m_kUpDirection(kUpDirection),
    m_fPreviousWorldScale(1.0f),
    m_bWorldSpace(bWorldSpace),
    m_bSorted(bSorted),
    m_bCompleteSimulation(false),
    m_bScaleSet(false),
    m_ucNumSpawnRateKeys(0),
    m_pkSpawnRateKeys(NULL),
    m_bPreRPIParticleSystem(false)
{
    EE_ASSERT(m_pkSimulator);

    AllocateDataBuffers(m_uiMaxNumParticles, bHasLivingSpawner, bHasColors, bHasRotations,
        bHasRotationAxes, bHasAnimatedTextures);
}

//--------------------------------------------------------------------------------------------------
NiPSParticleSystem::~NiPSParticleSystem()
{
    // Detach the modifier to free the tasks which hold pointers to the
    // streams we are about to delete.
    DetachAllModifiers();

    RemoveAllTargettingSpawners();

    RemoveAllSpawners();

    m_spBoundUpdater = 0;

    NiAlignedFree(m_pkSpawnRateKeys);
    m_pkSpawnRateKeys = NULL;

    FreeDataBuffers();
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::ResetParticleSystem(const float fNewLastTime)
{
    // Completes tasks.
    m_pkSimulator->ResetSimulator(this);
    
    // Reset emitter locations.
    NiUInt32 uiEmitterCount = m_kEmitters.GetSize();
    for (NiUInt32 ui = 0; ui < uiEmitterCount; ++ui)
    {
        EE_ASSERT(m_kEmitters.GetAt(ui));
        m_kEmitters.GetAt(ui)->ResetTransformation();
    }

    m_uiNumParticles = 0;
    m_uiNumAddedParticles = 0;
    m_uiAddedParticlesBase = 0;
    m_fLastTime = fNewLastTime;
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiPSParticleSystem::AddParticle()
{
    // Simulation may be active. We can't just add the particle because it
    // will confuse the particle count. So stash it at the end but do
    // not increment the particle count.
    if (m_uiNumAddedParticles == 0)
    {
        m_uiAddedParticlesBase = m_uiNumParticles;
    }

    // Ensure that we are not exceeding our max number of particles.
    if (m_uiAddedParticlesBase + m_uiNumAddedParticles <
        m_uiMaxNumParticles)
    {
        return (m_uiAddedParticlesBase + m_uiNumAddedParticles++);
    }
    else
    {
        return INVALID_PARTICLE;
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::InitializeParticle(NiUInt32 uiNewParticle)
{
    if (m_pfVariance)
    {
        m_pfVariance[uiNewParticle] = NiSymmetricRandom();
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::InitializeSpawnedParticle(NiUInt32 uiNewParticle)
{
    InitializeParticle(uiNewParticle);

    // Set the size correctly. The emitter does not have enough information to
    // set this and it must be set before the first particle update.
    NiPSSimulatorGeneralStep* pkGeneralStep = (NiPSSimulatorGeneralStep*)
        m_pkSimulator->GetSimulatorStepByType(
        &NiPSSimulatorGeneralStep::ms_RTTI);
    if (pkGeneralStep)
    {
        pkGeneralStep->InitializeNewParticle(this, uiNewParticle);
    }
}

//--------------------------------------------------------------------------------------------------
NiPSEmitter* NiPSParticleSystem::GetEmitterByName(const NiFixedString& kName)
    const
{
    const NiUInt32 uiEmitterCount = m_kEmitters.GetSize();
    for (NiUInt32 ui = 0; ui < uiEmitterCount; ++ui)
    {
        NiPSEmitter* pkEmitter = m_kEmitters.GetAt(ui);
        if (pkEmitter->GetName() == kName)
        {
            return pkEmitter;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::AddSpawner(NiPSSpawner* pkSpawner)
{
    EE_ASSERT(pkSpawner);

    // We do not allow circular dependencies.
    if (pkSpawner->GetMasterPSystem() == this)
    {
        // If this occurs simply set the master particle system to null.
        // A null master particle system will result in particles
        // spawned from the old particle system.
        pkSpawner->SetMasterPSystem(NULL);
    }

    m_kSpawners.Add(pkSpawner);
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::RemoveSpawnerAt(NiUInt32 uiIndex, bool bMaintainOrder)
{
    EE_ASSERT(uiIndex < m_kSpawners.GetSize());

    NiPSSpawner* pkSpawner = m_kSpawners.GetAt(uiIndex);
    if (m_pkDeathSpawner == pkSpawner)
        m_pkDeathSpawner = NULL;
    else if (m_pkLivingSpawner == pkSpawner)
        m_pkLivingSpawner = NULL;

    if (bMaintainOrder)
    {
        m_kSpawners.OrderedRemoveAt(uiIndex);
    }
    else
    {
        m_kSpawners.RemoveAt(uiIndex);
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::RemoveAllSpawners()
{
    m_pkDeathSpawner = NULL;
    m_pkLivingSpawner = NULL;
    m_kSpawners.RemoveAll();
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::UpdateDownwardPass(NiUpdateProcess& kUpdate)
{
    // Perform pre-update steps.
    PreUpdate(kUpdate);

    NiMesh::UpdateDownwardPass(kUpdate);

    // Perform post-update steps.
    PostUpdate(kUpdate);
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::UpdateSelectedDownwardPass(NiUpdateProcess& kUpdate)
{
    // Perform pre-update steps.
    PreUpdate(kUpdate);

    NiMesh::UpdateSelectedDownwardPass(kUpdate);

    // Perform post-update steps.
    PostUpdate(kUpdate);
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::UpdateRigidDownwardPass(NiUpdateProcess& kUpdate)
{
    // Perform pre-update steps.
    PreUpdate(kUpdate);

    NiMesh::UpdateRigidDownwardPass(kUpdate);

    // Perform post-update steps.
    PostUpdate(kUpdate);
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::PreUpdate(NiUpdateProcess& kUpdate)
{
    // Do nothing if modifiers will not be submitted.
    if (!kUpdate.GetSubmitModifiers())
        return;

    // Catch the case where object is updated twice in a row without being
    // rendered.
    if (m_bCompleteSimulation)
    {
        NiSyncArgs kCompleteArgs;
        kCompleteArgs.m_uiSubmitPoint = NiSyncArgs::SYNC_UPDATE;
        kCompleteArgs.m_uiCompletePoint = NiSyncArgs::SYNC_ANY;
        CompleteModifiers(&kCompleteArgs);
    }
    m_bCompleteSimulation = true;

    NIMETRICS_PARTICLE_SCOPETIMER(UPDATE_PSYS_TIME);

    float fTime = kUpdate.GetTime();

    // Initialize the last time.
    if (m_fLastTime == -NI_INFINITY)
    {
        m_fLastTime = fTime;
    }

    // Reset particle system if the last time is greater than the current time.
    if (m_fLastTime > fTime)
    {
        ResetParticleSystem();
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::PostUpdate(NiUpdateProcess& kUpdate)
{
    // Do nothing if modifiers will not be submitted.
    if (!kUpdate.GetSubmitModifiers())
        return;

    // Update the last time.
    m_fLastTime = kUpdate.GetTime();
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::CompleteSimulation()
{
    UpdateParticlesUponCompletion();

    // Update bound.
    if (m_spBoundUpdater)
    {
        m_spBoundUpdater->UpdateBound(this);
    }

    m_bCompleteSimulation = false;

    NIMETRICS_PARTICLE_ADDVALUE(UPDATED_PARTICLES, m_uiNumParticles);
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::UpdateParticlesUponCompletion()
{
    // Add any particles that were emitted during the simulation phase. Doing
    // this now, before particles are deleted, reduces copying of particle
    // data.
    ResolveAddedParticles(false);

    // Process particles that need to be spawned or killed.
    ResolveSpawnedAndRemovedParticles(m_fLastTime);

    if (m_bWorldSpace)
    {
        if (m_bScaleSet && m_kWorld.m_fScale != m_fPreviousWorldScale)
        {
            // The world scale has been changed. We need to un-apply the
            // difference between the old scale and the new scale from the
            // particle positions so that existing particles do not change
            // position.
            float fScaleDiffInv = m_fPreviousWorldScale / m_kWorld.m_fScale;
            for (NiUInt32 ui = 0; ui < m_uiNumParticles; ++ui)
            {
                m_pkPositions[ui] *= fScaleDiffInv;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::SetSelectiveUpdateFlags(
    bool& bSelectiveUpdate,
    bool bSelectiveUpdateTransforms,
    bool& bRigid)
{
    bSelectiveUpdate = true;
    bSelectiveUpdateTransforms = true;
    bRigid = false;
    SetSelectiveUpdate(bSelectiveUpdate);
    SetSelectiveUpdateTransforms(bSelectiveUpdateTransforms);
    SetSelectiveUpdatePropertyControllers(true);
    SetSelectiveUpdateRigid(bRigid);
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::RenderImmediate(NiRenderer* pkRenderer)
{
    if (m_uiNumParticles > 0)
    {
        NiMesh::RenderImmediate(pkRenderer);
    }
    else
    {
        // This code is taken from NiMesh::RenderImmediate. If no particles
        // exist, we don't want to render the system, but we do want to
        // complete any modifiers that are currently executing.
        NiSyncArgs kCompleteArgs;
        kCompleteArgs.m_uiSubmitPoint = NiSyncArgs::SYNC_ANY;
        kCompleteArgs.m_uiCompletePoint = NiSyncArgs::SYNC_RENDER;
        CompleteModifiers(&kCompleteArgs);
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::UpdateWorldData()
{
    m_fPreviousWorldScale = m_kWorld.m_fScale;

    NiMesh::UpdateWorldData();

    m_kUnmodifiedWorld = m_kWorld;

    if (m_bWorldSpace)
    {
        m_kWorld.m_Translate = NiPoint3::ZERO;
        m_kWorld.m_Rotate = NiMatrix3::IDENTITY;
    }

    m_bScaleSet = true;
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::UpdateWorldBound()
{
    if (m_bWorldSpace)
    {
        if (m_spBoundUpdater)
        {
            m_kWorldBound.SetCenterAndRadius(
                m_kWorld.m_fScale * m_kBound.GetCenter(),
                m_kWorld.m_fScale * m_kBound.GetRadius());
        }
        else
        {
            m_kWorldBound.Update(m_kBound, m_kUnmodifiedWorld);
        }
    }
    else
    {
        NiMesh::UpdateWorldBound();
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::AllocSpawnRateKeys(NiUInt8 ucNumSpawnRateKeys)
{
    // Are correct number of keys are already allocated?
    if (ucNumSpawnRateKeys == m_ucNumSpawnRateKeys)
    {
        return;
    }

    // Release previously allocated keys
    if (m_pkSpawnRateKeys)
    {
        NiAlignedFree(m_pkSpawnRateKeys);
        m_pkSpawnRateKeys = NULL;
    }

    if (ucNumSpawnRateKeys > 0)
    {
        m_pkSpawnRateKeys = NiAlignedAlloc(NiPSKernelFloatKey, ucNumSpawnRateKeys,
            NIPSKERNEL_ALIGNMENT);
    }

    m_ucNumSpawnRateKeys = ucNumSpawnRateKeys;
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::CopySpawnRateKeys(const NiPSKernelFloatKey* pkSpawnRateKeys,
                                            const NiUInt8 ucNumKeys)
{
    EE_ASSERT((!pkSpawnRateKeys && ucNumKeys == 0) || (pkSpawnRateKeys && ucNumKeys > 0));

    AllocSpawnRateKeys(ucNumKeys);

    if (ucNumKeys > 0)
    {
        NiMemcpy(m_pkSpawnRateKeys, sizeof(NiPSKernelFloatKey) * ucNumKeys, pkSpawnRateKeys,
            sizeof(NiPSKernelFloatKey) * ucNumKeys);
    }
}

//--------------------------------------------------------------------------------------------------
bool NiPSParticleSystem::GetNextSpawnTime(float fTime, float fAge, float fLifeSpan,
    float& fSpawnTime) const
{
    fSpawnTime = FLT_MAX;

    if (!m_pkSpawnRateKeys || fLifeSpan <= 0.0f || m_ucNumSpawnRateKeys == 0)
        return false;

    // Find key frame - look while keyframe time < age
    float fScaledAge = fAge / fLifeSpan;

    NiUInt32 ui = 0;
    NiUInt32 uiNumSpawnRateKeysMinusOne = m_ucNumSpawnRateKeys-1;
    while (fScaledAge > m_pkSpawnRateKeys[ui].m_fTime && ui < uiNumSpawnRateKeysMinusOne)
        ui++;

    float fValueMin = (ui > 0) ? m_pkSpawnRateKeys[ui-1].m_fValue : m_pkSpawnRateKeys[ui].m_fValue;
    float fTimeMin = (ui > 0) ? m_pkSpawnRateKeys[ui-1].m_fTime : m_pkSpawnRateKeys[ui].m_fTime;
    float fValueMax = m_pkSpawnRateKeys[ui].m_fValue;
    float fTimeMax = m_pkSpawnRateKeys[ui].m_fTime;

    float fValue = 0.0f;
    float fTimeRange = fTimeMax - fTimeMin;
    if (fTimeRange <= 0.0f)
    {
        fValue = fValueMin;
    }
    else
    {
        float fFactorMin = 1.0f - ((fScaledAge - fTimeMin) / fTimeRange);
        float fFactorMax = 1.0f - ((fTimeMax - fScaledAge) / fTimeRange);
        fValue = fFactorMin * fValueMin + fFactorMax * fValueMax;
    }

    if (fValue <= 0.0f)
        return false;

    float fSpawnPeriod = 1.0f / fValue;

    fSpawnTime = fTime + fSpawnPeriod;

    return true;
}

//--------------------------------------------------------------------------------------------------
float NiPSParticleSystem::GetRunUpTime(bool bIncludeSpawners) const
{
    float fMaxLifeSpan = 0.0f;
    const NiUInt32 uiEmitterCount = m_kEmitters.GetSize();
    for (NiUInt32 ui = 0; ui < uiEmitterCount; ui++)
    {
        NiPSEmitter* pkEmitter = m_kEmitters.GetAt(ui);
        fMaxLifeSpan = NiMax(pkEmitter->GetLifeSpan() + pkEmitter->GetLifeSpanVar() * 0.5f,
            fMaxLifeSpan);
    }

    if (bIncludeSpawners)
    {
        float fLivingSpawnerLifeSpan = 0.0f;
        if (m_pkLivingSpawner)
        {
            fLivingSpawnerLifeSpan = m_pkLivingSpawner->GetLifeSpan() +
                m_pkLivingSpawner->GetLifeSpanVar() * 0.5f;
        }

        float fDeathSpawnerLifeSpan = 0.0f;
        if (m_pkDeathSpawner)
        {
            fDeathSpawnerLifeSpan = m_pkDeathSpawner->GetLifeSpan() +
                m_pkDeathSpawner->GetLifeSpanVar() * 0.5f;
        }

        fMaxLifeSpan += NiMax(fLivingSpawnerLifeSpan, fDeathSpawnerLifeSpan);

        // Set a minimum run up time. This tries to handle spawner particle systems.
        fMaxLifeSpan = NiMax(fMaxLifeSpan, 3.0f);
    }

    return fMaxLifeSpan;
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::ResolveSpawnedAndRemovedParticles(float fTime)
{
#ifdef _XENON
    // Prefetch the flags array to reduce L2 cache misses.
    NiUInt32 uiTotalSize = m_uiNumParticles * sizeof(NiUInt32);
    for (NiUInt32 ui = 0; ui < uiTotalSize; ui += 128)
    {
        __dcbt(ui, m_puiFlags);
    }
#endif  // #ifdef _XENON

    // Build set of particle indices that should be removed.
    EE_ASSERT(m_kParticlesToProcess.GetSize() == 0);
    for (NiUInt32 ui = 0; ui < m_uiNumParticles; ++ui)
    {
        ParticleProcessInfo kInfo;
        bool bAddToSet = false;

        NiUInt32& uiFlags = m_puiFlags[ui];

        if (NiPSFlagsHelpers::GetShouldDie(uiFlags))
        {
            kInfo.m_bShouldDie = true;
            bAddToSet = true;
            NiPSFlagsHelpers::SetShouldDie(uiFlags, false);
        }
        else if (m_pkLivingSpawner) // Still alive, see if its time to spawn.
        {
            if (m_pfNextSpawnTimes[ui] <= fTime)
            {
                kInfo.m_bShouldSpawn = true;
                kInfo.m_ucSpawnerID = NiPSFlagsHelpers::GetSpawnerID(uiFlags);
                bAddToSet = true;
                NiPSFlagsHelpers::SetShouldSpawn(uiFlags, false);
            }
        }

        if (NiPSFlagsHelpers::GetShouldSpawn(uiFlags))
        {
            // Check for death
            if (m_pkDeathSpawner)
            {
                kInfo.m_bShouldSpawn = true;
                kInfo.m_ucSpawnerID = NiPSFlagsHelpers::GetSpawnerID(uiFlags);
                bAddToSet = true;
            }
            else
            {
                // Collision spawner
                kInfo.m_bShouldSpawn = true;
                kInfo.m_ucSpawnerID = NiPSFlagsHelpers::GetSpawnerID(uiFlags);
                bAddToSet = true;
            }

            NiPSFlagsHelpers::SetShouldSpawn(uiFlags, false);
        }

        if (bAddToSet)
        {
            kInfo.m_uiIndex = ui;
            m_kParticlesToProcess.Add(kInfo);
        }
    }

    // Iterate over array of indices to remove backwards, removing each
    // particle.
#if NIMETRICS
    NiUInt32 uiNumSpawned = 0;
    NiUInt32 uiNumDestroyed = 0;
#endif  // #if NIMETRICS
    for (NiUInt32 ui = m_kParticlesToProcess.GetSize(); ui > 0; --ui)
    {
        ParticleProcessInfo kInfo = m_kParticlesToProcess.GetAt(ui - 1);

        if (kInfo.m_bShouldSpawn)
        {
            EE_ASSERT(kInfo.m_ucSpawnerID < GetMaxValidSpawnerID());

            // Retrieve spawner from ID.
            NiPSSpawner* pkSpawner = GetSpawnerFromID(kInfo.m_ucSpawnerID);

            if (pkSpawner)
            {
                // Set the next time we need to spawn
                float fSpawnTime = fTime;
                if (m_pfNextSpawnTimes)
                {
                    fSpawnTime = m_pfNextSpawnTimes[kInfo.m_uiIndex];
                    GetNextSpawnTime(fTime, m_pfAges[kInfo.m_uiIndex],
                        m_pfLifeSpans[kInfo.m_uiIndex], m_pfNextSpawnTimes[kInfo.m_uiIndex]);
                }

                pkSpawner->SpawnParticles(fTime, fSpawnTime, kInfo.m_uiIndex,
                    this);

#if NIMETRICS
                uiNumSpawned++;
#endif  // #if NIMETRICS
            }
        }

        if (kInfo.m_bShouldDie)
        {
            RemoveParticle(kInfo.m_uiIndex);

#if NIMETRICS
            uiNumDestroyed++;
#endif  // #if NIMETRICS
        }
    }

    m_kParticlesToProcess.RemoveAll();

    // Clear active spawners. The list is repopulated each time update is
    // called.
    m_kActiveSpawners.RemoveAll();

#if NIMETRICS
    if (uiNumSpawned > 0)
    {
        NIMETRICS_PARTICLE_ADDVALUE(PARTICLES_SPAWNED, uiNumSpawned);
    }
    if (uiNumDestroyed > 0)
    {
        NIMETRICS_PARTICLE_ADDVALUE(PARTICLES_DESTROYED, uiNumDestroyed);
    }
#endif  // #if NIMETRICS
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::ResolveAddedParticles(const bool bUpdateBound)
{
    if (m_uiNumParticles < m_uiAddedParticlesBase)
    {
        // Some particles were removed; fill array so that it is packed.
        // This only happens when a spawner adds particles.
        NiUInt32 uiIndex = m_uiNumParticles;
        m_uiNumParticles = m_uiAddedParticlesBase + m_uiNumAddedParticles;
        while (uiIndex < m_uiAddedParticlesBase &&
            m_uiNumParticles > m_uiAddedParticlesBase)
        {
            RemoveParticle(uiIndex++);
        }

        // If more particles were removed than were added, adjust number
        // of particles appropriately.
        if (uiIndex < m_uiAddedParticlesBase)
        {
            m_uiNumParticles = uiIndex;
        }
    }
    else
    {
        // m_uiNumParticles should never be greater than
        // m_uiAddedParticlesBase.
        EE_ASSERT(m_uiNumParticles == m_uiAddedParticlesBase);
        m_uiNumParticles += m_uiNumAddedParticles;

        if (bUpdateBound && m_spBoundUpdater && m_uiNumParticles &&
            m_uiNumParticles == m_uiNumAddedParticles)
        {
            // Update the bound because the entire set of particles is new.
            m_spBoundUpdater->UpdateBound(this);
        }
    }

    m_uiNumAddedParticles = 0;
    m_uiAddedParticlesBase = m_uiNumParticles;
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::RemoveParticle(NiUInt32 uiIndexToRemove)
{
    EE_ASSERT(uiIndexToRemove < m_uiNumParticles);
    NiUInt32 uiFinalIndex = m_uiNumParticles - 1;

    // Copy the particle data from the final particle into the slot for the
    // removed particle to ensure that the particle array is always packed.
    m_pkPositions[uiIndexToRemove] = m_pkPositions[uiFinalIndex];
    m_pkVelocities[uiIndexToRemove] = m_pkVelocities[uiFinalIndex];
    m_pfAges[uiIndexToRemove] = m_pfAges[uiFinalIndex];
    m_pfLifeSpans[uiIndexToRemove] = m_pfLifeSpans[uiFinalIndex];
    m_puiFlags[uiIndexToRemove] = m_puiFlags[uiFinalIndex];
    m_pfInitialSizes[uiIndexToRemove] = m_pfInitialSizes[uiFinalIndex];
    m_pfSizes[uiIndexToRemove] = m_pfSizes[uiFinalIndex];
    if (m_pfNextSpawnTimes)
    {
        m_pfNextSpawnTimes[uiIndexToRemove] = m_pfNextSpawnTimes[uiFinalIndex];
    }
    if (m_pkColors)
    {
        m_pkColors[uiIndexToRemove] = m_pkColors[uiFinalIndex];
    }
    if (m_pfInitialRotationAngles)
    {
        m_pfInitialRotationAngles[uiIndexToRemove] = m_pfInitialRotationAngles[uiFinalIndex];
    }
    if (m_pfRotationAngles)
    {
        m_pfRotationAngles[uiIndexToRemove] = m_pfRotationAngles[uiFinalIndex];
    }
    if (m_pfRotationSpeeds)
    {
        m_pfRotationSpeeds[uiIndexToRemove] = m_pfRotationSpeeds[uiFinalIndex];
    }
    if (m_pkRotationAxes)
    {
        m_pkRotationAxes[uiIndexToRemove] = m_pkRotationAxes[uiFinalIndex];
    }
    if (m_pfVariance)
    {
        m_pfVariance[uiIndexToRemove] = m_pfVariance[uiFinalIndex];
    }

    m_uiNumParticles--;
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::AllocateDataBuffers(
    NiUInt32 uiBufferSize,
    bool bAllocateSpawnTime,
    bool bAllocateColors,
    bool bAllocateRotations,
    bool bAllocateRotationAxes,
    bool bAllocateTextureAnimations)
{
    FreeDataBuffers();

    // Floodgate performance is better with aligned memory.
    m_pkPositions = NiAlignedAlloc(NiPoint3, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    m_pkVelocities = NiAlignedAlloc(NiPoint3, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    m_pfAges = NiAlignedAlloc(float, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    m_pfLifeSpans = NiAlignedAlloc(float, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    m_pfLastUpdateTimes = NiAlignedAlloc(float, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    m_puiFlags = NiAlignedAlloc(NiUInt32, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    m_pfInitialSizes = NiAlignedAlloc(float, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    m_pfSizes = NiAlignedAlloc(float, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    if (bAllocateSpawnTime)
    {
        m_pfNextSpawnTimes = NiAlignedAlloc(float, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    }
    if (bAllocateColors)
    {
        m_pkColors = NiAlignedAlloc(NiRGBA, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    }
    if (bAllocateRotations)
    {
        m_pfInitialRotationAngles = NiAlignedAlloc(float, uiBufferSize, NIPSKERNEL_ALIGNMENT);
        m_pfRotationAngles = NiAlignedAlloc(float, uiBufferSize, NIPSKERNEL_ALIGNMENT);
        m_pfRotationSpeeds = NiAlignedAlloc(float, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    }
    if (bAllocateRotationAxes)
    {
        m_pkRotationAxes = NiAlignedAlloc(NiPoint3, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    }
    if (bAllocateTextureAnimations)
    {
        m_pfVariance = NiAlignedAlloc(float, uiBufferSize, NIPSKERNEL_ALIGNMENT);
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::FreeDataBuffers()
{
    NiAlignedFree(m_pkPositions);
    m_pkPositions = NULL;

    NiAlignedFree(m_pkVelocities);
    m_pkVelocities = NULL;

    NiAlignedFree(m_pfAges);
    m_pfAges = NULL;

    NiAlignedFree(m_pfLifeSpans);
    m_pfLifeSpans = NULL;

    NiAlignedFree(m_pfLastUpdateTimes);
    m_pfLastUpdateTimes = NULL;

    NiAlignedFree(m_pfNextSpawnTimes);
    m_pfNextSpawnTimes = NULL;

    NiAlignedFree(m_puiFlags);
    m_puiFlags = NULL;

    NiAlignedFree(m_pfInitialSizes);
    m_pfInitialSizes = NULL;

    NiAlignedFree(m_pfSizes);
    m_pfSizes = NULL;

    NiAlignedFree(m_pkColors);
    m_pkColors = NULL;

    NiAlignedFree(m_pfInitialRotationAngles);
    m_pfInitialRotationAngles = NULL;

    NiAlignedFree(m_pfRotationAngles);
    m_pfRotationAngles = NULL;

    NiAlignedFree(m_pfRotationSpeeds);
    m_pfRotationSpeeds = NULL;

    NiAlignedFree(m_pkRotationAxes);
    m_pkRotationAxes = NULL;

    NiAlignedFree(m_pfVariance);
    m_pfVariance = NULL;
}

//--------------------------------------------------------------------------------------------------
NiPSParticleSystem::ParticleProcessInfo::ParticleProcessInfo() :
    m_uiIndex(0),
    m_ucSpawnerID(0),
    m_bShouldSpawn(false),
    m_bShouldDie(false)
{
}

//--------------------------------------------------------------------------------------------------
const char* NiPSParticleSystem::GetMethodString(const AlignMethod eMethod)
{
    switch (eMethod)
    {
        case ALIGN_PER_PARTICLE:
            return "Per Particle";

        case ALIGN_LOCAL_FIXED:
            return "Local Fixed";

        case ALIGN_LOCAL_POSITION:
            return "Local Position";

        case ALIGN_LOCAL_VELOCITY:
            return "Local Velocity";

        case ALIGN_CAMERA:
            return "Camera";

        default:
            return NULL;
    }
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSParticleSystem);

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::CopyMembers(
    NiPSParticleSystem* pkDest,
    NiCloningProcess& kCloning)
{
    // We must complete all modifiers before accessing the particle data.
    ForceSimulationToComplete();

    NiMesh::CopyMembers(pkDest, kCloning);

    if (m_spBoundUpdater)
    {
        pkDest->m_spBoundUpdater = (NiPSBoundUpdater*)
            m_spBoundUpdater->CreateClone(kCloning);
    }

    const NiUInt32 uiEmittersCount = m_kEmitters.GetSize();
    for (NiUInt32 ui = 0; ui < uiEmittersCount; ++ui)
    {
        pkDest->AddEmitter((NiPSEmitter*)
            m_kEmitters.GetAt(ui)->CreateClone(kCloning));
    }

    const NiUInt32 uiSpawnersCount = m_kSpawners.GetSize();
    for (NiUInt32 ui = 0; ui < uiSpawnersCount; ++ui)
    {
        pkDest->AddSpawner((NiPSSpawner*)
            m_kSpawners.GetAt(ui)->CreateClone(kCloning));
    }

    pkDest->m_uiMaxNumParticles = m_uiMaxNumParticles;
    pkDest->m_uiNumParticles = m_uiNumParticles;

    pkDest->m_uiAddedParticlesBase = m_uiAddedParticlesBase;
    pkDest->m_uiNumAddedParticles = m_uiNumAddedParticles;

    pkDest->m_bWorldSpace = m_bWorldSpace;
    pkDest->m_bCompleteSimulation = m_bCompleteSimulation;

    // Particle data is not shared, so copy it here.
    pkDest->AllocateDataBuffers(m_uiMaxNumParticles, HasLivingSpawner(), HasColors(),
        HasRotations(), HasRotationAxes(), HasAnimatedTextures());
    NiMemcpy(pkDest->m_pkPositions, m_uiMaxNumParticles * sizeof(NiPoint3),
        m_pkPositions, m_uiNumParticles * sizeof(NiPoint3));
    NiMemcpy(pkDest->m_pkVelocities, m_uiMaxNumParticles * sizeof(NiPoint3),
        m_pkVelocities, m_uiNumParticles * sizeof(NiPoint3));
    NiMemcpy(pkDest->m_pfAges, m_uiMaxNumParticles * sizeof(float), m_pfAges,
        m_uiNumParticles * sizeof(float));
    NiMemcpy(pkDest->m_pfLifeSpans, m_uiMaxNumParticles * sizeof(float),
        m_pfLifeSpans, m_uiNumParticles * sizeof(float));
    NiMemcpy(pkDest->m_pfLastUpdateTimes, m_uiMaxNumParticles * sizeof(float),
        m_pfLastUpdateTimes, m_uiNumParticles * sizeof(float));
    NiMemcpy(pkDest->m_puiFlags, m_uiMaxNumParticles * sizeof(NiUInt32),
        m_puiFlags, m_uiNumParticles * sizeof(NiUInt32));
    NiMemcpy(pkDest->m_pfInitialSizes, m_uiMaxNumParticles * sizeof(float), m_pfInitialSizes,
        m_uiNumParticles * sizeof(float));
    NiMemcpy(pkDest->m_pfSizes, m_uiMaxNumParticles * sizeof(float), m_pfSizes,
        m_uiNumParticles * sizeof(float));
    if (HasLivingSpawner())
    {
        NiMemcpy(pkDest->m_pfNextSpawnTimes, m_uiMaxNumParticles * sizeof(float),
        m_pfNextSpawnTimes, m_uiNumParticles * sizeof(float));
    }
    if (HasColors())
    {
        NiMemcpy(pkDest->m_pkColors, m_uiMaxNumParticles * sizeof(NiRGBA),
            m_pkColors, m_uiNumParticles * sizeof(NiRGBA));
    }
    if (HasRotations())
    {
        NiMemcpy(pkDest->m_pfInitialRotationAngles,
            m_uiMaxNumParticles * sizeof(float), m_pfInitialRotationAngles,
            m_uiNumParticles * sizeof(float));
        NiMemcpy(pkDest->m_pfRotationAngles,
            m_uiMaxNumParticles * sizeof(float), m_pfRotationAngles,
            m_uiNumParticles * sizeof(float));
    }
    if (HasRotationSpeeds())
    {
        NiMemcpy(pkDest->m_pfRotationSpeeds,
            m_uiMaxNumParticles * sizeof(float), m_pfRotationSpeeds,
            m_uiNumParticles * sizeof(float));
    }
    if (HasRotationAxes())
    {
        NiMemcpy(pkDest->m_pkRotationAxes,
            m_uiMaxNumParticles * sizeof(NiPoint3), m_pkRotationAxes,
            m_uiNumParticles * sizeof(NiPoint3));
    }
    if (HasAnimatedTextures())
    {
        NiMemcpy(pkDest->m_pfVariance,
            m_uiMaxNumParticles * sizeof(float), m_pfVariance,
            m_uiNumParticles * sizeof(float));
    }

    pkDest->m_eNormalMethod = m_eNormalMethod;
    pkDest->m_kNormalDirection = m_kNormalDirection;
    pkDest->m_eUpMethod = m_eUpMethod;
    pkDest->m_kUpDirection = m_kUpDirection;

    pkDest->AllocSpawnRateKeys(m_ucNumSpawnRateKeys);
    for (NiUInt8 ui = 0; ui < m_ucNumSpawnRateKeys; ++ui)
    {
        pkDest->m_pkSpawnRateKeys[ui].m_fValue = m_pkSpawnRateKeys[ui].m_fValue;
        pkDest->m_pkSpawnRateKeys[ui].m_fTime = m_pkSpawnRateKeys[ui].m_fTime;
    }

    pkDest->m_bPreRPIParticleSystem = m_bPreRPIParticleSystem;

}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::ProcessClone(NiCloningProcess& kCloning)
{
    NiMesh::ProcessClone(kCloning);

    NiObject* pkObject = NULL;
    EE_VERIFY(kCloning.m_pkCloneMap->GetAt(this, pkObject));
    NiPSParticleSystem* pkDest = (NiPSParticleSystem*) pkObject;

    // The simulator must have been cloned during this cloning operation. It
    // cannot be shared between particle system clones.
    EE_VERIFY(kCloning.m_pkCloneMap->GetAt(m_pkSimulator, pkObject));
    pkDest->m_pkSimulator = (NiPSSimulator*) pkObject;

    if (m_spBoundUpdater)
    {
        m_spBoundUpdater->ProcessClone(kCloning);
    }

    const NiUInt32 uiEmittersCount = m_kEmitters.GetSize();
    for (NiUInt32 ui = 0; ui < uiEmittersCount; ++ui)
    {
        m_kEmitters.GetAt(ui)->ProcessClone(kCloning);
    }

    const NiUInt32 uiSpawnersCount = m_kSpawners.GetSize();
    for (NiUInt32 ui = 0; ui < uiSpawnersCount; ++ui)
    {
        m_kSpawners.GetAt(ui)->ProcessClone(kCloning);
    }

    if (kCloning.m_pkCloneMap->GetAt(m_pkDeathSpawner, pkObject))
    {
        pkDest->m_pkDeathSpawner = (NiPSSpawner*) pkObject;
    }
    else
    {
        pkDest->m_pkDeathSpawner = m_pkDeathSpawner;
    }

    if (kCloning.m_pkCloneMap->GetAt(m_pkLivingSpawner, pkObject))
    {
        pkDest->m_pkLivingSpawner = (NiPSSpawner*) pkObject;
    }
    else
    {
        pkDest->m_pkLivingSpawner = m_pkLivingSpawner;
    }
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSParticleSystem);

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::LoadBinary(NiStream& kStream)
{
    NiMesh::LoadBinary(kStream);

    m_pkSimulator = (NiPSSimulator*) kStream.ResolveLinkID();

    m_spBoundUpdater = (NiPSBoundUpdater*) kStream.ResolveLinkID();

    NiUInt32 uiEmittersCount;
    NiStreamLoadBinary(kStream, uiEmittersCount);
    for (NiUInt32 ui = 0; ui < uiEmittersCount; ++ui)
    {
        AddEmitter((NiPSEmitter*) kStream.ResolveLinkID());
    }

    NiUInt32 uiSpawnersCount;
    NiStreamLoadBinary(kStream, uiSpawnersCount);
    for (NiUInt32 ui = 0; ui < uiSpawnersCount; ++ui)
    {
        NiPSSpawner* pkSpawner = (NiPSSpawner*) kStream.ResolveLinkID();

        if (kStream.GetFileVersion() < NiStream::GetVersion(20, 6, 1, 0))
        {

            // Old assets were using the owner particle system as the master
            // so simply set master particle system to null. This will result
            // in the owner particle system to be used as the master.
            pkSpawner->SetMasterPSystem(NULL);
        }

        AddSpawner(pkSpawner);
    }

    m_pkDeathSpawner = (NiPSSpawner*) kStream.ResolveLinkID();

    NiStreamLoadBinary(kStream, m_uiMaxNumParticles);

    NiBool bValue;
    NiStreamLoadBinary(kStream, bValue);
    bool bHasColors = NIBOOL_IS_TRUE(bValue);
    NiStreamLoadBinary(kStream, bValue);
    bool bHasRotations = NIBOOL_IS_TRUE(bValue);
    NiStreamLoadBinary(kStream, bValue);
    bool bHasRotationAxes = NIBOOL_IS_TRUE(bValue);
    bool bHasAnimatedTextures = false;
    if (kStream.GetFileVersion() >= NiStream::GetVersion(20, 6, 1, 0))
    {
        NiStreamLoadBinary(kStream, bValue);
        bHasAnimatedTextures = NIBOOL_IS_TRUE(bValue);
    }
    NiStreamLoadBinary(kStream, bValue);
    m_bWorldSpace = NIBOOL_IS_TRUE(bValue);

    if (kStream.GetFileVersion() >= NiStream::GetVersion(20, 6, 1, 0))
    {
        NiStreamLoadEnum(kStream, m_eNormalMethod);
        NiStreamLoadBinary(kStream, m_kNormalDirection.x);
        NiStreamLoadBinary(kStream, m_kNormalDirection.y);
        NiStreamLoadBinary(kStream, m_kNormalDirection.z);
        NiStreamLoadEnum(kStream, m_eUpMethod);
        NiStreamLoadBinary(kStream, m_kUpDirection.x);
        NiStreamLoadBinary(kStream, m_kUpDirection.y);
        NiStreamLoadBinary(kStream, m_kUpDirection.z);

        m_pkLivingSpawner = (NiPSSpawner*) kStream.ResolveLinkID();

        NiUInt8 ucNumSpawnRateKeys = 0;
        NiStreamLoadBinary(kStream, ucNumSpawnRateKeys);

        AllocSpawnRateKeys(ucNumSpawnRateKeys);
        for (NiUInt8 uc = 0; uc < m_ucNumSpawnRateKeys; ++uc)
        {
            NiStreamLoadBinary(kStream, m_pkSpawnRateKeys[uc].m_fValue);
            NiStreamLoadBinary(kStream, m_pkSpawnRateKeys[uc].m_fTime);
        }
    }

    if (kStream.GetFileVersion() >= NiStream::GetVersion(30, 0, 0, 2))
    {
        NiStreamLoadBinary(kStream, m_bPreRPIParticleSystem);
    }
    // If the compute direction is added to a 2.6x version we would need to make sure
    // Max has been set with the correct value of true.
    else if (kStream.GetFileVersion() == NiStream::GetVersion(20, 6, 1, 0))
    {
        m_bPreRPIParticleSystem = false;
    }
    else
    {
        m_bPreRPIParticleSystem = true;
    }

    if (kStream.GetFileVersion() >= NiStream::GetVersion(30, 2, 0, 4))
    {
        NiStreamLoadBinary(kStream, m_bSorted);
    }

    AllocateDataBuffers(m_uiMaxNumParticles, (m_pkLivingSpawner != NULL), bHasColors, bHasRotations,
        bHasRotationAxes, bHasAnimatedTextures);
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::LinkObject(NiStream& kStream)
{
    NiMesh::LinkObject(kStream);

    // Handle back compatibility for colors on emitters.
    if (HasColors() && kStream.GetFileVersion() < NiStream::GetVersion(20, 6, 1, 0))
    {
        NiPSSimulatorGeneralStep* pkGeneralStep = (NiPSSimulatorGeneralStep*)
            m_pkSimulator->GetSimulatorStepByType(&NiPSSimulatorGeneralStep::ms_RTTI);
        EE_ASSERT(pkGeneralStep);

        NiUInt8 ucNumColorKeys;
        NiPSKernelColorKey* pkColorKeys = pkGeneralStep->GetColorKeys(ucNumColorKeys);

        // If there are already color keys, do nothing.
        if (!pkColorKeys)
        {
            for (NiUInt32 ui = 0; ui < GetEmitterCount(); ++ui)
            {
                NiPSEmitter* pkEmitter = GetEmitterAt(ui);
                if (!pkEmitter)
                    continue;

                NiPSKernelColorKey kKey;
                kKey.m_fTime = 0.0f;
                kKey.m_kColor = pkEmitter->GetColorBackCompat();
                pkGeneralStep->CopyColorKeys(&kKey, 1);
                break;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
bool NiPSParticleSystem::RegisterStreamables(NiStream& kStream)
{
    // We must complete all modifiers before accessing the particle data.
    ForceSimulationToComplete();

    if (!NiMesh::RegisterStreamables(kStream))
    {
        return false;
    }

    if (m_spBoundUpdater)
    {
        m_spBoundUpdater->RegisterStreamables(kStream);
    }

    const NiUInt32 uiEmittersCount = m_kEmitters.GetSize();
    for (NiUInt32 ui = 0; ui < uiEmittersCount; ++ui)
    {
        m_kEmitters.GetAt(ui)->RegisterStreamables(kStream);
    }

    const NiUInt32 uiSpawnersCount = m_kSpawners.GetSize();
    for (NiUInt32 ui = 0; ui < uiSpawnersCount; ++ui)
    {
        m_kSpawners.GetAt(ui)->RegisterStreamables(kStream);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::SaveBinary(NiStream& kStream)
{
    // We must complete all modifiers before accessing the particle data.
    ForceSimulationToComplete();

    NiMesh::SaveBinary(kStream);

    kStream.SaveLinkID(m_pkSimulator);

    kStream.SaveLinkID(m_spBoundUpdater);

    const NiUInt32 uiEmittersCount = m_kEmitters.GetSize();
    NiStreamSaveBinary(kStream, uiEmittersCount);
    for (NiUInt32 ui = 0; ui < uiEmittersCount; ++ui)
    {
        kStream.SaveLinkID(m_kEmitters.GetAt(ui));
    }

    const NiUInt32 uiSpawnersCount = m_kSpawners.GetSize();
    NiStreamSaveBinary(kStream, uiSpawnersCount);
    for (NiUInt32 ui = 0; ui < uiSpawnersCount; ++ui)
    {
        kStream.SaveLinkID(m_kSpawners.GetAt(ui));
    }

    kStream.SaveLinkID(m_pkDeathSpawner);

    NiStreamSaveBinary(kStream, m_uiMaxNumParticles);

    NiStreamSaveBinary(kStream, NiBool(HasColors()));
    NiStreamSaveBinary(kStream, NiBool(HasRotations()));
    NiStreamSaveBinary(kStream, NiBool(HasRotationAxes()));
    NiStreamSaveBinary(kStream, NiBool(HasAnimatedTextures()));
    NiStreamSaveBinary(kStream, NiBool(m_bWorldSpace));

    NiStreamSaveEnum(kStream, m_eNormalMethod);
    NiStreamSaveBinary(kStream, m_kNormalDirection.x);
    NiStreamSaveBinary(kStream, m_kNormalDirection.y);
    NiStreamSaveBinary(kStream, m_kNormalDirection.z);
    NiStreamSaveEnum(kStream, m_eUpMethod);
    NiStreamSaveBinary(kStream, m_kUpDirection.x);
    NiStreamSaveBinary(kStream, m_kUpDirection.y);
    NiStreamSaveBinary(kStream, m_kUpDirection.z);

    kStream.SaveLinkID(m_pkLivingSpawner);

    NiStreamSaveBinary(kStream, m_ucNumSpawnRateKeys);
    for (NiUInt8 uc = 0; uc < m_ucNumSpawnRateKeys; ++uc)
    {
        NiStreamSaveBinary(kStream, m_pkSpawnRateKeys[uc].m_fValue);
        NiStreamSaveBinary(kStream, m_pkSpawnRateKeys[uc].m_fTime);
    }

    NiStreamSaveBinary(kStream, m_bPreRPIParticleSystem);
    NiStreamSaveBinary(kStream, m_bSorted);
}

//--------------------------------------------------------------------------------------------------
bool NiPSParticleSystem::IsEqual(NiObject* pkObject)
{
    // We must complete all modifiers before accessing the particle data.
    ForceSimulationToComplete();

    if (!NiMesh::IsEqual(pkObject))
    {
        return false;
    }

    NiPSParticleSystem* pkDest = (NiPSParticleSystem*) pkObject;

    if ((pkDest->m_pkSimulator && !m_pkSimulator) ||
        (!pkDest->m_pkSimulator && m_pkSimulator))
    {
        return false;
    }

    if ((pkDest->m_spBoundUpdater && !m_spBoundUpdater) ||
        (!pkDest->m_spBoundUpdater && m_spBoundUpdater) ||
        (pkDest->m_spBoundUpdater &&
            !pkDest->m_spBoundUpdater->IsEqual(m_spBoundUpdater)))
    {
        return false;
    }

    if (pkDest->m_kEmitters.GetSize() != m_kEmitters.GetSize())
    {
        return false;
    }

    const NiUInt32 uiEmittersCount = pkDest->m_kEmitters.GetSize();
    for (NiUInt32 ui = 0; ui < uiEmittersCount; ++ui)
    {
        if (!pkDest->m_kEmitters.GetAt(ui)->IsEqual(m_kEmitters.GetAt(ui)))
        {
            return false;
        }
    }

    if (pkDest->m_kSpawners.GetSize() != pkDest->m_kSpawners.GetSize())
    {
        return false;
    }

    const NiUInt32 uiSpawnersCount = pkDest->m_kSpawners.GetSize();
    for (NiUInt32 ui = 0; ui < uiSpawnersCount; ++ui)
    {
        if (!pkDest->m_kSpawners.GetAt(ui)->IsEqual(m_kSpawners.GetAt(ui)))
        {
            return false;
        }
    }

    if ((pkDest->m_pkDeathSpawner && !m_pkDeathSpawner) ||
        (!pkDest->m_pkDeathSpawner && m_pkDeathSpawner))
    {
        return false;
    }

    if ((pkDest->m_pkLivingSpawner && !m_pkLivingSpawner) ||
        (!pkDest->m_pkLivingSpawner && m_pkLivingSpawner))
    {
        return false;
    }

    if (pkDest->m_uiMaxNumParticles != m_uiMaxNumParticles ||
        pkDest->m_bWorldSpace != m_bWorldSpace)
    {
        return false;
    }

    if ((pkDest->m_pkPositions && !m_pkPositions) ||
        (!pkDest->m_pkPositions && m_pkPositions) ||
        (pkDest->m_pkVelocities && !m_pkVelocities) ||
        (!pkDest->m_pkVelocities && m_pkVelocities) ||
        (pkDest->m_pfAges && !m_pfAges) ||
        (!pkDest->m_pfAges && m_pfAges) ||
        (pkDest->m_pfLifeSpans && !m_pfLifeSpans) ||
        (!pkDest->m_pfLifeSpans && m_pfLifeSpans) ||
        (pkDest->m_pfLastUpdateTimes && !m_pfLastUpdateTimes) ||
        (!pkDest->m_pfLastUpdateTimes && m_pfLastUpdateTimes) ||
        (pkDest->m_pfNextSpawnTimes && !m_pfNextSpawnTimes) ||
        (!pkDest->m_pfNextSpawnTimes && m_pfNextSpawnTimes) ||
        (pkDest->m_puiFlags && !m_puiFlags) ||
        (!pkDest->m_puiFlags && m_puiFlags) ||
        (pkDest->m_pfInitialSizes && !m_pfInitialSizes) ||
        (!pkDest->m_pfInitialSizes && m_pfInitialSizes) ||
        (pkDest->m_pfSizes && !m_pfSizes) ||
        (!pkDest->m_pfSizes && m_pfSizes) ||
        (pkDest->m_pkColors && !m_pkColors) ||
        (!pkDest->m_pkColors && m_pkColors) ||
        (pkDest->m_pfInitialRotationAngles && !m_pfInitialRotationAngles) ||
        (!pkDest->m_pfInitialRotationAngles && m_pfInitialRotationAngles) ||
        (pkDest->m_pfRotationAngles && !m_pfRotationAngles) ||
        (!pkDest->m_pfRotationAngles && m_pfRotationAngles) ||
        (pkDest->m_pfRotationSpeeds && !m_pfRotationSpeeds) ||
        (!pkDest->m_pfRotationSpeeds && m_pfRotationSpeeds) ||
        (pkDest->m_pkRotationAxes && !m_pkRotationAxes) ||
        (!pkDest->m_pkRotationAxes && m_pkRotationAxes) ||
        (pkDest->m_pfVariance && !m_pfVariance) ||
        (!pkDest->m_pfVariance && m_pfVariance))
    {
        return false;
    }

    if (pkDest->m_eNormalMethod != m_eNormalMethod ||
        pkDest->m_kNormalDirection != m_kNormalDirection ||
        pkDest->m_eUpMethod != m_eUpMethod ||
        pkDest->m_kUpDirection != m_kUpDirection)
    {
        return false;
    }

    if (pkDest->m_ucNumSpawnRateKeys != m_ucNumSpawnRateKeys)
    {
        return false;
    }

    for (NiUInt8 uc = 0; uc < m_ucNumSpawnRateKeys; ++uc)
    {
        if (pkDest->m_pkSpawnRateKeys[uc].m_fValue != m_pkSpawnRateKeys[uc].m_fValue ||
            pkDest->m_pkSpawnRateKeys[uc].m_fTime != m_pkSpawnRateKeys[uc].m_fTime)
        {
            return false;
        }
    }

    if (pkDest->m_bPreRPIParticleSystem != m_bPreRPIParticleSystem)
        return false;

    if (pkDest->m_bSorted != m_bSorted)
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiMesh::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSParticleSystem::ms_RTTI.GetName()));

    pkStrings->Add(NiGetViewerString("Simulator", m_pkSimulator));
    pkStrings->Add(NiGetViewerString("BoundUpdater", m_spBoundUpdater));
    pkStrings->Add(NiGetViewerString("NumEmitters", m_kEmitters.GetSize()));
    pkStrings->Add(NiGetViewerString("NumSpawners", m_kSpawners.GetSize()));
    pkStrings->Add(NiGetViewerString("DeathSpawner", m_pkDeathSpawner));
    pkStrings->Add(NiGetViewerString("LivingSpawner", m_pkLivingSpawner));
    pkStrings->Add(NiGetViewerString("MaxNumParticles", m_uiMaxNumParticles));
    pkStrings->Add(NiGetViewerString("NumParticles", m_uiNumParticles));
    pkStrings->Add(NiGetViewerString("HasColors", HasColors()));
    pkStrings->Add(NiGetViewerString("HasRotations", HasRotations()));
    pkStrings->Add(NiGetViewerString("HasRotationAxes", HasRotationAxes()));
    pkStrings->Add(NiGetViewerString("HasAnimatedTextures", HasAnimatedTextures()));
    pkStrings->Add(NiGetViewerString("NormalMethod", GetMethodString(m_eNormalMethod)));
    pkStrings->Add(m_kNormalDirection.GetViewerString("NormalDirection"));
    pkStrings->Add(NiGetViewerString("UpMethod", GetMethodString(m_eUpMethod)));
    pkStrings->Add(m_kUpDirection.GetViewerString("UpDirection"));
    pkStrings->Add(NiGetViewerString("HasAnimatedTextures", HasAnimatedTextures()));
    pkStrings->Add(NiGetViewerString("WorldSpace", m_bWorldSpace));
    pkStrings->Add(NiGetViewerString("Sorted", m_bSorted));

    NiUInt8 ucNumSpawnRateKeys;
    NiPSKernelFloatKey* pkSpawnRateKeys = GetSpawnRateKeys(ucNumSpawnRateKeys);
    if (pkSpawnRateKeys)
    {
        pkStrings->Add(NiGetViewerString("Num Spawn Rate Keys", ucNumSpawnRateKeys));
        for (NiUInt8 uc = 0; uc < ucNumSpawnRateKeys; uc++)
        {
            pkStrings->Add(NiGetViewerString("Spawn Rate Key Time", pkSpawnRateKeys[uc].m_fTime));
            pkStrings->Add(NiGetViewerString("Spawn Rate Key Value", pkSpawnRateKeys[uc].m_fValue));
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::AddSpawnerTargettingThisPS(NiPSSpawner* pkSpawner)
{
    m_kSpawnersIntoThisSystem.Add(pkSpawner);
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::RemoveSpawnerTargettingThisPS(NiPSSpawner* pkSpawner)
{
    int position = m_kSpawnersIntoThisSystem.Find(pkSpawner);
    m_kSpawnersIntoThisSystem.RemoveAt(position);
}

//--------------------------------------------------------------------------------------------------
void NiPSParticleSystem::RemoveAllTargettingSpawners()
{
    const NiUInt32 uiSpawnerCount = m_kSpawnersIntoThisSystem.GetSize();
    for (NiUInt32 ui = 0; ui < uiSpawnerCount; ++ui)
    {
        NiPSSpawner* pkSpawner = m_kSpawnersIntoThisSystem.GetAt(ui);
        pkSpawner->RemoveMasterPSystem(this);
    }
    m_kSpawnersIntoThisSystem.RemoveAll();
}

//--------------------------------------------------------------------------------------------------
