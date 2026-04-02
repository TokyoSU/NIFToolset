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

#include "NiPoseBlender.h"
#include "NiControllerSequence.h"
#include "NiControllerManager.h"

NiImplementRTTI(NiPoseBlender,NiObject);

//--------------------------------------------------------------------------------------------------
NiPoseBlender::NiPoseBlender(NiControllerManager* pkOwner) :
    m_pkOwner(NULL), m_spFinalPoseBuffer(NULL), m_uiNumLODs(0),
    m_psLODs(NULL), m_uiActivePoseSize(0), m_pkActivePoseList(NULL),
    m_uiNumBitPatterns(0), m_uiFlagWeightSize(0), m_puiFlagWeightArray(NULL)
{
    Init(pkOwner);
}

//--------------------------------------------------------------------------------------------------
NiPoseBlender::~NiPoseBlender()
{
    Shutdown();
}

//--------------------------------------------------------------------------------------------------
bool NiPoseBlender::Update(unsigned int uiNumContributingSequences, int iLOD,
    NiPoseBuffer*& pkFinalPoseBuffer,
    NiControllerSequence*& pkSoleSequence)
{
    // This function computes the prioritized weighted blend of multiple
    // active sequences playing on the owning controller manager.
    //
    // uiNumContributingSequences specifies the number of sequences in the
    // owner's NiControllerSequence list that should be processed. This may be
    // less than the size of the list if the sequences at the tail of the list
    // make no contribution (due to priority, weight, and animation state) to
    // the final result.
    //
    // iLOD specifies the current level of detail for the controller manager.
    // As an optimization, the pose blender does not blend data items which
    // are hidden due to the specified level of detail.
    //
    // The pose buffer containing the final blended result is returned via
    // pkFinalPoseBuffer. If a single sequence contributes to the final result,
    // the contributing sequence will be returned via pkSoleSequence. In this
    // case, pkFinalPoseBuffer will point to the pose buffer associated with
    // pkSoleSequence as opposed to m_spFinalPoseBuffer, the pose buffer
    // maintained by the pose blender.
    //
    // This function returns true if at least one sequence contributes to the
    // final result. false is returned when all of the sequences are hidden
    // (say, due to invalid data in all pose buffers).

    EE_ASSERT(uiNumContributingSequences > 1);
    EE_ASSERT(uiNumContributingSequences <= m_pkOwner->GetSequenceCount());
    EE_ASSERT(m_pkOwner);
    EE_ASSERT(m_pkOwner->GetPoseBinding());

    // Limit the number of contributing sequences to simplify the algorithm.
    // It's unlikely that 30+ sequences contribute to the final animation.
    // Extra sequences at the tail of the list (i.e. the least significant)
    // will be ignored.
    if (uiNumContributingSequences > MAXBLENDEDSEQUENCES)
    {
        NILOG("NiPoseBlender::Update attempted to blend %d sequences. "
            "Only the first %d will be blended.\n",
            uiNumContributingSequences, MAXBLENDEDSEQUENCES);
        uiNumContributingSequences = MAXBLENDEDSEQUENCES;
    }

    // Ensure the flag weight array is large enough.
    unsigned int uiRequiredSize = uiNumContributingSequences * MAXBITPATTERNS;
    if (uiRequiredSize > m_uiFlagWeightSize)
    {
        NiFree(m_puiFlagWeightArray);
        m_puiFlagWeightArray = (unsigned int*)NiMalloc(
            uiRequiredSize * sizeof(unsigned int));
        m_uiFlagWeightSize = uiRequiredSize;
    }

    // Ensure LOD list is the correct size.
    NiPoseBinding* pkPoseBinding = m_pkOwner->GetPoseBinding();
    unsigned int uiNumTotalBindings = pkPoseBinding->GetNumTotalBindings();
    if (uiNumTotalBindings > m_uiNumLODs)
    {
        AddNewLODsFromPoseBinding();
    }
    EE_ASSERT(m_uiNumLODs == uiNumTotalBindings);

    // Ensure the active pose list is large enough.
    if (uiNumContributingSequences > m_uiActivePoseSize)
    {
        NiFree(m_pkActivePoseList);
        m_pkActivePoseList = (ActivePose*)NiMalloc(
            uiNumContributingSequences * sizeof(ActivePose));
        m_uiActivePoseSize = uiNumContributingSequences;
    }

    // Initialize the active pose list.
    for (unsigned int ui = 0; ui < uiNumContributingSequences; ui++)
    {
        NiControllerSequence* pkSequence = m_pkOwner->GetSequenceAt(ui);
        EE_ASSERT(pkSequence);
        NiPoseBuffer* pkPoseBuffer = pkSequence->GetPoseBuffer();
        EE_ASSERT(pkPoseBuffer);
        EE_ASSERT(pkPoseBuffer->GetPoseBinding());
        EE_ASSERT(pkPoseBuffer->GetPoseBinding() == pkPoseBinding);
        EE_ASSERT(pkPoseBuffer->GetNumTotalItems() ==
            pkPoseBuffer->GetPoseBinding()->GetNumTotalBindings());

        ActivePose& kActivePose = m_pkActivePoseList[ui];
        kActivePose.m_pkPoseBuffer = pkPoseBuffer;
        kActivePose.m_puiFlagWeight = pkPoseBuffer->GetFlagWeightArray();
        kActivePose.m_bHasContributingItems = false;
        kActivePose.m_bIsAdditive = pkSequence->IsAdditiveBlend();
        kActivePose.m_iPriority = pkSequence->GetPriority();
        kActivePose.m_fWeight = pkSequence->GetSpinnerScaledWeight();
        kActivePose.m_pkSequence = pkSequence;
    }

    // Determine the finalized weight for each entry in the
    // contributing pose buffers.
    m_uiNumBitPatterns = 0;
    short sLOD = (short)iLOD;
    short* psLOD = m_psLODs;
    for (unsigned int ui = 0; ui < m_uiNumLODs; ui++, psLOD++)
    {
        if (sLOD <= *psLOD)
        {
            // Data entry is visible at the specified LOD.

            // Create bit pattern of valid items.
            unsigned int uiBitPattern = 0;
            unsigned int uiCurBit = 1 << (uiNumContributingSequences - 1);
            for (unsigned int uj = 0; uj < uiNumContributingSequences; uj++)
            {
                if (NiPoseBuffer::IsItemValid(
                    *m_pkActivePoseList[uj].m_puiFlagWeight))
                {
                    uiBitPattern |= uiCurBit;
                }
                uiCurBit >>= 1;
            }

            // Check if this matches a known pattern.
            unsigned int uk = 0;
            for (; uk < m_uiNumBitPatterns; uk++)
            {
                if ((uiBitPattern & m_auiBitMaskList[uk]) ==
                    m_auiBitPatternList[uk])
                {
                    // Match found. Copy associated finalized weights.
                    unsigned int* puiFlagWeight = m_puiFlagWeightArray +
                        uk * uiNumContributingSequences;
                    for (unsigned int uj = 0; uj < uiNumContributingSequences;
                        uj++)
                    {
                        if (NiPoseBuffer::IsItemValid(
                            *m_pkActivePoseList[uj].m_puiFlagWeight))
                        {
                            *m_pkActivePoseList[uj].m_puiFlagWeight =
                                *puiFlagWeight;
                        }
                        puiFlagWeight++;
                    }
                    break;
                }
            }

            // Compute the finalized weights, if needed.
            if (uk >= m_uiNumBitPatterns)
            {
                int iHighPriority = -INT_MAX;
                int iSecondPriority = -INT_MAX;
                int iHighCount = 0;
                int iSecondCount = 0;
                float fTotalHighWeight = 0.0f;
                float fTotalSecondWeight = 0.0f;
                float fMaxHighEaseSpinner = 0.0f;
                unsigned int uiNonCulledValidItems = 0;
                unsigned int uiCurrentBit =
                    1 << (uiNumContributingSequences - 1);
                for (unsigned int uj = 0; uj < uiNumContributingSequences;
                    uj++)
                {
                    ActivePose& kActivePose = m_pkActivePoseList[uj];
                    if ((uiBitPattern & uiCurrentBit) == 0)
                    {
                        // Item is invalid: no need to cull it.
                        EE_ASSERT(!NiPoseBuffer::IsItemValid(
                            *kActivePose.m_puiFlagWeight));
                        EE_ASSERT(*kActivePose.m_puiFlagWeight == 0);
                        uiCurrentBit >>= 1;
                        continue;
                    }

                    // Clear any cull flag from the last update.
                    // Overwrite flags and finalized weight.
                    *kActivePose.m_puiFlagWeight =
                        (unsigned int)NiPoseBuffer::VALIDITEMFLAG;
                    uiNonCulledValidItems++;

                    // This code relies upon the sorted order of sequences
                    // from most to least importance as defined by
                    // NiControllerSequence::IsMoreImportantThan().

                    if (kActivePose.m_bIsAdditive)
                    {
                        // Make sure that additive blends are sorted first.
                        // If this is not the case, iHighPriority will have been set below
                        // by some other sequence.
                        EE_ASSERT(iHighPriority == -INT_MAX);

                        EE_ASSERT(kActivePose.m_pkSequence->IsAdditiveBlend());
                    }
                    else if (iHighPriority == -INT_MAX)
                    {
                        iHighPriority = kActivePose.m_iPriority;
                        fMaxHighEaseSpinner =
                            kActivePose.m_pkSequence->GetEaseSpinner();
                        EE_ASSERT(fMaxHighEaseSpinner >= 0.0f);
                        EE_ASSERT(fMaxHighEaseSpinner <= 1.0f);
                        fTotalHighWeight = kActivePose.m_fWeight;
                        iHighCount = 1;
                    }
                    else if (kActivePose.m_iPriority == iHighPriority)
                    {
                        fTotalHighWeight += kActivePose.m_fWeight;
                        iHighCount++;
                    }
                    else if (fMaxHighEaseSpinner < 1.0f &&
                        kActivePose.m_iPriority >= iSecondPriority)
                    {
                        EE_ASSERT(kActivePose.m_iPriority < iHighPriority);
                        iSecondPriority = kActivePose.m_iPriority;
                        fTotalSecondWeight += kActivePose.m_fWeight;
                        iSecondCount++;
                    }
                    else
                    {
                        // No other sequences contribute to the result.
                        EE_ASSERT(NiPoseBuffer::IsItemValid(
                            *kActivePose.m_puiFlagWeight));
                        *kActivePose.m_puiFlagWeight = (unsigned int)
                            (NiPoseBuffer::VALIDITEMFLAG |
                            NiPoseBuffer::CULLEDVALIDITEMFLAG);
                        uj++;

                        // Counted one too many.
                        uiNonCulledValidItems--;

                        // Cull remaining valid items.
                        while (uj < uiNumContributingSequences)
                        {
                            if (NiPoseBuffer::IsItemValid(
                                *m_pkActivePoseList[uj].m_puiFlagWeight))
                            {
                                *m_pkActivePoseList[uj].m_puiFlagWeight =
                                    (unsigned int)
                                    (NiPoseBuffer::VALIDITEMFLAG |
                                    NiPoseBuffer::CULLEDVALIDITEMFLAG);
                            }
                            else
                            {
                                EE_ASSERT(*m_pkActivePoseList[uj].m_puiFlagWeight == 0);
                            }
                            uj++;
                        }
                        break;
                    }

                    uiCurrentBit >>= 1;
                }

                // Compute normalization scaler for second priority sequences.
                float fSecondNormalizer = 0.0f;
                if (fTotalSecondWeight > 0.0f)
                {
                    fSecondNormalizer =
                        (1.0f - fMaxHighEaseSpinner) / fTotalSecondWeight;
                }
                else
                {
                    if (iSecondCount == 0)
                    {
                        // No second priority sequences contribute to the
                        // result.
                        // Force the high priority ease spinner to full value.
                        fMaxHighEaseSpinner = 1.0f;
                    }
                    else
                    {
                        // Normalize the weight across all sequences at
                        // this priority.
                        fSecondNormalizer =
                            (1.0f - fMaxHighEaseSpinner) / (float)iSecondCount;
                    }
                }

                // Compute normalization scaler for high priority sequences.
                float fHighNormalizer = 0.0f;
                if (fTotalHighWeight > 0.0f)
                {
                    fHighNormalizer =
                        fMaxHighEaseSpinner / fTotalHighWeight;
                }
                else
                {
                    if (iHighCount > 0)
                    {
                        // Normalize the weight across all sequences at
                        // this priority.
                        fHighNormalizer =
                            fMaxHighEaseSpinner / (float)iHighCount;
                    }
                }

                // Keep tab of the remaining weight to avoid round-off errors.
                unsigned int uiRemainingWeight =
                    NiPoseBuffer::FINALIZEDWEIGHTSCALER;
                unsigned int uiActivePoseIndex = 0;
                while (uiNonCulledValidItems > 0)
                {
                    EE_ASSERT(uiActivePoseIndex < uiNumContributingSequences);
                    ActivePose& kActivePose =
                        m_pkActivePoseList[uiActivePoseIndex];
                    uiActivePoseIndex++;
                    if (NiPoseBuffer::IsItemValidAndNotCulled(
                        *kActivePose.m_puiFlagWeight))
                    {
                        unsigned int uiWeight = uiRemainingWeight;
                        uiNonCulledValidItems--;
                        if (uiNonCulledValidItems > 0)
                        {
                            float fWeight = kActivePose.m_fWeight;
                            if (!kActivePose.m_bIsAdditive)
                            {
                                if (kActivePose.m_iPriority == iHighPriority)
                                {
                                    if (fTotalHighWeight == 0.0f)
                                    {
                                        fWeight = fHighNormalizer;
                                    }
                                    else
                                    {
                                        fWeight *= fHighNormalizer;
                                    }
                                }
                                else
                                {
                                    if (fTotalSecondWeight == 0.0f)
                                    {
                                        fWeight = fSecondNormalizer;
                                    }
                                    else
                                    {
                                        fWeight *= fSecondNormalizer;
                                    }
                                }
                            }

                            EE_ASSERT(fWeight >= 0.0f && fWeight <= 1.0f);
                            uiWeight = (unsigned int)(fWeight *
                                (float)NiPoseBuffer::FINALIZEDWEIGHTSCALER);
                            if (!kActivePose.m_bIsAdditive && uiWeight > uiRemainingWeight)
                            {
                                EE_ASSERT(uiWeight - uiRemainingWeight < 4);
                                uiWeight = uiRemainingWeight;
                            }
                        }

                        if (uiWeight > 0)
                        {
                            // This item contributes to the final result.
                            kActivePose.m_bHasContributingItems = true;
                            *kActivePose.m_puiFlagWeight =
                                NiPoseBuffer::VALIDITEMFLAG | uiWeight;
                        }
                        else
                        {
                            // The weight is zero. Cull this item.
                            EE_ASSERT(NiPoseBuffer::IsItemValid(
                                *kActivePose.m_puiFlagWeight));
                            *kActivePose.m_puiFlagWeight = (unsigned int)
                                (NiPoseBuffer::VALIDITEMFLAG |
                                NiPoseBuffer::CULLEDVALIDITEMFLAG);
                        }

                        if (!kActivePose.m_bIsAdditive)
                        {
                            EE_ASSERT(uiRemainingWeight >= uiWeight);
                            uiRemainingWeight -= uiWeight;
                        }
                    }
                }
                EE_ASSERT(uiRemainingWeight == 0 ||
                    uiRemainingWeight == NiPoseBuffer::FINALIZEDWEIGHTSCALER);

                // Save the new pattern and associated weights.
                if (m_uiNumBitPatterns < MAXBITPATTERNS)
                {
                    unsigned int uiBitMask = 0;
                    if (uiCurrentBit > 0)
                    {
                        uiBitMask = (uiCurrentBit << 1) - 1;
                    }
                    uiBitMask = ~uiBitMask;
                    m_auiBitMaskList[m_uiNumBitPatterns] = uiBitMask;
                    m_auiBitPatternList[m_uiNumBitPatterns] =
                        uiBitPattern & uiBitMask;
                    unsigned int* puiFlagWeight = m_puiFlagWeightArray +
                        m_uiNumBitPatterns * uiNumContributingSequences;
                    uiCurrentBit = 1 << (uiNumContributingSequences - 1);
                    for (unsigned int uj = 0; uj < uiNumContributingSequences;
                        uj++)
                    {
                        if ((uiBitMask & uiCurrentBit) != 0)
                        {
                            // Save the weight of visible sequences.
                            *puiFlagWeight =
                                *m_pkActivePoseList[uj].m_puiFlagWeight;
                        }
                        else
                        {
                            // Treat hidden sequences as valid, but culled.
                            *puiFlagWeight = (unsigned int)
                                (NiPoseBuffer::VALIDITEMFLAG |
                                NiPoseBuffer::CULLEDVALIDITEMFLAG);
                        }
                        uiCurrentBit >>= 1;
                        puiFlagWeight++;
                    }
                    m_uiNumBitPatterns++;
                }
            }

            // Advance the flag weight pointers.
            for (unsigned int uj = 0; uj < uiNumContributingSequences; uj++)
            {
                m_pkActivePoseList[uj].m_puiFlagWeight++;
            }
        }
        else
        {
            // Data entry is hidden at the specified LOD.
            for (unsigned int uj = 0; uj < uiNumContributingSequences; uj++)
            {
                // Cull valid items.
                if (NiPoseBuffer::IsItemValid(
                    *m_pkActivePoseList[uj].m_puiFlagWeight))
                {
                    *m_pkActivePoseList[uj].m_puiFlagWeight = (unsigned int)
                        (NiPoseBuffer::VALIDITEMFLAG |
                        NiPoseBuffer::CULLEDVALIDITEMFLAG);
                }
                else
                {
                    EE_ASSERT(*m_pkActivePoseList[uj].m_puiFlagWeight == 0);
                }

                // Advance the flag weight pointer.
                m_pkActivePoseList[uj].m_puiFlagWeight++;
            }
        }
    }

    // Now that the finalized weights have been computed,
    // check if there's less than two contributing sequences.
    ActivePose* pkContributingActivePose = m_pkActivePoseList;
    ActivePose* pkActivePose = m_pkActivePoseList;
    ActivePose* pkEndActivePose = m_pkActivePoseList +
        uiNumContributingSequences;
    unsigned int uiNumAdditivePoses = 0;
    while (pkActivePose < pkEndActivePose)
    {
        if (pkActivePose->m_bHasContributingItems)
        {
            if (pkActivePose != pkContributingActivePose)
            {
                *pkContributingActivePose = *pkActivePose;
            }

            if (pkContributingActivePose->m_bIsAdditive)
                uiNumAdditivePoses++;

            pkContributingActivePose++;

        }
        pkActivePose++;
    }
    ActivePose* pkLastAdditivePose = m_pkActivePoseList + uiNumAdditivePoses;

    uiNumContributingSequences =
        (unsigned int)(pkContributingActivePose - m_pkActivePoseList);
    EE_ASSERT(uiNumContributingSequences >= uiNumAdditivePoses);
    if (uiNumContributingSequences - uiNumAdditivePoses == 0)
    {
        // No sequences contribute to the final result.
        // Thus, there's no pose buffer to return.
        pkSoleSequence = NULL;
        pkFinalPoseBuffer = NULL;
        return false;
    }

    if (uiNumContributingSequences == 1)
    {
        // Just one sequence contributes to the final result.
        // Return the sequence and associated pose buffer.
        EE_ASSERT(m_pkActivePoseList[0].m_bHasContributingItems);
        EE_ASSERT(!m_pkActivePoseList[0].m_bIsAdditive);
        pkSoleSequence = m_pkActivePoseList[0].m_pkSequence;
        pkFinalPoseBuffer = m_pkActivePoseList[0].m_pkPoseBuffer;
        return true;
    }

    // Multiple sequences contribute to the final result.
    // Compute the blended result in the final pose buffer.

    // Ensure the final pose buffer is the correct size.
    pkFinalPoseBuffer = m_spFinalPoseBuffer;
    EE_ASSERT(pkFinalPoseBuffer);
    EE_ASSERT(pkFinalPoseBuffer->GetPoseBinding() ==
        m_pkOwner->GetPoseBinding());
    if (!pkFinalPoseBuffer->AddNewItemsFromPoseBinding(false, false))
    {
        // Invalidate all items in the final pose buffer.
        pkFinalPoseBuffer->InvalidateAllItems();
    }

    // Reset the flag weight pointers to the start of their lists.
    pkActivePose = m_pkActivePoseList;
    pkEndActivePose = m_pkActivePoseList + uiNumContributingSequences;
    while (pkActivePose < pkEndActivePose)
    {
        pkActivePose->m_puiFlagWeight =
            pkActivePose->m_pkPoseBuffer->GetFlagWeightArray();
        pkActivePose++;
    }

    // Blend the color items.
    unsigned int uiNumColors = pkFinalPoseBuffer->GetNumColors();
    for (unsigned int ui = 0; ui < uiNumColors; ui++)
    {
        bool bValid = false;
        NiColorA kFinalColor(0.0f, 0.0f, 0.0f, 0.0f);
        NiPoseBufferHandle kPBHandle(PBCOLORCHANNEL, (unsigned short)ui);
        pkActivePose = m_pkActivePoseList;
        while (pkActivePose < pkLastAdditivePose)
        {
            NiColorA kAddColor;
            if (pkActivePose->m_pkPoseBuffer->GetColorIfNotCulled(
                kPBHandle, kAddColor))
            {
                NiColorA kBaseColor;
                NiPoseBuffer* pkRefFrame =
                    pkActivePose->m_pkSequence->GetAdditiveRefFrame();
                EE_ASSERT(pkRefFrame);
                EE_VERIFY(pkRefFrame->GetColor(kPBHandle, kBaseColor));

                // This does not make this entry valid.
                // There has to be a non-additive value.
                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);
                kFinalColor += (kAddColor - kBaseColor) * fWeight;
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }
        while (pkActivePose < pkEndActivePose)
        {
            NiColorA kColor;
            if (pkActivePose->m_pkPoseBuffer->GetColorIfNotCulled(
                kPBHandle, kColor))
            {
                bValid = true;
                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);
                kFinalColor += kColor * fWeight;
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }

        if (bValid)
        {
            pkFinalPoseBuffer->SetColorValid(kPBHandle, true);
            pkFinalPoseBuffer->SetColor(kPBHandle, kFinalColor);
        }
    }

    // Blend the bool items.
    unsigned int uiNumBools = pkFinalPoseBuffer->GetNumBools();
    for (unsigned int ui = 0; ui < uiNumBools; ui++)
    {
        bool bValid = false;
        float fFinalBool = 0.0f;
        NiPoseBufferHandle kPBHandle(PBBOOLCHANNEL, (unsigned short)ui);
        pkActivePose = m_pkActivePoseList;
        while (pkActivePose < pkLastAdditivePose)
        {
            float fAddBool;
            if (pkActivePose->m_pkPoseBuffer->GetBoolIfNotCulled(
                kPBHandle, fAddBool))
            {
                float fBaseBool;
                NiPoseBuffer* pkRefFrame =
                    pkActivePose->m_pkSequence->GetAdditiveRefFrame();
                EE_ASSERT(pkRefFrame);
                EE_VERIFY(pkRefFrame->GetBool(kPBHandle, fBaseBool));

                // This does not make this entry valid.
                // There has to be a non-additive value.
                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);
                fFinalBool += (fAddBool - fBaseBool) * fWeight;
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }
        while (pkActivePose < pkEndActivePose)
        {
            float fBool;
            if (pkActivePose->m_pkPoseBuffer->GetBoolIfNotCulled(
                kPBHandle, fBool))
            {
                bValid = true;
                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);
                fFinalBool += fBool * fWeight;
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }

        if (bValid)
        {
            pkFinalPoseBuffer->SetBoolValid(kPBHandle, true);
            pkFinalPoseBuffer->SetBool(kPBHandle, fFinalBool);
        }
    }

    // Blend the float items.
    unsigned int uiNumFloats = pkFinalPoseBuffer->GetNumFloats();
    for (unsigned int ui = 0; ui < uiNumFloats; ui++)
    {
        bool bValid = false;
        float fFinalFloat = 0.0f;
        NiPoseBufferHandle kPBHandle(PBFLOATCHANNEL, (unsigned short)ui);
        pkActivePose = m_pkActivePoseList;
        while (pkActivePose < pkLastAdditivePose)
        {
            float fAddFloat;
            if (pkActivePose->m_pkPoseBuffer->GetFloatIfNotCulled(
                kPBHandle, fAddFloat))
            {
                float fBaseFloat;
                NiPoseBuffer* pkRefFrame =
                    pkActivePose->m_pkSequence->GetAdditiveRefFrame();
                EE_ASSERT(pkRefFrame);
                EE_VERIFY(pkRefFrame->GetFloat(kPBHandle, fBaseFloat));

                // This does not make this entry valid.
                // There has to be a non-additive value.
                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);
                fFinalFloat += (fAddFloat - fBaseFloat) * fWeight;
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }
        while (pkActivePose < pkEndActivePose)
        {
            float fFloat;
            if (pkActivePose->m_pkPoseBuffer->GetFloatIfNotCulled(
                kPBHandle, fFloat))
            {
                bValid = true;
                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);
                fFinalFloat += fFloat * fWeight;
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }

        if (bValid)
        {
            pkFinalPoseBuffer->SetFloatValid(kPBHandle, true);
            pkFinalPoseBuffer->SetFloat(kPBHandle, fFinalFloat);
        }
    }

    // Blend the point3 items.
    unsigned int uiNumPoint3s = pkFinalPoseBuffer->GetNumPoint3s();
    for (unsigned int ui = 0; ui < uiNumPoint3s; ui++)
    {
        bool bValid = false;
        NiPoint3 kFinalPoint3(NiPoint3::ZERO);
        NiPoseBufferHandle kPBHandle(PBPOINT3CHANNEL, (unsigned short)ui);
        pkActivePose = m_pkActivePoseList;
        while (pkActivePose < pkLastAdditivePose)
        {
            NiPoint3 kAddPoint3;
            if (pkActivePose->m_pkPoseBuffer->GetPoint3IfNotCulled(
                kPBHandle, kAddPoint3))
            {
                NiPoint3 kBasePoint3;
                NiPoseBuffer* pkRefFrame =
                    pkActivePose->m_pkSequence->GetAdditiveRefFrame();
                EE_ASSERT(pkRefFrame);
                EE_VERIFY(pkRefFrame->GetPoint3(kPBHandle, kBasePoint3));

                // This does not make this entry valid.
                // There has to be a non-additive value.
                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);
                kFinalPoint3 += (kAddPoint3 - kBasePoint3) * fWeight;
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }
        while (pkActivePose < pkEndActivePose)
        {
            NiPoint3 kPoint3;
            if (pkActivePose->m_pkPoseBuffer->GetPoint3IfNotCulled(
                kPBHandle, kPoint3))
            {
                bValid = true;
                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);
                kFinalPoint3 += kPoint3 * fWeight;
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }

        if (bValid)
        {
            pkFinalPoseBuffer->SetPoint3Valid(kPBHandle, true);
            pkFinalPoseBuffer->SetPoint3(kPBHandle, kFinalPoint3);
        }
    }

    // Blend the rot items.
    //
    // Generally speaking, when you want to blend between quaternions,
    // you use slerp. There are three qualities that any rotation
    // interpolation needs to satisfy.
    // 1) Commutativity
    // 2) Constant Velocity
    // 3) Minimal Torque (ie shortest path on great sphere)
    // Slerp has 2 and 3. For blending like this, we really want 1.
    // Therefore, we will use normalized lerp. This algorithm has the
    // properties of 1 and 3.
    unsigned int uiNumRots = pkFinalPoseBuffer->GetNumRots();
    for (unsigned int ui = 0; ui < uiNumRots; ui++)
    {
        bool bValid = false;

        // Final rotation is calculated via normalized lerp.
        NiQuaternion kFinalRot(0.0f, 0.0f, 0.0f, 0.0f);
        NiQuaternion kFinalAdd(1.0f, 0.0f, 0.0f, 0.0f);
        NiPoseBufferHandle kPBHandle(PBROTCHANNEL, (unsigned short)ui);
        pkActivePose = m_pkActivePoseList;
        while (pkActivePose < pkLastAdditivePose)
        {
            NiQuaternion kAddRot;
            if (pkActivePose->m_pkPoseBuffer->GetRotIfNotCulled(
                kPBHandle, kAddRot))
            {
                NiQuaternion kBaseRot;
                NiPoseBuffer* pkRefFrame =
                    pkActivePose->m_pkSequence->GetAdditiveRefFrame();
                EE_ASSERT(pkRefFrame);
                EE_VERIFY(pkRefFrame->GetRot(kPBHandle, kBaseRot));

                // Reverse the direction of the base rotation.
                kBaseRot.m_fW *= -1;

                // Combine reverse base rotation with this frame's rotation.
                // This is the fully weighted rotation.
                NiQuaternion kCom = kBaseRot * kAddRot;

                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);

                float fBaseW = (kCom.m_fW < 0.0f ? -1.0f : 1.0f);

                // Nlerp with (1,0,0,0)
                NiQuaternion kWeighted(
                    kCom.m_fW * fWeight + (1.0f - fWeight) * fBaseW,
                    kCom.m_fX * fWeight,
                    kCom.m_fY * fWeight,
                    kCom.m_fZ * fWeight
                );
                kWeighted.Normalize();

                kFinalAdd = kFinalAdd * kWeighted;
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }
        while (pkActivePose < pkEndActivePose)
        {
            NiQuaternion kRot;
            if (pkActivePose->m_pkPoseBuffer->GetRotIfNotCulled(
                kPBHandle, kRot))
            {
                // Dot only represents the angle between quats
                // when they are unitized.
                float fCos = NiQuaternion::Dot(kFinalRot, kRot);

                // If the angle is negative, we need to
                // invert the quat to get the best path.
                if (fCos < 0.0f)
                {
                    kRot = -kRot;
                }

                bValid = true;

                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);

                // Accumulate the total weighted values.
                //
                // Multiply in the weights to the quaternions.
                // Note that this makes them non-rotations.
                kFinalRot.SetValues(
                    kRot.GetW() * fWeight + kFinalRot.GetW(),
                    kRot.GetX() * fWeight + kFinalRot.GetX(),
                    kRot.GetY() * fWeight + kFinalRot.GetY(),
                    kRot.GetZ() * fWeight + kFinalRot.GetZ());
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }

        if (bValid)
        {
            kFinalRot = kFinalRot * kFinalAdd;

            // Need to re-normalize the quaternion.
            kFinalRot.Normalize();

            pkFinalPoseBuffer->SetRotValid(kPBHandle, true);
            pkFinalPoseBuffer->SetRot(kPBHandle, kFinalRot);
        }
    }

    // Set the highest weighted referenced item for each entry.
    // Additive entries are considered normally for referenced items.
    unsigned int uiNumReferencedItems =
        pkFinalPoseBuffer->GetNumReferencedItems();
    for (unsigned int ui = 0; ui < uiNumReferencedItems; ui++)
    {
        float fMaxWeight = 0.0f;
        NiPoseBuffer::ReferencedItem kFinalReferencedItem;
        kFinalReferencedItem.m_pkControllerSequence = NULL;
        kFinalReferencedItem.m_pkReferencedEvaluator = NULL;
        kFinalReferencedItem.m_apkEvalSPData[0] = NULL;
        kFinalReferencedItem.m_apkEvalSPData[1] = NULL;
        kFinalReferencedItem.m_apkEvalSPData[2] = NULL;
        NiPoseBufferHandle kPBHandle(PBREFERENCEDCHANNEL, (unsigned short)ui);
        pkActivePose = m_pkActivePoseList;
        while (pkActivePose < pkEndActivePose)
        {
            NiPoseBuffer::ReferencedItem kReferencedItem;
            if (pkActivePose->m_pkPoseBuffer->GetReferencedItemIfNotCulled(
                kPBHandle, kReferencedItem))
            {
                float fWeight = NiPoseBuffer::GetItemFinalizedWeight(
                    *pkActivePose->m_puiFlagWeight);
                if (fWeight > fMaxWeight)
                {
                    fMaxWeight = fWeight;
                    kFinalReferencedItem = kReferencedItem;
                }
            }
            pkActivePose->m_puiFlagWeight++;
            pkActivePose++;
        }

        if (fMaxWeight > 0.0f)
        {
            pkFinalPoseBuffer->SetReferencedItemValid(kPBHandle, true);
            pkFinalPoseBuffer->SetReferencedItem(kPBHandle,
                kFinalReferencedItem);
        }
    }

    // More than one sequence contributes to the result.
    pkSoleSequence = NULL;
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiPoseBlender::Init(NiControllerManager* pkOwner)
{
    EE_ASSERT(pkOwner && !m_pkOwner);
    EE_ASSERT(pkOwner->GetPoseBinding());
    EE_ASSERT(!m_spFinalPoseBuffer);
    EE_ASSERT(!m_psLODs && m_uiNumLODs == 0);

    m_pkOwner = pkOwner;

    NiPoseBinding* pkPoseBinding = pkOwner->GetPoseBinding();
    m_spFinalPoseBuffer = NiNew NiPoseBuffer(pkPoseBinding);

    unsigned int uiNumTotalBindings = pkPoseBinding->GetNumTotalBindings();
    if (uiNumTotalBindings > 0)
    {
        AddNewLODsFromPoseBinding();
    }
}

//--------------------------------------------------------------------------------------------------
void NiPoseBlender::Shutdown()
{
    NiFree(m_pkActivePoseList);
    m_pkActivePoseList = NULL;
    m_uiActivePoseSize = 0;

    NiFree(m_psLODs);
    m_psLODs = NULL;
    m_uiNumLODs = 0;

    NiFree(m_puiFlagWeightArray);
    m_puiFlagWeightArray = NULL;
    m_uiFlagWeightSize = 0;
    m_uiNumBitPatterns = 0;

    m_spFinalPoseBuffer = NULL;
    m_pkOwner = NULL;
}

//--------------------------------------------------------------------------------------------------
void NiPoseBlender::AddNewLODsFromPoseBinding()
{
    EE_ASSERT(m_pkOwner);
    EE_ASSERT(m_pkOwner->GetPoseBinding());

    // Delete existing data.
    NiFree(m_psLODs);

    // Determine new size of array.
    NiPoseBinding* pkPoseBinding = m_pkOwner->GetPoseBinding();
    EE_ASSERT(m_uiNumLODs < pkPoseBinding->GetNumTotalBindings());
    m_uiNumLODs = pkPoseBinding->GetNumTotalBindings();

    // Allocate larger array.
    m_psLODs = (short*)NiMalloc(m_uiNumLODs * sizeof(short));

    // Fill in the new array.
    NiPoseBinding::BindInfo* pkBindInfos = pkPoseBinding->GetBindInfos();
    unsigned short* pusIndexList = pkPoseBinding->GetBindInfoIndexList();
    for (unsigned int ui = 0; ui < m_uiNumLODs; ui++)
    {
        unsigned short usIndex = pusIndexList[ui];
        EE_ASSERT(usIndex < pkPoseBinding->GetNumBindInfos());
        m_psLODs[ui] = pkBindInfos[usIndex].m_sLOD;
    }
}

//--------------------------------------------------------------------------------------------------
