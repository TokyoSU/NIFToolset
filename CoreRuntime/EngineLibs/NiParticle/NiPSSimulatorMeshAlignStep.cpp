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

#include "NiPSSimulatorMeshAlignStep.h"
#include "NiPSSimulatorMeshAlignKernelFF.h"
#include "NiPSSimulatorMeshAlignKernelFV.h"
#include "NiPSSimulatorMeshAlignKernelVF.h"
#include "NiPSSimulatorMeshAlignKernelVV.h"
#include "NiPSSimulatorKernelHelpers.h"
#include "NiPSCommonSemantics.h"
#include "NiPSKernelDefinitions.h"
#include "NiPSMeshParticleSystem.h"

NiImplementRTTI(NiPSSimulatorMeshAlignStep, NiPSSimulatorStep);

//--------------------------------------------------------------------------------------------------
NiPSSimulatorMeshAlignStep::NiPSSimulatorMeshAlignStep(
    const float fScaleAmount, const float fScaleRest, const float fScaleLimit) :
    m_pkKernel(NULL),
    m_kInputStructIS(&m_kKernelStruct, 1),
    m_pkPositionIS(NULL),
    m_pkVelocityIS(NULL),
    m_pkRotAnglesIS(NULL),
    m_pkRotAxesIS(NULL),
    m_pkAgesIS(NULL),
    m_pkLifeSpansIS(NULL),
    m_pkRotationsOS(NULL),
    m_pkScalesOS(NULL),
    m_pkRotationKeys(NULL)
{
    m_kKernelStruct.m_ucNumRotationKeys = 0;
    m_kKernelStruct.m_eRotationLoopBehavior = PSKERNELLOOP_CLAMP_BIRTH;

    // normalize scale amount before storing in the kernel struct

    // Normalize stretching values and set in kernel
    if (fabs(fScaleLimit - fScaleRest) < NIPSKERNEL_EPSILON)
    {
        m_kKernelStruct.m_fScaleAmount = 0.0f;
    }
    else
    {
        m_kKernelStruct.m_fScaleAmount = fScaleAmount / (fScaleLimit - fScaleRest);
    }

    m_kKernelStruct.m_fScaleRest = fScaleRest;
    m_kKernelStruct.m_fScaleLimit = fScaleLimit;
}

//--------------------------------------------------------------------------------------------------
NiPSSimulatorMeshAlignStep::~NiPSSimulatorMeshAlignStep()
{
    NiAlignedFree(m_pkRotationKeys);
}

//--------------------------------------------------------------------------------------------------
NiSPKernel* NiPSSimulatorMeshAlignStep::GetKernel()
{
    return m_pkKernel;
}

