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
#include "efdPCH.h"

#include <efd/ID128.h>
#include <efd/EEMath.h>

//--------------------------------------------------------------------------------------------------
efd::ID128 efd::ID128::GenUUID()
{
    EE_COMPILETIME_ASSERT(RAND_MAX == 0x7FFF);
    efd::UInt8 idValue[16];
    //DT32353 This loop could be more efficient as we are only using 8 of the 15
    /// randomly generated bits per call to rand.
    for (efd::UInt32 i = 0; i < 16; ++i)
    {
        idValue[i] = (efd::UInt8)Rand();
    }
    // set the bits for version 4 from RFC4122
    //Set the 2 most significant bits (bits numbered 6 and 7) of the
    //clock_seq_hi_and_reserved to 0 and 1, respectively. (byte 8)
    // set bit 6 to 0
    idValue[8] &= 0xBF;
    // set bit 7 to 1
    idValue[8] |= 0x80;

    //Set the 4 most significant bits (bits numbered 12 to 15 inclusive) (byte 7)
    //of the time_hi_and_version field to the 4-bit version number
    //corresponding to the UUID version being created. In this case 0100 or 4
    // set our bits to 0
    idValue[7] &= 0x0F;
    // now set them to 4
    idValue[7] |= 0x40;
    return ID128(idValue);
}