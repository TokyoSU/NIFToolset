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

#include "NiPSCurveEmitter.h"
#include <NiCurve3.h>
#include <NiToolDataStream.h>
#include "NiPSParticleSystem.h"
#include <NiDataStreamElementLock.h>

NiImplementRTTI(NiPSCurveEmitter, NiPSEmitter);

//--------------------------------------------------------------------------------------------------
NiPSCurveEmitter::NiPSCurveEmitter(
    const NiFixedString& kName,
    NiNode* pkCurveParent,
    NiAVObject* pkEmitterObj,
    NiCurve3* pkCurve,
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
    NiPSEmitter(
        kName,
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
        bRandomRotAxis)
{
    SetCurveParent(pkCurveParent);
    SetEmitterObj(pkEmitterObj);
    SetCurve(pkCurve);
}

//--------------------------------------------------------------------------------------------------
NiPSCurveEmitter::NiPSCurveEmitter()
{
}

//--------------------------------------------------------------------------------------------------
bool NiPSCurveEmitter::ComputeInitialPositionAndVelocity(
    NiPSParticleSystem* pkParticleSystem,
    NiQuatTransform& kEmitterToPSys,
    NiPoint3& kPosition,
    NiPoint3& kVelocity)
{
    // Calculate the velocity
    float fSpeed = ComputeSpeed();
    NiPoint3 kDir = ComputeDirection(!pkParticleSystem->GetPreRPIParticleSystem());
    // Update velocity.
    kVelocity = kEmitterToPSys.GetRotate() * kDir * fSpeed;

    // Use the Curve API to pick a random point on the curve to emit from.
    NiPoint3 kLocalPosition = m_spCurve->GetPointAlongCurve(NiUnitRandom());
    if (m_spCurveParent)
    {
        NiTransform kEToP;
        kEToP.m_Translate = kEmitterToPSys.GetTranslate();
        kEmitterToPSys.GetRotate().ToRotation(kEToP.m_Rotate);
        kEToP.m_fScale = kEmitterToPSys.GetScale();
        
        kPosition = kEToP * m_spCurveParent->GetLocalTransform() * kLocalPosition;
    }
    else
    {
        kPosition = kEmitterToPSys.GetRotate() * kLocalPosition;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSCurveEmitter);

//--------------------------------------------------------------------------------------------------
void NiPSCurveEmitter::CopyMembers(
    NiPSCurveEmitter* pkDest,
    NiCloningProcess& kCloning)
{
    NiPSEmitter::CopyMembers(pkDest, kCloning);
}

//--------------------------------------------------------------------------------------------------
void NiPSCurveEmitter::ProcessClone(NiCloningProcess& kCloning)
{
    NiPSEmitter::ProcessClone(kCloning);

    NiObject* pkObject = NULL;
    EE_VERIFY(kCloning.m_pkCloneMap->GetAt(this, pkObject));
    NiPSCurveEmitter* pkDest = (NiPSCurveEmitter*) pkObject;

    if (kCloning.m_pkCloneMap->GetAt(m_pkEmitterObj, pkObject))
    {
        pkDest->m_pkEmitterObj = (NiAVObject*) pkObject;
    }
    else
    {
        pkDest->m_pkEmitterObj = m_pkEmitterObj;
    }

    if (kCloning.m_pkCloneMap->GetAt(m_spCurveParent, pkObject))
    {
        pkDest->m_spCurveParent = (NiNode*)pkObject;
    }
    else
    {
        pkDest->m_spCurveParent = m_spCurveParent;
    }

    if (kCloning.m_pkCloneMap->GetAt(m_spCurve, pkObject))
    {
        pkDest->m_spCurve = (NiCurve3*)pkObject;
    }
    else
    {
        pkDest->m_spCurve = m_spCurve;
    }
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSCurveEmitter);

//--------------------------------------------------------------------------------------------------
void NiPSCurveEmitter::LoadBinary(NiStream& kStream)
{
    NiPSEmitter::LoadBinary(kStream);

    bool bHasCurve = false;
    NiStreamLoadBinary(kStream, bHasCurve);

    if (bHasCurve)
    {
        m_spCurve = NiNew NiCurve3();
        m_spCurve->LoadBinary(kStream);
    }

    m_spCurveParent = (NiNode*)kStream.ResolveLinkID();
    m_pkEmitterObj = (NiNode*)kStream.ResolveLinkID();
}

//--------------------------------------------------------------------------------------------------
void NiPSCurveEmitter::LinkObject(NiStream& kStream)
{
    NiPSEmitter::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSCurveEmitter::RegisterStreamables(NiStream& kStream)
{
    return NiPSEmitter::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSCurveEmitter::SaveBinary(NiStream& kStream)
{
    NiPSEmitter::SaveBinary(kStream);

    bool bHasCurve = (m_spCurve != NULL);
    NiStreamSaveBinary(kStream, bHasCurve);
    if (bHasCurve)
    {
        m_spCurve->SaveBinary(kStream);
    }

    kStream.SaveLinkID(m_spCurveParent);
    kStream.SaveLinkID(m_pkEmitterObj);
}

//--------------------------------------------------------------------------------------------------
bool NiPSCurveEmitter::IsEqual(NiObject* pkObject)
{
    if (!NiPSEmitter::IsEqual(pkObject))
    {
        return false;
    }

    NiPSCurveEmitter* pkDest = (NiPSCurveEmitter*) pkObject;

    if ((pkDest->m_spCurveParent && !m_spCurveParent) ||
        (!pkDest->m_spCurveParent && m_spCurveParent))
    {
        return false;
    }

    if ((pkDest->m_pkEmitterObj && !m_pkEmitterObj) ||
        (!pkDest->m_pkEmitterObj && m_pkEmitterObj))
    {
        return false;
    }

    if ((pkDest->m_spCurve && !m_spCurve) ||
        (!pkDest->m_spCurve && m_spCurve))
    {
        return false;
    }

    if (m_spCurve && !m_spCurve->IsEqual(pkDest->m_spCurve))
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiPSCurveEmitter::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiPSEmitter::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSCurveEmitter::ms_RTTI.GetName()));
    if (m_spCurveParent)
    {
        m_spCurveParent->GetViewerStrings(pkStrings);
    }
    if (m_pkEmitterObj)
    {
        m_pkEmitterObj->GetViewerStrings(pkStrings);
    }
    if (m_spCurve)
    {
        m_spCurve->GetViewerStrings(pkStrings);
    }
}

//--------------------------------------------------------------------------------------------------
