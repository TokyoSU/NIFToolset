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

#include <efd/EEMath.h>

using namespace efd;
//-------------------------------------------------------------------------------------------------
Float32 efd::FastATan2(Float32 y, Float32 x)
{
    // Poly approximation valid for |z| <= 1.  To compute ATAN(z)
    // for z > 1, use ATAN(z) = PI/2 - ATAN(1/z).  For z < -1, use
    // ATAN(z) = -PI/2 - ATAN(1/z).

    if (x == 0.0f && y == 0.0f)
        return 0.0f;

    Float32 offset = 0.0f;
    Float32 z;
    if (efd::Abs(y) > efd::Abs(x))
    {
        //  |y/x| > 1 so use 1/z identities.
        z = x / y;
        if (z > 0.0f)
        {
            offset = EE_HALF_PI;
        }
        else if (z < 0.0f)
        {
            offset = -EE_HALF_PI;
        }
        else // z == 0.0f
        {
            // special case for 90deg and -90deg
            return (y > 0.0f) ? EE_HALF_PI : -EE_HALF_PI;
        }
    }
    else
    {
        z = y / x;

        if (z == 0.0f)
        {
            // special case for 0deg and 180deg
            return (x > 0.0f) ? 0.0f : EE_PI;
        }
    }

    Float32 z2 = z * z;

    // Polynomial approximation of degree 9, P(z).
    // |ATAN(z)-P(z)| <= 1e-05

    Float32 result;
    result = 0.0208351f;
    result *= z2;
    result -= 0.0851330f;
    result *= z2;
    result += 0.1801410f;
    result *= z2;
    result -= 0.3302995f;
    result *= z2;
    result += 0.9998660f;
    result *= z;

    if (offset)
        result = offset - result;

    // find proper solution to two arg arctan
    if (y < 0.0f && x < 0.0f)  // quadrant IV
        result -= EE_PI;
    if (y > 0.0f && x < 0.0f)  // quadrant II
        result += EE_PI;

    return result;
}
//-------------------------------------------------------------------------------------------------