//--------------------------------------------------------------------------------------------------
NiUInt16 NiPSSimulatorMeshAlignStep::GetLargestInputStride()
{
    return sizeof(NiPSKernelQuaternionKey);
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::PrepareInputStream(
    NiPSParticleSystem* pkParticleSystem,
    const NiFixedString& kSemantic,
    NiSPStream* pkStream)
{
    // Update the stored pointer to the stream.
    // Associate particle data with Floodgate streams.
    if (kSemantic == NiPSCommonSemantics::PARTICLEPOSITION())
    {
        if (m_pkPositionIS != pkStream)
        {
            NiDelete m_pkPositionIS;
            m_pkPositionIS = pkStream;
        }
        m_pkPositionIS->SetData(pkParticleSystem->GetPositions());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLEVELOCITY())
    {
        if (m_pkVelocityIS != pkStream)
        {
            NiDelete m_pkVelocityIS;
            m_pkVelocityIS = pkStream;
        }
        m_pkVelocityIS->SetData(pkParticleSystem->GetVelocities());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLEINITSIZE())
    {
        if (m_pkRadiiIS != pkStream)
        {
            NiDelete m_pkRadiiIS;
            m_pkRadiiIS = pkStream;
        }
        m_pkRadiiIS->SetData(pkParticleSystem->GetInitialSizes());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLESIZE())
    {
        if (m_pkSizesIS != pkStream)
        {
            NiDelete m_pkSizesIS;
            m_pkSizesIS = pkStream;
        }
        m_pkSizesIS->SetData(pkParticleSystem->GetSizes());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLEROTANGLE())
    {
        if (m_pkRotAnglesIS != pkStream)
        {
            NiDelete m_pkRotAnglesIS;
            m_pkRotAnglesIS = pkStream;
        }
        m_pkRotAnglesIS->SetData(pkParticleSystem->GetRotationAngles());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLEROTAXIS())
    {
        if (m_pkRotAxesIS != pkStream)
        {
            NiDelete m_pkRotAxesIS;
            m_pkRotAxesIS = pkStream;
        }
        m_pkRotAxesIS->SetData(pkParticleSystem->GetRotationAxes());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLEAGE())
    {
        if (m_pkAgesIS != pkStream)
        {
            NiDelete m_pkAgesIS;
            m_pkAgesIS = pkStream;
        }
        m_pkAgesIS->SetData(pkParticleSystem->GetAges());
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLELIFESPAN())
    {
        if (m_pkLifeSpansIS != pkStream)
        {
            NiDelete m_pkLifeSpansIS;
            m_pkLifeSpansIS = pkStream;
        }
        m_pkLifeSpansIS->SetData(pkParticleSystem->GetLifeSpans());
    }
    else
    {
        EE_FAIL("Unknown semantic type!");
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::PrepareOutputStream(
    NiPSParticleSystem* pkParticleSystem,
    const NiFixedString& kSemantic,
    NiSPStream* pkStream)
{
    EE_ASSERT(NiIsKindOf(NiPSMeshParticleSystem, pkParticleSystem));
    NiPSMeshParticleSystem* pkMeshPS = (NiPSMeshParticleSystem*)pkParticleSystem;

    // Update the stored pointer to the stream.
    // Associate particle data with Floodgate streams.
    if (kSemantic == NiPSCommonSemantics::PARTICLEROTATION())
    {
        EE_ASSERT(m_pkRotationsOS == pkStream);
        pkStream->SetData(pkMeshPS->GetRotations());
        pkStream->SetStride(sizeof(NiPoint3) * 3);
    }
    else if (kSemantic == NiPSCommonSemantics::PARTICLESCALE())
    {
        EE_ASSERT(m_pkScalesOS == pkStream);
        pkStream->SetData(pkMeshPS->GetScales());
    }
    else
    {
        EE_FAIL("Unknown semantic type!");
    }
}

//--------------------------------------------------------------------------------------------------
NiSPTaskPtr NiPSSimulatorMeshAlignStep::Attach(NiPSParticleSystem* pkParticleSystem)
{
    EE_ASSERT(NiIsKindOf(NiPSMeshParticleSystem, pkParticleSystem));

    NiPSParticleSystem::AlignMethod eNormalMethod = pkParticleSystem->GetNormalMethod();
    NiPSParticleSystem::AlignMethod eUpMethod = pkParticleSystem->GetUpMethod();
    if ((eNormalMethod & NiPSParticleSystem::ALIGN_CAMERA) ||
        (eUpMethod & NiPSParticleSystem::ALIGN_CAMERA))
    {
        EE_FAIL("Mesh particles cannot be aligned to the camera.\n");
        return NULL;
    }

    NiSPTaskPtr spTask = NiSPTask::GetNewTask(10, 2);

    // Create input streams.
    SetInputCount(8);
    m_pkPositionIS = NiNew NiTSPStream<NiPoint3>();
    AddInput(NiPSCommonSemantics::PARTICLEPOSITION(), m_pkPositionIS);
    m_pkVelocityIS = NiNew NiTSPStream<NiPoint3>();
    AddInput(NiPSCommonSemantics::PARTICLEVELOCITY(), m_pkVelocityIS);
    m_pkRadiiIS = NiNew NiTSPStream<float>();
    AddInput(NiPSCommonSemantics::PARTICLEINITSIZE(), m_pkRadiiIS);
    m_pkSizesIS = NiNew NiTSPStream<float>();
    AddInput(NiPSCommonSemantics::PARTICLESIZE(), m_pkSizesIS);
    m_pkRotAnglesIS = NiNew NiTSPStream<float>();
    AddInput(NiPSCommonSemantics::PARTICLEROTANGLE(), m_pkRotAnglesIS);
    m_pkRotAxesIS = NiNew NiTSPStream<NiPoint3>();
    AddInput(NiPSCommonSemantics::PARTICLEROTAXIS(), m_pkRotAxesIS);
    m_pkAgesIS = NiNew NiTSPStream<float>();
    AddInput(NiPSCommonSemantics::PARTICLEAGE(), m_pkAgesIS);
    m_pkLifeSpansIS = NiNew NiTSPStream<float>();
    AddInput(NiPSCommonSemantics::PARTICLELIFESPAN(), m_pkLifeSpansIS);

    // Create output streams.
    SetOutputCount(2);
    m_pkRotationsOS = NiNew NiTSPStream<NiPoint3>();
    m_pkRotationsOS->SetStride(sizeof(NiPoint3) * 3);
    AddOutput(NiPSCommonSemantics::PARTICLEROTATION(), m_pkRotationsOS);
    m_pkScalesOS = NiNew NiTSPStream<float>();
    AddOutput(NiPSCommonSemantics::PARTICLESCALE(), m_pkScalesOS);

    // Add local streams to the task.
    spTask->AddInput(&m_kInputStructIS);
    spTask->AddInput(&m_kRotationKeyIS);

    // Allocate the kernel
    if (eNormalMethod & NiPSParticleSystem::ALIGN_PER_PARTICLE)
    {
        if (eUpMethod & NiPSParticleSystem::ALIGN_PER_PARTICLE)
        {
            m_pkKernel = NiNew NiPSSimulatorMeshAlignKernelVV();
        }
        else
        {
            m_pkKernel = NiNew NiPSSimulatorMeshAlignKernelVF();
        }
    }
    else
    {
        if (eUpMethod & NiPSParticleSystem::ALIGN_PER_PARTICLE)
        {
            m_pkKernel = NiNew NiPSSimulatorMeshAlignKernelFV();
        }
        else
        {
            m_pkKernel = NiNew NiPSSimulatorMeshAlignKernelFF();
        }
    }

    return spTask;
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::Detach(NiPSParticleSystem* pkParticleSystem)
{
    // Call base class version to ensure that streams and semantics arrays
    // are properly cleared.
    NiPSSimulatorStep::Detach(pkParticleSystem);

    // Clear out input stream pointers.
    m_pkPositionIS = NULL;
    m_pkVelocityIS = NULL;
    m_pkRadiiIS = NULL;
    m_pkSizesIS = NULL;
    m_pkRotAnglesIS = NULL;
    m_pkRotAxesIS = NULL;
    m_pkAgesIS = NULL;
    m_pkLifeSpansIS = NULL;

    // Clear out output stream pointers.
    m_pkRotationsOS = NULL;
    m_pkScalesOS = NULL;

    NiDelete m_pkKernel;
}

//--------------------------------------------------------------------------------------------------
bool NiPSSimulatorMeshAlignStep::Update(NiPSParticleSystem* pkParticleSystem, float fTime)
{
    EE_UNUSED_ARG(fTime);

    // Update block count for Floodgate streams to the number of active
    // particles.
    const NiUInt32 uiNumParticles = pkParticleSystem->GetNumParticles();
    m_pkPositionIS->SetBlockCount(uiNumParticles);
    m_pkVelocityIS->SetBlockCount(uiNumParticles);
    m_pkRadiiIS->SetBlockCount(uiNumParticles);
    m_pkSizesIS->SetBlockCount(uiNumParticles);
    m_pkRotAnglesIS->SetBlockCount(uiNumParticles);
    m_pkRotAxesIS->SetBlockCount(uiNumParticles);
    m_pkAgesIS->SetBlockCount(uiNumParticles);
    m_pkLifeSpansIS->SetBlockCount(uiNumParticles);
    m_pkRotationsOS->SetBlockCount(uiNumParticles);
    m_pkScalesOS->SetBlockCount(uiNumParticles);

    if (m_kKernelStruct.m_ucNumRotationKeys > 0)
    {
        // Set up color key stream.
        m_kRotationKeyIS.SetData(m_pkRotationKeys);
        m_kRotationKeyIS.SetBlockCount(m_kKernelStruct.m_ucNumRotationKeys);
    }

    //
    // Set up input struct.
    //
    PopulateKernelStruct(pkParticleSystem);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::PopulateKernelStruct(NiPSParticleSystem* pkParticleSystem)
{
    NiPSParticleSystem::AlignMethod eNormalMethod = pkParticleSystem->GetNormalMethod();
    NiPSParticleSystem::AlignMethod eUpMethod = pkParticleSystem->GetUpMethod();

    NiPoint3 kUpDirection = pkParticleSystem->GetUpDirection();
    NiPoint3 kNormalDirection = pkParticleSystem->GetNormalDirection();

    // Set the transforms to apply to the alignment directions.
    if (pkParticleSystem->GetWorldSpace())
    {
        switch (eNormalMethod)
        {
            case NiPSParticleSystem::ALIGN_LOCAL_FIXED:
            {
                // Transform the direction from local to world coordinates. Don't translate.
                m_kKernelStruct.m_kNormal =
                    pkParticleSystem->GetOriginalWorldTransform().m_Rotate * kNormalDirection;
                m_kKernelStruct.m_kNormal.Unitize(NiPoint3::UNIT_X);
            } break;

            case NiPSParticleSystem::ALIGN_LOCAL_POSITION:
            {
                // Position in the array is in world space, so it needs to be converted to
                // local space for use as the alignment direction, but then that direction needs
                // to be converted back into world space because the particle system transform
                // will be the identity except for scaling. THe following transform does the job.
                m_kKernelStruct.m_kNormalTransform.m_Rotate.MakeIdentity();
                m_kKernelStruct.m_kNormalTransform.m_fScale =
                    1.0f / pkParticleSystem->GetOriginalWorldTransform().m_fScale;
                m_kKernelStruct.m_kNormalTransform.m_Translate =
                    pkParticleSystem->GetOriginalWorldTransform().m_Translate *
                    -m_kKernelStruct.m_kNormalTransform.m_fScale;
                m_kKernelStruct.m_bNormalUsePosition = true;
            } break;

            case NiPSParticleSystem::ALIGN_LOCAL_VELOCITY:
            {
                // Velocity in the array is in world space, so it needs to be converted to
                // local space for use as the alignment direction, but then that direction needs
                // to be converted back into world space because the particle system transform
                // will be the identity except for scaling. But note that we should not include
                // and translation in computing the velocity.
                m_kKernelStruct.m_kNormalTransform.m_Rotate.MakeIdentity();
                m_kKernelStruct.m_kNormalTransform.m_fScale =
                    1.0f / pkParticleSystem->GetOriginalWorldTransform().m_fScale;
                m_kKernelStruct.m_kNormalTransform.m_Translate = NiPoint3::ZERO;
                m_kKernelStruct.m_bNormalUsePosition = false;
            } break;

            default: // ALIGN_CAMERA not valid
                break;
        }

        switch (eUpMethod)
        {
            case NiPSParticleSystem::ALIGN_LOCAL_FIXED:
            {
                // Transform the direction from local to world coordinates. Don't translate.
                m_kKernelStruct.m_kUp =
                    pkParticleSystem->GetOriginalWorldTransform().m_Rotate * kUpDirection;
                m_kKernelStruct.m_kUp.Unitize(NiPoint3::UNIT_Z);
            } break;

            case NiPSParticleSystem::ALIGN_LOCAL_POSITION:
            {
                // Position in the array is in world space, so it needs to be converted to
                // local space for use as the alignment direction, but then that direction needs
                // to be converted back into world space because the particle system transform
                // will be the identity except for scaling. THe following transform does the job.
                m_kKernelStruct.m_kUpTransform.m_Rotate.MakeIdentity();
                m_kKernelStruct.m_kUpTransform.m_fScale =
                    1.0f / pkParticleSystem->GetOriginalWorldTransform().m_fScale;
                m_kKernelStruct.m_kUpTransform.m_Translate =
                    pkParticleSystem->GetOriginalWorldTransform().m_Translate *
                    -m_kKernelStruct.m_kUpTransform.m_fScale;
                m_kKernelStruct.m_bUpUsePosition = true;
            } break;

            case NiPSParticleSystem::ALIGN_LOCAL_VELOCITY:
            {
                // Velocity in the array is in world space, so it needs to be converted to
                // local space for use as the alignment direction, but then that direction needs
                // to be converted back into world space because the particle system transform
                // will be the identity except for scaling. But note that we should not include
                // and translation in computing the velocity.
                m_kKernelStruct.m_kUpTransform.m_Rotate.MakeIdentity();
                m_kKernelStruct.m_kUpTransform.m_fScale =
                    1.0f / pkParticleSystem->GetOriginalWorldTransform().m_fScale;
                m_kKernelStruct.m_kUpTransform.m_Translate = NiPoint3::ZERO;
                m_kKernelStruct.m_bUpUsePosition = false;
            } break;

            default: // ALIGN_CAMERA not valid
                break;
        }
    }
    else
    {
        switch (eNormalMethod)
        {
            case NiPSParticleSystem::ALIGN_LOCAL_FIXED:
            {
                // Normal is already in correct coordinate system
                m_kKernelStruct.m_kNormal = kNormalDirection;
                m_kKernelStruct.m_kNormal.Unitize(NiPoint3::UNIT_X);
                break;
            }

            case NiPSParticleSystem::ALIGN_LOCAL_POSITION:
            {
                m_kKernelStruct.m_kNormalTransform.MakeIdentity();
                m_kKernelStruct.m_bNormalUsePosition = true;
            } break;

            case NiPSParticleSystem::ALIGN_LOCAL_VELOCITY:
            {
                m_kKernelStruct.m_kNormalTransform.MakeIdentity();
                m_kKernelStruct.m_bNormalUsePosition = false;
            } break;

            default: // ALIGN_CAMERA not valid
                break;
        }

        switch (eUpMethod)
        {
            case NiPSParticleSystem::ALIGN_LOCAL_FIXED:
            {
                // Normal is already in correct coordinate system
                m_kKernelStruct.m_kUp = kUpDirection;
                m_kKernelStruct.m_kUp.Unitize(NiPoint3::UNIT_Z);
                break;
            }

            case NiPSParticleSystem::ALIGN_LOCAL_POSITION:
            {
                m_kKernelStruct.m_kUpTransform.MakeIdentity();
                m_kKernelStruct.m_bUpUsePosition = true;
            } break;

            case NiPSParticleSystem::ALIGN_LOCAL_VELOCITY:
            {
                m_kKernelStruct.m_kUpTransform.MakeIdentity();
                m_kKernelStruct.m_bUpUsePosition = false;
            } break;

            default: // ALIGN_CAMERA not valid
                break;
        }
    }

}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::InitializeNewParticle(NiPSMeshParticleSystem* pkParticleSystem,
    NiUInt32 uiIndex)
{
    PopulateKernelStruct(pkParticleSystem);

    NiPSParticleSystem::AlignMethod eNormalMethod = pkParticleSystem->GetNormalMethod();
    NiPSParticleSystem::AlignMethod eUpMethod = pkParticleSystem->GetUpMethod();

    if (eNormalMethod & NiPSParticleSystem::ALIGN_PER_PARTICLE)
    {
        const NiPoint3 kNormal = m_kKernelStruct.m_bNormalUsePosition ?
            pkParticleSystem->GetPositions()[uiIndex] :
            pkParticleSystem->GetVelocities()[uiIndex];

        NiPoint3 kNormalXformed = m_kKernelStruct.m_kNormalTransform * kNormal;
        kNormalXformed.Unitize(NiPoint3::UNIT_X);

        if (eUpMethod & NiPSParticleSystem::ALIGN_PER_PARTICLE)
        {
            const NiPoint3 kUp = m_kKernelStruct.m_bUpUsePosition ?
                pkParticleSystem->GetPositions()[uiIndex] :
                pkParticleSystem->GetVelocities()[uiIndex];

            NiPoint3 kUpXformed = m_kKernelStruct.m_kUpTransform * kUp;
            kUpXformed.Unitize(NiPoint3::UNIT_Z);

            if (pkParticleSystem->HasRotations())
            {
                NiPSSimulatorKernelHelpers::UpdateMeshAlignmentVVR(
                    pkParticleSystem->GetRotations()[uiIndex * 3],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 1],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 2],
                    pkParticleSystem->GetScales()[uiIndex],
                    kNormalXformed, kUpXformed,
                    pkParticleSystem->GetVelocities()[uiIndex],
                    pkParticleSystem->GetInitialSizes()[uiIndex],
                    pkParticleSystem->GetSizes()[uiIndex],
                    pkParticleSystem->GetRotationAngles()[uiIndex],
                    pkParticleSystem->GetRotationAxes()[uiIndex],
                    pkParticleSystem->GetAges()[uiIndex],
                    pkParticleSystem->GetLifeSpans()[uiIndex],
                    m_kKernelStruct.m_ucNumRotationKeys,
                    m_pkRotationKeys,
                    m_kKernelStruct.m_eRotationLoopBehavior,
                    m_kKernelStruct.m_fScaleAmount,
                    m_kKernelStruct.m_fScaleRest,
                    m_kKernelStruct.m_fScaleLimit);
            }
            else
            {
                NiPSSimulatorKernelHelpers::UpdateMeshAlignmentVV(
                    pkParticleSystem->GetRotations()[uiIndex * 3],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 1],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 2],
                    pkParticleSystem->GetScales()[uiIndex],
                    kNormalXformed, kUpXformed,
                    pkParticleSystem->GetVelocities()[uiIndex],
                    pkParticleSystem->GetInitialSizes()[uiIndex],
                    pkParticleSystem->GetSizes()[uiIndex],
                    m_kKernelStruct.m_fScaleAmount,
                    m_kKernelStruct.m_fScaleRest,
                    m_kKernelStruct.m_fScaleLimit);
            }
        }
        else
        {
            if (pkParticleSystem->HasRotations())
            {
                NiPSSimulatorKernelHelpers::UpdateMeshAlignmentVVR(
                    pkParticleSystem->GetRotations()[uiIndex * 3],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 1],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 2],
                    pkParticleSystem->GetScales()[uiIndex],
                    kNormalXformed, m_kKernelStruct.m_kUp,
                    pkParticleSystem->GetVelocities()[uiIndex],
                    pkParticleSystem->GetInitialSizes()[uiIndex],
                    pkParticleSystem->GetSizes()[uiIndex],
                    pkParticleSystem->GetRotationAngles()[uiIndex],
                    pkParticleSystem->GetRotationAxes()[uiIndex],
                    pkParticleSystem->GetAges()[uiIndex],
                    pkParticleSystem->GetLifeSpans()[uiIndex],
                    m_kKernelStruct.m_ucNumRotationKeys,
                    m_pkRotationKeys,
                    m_kKernelStruct.m_eRotationLoopBehavior,
                    m_kKernelStruct.m_fScaleAmount,
                    m_kKernelStruct.m_fScaleRest,
                    m_kKernelStruct.m_fScaleLimit);
            }
            else
            {
                NiPSSimulatorKernelHelpers::UpdateMeshAlignmentVV(
                    pkParticleSystem->GetRotations()[uiIndex * 3],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 1],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 2],
                    pkParticleSystem->GetScales()[uiIndex],
                    kNormalXformed, m_kKernelStruct.m_kUp,
                    pkParticleSystem->GetVelocities()[uiIndex],
                    pkParticleSystem->GetInitialSizes()[uiIndex],
                    pkParticleSystem->GetSizes()[uiIndex],
                    m_kKernelStruct.m_fScaleAmount,
                    m_kKernelStruct.m_fScaleRest,
                    m_kKernelStruct.m_fScaleLimit);
            }
        }
    }
    else
    {
        if (eUpMethod & NiPSParticleSystem::ALIGN_PER_PARTICLE)
        {
            const NiPoint3 kUp = m_kKernelStruct.m_bUpUsePosition ?
                pkParticleSystem->GetPositions()[uiIndex] :
                pkParticleSystem->GetVelocities()[uiIndex];

            const NiPoint3 kUpXformed = m_kKernelStruct.m_kUpTransform * kUp;

            if (pkParticleSystem->HasRotations())
            {
                NiPSSimulatorKernelHelpers::UpdateMeshAlignmentVVR(
                    pkParticleSystem->GetRotations()[uiIndex * 3],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 1],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 2],
                    pkParticleSystem->GetScales()[uiIndex],
                    m_kKernelStruct.m_kNormal, kUpXformed,
                    pkParticleSystem->GetVelocities()[uiIndex],
                    pkParticleSystem->GetInitialSizes()[uiIndex],
                    pkParticleSystem->GetSizes()[uiIndex],
                    pkParticleSystem->GetRotationAngles()[uiIndex],
                    pkParticleSystem->GetRotationAxes()[uiIndex],
                    pkParticleSystem->GetAges()[uiIndex],
                    pkParticleSystem->GetLifeSpans()[uiIndex],
                    m_kKernelStruct.m_ucNumRotationKeys,
                    m_pkRotationKeys,
                    m_kKernelStruct.m_eRotationLoopBehavior,
                    m_kKernelStruct.m_fScaleAmount,
                    m_kKernelStruct.m_fScaleRest,
                    m_kKernelStruct.m_fScaleLimit);
            }
            else
            {
                NiPSSimulatorKernelHelpers::UpdateMeshAlignmentVV(
                    pkParticleSystem->GetRotations()[uiIndex * 3],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 1],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 2],
                    pkParticleSystem->GetScales()[uiIndex],
                    m_kKernelStruct.m_kNormal, kUpXformed,
                    pkParticleSystem->GetVelocities()[uiIndex],
                    pkParticleSystem->GetInitialSizes()[uiIndex],
                    pkParticleSystem->GetSizes()[uiIndex],
                    m_kKernelStruct.m_fScaleAmount,
                    m_kKernelStruct.m_fScaleRest,
                    m_kKernelStruct.m_fScaleLimit);
            }
        }
        else
        {
            NiPoint3 kRight = m_kKernelStruct.m_kUp.UnitCross(m_kKernelStruct.m_kNormal);
            NiPoint3 kUp = m_kKernelStruct.m_kNormal.Cross(kRight);

            NiMatrix3 kBasis(m_kKernelStruct.m_kNormal, kRight, kUp);

            if (pkParticleSystem->HasRotations())
            {
                NiPSSimulatorKernelHelpers::UpdateMeshAlignmentFFR(
                    pkParticleSystem->GetRotations()[uiIndex * 3],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 1],
                    pkParticleSystem->GetRotations()[uiIndex * 3 + 2],
                    pkParticleSystem->GetScales()[uiIndex],
                    kBasis,
                    pkParticleSystem->GetVelocities()[uiIndex],
                    pkParticleSystem->GetInitialSizes()[uiIndex],
                    pkParticleSystem->GetSizes()[uiIndex],
                    pkParticleSystem->GetRotationAngles()[uiIndex],
                    pkParticleSystem->GetRotationAxes()[uiIndex],
                    pkParticleSystem->GetAges()[uiIndex],
                    pkParticleSystem->GetLifeSpans()[uiIndex],
                    m_kKernelStruct.m_ucNumRotationKeys,
                    m_pkRotationKeys,
                    m_kKernelStruct.m_eRotationLoopBehavior,
                    m_kKernelStruct.m_fScaleAmount,
                    m_kKernelStruct.m_fScaleRest,
                    m_kKernelStruct.m_fScaleLimit);
            }
            else
            {
                NiPSSimulatorKernelHelpers::UpdateMeshAlignmentFF(
                    pkParticleSystem->GetScales()[uiIndex],
                    pkParticleSystem->GetVelocities()[uiIndex],
                    pkParticleSystem->GetInitialSizes()[uiIndex],
                    pkParticleSystem->GetSizes()[uiIndex],
                    m_kKernelStruct.m_fScaleAmount,
                    m_kKernelStruct.m_fScaleRest,
                    m_kKernelStruct.m_fScaleLimit);
                pkParticleSystem->GetRotations()[uiIndex * 3] = m_kKernelStruct.m_kNormal;
                pkParticleSystem->GetRotations()[uiIndex * 3 + 1] = kRight;
                pkParticleSystem->GetRotations()[uiIndex * 3 + 2] = kUp;
            }
        }
    }

}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::CopyRotationKeys(
    const NiPSKernelQuaternionKey* pkRotationKeys, const NiUInt8 ucNumKeys)
{

    EE_ASSERT((!pkRotationKeys && ucNumKeys == 0) || (pkRotationKeys && ucNumKeys > 0));

    if (ucNumKeys != m_kKernelStruct.m_ucNumRotationKeys)
    {
        NiAlignedFree(m_pkRotationKeys);
        m_pkRotationKeys = NiAlignedAlloc(NiPSKernelQuaternionKey, ucNumKeys, NIPSKERNEL_ALIGNMENT);
        m_kKernelStruct.m_ucNumRotationKeys = ucNumKeys;
    }
    NiMemcpy(m_pkRotationKeys, sizeof(NiPSKernelQuaternionKey) * ucNumKeys,
        pkRotationKeys, sizeof(NiPSKernelQuaternionKey) * ucNumKeys);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSSimulatorMeshAlignStep);

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::CopyMembers(NiPSSimulatorMeshAlignStep* pkDest,
    NiCloningProcess& kCloning)
{
    NiPSSimulatorStep::CopyMembers(pkDest, kCloning);

    pkDest->m_kKernelStruct.m_ucNumRotationKeys = m_kKernelStruct.m_ucNumRotationKeys;
    pkDest->m_kKernelStruct.m_eRotationLoopBehavior = m_kKernelStruct.m_eRotationLoopBehavior;

    pkDest->m_pkRotationKeys = NiAlignedAlloc(NiPSKernelQuaternionKey,
        m_kKernelStruct.m_ucNumRotationKeys, NIPSKERNEL_ALIGNMENT);
    for (NiUInt8 uc = 0; uc < m_kKernelStruct.m_ucNumRotationKeys; ++uc)
    {
        pkDest->m_pkRotationKeys[uc].m_fTime = m_pkRotationKeys[uc].m_fTime;
        pkDest->m_pkRotationKeys[uc].m_kValue = m_pkRotationKeys[uc].m_kValue;
    }
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSSimulatorMeshAlignStep);

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::LoadBinary(NiStream& kStream)
{
    NiPSSimulatorStep::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_kKernelStruct.m_ucNumRotationKeys);
    m_pkRotationKeys = NiAlignedAlloc(NiPSKernelQuaternionKey, m_kKernelStruct.m_ucNumRotationKeys,
        NIPSKERNEL_ALIGNMENT);
    for (NiUInt8 uc = 0; uc < m_kKernelStruct.m_ucNumRotationKeys; ++uc)
    {
        m_pkRotationKeys[uc].m_kValue.LoadBinary(kStream);
        NiStreamLoadBinary(kStream, m_pkRotationKeys[uc].m_fTime);
    }
    NiStreamLoadEnum(kStream, m_kKernelStruct.m_eRotationLoopBehavior);
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::LinkObject(NiStream& kStream)
{
    NiPSSimulatorStep::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSSimulatorMeshAlignStep::RegisterStreamables(NiStream& kStream)
{
    return NiPSSimulatorStep::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::SaveBinary(NiStream& kStream)
{
    NiPSSimulatorStep::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_kKernelStruct.m_ucNumRotationKeys);
    for (NiUInt8 uc = 0; uc < m_kKernelStruct.m_ucNumRotationKeys; ++uc)
    {
        m_pkRotationKeys[uc].m_kValue.SaveBinary(kStream);
        NiStreamSaveBinary(kStream, m_pkRotationKeys[uc].m_fTime);
    }
    NiStreamSaveEnum(kStream, m_kKernelStruct.m_eRotationLoopBehavior);
}

//--------------------------------------------------------------------------------------------------
bool NiPSSimulatorMeshAlignStep::IsEqual(NiObject* pkObject)
{
    if (!NiPSSimulatorStep::IsEqual(pkObject))
        return false;

    NiPSSimulatorMeshAlignStep* pkDest = (NiPSSimulatorMeshAlignStep*) pkObject;

    if (pkDest->m_kKernelStruct.m_eRotationLoopBehavior !=
            m_kKernelStruct.m_eRotationLoopBehavior ||
        pkDest->m_kKernelStruct.m_ucNumRotationKeys != m_kKernelStruct.m_ucNumRotationKeys)
    {
        return false;
    }

    for (NiUInt8 uc = 0; uc < m_kKernelStruct.m_ucNumRotationKeys; ++uc)
    {
        if (pkDest->m_pkRotationKeys[uc].m_kValue != m_pkRotationKeys[uc].m_kValue ||
            pkDest->m_pkRotationKeys[uc].m_fTime != m_pkRotationKeys[uc].m_fTime)
        {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiPSSimulatorMeshAlignStep::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiPSSimulatorStep::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSSimulatorMeshAlignStep::ms_RTTI
        .GetName()));

    NiUInt8 ucNumRotationKeys;
    NiPSKernelQuaternionKey* pkRotationKeys = GetRotationKeys(ucNumRotationKeys);
    pkStrings->Add(NiGetViewerString("Num Rotation Keys", ucNumRotationKeys));
    for (NiUInt8 uc = 0; uc < ucNumRotationKeys; uc++)
    {
        pkStrings->Add(NiGetViewerString("Rotation Key Time", pkRotationKeys[uc].m_fTime));
        pkStrings->Add(pkRotationKeys[uc].m_kValue.GetViewerString("Rotation Key Value"));
    }
    pkStrings->Add(NiGetViewerString("Rotation Loop Behavior",
        (NiUInt8)GetRotationLoopBehavior()));
}

//--------------------------------------------------------------------------------------------------
