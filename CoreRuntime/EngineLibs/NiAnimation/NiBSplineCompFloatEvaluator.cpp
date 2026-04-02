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

#include "NiBSplineCompFloatEvaluator.h"
#include <NiCompUtility.h>
#include "NiEvaluatorSPData.h"
#include "NiScratchPad.h"

NiImplementRTTI(NiBSplineCompFloatEvaluator,
    NiBSplineFloatEvaluator);

//--------------------------------------------------------------------------------------------------
NiBSplineCompFloatEvaluator::NiBSplineCompFloatEvaluator() :
    NiBSplineFloatEvaluator()
{
    for (unsigned int ui = 0; ui < NUM_SCALARS; ui++)
    {
        m_afCompScalars[ui] = NI_INFINITY;
    }
    // No need to adjust the eval channel types.
}

//--------------------------------------------------------------------------------------------------
NiBSplineCompFloatEvaluator::NiBSplineCompFloatEvaluator(
    NiBSplineData* pkData, NiBSplineData::Handle kFloatCPHandle,
    NiBSplineBasisData* pkBasisData) :
    NiBSplineFloatEvaluator(pkData, kFloatCPHandle, pkBasisData, true)
{
    for (unsigned int ui = 0; ui < NUM_SCALARS; ui++)
    {
        m_afCompScalars[ui] = NI_INFINITY;
    }
    SetEvalChannelTypes();
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineCompFloatEvaluator::UsesCompressedControlPoints() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
float NiBSplineCompFloatEvaluator::GetOffset(unsigned short usChannel) const
{
    switch (usChannel)
    {
        case FLOAT:
            return m_afCompScalars[FLOAT_OFFSET];
    }
    return NI_INFINITY;
}

//--------------------------------------------------------------------------------------------------
void NiBSplineCompFloatEvaluator::SetOffset(float fOffset,
    unsigned short usChannel)
{
    switch (usChannel)
    {
        case FLOAT:
            m_afCompScalars[FLOAT_OFFSET] = fOffset;
            break;
    }
}

//--------------------------------------------------------------------------------------------------
float NiBSplineCompFloatEvaluator::GetHalfRange(
    unsigned short usChannel) const
{
    switch (usChannel)
    {
        case FLOAT:
            return m_afCompScalars[FLOAT_RANGE];
    }
    return NI_INFINITY;
}

//--------------------------------------------------------------------------------------------------
void NiBSplineCompFloatEvaluator::SetHalfRange(float fHalfRange,
    unsigned short usChannel)
{
    switch (usChannel)
    {
        case FLOAT:
            m_afCompScalars[FLOAT_RANGE] = fHalfRange;
            break;
    }
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineCompFloatEvaluator::GetChannelPosedValue(unsigned int uiChannel,
    void* pvResult) const
{
    EE_ASSERT(uiChannel == FLOAT);
    EE_ASSERT(pvResult);

    if (IsRawEvalChannelPosed(uiChannel))
    {
        EE_ASSERT(!IsEvalChannelInvalid(uiChannel));
        EE_ASSERT(GetControlPointCount(FLOAT) > 0);
        EE_ASSERT(m_spData);
        const short* psValue0 = m_spData->GetCompactControlPoint(
            m_kFloatCPHandle, 0, 1);

        NiCompUtility::DecompressFloatArray(psValue0, 1,
            m_afCompScalars[FLOAT_OFFSET], m_afCompScalars[FLOAT_RANGE],
            (float*)pvResult, 1);

        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineCompFloatEvaluator::GetChannelScratchPadInfo(
    unsigned int uiChannel, bool bForceAlwaysUpdate,
    NiAVObjectPalette* pkPalette, unsigned int& uiFillSize,
    bool& bSharedFillData, NiScratchPadBlock& eSPBSegmentData,
    NiBSplineBasisData*& pkBasisData) const
{
    EE_ASSERT(uiChannel == FLOAT);

    // Initialize using the base class, then fix up a few entries.
    if (!NiBSplineFloatEvaluator::GetChannelScratchPadInfo(uiChannel,
        bForceAlwaysUpdate, pkPalette, uiFillSize, bSharedFillData,
        eSPBSegmentData, pkBasisData))
    {
        return false;
    }

    // Fix up the fill size.
    if (uiFillSize > 0)
    {
        uiFillSize = sizeof(NiScratchPad::BSplineCompFloatFillData);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineCompFloatEvaluator::InitChannelScratchPadData(
    unsigned int uiChannel, NiEvaluatorSPData* pkEvalSPData,
    NiBSplineBasisData* pkSPBasisData, bool bInitSharedData,
    NiAVObjectPalette* pkPalette, NiPoseBufferHandle kPBHandle) const
{
    EE_ASSERT(uiChannel == FLOAT);
    EE_ASSERT(pkEvalSPData);
    EE_ASSERT(pkEvalSPData->GetEvaluator() == this);
    EE_ASSERT((unsigned int)pkEvalSPData->GetEvalChannelIndex() == uiChannel);
    EE_ASSERT(!IsEvalChannelInvalid(uiChannel));
    EE_ASSERT(m_kFloatCPHandle != NiBSplineData::INVALID_HANDLE);
    EE_ASSERT(m_spData);
    EE_ASSERT(m_spBasisData);

    // Initialize using the base class, then fix up a few entries.
    if (!NiBSplineFloatEvaluator::InitChannelScratchPadData(uiChannel,
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
    // compact float control points.
    pkEvalSPData->SetSPFillFunc(&BSplineCompFloatFillFunction);

    // Initialize the scratch pad fill data members in the derived class.
    NiScratchPad::BSplineCompFloatFillData* pkFillData =
        (NiScratchPad::BSplineCompFloatFillData*)
        pkEvalSPData->GetSPFillData();
    EE_ASSERT(pkFillData);
    EE_ASSERT((void*)&pkFillData->m_kBaseData == (void*)pkFillData);
    pkFillData->m_fOffset = m_afCompScalars[FLOAT_OFFSET];
    pkFillData->m_fHalfRange = m_afCompScalars[FLOAT_RANGE];

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineCompFloatEvaluator::BSplineCompFloatFillFunction(float fTime,
    NiEvaluatorSPData* pkEvalSPData)
{
    EE_ASSERT(pkEvalSPData);

    // Get the scratch pad fill and segment data.
    NiScratchPad::BSplineCompFloatFillData* pkFillData =
        (NiScratchPad::BSplineCompFloatFillData*)
        pkEvalSPData->GetSPFillData();
    EE_ASSERT(pkFillData);
    NiScratchPad::BSplineFloatFillData* pkBaseFillData =
        &pkFillData->m_kBaseData;

    NiScratchPad::BSplineFloatSegmentData* pkBSplineSeg =
        (NiScratchPad::BSplineFloatSegmentData*)
        pkEvalSPData->GetSPSegmentData();
    EE_ASSERT(pkBSplineSeg);

    // Update the basis to the specified time.
    int iMin, iMax;
    NiBSplineBasis<float, 3>& kBasis =
        pkBSplineSeg->m_pkSPBasisData->GetDegree3Basis();
    float fNormTime = (fTime - pkBaseFillData->m_fStartTime) *
        pkBaseFillData->m_fInvDeltaTime;
    kBasis.Compute(fNormTime, iMin, iMax);

    // Check if we need to load new control points.
    if (iMin != pkBaseFillData->m_iLastMin)
    {
        const short* psSource =
            pkBaseFillData->m_pkData->GetCompactControlPoint(
            pkBaseFillData->m_kCPHandle, iMin, 1);

        NiCompUtility::DecompressFloatArray(psSource, 4,
            pkFillData->m_fOffset, pkFillData->m_fHalfRange,
            pkBSplineSeg->m_afSourceArray, 4);

        pkBaseFillData->m_iLastMin = iMin;
    }

    // There's no need to fill the scratch pad at an identical time.
    pkEvalSPData->SetSPSegmentTimeRange(fTime, fTime);

    return true;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiBSplineCompFloatEvaluator);

//--------------------------------------------------------------------------------------------------
void NiBSplineCompFloatEvaluator::CopyMembers(
    NiBSplineCompFloatEvaluator* pkDest, NiCloningProcess& kCloning)
{
    NiBSplineFloatEvaluator::CopyMembers(pkDest, kCloning);

    for (unsigned int ui = 0; ui < NUM_SCALARS; ui++)
    {
        pkDest->m_afCompScalars[ui] = m_afCompScalars[ui];
    }
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiBSplineCompFloatEvaluator);

//--------------------------------------------------------------------------------------------------
void NiBSplineCompFloatEvaluator::LoadBinary(NiStream& kStream)
{
    NiBSplineFloatEvaluator::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_afCompScalars, NUM_SCALARS);
}

//--------------------------------------------------------------------------------------------------
void NiBSplineCompFloatEvaluator::LinkObject(NiStream& kStream)
{
    NiBSplineFloatEvaluator::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineCompFloatEvaluator::RegisterStreamables(NiStream& kStream)
{
    if (!NiBSplineFloatEvaluator::RegisterStreamables(kStream))
    {
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiBSplineCompFloatEvaluator::SaveBinary(NiStream& kStream)
{
    NiBSplineFloatEvaluator::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_afCompScalars, NUM_SCALARS);
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineCompFloatEvaluator::IsEqual(NiObject* pkObject)
{
    if (!NiBSplineFloatEvaluator::IsEqual(pkObject))
    {
        return false;
    }

    NiBSplineCompFloatEvaluator* pkDest =
        (NiBSplineCompFloatEvaluator*) pkObject;

    for (unsigned int ui = 0; ui < NUM_SCALARS; ui++)
    {
        if (m_afCompScalars[ui] != pkDest->m_afCompScalars[ui])
            return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiBSplineCompFloatEvaluator::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiBSplineFloatEvaluator::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(
        NiBSplineCompFloatEvaluator::ms_RTTI.GetName()));
}

//--------------------------------------------------------------------------------------------------
