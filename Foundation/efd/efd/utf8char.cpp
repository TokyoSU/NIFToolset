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

#include <efd/Asserts.h>
#include <efd/utf8char.h>
#include <efd/Serialize.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
//  This table maps the first byte of an encoded UTF-8 character to the
//  total number of bytes used to encode the character.
//------------------------------------------------------------------------------------------------
static const char _utf8_multibyte_count[256] =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x00 - 0x0F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x10 - 0x1F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x20 - 0x2F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x30 - 0x3F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x40 - 0x4F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x50 - 0x5F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x60 - 0x6F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x70 - 0x7F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x80 - 0x8F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0x90 - 0x9F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0xA0 - 0xAF
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     // 0xB0 - 0xBF
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,     // 0xC0 - 0xCF
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,     // 0xD0 - 0xDF
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // 0xE0 - 0xEF

    // The final row of this table is tricky.  The code we originally used to model this was
    // invalid according to the most recent specs (it was old code that pre-dated the finally
    // drafting of the official standard).  We can do this in one of three ways:

// 1.) This version treats invalid values as if they were valid values under the pre-standardized
// UTF8 system
//    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1     // 0xF0 - 0xFF

// 2.) This version treats invalid values as zero bytes.  This would force a string of any
// length to terminate at the first invalid character.  Is that more or less desireable than
// the following?  Of course if we do this then values between 0x80 and 0xBF should also be 0.
//    4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0     // 0xF0 - 0xFF

// 3.) This version treats invalid values as single bytes.  Currently we pass many invalid
// strings to these UTF functions.  This is mainly the result of passing in ACSII strings
// as if they are guaranteed to be valid UTF8 strings.  This version will maintain the "invalid"
// strings better than the other versions.  Of course, trying to handle invalid input might
// lead to us walking past the end of our buffer in some cases.  We already treat "invalid"
// lead units in the range 0x80 to 0xBF as single bytes so this seems most compatible with
// that and should make it harder for us to walk past terminating NULL characters.  Then
// again we treat invalid lead values 0xC0 and 0xC1 as 2 bytes.
    4, 4, 4, 4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1     // 0xF0 - 0xFF
};

//------------------------------------------------------------------------------------------------
//  utf8_num_units()
//------------------------------------------------------------------------------------------------
unsigned efd::utf8_num_units(const char *buffer_sz)
{
    EE_ASSERT(buffer_sz != NULL);
    unsigned index = buffer_sz[0] & 0xFF;
    EE_ASSERT(index >= 0 && index <= 255);
    unsigned size = _utf8_multibyte_count[index];
    EE_ASSERT(size > 0 && size <= 4);
    return size;
}

//------------------------------------------------------------------------------------------------
//  utf8_num_units()
//------------------------------------------------------------------------------------------------
unsigned efd::utf8_num_units(unsigned char byte)
{
    return _utf8_multibyte_count[byte];
}


//------------------------------------------------------------------------------------------------
//  utf8_validate_char()
//
//  See the offical spec at http://tools.ietf.org/html/rfc3629 for details.
//
//  NOTE: Never touch memory beyond a NULL character while verifying the correctness of the
//      input string!  If we walk past a NULL we might be walking off the end of a buffer
//      and we might fault trying to read invalid memory locations.  The previous implementaion
//      of this function might have been faster but we often walked past the end of our memory
//      buffers when using it
//------------------------------------------------------------------------------------------------
bool efd::utf8_validate_char(const char* source, unsigned length)
{
    // Early out to avoid reading from source if its of size zero.
    if (0 == length || length > 4)
    {
        return false;
    }

    unsigned char a = (unsigned char)*source;
    unsigned int expectedLength = _utf8_multibyte_count[a];
    if (expectedLength != length)
    {
        return false;
    }

    // The first character is the tricky one.  Its valid values depend on the length.
    switch (length)
    {
    case 1:
        if (a > 0x7F)
            return false;
        break;

    case 2:
        // The two values 0xC0?? and 0xC1?? are illegal because they fit within a single
        // byte and should never be sent as two bytes.  Otherwise all values for which we
        // return 2 in the _utf8_multibyte_count table are valid.  Perhaps we should fix
        // this special case by updating the lookup table.
        if (a == 0x80 || a == 0x81)
            return false;
    case 3:
        // No aditional checking beyond the expectedLength check above is required since length
        // 2 and 3 connect consecutively into the length 1 and 4 cases.
        break;

    case 4:
        if (a > 0xF7)
            return false;
        break;
    }

    // Any remaining characters are easy, they must have 0x80 as the high two bits:
    for (unsigned int i=1; i < length; ++i)
    {
        a = (unsigned char)source[i];
        if (a < 0x80 || a > 0xBF)
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
//  utf8_tail_fragments()
//------------------------------------------------------------------------------------------------
unsigned efd::utf8_tail_fragments(
    const char* source,
    unsigned length)
{
    if (!length)
        return 0;

    const char* srcptr = source + length;
    unsigned fragment = 0;

    bool flag = true;
    while (flag)
    {
        --srcptr;
        ++fragment;

        if ((*srcptr & '\xC0') != '\x80')
        {
            break;
        }
    }

    unsigned chlen = utf8_num_units(*srcptr);

    if (!utf8_validate_char(srcptr, chlen))
    {
        return chlen - fragment;
    }

    return 0;
}

//------------------------------------------------------------------------------------------------
void utf8char_t::Serialize(efd::Archive& ar)
{
    if (!ar.CheckBytes(1))
    {
        ar.RaiseError();
        return;
    }

    UInt32 charSize;
    if (ar.IsPacking())
    {
        charSize = size();
    }
    else
    {
        charSize = utf8_num_units((const char*)ar.PeekBytes(ar.GetCurrentPosition(), 1));
    }

    Serializer::SerializeRawBytes((efd::UInt8*)bytes, charSize, ar);
}

//------------------------------------------------------------------------------------------------
