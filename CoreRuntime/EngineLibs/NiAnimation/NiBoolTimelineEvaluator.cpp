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

#include "NiBoolTimelineEvaluator.h"
#include "NiEvaluatorSPData.h"
#include "NiScratchPad.h"

NiImplementRTTI(NiBoolTimelineEvaluator, NiBoolEvaluator);

//--------------------------------------------------------------------------------------------------
NiBoolTimelineEvaluator::NiBoolTimelineEvaluator()
    : NiBoolEvaluator()
{
    // Base class sets the eval channel types if needed.
}

//--------------------------------------------------------------------------------------------------
NiBoolTimelineEvaluator::NiBoolTimelineEvaluator(NiBoolData* pkBoolData)
    : NiBoolEvaluator(pkBoolData)
{
    // Base class sets the eval channel types if needed.
}

//--------------------------------------------------------------------------------------------------
bool NiBoolTimelineEvaluator::GetChannelScratchPadInfo(unsigned int uiChannel,
    bool bForceAlwaysUpdate, NiAVObjectPalette* pkPalette,
    unsigned int& uiFillSize, bool& bSharedFillData,
    NiScratchPadBlock& eSPBSegmentData,
    NiBSplineBasisData*& pkBasisData) const
{
    EE_ASSERT(uiChannel == BOOL);

    // Initialize using the base class, then fix up a few entries.
    if (!NiBoolEvaluator::GetChannelScratchPadInfo(uiChannel,
        bForceAlwaysUpdate, pkPalette, uiFillSize, bSharedFillData,
        eSPBSegmentData, pkBasisData))
    {
        return false;
    }

    // Fix up the fill size.
    if (uiFillSize > 0)
    {
        uiFillSize = sizeof(NiScratchPad::ConstantBoolTimelineFillData);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiBoolTimelineEvaluator::InitChannelScratchPadData(unsigned int uiChannel,
    NiEvaluatorSPData* pkEvalSPData, NiBSplineBasisData* pkSPBasisData,
    bool bInitSharedData, NiAVObjectPalette* pkPalette,
    NiPoseBufferHandle kPBHandle) const
{
    EE_ASSERT(uiChannel == BOOL);
    EE_ASSERT(pkEvalSPData);
    EE_ASSERT(pkEvalSPData->GetEvaluator() == this);
    EE_ASSERT((unsigned int)pkEvalSPData->GetEvalChannelIndex() == uiChannel);
    EE_ASSERT(!IsEvalChannelInvalid(uiChannel));
    EE_ASSERT(m_spBoolData);

    // Initialize using the base class, then fix up a few entries.
    if (!NiBoolEvaluator::InitChannelScratchPadData(uiChannel,
        pkEvalSPData, pkSPBasisData, bInitSharedData, pkPalette, kPBHandle))
    {
        return false;
    }

    if (IsRawEvalChannelPosed(uiChannel))
    {
        // Nothing to fix up.
        EE_ASSERT(pkEvalSPData->GetSPSegmentMinTime() == 0.0f);
        EE_ASSERT(pkEvalSPData->GetSPSegmentMaxTime() == NI_INFINITY);
        EE_ASSERT(pkEvalSPData->GetSPFillFunc() == NULL);
        return true;
    }

    // Switch to the scratch pad fill function for
    // timeline based bool channels.
    pkEvalSPData->SetSPFillFunc(&ConstantBoolTimelineFillFunction);

    // Initialize the scratch pad fill data members in the derived class.
    NiScratchPad::ConstantBoolTimelineFillData* pkFillData =
        (NiScratchPad::ConstantBoolTimelineFillData*)
        pkEvalSPData->GetSPFillData();
    EE_ASSERT(pkFillData);
    EE_ASSERT((void*)&pkFillData->m_kBaseData == (void*)pkFillData);
    pkFillData->m_uiPreviousIndex = 0;
    pkFillData->m_bPreviousValue = false;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiBoolTimelineEvaluator::ConstantBoolTimelineFillFunction(float fTime,
    NiEvaluatorSPData* pkEvalSPData)
{
    EE_ASSERT(pkEvalSPData);

    // Get the scratch pad fill data.
    NiScratchPad::ConstantBoolTimelineFillData* pkFillData =
        (NiScratchPad::ConstantBoolTimelineFillData*)
        pkEvalSPData->GetSPFillData();
    EE_ASSERT(pkFillData);
    NiScratchPad::ConstantBoolFillData* pkBaseData =
        &pkFillData->m_kBaseData;
    EE_ASSERT(pkBaseData->m_pkKeys);
    EE_ASSERT(pkBaseData->m_uiNumKeys > 0);

    // Determine the current constant value from the timeline.
    // Use GenCubicCoefs instead of GenInterp to obtain a time range.
    pkFillData->m_uiPreviousIndex = pkBaseData->m_uiLastIndex;
    float fMinTime;
    float fMaxTime;
    float fValue0;
    float fOutTangent0;
    float fA0;
    float fB0;
    NiBoolKey::GenCubicCoefs(fTime, pkBaseData->m_pkKeys,
        pkBaseData->m_eType, pkBaseData->m_uiNumKeys,
        pkBaseData->m_uiLastIndex, pkBaseData->m_ucKeySize,
        fMinTime, fMaxTime, fValue0, fOutTangent0, fA0, fB0);
    EE_ASSERT(fOutTangent0 == 0.0f);
    EE_ASSERT(fA0 == 0.0f);
    EE_ASSERT(fB0 == 0.0f);

    unsigned int uiCurrentIndex = pkBaseData->m_uiLastIndex;
    bool bInterpValue = (fValue0 >= 0.5f);
    bool bBoolValue = bInterpValue;
    if (bBoolValue == pkFillData->m_bPreviousValue)
    {
        bool bValueToFind = !bBoolValue;

        // Check to see if there was an opposite key between these two
        // keys. This is to prevent missing events in the timeline.
        if (pkFillData->m_uiPreviousIndex != uiCurrentIndex &&
            pkFillData->m_uiPreviousIndex + 1 > uiCurrentIndex)
        {
            // We have looped. Iterate from the previous index to the end
            // index.
            for (unsigned int ui = pkFillData->m_uiPreviousIndex + 1;
                ui < pkBaseData->m_uiNumKeys; ui++)
            {
                if (pkBaseData->m_pkKeys->GetKeyAt(ui,
                    pkBaseData->m_ucKeySize)->GetBool() == bValueToFind)
                {
                    bBoolValue = bValueToFind;
                    break;
                }
            }

            // If true key not found, iterate from the start index to the
            // current index.
            if (bBoolValue != bValueToFind)
            {
                for (unsigned int ui = 0; ui < uiCurrentIndex; ui++)
                {
                    if (pkBaseData->m_pkKeys->GetKeyAt(ui,
                        pkBaseData->m_ucKeySize)->GetBool() == bValueToFind)
                    {
                        bBoolValue = bValueToFind;
                        break;
                    }
                }
            }
        }
        else
        {
            // We have not looped. Iterate from previous index to current
            // index looking for an opposite key.
            for (unsigned int ui = pkFillData->m_uiPreviousIndex + 1;
                ui < uiCurrentIndex && ui < pkBaseData->m_uiNumKeys; ui++)
            {
                if (pkBaseData->m_pkKeys->GetKeyAt(ui,
                    pkBaseData->m_ucKeySize)->GetBool() == bValueToFind)
                {
                    bBoolValue = bValueToFind;
                    break;
                }
            }
        }
    }

    pkFillData->m_bPreviousValue = bInterpValue;

    // Fill the scratch pad with the timeline based constant segment.
    NiScratchPad::ConstantBoolSegmentData* pkConstantSeg =
        (NiScratchPad::ConstantBoolSegmentData*)
        pkEvalSPData->GetSPSegmentData();
    EE_ASSERT(pkConstantSeg);
    pkConstantSeg->m_fValue0 = bBoolValue ? 1.0f : 0.0f;

    if (bBoolValue == bInterpValue)
    {
        // There was no alternate intermediate value.
        // Update the time range to match the segment.
        pkEvalSPData->SetSPSegmentTimeRange(fMinTime, fMaxTime);
    }
    else
    {
        // Hold the intermediate value until the next update time.
        pkEvalSPData->SetSPSegmentTimeRange(fTime, fTime);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiBoolTimelineEvaluator);

//--------------------------------------------------------------------------------------------------
void NiBoolTimelineEvaluator::CopyMembers(
    NiBoolTimelineEvaluator* pkDest, NiCloningProcess& kCloning)
{
    NiBoolEvaluator::CopyMembers(pkDest, kCloning);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiBoolTimelineEvaluator);

//--------------------------------------------------------------------------------------------------
void NiBoolTimelineEvaluator::LoadBinary(NiStream& kStream)
{
    NiBoolEvaluator::LoadBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiBoolTimelineEvaluator::LinkObject(NiStream& kStream)
{
    NiBoolEvaluator::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiBoolTimelineEvaluator::RegisterStreamables(NiStream& kStream)
{
    return NiBoolEvaluator::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiBoolTimelineEvaluator::SaveBinary(NiStream& kStream)
{
    NiBoolEvaluator::SaveBinary(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiBoolTimelineEvaluator::IsEqual(NiObject* pkObject)
{
    return NiBoolEvaluator::IsEqual(pkObject);
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiBoolTimelineEvaluator::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiBoolEvaluator::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiBoolTimelineEvaluator::ms_RTTI
        .GetName()));
}

//--------------------------------------------------------------------------------------------------
