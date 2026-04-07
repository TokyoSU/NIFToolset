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

#include "NiConstFloatEvaluator.h"
#include "NiEvaluatorSPData.h"
#include "NiScratchPad.h"

NiImplementRTTI(NiConstFloatEvaluator, NiEvaluator);

//--------------------------------------------------------------------------------------------------
NiConstFloatEvaluator::NiConstFloatEvaluator() :
    m_fFloatValue(INVALID_FLOAT)
{
    // No need to adjust the eval channel types.
}

//--------------------------------------------------------------------------------------------------
NiConstFloatEvaluator::NiConstFloatEvaluator(float fPoseValue) :
    m_fFloatValue(fPoseValue)
{
    SetEvalChannelTypes();
}

//--------------------------------------------------------------------------------------------------
bool NiConstFloatEvaluator::GetChannelPosedValue(unsigned int uiChannel,
    void* pvResult) const
{
    EE_ASSERT(uiChannel == FLOAT);
    EE_ASSERT(pvResult);

    if (IsRawEvalChannelPosed(uiChannel))
    {
        EE_ASSERT(!IsEvalChannelInvalid(uiChannel));
        *(float*)pvResult = m_fFloatValue;
        return true;
    }

    EE_ASSERT(IsEvalChannelInvalid(uiChannel));
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiConstFloatEvaluator::UpdateChannel(float, unsigned int uiChannel,
    NiEvaluatorSPData* pkEvalSPData, void* pvResult) const
{
    EE_ASSERT(uiChannel == FLOAT);
    EE_ASSERT(pkEvalSPData);
    EE_ASSERT(pkEvalSPData->GetEvaluator() == this);
    EE_ASSERT((unsigned int)pkEvalSPData->GetEvalChannelIndex() == uiChannel);
    EE_ASSERT(pvResult);

    if (IsRawEvalChannelPosed(uiChannel))
    {
        EE_ASSERT(!IsEvalChannelInvalid(uiChannel));
        NiScratchPad::CubicFloatSegmentData* pkCubicSeg =
            (NiScratchPad::CubicFloatSegmentData*)
            pkEvalSPData->GetSPSegmentData();
        EE_ASSERT(pkCubicSeg);
        *(float*)pvResult = pkCubicSeg->m_fValue0;
        EE_ASSERT(pkCubicSeg->m_fOutTangent0 == 0.0f);
        EE_ASSERT(pkCubicSeg->m_fA0 == 0.0f);
        EE_ASSERT(pkCubicSeg->m_fB0 == 0.0f);
        return true;
    }

    EE_ASSERT(IsEvalChannelInvalid(uiChannel));
    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiConstFloatEvaluator::GetChannelScratchPadInfo(unsigned int uiChannel,
    bool bForceAlwaysUpdate, NiAVObjectPalette*,
    unsigned int& uiFillSize, bool& bSharedFillData,
    NiScratchPadBlock& eSPBSegmentData,
    NiBSplineBasisData*& pkBasisData) const
{
    EE_ASSERT(uiChannel == FLOAT);

    if (IsEvalChannelInvalid(uiChannel) ||
        (!bForceAlwaysUpdate && !AlwaysUpdate()))
    {
        // Channel is invalid or constant: scratch pad is not required.
        return false;
    }

    // Indicate the scratch pad info that is required.
    uiFillSize = 0;
    bSharedFillData = false;
    eSPBSegmentData = SPBCUBICFLOATSEGMENT;
    pkBasisData = 0;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiConstFloatEvaluator::InitChannelScratchPadData(
    unsigned int uiChannel,
    NiEvaluatorSPData* pkEvalSPData, NiBSplineBasisData*,
    bool, NiAVObjectPalette*,
    NiPoseBufferHandle kPBHandle) const
{
    EE_UNUSED_ARG(uiChannel);
    EE_ASSERT(uiChannel == FLOAT);
    EE_ASSERT(pkEvalSPData);
    EE_ASSERT(pkEvalSPData->GetEvaluator() == this);
    EE_ASSERT((unsigned int)pkEvalSPData->GetEvalChannelIndex() == uiChannel);
    EE_ASSERT(!IsEvalChannelInvalid(uiChannel));

    // Mark the segment data as valid for all sequence times.
    pkEvalSPData->SetSPSegmentTimeRange(0.0f, NI_INFINITY);
    EE_ASSERT(pkEvalSPData->GetSPFillFunc() == NULL);

    // Initialize the scratch pad segment data.
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
    pkCubicSeg->m_fStartTime = 0.0f;
    pkCubicSeg->m_fInvDeltaTime = 0.0f;
    pkCubicSeg->m_fValue0 = m_fFloatValue;
    pkCubicSeg->m_fOutTangent0 = 0.0f;
    pkCubicSeg->m_fA0 = 0.0f;
    pkCubicSeg->m_fB0 = 0.0f;

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiConstFloatEvaluator::SetEvalChannelTypes()
{
    if (m_fFloatValue != INVALID_FLOAT)
    {
        m_aiEvalChannelTypes[FLOAT] = EVALFLOATCHANNEL | EVALPOSEDFLAG;
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
NiImplementCreateClone(NiConstFloatEvaluator);

//--------------------------------------------------------------------------------------------------
void NiConstFloatEvaluator::CopyMembers(NiConstFloatEvaluator* pkDest,
    NiCloningProcess& kCloning)
{
    NiEvaluator::CopyMembers(pkDest, kCloning);

    pkDest->m_fFloatValue = m_fFloatValue;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiConstFloatEvaluator);

//--------------------------------------------------------------------------------------------------
void NiConstFloatEvaluator::LoadBinary(NiStream& kStream)
{
    NiEvaluator::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_fFloatValue);
}

//--------------------------------------------------------------------------------------------------
void NiConstFloatEvaluator::LinkObject(NiStream& kStream)
{
    NiEvaluator::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiConstFloatEvaluator::RegisterStreamables(NiStream& kStream)
{
    if (!NiEvaluator::RegisterStreamables(kStream))
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiConstFloatEvaluator::SaveBinary(NiStream& kStream)
{
    NiEvaluator::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_fFloatValue);
}

//--------------------------------------------------------------------------------------------------
bool NiConstFloatEvaluator::IsEqual(NiObject* pkObject)
{
    if (!NiEvaluator::IsEqual(pkObject))
    {
        return false;
    }

    NiConstFloatEvaluator* pkDest = (NiConstFloatEvaluator*) pkObject;

    if (m_fFloatValue != pkDest->m_fFloatValue)
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiConstFloatEvaluator::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    NiEvaluator::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(
        NiConstFloatEvaluator::ms_RTTI.GetName()));

    pkStrings->Add(NiGetViewerString("m_fFloatValue", m_fFloatValue));
}

//--------------------------------------------------------------------------------------------------
