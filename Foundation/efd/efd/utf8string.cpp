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
#include <efd/utf8string.h>
#include <efd/SerializeRoutines.h>


namespace efd
{

#if defined(EE_PLATFORM_PS3)
const size_t utf8string::npos = internal_string::npos;
#endif

//------------------------------------------------------------------------------------------------
utf8string::utf8string(
    const utf8string& src_utf8,
    size_type pos_mb,
    size_type count_mb)
{
    size_type pos_sb = utf8_mb_to_sb(src_utf8.data(), pos_mb, src_utf8.size());

    if (pos_sb != npos)
    {
        EE_ASSERT(src_utf8.size() >= pos_sb);

        size_type remaining_buffer = src_utf8.size() - pos_sb;

        size_type count_sb =
            utf8_mb_to_sb(src_utf8.data() + pos_sb, count_mb, remaining_buffer);

        m_string.assign(src_utf8.m_string, pos_sb, count_sb);
    }
    else
    {
        // The starting position is beyond the end of the string, correct result is an empty
        // string so we don't have to do anything else.
        EE_ASSERT(m_string.empty());
    }
}

//------------------------------------------------------------------------------------------------
utf8string::utf8string(_Formatted, const char* format_sz, ...)
{
    va_list args;
    va_start(args, format_sz);
    /*int result =*/ vsprintf(format_sz, args);
    va_end(args);
}

//------------------------------------------------------------------------------------------------
const utf8string::value_type utf8string::operator[](
    size_type pos_mb) const
{
    size_type offset_sb = utf8_mb_to_sb(m_string.data(), pos_mb, m_string.size());
    // Lets assert if we walked past the end of our string:
    EE_ASSERT(offset_sb <= m_string.size());
    return utf8char_t::from_stream((const char *)m_string.data() + offset_sb);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::assign(
    size_type count_mb,
    const utf8char_t& uc)
{
    utf8string tmp;
    tmp.reserve(count_mb * uc.size());
    for (unsigned i = 0; i < count_mb; ++i)
    {
        tmp.push_back(uc);
    }

    m_string.assign(tmp.raw());

    //DT20089 We should append a NULL character to the end of the string if m_string[size] isn't
    // already a NULL.

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::assign(const char* src_sz)
{
    m_string.assign(src_sz);

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::assign(
    const char *src_sz,
    _CountType ct,
    utf8string::size_type count)
{
    if (CT_LENGTH == ct)
    {
        count = utf8_mb_to_sb(src_sz, count, npos);
    }
    m_string.assign(src_sz, count);

    //DT20089 We should append a NULL character to the end of the string if m_string[size] isn't
    // already a NULL.

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::assign(
    const char *src_sz, size_type pos_mb, size_type count_mb)
{
    const size_type pos_sb = utf8_mb_to_sb(src_sz, pos_mb, npos);

    if (npos == pos_sb)
    {
        // if the starting position is beyond the NULL terminator of the string then we'll
        // simply set ourselves to the empty string instead of, say, faulting.
        m_string.clear();
    }
    else
    {
        const size_type count_sb =
            utf8_mb_to_sb(src_sz + pos_sb, count_mb, npos);

        m_string.assign(src_sz + pos_sb, count_sb);
    }

    //DT20089 We should append a NULL character to the end of the string if m_string[size] isn't
    // already a NULL.

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::assign(const utf8string& src_utf8)
{
    m_string.assign(src_utf8.raw());
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::assign(
    const utf8string& src_utf8,
    size_type count_mb)
{
    (*this).assign(src_utf8.raw().data(), CT_LENGTH, count_mb);
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::assign(
    const utf8string& src_utf8,
    size_type pos_mb,
    size_type count_mb)
{
    (*this).assign(src_utf8.raw().data(), pos_mb, count_mb);
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string::size_type utf8string::find(
    const utf8string& str_utf8,
    size_type pos_mb) const
{
    size_type pos_sb = utf8_mb_to_sb(m_string.c_str(), pos_mb, m_string.size());
    if (npos == pos_sb)
    {
        return npos;
    }

    return utf8_sb_to_mb(m_string.c_str(),
                             m_string.find(str_utf8.m_string, pos_sb),
                             m_string.size());
}

//------------------------------------------------------------------------------------------------
utf8string::size_type utf8string::find(
    const char* str_sz,
    size_type pos_mb,
    size_type count_mb) const
{
    size_type pos_sb = utf8_mb_to_sb(m_string.c_str(), pos_mb, m_string.size());
    size_type count_sb = utf8_mb_to_sb(str_sz, count_mb, npos);
    size_type found_sb = m_string.find(str_sz, pos_sb, count_sb);
    size_type found_mb = utf8_sb_to_mb(m_string.c_str(), found_sb, m_string.size());

    return found_mb;
}

//------------------------------------------------------------------------------------------------
utf8string::size_type utf8string::find(
    const char* str_sz,
    size_type pos_mb) const
{
    if (pos_mb == npos)
    {
        pos_mb = 0;
    }

    size_type pos_sb = utf8_mb_to_sb(m_string.c_str(), pos_mb, m_string.size());
    size_type found_sb = m_string.find(str_sz, pos_sb);
    size_type found_mb = utf8_sb_to_mb(m_string.c_str(), found_sb, m_string.size());

    return found_mb;
}

//------------------------------------------------------------------------------------------------
utf8string::size_type utf8string::find(
    const utf8char_t& uc,
    size_type pos_mb) const
{
    size_type pos_sb = utf8_mb_to_sb(m_string.c_str(), pos_mb, m_string.size());
    return utf8_sb_to_mb(m_string.c_str(),
                             m_string.find(uc.c_str(), pos_sb),
                             m_string.size());
}

//------------------------------------------------------------------------------------------------
utf8string::size_type
utf8string::rfind(
    const utf8string& str_utf8,
    size_type pos_mb) const
{
    size_type pos_sb = utf8_mb_to_sb(m_string.c_str(), pos_mb, m_string.size());
    size_type found_sb = m_string.rfind(str_utf8.m_string, pos_sb);
    size_type found_mb = utf8_sb_to_mb(m_string.c_str(), found_sb, m_string.size());

    return found_mb;
}

//------------------------------------------------------------------------------------------------
utf8string::size_type
utf8string::rfind(
    const char* str_sz,
    size_type pos_mb,
    size_type count_mb) const
{
    EE_ASSERT(str_sz);

    size_type pos_sb = utf8_mb_to_sb(m_string.c_str(), pos_mb, m_string.size());
    size_type count_sb = utf8_mb_to_sb(str_sz, count_mb, npos);
    size_type found_sb = m_string.rfind(str_sz, pos_sb, count_sb);
    size_type found_mb = utf8_sb_to_mb(m_string.c_str(), found_sb, m_string.size());

    return found_mb;
}

//------------------------------------------------------------------------------------------------
utf8string::size_type
utf8string::rfind(const char* str_sz, size_type pos_mb) const
{
    EE_ASSERT(str_sz);

    size_type pos_sb = utf8_mb_to_sb(m_string.c_str(), pos_mb, m_string.size());
    size_type found_sb = m_string.rfind(str_sz, pos_sb);
    size_type found_mb = utf8_sb_to_mb(m_string.c_str(), found_sb, m_string.size());

    return found_mb;
}

//------------------------------------------------------------------------------------------------
utf8string::size_type
utf8string::rfind(const utf8char_t& uc, size_type pos_mb) const
{
    return rfind(uc.c_str(), pos_mb);
}

//------------------------------------------------------------------------------------------------
//  Find Helper Functions
//------------------------------------------------------------------------------------------------
bool _find_match(
    const char *szSubstr,
    const utf8string& match_utf8,
    bool invert)
{
    bool found = false;
    utf8char_t uc = utf8char_t::from_stream(szSubstr);

    utf8string::const_iterator i;
    for (i = match_utf8.begin(); i != match_utf8.end() && !found; ++i)
    {
        if (uc == (*i))
        {
            found = true;
        }
    }

    //  The "invert" flag is set for the "not_of" family of find functions.
    return (found ^ invert);
}

//------------------------------------------------------------------------------------------------
utf8string::size_type utf8string::_find_first_of(
    const utf8string& match_utf8,
    utf8string::size_type pos_mb,
    utf8string::size_type count_mb,
    bool invert) const
{
    if (pos_mb == utf8string::npos)
    {
        return utf8string::npos;
    }

    utf8string::size_type pos_sb = utf8string::utf8_mb_to_sb(data(), pos_mb, size());

    if (pos_sb == utf8string::npos)
    {
        return utf8string::npos;
    }

    const char* ptr = data() + pos_sb;
    const char* ptrEnd = data() + size();

    utf8string::size_type visited_mb = 0;
    bool found = false;

    while (ptr <= ptrEnd && visited_mb < count_mb)
    {
        if (_find_match(ptr, match_utf8, invert))
        {
            found = true;
            break;
        }

        //  Scan to the next character.
        ptr += utf8string::utf8_mb_to_sb(ptr, 1, ptrEnd - ptr);
        ++visited_mb;
    }

    return (found) ? (visited_mb + pos_mb) : (utf8string::npos);
}

//------------------------------------------------------------------------------------------------
utf8string::size_type utf8string::_find_last_of(
    const utf8string& match_utf8,
    utf8string::size_type pos_mb,
    utf8string::size_type count_mb,
    bool invert) const
{
    utf8string::size_type pos_sb;
    if (pos_mb != utf8string::npos)
    {
        pos_sb = utf8string::utf8_mb_to_sb(data(), pos_mb, size());
    }
    else
    {
        pos_mb = length();
        pos_sb = size();
    }

    const char* ptr = data() + pos_sb;
    const char* ptrHead = data();

    utf8string::size_type visited_mb = 0;
    bool found = false;

    // size_type is unsigned so npos is the maximum possible value:
    EE_ASSERT(utf8string::npos >= count_mb);

    while (ptr > ptrHead && visited_mb < count_mb)
    {
        //  Scan backwards to the previous character.
        while (ptr > ptrHead)
        {
            --ptr;

            if ((*ptr & '\xC0') != '\x80')
            {
                break;
            }
        }

        ++visited_mb;

        if (_find_match(ptr, match_utf8, invert))
        {
            found = true;
            break;
        }
    }

    return (found) ? (pos_mb - visited_mb) : (utf8string::npos);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::append(
    size_type count_mb,
    const utf8char_t& uc)
{
    size_type currlen = m_string.size();
    m_string.reserve(currlen + count_mb * uc.size());

    if (uc.size() == 1)
    {
        m_string.append(count_mb, uc.bytes[0]);
    }
    else
    {
        while (count_mb--)
        {
            m_string.append(uc.c_str());
        }
    }

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::insert(
    size_type pos_mb,
    const utf8string& src_utf8)
{
    size_type pos_sb = utf8_mb_to_sb(m_string.data(), pos_mb, m_string.size());
    m_string.insert(pos_sb, src_utf8.m_string);

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::insert(
    size_type pos_mb,
    const char* src_sz)
{
    size_type pos_sb = utf8_mb_to_sb(m_string.data(), pos_mb, m_string.size());
    m_string.insert(pos_sb, src_sz);
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::insert(
    size_type pos_mb,
    const utf8string& src_utf8,
    size_type pos_mb2,
    size_type count_mb)
{
    m_string.insert(
        utf8_mb_to_sb(m_string.data(), pos_mb, m_string.size()),
        src_utf8.substr(pos_mb2, count_mb).raw());

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::insert(
    size_type pos_mb,
    const char* src_sz,
    size_type count_mb)
{
    size_type pos_sb = utf8_mb_to_sb(m_string.data(), pos_mb, m_string.size());
    if (npos == pos_sb)
    {
        return append(src_sz, count_mb);
    }

    size_type count_sb = utf8_mb_to_sb(src_sz, count_mb, npos);
    if (npos == count_sb)
    {
        m_string.insert(pos_sb, src_sz);
    }
    else
    {
        m_string.insert(pos_sb, src_sz, count_sb);
    }

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::insert(
    size_type pos_mb,
    size_type count_mb,
    const utf8char_t& uc)
{
    (*this).insert(pos_mb, utf8string(count_mb, uc));
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string::iterator utf8string::insert(
    utf8string::iterator it,
    const utf8char_t& uc)
{
    const size_type pos_sb = it.base() - m_string.begin();
    EE_ASSERT((signed)pos_sb >= 0);

    m_string.insert(pos_sb, uc.c_str());

    return utf8string::iterator(m_string.begin() + pos_sb);
}

//------------------------------------------------------------------------------------------------
void utf8string::insert(
    utf8string::iterator itPos,
    size_type count_mb,
    const utf8char_t& uc)
{
    const size_type pos_sb = itPos.base() - m_string.begin();
    EE_ASSERT((signed)pos_sb >= 0);

    m_string.insert(pos_sb, utf8string(count_mb, uc).data());
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::insert(
    size_type pos_mb,
    const utf8string& src_utf8,
    size_type count_mb)
{
    m_string.insert(
        utf8_mb_to_sb(m_string.data(), pos_mb, m_string.size()),
        src_utf8.m_string,
        0,
        utf8_mb_to_sb(src_utf8.data(), count_mb, src_utf8.size()));

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::replace(
    size_type pos_mb,
    size_type count_mb,
    const utf8string& src_utf8)
{
    size_type pos_sb = utf8_mb_to_sb(m_string.data(), pos_mb, m_string.size());
    if (npos != pos_sb)
    {
        size_type count_sb = utf8_mb_to_sb(m_string.data() + pos_sb, count_mb,
            m_string.size()-pos_sb);
        m_string.replace(pos_sb, count_sb, src_utf8.m_string);
    }
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::replace(
    size_type pos_mb,
    size_type count_mb,
    const utf8string& src_utf8,
    size_type pos_mb2,
    size_type count_mb2)
{
    (*this).replace(pos_mb, count_mb, src_utf8.substr(pos_mb2, count_mb2));
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::replace(
    size_type pos_mb,
    size_type count_mb,
    const utf8string& src_utf8,
    size_type count_mb2)
{
    (*this).replace(pos_mb, count_mb, src_utf8.substr(0, count_mb2));
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::replace(
    size_type pos_mb,
    size_type count_mb,
    const char* src_sz)
{
    (*this).replace(pos_mb, count_mb, utf8string(src_sz));
    return (*this);
}


//------------------------------------------------------------------------------------------------
utf8string& utf8string::replace(
    size_type pos_mb,
    size_type count_mb,
    size_type count_mb2,
    const utf8char_t& uc)
{
    (*this).replace(pos_mb, count_mb, utf8string(count_mb2, uc));
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::replace(
    utf8string::iterator itBegin,
    utf8string::iterator itEnd,
    const utf8string& src_utf8)
{
    m_string.replace(itBegin.base(), itEnd.base(), src_utf8.raw());

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::replace(
    utf8string::iterator itBegin,
    utf8string::iterator itEnd,
    const char* src_sz,
    size_type count_mb)
{
    m_string.replace(
        itBegin.base(),
        itEnd.base(),
        src_sz,
        utf8_mb_to_sb(src_sz, count_mb, npos));

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::replace(
    utf8string::iterator itBegin,
    utf8string::iterator itEnd,
    const char* src_sz)
{
    m_string.replace(itBegin.base(), itEnd.base(), src_sz);
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::replace(
    utf8string::iterator itBegin,
    utf8string::iterator itEnd,
    size_type count_mb,
    const utf8char_t& uc)
{
    m_string.replace(
        itBegin.base(),
        itEnd.base(),
        utf8string(count_mb, uc).raw());

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::replace(
    utf8string::iterator itBegin,
    utf8string::iterator itEnd,
    const utf8string& src_utf8,
    size_type count_mb)
{
    m_string.replace(
        itBegin.base(),
        itEnd.base(),
        src_utf8.data(),
        utf8_mb_to_sb(src_utf8.data(), count_mb, src_utf8.size()));

    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string& utf8string::erase(
    size_type pos_mb,
    size_type count_mb)
{
    size_type pos_sb = utf8_mb_to_sb(m_string.data(), pos_mb, m_string.size());
    if (npos != pos_sb)
    {
        size_type count_sb = utf8_mb_to_sb(m_string.data() + pos_sb,
                                              count_mb,
                                              m_string.size() - pos_sb);
        m_string.erase(pos_sb, count_sb);
    }
    return (*this);
}

//------------------------------------------------------------------------------------------------
utf8string::size_type utf8string::length() const
{
    size_type len = 0;

    for (const_iterator i = begin(); i != end(); ++i)
    {
        ++len;
    }

    return len;
}

//------------------------------------------------------------------------------------------------
void utf8string::resize(size_type count_sb)
{
    size_type oldCount_sb = m_string.size();
    m_string.resize(count_sb);

    if (count_sb < oldCount_sb)
    {
        //  Traverse from the end and ensure the last UTF8 character is
        //  valid, and erase it if it was severed in the resizing.

        unsigned fragment_sb = utf8_tail_fragments(m_string.data(), (unsigned)count_sb);

        m_string.erase(count_sb - fragment_sb, npos);
    }
}

//------------------------------------------------------------------------------------------------
void utf8string::resize(
    size_type count_sb,
    const utf8char_t& uc)
{
    size_type oldCount_sb = m_string.size();

    if (count_sb >= oldCount_sb)
    {
        m_string.reserve(count_sb);
        size_type mbPadding =
            (count_sb - oldCount_sb) / uc.size();
        (*this).append(mbPadding, uc);
    }
    else
    {
        m_string.resize(count_sb);

        //  Traverse from the end and ensure the last UTF8 character is
        //  valid, and erase it if it was severed in the resizing.

        unsigned fragment_sb = utf8_tail_fragments(m_string.data(), (unsigned)count_sb);

        m_string.erase(count_sb - fragment_sb, npos);
    }
}

//------------------------------------------------------------------------------------------------
bool utf8string::is_ascii() const
{
    const char* i = m_string.data();
    const char* const j = i + m_string.size();

    while (i != j)
    {
        if ((*i) & 0x80)
        {
            return false;
        }

        ++i;
    }

    return true;
}

//------------------------------------------------------------------------------------------------
int utf8string::sprintf(const char* format_sz, ...)
{
    va_list args;
    va_start(args, format_sz);
    int result = vsprintf(format_sz, args);
    va_end(args);
    return result;
}

//------------------------------------------------------------------------------------------------
int utf8string::sprintf(utf8string format_str, ...)
{
    va_list args;
    va_start(args, format_str);
    int result = vsprintf(format_str.c_str(), args);
    va_end(args);
    return result;
}

//------------------------------------------------------------------------------------------------
int utf8string::sprintf_append(const char* format_sz, ...)
{
    va_list args;
    va_start(args, format_sz);
    int result = vsprintf_append(format_sz, args);
    va_end(args);
    return result;
}

//------------------------------------------------------------------------------------------------
int utf8string::sprintf_append(utf8string format_str, ...)
{
    va_list args;
    va_start(args, format_str);
    int result = vsprintf_append(format_str.c_str(), args);
    va_end(args);
    return result;
}

//------------------------------------------------------------------------------------------------
int utf8string::vsprintf(const char* format_sz, va_list args)
{
    int bytesNeeded = efd::Vscprintf(format_sz, args);
    internal_string temp;
    temp.reserve(bytesNeeded + 1);
    int bytesUsed = ::vsprintf(&temp[0], format_sz, args);
    EE_ASSERT(bytesUsed == bytesNeeded);
    EE_ASSERT((size_type)bytesUsed < temp.capacity());
    temp[bytesUsed] = 0;

    // This has to build the string into a temporary and then allocate and copy to the final
    // simply so that m_string will update its notion of how long it is (as it's impossible to
    // update the size() directly)
    m_string = temp.c_str();

    return bytesUsed;
}

//------------------------------------------------------------------------------------------------
int utf8string::vsprintf_append(const char* format_sz, va_list args)
{
    int bytesNeeded = efd::Vscprintf(format_sz, args);
    internal_string temp;
    temp.reserve(bytesNeeded + 1);
    int bytesUsed = ::vsprintf(&temp[0], format_sz, args);
    EE_ASSERT(bytesUsed == bytesNeeded);
    EE_ASSERT((size_type)bytesUsed < temp.capacity());
    temp[bytesUsed] = 0;

    // This has to build the string into a temporary and then allocate and copy to the final
    // simply so that m_string will update its notion of how long it is (as it's impossible to
    // update the size() directly)
    m_string += temp.c_str();

    return bytesUsed;
}

//------------------------------------------------------------------------------------------------
void utf8string::trim(_Trim t, const utf8string& set)
{
    size_type idxBegin = (t == TrimBack) ? 0 : find_first_not_of(set);
    if (npos == idxBegin)
    {
        clear();
        return;
    }

    size_type idxEnd   = (t == TrimFront) ? npos-1 : find_last_not_of(set);
    ++idxEnd;

    EE_ASSERT(idxEnd >= idxBegin);
    *this = substr(idxBegin, idxEnd - idxBegin);
}

//------------------------------------------------------------------------------------------------
void utf8string::trim(_Trim t, const char* set)
{
    size_type idxBegin = (t == TrimBack) ? 0 : m_string.find_first_not_of(set) ;
    if (npos == idxBegin)
    {
        clear();
        return;
    }

    size_type idxEnd = (t == TrimFront) ? npos-1 : m_string.find_last_not_of(set) ;
    ++idxEnd;

    EE_ASSERT(idxEnd >= idxBegin);
    m_string = m_string.substr(idxBegin, idxEnd - idxBegin);
}

//------------------------------------------------------------------------------------------------
//DT20088 This function only handles utf8 strings that are an English char string.
utf8string& utf8string::toupper()
{
    // ensure string is NULL terminated:
    EE_ASSERT_MESSAGE(m_string[m_string.size()] == '\0',
        ("NULL terminated string required for %s", __FUNCTION__));

    // add 1 to string length to include null terminator
    efd::Strupr(&m_string[0], m_string.size()+1);

    return *this;
}

//------------------------------------------------------------------------------------------------
//DT20088 This function only handles utf8 strings that are an English char string.
utf8string& utf8string::tolower()
{
    // ensure string is NULL terminated:
    EE_ASSERT_MESSAGE(m_string[m_string.size()] == '\0',
        ("NULL terminated string required for %s", __FUNCTION__));

    // add 1 to string length to include null terminator
    efd::Strlwr(&m_string[0], m_string.size()+1);

    return *this;
}

//------------------------------------------------------------------------------------------------
int utf8string::replace_substr(const char* find_sz, const char* replace_sz)
{
    int cReplacements = 0;

    size_type cbFind = strlen(find_sz);
    size_type cbReplace = strlen(replace_sz);

    size_type cbCurrent = 0;
    while (utf8string::npos != (cbCurrent = m_string.find(find_sz, cbCurrent)))
    {
        m_string.replace(cbCurrent, cbFind, replace_sz, cbReplace);
        cbCurrent += cbReplace;
        ++cReplacements;
    }

    return cReplacements;
}

//------------------------------------------------------------------------------------------------
int utf8string::ol_replace_substr(const char* find_sz, const char* replace_sz)
{
    // If the replacement string contains the search string then we would get stuck in an
    // infinite loop if we continued, so we specifically check for this case:
    if (strstr(replace_sz, find_sz))
    {
        return -1;
    }

    int cReplacements = 0;

    size_type cbFind = strlen(find_sz);
    size_type cbReplace = strlen(replace_sz);

    size_type cbCurrent = 0;
    while (utf8string::npos != (cbCurrent = m_string.find(find_sz, cbCurrent)))
    {
        m_string.replace(cbCurrent, cbFind, replace_sz, cbReplace);
        ++cReplacements;
    }

    return cReplacements;
}

//------------------------------------------------------------------------------------------------
int utf8string::icompare(const char* cmp_sz) const
{
    // Note: Our size does NOT include the NULL terminator, but we don't guarantee that there
    // IS a NULL terminator so we can't be 100% sure that size()+1 is valid.  Still, since
    // we are allowed to work on non-terminated strings we must pass in SOME size limit and
    // we can't pass in just size() or we'd consider the strings equal in the case where the
    // other string is longer than we are.  So we simply assume that we're actually NULL
    // terminated and we'll just have to rely on the user not calling this function when that
    // assumption isn't true.
    EE_ASSERT(0 == m_string[m_string.size()]);
    return efd::Strnicmp(m_string.data(), cmp_sz, size()+1);
}

//------------------------------------------------------------------------------------------------
int utf8string::icompare(
    size_type pos_mb,
    size_type count_mb,
    const utf8string& cmp_utf8) const
{
    return utf8string(*this, pos_mb, count_mb).icompare(cmp_utf8);
}

//------------------------------------------------------------------------------------------------
int utf8string::icompare(
    size_type pos_mb,
    size_type count_mb,
    const utf8string& cmp_utf8,
    size_type pos_mb2,
    size_type count_mb2) const
{
    return utf8string(*this, pos_mb, count_mb).icompare(utf8string(cmp_utf8, pos_mb2, count_mb2));
}

//------------------------------------------------------------------------------------------------
int utf8string::icompare(
    size_type pos_mb,
    size_type count_mb,
    const char* cmp_sz,
    size_type count_mb2) const
{
    return utf8string(*this, pos_mb, count_mb).icompare(utf8string(cmp_sz, count_mb2));
}

//------------------------------------------------------------------------------------------------
int utf8string::icompare(
    size_type pos_mb,
    size_type count_mb,
    const char* cmp_sz) const
{
    return utf8string(*this, pos_mb, count_mb).icompare(cmp_sz);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 utf8string::split(
    const utf8string& i_set,
    efd::vector< utf8string >& o_results) const
{
    o_results.clear();

    //DT20090 strtok is not utf8 aware.

    // in order to avoid changing the source string it must be copied since strtok directly
    // munges the string:
    utf8string temp = *this;

    efd::Char* pszContext;
    efd::Char* pszToken = efd::Strtok(const_cast<char*>(temp.c_str()), i_set.c_str(), &pszContext);

    efd::UInt32 i=0;
    while (NULL != pszToken)
    {
        ++i;
        o_results.push_back(pszToken);
        pszToken = efd::Strtok(NULL, i_set.c_str(), &pszContext);
    }

    return i;
}

//------------------------------------------------------------------------------------------------
utf8string::size_type utf8string::utf8_mb_to_sb(
    const char* buffer_sz,
    size_type pos_mb,
    size_type byteLimit)
{
    EE_ASSERT(buffer_sz);

    if (pos_mb == npos)
    {
        return npos;
    }

    // If the passed in byteLimit is npos then we are dealing with a NULL terminated string.
    // Otherwise we know the buffer size and will step over NULL characters.
    bool terminateOnNull = byteLimit == npos;

    const char* szIter = buffer_sz;
    const char* szEnd = terminateOnNull ? (const char*)-1 : szIter + byteLimit;

    while (pos_mb)
    {
        --pos_mb;

        if (terminateOnNull && 0 == szIter[0])
        {
            return npos;
        }

        if (szIter > szEnd)
        {
            return npos;
        }

        unsigned int units = utf8_num_units(szIter);

#if defined(EE_EFD_CONFIG_DEBUG)
        // Lets make sure we really have a valid character.  Otherwise we could easily be
        // stepping beyond our memory allotment which could crash or lead to corruption.
        utf8_validate_char(szIter, units);
#endif

        szIter += units;
    }

    return (szIter - buffer_sz);
}

//------------------------------------------------------------------------------------------------
utf8string::size_type utf8string::utf8_sb_to_mb(
    const char* buffer_sz,
    size_type pos_sb,
    size_type byteLimit)
{
    EE_ASSERT(buffer_sz);

    if (pos_sb == npos)
    {
        return npos;
    }

    if (pos_sb > byteLimit)
    {
        return npos;
    }

    unsigned index = 0;
    size_type counter = 0;

    while (counter < pos_sb)
    {
        unsigned bytes = utf8_num_units(buffer_sz);
        buffer_sz += bytes;
        counter += bytes;
        ++index;
    }

    EE_ASSERT(counter <= byteLimit);

    return index;
}

//------------------------------------------------------------------------------------------------
void utf8string::Serialize(efd::Archive& ar)
{
    // we stream the string plus a terminating NULL character. You could argue this terminating
    // NULL is redundant since we also stream the size, but here I am choosing safety over size.
    efd::UInt32 cch = m_string.length();
    efd::SR_As32Bit_Compressed::Serialize(cch, ar);
    char* psz = (char*)ar.GetBytes(cch + 1);
    if (psz)
    {
        if (ar.IsPacking())
        {
            // The string is a byte stream, so no endianness concerns. Just memcpy the data
            // including the terminating NULL.
            memcpy(psz, m_string.c_str(), cch + 1);
        }
        else
        {
            m_string.assign(psz, cch);
        }

    }
}

//------------------------------------------------------------------------------------------------
} // end namespace efd
