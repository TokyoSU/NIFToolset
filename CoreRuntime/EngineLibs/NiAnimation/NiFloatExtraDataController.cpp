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

#include "NiFloatExtraDataController.h"
#include <NiFloatExtraData.h>
#include "NiBlendFloatInterpolator.h"
#include "NiFloatData.h"
#include "NiFloatInterpolator.h"
#include "NiConstFloatEvaluator.h"

NiImplementRTTI(NiFloatExtraDataController, NiExtraDataController);

//------------------------------------------------------------------------------------------------
NiFloatExtraDataController::NiFloatExtraDataController(
    const NiFixedString& kFloatExtraDataName) : NiExtraDataController(
    kFloatExtraDataName)
{
}

//------------------------------------------------------------------------------------------------
NiFloatExtraDataController::NiFloatExtraDataController()
{
}

//------------------------------------------------------------------------------------------------
NiEvaluator* NiFloatExtraDataController::CreatePoseEvaluator(
    unsigned short usIndex)
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    return NiNew NiConstFloatEvaluator(((NiFloatExtraData*)(m_spExtraData.data()))->GetValue());
}

//------------------------------------------------------------------------------------------------
NiInterpolator* NiFloatExtraDataController::CreatePoseInterpolator(
    unsigned short usIndex)
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    return NiNew NiFloatInterpolator(((NiFloatExtraData*)(m_spExtraData.data()))->GetValue());
}

//------------------------------------------------------------------------------------------------
void NiFloatExtraDataController::SynchronizePoseInterpolator(NiInterpolator*
    pkInterp,
    unsigned short usIndex)
{
    EE_UNUSED_ARG(usIndex);
    NiFloatInterpolator* pkFloatInterp =
        NiDynamicCast(NiFloatInterpolator, pkInterp);

    EE_ASSERT(usIndex == 0);
    EE_ASSERT(pkFloatInterp);
    pkFloatInterp->SetPoseValue(((NiFloatExtraData*)(m_spExtraData.data()))->GetValue());
}

//------------------------------------------------------------------------------------------------
NiBlendInterpolator* NiFloatExtraDataController::CreateBlendInterpolator(
    unsigned short usIndex,
    bool bManagerControlled,
    float fWeightThreshold,
    unsigned char ucArraySize) const
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    return NiNew NiBlendFloatInterpolator(bManagerControlled, fWeightThreshold,
        ucArraySize);
}

//------------------------------------------------------------------------------------------------
void NiFloatExtraDataController::Update(float fTime)
{
    if (GetManagerControlled())
    {
        return;
    }

    if (DontDoUpdate(fTime) &&
        (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate()))
    {
        return;
    }

    if (m_spInterpolator)
    {
        float fValue;
        if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, fValue))
        {
            if (m_spExtraData)
            {
                EE_ASSERT(NiIsKindOf(NiFloatExtraData, m_spExtraData));
                ((NiFloatExtraData*)(m_spExtraData.data()))->SetValue(fValue);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
bool NiFloatExtraDataController::UpdateValue(float, float fFloat,
    unsigned short)
{
    EE_ASSERT(GetManagerControlled());
    if (m_spExtraData)
    {
        EE_ASSERT(NiIsKindOf(NiFloatExtraData, m_spExtraData));
        ((NiFloatExtraData*)(m_spExtraData.data()))->SetValue(fFloat);
    }
    return true;
}

//------------------------------------------------------------------------------------------------
bool NiFloatExtraDataController::InterpolatorIsCorrectType(
    NiInterpolator* pkInterpolator,
    unsigned short usIndex) const
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    return pkInterpolator->IsFloatValueSupported();
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Cloning
//------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiFloatExtraDataController);

//------------------------------------------------------------------------------------------------
void NiFloatExtraDataController::CopyMembers(
    NiFloatExtraDataController* pkDest, NiCloningProcess& kCloning)
{
    NiExtraDataController::CopyMembers(pkDest, kCloning);
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Streaming
//------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiFloatExtraDataController);

//------------------------------------------------------------------------------------------------
void NiFloatExtraDataController::LoadBinary(NiStream& kStream)
{
    NiExtraDataController::LoadBinary(kStream);
}

//------------------------------------------------------------------------------------------------
void NiFloatExtraDataController::LinkObject(NiStream& kStream)
{
    NiExtraDataController::LinkObject(kStream);
}

//------------------------------------------------------------------------------------------------
bool NiFloatExtraDataController::RegisterStreamables(NiStream& kStream)
{
    return NiExtraDataController::RegisterStreamables(kStream);
}

//------------------------------------------------------------------------------------------------
void NiFloatExtraDataController::SaveBinary(NiStream& kStream)
{
    NiExtraDataController::SaveBinary(kStream);
}

//------------------------------------------------------------------------------------------------
bool NiFloatExtraDataController::IsEqual(NiObject* pkObject)
{
    return NiExtraDataController::IsEqual(pkObject);
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Viewer Strings
//------------------------------------------------------------------------------------------------
void NiFloatExtraDataController::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiExtraDataController::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiFloatExtraDataController::ms_RTTI
        .GetName()));
}

//------------------------------------------------------------------------------------------------
