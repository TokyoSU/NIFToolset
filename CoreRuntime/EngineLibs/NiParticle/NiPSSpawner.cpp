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

#include "NiPSSpawner.h"
#include "NiPSParticleSystem.h"
#include "NiPSFlagsHelpers.h"
#include "NiPSKernelDefinitions.h"

NiImplementRTTI(NiPSSpawner, NiObject);

const NiUInt16 NiPSSpawner::INVALID_ID = 0xFFF;

//--------------------------------------------------------------------------------------------------
NiPSSpawner::NiPSSpawner(
    NiUInt16 usNumSpawnGenerations,
    float fPercentageSpawned,
    NiUInt32 uiMinNumToSpawn,
    NiUInt32 uiMaxNumToSpawn,
    float fSpawnSpeedFactor,
    float fSpawnSpeedFactorVar,
    float fSpawnDirChaos,
    float fLifeSpan,
    float fLifeSpanVar,
    bool bRelativeSpeed) :
    m_pkMasterPSystem(NULL),
    m_fPercentageSpawned(fPercentageSpawned),
    m_fSpawnSpeedFactor(fSpawnSpeedFactor),
    m_fSpawnSpeedFactorVar(fSpawnSpeedFactorVar),
    m_fSpawnDirChaos(fSpawnDirChaos),
    m_fLifeSpan(fLifeSpan),
    m_fLifeSpanVar(fLifeSpanVar),
    m_uiMinNumToSpawn(uiMinNumToSpawn),
    m_uiMaxNumToSpawn(uiMaxNumToSpawn),
    m_usNumSpawnGenerations(usNumSpawnGenerations),
    m_bRelativeSpeed(bRelativeSpeed)
{
}

//--------------------------------------------------------------------------------------------------
NiPSSpawner::~NiPSSpawner()
{
    SetMasterPSystem(NULL);
}

