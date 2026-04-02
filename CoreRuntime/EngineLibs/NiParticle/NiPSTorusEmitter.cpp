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

#include "NiPSTorusEmitter.h"
#include <NiTransform.h>
#include <NiStream.h>

NiImplementRTTI(NiPSTorusEmitter, NiPSVolumeEmitter);

//--------------------------------------------------------------------------------------------------
NiPSTorusEmitter::NiPSTorusEmitter(
    const NiFixedString& kName,
    float fEmitterRadius,
    float fEmitterSectionRadius,
    NiAVObject* pkEmitterObj,
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
    NiPSVolumeEmitter(
        kName,
        pkEmitterObj,
        fSpeed,
        fSpeedVar,
        fSpeedFlipRatio,
        fDeclination,
        fDeclinationVar,
        fPlanarAngle,
        fPlanarAngleVar,
        fSize,
        fSizeVar,
        fLifeSpan,
        fLifeSpanVar,
        fRotAngle,
        fRotAngleVar,
        fRotSpeed,
        fRotSpeedVar,
        bRandomRotSpeedSign,
        kRotAxis,
        bRandomRotAxis),
    m_fEmitterRadius(fEmitterRadius),
    m_fEmitterSectionRadius(fEmitterSectionRadius)
{
}

//--------------------------------------------------------------------------------------------------
NiPSTorusEmitter::NiPSTorusEmitter() : m_fEmitterRadius(0.0f), m_fEmitterSectionRadius(0.0f)
{
}

//--------------------------------------------------------------------------------------------------
bool NiPSTorusEmitter::ComputeVolumeInitialPositionAndVelocity(
    NiQuatTransform& kEmitterToPSys,
    NiPoint3& kPosition,
    NiPoint3& kVelocity)
{
    // Compute random initial position in torus.
    if (m_fEmitterRadius == 0.0f)
    {
        kPosition = NiPoint3::ZERO;
    }
    else
    {
        float fRadius = m_fEmitterRadius;
        float fSectionRadius = m_fEmitterSectionRadius * NiUnitRandom();
        float fPhi = NiUnitRandom() * NI_TWO_PI;
        float fTheta = NiUnitRandom() * NI_TWO_PI;

        float fSinTheta, fCosTheta, fSinPhi, fCosPhi;
        NiSinCos(fPhi, fSinPhi, fCosPhi);
        NiSinCos(fTheta, fSinTheta, fCosTheta);

        fRadius = m_fEmitterRadius + fSectionRadius * fCosPhi;
        kPosition.x = fRadius * fCosTheta;
        kPosition.y = fRadius * fSinTheta;
        kPosition.z = fSectionRadius * fSinPhi;
    }

    // Update position.
    kPosition = kEmitterToPSys * kPosition;

    // Update velocity.
    kVelocity = kEmitterToPSys.GetRotate() * kVelocity;

    return true;
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSTorusEmitter);

//--------------------------------------------------------------------------------------------------
void NiPSTorusEmitter::CopyMembers(NiPSTorusEmitter* pkDest,
    NiCloningProcess& kCloning)
{
    NiPSVolumeEmitter::CopyMembers(pkDest, kCloning);

    pkDest->m_fEmitterRadius = m_fEmitterRadius;
    pkDest->m_fEmitterSectionRadius = m_fEmitterSectionRadius;
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSTorusEmitter);

//--------------------------------------------------------------------------------------------------
void NiPSTorusEmitter::LoadBinary(NiStream& kStream)
{
    NiPSVolumeEmitter::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_fEmitterRadius);
    NiStreamLoadBinary(kStream, m_fEmitterSectionRadius);
}

//--------------------------------------------------------------------------------------------------
void NiPSTorusEmitter::LinkObject(NiStream& kStream)
{
    NiPSVolumeEmitter::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSTorusEmitter::RegisterStreamables(NiStream& kStream)
{
    return NiPSVolumeEmitter::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSTorusEmitter::SaveBinary(NiStream& kStream)
{
    NiPSVolumeEmitter::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_fEmitterRadius);
    NiStreamSaveBinary(kStream, m_fEmitterSectionRadius);
}

//--------------------------------------------------------------------------------------------------
bool NiPSTorusEmitter::IsEqual(NiObject* pkObject)
{
    if (!NiPSVolumeEmitter::IsEqual(pkObject))
    {
        return false;
    }

    NiPSTorusEmitter* pkDest = (NiPSTorusEmitter*) pkObject;

    if (pkDest->m_fEmitterRadius != m_fEmitterRadius)
    {
        return false;
    }

    if (pkDest->m_fEmitterSectionRadius != m_fEmitterSectionRadius)
    {
        return false;
    }


    return true;
}

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiPSTorusEmitter::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiPSVolumeEmitter::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSTorusEmitter::ms_RTTI.GetName()));

    pkStrings->Add(NiGetViewerString("EmitterRadius", m_fEmitterRadius));
    pkStrings->Add(NiGetViewerString("EmitterSectionRadius", m_fEmitterSectionRadius));
}

//--------------------------------------------------------------------------------------------------
