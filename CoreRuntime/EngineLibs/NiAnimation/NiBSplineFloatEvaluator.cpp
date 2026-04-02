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

#include "NiBSplineFloatEvaluator.h"
#include "NiEvaluatorSPData.h"
#include "NiScratchPad.h"

NiImplementRTTI(NiBSplineFloatEvaluator, NiBSplineEvaluator);

//--------------------------------------------------------------------------------------------------
NiBSplineFloatEvaluator::NiBSplineFloatEvaluator() :
    NiBSplineEvaluator(NULL, NULL),
    m_kFloatCPHandle(NiBSplineData::INVALID_HANDLE)
{
    // No need to adjust the eval channel types.
}

//--------------------------------------------------------------------------------------------------
NiBSplineFloatEvaluator::NiBSplineFloatEvaluator(
    NiBSplineData* pkData, NiBSplineData::Handle kFloatCPHandle,
    NiBSplineBasisData* pkBasisData) :
    NiBSplineEvaluator(pkData, pkBasisData), m_kFloatCPHandle(kFloatCPHandle)
{
    SetEvalChannelTypes();
}

//--------------------------------------------------------------------------------------------------
NiBSplineFloatEvaluator::NiBSplineFloatEvaluator(
    NiBSplineData* pkData, NiBSplineData::Handle kFloatCPHandle,
    NiBSplineBasisData* pkBasisData,
    bool bUseCompactCPs) :
    NiBSplineEvaluator(pkData, pkBasisData), m_kFloatCPHandle(kFloatCPHandle)
{
    EE_UNUSED_ARG(bUseCompactCPs);
    EE_ASSERT(bUseCompactCPs);
    // No need to adjust the eval channel types. The derived class will do it.
}

