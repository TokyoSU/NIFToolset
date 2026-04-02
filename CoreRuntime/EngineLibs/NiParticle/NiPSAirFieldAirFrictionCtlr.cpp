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

#include "NiPSAirFieldAirFrictionCtlr.h"
#include "NiPSParticleSystem.h"
#include "NiPSSimulatorForcesStep.h"
#include "NiPSAirFieldForce.h"

NiImplementRTTI(NiPSAirFieldAirFrictionCtlr, NiPSForceFloatCtlr);

//--------------------------------------------------------------------------------------------------
NiPSAirFieldAirFrictionCtlr::NiPSAirFieldAirFrictionCtlr(
    const NiFixedString& kForceName) :
    NiPSForceFloatCtlr(kForceName)
{
}

//--------------------------------------------------------------------------------------------------
NiPSAirFieldAirFrictionCtlr::NiPSAirFieldAirFrictionCtlr()
{
}

//--------------------------------------------------------------------------------------------------
bool NiPSAirFieldAirFrictionCtlr::InterpTargetIsCorrectType(
    NiObjectNET* pkTarget) const
{
    if (!NiPSForceFloatCtlr::InterpTargetIsCorrectType(pkTarget))
    {
        return false;
    }

    NiPSSimulator* pkSimulator = ((NiPSParticleSystem*) pkTarget)
        ->GetSimulator();
    EE_ASSERT(pkSimulator);
    NiPSSimulatorForcesStep* pkForcesStep = NULL;
    for (NiUInt32 ui = 0; ui < pkSimulator->GetStepCount(); ++ui)
    {
        pkForcesStep = NiDynamicCast(NiPSSimulatorForcesStep,
            pkSimulator->GetStepAt(ui));
        if (pkForcesStep)
        {
            break;
        }
    }
    EE_ASSERT(pkForcesStep);
    NiPSForce* pkForce = pkForcesStep->GetForceByName(m_kForceName);
    EE_ASSERT(pkForce);

    return NiIsKindOf(NiPSAirFieldForce, pkForce);
}

//--------------------------------------------------------------------------------------------------
void NiPSAirFieldAirFrictionCtlr::GetTargetFloatValue(float& fValue)
{
    fValue = ((NiPSAirFieldForce*) m_pkForce)->GetAirFriction();
}

//--------------------------------------------------------------------------------------------------
void NiPSAirFieldAirFrictionCtlr::SetTargetFloatValue(float fValue)
{
    ((NiPSAirFieldForce*) m_pkForce)->SetAirFriction(fValue);
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSAirFieldAirFrictionCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSAirFieldAirFrictionCtlr::CopyMembers(
    NiPSAirFieldAirFrictionCtlr* pkDest,
    NiCloningProcess& kCloning)
{
    NiPSForceFloatCtlr::CopyMembers(pkDest, kCloning);
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSAirFieldAirFrictionCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSAirFieldAirFrictionCtlr::LoadBinary(NiStream& kStream)
{
    NiPSForceFloatCtlr::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSAirFieldAirFrictionCtlr::LinkObject(NiStream& kStream)
{
    NiPSForceFloatCtlr::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSAirFieldAirFrictionCtlr::RegisterStreamables(NiStream& kStream)
{
    return NiPSForceFloatCtlr::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSAirFieldAirFrictionCtlr::SaveBinary(NiStream& kStream)
{
    NiPSForceFloatCtlr::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSAirFieldAirFrictionCtlr::IsEqual(NiObject* pkObject)
{
    return NiPSForceFloatCtlr::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------
// Viewer Strings
//--------------------------------------------------------------------------------------------------
void NiPSAirFieldAirFrictionCtlr::GetViewerStrings(NiViewerStringsArray*
    pkStrings)
{
    NiPSForceFloatCtlr::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSAirFieldAirFrictionCtlr::ms_RTTI
        .GetName()));
}

//--------------------------------------------------------------------------------------------------
