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

//==========================================================================
//  utf16char_t implementation
//==========================================================================

#include "efdPCH.h"

#include <efd/utf8char.h>

using namespace efd;
//--------------------------------------------------------------------------------------------------
//  utf16_num_units()
//--------------------------------------------------------------------------------------------------
unsigned efd::utf16_num_units(const efd::WChar* buffer_sz)
{
    return utf16_num_units(*buffer_sz);
}

//--------------------------------------------------------------------------------------------------
//  utf16_num_units()
//--------------------------------------------------------------------------------------------------
unsigned efd::utf16_num_units(efd::WChar byte)
{
    if (byte < 0xD800 || byte > 0xDFFF)
    {
        return 1;
    }
    return 2;
}


//--------------------------------------------------------------------------------------------------
//  utf16_validate_char()
//--------------------------------------------------------------------------------------------------
bool efd::utf16_validate_char(const efd::WChar* source, unsigned length)
{
    switch (length)
    {
        case 1:
            return (source[0] < 0xD800 || source[0] > 0xDFFF);

        case 2:
            return ((source[0] >= 0xD800 && source[0] <= 0xDBFF) &&
                    (source[1] >= 0xDC00 && source[1] <= 0xDFFF));
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
//  utf16_tail_fragments()
//--------------------------------------------------------------------------------------------------
unsigned efd::utf16_tail_fragments(const efd::WChar* source,  unsigned length)
{
    EE_ASSERT(length > 0);

    // If the last character is single uint then we have no fragments
    if (1 == utf16_num_units(source[ length-1 ]))
    {
        return 0;
    }

    // Otherwise the last character needs two units so the length had best be at least 2 or
    // else we have a fragment.  If we can safely access the previous character then it must
    // be a two unit character or else we have a fragment.
    if ((length > 1) &&
        (2 == utf16_num_units(source[ length-2 ])))
    {
        return 0;
    }
    return 1;
}

