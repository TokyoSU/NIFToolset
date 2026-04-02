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

#include "NiFloatEvaluator.h"
#include "NiLinFloatKey.h"
#include "NiEvaluatorSPData.h"
#include "NiScratchPad.h"

NiImplementRTTI(NiFloatEvaluator, NiKeyBasedEvaluator);

//--------------------------------------------------------------------------------------------------
NiFloatEvaluator::NiFloatEvaluator() :
    m_spFloatData(NULL)
{
    // No need to adjust the eval channel types.
}

//--------------------------------------------------------------------------------------------------
NiFloatEvaluator::NiFloatEvaluator(NiFloatData* pkFloatData) :
    m_spFloatData(pkFloatData)
{
    SetEvalChannelTypes();
}

//--------------------------------------------------------------------------------------------------
unsigned short NiFloatEvaluator::GetKeyChannelCount() const
{
    return 1;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiFloatEvaluator::GetKeyCount(unsigned short) const
{
    if (!m_spFloatData)
        return 0;
    else
        return m_spFloatData->GetNumKeys();
}

//--------------------------------------------------------------------------------------------------
NiAnimationKey::KeyType NiFloatEvaluator::GetKeyType(
    unsigned short) const
{
    if (!m_spFloatData)
        return NiAnimationKey::NOINTERP;

    unsigned int uiNumKeys;
    NiFloatKey::KeyType eType;
    unsigned char ucSize;
    m_spFloatData->GetAnim(uiNumKeys, eType, ucSize);
    return eType;
}

//--------------------------------------------------------------------------------------------------
NiAnimationKey::KeyContent NiFloatEvaluator::GetKeyContent(
    unsigned short) const
{
    return NiAnimationKey::FLOATKEY;
}

//--------------------------------------------------------------------------------------------------
NiAnimationKey* NiFloatEvaluator::GetKeyArray(unsigned short) const
{
    if (!m_spFloatData)
        return NULL;

    unsigned int uiNumKeys;
    NiFloatKey::KeyType eType;
    unsigned char ucSize;
    NiFloatKey* pkKeys = m_spFloatData->GetAnim(uiNumKeys, eType, ucSize);
    return pkKeys;
}

//--------------------------------------------------------------------------------------------------
unsigned char NiFloatEvaluator::GetKeyStride(unsigned short) const
{
    if (!m_spFloatData)
        return 0;

    unsigned int uiNumKeys;
    NiFloatKey::KeyType eType;
    unsigned char ucSize;
    m_spFloatData->GetAnim(uiNumKeys, eType, ucSize);
    return ucSize;
}

//--------------------------------------------------------------------------------------------------
void NiFloatEvaluator::Collapse()
{
    if (m_spFloatData)
    {
        unsigned int uiNumKeys;
        NiAnimationKey::KeyType eType;
        unsigned char ucSize;
        NiFloatKey* pkKeys = m_spFloatData->GetAnim(uiNumKeys, eType, ucSize);
        if (uiNumKeys == 0)
        {
            m_spFloatData = NULL;
        }
        else
        {
            NiAnimationKey::IsPosedFunction isposed =
                NiFloatKey::GetIsPosedFunction(eType);
            EE_ASSERT(isposed);
            if (isposed(pkKeys, uiNumKeys))
            {
                NiFloatKey* pkLinKeys =
                    (NiFloatKey*)NiNew NiLinFloatKey[1];
                pkLinKeys[0].SetTime(0.0f);
                float fValue = pkKeys->GetKeyAt(0, ucSize)->GetValue();
                pkLinKeys[0].SetValue(fValue);
                m_spFloatData->ReplaceAnim(pkLinKeys, 1,
                    NiAnimationKey::LINKEY);
            }
        }
    }
    SetEvalChannelTypes();
}

//--------------------------------------------------------------------------------------------------
bool NiFloatEvaluator::GetChannelPosedValue(unsigned int uiChannel,
    void* pvResult) const
{
    EE_ASSERT(uiChannel == FLOAT);
    EE_ASSERT(pvResult);

    if (IsRawEvalChannelPosed(uiChannel))
    {
        EE_ASSERT(!IsEvalChannelInvalid(uiChannel));
        EE_ASSERT(m_spFloatData);
        unsigned int uiNumKeys;
        NiAnimationKey::KeyType eType;
        unsigned char ucSize;
        NiFloatKey* pkKeys = m_spFloatData->GetAnim(uiNumKeys,
            eType, ucSize);
        EE_ASSERT(uiNumKeys == 1);
        *(float*)pvResult = pkKeys->GetKeyAt(0, ucSize)->GetValue();
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiFloatEvaluator::UpdateChannel(float fTime, unsigned int uiChannel,
    NiEvaluatorSPData* pkEvalSPData, void* pvResult) const
{
    EE_ASSERT(uiChannel == FLOAT);
    EE_ASSERT(pkEvalSPData);
    EE_ASSERT(pkEvalSPData->GetEvaluator() == this);
    EE_ASSERT((unsigned int)pkEvalSPData->GetEvalChannelIndex() == uiChannel);
    EE_ASSERT(pvResult);

    if (IsEvalChannelInvalid(uiChannel))
    {
        return false;
    }

    // Fill the segment data, if stale.
    if (!pkEvalSPData->IsSPSegmentDataValid(fTime))
    {
        NiScratchPadFillFunc pfnFillFunc = pkEvalSPData->GetSPFillFunc();
        if (!pfnFillFunc || !(*pfnFillFunc)(fTime, pkEvalSPData))
        {
            return false;
        }
    }

    // Compute the value based on the cubic segment data.
    NiScratchPad::CubicFloatSegmentData* pkCubicSeg =
        (NiScratchPad::CubicFloatSegmentData*)
        pkEvalSPData->GetSPSegmentData();
    EE_ASSERT(pkCubicSeg);
    float fT = (fTime - pkCubicSeg->m_fStartTime) *
        pkCubicSeg->m_fInvDeltaTime;
    *(float*)pvResult = pkCubicSeg->m_fValue0 + (pkCubicSeg->m_fOutTangent0 +
        (pkCubicSeg->m_fA0 + pkCubicSeg->m_fB0 * fT) * fT) * fT;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFloatEvaluator::GetChannelScratchPadInfo(unsigned int uiChannel,
    bool bForceAlwaysUpdate, NiAVObjectPalette*,
    unsigned int& uiFillSize, bool& bSharedFillData,
    NiScratchPadBlock& eSPBSegmentData,
    NiBSplineBasisData*& pkBasisData) const
{
    EE_ASSERT(uiChannel == FLOAT);

    bool bPosed = IsRawEvalChannelPosed(uiChannel);
    if (IsEvalChannelInvalid(uiChannel) ||
        (bPosed && !bForceAlwaysUpdate && !AlwaysUpdate()))
    {
        // Channel is invalid or constant: scratch pad is not required.
        return false;
    }

    // Indicate the scratch pad info that is required.
    uiFillSize = bPosed ? 0 : sizeof(NiScratchPad::CubicFloatFillData);
    bSharedFillData = false;
    eSPBSegmentData = SPBCUBICFLOATSEGMENT;
    pkBasisData = 0;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFloatEvaluator::InitChannelScratchPadData(unsigned int uiChannel,
    NiEvaluatorSPData* pkEvalSPData, NiBSplineBasisData*,
    bool, NiAVObjectPalette*,
    NiPoseBufferHandle kPBHandle) const
{
    EE_ASSERT(uiChannel == FLOAT);
    EE_ASSERT(pkEvalSPData);
    EE_ASSERT(pkEvalSPData->GetEvaluator() == this);
    EE_ASSERT((unsigned int)pkEvalSPData->GetEvalChannelIndex() == uiChannel);
    EE_ASSERT(!IsEvalChannelInvalid(uiChannel));
    EE_ASSERT(m_spFloatData);

    // Partially initialize the scratch pad segment data.
    NiScratchPad::CubicFloatSegmentData* pkCubicSeg =
        (NiScratchPad::CubicFloatSegmentData*)
        pkEvalSPData->GetSPSegmentData();
    EE_ASSERT(pkCubicSeg);
    pkCubicSeg->m_kHeader.m_sLOD = pkEvalSPData->GetLOD();
    if (IsReferencedEvaluator())
    {
        EE_ASSERT(kPBHandle.GetChannelType() == PBREFERENCEDCHANNEL);
        pkCubicSeg->m_kHeader.m_usOutputIndex =
            NiScratchPad::INVALIDOUTPUTINDEX;
    }
    else
    {
        EE_ASSERT(kPBHandle.GetChannelType() == PBFLOATCHANNEL);
        pkCubicSeg->m_kHeader.m_usOutputIndex =
            kPBHandle.GetChannelIndex();
    }
#ifdef NIDEBUG
    // The fill function should set these fields.
    pkCubicSeg->m_fStartTime = NI_INFINITY;
    pkCubicSeg->m_fInvDeltaTime = 0.0f;
    pkCubicSeg->m_fValue0 = 0.0f;
    pkCubicSeg->m_fOutTangent0 = 0.0f;
    pkCubicSeg->m_fA0 = 0.0f;
    pkCubicSeg->m_fB0 = 0.0f;
#endif

    unsigned int uiNumKeys;
    NiAnimationKey::KeyType eType;
    unsigned char ucSize;
    NiFloatKey* pkKeys = m_spFloatData->GetAnim(uiNumKeys, eType, ucSize);
    EE_ASSERT(pkKeys);
    EE_ASSERT(uiNumKeys > 0);

    bool bPosed = IsRawEvalChannelPosed(uiChannel);
    if (bPosed)
    {
        // Mark the segment data as valid for all sequence times.
        pkEvalSPData->SetSPSegmentTimeRange(0.0f, NI_INFINITY);
        EE_ASSERT(pkEvalSPData->GetSPFillFunc() == NULL);

        // Fill the remainder of the scratch pad segment data.
        pkCubicSeg->m_fStartTime = 0.0f;
        pkCubicSeg->m_fInvDeltaTime = 0.0f;
        pkCubicSeg->m_fValue0 = pkKeys->GetKeyAt(0, ucSize)->GetValue();
        pkCubicSeg->m_fOutTangent0 = 0.0f;
        pkCubicSeg->m_fA0 = 0.0f;
        pkCubicSeg->m_fB0 = 0.0f;
    }
    else
    {
        if (eType == NiAnimationKey::STEPKEY)
        {
            pkEvalSPData->SetStepFunction(true);
        }
        EE_ASSERT(pkEvalSPData->GetSPSegmentMinTime() == NI_INFINITY);
        EE_ASSERT(pkEvalSPData->GetSPSegmentMaxTime() == -NI_INFINITY);
        pkEvalSPData->SetSPFillFunc(&CubicFloatFillFunction);

        // Initialize the scratch pad fill data.
        NiScratchPad::CubicFloatFillData* pkFillData =
            (NiScratchPad::CubicFloatFillData*)pkEvalSPData->GetSPFillData();
        EE_ASSERT(pkFillData);
        pkFillData->m_uiLastIndex = 0;
        pkFillData->m_pkKeys = pkKeys;
        pkFillData->m_uiNumKeys = uiNumKeys;
        pkFillData->m_eType = eType;
        pkFillData->m_ucKeySize = ucSize;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiFloatEvaluator::CubicFloatFillFunction(float fTime,
    NiEvaluatorSPData* pkEvalSPData)
{
    EE_ASSERT(pkEvalSPData);

    // Get the scratch pad fill data.
    NiScratchPad::CubicFloatFillData* pkFillData =
        (NiScratchPad::CubicFloatFillData*)
        pkEvalSPData->GetSPFillData();
    EE_ASSERT(pkFillData);
    EE_ASSERT(pkFillData->m_pkKeys);
    EE_ASSERT(pkFillData->m_uiNumKeys > 0);

    // Fill the scratch pad with the cubic segment matching fTime.
    NiScratchPad::CubicFloatSegmentData* pkCubicSeg =
        (NiScratchPad::CubicFloatSegmentData*)
        pkEvalSPData->GetSPSegmentData();
    EE_ASSERT(pkCubicSeg);
    float fMinTime;
    float fMaxTime;
    NiFloatKey::GenCubicCoefs(fTime, pkFillData->m_pkKeys,
        pkFillData->m_eType, pkFillData->m_uiNumKeys,
        pkFillData->m_uiLastIndex, pkFillData->m_ucKeySize,
        fMinTime, fMaxTime, pkCubicSeg->m_fValue0,
        pkCubicSeg->m_fOutTangent0, pkCubicSeg->m_fA0,
        pkCubicSeg->m_fB0);
    pkCubicSeg->m_fStartTime = fMinTime;
    pkCubicSeg->m_fInvDeltaTime = (fMaxTime != fMinTime) ?
        1.0f / (fMaxTime - fMinTime) : 0.0f;

    // Update the time range to match the segment.
    pkEvalSPData->SetSPSegmentTimeRange(fMinTime, fMaxTime);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiFloatEvaluator::GuaranteeTimeRange(float fStartTime,
    float fEndTime)
{
    if (m_spFloatData)
    {
        m_spFloatData->GuaranteeKeysAtStartAndEnd(fStartTime,
            fEndTime);
    }
}

//--------------------------------------------------------------------------------------------------
NiEvaluator* NiFloatEvaluator::GetSequenceEvaluator(float fStartTime,
    float fEndTime)
{
    NiFloatEvaluator* pkSeqEval = (NiFloatEvaluator*)
        NiKeyBasedEvaluator::GetSequenceEvaluator(fStartTime, fEndTime);
    if (m_spFloatData)
    {
        NiFloatDataPtr spNewFloatData = m_spFloatData->GetSequenceData(
            fStartTime, fEndTime);
        pkSeqEval->SetFloatData(spNewFloatData);
    }

    return pkSeqEval;
}

//--------------------------------------------------------------------------------------------------
void NiFloatEvaluator::SetEvalChannelTypes()
{
    if (m_spFloatData && m_spFloatData->GetNumKeys() > 0)
    {
        m_aiEvalChannelTypes[FLOAT] = EVALFLOATCHANNEL;

        if (m_spFloatData->GetNumKeys() == 1)
        {
            m_aiEvalChannelTypes[FLOAT] |= EVALPOSEDFLAG;
        }
    }
    else
    {
        m_aiEvalChannelTypes[FLOAT] = EVALINVALIDCHANNEL;
    }
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiFloatEvaluator);

//--------------------------------------------------------------------------------------------------
void NiFloatEvaluator::CopyMembers(NiFloatEvaluator* pkDest,
    NiCloningProcess& kCloning)
{
    NiKeyBasedEvaluator::CopyMembers(pkDest, kCloning);

    pkDest->m_spFloatData = m_spFloatData;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiFloatEvaluator);

//--------------------------------------------------------------------------------------------------
void NiFloatEvaluator::LoadBinary(NiStream& kStream)
{
    NiKeyBasedEvaluator::LoadBinary(kStream);

    m_spFloatData = (NiFloatData*)kStream.ResolveLinkID();
}

//--------------------------------------------------------------------------------------------------
void NiFloatEvaluator::LinkObject(NiStream& kStream)
{
    NiKeyBasedEvaluator::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiFloatEvaluator::RegisterStreamables(NiStream& kStream)
{
    if (!NiKeyBasedEvaluator::RegisterStreamables(kStream))
    {
        return false;
    }

    if (m_spFloatData)
    {
        m_spFloatData->RegisterStreamables(kStream);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiFloatEvaluator::SaveBinary(NiStream& kStream)
{
    NiKeyBasedEvaluator::SaveBinary(kStream);

    kStream.SaveLinkID(m_spFloatData);
}

//--------------------------------------------------------------------------------------------------
bool NiFloatEvaluator::IsEqual(NiObject* pkObject)
{
    if (!NiKeyBasedEvaluator::IsEqual(pkObject))
    {
        return false;
    }

    NiFloatEvaluator* pkDest = (NiFloatEvaluator*) pkObject;

    if ((m_spFloatData && !pkDest->m_spFloatData) ||
        (!m_spFloatData && pkDest->m_spFloatData) ||
        (m_spFloatData && !m_spFloatData->IsEqual(pkDest->m_spFloatData)))
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiFloatEvaluator::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiKeyBasedEvaluator::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiFloatEvaluator::ms_RTTI.GetName()));

    pkStrings->Add(NiGetViewerString("m_spFloatData", m_spFloatData));
}

//--------------------------------------------------------------------------------------------------
