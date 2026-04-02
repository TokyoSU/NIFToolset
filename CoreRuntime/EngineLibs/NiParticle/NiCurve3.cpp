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
#include "NiParticlePCH.h"
#include "NiCurve3.h"
#include "NiPSKernelDefinitions.h"
#include "NiMath.h"
#include "NiStream.h"

NiImplementRTTI(NiCurve3, NiObject);

//--------------------------------------------------------------------------------------------------
NiCurve3::NiCurve3() :
    m_uiDegree(1),
    m_uiNumControlPoints(0),
    m_uiNumKnots(0),
    m_pkControlPoints(NULL),
    m_pfKnots(NULL)
{
}

//--------------------------------------------------------------------------------------------------
NiCurve3::~NiCurve3()
{
    NiDelete[] m_pkControlPoints;
    m_pkControlPoints = NULL;
    m_uiNumControlPoints = 0;

    NiFree(m_pfKnots);
    m_pfKnots = NULL;
    m_uiNumKnots = 0;
}

//--------------------------------------------------------------------------------------------------
void NiCurve3::Initialize(NiUInt32 uiDegree, NiUInt32 uiNumControlPoints,
    NiPoint3* pkControlPoints, NiUInt32 uiNumKnots, float *pfKnots)
{
    EE_ASSERT(uiDegree == 1 || uiNumKnots == uiNumControlPoints + uiDegree + 1);

    SetDegree(uiDegree);
    SetControlPoints(uiNumControlPoints, pkControlPoints);
    SetKnots(uiNumKnots, pfKnots);
}

