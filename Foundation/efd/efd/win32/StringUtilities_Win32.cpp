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

#include <efd/StringUtilities.h>

using namespace efd;

//--------------------------------------------------------------------------------------------------
efd::SInt32 efd::Sprintf(efd::Char* dest, size_t destSize, const efd::Char* format, ...)
{
    EE_ASSERT(format);

    va_list args;
    va_start(args, format);
    efd::SInt32 ret = Vsprintf(dest, destSize, format, args);
    va_end(args);

    return ret;
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 efd::Vsprintf(efd::Char* dest, size_t destSize, const efd::Char* format,
    va_list args)
{
    return Vsnprintf(dest, destSize, EE_TRUNCATE, format, args);
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 efd::Vscprintf(const efd::Char* format, va_list args)
{
    return _vscprintf(format, args);
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 efd::Snprintf(efd::Char* dest, size_t destSize, size_t count,
    const efd::Char* format, ...)
{
    EE_ASSERT(format);

    va_list args;
    va_start(args, format);
    efd::SInt32 ret = Vsnprintf(dest, destSize, count, format, args);
    va_end(args);

    return ret;
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 efd::Vsnprintf(efd::Char* dest, size_t destSize, size_t count,
    const efd::Char* format, va_list args)
{
    if (destSize == 0)
    {
        return 0;
    }

    EE_ASSERT(dest);
    EE_ASSERT(count < destSize || count == EE_TRUNCATE);
    EE_ASSERT(format);

    // Ensure that input buffer is cleared out.
    dest[0] = '\0';

    bool bTruncate = (count == EE_TRUNCATE);

#if _MSC_VER >= 1400
    efd::SInt32 ret = vsnprintf_s(dest, destSize, count, format, args);
#else   // _MSC_VER >= 1400
    if (bTruncate)
    {
        count = destSize - 1;
    }
    efd::SInt32 ret = _vsnprintf(dest, count, format, args);
#endif  // _MSC_VER >= 1400

    if (ret == -1 && !bTruncate)
    {
        ret = (efd::SInt32)count;
    }

#if _MSC_VER < 1400
    // Ensure that the string ends in a null character.
    if (ret == -1)
    {
        dest[destSize - 1] = '\0';
    }
    else
    {
        dest[ret] = '\0';
    }
#endif  // #if _MSC_VER < 1400

    return ret;
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 efd::Stricmp(const efd::Char* s1, const efd::Char* s2)
{
#if _MSC_VER >= 1400
    return _stricmp(s1, s2);
#else // #if _MSC_VER >= 1400
    return stricmp(s1, s2);
#endif // #if _MSC_VER >= 1400
}

//--------------------------------------------------------------------------------------------------
efd::SInt32 efd::Strnicmp(const efd::Char* s1, const efd::Char* s2, size_t n)
{
#if _MSC_VER >= 1400
    return _strnicmp(s1, s2, n);
#else // #if _MSC_VER >= 1400
    return strnicmp(s1, s2, n);
#endif // #if _MSC_VER >= 1400
}

//--------------------------------------------------------------------------------------------------
efd::Char* efd::Strdup(const efd::Char* str)
{
    if (str == NULL)
        return NULL;

    size_t stLen =  strlen(str);
    efd::Char* pcReturn = EE_ALLOC(efd::Char, stLen+1);
    memcpy(pcReturn, str, stLen);
    pcReturn[stLen] = '\0';
    return pcReturn;
}

//--------------------------------------------------------------------------------------------------
// WChar version of function to write formatted output using a pointer to a
// list of arguments.
efd::SInt32 efd::WSprintf(WChar* dest, size_t destSize, const WChar* format, ...)
{
    if (destSize == 0)
        return 0;

    va_list args;
    va_start(args, format);

#if _MSC_VER >= 1400
    efd::SInt32 ret = vswprintf_s((wchar_t *)dest, destSize,
        (const wchar_t *)format, args);
#else // #if _MSC_VER >= 1400
    efd::SInt32 ret = vswprintf(dest, destSize, format, args);

    if (ret >= 0 && ((efd::UInt32)ret == destSize - 1) &&
        dest[destSize - 1] != '\0')
    {
        // This is a rare case where the written string fits but
        // is not null terminated. We will report this as an error.
        ret = -1;
    }
#endif // #if _MSC_VER >= 1400

    va_end(args);

    dest[destSize - 1] = '\0';

    return ret;
}

//--------------------------------------------------------------------------------------------------
