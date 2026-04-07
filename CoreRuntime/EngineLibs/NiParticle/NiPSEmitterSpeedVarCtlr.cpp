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

#include "NiPSEmitterSpeedVarCtlr.h"

NiImplementRTTI(NiPSEmitterSpeedVarCtlr, NiPSEmitterFloatCtlr);

//--------------------------------------------------------------------------------------------------
NiPSEmitterSpeedVarCtlr::NiPSEmitterSpeedVarCtlr(
    const NiFixedString& kEmitterName) :
    NiPSEmitterFloatCtlr(kEmitterName)
{
}

//--------------------------------------------------------------------------------------------------
NiPSEmitterSpeedVarCtlr::NiPSEmitterSpeedVarCtlr()
{
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedVarCtlr::GetTargetFloatValue(float& fValue)
{
    fValue = m_pkEmitter->GetSpeedVar();
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedVarCtlr::SetTargetFloatValue(float fValue)
{
    m_pkEmitter->SetSpeedVar(fValue);
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSEmitterSpeedVarCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedVarCtlr::CopyMembers(
    NiPSEmitterSpeedVarCtlr* pkDest,
    NiCloningProcess& kCloning)
{
    NiPSEmitterFloatCtlr::CopyMembers(pkDest, kCloning);
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSEmitterSpeedVarCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedVarCtlr::LoadBinary(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedVarCtlr::LinkObject(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSEmitterSpeedVarCtlr::RegisterStreamables(NiStream& kStream)
{
    return NiPSEmitterFloatCtlr::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedVarCtlr::SaveBinary(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSEmitterSpeedVarCtlr::IsEqual(NiObject* pkObject)
{
    return NiPSEmitterFloatCtlr::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------
// Viewer Strings
//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedVarCtlr::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiPSEmitterFloatCtlr::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSEmitterSpeedVarCtlr::ms_RTTI
        .GetName()));
}

//--------------------------------------------------------------------------------------------------
