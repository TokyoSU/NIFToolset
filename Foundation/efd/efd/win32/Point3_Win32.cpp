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

#include <efd/Point3.h>

using namespace efd;

// initialization for fast square roots
UInt32* Point3::ms_pSqrtTable = 0;
//-------------------------------------------------------------------------------------------------
void Point3::_SDMInit()
{
    ms_pSqrtTable = InitSqrtTable();
}
//-------------------------------------------------------------------------------------------------
void Point3::_SDMShutdown()
{
    EE_FREE(ms_pSqrtTable);
    ms_pSqrtTable = 0;
}
//-------------------------------------------------------------------------------------------------
// This algorithm was published as "A High Speed, Low Precision Square Root",
// by Paul Lalonde and Robert Dawon, Dalhousie University, Halifax, Nova
// Scotia, Canada, on pp. 424-6 of "Graphics Gems", edited by Andrew Glassner,
// Academic Press, 1990.

// These results are generally faster than their full-precision counterparts
// (except on modern PC hardware), but are only worth 7 bits of binary
// precision (1 in 128).
// [A table for 7-bit precision requires 256 entries.]
void Point3::UnitizeVectors(Point3* EE_RESTRICT pVectors, UInt32 count, UInt32 stride)
{
    Float32 leng;
    SInt16 exponent;

    // This pointer allows us to treat the float as its integer bit
    // representation.
    UInt32 *pRep = (UInt32*)&leng;

    EE_ASSERT(stride % sizeof(Float32) == 0);

    // WARNING:  SERIOUS ALIASING going on here.  Be very careful with
    // optimization flags.
    for (UInt32 i = 0; i < count; i++)
    {
        // Compute the squared length normally.
        leng = pVectors->x*pVectors->x + pVectors->y*pVectors->y +
            pVectors->z*pVectors->z;

        if (!(*pRep)) // If the squared length is zero, exit.
        {
            leng = 0.0f;
        }
        else
        {
            // Shift and mask the exponent from the float.
            exponent = static_cast<efd::SInt16>(((*pRep) >> 23) - 127);

            // Mask the exponent away.
            *pRep &= 0x7fffff;

            // If the exponent is odd, use the upper half of the square root
            // table.
            if (exponent & 0x1)
                *pRep |= 0x800000;

            // Compute the sqrt'ed exponent (divide by 2).
            exponent >>= 1;

            // Build the new floating point representation by ORing the
            // looked-up mantissa with the computed exponent.
            *pRep = ms_pSqrtTable[(*pRep) >> 16] | ((exponent + 127) << 23);

            leng = 1.0f / leng;   // Invert the length.
        }

        pVectors->x *= leng;
        pVectors->y *= leng;
        pVectors->z *= leng;

        pVectors = (Point3*)((size_t)pVectors + stride);
    }
}
//-------------------------------------------------------------------------------------------------
void Point3::PointsPlusEqualFloatTimesPoints(
    Point3* EE_RESTRICT pDst,
    Float32 f,
    const Point3* EE_RESTRICT pSrc,
    UInt32 count)
{
    // This assert tests the validity of the restrict modifier
    EE_ASSERT(pDst != pSrc);

    for (UInt32 i = 0; i < count; i++)
    {
        pDst[i] += f * pSrc[i];
    }
}
//-------------------------------------------------------------------------------------------------
void Point3::WeightedPointsPlusWeightedPoints(
    Point3* EE_RESTRICT pDst,
    Float32 weight,
    const Point3* EE_RESTRICT pSrc,
    UInt32 count)
{
    // This assert tests the validity of the restrict modifier
    EE_ASSERT(pDst != pSrc);

    float oneMinusWeight = 1.0f - weight;
    for (UInt32 i = 0; i < count; i++)
    {
        pDst[i] = oneMinusWeight * pSrc[i] + weight * pDst[i];
    }
}
//-------------------------------------------------------------------------------------------------
