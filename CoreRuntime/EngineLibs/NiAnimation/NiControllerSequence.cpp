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

#include "NiControllerSequence.h"
#include "NiAnimationConstants.h"
#include <NiCloningProcess.h>
#include <NiAVObjectPalette.h>
#include "NiControllerManager.h"

NiImplementRTTI(NiControllerSequence, NiObject);

const float NiControllerSequence::INVALID_TIME = -FLT_MAX;
const float NiControllerSequence::CURRENT_FRAME = -NI_INFINITY;
const float NiControllerSequence::SEQUENCE_DATA_FREQUENCY = -1.0f;

//--------------------------------------------------------------------------------------------------
NiControllerSequence::NiControllerSequence() :
    m_uiActivationID(0), m_pkOwner(NULL),
    m_spPoseBuffer(NULL), m_usNumPoseBufferItems(0),
    m_sLastPoseBufferUpdateLOD(LODSKIPEVALUATOR),
    m_fLastPoseBufferUpdateTime(-NI_INFINITY),
    m_spScratchPad(NULL), m_spSeqData(NULL), m_spTextKeys(NULL),
    m_iPriority(0), m_fWeight(1.0f), m_fTransSpinner(0.0f),
    m_fEaseSpinner(0.0f), m_fDuration(0),
    m_eCycleType(NiTimeController::LOOP), m_fFrequency(1.0f),
    m_fLastTime(-NI_INFINITY), m_fWeightedLastTime(-NI_INFINITY),
    m_fLastScaledTime(-NI_INFINITY), m_eState(INACTIVE),
    m_fOffset(-NI_INFINITY), m_fStartTime(-NI_INFINITY),
    m_fEndTime(-NI_INFINITY), m_fDestFrame(-NI_INFINITY),
    m_bHoldDestFrame(false), m_bNeedFirstTextKey(true),
    m_spPartnerSequence(NULL)
{
}

//--------------------------------------------------------------------------------------------------
NiControllerSequence::~NiControllerSequence()
{
    // Clean up before shutting down.
    PrepareForRecycling();

    // Release all held objects not handled by PrepareForRecycling.
    m_spScratchPad = NULL;
    m_spPoseBuffer = NULL;
    m_spSeqData = NULL;
    m_pkOwner = NULL;

    m_kTextKeyCallbacks.RemoveAll();
}

//--------------------------------------------------------------------------------------------------
float NiControllerSequence::GetTimeAt(NiTextKeyMatch* pkMatchObject,
    float fCurrentTime)
{
    // Need to find first occurence of pcTextKey _after_ fCurrentTime.

    float fMatchFrame = 0.0f;
    float fCurrentFrame = ComputeScaledTime(fCurrentTime + m_fOffset, false);

    // Only LOOP and CLAMP cycle types are supported by sequences.
    EE_ASSERT(m_eCycleType == NiTimeController::CLAMP ||
        m_eCycleType == NiTimeController::LOOP);

    bool bFirstMatch = (m_eCycleType == NiTimeController::LOOP);
    bool bExactMatch = false;
    bool bFoundMatch = false;

    if (m_spTextKeys)
    {
        unsigned int uiNumKeys;
        NiTextKey* pkKeys = m_spTextKeys->GetKeys(uiNumKeys);
        for (unsigned int i = 0; i < uiNumKeys; i++)
        {
            if (pkMatchObject->IsKeyMatch(pkKeys[i].GetText()))
            {
                float fTime = pkKeys[i].GetTime();
                if (fTime >= fCurrentFrame)
                {
                    bFoundMatch = true;
                    fMatchFrame = fTime;
                    bExactMatch = true;
                    break;
                }
                if (bFirstMatch)
                {
                    bFoundMatch = true;
                    fMatchFrame = fTime;
                    bFirstMatch = false;
                }
            }
        }
    }

    if (!bFoundMatch)
    {
        return INVALID_TIME;
    }

    // Check for time sync sequence.
    if (m_spPartnerSequence)
    {
        // Recursively walk the time sync sequences to find
        // the source sequence.
        NiControllerSequence* pkTimeSyncSeq = m_spPartnerSequence;
        while (pkTimeSyncSeq->GetTimeSyncSequence())
        {
            pkTimeSyncSeq = pkTimeSyncSeq->GetTimeSyncSequence();
        }

        // Find corresponding morph frame in time sync sequence.
        float fTimeSyncFrame = pkTimeSyncSeq->FindCorrespondingMorphFrame(
            this, fMatchFrame / m_fFrequency);

        // Compute delta frames of sync sequence.
        float fCurrentSyncFrame = pkTimeSyncSeq->ComputeScaledTime(
            pkTimeSyncSeq->GetOffset() + fCurrentTime, false);
        float fDeltaSyncFrame = fTimeSyncFrame - fCurrentSyncFrame;
        if (fDeltaSyncFrame < 0)
        {
            fDeltaSyncFrame += pkTimeSyncSeq->GetDuration();
            EE_ASSERT(fDeltaSyncFrame >= 0);
        }

        // Convert sync delta frames to global delta time.
        float fDeltaTime = fDeltaSyncFrame / pkTimeSyncSeq->GetFrequency();

        // Return next time of match object.
        return fCurrentTime + fDeltaTime;
    }

    if (!bExactMatch && m_eCycleType == NiTimeController::LOOP)
    {
        fMatchFrame += m_fDuration;
    }

    float fUnscaledTime = (fMatchFrame - fCurrentFrame) /
        m_fFrequency + fCurrentTime;

    return fUnscaledTime;
}

//--------------------------------------------------------------------------------------------------
float NiControllerSequence::GetKeyTimeAt(NiTextKeyMatch* pkMatchObject) const
{
    if (!pkMatchObject || !m_spTextKeys)
    {
        return INVALID_TIME;
    }

    unsigned int uiNumKeys;
    NiTextKey* pkKeys = m_spTextKeys->GetKeys(uiNumKeys);
    for (unsigned int ui = 0; ui < uiNumKeys; ui++)
    {
        if (pkMatchObject->IsKeyMatch(pkKeys[ui].GetText()))
        {
            return pkKeys[ui].GetTime();
        }
    }

    return INVALID_TIME;
}

