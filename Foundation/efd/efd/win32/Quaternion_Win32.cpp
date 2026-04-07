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

#include "efdPCH.h"

#include <efd/RTLib.h>
#include <efd/Quaternion.h>
#include <efd/EEMath.h>

using namespace efd;
//-------------------------------------------------------------------------------------------------
Quaternion Quaternion::Exp(const Quaternion& q)
{
    // q = A*(x*i+y*j+z*k) where (x,y,z) is unit length
    // exp(q) = cos(A)+sin(A)*(x*i+y*j+z*k)
    Float32 angle = efd::Sqrt(q.m_fX*q.m_fX + q.m_fY*q.m_fY + q.m_fZ*q.m_fZ);
    Float32 sn, cs;
    efd::SinCos(angle, sn, cs);

    // When A is near zero, sin(A)/A is approximately 1.  Use
    // exp(q) = cos(A)+A*(x*i+y*j+z*k)
    Float32 coeff = (efd::Abs(sn) < ms_epsilon ? 1.0f : sn/angle);
    Quaternion result(cs, coeff * q.m_fX, coeff * q.m_fY, coeff * q.m_fZ);
    return result;
}
//-------------------------------------------------------------------------------------------------
// *** The following code must be updated in the Quaternion documentation
// if it changes. ***

// Compute 1/sqrt(s) using a tangent line approximation.
// These constants are outside of the function because
// not all compilers are smart enough to precompute the values.
static const Float32 EE_ISQRT_NEIGHBORHOOD = 0.959066f;
static const Float32 EE_ISQRT_SCALE = 1.000311f;
static const Float32 EE_ISQRT_ADDITIVE_CONSTANT =
    EE_ISQRT_SCALE / (Float32)efd::Sqrt(EE_ISQRT_NEIGHBORHOOD);
static const Float32 EE_ISQRT_FACTOR = EE_ISQRT_SCALE * (-0.5f /
    (EE_ISQRT_NEIGHBORHOOD * (Float32)efd::Sqrt(EE_ISQRT_NEIGHBORHOOD)));
//-------------------------------------------------------------------------------------------------
Float32 Quaternion::ISqrtApproxInNeighborhood(Float32 s)
{
    return EE_ISQRT_ADDITIVE_CONSTANT + (s - EE_ISQRT_NEIGHBORHOOD) *
        EE_ISQRT_FACTOR;
}
//-------------------------------------------------------------------------------------------------
// Normalize a quaternion using the above approximation.
void Quaternion::FastNormalize()
{
    Float32 s = m_fX*m_fX + m_fY*m_fY + m_fZ*m_fZ + m_fW*m_fW; // length^2
    Float32 k = ISqrtApproxInNeighborhood(s);

    if (s <= 0.91521198f) {
        k *= ISqrtApproxInNeighborhood(k * k * s);

        if (s <= 0.65211970f) {
            k *= ISqrtApproxInNeighborhood(k * k * s);
        }
    }

    m_fX *= k;
    m_fY *= k;
    m_fZ *= k;
    m_fW *= k;
}
//-------------------------------------------------------------------------------------------------
Float32 Quaternion::Lerp(Float32 v0, Float32 v1, Float32 perc)
{
    return v0 + perc * (v1 - v0);
}
//-------------------------------------------------------------------------------------------------
Float32 Quaternion::CounterWarp(Float32 t, Float32 cosine)
{
    const Float32 EE_ATTENUATION = 0.82279687f;
    const Float32 EE_WORST_CASE_SLOPE = 0.58549219f;

    Float32 factor = 1.0f - EE_ATTENUATION * cosine;
    factor *= factor;
    Float32 k = EE_WORST_CASE_SLOPE * factor;

    return t*(k*t*(2.0f*t - 3.0f) + 1.0f + k);
}
//-------------------------------------------------------------------------------------------------
Quaternion Quaternion::Slerp(
    Float32 t,
    const Quaternion& p,
    const Quaternion& q)
{
    // assert:  Dot(p,q) >= 0 (guaranteed in NiRotKey::Interpolate methods)
    // (but not necessarily true when coming from a Squad call)

    // This algorithm is Copyright (c) 2002 Jonathan Blow, from his article
    // "Hacking Quaternions" in Game Developer Magazine, March 2002.

    Float32 cosine = Dot(p, q);

    Float32 prime;
    if (t <= 0.5f)
    {
        prime = CounterWarp(t, cosine);
    }
    else
    {
        prime = 1.0f - CounterWarp(1.0f - t, cosine);
    }

    Quaternion result(
        Lerp(p.GetW(), q.GetW(), prime),
        Lerp(p.GetX(), q.GetX(), prime),
        Lerp(p.GetY(), q.GetY(), prime),
        Lerp(p.GetZ(), q.GetZ(), prime));

    result.FastNormalize();
    return result;
}
//-------------------------------------------------------------------------------------------------
void Quaternion::Slerp(
    Float32 t,
    const Quaternion& p,
    const Quaternion& q,
    Quaternion* pResults)
{
    EE_ASSERT(pResults);

    // assert:  Dot(p,q) >= 0 (guaranteed in NiRotKey::Interpolate methods)
    // (but not necessarily true when coming from a Squad call)

    // This algorithm is Copyright (c) 2002 Jonathan Blow, from his article
    // "Hacking Quaternions" in Game Developer Magazine, March 2002.

    Float32 cosine = Dot(p, q);

    Float32 prime;
    if (t <= 0.5f)
    {
        prime = CounterWarp(t, cosine);
    }
    else
    {
        prime = 1.0f - CounterWarp(1.0f - t, cosine);
    }

    pResults->m_fW = Lerp(p.GetW(), q.GetW(), prime);
    pResults->m_fX = Lerp(p.GetX(), q.GetX(), prime);
    pResults->m_fY = Lerp(p.GetY(), q.GetY(), prime);
    pResults->m_fZ = Lerp(p.GetZ(), q.GetZ(), prime);

    pResults->FastNormalize();
}
//-------------------------------------------------------------------------------------------------
