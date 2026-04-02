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
#include <efd/MemoryDefines.h>
#ifndef __SPU__
#include <efd/Serialize.h>
#endif

using namespace efd;

// Not using static initialization on SPU
#ifndef __SPU__

const Point3 Point3::ZERO(0.0f,0.0f,0.0f);
const Point3 Point3::UNIT_X(1.0f,0.0f,0.0f);
const Point3 Point3::UNIT_Y(0.0f,1.0f,0.0f);
const Point3 Point3::UNIT_Z(0.0f,0.0f,1.0f);
const Point3 Point3::UNIT_ALL(1.0f,1.0f,1.0f);

#endif // #ifndef __SPU__

EE_COMPILETIME_ASSERT(sizeof(Point3) == 3 * sizeof(float));

#ifndef __SPU__

//------------------------------------------------------------------------------------------------
// This algorithm for fast square roots was published as "A High Speed, Low
// Precision Square Root", by Paul Lalonde and Robert Dawon, Dalhousie
// University, Halifax, Nova Scotia, Canada, on pp. 424-6 of "Graphics Gems",
// edited by Andrew Glassner, Academic Press, 1990.

// These results are generally faster than their full-precision counterparts
// (except on modern PC hardware), but are only worth 7 bits of binary
// precision (1 in 128).
UInt32* Point3::InitSqrtTable()
{
    union FloatIntUnion
    {
        UInt32 ui;
        Float32 f;
    };

    FloatIntUnion kRep;

    // A table of square roots with 7-bit precision requires 256 entries.
    UInt32* pSqrtTable = EE_ALLOC(UInt32, 256);

    for (unsigned int i = 0; i < 128; i++)
    {
        // Build a float with the bit pattern i as mantissa and 0 as exponent.
        kRep.ui = (i<<16) | (127<<23);
        kRep.f = efd::Sqrt(kRep.f);

        // Store the first 7 bits of the mantissa in the table.
        pSqrtTable[i] = (kRep.ui & 0x7fffff);

        // Build a float with the bit pattern i as mantissa and 1 as exponent.
        kRep.ui = (i << 16) | (128 << 23);
        kRep.f = efd::Sqrt(kRep.f);

        // Store the first 7 bits of the mantissa in the table.
        pSqrtTable[i+128] = (kRep.ui & 0x7fffff);
    }

    return pSqrtTable;
}

//------------------------------------------------------------------------------------------------
void Point3::Serialize(efd::Archive& ar)
{
    efd::Serializer::SerializeObject(x, ar);
    efd::Serializer::SerializeObject(y, ar);
    efd::Serializer::SerializeObject(z, ar);
}

//------------------------------------------------------------------------------------------------
#endif
