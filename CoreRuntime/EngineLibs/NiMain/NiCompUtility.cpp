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
#include "NiMainPCH.h"

#include <NiRTLib.h>
#include "NiCompUtility.h"
#include "NiMath.h"
#include <NiDebug.h>
#include <NiMemoryDefines.h>
#include <NiOS.h>

//--------------------------------------------------------------------------------------------------
NiCompUtility::NiCompArray NiCompUtility::CompressFloatArray(
    const float* pfSrcArray, unsigned int uiSrcCount)
{
    float fMin, fMax;
    GetMinMax(pfSrcArray, uiSrcCount, fMin, fMax);
    float fOffset = (fMin + fMax) * 0.5f;
    float fHalfRange = (fMax - fMin) * 0.5f;
    EE_ASSERT(fMax > fMin);

    NiCompArray kArray;
    kArray.m_asArray = NiAlloc(short, uiSrcCount);
    kArray.m_fOffset = fOffset;
    kArray.m_fHalfRange = fHalfRange;
    kArray.m_uiNumValues = uiSrcCount;

    float fShortMax = (float)SHRT_MAX;
    for (unsigned int ui = 0; ui < uiSrcCount; ui++)
    {
        float fValue = ((pfSrcArray[ui] - fOffset) /
            (fHalfRange) * (fShortMax));
        if (fValue < -fShortMax)
            fValue = -fShortMax;
        if (fValue > fShortMax)
            fValue = fShortMax;
        kArray.m_asArray[ui] = (short) fValue;
    }

    return kArray;
}

//--------------------------------------------------------------------------------------------------
void NiCompUtility::DecompressFloatArray(const short* psSrcArray,
    unsigned int uiSrcCount,
    float fOffset,
    float fHalfRange,
    float* pfDestArray,
    unsigned int uiDestCount)
{
    EE_UNUSED_ARG(uiSrcCount);
    EE_ASSERT(uiSrcCount >= uiDestCount);

    const float fInvShortMax = 1.0f / ((float)SHRT_MAX);
    const float fScale = fInvShortMax * fHalfRange;

    for (unsigned int ui = 0; ui < uiDestCount; ui++)
    {
        pfDestArray[ui] = (((float)psSrcArray[ui]) * fScale) + fOffset;
    }
}

//--------------------------------------------------------------------------------------------------
void NiCompUtility::GetMinMax(const float* pfArray, unsigned int uiCount,
    float& fMin, float& fMax)
{
    fMin = NI_INFINITY;
    fMax = -NI_INFINITY;
    for (unsigned int ui = 0; ui < uiCount; ui++)
    {
        if (pfArray[ui] < fMin)
            fMin = pfArray[ui];
        if (pfArray[ui] > fMax)
            fMax = pfArray[ui];
    }

    EE_ASSERT(fMin < fMax);
}

//--------------------------------------------------------------------------------------------------