//--------------------------------------------------------------------------------------------------
unsigned short NiBSplineFloatEvaluator::GetChannelCount() const
{
    return 1;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiBSplineFloatEvaluator::GetDimension(
    unsigned short usChannel) const
{
    if (FLOAT == usChannel)
        return 1;
    else
        return 0;
}

//--------------------------------------------------------------------------------------------------
unsigned int NiBSplineFloatEvaluator::GetDegree(
    unsigned short usChannel) const
{
    if (FLOAT == usChannel)
        return 3;
    else
        return 0;
}

//--------------------------------------------------------------------------------------------------
NiBSplineData::Handle NiBSplineFloatEvaluator::GetControlHandle(
    unsigned short usChannel) const
{
    switch (usChannel)
    {
        case FLOAT:
            return m_kFloatCPHandle;
    }
    return NiBSplineData::INVALID_HANDLE;
}

//--------------------------------------------------------------------------------------------------
void NiBSplineFloatEvaluator::SetControlHandle(
    NiBSplineData::Handle kControlHandle, unsigned short usChannel)
{
    switch (usChannel)
    {
        case FLOAT:
            m_kFloatCPHandle = kControlHandle;
            break;
    }
    SetEvalChannelTypes();
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineFloatEvaluator::GetChannelPosedValue(unsigned int uiChannel,
    void* pvResult) const
{
    EE_ASSERT(uiChannel == FLOAT);
    EE_ASSERT(pvResult);

    if (IsRawEvalChannelPosed(uiChannel))
    {
        EE_ASSERT(!IsEvalChannelInvalid(uiChannel));
        EE_ASSERT(GetControlPointCount(FLOAT) > 0);
        EE_ASSERT(m_spData);
        const float* pfValue0 = m_spData->GetControlPoint(
            m_kFloatCPHandle, 0, 1);
        *(float*)pvResult = *pfValue0;
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineFloatEvaluator::UpdateChannel(float fTime, unsigned int uiChannel,
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
        NiScratchPadFillFunc pfnFillFunc =
            pkEvalSPData->GetSPFillFunc();
        if (!pfnFillFunc || !(*pfnFillFunc)(fTime, pkEvalSPData))
        {
            return false;
        }
    }

    if (IsRawEvalChannelPosed(uiChannel))
    {
        // Compute the value based on the cubic segment data.
        EE_ASSERT(!IsEvalChannelInvalid(uiChannel));
        NiScratchPad::CubicFloatSegmentData* pkCubicSeg =
            (NiScratchPad::CubicFloatSegmentData*)
            pkEvalSPData->GetSPSegmentData();
        EE_ASSERT(pkCubicSeg);
        *(float*)pvResult = pkCubicSeg->m_fValue0;
        EE_ASSERT(pkCubicSeg->m_fOutTangent0 == 0.0f);
        EE_ASSERT(pkCubicSeg->m_fA0 == 0.0f);
        EE_ASSERT(pkCubicSeg->m_fB0 == 0.0f);
    }
    else
    {
        // Compute the value based on the b-spline segment data.
        NiScratchPad::BSplineFloatSegmentData* pkBSplineSeg =
            (NiScratchPad::BSplineFloatSegmentData*)
            pkEvalSPData->GetSPSegmentData();
        EE_ASSERT(pkBSplineSeg);
        EE_ASSERT(pkBSplineSeg->m_pkSPBasisData);
        NiBSplineBasis<float, 3>& kBasis =
            pkBSplineSeg->m_pkSPBasisData->GetDegree3Basis();
        float fBasis0 = kBasis.GetValue(0);
        float fBasis1 = kBasis.GetValue(1);
        float fBasis2 = kBasis.GetValue(2);
        float fBasis3 = kBasis.GetValue(3);
        *(float*)pvResult = fBasis0 * pkBSplineSeg->m_afSourceArray[0] +
            fBasis1 * pkBSplineSeg->m_afSourceArray[1] +
            fBasis2 * pkBSplineSeg->m_afSourceArray[2] +
            fBasis3 * pkBSplineSeg->m_afSourceArray[3];
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineFloatEvaluator::GetChannelScratchPadInfo(unsigned int uiChannel,
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
    if (bPosed)
    {
        uiFillSize = 0;
        eSPBSegmentData = SPBCUBICFLOATSEGMENT;
        pkBasisData = NULL;
    }
    else
    {
        uiFillSize = sizeof(NiScratchPad::BSplineFloatFillData);
        eSPBSegmentData = SPBBSPLINEFLOATSEGMENT;
        pkBasisData = m_spBasisData;
    }
    bSharedFillData = false;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineFloatEvaluator::InitChannelScratchPadData(unsigned int uiChannel,
    NiEvaluatorSPData* pkEvalSPData, NiBSplineBasisData* pkSPBasisData,
    bool, NiAVObjectPalette*,
    NiPoseBufferHandle kPBHandle) const
{
    EE_ASSERT(uiChannel == FLOAT);
    EE_ASSERT(pkEvalSPData);
    EE_ASSERT(pkEvalSPData->GetEvaluator() == this);
    EE_ASSERT((unsigned int)pkEvalSPData->GetEvalChannelIndex() == uiChannel);
    EE_ASSERT(!IsEvalChannelInvalid(uiChannel));
    EE_ASSERT(m_kFloatCPHandle != NiBSplineData::INVALID_HANDLE);
    EE_ASSERT(m_spData);
    EE_ASSERT(m_spBasisData);

    if (IsRawEvalChannelPosed(uiChannel))
    {
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
        EE_VERIFY(GetChannelPosedValue(uiChannel, &pkCubicSeg->m_fValue0));
        pkCubicSeg->m_fOutTangent0 = 0.0f;
        pkCubicSeg->m_fA0 = 0.0f;
        pkCubicSeg->m_fB0 = 0.0f;
    }
    else
    {
        // Initialize the evaluator scratch pad data header.
        EE_ASSERT(pkEvalSPData->GetSPSegmentMinTime() == NI_INFINITY);
        EE_ASSERT(pkEvalSPData->GetSPSegmentMaxTime() == -NI_INFINITY);
        pkEvalSPData->SetSPFillFunc(&BSplineFloatFillFunction);

        // Initialize the scratch pad fill data.
        NiScratchPad::BSplineFloatFillData* pkFillData =
            (NiScratchPad::BSplineFloatFillData*)
            pkEvalSPData->GetSPFillData();
        EE_ASSERT(pkFillData);
        pkFillData->m_kCPHandle = m_kFloatCPHandle;
        pkFillData->m_fStartTime = m_fStartTime;
        pkFillData->m_fInvDeltaTime = (m_fEndTime != m_fStartTime) ?
            1.0f / (m_fEndTime - m_fStartTime) : 0.0f;
        pkFillData->m_iLastMin = -INT_MAX;
        // Reference the source control point data since these remain
        // unchanged regardless of the specified time.
        pkFillData->m_pkData = m_spData;

        // Partially initialize the scratch pad segment data.
        NiScratchPad::BSplineFloatSegmentData* pkBSplineSeg =
            (NiScratchPad::BSplineFloatSegmentData*)
            pkEvalSPData->GetSPSegmentData();
        EE_ASSERT(pkBSplineSeg);
        pkBSplineSeg->m_kHeader.m_sLOD = pkEvalSPData->GetLOD();
        if (IsReferencedEvaluator())
        {
            EE_ASSERT(kPBHandle.GetChannelType() == PBREFERENCEDCHANNEL);
            pkBSplineSeg->m_kHeader.m_usOutputIndex =
                NiScratchPad::INVALIDOUTPUTINDEX;
        }
        else
        {
            EE_ASSERT(kPBHandle.GetChannelType() == PBFLOATCHANNEL);
            pkBSplineSeg->m_kHeader.m_usOutputIndex =
                kPBHandle.GetChannelIndex();
        }
        // Reference the scratch pad basis data since this must be
        // updated based on the specified time.
        EE_ASSERT(pkSPBasisData);
        pkBSplineSeg->m_pkSPBasisData = pkSPBasisData;
#ifdef NIDEBUG
        // The fill function should set these fields.
        pkBSplineSeg->m_afSourceArray[0] = 0.0f;
        pkBSplineSeg->m_afSourceArray[1] = 0.0f;
        pkBSplineSeg->m_afSourceArray[2] = 0.0f;
        pkBSplineSeg->m_afSourceArray[3] = 0.0f;
#endif
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineFloatEvaluator::BSplineFloatFillFunction(float fTime,
    NiEvaluatorSPData* pkEvalSPData)
{
    EE_ASSERT(pkEvalSPData);

    // Get the scratch pad fill and segment data.
    NiScratchPad::BSplineFloatFillData* pkFillData =
        (NiScratchPad::BSplineFloatFillData*)
        pkEvalSPData->GetSPFillData();
    EE_ASSERT(pkFillData);

    NiScratchPad::BSplineFloatSegmentData* pkBSplineSeg =
        (NiScratchPad::BSplineFloatSegmentData*)
        pkEvalSPData->GetSPSegmentData();
    EE_ASSERT(pkBSplineSeg);

    // Update the basis to the specified time.
    int iMin, iMax;
    NiBSplineBasis<float, 3>& kBasis =
        pkBSplineSeg->m_pkSPBasisData->GetDegree3Basis();
    float fNormTime = (fTime - pkFillData->m_fStartTime) *
        pkFillData->m_fInvDeltaTime;
    kBasis.Compute(fNormTime, iMin, iMax);

    // Check if we need to load new control points.
    if (iMin != pkFillData->m_iLastMin)
    {
        const float* pfSource =
            pkFillData->m_pkData->GetControlPoint(
            pkFillData->m_kCPHandle, iMin, 1);
        float* pfDest = pkBSplineSeg->m_afSourceArray;
        float* pfEndDest = pfDest + 4;
        while (pfDest < pfEndDest)
        {
            *pfDest = *pfSource;
            pfSource++;
            pfDest++;
        }
        pkFillData->m_iLastMin = iMin;
    }

    // There's no need to fill the scratch pad at an identical time.
    pkEvalSPData->SetSPSegmentTimeRange(fTime, fTime);

    return true;
}

//--------------------------------------------------------------------------------------------------
void NiBSplineFloatEvaluator::SetEvalChannelTypes()
{
    NiUInt32 uiCPCount = GetControlPointCount(FLOAT);
    if (uiCPCount > 0)
    {
        EE_ASSERT(m_kFloatCPHandle != NiBSplineData::INVALID_HANDLE);
        EE_ASSERT(m_spData);
        EE_ASSERT(m_spBasisData);

        m_aiEvalChannelTypes[FLOAT] = EVALFLOATCHANNEL;

        // Determine if the channel is posed.
        bool bPosed = true;
        if (UsesCompressedControlPoints())
        {
            const short* psValue = m_spData->GetCompactControlPoint(
                m_kFloatCPHandle, 0, 1);
            const short* psEndValue = psValue + uiCPCount;
            short sValue0 = *psValue;
            psValue++;
            while (psValue < psEndValue)
            {
                if (*psValue != sValue0)
                {
                    bPosed = false;
                    break;
                }
                psValue++;
            }
        }
        else
        {
            const float* pfValue = m_spData->GetControlPoint(
                m_kFloatCPHandle, 0, 1);
            const float* pfEndValue = pfValue + uiCPCount;
            float fValue0 = *pfValue;
            pfValue++;
            while (pfValue < pfEndValue)
            {
                if (*pfValue != fValue0)
                {
                    bPosed = false;
                    break;
                }
                pfValue++;
            }
        }

        if (bPosed)
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
NiImplementCreateClone(NiBSplineFloatEvaluator);

//--------------------------------------------------------------------------------------------------
void NiBSplineFloatEvaluator::CopyMembers(
    NiBSplineFloatEvaluator* pkDest,
    NiCloningProcess& kCloning)
{
    NiBSplineEvaluator::CopyMembers(pkDest, kCloning);

    pkDest->m_kFloatCPHandle = m_kFloatCPHandle;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiBSplineFloatEvaluator);

//--------------------------------------------------------------------------------------------------
void NiBSplineFloatEvaluator::LoadBinary(NiStream& kStream)
{
    NiBSplineEvaluator::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_kFloatCPHandle);
}

//--------------------------------------------------------------------------------------------------
void NiBSplineFloatEvaluator::LinkObject(NiStream& kStream)
{
    NiBSplineEvaluator::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineFloatEvaluator::RegisterStreamables(NiStream& kStream)
{
    if (!NiBSplineEvaluator::RegisterStreamables(kStream))
    {
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void NiBSplineFloatEvaluator::SaveBinary(NiStream& kStream)
{
    NiBSplineEvaluator::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_kFloatCPHandle);
}

//--------------------------------------------------------------------------------------------------
bool NiBSplineFloatEvaluator::IsEqual(NiObject* pkObject)
{
    if (!NiBSplineEvaluator::IsEqual(pkObject))
    {
        return false;
    }

    NiBSplineFloatEvaluator* pkDest =
        (NiBSplineFloatEvaluator*) pkObject;

    if (pkDest->m_kFloatCPHandle != m_kFloatCPHandle)
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Viewer strings
//--------------------------------------------------------------------------------------------------
void NiBSplineFloatEvaluator::GetViewerStrings(
    NiViewerStringsArray* pkStrings)
{
    NiBSplineEvaluator::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(NiBSplineFloatEvaluator::ms_RTTI
        .GetName()));

    pkStrings->Add(NiGetViewerString("m_kFloatCPHandle", m_kFloatCPHandle));
}

//--------------------------------------------------------------------------------------------------