//--------------------------------------------------------------------------------------------------
void NiPSSpawner::SetMasterPSystem(NiPSParticleSystem* pkMasterPSystem)
{
    if (m_pkMasterPSystem)
    {
        m_pkMasterPSystem->RemoveSpawnerTargettingThisPS(this);
    }

    m_pkMasterPSystem = pkMasterPSystem;
    if (m_pkMasterPSystem)
    {
        m_pkMasterPSystem->AddSpawnerTargettingThisPS(this);
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSSpawner::RemoveMasterPSystem(NiPSParticleSystem* pkMasterPSystem)
{
    EE_UNUSED_ARG(pkMasterPSystem);
    EE_ASSERT(pkMasterPSystem == m_pkMasterPSystem);

    m_pkMasterPSystem = NULL;
}

//--------------------------------------------------------------------------------------------------
void NiPSSpawner::SpawnParticles(
    float fCurrentTime,
    float fSpawnTime,
    NiUInt32 uiOldIndex,
    NiPSParticleSystem* pkOldPSystem)
{
    // Do not spawn if exceeded number of spawn generations or percentage
    // spawned.
    NiUInt32 uiFlags = pkOldPSystem->GetFlags()[uiOldIndex];
    NiUInt16 usOldGeneration = NiPSFlagsHelpers::GetGeneration(uiFlags);
    if (usOldGeneration >= m_usNumSpawnGenerations ||
        NiUnitRandom() > m_fPercentageSpawned)
    {
        return;
    }

    // Determine number to spawn.
    if (m_uiMaxNumToSpawn < m_uiMinNumToSpawn)
    {
        EE_FAIL("Spawner MaxNumToSpawn is less than MinNumToSpawn");
        // Avoid max < min since this will lock up the particle system
        m_uiMaxNumToSpawn = m_uiMinNumToSpawn;
    }

    NiUInt32 uiVariation = 0;
    if (m_uiMaxNumToSpawn != m_uiMinNumToSpawn)
    {
        float fVariation = NiUnitRandom() * (m_uiMaxNumToSpawn -
            m_uiMinNumToSpawn);
        uiVariation = (NiUInt32)(fVariation + 0.5f);
    }
    NiUInt32 uiNumToSpawn = m_uiMinNumToSpawn + uiVariation;
    if (uiNumToSpawn == 0)
    {
        uiNumToSpawn = 1;
    }

    // Spawn particles.
    for (NiUInt32 ui = 0; ui < uiNumToSpawn; ++ui)
    {
        SpawnParticle(fCurrentTime, fSpawnTime, uiOldIndex, pkOldPSystem);
    }
}

//--------------------------------------------------------------------------------------------------
void NiPSSpawner::SpawnParticle(
    float fCurrentTime,
    float fSpawnTime,
    NiUInt32 uiOldIndex,
    NiPSParticleSystem* pkOldPSystem)
{
    // We do not allow circular dependencies. m_pkMasterPSystem can be NULL.
    EE_ASSERT (m_pkMasterPSystem != pkOldPSystem);

    // If our master particle system is null we simply set it to be our
    // old one.
    NiPSParticleSystem* pkMasterPSystem = m_pkMasterPSystem;
    if (!pkMasterPSystem)
    {
        pkMasterPSystem = pkOldPSystem;
    }

    // Transform old position and velocity.
    NiPoint3 kOldPosition = pkOldPSystem->GetPositions()[uiOldIndex];
    NiPoint3 kOldVelocity = pkOldPSystem->GetVelocities()[uiOldIndex];
    if (pkOldPSystem != pkMasterPSystem)
    {
        NiTransform kOldXForm = pkOldPSystem->GetWorldTransform();
        kOldPosition = kOldXForm * kOldPosition;
        kOldVelocity = kOldXForm.m_Rotate * kOldVelocity;

        NiTransform kNewXForm = pkMasterPSystem->GetWorldTransform();
        NiTransform kInvNewXForm;
        kNewXForm.Invert(kInvNewXForm);
        kOldPosition = kInvNewXForm * kOldPosition;
        kOldVelocity = kInvNewXForm.m_Rotate * kOldVelocity;
    }

    // Get pointer to new particle data.
    NiUInt32 uiNewIndex = pkMasterPSystem->AddParticle();
    if (uiNewIndex == NiPSParticleSystem::INVALID_PARTICLE)
    {
        return;
    }

    // Calculate age based on spawn time.
    float fNewAge = (fCurrentTime - fSpawnTime);
    pkMasterPSystem->GetAges()[uiNewIndex] = fNewAge;

    // Calculate new velocity based on original and speed, direction chaos.
    float fOrigSpeed = kOldVelocity.Length();
    float fNewSpeed = m_fSpawnSpeedFactor;
    if (!pkMasterPSystem->GetPreRPIParticleSystem())
    {
        if (m_bRelativeSpeed)
        {
            // Add or subtract based on speed up/slow down factor.
            if (m_fSpawnSpeedFactorVar != 0.0f)
            {
                fNewSpeed = (fOrigSpeed *
                    (m_fSpawnSpeedFactor + m_fSpawnSpeedFactorVar * NiSymmetricRandom()));
            }
        }
    }
    else
    {
        // Add or subtract based on speed up/slow down factor.
        fNewSpeed = fOrigSpeed;
        if (m_fSpawnSpeedFactorVar != 0.0f)
        {
            fNewSpeed *= 1.0f + (m_fSpawnSpeedFactorVar * NiUnitRandom());
        }
    }

    // Calculate directional chaos.
    NiPoint3 kDirChaos(0.0f, 0.0f, 1.0f);
    if (m_fSpawnDirChaos != 0.0f)
    {
        float fDecChaos = NiUnitRandom() * m_fSpawnDirChaos * NI_PI;
        float fPlanChaos = NiUnitRandom() * NI_TWO_PI;

        float fSinDecChaos = NiSin(fDecChaos);
        kDirChaos = NiPoint3(fSinDecChaos * NiCos(fPlanChaos),
            fSinDecChaos * NiSin(fPlanChaos), NiCos(fDecChaos));
    }

    // Rotate directional chaos to line up with original direction.
    // Original direction = (x,y,z), length = l
    // Rotation: angle = acos(z/l), axis = (y,-x,0)
    float fX = kOldVelocity.x;
    float fY = kOldVelocity.y;
    NiMatrix3 kS(NiPoint3(0.0f,0.0f,fX),NiPoint3(0.0f,0.0f,fY),
        NiPoint3(-fX,-fY,0.0f));

    float fTestVal = (kDirChaos.Cross(kOldVelocity))
        * NiPoint3(fY, -fX, 0.0f);
    NiMatrix3 kRot = NiMatrix3::IDENTITY;
    if (fTestVal > NIPSKERNEL_EPSILON)
    {
        // fX or fY != 0
        kRot = kRot + kS * (kRot + kS *
            ((fOrigSpeed - kOldVelocity.z) /
            (fX * fX + fY * fY))) * (1.0f / fOrigSpeed);
    }
    else if (-fTestVal > NIPSKERNEL_EPSILON)
    {
        // fX or fY != 0
        kRot = kRot - kS * (kRot - kS *
            ((fOrigSpeed - kOldVelocity.z) /
            (fX * fX + fY * fY))) * (1.0f / fOrigSpeed);
    }
    else // fX and fY == 0.0f
    {
        if (kOldVelocity.z < 0.0f)
        {
            kRot = NiMatrix3(NiPoint3(-1.0f, 0.0f, 0.0f),
                NiPoint3(0.0f, -1.0f, 0.0f), NiPoint3(0.0f, 0.0f, -1.0f));
        }
    }

    NiMatrix3 kTempMat = kRot * fNewSpeed;
    pkMasterPSystem->GetVelocities()[uiNewIndex] = kTempMat * kDirChaos;

    // Calculate and set life span.
    float fNewLifeSpan = m_fLifeSpan + (m_fLifeSpanVar * (NiUnitRandom() - 0.5f));
    pkMasterPSystem->GetLifeSpans()[uiNewIndex] = fNewLifeSpan;

    // Set new generation value.
    NiUInt32 uiNewFlags = 0;
    if (pkOldPSystem == pkMasterPSystem)
    {
        NiUInt32 uiOldFlags = pkOldPSystem->GetFlags()[uiOldIndex];
        NiUInt16 usOldGeneration = NiPSFlagsHelpers::GetGeneration(uiOldFlags);
        NiPSFlagsHelpers::SetGeneration(uiNewFlags, usOldGeneration + 1);
    }
    pkMasterPSystem->GetFlags()[uiNewIndex] = uiNewFlags;

    // Propagate particle position.
    pkMasterPSystem->GetPositions()[uiNewIndex] = kOldPosition;

    // Make sure we set the next spawn time if we need to
    if (pkMasterPSystem->HasLivingSpawner())
    {
        pkMasterPSystem->GetNextSpawnTime(fCurrentTime, fNewAge, fNewLifeSpan,
            pkMasterPSystem->GetNextSpawnTimes()[uiNewIndex]);
    }

    // Propagate particle color.
    NiRGBA kOldColor = NiRGBA::WHITE;
    if (pkOldPSystem->HasColors())
    {
        kOldColor = pkOldPSystem->GetColors()[uiOldIndex];
    }
    if (pkMasterPSystem->HasColors())
    {
        pkMasterPSystem->GetColors()[uiNewIndex] = kOldColor;
    }

    // Propagate particle radius.
    float fOldSize = pkOldPSystem->GetSizes()[uiOldIndex];
    float fOldInitialSize = pkOldPSystem->GetInitialSizes()[uiOldIndex];
    pkMasterPSystem->GetInitialSizes()[uiNewIndex] = fOldSize * fOldInitialSize;
    pkMasterPSystem->GetSizes()[uiNewIndex] = 0.0f;

    // Propagate particle rotation angle.
    float fOldRotationAngle = 0.0f;
    float fOldRotationSpeed = 0.0f;
    if (pkOldPSystem->HasRotations())
    {
        fOldRotationAngle = pkOldPSystem->GetRotationAngles()[uiOldIndex];
        if (pkOldPSystem->HasRotationSpeeds())
            fOldRotationSpeed = pkOldPSystem->GetRotationSpeeds()[uiOldIndex];
    }
    if (pkMasterPSystem->HasRotations())
    {
        pkMasterPSystem->GetInitialRotationAngles()[uiNewIndex] = fOldRotationAngle;
        pkMasterPSystem->GetRotationAngles()[uiNewIndex] = fOldRotationAngle;
        if (pkMasterPSystem->HasRotationSpeeds())
            pkMasterPSystem->GetRotationSpeeds()[uiNewIndex] = fOldRotationSpeed;
    }

    // Propagate particle rotation axis.
    NiPoint3 kOldRotationAxis = NiPoint3::UNIT_X;
    if (pkOldPSystem->HasRotationAxes())
    {
        kOldRotationAxis = pkOldPSystem->GetRotationAxes()[uiOldIndex];
    }
    if (pkMasterPSystem->HasRotationAxes())
    {
        pkMasterPSystem->GetRotationAxes()[uiNewIndex] = kOldRotationAxis;
    }

    // Set new last update time.
    pkMasterPSystem->GetLastUpdateTimes()[uiNewIndex] = fCurrentTime -
        fNewAge;

    pkMasterPSystem->InitializeSpawnedParticle(uiNewIndex);
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSSpawner);

//--------------------------------------------------------------------------------------------------
void NiPSSpawner::CopyMembers(NiPSSpawner* pkDest,
    NiCloningProcess& kCloning)
{
    NiObject::CopyMembers(pkDest, kCloning);

    pkDest->m_fPercentageSpawned = m_fPercentageSpawned;
    pkDest->m_fSpawnSpeedFactor = m_fSpawnSpeedFactor;
    pkDest->m_fSpawnSpeedFactorVar = m_fSpawnSpeedFactorVar;
    pkDest->m_fSpawnDirChaos = m_fSpawnDirChaos;
    pkDest->m_fLifeSpan = m_fLifeSpan;
    pkDest->m_fLifeSpanVar = m_fLifeSpanVar;
    pkDest->m_usNumSpawnGenerations = m_usNumSpawnGenerations;
    pkDest->m_uiMinNumToSpawn = m_uiMinNumToSpawn;
    pkDest->m_uiMaxNumToSpawn = m_uiMaxNumToSpawn;
}

//--------------------------------------------------------------------------------------------------
void NiPSSpawner::ProcessClone(NiCloningProcess& kCloning)
{
    NiObject::ProcessClone(kCloning);

    NiObject* pkObject = NULL;
    EE_VERIFY(kCloning.m_pkCloneMap->GetAt(this, pkObject));
    NiPSSpawner* pkDest = (NiPSSpawner*) pkObject;

    if (kCloning.m_pkCloneMap->GetAt(m_pkMasterPSystem, pkObject))
    {
        pkDest->SetMasterPSystem(NiDynamicCast(NiPSParticleSystem, pkObject));
    }
    else
    {
        pkDest->SetMasterPSystem(m_pkMasterPSystem);
    }
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSSpawner);

//--------------------------------------------------------------------------------------------------
void NiPSSpawner::LoadBinary(NiStream& kStream)
{
    NiObject::LoadBinary(kStream);

    if (kStream.GetFileVersion() >= NiStream::GetVersion(20, 6, 1, 0))
    {
        SetMasterPSystem((NiPSParticleSystem*)kStream.ResolveLinkID());
    }

    NiStreamLoadBinary(kStream, m_fPercentageSpawned);
    if (kStream.GetFileVersion() >= NiStream::GetVersion(20, 6, 1, 0))
    {
        NiStreamLoadBinary(kStream, m_fSpawnSpeedFactor);
    }

    NiStreamLoadBinary(kStream, m_fSpawnSpeedFactorVar);
    NiStreamLoadBinary(kStream, m_fSpawnDirChaos);
    NiStreamLoadBinary(kStream, m_fLifeSpan);
    NiStreamLoadBinary(kStream, m_fLifeSpanVar);
    NiStreamLoadBinary(kStream, m_usNumSpawnGenerations);
    NiStreamLoadBinary(kStream, m_uiMinNumToSpawn);
    NiStreamLoadBinary(kStream, m_uiMaxNumToSpawn);
}

//--------------------------------------------------------------------------------------------------
void NiPSSpawner::LinkObject(NiStream& kStream)
{
    NiObject::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSSpawner::RegisterStreamables(NiStream& kStream)
{
    return NiObject::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSSpawner::SaveBinary(NiStream& kStream)
{
    NiObject::SaveBinary(kStream);

    kStream.SaveLinkID(m_pkMasterPSystem);

    NiStreamSaveBinary(kStream, m_fPercentageSpawned);
    NiStreamSaveBinary(kStream, m_fSpawnSpeedFactor);
    NiStreamSaveBinary(kStream, m_fSpawnSpeedFactorVar);
    NiStreamSaveBinary(kStream, m_fSpawnDirChaos);
    NiStreamSaveBinary(kStream, m_fLifeSpan);
    NiStreamSaveBinary(kStream, m_fLifeSpanVar);
    NiStreamSaveBinary(kStream, m_usNumSpawnGenerations);
    NiStreamSaveBinary(kStream, m_uiMinNumToSpawn);
    NiStreamSaveBinary(kStream, m_uiMaxNumToSpawn);
}

//--------------------------------------------------------------------------------------------------
bool NiPSSpawner::IsEqual(NiObject* pkObject)
{
    if (!NiObject::IsEqual(pkObject))
    {
        return false;
    }

    NiPSSpawner* pkDest = (NiPSSpawner*) pkObject;

    if (pkDest->m_fPercentageSpawned != m_fPercentageSpawned ||
        pkDest->m_fSpawnSpeedFactor != m_fSpawnSpeedFactor ||
        pkDest->m_fSpawnSpeedFactorVar != m_fSpawnSpeedFactorVar ||
        pkDest->m_fSpawnDirChaos != m_fSpawnDirChaos ||
        pkDest->m_fLifeSpan != m_fLifeSpan ||
        pkDest->m_fLifeSpanVar != m_fLifeSpanVar ||
        pkDest->m_usNumSpawnGenerations != m_usNumSpawnGenerations ||
        pkDest->m_uiMinNumToSpawn != m_uiMinNumToSpawn ||
        pkDest->m_uiMaxNumToSpawn != m_uiMaxNumToSpawn)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiPSSpawner::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiObject::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSSpawner::ms_RTTI.GetName()));

    pkStrings->Add(NiGetViewerString("PercentageSpawned",
        m_fPercentageSpawned));
    pkStrings->Add(NiGetViewerString("SpawnSpeedFactor", m_fSpawnSpeedFactor));
    pkStrings->Add(NiGetViewerString("SpawnSpeedFactorVar", m_fSpawnSpeedFactorVar));
    pkStrings->Add(NiGetViewerString("SpawnDirChaos", m_fSpawnDirChaos));
    pkStrings->Add(NiGetViewerString("LifeSpan", m_fLifeSpan));
    pkStrings->Add(NiGetViewerString("LifeSpanVar", m_fLifeSpanVar));
    pkStrings->Add(NiGetViewerString("NumSpawnGenerations",
        m_usNumSpawnGenerations));
    pkStrings->Add(NiGetViewerString("MinNumToSpawn", m_uiMinNumToSpawn));
    pkStrings->Add(NiGetViewerString("MaxNumToSpawn", m_uiMaxNumToSpawn));

    pkStrings->Add(NiGetViewerString("Master Particle System",
        m_pkMasterPSystem));
}

//--------------------------------------------------------------------------------------------------