//--------------------------------------------------------------------------------------------------
void NiControllerSequence::DisableEvaluatorAt(unsigned int uiIndex,
    bool bInvalidatePBData)
{
    EE_ASSERT(uiIndex < GetNumEvaluators());
    EE_ASSERT(m_spSeqData);
    EE_ASSERT(m_spScratchPad);

    // Disable the specified evaluator and all its channels.
    NiEvaluator* pkEvaluator = GetEvaluatorAt(uiIndex);
    NiEvaluatorSPData* pkEvalSPDataArray =
        (NiEvaluatorSPData*)m_spScratchPad->GetDataBlock(SPBEVALUATORSPDATA);
    unsigned int uiNumEvalSPDatas =
        m_spScratchPad->GetNumBlockItems(SPBEVALUATORSPDATA);
    for (unsigned int ui = 0; ui < uiNumEvalSPDatas; ui++)
    {
        if (pkEvalSPDataArray[ui].GetEvaluator() == pkEvaluator)
        {
            pkEvalSPDataArray[ui].SetLOD(LODSKIPEVALUATOR);
            NiScratchPad::SegmentDataHeader* pkSegHeader =
                (NiScratchPad::SegmentDataHeader*)
                pkEvalSPDataArray[ui].GetSPSegmentData();
            if (pkSegHeader)
            {
                pkSegHeader->m_sLOD = LODSKIPEVALUATOR;
            }
        }
    }

    // Invalidate the associated data items in the pose buffer, if requested.
    if (bInvalidatePBData)
    {
        EE_ASSERT(m_pkOwner);
        EE_ASSERT(m_pkOwner->GetPoseBinding());

        const NiPoseBinding::BindInfo* pkBindInfo =
            m_pkOwner->GetPoseBinding()->GetBindInfo(pkEvaluator->GetIDTag());
        if (pkBindInfo)
        {
            unsigned int uiMaxEvalChannels =
                pkEvaluator->GetMaxEvalChannels();
            for (unsigned int uj = 0; uj < uiMaxEvalChannels; uj++)
            {
                NiPoseBufferChannelType ePBChannelType =
                    pkEvaluator->GetEvalPBChannelType(uj);
                if (ePBChannelType != PBINVALIDCHANNEL)
                {
                    NiPoseBufferHandle kPBHandle =
                        pkBindInfo->GetPBHandle(ePBChannelType);
                    if (kPBHandle.IsValid())
                    {
                        m_spPoseBuffer->SetValid(kPBHandle, false);
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiControllerSequence::DisableEvaluatorChannelAt(
    unsigned int uiEvaluatorIndex, unsigned int uiChannelIndex,
    bool bInvalidatePBData)
{
    EE_ASSERT(uiEvaluatorIndex < GetNumEvaluators());
    EE_ASSERT(uiChannelIndex < EVALMAXCHANNELS);
    EE_ASSERT(m_spSeqData);
    EE_ASSERT(m_spScratchPad);

    // Disable the specified evaluator channel.
    NiEvaluator* pkEvaluator = GetEvaluatorAt(uiEvaluatorIndex);
    NiEvaluatorSPData* pkEvalSPDataArray =
        (NiEvaluatorSPData*)m_spScratchPad->GetDataBlock(SPBEVALUATORSPDATA);
    unsigned int uiNumEvalSPDatas =
        m_spScratchPad->GetNumBlockItems(SPBEVALUATORSPDATA);
    for (unsigned int ui = 0; ui < uiNumEvalSPDatas; ui++)
    {
        if (pkEvalSPDataArray[ui].GetEvaluator() == pkEvaluator &&
            pkEvalSPDataArray[ui].GetEvalChannelIndex() == (int)uiChannelIndex)
        {
            pkEvalSPDataArray[ui].SetLOD(LODSKIPEVALUATOR);
            NiScratchPad::SegmentDataHeader* pkSegHeader =
                (NiScratchPad::SegmentDataHeader*)
                pkEvalSPDataArray[ui].GetSPSegmentData();
            if (pkSegHeader)
            {
                pkSegHeader->m_sLOD = LODSKIPEVALUATOR;
            }
            break;
        }
    }

    // Invalidate the associated data items in the pose buffer, if requested.
    if (bInvalidatePBData)
    {
        EE_ASSERT(m_pkOwner);
        EE_ASSERT(m_pkOwner->GetPoseBinding());

        const NiPoseBinding::BindInfo* pkBindInfo =
            m_pkOwner->GetPoseBinding()->GetBindInfo(pkEvaluator->GetIDTag());
        if (pkBindInfo)
        {
            NiPoseBufferChannelType ePBChannelType =
                pkEvaluator->GetEvalPBChannelType(uiChannelIndex);
            if (ePBChannelType != PBINVALIDCHANNEL)
            {
                NiPoseBufferHandle kPBHandle =
                    pkBindInfo->GetPBHandle(ePBChannelType);
                if (kPBHandle.IsValid())
                {
                    m_spPoseBuffer->SetValid(kPBHandle, false);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiControllerSequence::Update(float fTime, int iLOD,
    bool bUpdateEvaluators, bool bThrowTextKeys)
{
    // This function updates the NiControllerSequence to the specified time.
    // This operation includes updating the animation state and associated
    // spinner values, updating the evaluators and associated pose buffer
    // entries, and deactivating the NiControllerSequence when the sequence
    // runs to completion.
    //
    // fTime typically is the application's global animation time.
    //
    // iLOD specifies the current level of detail for the associated
    // controller manager. As an optimization, the animation system
    // does not update evaluators whose effects are not visible
    // due to the specified level of detail.
    //
    // When bUpdateEvaluators is false the animation system does not
    // update the evaluators nor their associated pose buffer entries.

    if (m_eState == INACTIVE)
    {
        return;
    }

    if (m_fOffset == -NI_INFINITY)
    {
        m_fOffset = -fTime;
    }

    if (m_fDestFrame != -NI_INFINITY && !m_bHoldDestFrame)
    {
        m_fOffset += m_fDestFrame;
        m_fDestFrame = -NI_INFINITY;
    }

    if (m_fStartTime == -NI_INFINITY)
    {
        if (m_eState == EASEOUT)
        {
            if (m_fEaseSpinner == 0.0f)
            {
                Deactivate(0.0f, false, false, true);
                return;
            }

            float fEaseOutTime = m_fEndTime;

            // Choose a start and end time so that the ease spinner starts
            // at the same value that it ended at.  Also, scale the total
            // ease out time by the ease spinner.  If the sequence is
            // X% eased out already, then reduce the ease out time by X%.
            m_fEndTime = fTime + m_fEaseSpinner * fEaseOutTime;

            // Note: Use the minimum here as the floating point math
            // may work out such that fStartTime > fTime, which should
            // never happen (and there are asserts about, below).
            m_fStartTime = efd::Min(m_fEndTime - fEaseOutTime, fTime);
        }
        else if (m_eState == EASEIN)
        {
            float fEaseInTime = m_fEndTime;

            // See comments above in EASEOUT case.
            m_fEndTime = fTime + (1.0f - m_fEaseSpinner) * fEaseInTime;
            m_fStartTime = efd::Min(m_fEndTime - fEaseInTime, fTime);
        }
        else
        {
            m_fStartTime = fTime;
            m_fEndTime = fTime + m_fEndTime;
        }
    }

    m_fEaseSpinner = 1.0f;
    m_fTransSpinner = 1.0f;
    switch (m_eState)
    {
        case EASEIN:
            if (fTime < m_fEndTime)
            {
                EE_ASSERT(fTime >= m_fStartTime && m_fEndTime != m_fStartTime);
                m_fEaseSpinner = (fTime - m_fStartTime) / (m_fEndTime -
                    m_fStartTime);
            }
            else
            {
                m_eState = ANIMATING;
            }
            break;
        case TRANSDEST:
            if (fTime < m_fEndTime)
            {
                EE_ASSERT(fTime >= m_fStartTime && m_fEndTime != m_fStartTime);
                m_fTransSpinner = (fTime - m_fStartTime) / (m_fEndTime -
                    m_fStartTime);
            }
            else
            {
                if (m_fDestFrame != -NI_INFINITY)
                {
                    // This case is hit when we were blending in this
                    // sequence. In this case, we need to reset the sequence
                    // offset and clear the destination frame.
                    EE_ASSERT(m_bHoldDestFrame);
                    m_bHoldDestFrame = false;
                    m_fOffset = -fTime + m_fDestFrame;
                    m_fDestFrame = -NI_INFINITY;
                }
                m_eState = ANIMATING;
            }
            break;
        case EASEOUT:
            if (fTime < m_fEndTime)
            {
                EE_ASSERT(fTime >= m_fStartTime && m_fEndTime != m_fStartTime);
                m_fEaseSpinner = (m_fEndTime - fTime) / (m_fEndTime -
                    m_fStartTime);
            }
            else
            {
                DoUpdate(m_fEndTime, iLOD, false, bThrowTextKeys);
                Deactivate(0.0f, false, false, true);
                return;
            }
            break;
        case MORPHSOURCE:
        {
            EE_ASSERT(m_spPartnerSequence);

            // Compute initial offset for partner sequence, undoing phase
            // and frequency adjustments. This assumes the phase and
            // frequency will not change between now and the end time of
            // the morph.
            float fStartFrame = m_spPartnerSequence
                ->FindCorrespondingMorphFrame(this, m_fOffset + fTime);
            fStartFrame /= m_spPartnerSequence->m_fFrequency;
            m_spPartnerSequence->m_fOffset = fStartFrame - fTime;
            m_spPartnerSequence->Update(fTime, 0, false, false);

            // Change sequence state appropriately.
            m_eState = TRANSSOURCE;

            // This case statement intentionally does not break. The code
            // for the TRANSSOURCE case should be subsequently run.
        }
        case TRANSSOURCE:
            if (fTime < m_fEndTime)
            {
                EE_ASSERT(fTime >= m_fStartTime && m_fEndTime != m_fStartTime);
                m_fTransSpinner = (m_fEndTime - fTime) / (m_fEndTime -
                    m_fStartTime);
            }
            else
            {
                DoUpdate(m_fEndTime, iLOD, false, bThrowTextKeys);
                Deactivate(0.0f, false, false, true);
                return;
            }
            break;
        case ANIMATING:
            // Do nothing.
            break;
        default:
            // All cases should be covered by prior code.
            EE_ASSERT(false);
            break;
    }

    DoUpdate(fTime, iLOD, bUpdateEvaluators, bThrowTextKeys);
}

//--------------------------------------------------------------------------------------------------
void NiControllerSequence::DoUpdate(float fTime,
    int iLOD, bool bUpdateEvaluators, bool bThrowTextKeys)
{
    float fUpdateTime;
    if (m_fDestFrame != -NI_INFINITY)
    {
        fUpdateTime = m_fDestFrame;
    }
    else if (m_spPartnerSequence)
    {
        if (m_spPartnerSequence->GetLastTime() !=
            m_spPartnerSequence->m_fOffset + fTime)
        {
            m_spPartnerSequence->Update(fTime, iLOD, false, true);
        }

        fUpdateTime = FindCorrespondingMorphFrame(m_spPartnerSequence,
            m_spPartnerSequence->m_fOffset + fTime);
        fUpdateTime /= m_fFrequency;
    }
    else
    {
        fUpdateTime = m_fOffset + fTime;
    }

    float fLastScaledTime = m_fLastScaledTime;
    float fPrevWeightedLastTime = m_fWeightedLastTime;

    // Update the evaluators and associated pose buffer entries.
    float fScaledTime = ComputeScaledTime(fUpdateTime);

    if (bUpdateEvaluators)
    {
        m_spScratchPad->Update(fTime, fScaledTime, (NiInt16)iLOD,
            m_spPoseBuffer);
    }

    float fLastUpdateTime = m_fLastUpdateTime;
    m_fLastUpdateTime = fTime;
    if (bThrowTextKeys && m_spTextKeys)
    {
        ThrowAllTextKeys(fLastUpdateTime, fTime,
            fLastScaledTime, fScaledTime, fPrevWeightedLastTime,
            m_fWeightedLastTime);
    }
}

//--------------------------------------------------------------------------------------------------
void NiControllerSequence::ThrowAllTextKeys(float fLastTime, float fTime,
    float fLastScaledTime, float fScaledTime,
    float fLastWeightedTime, float fWeightedTime)
{
    fLastScaledTime = NiMax(0.0f, fLastScaledTime);
    fLastWeightedTime = NiMax(0.0f, fLastWeightedTime);

    // Correct for ComputeScaledTime inconsistencies on the first frame.
    fScaledTime = NiSelect(fScaledTime == m_fDuration &&
        m_eCycleType == NiTimeController::LOOP, 0.0f, fScaledTime);
    fLastScaledTime = NiSelect(fLastScaledTime == m_fDuration &&
        m_eCycleType == NiTimeController::LOOP, 0.0f, fLastScaledTime);

    // Correct for weighted time inconsistencies when a sequence is
    // synchronized and loops.
    fWeightedTime = NiSelect(fWeightedTime < fLastWeightedTime,
        fWeightedTime + m_fDuration, fWeightedTime);

    // On the first update, use the current time as the last time.
    fLastTime = NiSelect(fLastTime != -NI_INFINITY, fLastTime, fTime);

    float fFrameDuration = fWeightedTime - fLastWeightedTime;
    bool bNeedFirstTextKey = m_bNeedFirstTextKey;
    m_bNeedFirstTextKey = false;

    // If we loop past the end of the sequence:
    if (fScaledTime < fLastScaledTime || fFrameDuration >= m_fDuration &&
        m_eCycleType == NiTimeController::LOOP)
    {
        float fTimeDuration = fTime - fLastTime;
        float fStartTime = fLastTime;
        float fElapsed = 0.0f;

        // Throw text keys to end of sequence: (fLastScaledTime, m_fDuration]
        {
            float fTimeToEnd = m_fDuration - fLastScaledTime;
            float fEndPercent = fTimeDuration / fFrameDuration;
            float fTimeAtEnd = fTimeToEnd * fEndPercent + fStartTime;

            ThrowTextKeysBetween(fStartTime, fTimeAtEnd,
                fLastScaledTime, m_fDuration, fTimeToEnd,
                bNeedFirstTextKey);

            fElapsed += fTimeToEnd;
            fLastTime = fTimeAtEnd;
            fLastScaledTime = 0.0f;
        }

        // Loop any remaining times: [0.0f, m_fDuration]
        while (fFrameDuration - fElapsed > m_fDuration)
        {
            float fEndPercent = (fElapsed + m_fDuration) / fFrameDuration;
            float fTimeAtEnd = fEndPercent * fTimeDuration + fStartTime;

            ThrowTextKeysBetween(fLastTime, fTimeAtEnd, 0.0f,
                m_fDuration, m_fDuration, true);

            fElapsed += m_fDuration;
            fLastTime = fTimeAtEnd;
        }

        // Throw last segment of times: [0, fScaledTime]
        ThrowTextKeysBetween(fLastTime, fTime, 0.0f, fScaledTime,
            fScaledTime, true);
    }
    else if (fScaledTime > fLastScaledTime || bNeedFirstTextKey)
    {
        ThrowTextKeysBetween(fLastTime, fTime,
            fLastScaledTime, fScaledTime, fFrameDuration,
            bNeedFirstTextKey);
    }
}

//--------------------------------------------------------------------------------------------------
void NiControllerSequence::ThrowTextKeysBetween(float fLastTime,
    float fTime, float fLastScaledTime, float fScaledTime,
    float fFrameDuration, bool bIncludeFirst)
{
    EE_ASSERT(fScaledTime >= fLastScaledTime);

    unsigned int uiNumKeys;
    NiTextKey* pkKeys = m_spTextKeys->GetKeys(uiNumKeys);

    float fTimeRange = fTime - fLastTime;
    float fTimeToFrameRatio = NiSelect(fFrameDuration <= 0.0f, 0.0f,
        fTimeRange / fFrameDuration);

    for (unsigned int i = 0; i < uiNumKeys; i++)
    {
        float fFrame = pkKeys[i].GetTime();
        if (fFrame < fLastScaledTime || fFrame > fScaledTime ||
            !bIncludeFirst && fFrame == fLastScaledTime)
        {
            continue;
        }

        const NiFixedString& kText = pkKeys[i].GetText();

        for (unsigned int j = 0; j < m_kTextKeyCallbacks.GetSize(); j++)
        {
            TextKeyCallbackData& kData = m_kTextKeyCallbacks.GetAt(j);
            if (!kData.m_spMatch->IsKeyMatch(kText))
                continue;

            float fFrameOffset = fFrame - fLastScaledTime;
            float fEventTime = fLastTime + fFrameOffset * fTimeToFrameRatio;

            kData.m_pkCallback->TextKeyEvent(this, kData.m_spMatch,
                kText, fEventTime);
        }
    }
}

//--------------------------------------------------------------------------------------------------
NiControllerSequence* NiControllerSequence::Activate(
    NiControllerManager* pkOwner, NiSequenceData* pkSeqData,
    int iPriority, float fWeight, float fEaseInTime,
    NiControllerSequence* pkTimeSyncSeq, float fFrequency,
    float fStartFrame, bool bTransition, bool bHoldTransitionFrame,
    bool bAdditiveBlend,
    float fAdditiveRefFrame)
{
    // This static function plays the specified NiSequenceData on the
    // specified NiControllerManager at the specified priority and weight.
    // An NiControllerSequence is created to represent the active animation
    // and maintain the required instance data to play back the sequence on
    // the given controller manager.
    //
    // fEaseInTime specifies a time period over which the weight will be
    // ramped up from 0.0 to the weight specified by fWeight.
    //
    // pkTimeSyncSeq is an optional sequence with which the time for the
    // activated sequence should be synchronized. If at least one pair of
    // identical morph text keys is not found in each sequence, this
    // function will return NULL.
    //
    // fFrequency specifies the speed at which the sequence should be played
    // where 2.0 is twice the normal playback speed. When fFrequency equals
    // SEQUENCE_DATA_FREQUENCY the frequency defaults to that of pkSeqData.
    //
    // fStartFrame specifies the frame at which to begin sequence playback.
    // This frame time is interpreted in the local timeline of pkSeqData
    // whereby 0.0 is its first frame and pkSeqData->GetDuration is its
    // final frame.
    //
    // bTransition specifies when this activation is part of a transition
    // with another sequence, such as a cross fade.
    //
    // bHoldTransitionFrame specifies when the starting frame should be held
    // for the duration of the ease-in interval.
    //
    // This function returns the newly created NiControllerSequence upon
    // successful activation, and NULL otherwise.
    //
    // Upon activation, callback objects associated with pkSeqData are added
    // to the newly created NiControllerSequence using a simple matching
    // scheme based on the associated NiControllerManager.
    // The ActivationChanged function of these added callbacks are
    // executed to communicate the initial animation state of the
    // NiControllerSequence.

    EE_ASSERT(pkOwner && pkSeqData);
    EE_ASSERT(pkOwner->GetPoseBinding());

    if (!pkOwner->IsKnownSequenceData(pkSeqData))
    {
        NiOutputDebugString("Attempting to activate a sequence data "
            "that has not been added to the specified controller manager!\n");
        return NULL;
    }

    // Check if sync sequence is valid.
    if (pkTimeSyncSeq)
    {
        if (!pkSeqData->VerifyMatchingMorphKeys(pkTimeSyncSeq))
        {
            return NULL;
        }
    }

    // Sanity check for additive blending.
    if (bAdditiveBlend)
    {
        EE_ASSERT(!bTransition);
        EE_ASSERT(!bHoldTransitionFrame);
        EE_ASSERT(fAdditiveRefFrame >= 0.0f);
        EE_ASSERT(fAdditiveRefFrame <= pkSeqData->GetDuration());
    }

    // Check if the owner has a recycled sequence that we can activate.
    NiControllerSequencePtr spSequence = pkOwner->GetRecycledSequence();
    if (spSequence)
    {
        // Reset selected data members to initial state.
        spSequence->m_usNumPoseBufferItems = 0;
        spSequence->m_sLastPoseBufferUpdateLOD = LODSKIPEVALUATOR;
        spSequence->m_fLastPoseBufferUpdateTime = -NI_INFINITY;
        spSequence->m_fTransSpinner = 0.0f;
        spSequence->m_fEaseSpinner = 0.0f;
        spSequence->m_fWeightedLastTime = -NI_INFINITY;
        spSequence->m_fLastScaledTime = -NI_INFINITY;
        spSequence->m_fOffset = -NI_INFINITY;
        spSequence->m_fStartTime = -NI_INFINITY;
        spSequence->m_fEndTime = -NI_INFINITY;
        spSequence->m_kTextKeyCallbacks.RemoveAll();
        spSequence->m_bNeedFirstTextKey = true;
    }
    else
    {
        // Create new sequence.
        spSequence = NiNew NiControllerSequence();
    }

    // Set a "unique" activation ID for this sequence instance.
    // This is intended to resolve aliasing issues when sequences
    // are recycled.
    spSequence->m_uiActivationID = pkOwner->GetNextActivationID();

    // Set parameters.
    spSequence->m_spSeqData = pkSeqData;
    spSequence->m_iPriority = iPriority;
    spSequence->SetWeight(fWeight);
    spSequence->m_fDuration = pkSeqData->GetDuration();
    spSequence->SetCycleType(pkSeqData->GetCycleType());
    spSequence->m_spTextKeys = pkSeqData->GetTextKeys();
    if (fFrequency == SEQUENCE_DATA_FREQUENCY)
    {
        spSequence->m_fFrequency = pkSeqData->GetFrequency();
    }
    else
    {
        spSequence->m_fFrequency = fFrequency;
    }
    EE_ASSERT(spSequence->m_fFrequency > 0);

    spSequence->m_fDestFrame = fStartFrame / spSequence->m_fFrequency;

    spSequence->m_bHoldDestFrame = false;
    if (fEaseInTime > 0.0f)
    {
        if (bTransition)
        {
            spSequence->m_eState = TRANSDEST;
            spSequence->m_bHoldDestFrame = bHoldTransitionFrame;
        }
        else
        {
            spSequence->m_eState = EASEIN;
        }
        spSequence->m_fStartTime = -NI_INFINITY;
        spSequence->m_fEndTime = fEaseInTime;
    }
    else
    {
        spSequence->m_eState = ANIMATING;
    }

    spSequence->m_fLastTime = -NI_INFINITY;
    spSequence->m_spPartnerSequence = pkTimeSyncSeq;

    // Add to owner.
    pkOwner->AddSequence(spSequence);

    // Create and initialize the pose buffer and scratch pad.
    NiPoseBinding* pkPoseBinding = pkOwner->GetPoseBinding();
    if (spSequence->m_spPoseBuffer)
    {
        // Bring the pose buffer up-to-date with latest pose binding.
        // Don't copy existing data (i.e. validity flags and constant items).
        EE_ASSERT(pkPoseBinding ==
            spSequence->m_spPoseBuffer->GetPoseBinding());
        if (!spSequence->m_spPoseBuffer->AddNewItemsFromPoseBinding(
            false, false))
        {
            // Invalidate all items in the pose buffer.
            spSequence->m_spPoseBuffer->InvalidateAllItems();
        }
    }
    else
    {
        spSequence->m_spPoseBuffer = NiNew NiPoseBuffer(pkPoseBinding);
    }

    if (spSequence->m_spScratchPad)
    {
        spSequence->m_spScratchPad->InitRecycledScratchPad(spSequence);
    }
    else
    {
        spSequence->m_spScratchPad =
            (NiScratchPad*)NiNew NiScratchPad(spSequence);
    }

    // Calculate initial reference frame, if additively blending.
    if (bAdditiveBlend)
    {
        if (!spSequence->m_spAdditiveRefFrame)
        {
            spSequence->m_spAdditiveRefFrame = NiNew NiPoseBuffer(pkPoseBinding);
        }

        // Copy all flags, weights, and number of elements from existing pose buffer.
        spSequence->m_spPoseBuffer->CopyInto(spSequence->m_spAdditiveRefFrame);

        // Update additive ref frame pose buffer to the value at the ref frame.
        // This will be used to subtract from the current frame.
        float fTime = 0.0f;
        float fScaledTime = fAdditiveRefFrame;
        short sLOD = 0;

        spSequence->m_spScratchPad->Update(
            fTime,
            fScaledTime,
            sLOD,
            spSequence->m_spAdditiveRefFrame);

        // Clear the last update time so that the scratch pad will still push
        // the values to the original pose buffer.
        spSequence->m_spScratchPad->ClearLastUpdateTime();
    }
    else
    {
        spSequence->m_spAdditiveRefFrame = NULL;
    }
    EE_ASSERT(bAdditiveBlend == spSequence->IsAdditiveBlend());

    // Add default callbacks which match the associated controller manager.
    unsigned int uiCallbackCount =
        pkSeqData->GetDefaultActivationCallbackCount();
    for (unsigned int ui = 0; ui < uiCallbackCount; ui++)
    {
        NiSequenceData::ActivationCallback* pkCallback;
        NiControllerManager* pkManager;
        pkSeqData->GetDefaultActivationCallbackAt(ui, pkCallback, pkManager);
        if (!pkManager || pkManager == pkOwner)
        {
            spSequence->AddActivationCallback(pkCallback);
        }
    }

    // Process callbacks.
    uiCallbackCount = spSequence->m_kActivationCallbacks.GetSize();
    for (unsigned int ui = 0; ui < uiCallbackCount; ui++)
    {
        spSequence->m_kActivationCallbacks.GetAt(ui)->ActivationChanged(
           spSequence, spSequence->m_eState);
    }

    return spSequence;
}

//--------------------------------------------------------------------------------------------------
bool NiControllerSequence::Reactivate(int iPriority, float fWeight,
    float fEaseInTime, NiControllerSequence* pkTimeSyncSeq,
    float fStartFrame, bool bTransition, bool bProcessCallbacks)
{
    // This function reactivates the temporarily inactive NiControllerSequence
    // using the specified priority and weight.
    //
    // fEaseInTime specifies a time period over which the weight will be
    // ramped up from 0.0 to the weight specified by fWeight.
    //
    // pkTimeSyncSeq is an optional sequence with which the time for the
    // activated sequence should be synchronized. If at least one pair of
    // identical morph text keys is not found in each sequence, this
    // function will return false.
    //
    // fStartFrame specifies the frame at which to reactivate sequence
    // playback. This frame time is interpreted in the local timeline of the
    // NiControllerSequence whereby 0.0 is the first frame and GetDuration
    // is the final frame. When fStartFrame equals CURRENT_FRAME the
    // sequence continues playback from its current frame.
    //
    // bTransition specifies when this reactivation is part of a transition
    // with another sequence, such as a morph transition.
    //
    // bProcessCallbacks specifies when the ActivationChanged function of all
    // callback objects associated with the NiControllerSequence should be
    // called to communicate the latest animation state.
    //
    // This function returns true when successful, and false otherwise.

    if (m_eState == ANIMATING)
    {
        NiOutputDebugString("Attempting to reactivate a sequence that is "
            "already animating!\n");
        return false;
    }

    m_spPartnerSequence = NULL;
    if (pkTimeSyncSeq)
    {
        if (!VerifyDependencies(pkTimeSyncSeq) ||
            !VerifyMatchingMorphKeys(pkTimeSyncSeq))
        {
            return false;
        }
        m_spPartnerSequence = pkTimeSyncSeq;
    }

    // Set parameters.
    m_iPriority = iPriority;
    SetWeight(fWeight);

    if (fEaseInTime > 0.0f)
    {
        if (bTransition)
        {
            m_eState = TRANSDEST;
        }
        else
        {
            m_eState = EASEIN;
        }
        m_fStartTime = -NI_INFINITY;
        m_fEndTime = fEaseInTime;
    }
    else
    {
        m_eState = ANIMATING;
    }

    if (fStartFrame != CURRENT_FRAME)
    {
        m_fOffset = -NI_INFINITY;
        m_fDestFrame = fStartFrame / m_fFrequency;
    }

    m_fLastTime = -NI_INFINITY;

    // Process callbacks.
    if (bProcessCallbacks)
    {
        unsigned int uiCount = m_kActivationCallbacks.GetSize();
        for (unsigned int ui = 0; ui < uiCount; ui++)
        {
            m_kActivationCallbacks.GetAt(ui)->ActivationChanged(this,
                m_eState);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiControllerSequence::Deactivate(float fEaseOutTime, bool bTransition,
    bool bHoldTransitionFrame, bool bProcessCallbacks)
{
    // This function deactivates the NiControllerSequence.
    //
    // fEaseOutTime specifies a time period over which the sequence weight
    // will be ramped down from its set value to 0.0.
    //
    // bTransition specifies when this deactivation is part of a transition
    // with another sequence, such as a cross fade.
    //
    // bHoldTransitionFrame specifies when the current frame should be held
    // for the duration of the ease-out interval.
    //
    // bProcessCallbacks specifies when the ActivationChanged function of all
    // callback objects associated with the NiControllerSequence should be
    // called to communicate the latest animation state.
    //
    // This function returns true when successful, and false otherwise.

    if (m_eState == INACTIVE)
    {
        return false;
    }

    m_bHoldDestFrame = false;

    if (fEaseOutTime > 0.0f)
    {
        if (bTransition)
        {
            m_eState = TRANSSOURCE;
            if (bHoldTransitionFrame)
            {
                m_bHoldDestFrame = true;
                // Check if we already have a destination frame.
                if (m_fDestFrame == -NI_INFINITY)
                {
                    // Is the last scaled time valid?
                    if (m_fLastScaledTime != -NI_INFINITY)
                    {
                        m_fDestFrame = m_fLastScaledTime / m_fFrequency;
                    }
                    else
                    {
                        m_fDestFrame = 0;
                    }
                }
            }
        }
        else
        {
            m_eState = EASEOUT;
        }

        m_fStartTime = -NI_INFINITY;
        m_fEndTime = fEaseOutTime;
    }
    else
    {
        // Store the new offset.
        if (m_fLastTime != -NI_INFINITY &&
            m_fOffset != -NI_INFINITY &&
            m_fWeightedLastTime != -NI_INFINITY)
        {
            m_fOffset += (m_fWeightedLastTime / m_fFrequency) - m_fLastTime;
        }

        m_eState = INACTIVE;

        m_spPartnerSequence = NULL;
        m_fDestFrame = -NI_INFINITY;
    }

    // Process callbacks.
    if (bProcessCallbacks)
    {
        // Process this list in reverse, because it is possible for the callback to remove
        // itself from the list during processing.
        unsigned int uiCount = m_kActivationCallbacks.GetSize();
        for (unsigned int ui = uiCount; ui > 0; --ui)
        {
            m_kActivationCallbacks.GetAt(ui - 1)->ActivationChanged(this, m_eState);
        }

        // The INACTIVE state indicates the sequence has run to completion.
        // Remove all activation callbacks so they cannot be called.
        if (m_eState == INACTIVE)
        {
            m_kActivationCallbacks.RemoveAll();
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
NiControllerSequence* NiControllerSequence::StartBlend(
    NiSequenceData* pkDestSeqData,
    float fDuration, float fDestFrame, int iPriority,
    float fSourceWeight, float fDestWeight,
    NiControllerSequence* pkDestTimeSyncSeq, float fDestFrequency)
{
    // This function initiates a blend transition from this source
    // NiControllerSequence to the specified destination NiSequenceData
    // over the specified duration (given in seconds).
    //
    // fDestFrame indicates the frame in the destination sequence to which
    // to blend. If fDestFrame is 0.0, the blend will be to the first frame
    // of the destination sequence.
    //
    // iPriority specifies the priority at which to (re)activate both
    // sequences.
    //
    // fSourceWeight and fDestWeight specify the final weights to assign to
    // the source and destination NiControllerSequence objects.
    //
    // If a time sync sequence is provided, the destination
    // NiControllerSequence is synchronized to it.
    //
    // fDestFrequency specifies the speed at which the destination sequence
    // should be played. When fDestFrequency equals SEQUENCE_DATA_FREQUENCY
    // the destination frequency defaults to that of pkDestSeqData.
    //
    // This function returns the newly created destination
    // NiControllerSequence on success, and NULL if the blend
    // cannot be launched successfully.

    EE_ASSERT(pkDestSeqData && m_pkOwner);

    // Activate destination sequence.
    NiControllerSequence* pkDestSequence = NiControllerSequence::Activate(
        m_pkOwner, pkDestSeqData, iPriority, fDestWeight, fDuration,
        pkDestTimeSyncSeq, fDestFrequency, fDestFrame, true, true);
    if (!pkDestSequence)
    {
        return NULL;
    }

    // Temporarily deactivate source sequence so we can reactivate
    // it with a different state. Don't process callbacks.
    Deactivate(0.0f, false, false, false);

    // Reactivate source sequence with new state. Don't process callbacks.
    if (!Reactivate(iPriority, fSourceWeight, 0.0f, NULL,
        CURRENT_FRAME, false, false))
    {
        pkDestSequence->Deactivate(0.0f);
        return NULL;
    }

    // Ease out source sequence. Process callbacks.
    Deactivate(fDuration, true, true, true);

    return pkDestSequence;
}

//--------------------------------------------------------------------------------------------------
NiControllerSequence* NiControllerSequence::StartMorph(
    NiSequenceData* pkDestSeqData,
    float fDuration, int iPriority, float fSourceWeight, float fDestWeight,
    float fDestFrequency)
{
    // This function performs a morph transition from this source
    // NiControllerSequence to the specified destination NiSequenceData.
    //
    // fDuration specifies the duration of the transition in seconds.
    //
    // iPriority indicates the priority at which each sequence should
    // be (re)activated.
    //
    // fSourceWeight and fDestWeight specify the final weights to assign
    // to the source and destination NiControllerSequence objects.
    //
    // fDestFrequency specifies the speed at which the destination sequence
    // should be played. When fDestFrequency equals SEQUENCE_DATA_FREQUENCY
    // the destination frequency defaults to that of pkDestSeqData.
    //
    // This function returns the newly created destination
    // NiControllerSequence on success, and NULL if the
    // morph transition cannot be successfully started.

    EE_ASSERT(pkDestSeqData && m_pkOwner);

    // Activate destination sequence.
    NiControllerSequence* pkDestSequence = NiControllerSequence::Activate(
        m_pkOwner, pkDestSeqData, iPriority, fDestWeight, fDuration,
        NULL, fDestFrequency, 0.0f, true, false);
    if (!pkDestSequence)
    {
        return NULL;
    }

    // Temporarily deactivate source sequence so we can reactivate
    // it with a different state. Don't process callbacks.
    Deactivate(0.0f, false, false, false);

    // Reactivate source sequence with new state. Process callbacks.
    if (!Reactivate(iPriority, fSourceWeight, fDuration, pkDestSequence,
        CURRENT_FRAME, true, true))
    {
        pkDestSequence->Deactivate(0.0f);
        return NULL;
    }

    // Set sequence state for source sequence. The source sequence state is
    // set to MORPHSOURCE so that special processing can be done on the first
    // update. After this processing is complete, the state will be changed to
    // TRANSSOURCE.
    m_eState = MORPHSOURCE;

    return pkDestSequence;
}

//--------------------------------------------------------------------------------------------------
float NiControllerSequence::FindCorrespondingMorphFrame(
    NiControllerSequence* pkSourceSequence, float fSourceSequenceTime)
{
    // This function uses all existing "morph:" key pairs between this
    // sequence and pkSourceSequence to map the time in pkSourceSequence,
    // fSourceSequenceTime, to a corresponding time in this sequence.
    // fSourceSequenceTime need not be within the range of pkSourceSequence
    // since ComputeScaledTime will be called on it. The time returned is
    // within the range of this sequence.

    // Compute the scaled source sequence time.
    float fScaledSourceTime = pkSourceSequence->ComputeScaledTime(
        fSourceSequenceTime, false);

    // Find the "morph:" keys in the (first set of text keys of the) source
    // sequence surrounding fSourceSequenceTime.
    // Assumptions:
    // - keys are in chronological order
    // - at least one "morph:" key exists
    // - in any "combined sequence" (>1 set of _text keys_), the _first_ set
    //   determines the morph values
    // - MORPHING SEQUENCES ARE PURELY LOOPED (not reversed or clamped)
    //   If they are not looped, the morph will only work if, during
    //   the transition, neither sequence hits its endpoint
    const char* pcMorphTextKey = NiAnimationConstants::GetMorphTextKey();
    int iMorphTextKeyLength = NiAnimationConstants::GetMorphTextKeyLength();
    unsigned int uiNumKeys;
    NiTextKey* pkKeys = pkSourceSequence->GetTextKeys()->GetKeys(uiNumKeys);
    NiTextKey* pkFirstKey = NULL;
    NiTextKey* pkPrevKey = NULL;
    NiTextKey* pkNextKey = NULL;
    NiTextKey* pkLastKey = NULL;
    unsigned int ui;
    for (ui = 0; ui < uiNumKeys; ui++)
    {
        const char* pcKeyText = pkKeys[ui].GetText();
        if (pcKeyText && strncmp(pcKeyText, pcMorphTextKey,
            iMorphTextKeyLength) == 0)
        {
            pkLastKey = &(pkKeys[ui]);
            if (!pkFirstKey)
                pkFirstKey = pkLastKey;

            if (pkKeys[ui].GetTime() < fScaledSourceTime)
                pkPrevKey = pkLastKey;
            else if (!pkNextKey)
                pkNextKey = pkLastKey;
        }
    }

    // If this assertion is hit, the base sequence does not have
    // any "morph:" text keys.
    EE_ASSERT(pkNextKey || pkPrevKey);

    bool bCreateNext = false;

    // Find the frame location relative to the base morph frames
    float fSourceRatio;
    if (!pkNextKey || !pkPrevKey)
    {
        // If either prev or next key is not found, supply one based on
        //   the CycleType attribute of the sequence
        if (!pkNextKey)
        {
            // Assuming LOOP
            float fStartTime = pkPrevKey->GetTime();
            float fEndTime = pkSourceSequence->m_fDuration +
                pkFirstKey->GetTime();
            fSourceRatio = (fScaledSourceTime - fStartTime) /
                (fEndTime - fStartTime);
            pkNextKey = pkFirstKey;
            bCreateNext = true;
        }
        else // (!pkPrevKey)
        {
            // Assuming LOOP
            float fStartTime = pkLastKey->GetTime() -
                pkSourceSequence->m_fDuration;
            float fEndTime = pkNextKey->GetTime();
            fSourceRatio = (fScaledSourceTime - fStartTime) /
                (fEndTime - fStartTime);
            pkPrevKey = pkLastKey;
        }
    }
    else
    {
        float fStartTime = pkPrevKey->GetTime();
        float fEndTime = pkNextKey->GetTime();
        fSourceRatio = (fScaledSourceTime - fStartTime) /
            (fEndTime - fStartTime);
    }

    // Find the corresponding keys in the Target sequence
    // Assumptions:
    // - there is exactly one "morph:" key in the Target sequence with the
    //   same name as each key in the base sequence
    // - the corresponding keys are in the same order in both sequences
    // - if the morph extends beyond the length of a sequence (ie loops or
    //   reverses), both sequences have the same CycleType value
    pkKeys = m_spTextKeys->GetKeys(uiNumKeys);

    bool bPrevFound = (pkPrevKey == NULL);
    bool bNextFound = (pkNextKey == NULL);
    NiTextKey* pkTargetPrevKey = NULL;
    NiTextKey* pkTargetNextKey = NULL;
    for (ui = 0; ui < uiNumKeys && !(bPrevFound && bNextFound); ui++)
    {
        if (!bPrevFound &&
            strcmp(pkKeys[ui].GetText(), pkPrevKey->GetText()) == 0)
        {
            pkTargetPrevKey = &(pkKeys[ui]);
            bPrevFound = true;
        }
        if (!bNextFound &&
            strcmp(pkKeys[ui].GetText(), pkNextKey->GetText()) == 0)
        {
            pkTargetNextKey = &(pkKeys[ui]);
            bNextFound = true;
        }
    }

    // If this assertion is hit, the target sequence does not contain a
    // corresponding "morph:" text key.
    EE_ASSERT(bPrevFound && bNextFound);

    // Find the corresponding frame based on the target morph frames
    float fTargetHi = 0.0f, fTargetLo = 0.0f;

    if (pkTargetPrevKey == pkTargetNextKey)
    {
        fTargetLo = pkTargetPrevKey->GetTime();
        fTargetHi = pkTargetNextKey->GetTime();

        // We are in the situation where only 1 morph tag was found
        if (bCreateNext)
            fTargetHi += m_fDuration;
        else // bCreatePrev implied
            fTargetLo -= m_fDuration;
    }
    else
    {
        if (pkTargetNextKey)
            fTargetHi = pkTargetNextKey->GetTime();
        if (pkTargetPrevKey)
            fTargetLo = pkTargetPrevKey->GetTime();

        // Handle case where tags "cross"
        if (fTargetLo > fTargetHi)
            fTargetHi += m_fDuration;
    }

    float fTargetFrame = fTargetLo + fSourceRatio * (fTargetHi - fTargetLo);

    // Ensure the target frame is within the range of this sequence.
    if (fTargetFrame < 0.0f)
        fTargetFrame += m_fDuration;
    else if (fTargetFrame > m_fDuration)
        fTargetFrame -= m_fDuration;

    return fTargetFrame;
}

//--------------------------------------------------------------------------------------------------
float NiControllerSequence::ComputeScaledTime(float fTime,
    bool bStoreLastTime)
{
    // This function converts the incoming time value to scaled time using
    // the frequency of the NiControllerSequence in combination with its
    // cycle type. The returned scaled time represents the local time units
    // found in the animation keys whereby 0.0 is the first frame and
    // GetDuration is the final frame. This function stores the latest values
    // when bStoreLastTime is true. This cached data can be retrieved via
    // GetLastTime and GetLastScaledTime.

    float fScaledTime = 0.0f;

    float fWeightedLastTime = m_fWeightedLastTime;
    float fDeltaTime;
    if (m_fLastTime == -NI_INFINITY)
    {
        fWeightedLastTime = 0.0f;
        fDeltaTime = fTime;
    }
    else
    {
        // Calculate time since last update
        fDeltaTime = fTime - m_fLastTime;
    }


    // Scale time linearly by frequency value
    // specified by the application.
    //
    // time' = freq * time
    //
    fWeightedLastTime += fDeltaTime * m_fFrequency;
    fScaledTime = fWeightedLastTime;

    // Switch on m_CycleType
    switch (m_eCycleType)
    {
        case NiTimeController::LOOP:
        {
            // Loop -- only need fractional time
            float fSpan = m_fDuration;
            if (fSpan != 0.0f)
            {
                if (fScaledTime != m_fDuration)
                {
                    fScaledTime = NiFmod(fScaledTime, fSpan);
                }

                if (fScaledTime < 0.0f)
                {
                    fScaledTime += fSpan;
                }
            }
            else
            {
                fScaledTime = 0.0f;
            }
            break;
        }
        case NiTimeController::CLAMP:
            // Clamp - do nothing, conditions below will handle clamping.
            break;
        default:
            // Only LOOP and CLAMP cycle types are supported by sequences.
            EE_ASSERT(false);
            break;
    }

    if (fScaledTime > m_fDuration)
    {
        fScaledTime = m_fDuration;
    }
    else if (fScaledTime < 0.0f)
    {
        fScaledTime = 0.0f;
    }

    if (bStoreLastTime)
    {
        m_fWeightedLastTime = fWeightedLastTime;
        m_fLastTime = fTime;
        m_fLastScaledTime = fScaledTime;
    }

    return fScaledTime;
}

//--------------------------------------------------------------------------------------------------
void NiControllerSequence::PrepareForRecycling()
{
    // Remove all activation callbacks.
    m_kActivationCallbacks.RemoveAll();

    // Clean up the scratch pad.
    if (m_spScratchPad)
    {
        m_spScratchPad->PrepareForRecycling();
    }

    // Release all held objects except owner, pose buffer, sequence data,
    // and scratch pad.
    m_spPartnerSequence = NULL;
    m_spTextKeys = NULL;
}

//--------------------------------------------------------------------------------------------------
bool NiControllerSequence::VerifyMatchingMorphKeys(
    NiControllerSequence* pkTimeSyncSeq)
{
    // This function determines if this sequence and pkTimeSyncSeq have
    // matching morph key pairs. If so, this sequence may use pkTimeSyncSeq
    // for time synchronization.

    EE_ASSERT(pkTimeSyncSeq);

    NiTextKeyExtraData* pkTextKeys1 = pkTimeSyncSeq->GetTextKeys();
    NiTextKeyExtraData* pkTextKeys2 = m_spTextKeys;
    if (!pkTextKeys1 || !pkTextKeys2)
    {
        return false;
    }

    unsigned int uiNumKeys1, uiNumKeys2;
    NiTextKey* pkKeys1 = pkTextKeys1->GetKeys(uiNumKeys1);
    NiTextKey* pkKeys2 = pkTextKeys2->GetKeys(uiNumKeys2);

    // Check that every "morph:" text key in pkTimeSyncSeq has an exact
    // match in this sequence.
    const char* pcMorphTextKey = NiAnimationConstants::GetMorphTextKey();
    int iMorphTextKeyLength = NiAnimationConstants::GetMorphTextKeyLength();
    bool bMorphKeysExist1 = false;
    unsigned int ui;
    for (ui = 0; ui < uiNumKeys1; ui++)
    {
        const char* pcText1 = pkKeys1[ui].GetText();
        EE_ASSERT(pcText1);
        if (strncmp(pcText1, pcMorphTextKey, iMorphTextKeyLength) == 0)
        {
            bMorphKeysExist1 = true;
            bool bFound = false;
            for (unsigned int uj = 0; uj < uiNumKeys2; uj++)
            {
                const char* pcText2 = pkKeys2[uj].GetText();
                EE_ASSERT(pcText2);
                if (strcmp(pcText1, pcText2) == 0)
                {
                    bFound = true;
                    break;
                }
            }
            if (!bFound)
            {
                return false;
            }
        }
    }
    if (!bMorphKeysExist1)
    {
        return false;
    }

    // Check that every "morph:" text key in this sequence has an exact match
    // in pkTimeSyncSequence.
    bool bMorphKeysExist2 = false;
    for (ui = 0; ui < uiNumKeys2; ui++)
    {
        const char* pcText2 = pkKeys2[ui].GetText();
        EE_ASSERT(pcText2);
        if (strncmp(pcText2, pcMorphTextKey, iMorphTextKeyLength) == 0)
        {
            bMorphKeysExist2 = true;
            bool bFound = false;
            for (unsigned int uj = 0; uj < uiNumKeys1; uj++)
            {
                const char* pcText1 = pkKeys1[uj].GetText();
                EE_ASSERT(pcText1);
                if (strcmp(pcText2, pcText1) == 0)
                {
                    bFound = true;
                    break;
                }
            }
            if (!bFound)
            {
                return false;
            }
        }
    }
    if (!bMorphKeysExist2)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
