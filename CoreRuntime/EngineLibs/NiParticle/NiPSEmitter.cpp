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

#include "NiPSEmitter.h"
#include "NiPSParticleSystem.h"
#include "NiPSSimulator.h"

NiImplementRTTI(NiPSEmitter, NiObject);

//--------------------------------------------------------------------------------------------------
NiPSEmitter::NiPSEmitter(
    const NiFixedString& kName,
    float fSpeed,
    float fSpeedVar,
    float fSpeedFlipRatio,
    float fDeclination,
    float fDeclinationVar,
    float fPlanarAngle,
    float fPlanarAngleVar,
    float fSize,
    float fSizeVar,
    float fLifeSpan,
    float fLifeSpanVar,
    float fRotAngle,
    float fRotAngleVar,
    float fRotSpeed,
    float fRotSpeedVar,
    bool bRandomRotSpeedSign,
    const NiPoint3& kRotAxis,
    bool bRandomRotAxis) :
    m_kName(kName),
    m_fSpeed(fSpeed),
    m_fSpeedVar(fSpeedVar),
    m_fSpeedFlipRatio(fSpeedFlipRatio),
    m_fDeclination(fDeclination),
    m_fDeclinationVar(fDeclinationVar),
    m_fPlanarAngle(fPlanarAngle),
    m_fPlanarAngleVar(fPlanarAngleVar),
    m_fSize(fSize),
    m_fSizeVar(fSizeVar),
    m_fLifeSpan(fLifeSpan),
    m_fLifeSpanVar(fLifeSpanVar),
    m_fRotAngle(fRotAngle),
    m_fRotAngleVar(fRotAngleVar),
    m_fRotSpeed(fRotSpeed),
    m_fRotSpeedVar(fRotSpeedVar),
    m_kRotAxis(kRotAxis),
    m_bRandomRotSpeedSign(bRandomRotSpeedSign),
    m_bRandomRotAxis(bRandomRotAxis),
    m_bResetRequired(true),
    m_kLastEmitterToPSys(),
    m_fLastEmissionTime(0.0f),
    m_kColorBackCompat(NiRGBA::WHITE)
{
}

