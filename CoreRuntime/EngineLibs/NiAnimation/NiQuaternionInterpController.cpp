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
#include "NiAnimationPCH.h"

#include "NiQuaternionInterpController.h"
#include "NiBlendQuaternionInterpolator.h"
#include "NiQuaternionInterpolator.h"
#include "NiConstQuaternionEvaluator.h"
#include <NiQuaternion.h>

NiImplementRTTI(NiQuaternionInterpController, NiSingleInterpController);

//--------------------------------------------------------------------------------------------------
NiQuaternionInterpController::NiQuaternionInterpController()
{
}

//--------------------------------------------------------------------------------------------------
NiEvaluator* NiQuaternionInterpController::CreatePoseEvaluator(
    unsigned short usIndex)
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    NiQuaternion kValue;
    GetTargetQuaternionValue(kValue);
    return NiNew NiConstQuaternionEvaluator(kValue);
}

//--------------------------------------------------------------------------------------------------
NiInterpolator* NiQuaternionInterpController::CreatePoseInterpolator(
    unsigned short usIndex)
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    NiQuaternion kValue;
    GetTargetQuaternionValue(kValue);
    return NiNew NiQuaternionInterpolator(kValue);
}

//--------------------------------------------------------------------------------------------------
void NiQuaternionInterpController::SynchronizePoseInterpolator(
    NiInterpolator* pkInterp,
    unsigned short usIndex)
{
    EE_UNUSED_ARG(usIndex);
    NiQuaternionInterpolator* pkQuaternionInterp =
        NiDynamicCast(NiQuaternionInterpolator, pkInterp);

    EE_ASSERT(usIndex == 0);
    EE_ASSERT(pkQuaternionInterp);
    NiQuaternion kValue;
    GetTargetQuaternionValue(kValue);
    pkQuaternionInterp->SetPoseValue(kValue);
}

//--------------------------------------------------------------------------------------------------
NiBlendInterpolator* NiQuaternionInterpController::CreateBlendInterpolator(
    unsigned short usIndex,
    bool bManagerControlled,
    float fWeightThreshold,
    unsigned char ucArraySize) const
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    return NiNew NiBlendQuaternionInterpolator(bManagerControlled,
        fWeightThreshold, ucArraySize);
}

//--------------------------------------------------------------------------------------------------
bool NiQuaternionInterpController::InterpolatorIsCorrectType(
    NiInterpolator* pkInterpolator,
    unsigned short usIndex) const
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    return pkInterpolator->IsQuaternionValueSupported();
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
void NiQuaternionInterpController::CopyMembers(
    NiQuaternionInterpController* pkDest, NiCloningProcess& kCloning)
{
    NiSingleInterpController::CopyMembers(pkDest, kCloning);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
void NiQuaternionInterpController::LoadBinary(NiStream& kStream)
{
    NiSingleInterpController::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiQuaternionInterpController::LinkObject(NiStream& kStream)
{
    NiSingleInterpController::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiQuaternionInterpController::RegisterStreamables(NiStream& kStream)
{
    return NiSingleInterpController::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiQuaternionInterpController::SaveBinary(NiStream& kStream)
{
    NiSingleInterpController::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiQuaternionInterpController::IsEqual(NiObject* pkObject)
{
    return NiSingleInterpController::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiQuaternionInterpController::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiSingleInterpController::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiQuaternionInterpController::ms_RTTI
        .GetName()));
}

//--------------------------------------------------------------------------------------------------
