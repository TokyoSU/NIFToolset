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

#include "NiPSEmitterRadiusVarCtlr.h"

NiImplementRTTI(NiPSEmitterRadiusVarCtlr, NiPSEmitterFloatCtlr);

//--------------------------------------------------------------------------------------------------
NiPSEmitterRadiusVarCtlr::NiPSEmitterRadiusVarCtlr(
    const NiFixedString& kEmitterName) :
    NiPSEmitterFloatCtlr(kEmitterName)
{
}

//--------------------------------------------------------------------------------------------------
NiPSEmitterRadiusVarCtlr::NiPSEmitterRadiusVarCtlr()
{
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterRadiusVarCtlr::GetTargetFloatValue(float& fValue)
{
    fValue = m_pkEmitter->GetSizeVar();
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterRadiusVarCtlr::SetTargetFloatValue(float fValue)
{
    m_pkEmitter->SetSizeVar(fValue);
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSEmitterRadiusVarCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSEmitterRadiusVarCtlr::CopyMembers(
    NiPSEmitterRadiusVarCtlr* pkDest,
    NiCloningProcess& kCloning)
{
    NiPSEmitterFloatCtlr::CopyMembers(pkDest, kCloning);
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSEmitterRadiusVarCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSEmitterRadiusVarCtlr::LoadBinary(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterRadiusVarCtlr::LinkObject(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSEmitterRadiusVarCtlr::RegisterStreamables(NiStream& kStream)
{
    return NiPSEmitterFloatCtlr::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterRadiusVarCtlr::SaveBinary(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSEmitterRadiusVarCtlr::IsEqual(NiObject* pkObject)
{
    return NiPSEmitterFloatCtlr::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------
// Viewer Strings
//--------------------------------------------------------------------------------------------------
void NiPSEmitterRadiusVarCtlr::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiPSEmitterFloatCtlr::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSEmitterRadiusVarCtlr::ms_RTTI
        .GetName()));
}

//--------------------------------------------------------------------------------------------------