//--------------------------------------------------------------------------------------------------
NiPSEmitter::NiPSEmitter() :
    m_fSpeed(1.0f),
    m_fSpeedVar(0.0f),
    m_fSpeedFlipRatio(0.0f),
    m_fDeclination(0.0f),
    m_fDeclinationVar(0.0f),
    m_fPlanarAngle(0.0f),
    m_fPlanarAngleVar(0.0f),
    m_fSize(1.0f),
    m_fSizeVar(0.0f),
    m_fLifeSpan(1.0f),
    m_fLifeSpanVar(0.0f),
    m_fRotAngle(0.0f),
    m_fRotAngleVar(0.0f),
    m_fRotSpeed(0.0f),
    m_fRotSpeedVar(0.0f),
    m_kRotAxis(NiPoint3::UNIT_X),
    m_bRandomRotSpeedSign(false),
    m_bRandomRotAxis(true),
    m_bResetRequired(true),
    m_kLastEmitterToPSys(),
    m_fLastEmissionTime(0.0f),
    m_kColorBackCompat(NiRGBA::WHITE)
{
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitter::EmitParticles(
    NiPSParticleSystem* pkParticleSystem,
    float fTime,
    NiUInt32 uiNumParticles,
    const float* pfAges)
{
    NiAVObject* pkEmitterObj = GetEmitterObj();
    NiQuatTransform kCurrentEmitterToPSys;

    if (pkEmitterObj)
    {
        NiQuaternion kQuat;
        kQuat.FromRotation(pkEmitterObj->GetWorldRotate());
        NiQuatTransform kEmitterTransform(
            pkEmitterObj->GetWorldTranslate(),
            kQuat,
            pkEmitterObj->GetWorldScale());
        kQuat.FromRotation(pkParticleSystem->GetWorldRotate());
        NiQuatTransform kPSys(
            pkParticleSystem->GetWorldTranslate(),
            kQuat,
            pkParticleSystem->GetWorldScale());
        NiQuatTransform kInvPSysTransform;
        kPSys.HierInvert(kInvPSysTransform);
        kCurrentEmitterToPSys = kInvPSysTransform.HierApply(kEmitterTransform);
    }
    else
    {
        kCurrentEmitterToPSys.SetTranslate(NiPoint3::ZERO);
        kCurrentEmitterToPSys.SetRotate(NiQuaternion::IDENTITY);
        kCurrentEmitterToPSys.SetScale(1.0f);
    }

    bool bInterpolate = !m_bResetRequired && pfAges && m_fLastEmissionTime < fTime;
    float fInvDt = 1.0f / (fTime - m_fLastEmissionTime);

    for (NiUInt32 ui = 0; ui < uiNumParticles; ++ui)
    {
        // Check to make sure the particle is not already dead. If so,
        // don't create the particle in the first place.
        float fAge = pfAges ? pfAges[ui] : 0.0f;
        float fLifeSpan = m_fLifeSpan + (m_fLifeSpanVar * (NiUnitRandom() - 0.5f));
        if (fAge > fLifeSpan)
        {
            continue;
        }

        // Compute initial position and velocity.
        NiPoint3 kPosition;
        NiPoint3 kVelocity;
        if (bInterpolate)
        {
            // Interpolate the emitter position between last position and current position
            float fU = fAge * fInvDt;
            
            NiQuatTransform kEmitterToPSys;
            kEmitterToPSys.SetScale(
                fU * m_kLastEmitterToPSys.GetScale() +
                (1.0f - fU) * kCurrentEmitterToPSys.GetScale());
            kEmitterToPSys.SetTranslate(
                fU * m_kLastEmitterToPSys.GetTranslate() +
                (1.0f - fU) * kCurrentEmitterToPSys.GetTranslate());
            kEmitterToPSys.SetRotate(NiQuaternion::Slerp(
                1.0f - fU,
                m_kLastEmitterToPSys.GetRotate(),
                kCurrentEmitterToPSys.GetRotate()));

            if (!ComputeInitialPositionAndVelocity(
                pkParticleSystem,
                kEmitterToPSys,
                kPosition,
                kVelocity))
            {
                continue;
            }
        }
        else
        {
            if (!ComputeInitialPositionAndVelocity(
                pkParticleSystem,
                kCurrentEmitterToPSys,
                kPosition,
                kVelocity))
            {
                continue;
            }
        }

        if (m_fSpeedFlipRatio > 0.0f)
        {
            bool bFlip = NiUnitRandom() < m_fSpeedFlipRatio;
            if (bFlip)
            {
                kVelocity *= -1.0f;
            }
        }

        // Get new particle index.
        NiUInt32 uiNewIndex = pkParticleSystem->AddParticle();
        if (uiNewIndex == NiPSParticleSystem::INVALID_PARTICLE)
        {
            break;
        }

        // Various particle initializations
        pkParticleSystem->GetAges()[uiNewIndex] = fAge;
        pkParticleSystem->GetLifeSpans()[uiNewIndex] = fLifeSpan;
        pkParticleSystem->GetFlags()[uiNewIndex] = 0;
        pkParticleSystem->GetPositions()[uiNewIndex] = kPosition;
        pkParticleSystem->GetVelocities()[uiNewIndex] = kVelocity;

        // This is done for backward compatibility. Once the initial color has been depricated this
        // can be removed.
        if (pkParticleSystem->HasColors() && m_kColorBackCompat.a() > 0)
        {
            pkParticleSystem->GetColors()[uiNewIndex] = m_kColorBackCompat;
        }

        // Initialize other particle data.
        float fSize = m_fSize + m_fSizeVar * NiSymmetricRandom();
        pkParticleSystem->GetInitialSizes()[uiNewIndex] = fSize;
        pkParticleSystem->GetSizes()[uiNewIndex] = 1.0f;
        pkParticleSystem->GetLastUpdateTimes()[uiNewIndex] = fTime - fAge;

        if (pkParticleSystem->HasLivingSpawner())
        {
            pkParticleSystem->GetNextSpawnTime(fTime, fAge, fLifeSpan,
                pkParticleSystem->GetNextSpawnTimes()[uiNewIndex]);
        }

        if (pkParticleSystem->HasRotations())
        {
            float fRotAngle = m_fRotAngle + m_fRotAngleVar * NiSymmetricRandom();
            pkParticleSystem->GetInitialRotationAngles()[uiNewIndex] = fRotAngle;
            pkParticleSystem->GetRotationAngles()[uiNewIndex] = fRotAngle;

            if (pkParticleSystem->HasRotationSpeeds())
            {
                float fRotSpeed = m_fRotSpeed + m_fRotSpeedVar * NiSymmetricRandom();
                if (m_bRandomRotSpeedSign)
                {
                    // Compute random rot speed sign.
                    fRotSpeed = (NiUnitRandom() > 0.5f) ? fRotSpeed : -fRotSpeed;
                }
                pkParticleSystem->GetRotationSpeeds()[uiNewIndex] = fRotSpeed;
            }
        }

        if (pkParticleSystem->HasRotationAxes())
        {
            NiPoint3 kRotAxis = m_kRotAxis;
            if (m_bRandomRotAxis)
            {
                // Compute random rotation axis.
                float fPhi = NiUnitRandom() * NI_PI;
                float fZ = NiCos(fPhi);
                float fHypot = NiSqrt(1.0f - fZ * fZ);
                float fTheta = NiUnitRandom() * NI_TWO_PI;
                kRotAxis.x = fHypot * NiCos(fTheta);
                kRotAxis.y = fHypot * NiSin(fTheta);
                kRotAxis.z = fZ;
            }
            pkParticleSystem->GetRotationAxes()[uiNewIndex] = kRotAxis;
        }

        pkParticleSystem->InitializeParticle(uiNewIndex);
    }

    m_kLastEmitterToPSys = kCurrentEmitterToPSys;
    m_fLastEmissionTime = fTime;
    m_bResetRequired = false;
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
void NiPSEmitter::CopyMembers(
    NiPSEmitter* pkDest,
    NiCloningProcess& kCloning)
{
    NiObject::CopyMembers(pkDest, kCloning);

    pkDest->m_kColorBackCompat = m_kColorBackCompat;

    pkDest->m_kName = m_kName;
    pkDest->m_fSpeed = m_fSpeed;
    pkDest->m_fSpeedVar = m_fSpeedVar;
    pkDest->m_fSpeedFlipRatio = m_fSpeedFlipRatio;
    pkDest->m_fDeclination = m_fDeclination;
    pkDest->m_fDeclinationVar = m_fDeclinationVar;
    pkDest->m_fPlanarAngle = m_fPlanarAngle;
    pkDest->m_fPlanarAngleVar = m_fPlanarAngleVar;
    pkDest->m_fSize = m_fSize;
    pkDest->m_fSizeVar = m_fSizeVar;
    pkDest->m_fLifeSpan = m_fLifeSpan;
    pkDest->m_fLifeSpanVar = m_fLifeSpanVar;
    pkDest->m_fRotAngle = m_fRotAngle;
    pkDest->m_fRotAngleVar = m_fRotAngleVar;
    pkDest->m_fRotSpeed = m_fRotSpeed;
    pkDest->m_fRotSpeedVar = m_fRotSpeedVar;
    pkDest->m_kRotAxis = m_kRotAxis;
    pkDest->m_bRandomRotSpeedSign = m_bRandomRotSpeedSign;
    pkDest->m_bRandomRotAxis = m_bRandomRotAxis;
    pkDest->m_bResetRequired = true;
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
void NiPSEmitter::LoadBinary(NiStream& kStream)
{
    NiObject::LoadBinary(kStream);

    kStream.LoadFixedString(m_kName);
    NiStreamLoadBinary(kStream, m_fSpeed);
    NiStreamLoadBinary(kStream, m_fSpeedVar);

    // If ever integrated into a later Gamebryo branch, this must be modified to correctly
    // handle NIF files that are newer but lack the data.
    if (kStream.GetFileVersion() >= NiStream::GetVersion(20, 6, 1, 0))
    {
        NiStreamLoadBinary(kStream, m_fSpeedFlipRatio);
    }

    NiStreamLoadBinary(kStream, m_fDeclination);
    NiStreamLoadBinary(kStream, m_fDeclinationVar);
    NiStreamLoadBinary(kStream, m_fPlanarAngle);
    NiStreamLoadBinary(kStream, m_fPlanarAngleVar);
    if (kStream.GetFileVersion() < NiStream::GetVersion(20, 6, 1, 0))
    {
        m_kColorBackCompat.LoadBinary(kStream);
    }
    else
    {
        // Make sure it has no alpha so it will not be used.
        m_kColorBackCompat.a() = 0;
    }
    NiStreamLoadBinary(kStream, m_fSize);
    NiStreamLoadBinary(kStream, m_fSizeVar);
    NiStreamLoadBinary(kStream, m_fLifeSpan);
    NiStreamLoadBinary(kStream, m_fLifeSpanVar);
    NiStreamLoadBinary(kStream, m_fRotAngle);
    NiStreamLoadBinary(kStream, m_fRotAngleVar);
    NiStreamLoadBinary(kStream, m_fRotSpeed);
    NiStreamLoadBinary(kStream, m_fRotSpeedVar);
    m_kRotAxis.LoadBinary(kStream);
    NiBool bValue;
    NiStreamLoadBinary(kStream, bValue);
    m_bRandomRotSpeedSign = NIBOOL_IS_TRUE(bValue);
    NiStreamLoadBinary(kStream, bValue);
    m_bRandomRotAxis = NIBOOL_IS_TRUE(bValue);

    if (kStream.GetFileVersion() >= NiStream::GetVersion(30, 0, 0, 0) &&
        kStream.GetFileVersion() < NiStream::GetVersion(30, 0, 0, 2))
    {
        NiStreamLoadBinary(kStream, bValue);
    }

    m_bResetRequired = true;
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitter::LinkObject(NiStream& kStream)
{
    NiObject::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSEmitter::RegisterStreamables(NiStream& kStream)
{
    if (!NiObject::RegisterStreamables(kStream))
    {
        return false;
    }

    kStream.RegisterFixedString(m_kName);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitter::SaveBinary(NiStream& kStream)
{
    NiObject::SaveBinary(kStream);

    kStream.SaveFixedString(m_kName);
    NiStreamSaveBinary(kStream, m_fSpeed);
    NiStreamSaveBinary(kStream, m_fSpeedVar);
    NiStreamSaveBinary(kStream, m_fSpeedFlipRatio);
    NiStreamSaveBinary(kStream, m_fDeclination);
    NiStreamSaveBinary(kStream, m_fDeclinationVar);
    NiStreamSaveBinary(kStream, m_fPlanarAngle);
    NiStreamSaveBinary(kStream, m_fPlanarAngleVar);
    NiStreamSaveBinary(kStream, m_fSize);
    NiStreamSaveBinary(kStream, m_fSizeVar);
    NiStreamSaveBinary(kStream, m_fLifeSpan);
    NiStreamSaveBinary(kStream, m_fLifeSpanVar);
    NiStreamSaveBinary(kStream, m_fRotAngle);
    NiStreamSaveBinary(kStream, m_fRotAngleVar);
    NiStreamSaveBinary(kStream, m_fRotSpeed);
    NiStreamSaveBinary(kStream, m_fRotSpeedVar);
    m_kRotAxis.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, NiBool(m_bRandomRotSpeedSign));
    NiStreamSaveBinary(kStream, NiBool(m_bRandomRotAxis));
}

//--------------------------------------------------------------------------------------------------
bool NiPSEmitter::IsEqual(NiObject* pkObject)
{
    if (!NiObject::IsEqual(pkObject))
    {
        return false;
    }

    NiPSEmitter* pkDest = (NiPSEmitter*) pkObject;

    if (pkDest->m_kName != m_kName ||
        pkDest->m_fSpeed != m_fSpeed ||
        pkDest->m_fSpeedVar != m_fSpeedVar ||
        pkDest->m_fSpeedFlipRatio != m_fSpeedFlipRatio ||
        pkDest->m_fDeclination != m_fDeclination ||
        pkDest->m_fDeclinationVar != m_fDeclinationVar ||
        pkDest->m_fPlanarAngle != m_fPlanarAngle ||
        pkDest->m_fPlanarAngleVar != m_fPlanarAngleVar ||
        pkDest->m_fSize != m_fSize ||
        pkDest->m_fSizeVar != m_fSizeVar ||
        pkDest->m_fLifeSpan != m_fLifeSpan ||
        pkDest->m_fLifeSpanVar != m_fLifeSpanVar ||
        pkDest->m_fRotAngle != m_fRotAngle ||
        pkDest->m_fRotAngleVar != m_fRotAngleVar ||
        pkDest->m_fRotSpeed != m_fRotSpeed ||
        pkDest->m_fRotSpeedVar != m_fRotSpeedVar ||
        pkDest->m_kRotAxis != m_kRotAxis ||
        pkDest->m_bRandomRotSpeedSign != m_bRandomRotSpeedSign ||
        pkDest->m_bRandomRotAxis != m_bRandomRotAxis)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiPSEmitter::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiObject::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSEmitter::ms_RTTI.GetName()));

    pkStrings->Add(NiGetViewerString("Name", m_kName));
    pkStrings->Add(NiGetViewerString("Speed", m_fSpeed));
    pkStrings->Add(NiGetViewerString("SpeedVar", m_fSpeedVar));
    pkStrings->Add(NiGetViewerString("SpeedFlipRatio", m_fSpeedFlipRatio));
    pkStrings->Add(NiGetViewerString("Declination", m_fDeclination));
    pkStrings->Add(NiGetViewerString("DeclinationVar", m_fDeclinationVar));
    pkStrings->Add(NiGetViewerString("PlanarAngle", m_fPlanarAngle));
    pkStrings->Add(NiGetViewerString("PlanarAngleVar", m_fPlanarAngleVar));
    pkStrings->Add(NiGetViewerString("Size", m_fSize));
    pkStrings->Add(NiGetViewerString("SizeVar", m_fSizeVar));
    pkStrings->Add(NiGetViewerString("LifeSpan", m_fLifeSpan));
    pkStrings->Add(NiGetViewerString("LifeSpanVar", m_fLifeSpanVar));
    pkStrings->Add(NiGetViewerString("RotAngle", m_fRotAngle));
    pkStrings->Add(NiGetViewerString("RotAngleVar", m_fRotAngleVar));
    pkStrings->Add(NiGetViewerString("RotSpeed", m_fRotSpeed));
    pkStrings->Add(NiGetViewerString("RotSpeedVar", m_fRotSpeedVar));
    pkStrings->Add(NiGetViewerString("RandomRotSpeedSign",
        m_bRandomRotSpeedSign));
    pkStrings->Add(m_kRotAxis.GetViewerString("RotAxis"));
    pkStrings->Add(NiGetViewerString("RandomRotAxis", m_bRandomRotAxis));
}

//--------------------------------------------------------------------------------------------------
