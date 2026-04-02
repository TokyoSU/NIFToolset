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

#include "NiColorExtraDataController.h"
#include <NiColorExtraData.h>
#include "NiBlendColorInterpolator.h"
#include "NiColorData.h"
#include "NiColorInterpolator.h"
#include "NiConstColorEvaluator.h"

NiImplementRTTI(NiColorExtraDataController, NiExtraDataController);

//------------------------------------------------------------------------------------------------
NiColorExtraDataController::NiColorExtraDataController(
    const NiFixedString& kColorExtraDataName) : NiExtraDataController(
    kColorExtraDataName)
{
}

//------------------------------------------------------------------------------------------------
NiColorExtraDataController::NiColorExtraDataController()
{
}

//------------------------------------------------------------------------------------------------
NiEvaluator* NiColorExtraDataController::CreatePoseEvaluator(
    unsigned short usIndex)
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    return NiNew NiConstColorEvaluator(((NiColorExtraData*)(m_spExtraData.data()))->GetValue());
}

//------------------------------------------------------------------------------------------------
NiInterpolator* NiColorExtraDataController::CreatePoseInterpolator(
    unsigned short usIndex)
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    return NiNew NiColorInterpolator(((NiColorExtraData*)(m_spExtraData.data()))->GetValue());
}

//------------------------------------------------------------------------------------------------
void NiColorExtraDataController::SynchronizePoseInterpolator(NiInterpolator*
    pkInterp,
    unsigned short usIndex)
{
    EE_UNUSED_ARG(usIndex);
    NiColorInterpolator* pkColorInterp =
        NiDynamicCast(NiColorInterpolator, pkInterp);

    EE_ASSERT(usIndex == 0);
    EE_ASSERT(pkColorInterp);
    pkColorInterp->SetPoseValue(((NiColorExtraData*)(m_spExtraData.data()))->GetValue());
}

//------------------------------------------------------------------------------------------------
NiBlendInterpolator* NiColorExtraDataController::CreateBlendInterpolator(
    unsigned short usIndex,
    bool bManagerControlled,
    float fWeightThreshold,
    unsigned char ucArraySize) const
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    return NiNew NiBlendColorInterpolator(bManagerControlled, fWeightThreshold,
        ucArraySize);
}

//------------------------------------------------------------------------------------------------
void NiColorExtraDataController::Update(float fTime)
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
        NiColorA kValue;
        if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, kValue))
        {
            if (m_spExtraData)
            {
                EE_ASSERT(NiIsKindOf(NiColorExtraData, m_spExtraData));
                ((NiColorExtraData*)(m_spExtraData.data()))->SetValue(kValue);
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
bool NiColorExtraDataController::UpdateValue(float,
    const NiColorA& kColor, unsigned short)
{
    EE_ASSERT(GetManagerControlled());
    if (m_spExtraData)
    {
        EE_ASSERT(NiIsKindOf(NiColorExtraData, m_spExtraData));
        ((NiColorExtraData*)(m_spExtraData.data()))->SetValue(kColor);
    }
    return true;
}

//------------------------------------------------------------------------------------------------
bool NiColorExtraDataController::InterpolatorIsCorrectType(
    NiInterpolator* pkInterpolator,
    unsigned short usIndex) const
{
    EE_UNUSED_ARG(usIndex);
    EE_ASSERT(usIndex == 0);
    return pkInterpolator->IsColorAValueSupported();
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Cloning
//------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiColorExtraDataController);

//------------------------------------------------------------------------------------------------
void NiColorExtraDataController::CopyMembers(
    NiColorExtraDataController* pkDest, NiCloningProcess& kCloning)
{
    NiExtraDataController::CopyMembers(pkDest, kCloning);
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Streaming
//------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiColorExtraDataController);

//------------------------------------------------------------------------------------------------
void NiColorExtraDataController::LoadBinary(NiStream& kStream)
{
    NiExtraDataController::LoadBinary(kStream);
}

//------------------------------------------------------------------------------------------------
void NiColorExtraDataController::LinkObject(NiStream& kStream)
{
    NiExtraDataController::LinkObject(kStream);
}

//------------------------------------------------------------------------------------------------
bool NiColorExtraDataController::RegisterStreamables(NiStream& kStream)
{
    return NiExtraDataController::RegisterStreamables(kStream);
}

//------------------------------------------------------------------------------------------------
void NiColorExtraDataController::SaveBinary(NiStream& kStream)
{
    NiExtraDataController::SaveBinary(kStream);
}

//------------------------------------------------------------------------------------------------
bool NiColorExtraDataController::IsEqual(NiObject* pkObject)
{
    return NiExtraDataController::IsEqual(pkObject);
}

//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
// Viewer Strings
//------------------------------------------------------------------------------------------------
void NiColorExtraDataController::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiExtraDataController::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiColorExtraDataController::ms_RTTI
        .GetName()));
}

//------------------------------------------------------------------------------------------------
