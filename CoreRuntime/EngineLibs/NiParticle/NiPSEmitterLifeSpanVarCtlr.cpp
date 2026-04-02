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

#include "NiPSEmitterLifeSpanVarCtlr.h"

NiImplementRTTI(NiPSEmitterLifeSpanVarCtlr, NiPSEmitterFloatCtlr);

//--------------------------------------------------------------------------------------------------
NiPSEmitterLifeSpanVarCtlr::NiPSEmitterLifeSpanVarCtlr(
    const NiFixedString& kEmitterName) :
    NiPSEmitterFloatCtlr(kEmitterName)
{
}

//--------------------------------------------------------------------------------------------------
NiPSEmitterLifeSpanVarCtlr::NiPSEmitterLifeSpanVarCtlr()
{
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterLifeSpanVarCtlr::GetTargetFloatValue(float& fValue)
{
    fValue = m_pkEmitter->GetLifeSpanVar();
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterLifeSpanVarCtlr::SetTargetFloatValue(float fValue)
{
    m_pkEmitter->SetLifeSpanVar(fValue);
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSEmitterLifeSpanVarCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSEmitterLifeSpanVarCtlr::CopyMembers(
    NiPSEmitterLifeSpanVarCtlr* pkDest,
    NiCloningProcess& kCloning)
{
    NiPSEmitterFloatCtlr::CopyMembers(pkDest, kCloning);
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSEmitterLifeSpanVarCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSEmitterLifeSpanVarCtlr::LoadBinary(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterLifeSpanVarCtlr::LinkObject(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSEmitterLifeSpanVarCtlr::RegisterStreamables(NiStream& kStream)
{
    return NiPSEmitterFloatCtlr::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterLifeSpanVarCtlr::SaveBinary(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSEmitterLifeSpanVarCtlr::IsEqual(NiObject* pkObject)
{
    return NiPSEmitterFloatCtlr::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------
// Viewer Strings
//--------------------------------------------------------------------------------------------------
void NiPSEmitterLifeSpanVarCtlr::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiPSEmitterFloatCtlr::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSEmitterLifeSpanVarCtlr::ms_RTTI
        .GetName()));
}

//--------------------------------------------------------------------------------------------------