//--------------------------------------------------------------------------------------------------
void NiCurve3::SetControlPoints(NiUInt32 uiNumControlPoints, NiPoint3* pkControlPoints)
{
    if (pkControlPoints && (uiNumControlPoints > 0))
    {
        NiDelete[] m_pkControlPoints;
        m_pkControlPoints = NiNew NiPoint3[uiNumControlPoints];
        EE_ASSERT(m_pkControlPoints);
        m_uiNumControlPoints = uiNumControlPoints;
        for (NiUInt32 ui=0; ui < uiNumControlPoints; ui++)
        {
            m_pkControlPoints[ui] = pkControlPoints[ui];
        }
    }
    else
    {
        m_uiNumControlPoints = 0;
        m_pkControlPoints = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
void NiCurve3::SetKnots(NiUInt32 uiNumKnots, float *pfKnots)
{
    if (pfKnots && (uiNumKnots > 0))
    {
        NiFree(m_pfKnots);
        m_pfKnots = NiAlloc(float, uiNumKnots);
        EE_ASSERT(m_pfKnots);
        m_uiNumKnots = uiNumKnots;
        for (NiUInt32 ui=0; ui < uiNumKnots; ui++)
        {
            m_pfKnots[ui] = pfKnots[ui];
        }
    }
    else
    {
        m_uiNumKnots = 0;
        m_pfKnots = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
NiPoint3 NiCurve3::GetPointAlongCurve(const float fDistance)
{
    float fT = NiClamp(fDistance, 0.0f, 1.0f);

    if (m_uiDegree == 1)
    {
        if (fT == 1.0f)
            fT = 0.9999f;

        // Convert fT from [0,1] to curve segment.
        NiUInt32 uiNumSpans = GetNumSpans();
        fT *= uiNumSpans;

        NiUInt32 uiSpan = static_cast<NiUInt32>(fT);
        EE_ASSERT(uiSpan < GetNumControlPoints() - 1);

        fT -= static_cast<float>(uiSpan);

        NiPoint3 pt = GetControlPoint(uiSpan) +
            (fT * (GetControlPoint(uiSpan + 1) - GetControlPoint(uiSpan)));

        return pt;
    }

    // Find i.
    NiUInt32 uiKnot = GetKnotBeforeOrAtT(fT);
    EE_ASSERT(uiKnot >= m_uiDegree);
    uiKnot -= m_uiDegree;

    NiUInt32 uiOrder = m_uiDegree + 1;

    // Convert fT from [0,1) to curve segment.
    fT *= GetNumSpans();

    NiPoint3 pt(NiPoint3::ZERO);

    // For all the relevant control points for this span
    for (NiUInt32 k = 0; k < uiOrder; k++)
    {
        float fBlend = GetBlendFunction(uiKnot, uiOrder, fT);
        if (fBlend != 0.0f)
        {
            NiPoint3 cp = GetControlPoint(uiKnot);
            pt += fBlend * cp;
        }
        uiKnot++;
    }

    return pt;
}

//--------------------------------------------------------------------------------------------------
float NiCurve3::GetBlendFunction(NiUInt32 uiCPIndex, NiUInt32 uiOrder, float fT) const
{
    float fBlend = 1.0f;

    EE_ASSERT(uiOrder >= 1 && uiOrder <= m_uiDegree + 1);
    EE_ASSERT(uiCPIndex + uiOrder < m_uiNumKnots);

    if (uiOrder == 1)
    {
        fBlend = ((GetKnot(uiCPIndex) <= fT) && fT < GetKnot(uiCPIndex + 1)) ? 1.0f : 0.0f;
    }
    else
    {
        float fTi = GetKnot(uiCPIndex);
        float fTiPlusOrderMinus1 = GetKnot(uiCPIndex + uiOrder - 1);
        float fNumer1 = fT - fTi;
        float fDenom1 = fTiPlusOrderMinus1 - fTi;

        float fTiPlusOrder = GetKnot(uiCPIndex + uiOrder);
        float fTiPlus1 = GetKnot(uiCPIndex + 1);

        float fNumer2 = fTiPlusOrder - fT;
        float fDenom2 = fTiPlusOrder - fTiPlus1;

        // Recursion step
        uiOrder--;
        if (fDenom1 != 0.0f && fDenom2 != 0.0f)
        {
            fBlend = ((fNumer1 / fDenom1) * GetBlendFunction(uiCPIndex, uiOrder, fT)) +
                ((fNumer2 / fDenom2) * GetBlendFunction(uiCPIndex + 1, uiOrder, fT));
        }
        else if (fDenom1 == 0.0f && fDenom2 == 0.0f)
        {
            fBlend = 0.0f;
        }
        else if (fDenom2 == 0.0f)
        {
            fBlend = (fNumer1 / fDenom1) * GetBlendFunction(uiCPIndex, uiOrder, fT);
        }
        else
        {
            fBlend = (fNumer2 / fDenom2) * GetBlendFunction(uiCPIndex + 1, uiOrder, fT);
        }
    }
    return fBlend;
}

//--------------------------------------------------------------------------------------------------
NiUInt32 NiCurve3::GetKnotBeforeOrAtT(float fT) const
{
    EE_ASSERT(fT >= 0.0f && fT <= 1.0f);

    // Convert fT from [0,1) to curve segment.
    fT *= GetNumSpans();

    // Find i.
    NiUInt32 i = 0;

    NiUInt32 uiNumKnotsToConsider = m_uiNumKnots - m_uiDegree - 2;
    while (i < uiNumKnotsToConsider && !((GetKnot(i) <= fT) && (fT < GetKnot(i+1))))
    {
        ++i;
    }

    EE_ASSERT(GetKnot(i) <= fT);

    return i;
}

//--------------------------------------------------------------------------------------------------
bool NiCurve3::IsEqual(NiObject* pkObject)
{
    if (!NiObject::IsEqual(pkObject))
    {
        return false;
    }

    NiCurve3* pkDest = (NiCurve3*) pkObject;

    if (pkDest->m_uiDegree != m_uiDegree ||
        pkDest->m_uiNumControlPoints != m_uiNumControlPoints ||
        pkDest->m_uiNumKnots != m_uiNumKnots)
    {
        return false;
    }

    if (pkDest->m_pfKnots && m_pfKnots)
    {
        for (NiUInt32 ui=0; ui < m_uiNumKnots; ui++)
        {
            if (m_pfKnots[ui] != m_pfKnots[ui])
            return false;
        }
    }

    if (pkDest->m_pkControlPoints && m_pkControlPoints)
    {
        for (NiUInt32 ui=0; ui < m_uiNumControlPoints; ui++)
        {
            if (m_pkControlPoints[ui] != m_pkControlPoints[ui])
                return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// Cloning
//--------------------------------------------------------------------------------------------------
NiImplementCreateClone(NiCurve3);

//--------------------------------------------------------------------------------------------------
void NiCurve3::CopyMembers(NiCurve3* pkDest, NiCloningProcess& kCloning)
{
    NiObject::CopyMembers(pkDest, kCloning);

    pkDest->m_uiDegree = m_uiDegree;

    // Copy control points.
    pkDest->m_uiNumControlPoints = m_uiNumControlPoints;
    pkDest->m_pkControlPoints = NiNew NiPoint3[m_uiNumControlPoints];
    NiMemcpy(pkDest->m_pkControlPoints, m_uiNumControlPoints * sizeof(NiPoint3),
        m_pkControlPoints, m_uiNumControlPoints * sizeof(NiPoint3));

    // Copy knots.
    pkDest->m_uiNumKnots = m_uiNumKnots;
    pkDest->m_pfKnots = NiAlloc(float, m_uiNumKnots);
    NiMemcpy(pkDest->m_pfKnots, m_uiNumKnots * sizeof(float),
        m_pfKnots, m_uiNumKnots * sizeof(float));
}

#ifndef __SPU__

//--------------------------------------------------------------------------------------------------
// streaming
//--------------------------------------------------------------------------------------------------
NiImplementCreateObject(NiCurve3);

//--------------------------------------------------------------------------------------------------
void NiCurve3::LoadBinary(NiStream& kStream)
{
    NiUInt32 uiValue;
    NiStreamLoadBinary(kStream, uiValue);
    m_uiDegree = uiValue;

    NiStreamLoadBinary(kStream, uiValue);
    m_uiNumControlPoints = uiValue;
    if (m_uiNumControlPoints > 0)
    {
        m_pkControlPoints = NiNew NiPoint3[m_uiNumControlPoints];
        for (NiUInt32 ui = 0; ui < m_uiNumControlPoints; ++ui)
        {
            m_pkControlPoints[ui].LoadBinary(kStream);
        }
    }

    NiStreamLoadBinary(kStream, uiValue);
    m_uiNumKnots = uiValue;
    if (m_uiNumKnots > 0)
    {
        m_pfKnots = NiAlloc(float, m_uiNumKnots);
        for (NiUInt32 ui = 0; ui < m_uiNumKnots; ++ui)
        {
            NiStreamLoadBinary(kStream, m_pfKnots[ui]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void NiCurve3::SaveBinary(NiStream& kStream)
{
    NiUInt32 uiValue = m_uiDegree;
    NiStreamSaveBinary(kStream, uiValue);

    uiValue = m_uiNumControlPoints;
    NiStreamSaveBinary(kStream, uiValue);
    for (NiUInt32 ui = 0; ui < m_uiNumControlPoints; ++ui)
    {
        m_pkControlPoints[ui].SaveBinary(kStream);
    }

    uiValue = m_uiNumKnots;
    NiStreamSaveBinary(kStream, uiValue);
    for (NiUInt32 ui = 0; ui < m_uiNumKnots; ++ui)
    {
        NiStreamSaveBinary(kStream, m_pfKnots[ui]);
    }
}

//--------------------------------------------------------------------------------------------------
void NiCurve3::LinkObject(NiStream& kStream)
{
    NiObject::LinkObject(kStream);
}

//--------------------------------------------------------------------------------------------------
bool NiCurve3::RegisterStreamables(NiStream& kStream)
{
    return NiObject::RegisterStreamables(kStream);
}

//--------------------------------------------------------------------------------------------------
void NiCurve3::GetViewerStrings(NiViewerStringsArray* pkStrings)
{
    pkStrings->Add(NiGetViewerString(NiCurve3::ms_RTTI.GetName()));

    pkStrings->Add(NiGetViewerString("Degree", m_uiDegree));
    pkStrings->Add(NiGetViewerString("Control Points", m_uiNumControlPoints));
    pkStrings->Add(NiGetViewerString("Knots", m_uiNumKnots));
}

//--------------------------------------------------------------------------------------------------
#endif
