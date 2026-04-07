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

#include "NiPSEmitterSpeedFlipRatioCtlr.h"

NiImplementRTTI(NiPSEmitterSpeedFlipRatioCtlr, NiPSEmitterFloatCtlr);

//--------------------------------------------------------------------------------------------------
NiPSEmitterSpeedFlipRatioCtlr::NiPSEmitterSpeedFlipRatioCtlr(
    const NiFixedString& kEmitterName) :
    NiPSEmitterFloatCtlr(kEmitterName)
{
}

//--------------------------------------------------------------------------------------------------
NiPSEmitterSpeedFlipRatioCtlr::NiPSEmitterSpeedFlipRatioCtlr()
{
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedFlipRatioCtlr::GetTargetFloatValue(float& fValue)
{
    fValue = m_pkEmitter->GetSpeedFlipRatio();
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedFlipRatioCtlr::SetTargetFloatValue(float fValue)
{
    m_pkEmitter->SetSpeedFlipRatio(fValue);
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiPSEmitterSpeedFlipRatioCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedFlipRatioCtlr::CopyMembers(
    NiPSEmitterSpeedFlipRatioCtlr* pkDest,
    NiCloningProcess& kCloning)
{
    NiPSEmitterFloatCtlr::CopyMembers(pkDest, kCloning);
}

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiPSEmitterSpeedFlipRatioCtlr);

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedFlipRatioCtlr::LoadBinary(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedFlipRatioCtlr::LinkObject(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSEmitterSpeedFlipRatioCtlr::RegisterStreamables(NiStream& kStream)
{
    return NiPSEmitterFloatCtlr::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedFlipRatioCtlr::SaveBinary(NiStream& kStream)
{
    NiPSEmitterFloatCtlr::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiPSEmitterSpeedFlipRatioCtlr::IsEqual(NiObject* pkObject)
{
    return NiPSEmitterFloatCtlr::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------
// Viewer Strings
//--------------------------------------------------------------------------------------------------
void NiPSEmitterSpeedFlipRatioCtlr::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiPSEmitterFloatCtlr::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiPSEmitterSpeedFlipRatioCtlr::ms_RTTI.GetName()));
}

//--------------------------------------------------------------------------------------------------
