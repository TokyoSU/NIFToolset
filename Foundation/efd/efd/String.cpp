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

#include <efd/String.h>
#include <efd/Utilities.h>
#include <efd/utf8char.h>
#include <efd/EEHelpers.h>
#include <efd/Point2.h>
#include <efd/Point3.h>
#include <efd/Color.h>

//--------------------------------------------------------------------------------------------------
using namespace efd;

//--------------------------------------------------------------------------------------------------
// Constructors / destructors
//--------------------------------------------------------------------------------------------------
String::String()
: m_kHandle(NULL)
{
    // To be safe lets ensure that the StringHeader, StringBody, and StringData classes are sized
    // the same on all platforms:
    EE_COMPILETIME_ASSERT(sizeof(StringHeader) == (sizeof(size_t)*3));
    EE_COMPILETIME_ASSERT(sizeof(StringBody) == 1);
    EE_COMPILETIME_ASSERT(sizeof(StringData) == sizeof(StringHeader) + sizeof(void*));
    // Also ensure that the char data buffer immediately follows the header:
    EE_COMPILETIME_ASSERT(EE_OFFSETOF(StringData, m_data) == sizeof(StringHeader));
}

//--------------------------------------------------------------------------------------------------
String::String(const String& kStr)
{
    m_kHandle = kStr.m_kHandle;
    IncRefCount(m_kHandle);
}

//--------------------------------------------------------------------------------------------------
String::String(const efd::Char* pcStr)
{
    // This call also updates string length
    m_kHandle = AllocateAndCopy(pcStr);
}

//--------------------------------------------------------------------------------------------------
String::String(const efd::Char* pcStr, size_t stStringLength)
{
    m_kHandle = AllocateAndCopy(pcStr, stStringLength);
    CalcLength();
}

//--------------------------------------------------------------------------------------------------
String::String(efd::Char ch)
{
    m_kHandle = Allocate(1);
    efd::Char* pcString = (efd::Char*) m_kHandle;
    pcString[0] = ch;
    pcString[1] = '\0';
    SetLength(m_kHandle, 1);
}

//--------------------------------------------------------------------------------------------------
String::String(const efd::utf8char_t& ch)
{
    // This call also updates string length
    m_kHandle = AllocateAndCopy(ch.c_str());
}

//--------------------------------------------------------------------------------------------------
String::String(size_t stBuffLength)
{
    m_kHandle = Allocate(stBuffLength);
    efd::Char* pcString = (efd::Char*)m_kHandle;
    pcString[0] = '\0';
    SetLength(m_kHandle, 0);
}

//--------------------------------------------------------------------------------------------------
String::String(_Formatted, const efd::Char* format_sz, ...)
{
    va_list args;
    va_start(args, format_sz);

    int bytesNeeded = efd::Vscprintf(format_sz, args);
    m_kHandle = Allocate(bytesNeeded);
    m_kHandle->m_data[0] = '\0';
    Vformat(format_sz, args);
    va_end(args);
    CalcLength();
}

//--------------------------------------------------------------------------------------------------
String::~String()
{
    DecRefCount(m_kHandle);
}

//--------------------------------------------------------------------------------------------------
// Handles methods (internal): allocation / deallocation, reference counting
//--------------------------------------------------------------------------------------------------
String::StringBody* String::Allocate(size_t stCount)
{
    if (stCount == 0)
        stCount = 1;

    size_t stBufferSize = stCount * sizeof(efd::Char) + sizeof(StringData);
    stBufferSize = GetBestBufferSize(stBufferSize);

    StringData* pString = reinterpret_cast<StringData*>(EE_ALLOC(efd::Char, stBufferSize));
    pString->m_cbBufferSize = stBufferSize;
    pString->m_RefCount = 1;
    pString->m_cchStringLength = 0;
    return (StringBody*)pString;
}

//--------------------------------------------------------------------------------------------------
String::StringBody* String::AllocateAndCopy(const char* pcStr, size_t stCount)
{
    if (pcStr == NULL)
        return NULL;

    if (stCount == 0)
        stCount = strlen(pcStr);

    StringBody* pBody = Allocate(stCount);
    if (pBody == NULL)
        return NULL;

    Memcpy(pBody->m_data, pcStr, stCount);
    pBody->m_data[stCount] = '\0';

#ifdef _DEBUG
    EE_ASSERT(strncmp(pBody->m_data, pcStr, stCount) == 0);
#endif

    StringHeader* pString = GetRealBufferStart(pBody);
    pString->m_cchStringLength = stCount;
    return pBody;
}

//--------------------------------------------------------------------------------------------------
String::StringBody* String::AllocateAndCopyHandle(StringBody* pBody)
{
    if (pBody == NULL)
        return NULL;

    size_t stLength = GetLength(pBody);
    size_t stBufferSize = GetAllocationSize(pBody);

    StringData* pString = reinterpret_cast<StringData*>(EE_ALLOC(efd::Char, stBufferSize));
    pString->m_cbBufferSize = stBufferSize;
    pString->m_RefCount = 1;
    pString->m_cchStringLength = stLength;

    Memcpy(pString->m_data, stLength + 1, pBody->m_data, stLength + 1);
    EE_ASSERT(strcmp(pString->m_data, pBody->m_data) == 0);

    return (StringBody*)pString;
}

//--------------------------------------------------------------------------------------------------
void String::Deallocate(StringBody*& io_pBody)
{
    if (io_pBody != NULL)
    {
        StringHeader* pString = GetRealBufferStart(io_pBody);
        EE_FREE(pString);
        io_pBody = NULL;
    }
}

//--------------------------------------------------------------------------------------------------
void String::IncRefCount(StringBody* pBody, efd::Bool bValidate)
{
    if (pBody == NULL)
        return;

#ifdef _DEBUG
    if (bValidate)
        EE_ASSERT(ValidateString(pBody));
#else
    EE_UNUSED_ARG(bValidate);
#endif
    StringHeader* pString = GetRealBufferStart(pBody);
    AtomicIncrement(pString->m_RefCount);
}

//--------------------------------------------------------------------------------------------------
void String::DecRefCount(StringBody*& io_pBody, efd::Bool bValidate)
{
    if (io_pBody == NULL)
        return;

#ifdef _DEBUG
    if (bValidate)
        EE_ASSERT(ValidateString(io_pBody));
#else
    EE_UNUSED_ARG(bValidate);
#endif
    StringHeader* pString = GetRealBufferStart(io_pBody);
    AtomicDecrement(pString->m_RefCount);

    if (pString->m_RefCount == 0)
    {
        Deallocate(io_pBody);
    }

    io_pBody = NULL;
}

//--------------------------------------------------------------------------------------------------
size_t String::GetRefCount(StringBody* pBody, efd::Bool bValidate)
{
    if (pBody == (StringBody*) NULL)
    {
        return 0;
    }
    else
    {
#ifdef _DEBUG
        if (bValidate)
            EE_ASSERT(ValidateString(pBody));
#else
    EE_UNUSED_ARG(bValidate);
#endif
        StringHeader* pString = GetRealBufferStart(pBody);
        return pString->m_RefCount;
    }
}

//--------------------------------------------------------------------------------------------------
void String::Swap(StringBody*& io_pBody, const efd::Char* pcNewValue,
    size_t stLength, efd::Bool bValidate)
{
    if (pcNewValue == NULL)
    {
        DecRefCount(io_pBody, bValidate);
        return;
    }

    if (stLength == 0)
    {
        stLength = strlen(pcNewValue);
    }

    if (io_pBody == NULL)
    {
        io_pBody = AllocateAndCopy(pcNewValue, stLength);
        return;
    }

#ifdef _DEBUG
    if (bValidate)
        EE_ASSERT(ValidateString(io_pBody));
#else
    EE_UNUSED_ARG(bValidate);
#endif

    StringHeader* pString = GetRealBufferStart(io_pBody);
    AtomicDecrement(pString->m_RefCount);

    if (pString->m_RefCount == 0)
    {
        if (pString->m_cbBufferSize >= (stLength + sizeof(StringData)))
        {
            pString->m_RefCount = 1;
        }
        else
        {
            Deallocate(io_pBody);
            io_pBody = Allocate(stLength);
            pString = GetRealBufferStart(io_pBody);
        }

        Memcpy(io_pBody->m_data, pcNewValue, stLength + 1);
        EE_ASSERT(strncmp(io_pBody->m_data, pcNewValue, stLength) == 0);

        if (stLength < strlen(pcNewValue))
        {
            io_pBody->m_data[stLength] = '\0';
        }

        pString->m_cchStringLength = stLength;
    }
    else
    {
        io_pBody = AllocateAndCopy(pcNewValue, stLength);
    }
}

//--------------------------------------------------------------------------------------------------
size_t String::GetAllocationSize(StringBody* pBody, efd::Bool bValidate)
{
    if (pBody == NULL)
    {
        return 0;
    }
    else
    {
#ifdef _DEBUG
        if (bValidate)
            EE_ASSERT(ValidateString(pBody));
#else
        EE_UNUSED_ARG(bValidate);
#endif
        StringHeader* pString = GetRealBufferStart(pBody);
        return pString->m_cbBufferSize;
    }
}

//--------------------------------------------------------------------------------------------------
size_t String::GetBufferSize(StringBody* pBody, efd::Bool bValidate)
{
    if (pBody == NULL)
    {
        return 0;
    }
    else
    {
#ifdef _DEBUG
        if (bValidate)
            EE_ASSERT(ValidateString(pBody));
#else
        EE_UNUSED_ARG(bValidate);
#endif
        StringHeader* pString = GetRealBufferStart(pBody);
        return pString->m_cbBufferSize - sizeof(StringHeader);
    }
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::ValidateString(StringBody* pBody)
{
    if (pBody == NULL)
        return true;

    StringHeader* pString = GetRealBufferStart(pBody);
    size_t stLength = pString->m_cchStringLength;

    if (stLength != strlen(pBody->m_data))
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
size_t String::GetBestBufferSize(size_t stSize)
{
    if (stSize < 32)
        return 32;
    else if (stSize < 64)
        return 64;
    else if (stSize < 128)
        return 128;
    else if (stSize < 256)
        return 256;
    else if (stSize < 512)
        return 512;
    else if (stSize < 1024)
        return 1024;
    else
        return stSize + 1;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::Resize(size_t stDelta)
{
    size_t stNewStringLength = Length() + stDelta;
    if (stNewStringLength + sizeof(efd::Char) > GetBufferSize(m_kHandle))
    {
        StringBody* pNewBody = Allocate(stNewStringLength);
        size_t stEndLoc = Length();

        Memcpy(pNewBody->m_data, stNewStringLength, m_kHandle->m_data, Length());
        pNewBody->m_data[stEndLoc] = '\0';

        DecRefCount(m_kHandle);
        m_kHandle = pNewBody;

        SetLength(m_kHandle, stEndLoc);
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
efd::Char* String::MakeExternalCopy() const
{
    // Use with care, you must deallocate buffer, returned by this method
    size_t uiSize = Length() + 1;
    efd::Char* pcStr = EE_ALLOC(efd::Char, uiSize);

    Strcpy(pcStr, uiSize, GetString(m_kHandle));

    return pcStr;
}

//--------------------------------------------------------------------------------------------------
// String comparison
//--------------------------------------------------------------------------------------------------
int String::Compare(const String& kStr) const
{
    // If handles equal, strings are the same. Fastest compare case.
    if (m_kHandle == kStr.m_kHandle)
        return 0;

    // If one of handles is NULL... (If strcmp handles NULL pointers correctly for all platforms,
    // these lines can be omitted).  A NULL string is considered equal to an empty string.
    else if (kStr.m_kHandle == NULL)
    {
        if (GetString(m_kHandle)[0] == '\0')
            return 0;
        return 1;
    }
    else if (m_kHandle == NULL)
    {
        if (GetString(kStr.m_kHandle)[0] == '\0')
            return 0;
        return -1;
    }

    // It is desirable to compare string lengths (they are obtained fast in single memory read op
    // for String), but we need to find first difference char to return correct value for sort.
    // strcmp does exactly the same thing ;) Use Equals instead of Compare for faster comparison.
    return strcmp(GetString(m_kHandle), GetString(kStr.m_kHandle));
}

//--------------------------------------------------------------------------------------------------
int String::Compare(const efd::Char* pcStr) const
{
    if (pcStr == NULL && m_kHandle == (StringBody*) NULL)
        return 0;

    // If one of handles is NULL... (If strcmp handles NULL pointers correctly for all platforms,
    // these lines can be omitted).  A NULL string is considered equal to an empty string.
    else if (pcStr == NULL)
    {
        if (GetString(m_kHandle)[0] == '\0')
            return 0;
        return 1;
    }
    else if (m_kHandle == NULL)
    {
        if (pcStr[0] == '\0')
            return 0;
        return -1;
    }

    return strcmp(GetString(m_kHandle), pcStr);
}

//--------------------------------------------------------------------------------------------------
int String::CompareNoCase(const efd::Char* pcStr) const
{
    if (pcStr == NULL && m_kHandle == (StringBody*) NULL)
        return 0;

    if (pcStr == NULL)
        return 1;//GetString(m_kHandle)[0];
    else if (m_kHandle == (StringBody*) NULL)
        return -1;//-pcString[0];

    return Stricmp(GetString(m_kHandle), pcStr);
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::Equals(const String& kStr) const
{
    // If handles equal, strings are the same. Fastest compare case.
    if (m_kHandle == kStr.m_kHandle)
        return true;

    // If lengths differs, strings are not equal. Length check eliminates need for NULL check.
    else if (kStr.Length() != Length())
        return false;

    // Strings are empty or handles point to a NULL
    if (!Length())
        return true;

    return strcmp(GetString(m_kHandle), GetString(kStr.m_kHandle)) == 0;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::Equals(const efd::Char* pcStr) const
{
    if (pcStr == NULL)
    {
        return m_kHandle == (StringBody*)NULL;
    }
    // If lengths differs, strings are not equal
    if (strlen(pcStr) != Length())
    {
        return false;
    }
    // Strings are empty or handles point to are NULL
    if (!Length())
    {
        return true;
    }
    return strcmp(GetString(m_kHandle), pcStr) == 0;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::EqualsNoCase(const efd::Char* pcStr) const
{
    if (pcStr == NULL)
    {
        return m_kHandle == (StringBody*)NULL;
    }
    // If lengths differs, strings are not equal
    if (strlen(pcStr) != Length())
    {
        return false;
    }
    // Strings are empty or handles point to are NULL
    if (!Length())
    {
        return true;
    }
    return Stricmp(GetString(m_kHandle), pcStr) == 0;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::ContainsNoCase(const efd::Char* pcStr) const
{
    if (reinterpret_cast<const efd::Char*>(m_kHandle) == pcStr && pcStr != NULL)
    {
        return true;
    }

    // The previous check would return true if BOTH were NULL. Now check
    // if the const efd::Char* is NULL
    if (pcStr == NULL  || m_kHandle == NULL)
    {
        return false;
    }

    const efd::Char* pcMyString = (const efd::Char*) m_kHandle;
    size_t stOtherLength = strlen(pcStr);
    for (size_t uiMyIdx = 0; uiMyIdx < Length(); uiMyIdx++)
    {
        if (Strnicmp(&pcMyString[uiMyIdx], pcStr, stOtherLength) == 0)
            return true;
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
// String searching
//--------------------------------------------------------------------------------------------------
size_t String::Find(efd::Char ch, size_t stStart) const
{
    if (stStart >= Length() || ch == '\0')
        return String::npos;

    efd::Char* pcSubStr = strchr(GetString(m_kHandle) + stStart, ch);

    if (pcSubStr == NULL)
    {
        return String::npos;
    }
    return pcSubStr - GetString(m_kHandle);
}

//--------------------------------------------------------------------------------------------------
size_t String::Find(const efd::Char* pcStr, size_t stStart) const
{
    if (stStart >= Length() || pcStr == NULL || pcStr[0] == '\0')
    {
        return String::npos;
    }

    efd::Char* pcSubStr = strstr(GetString(m_kHandle) + stStart, pcStr);

    if (pcSubStr == NULL)
        return String::npos;
    return pcSubStr - GetString(m_kHandle);
}

//--------------------------------------------------------------------------------------------------
size_t String::FindReverse(efd::Char ch, size_t stStart) const
{
    if (m_kHandle == NULL || ch == '\0' || Length() == 0)
    {
        return String::npos;
    }

    if (stStart >= Length() || stStart == String::npos)
        stStart = Length() - 1;

    const efd::Char* pcValue = GetString(m_kHandle);
    ++stStart;

    while (stStart > 0)
    {
        --stStart;
        if (pcValue[stStart] == ch)
            return stStart;
    }

    return String::npos;
}

//--------------------------------------------------------------------------------------------------
size_t String::FindReverse(const efd::Char* pcStr, size_t stStart) const
{
    if (m_kHandle == NULL || pcStr == NULL || pcStr[0] == '\0')
        return String::npos;

    if (stStart >= Length() || stStart == String::npos)
    {
        stStart = Length() - 1;
    }

    size_t stLength = strlen(pcStr);

    if (stStart + 1 < stLength)
    {
        return String::npos;
    }

    stStart = stStart - stLength + 2;

    while (stStart > 0)
    {
        stStart--;
        if (!strncmp(GetString(m_kHandle) + stStart, pcStr, stLength))
            return stStart;
    }

    return String::npos;
}

//--------------------------------------------------------------------------------------------------
size_t String::FindOneOf(const efd::Char* pcDelimiters, size_t stStart) const
{
    if (pcDelimiters == NULL || pcDelimiters[0] == '\0' || m_kHandle == NULL)
    {
        return String::npos;
    }

    efd::Char* pcSubStr = strpbrk(GetString(m_kHandle) + stStart, pcDelimiters);

    if (pcSubStr == NULL)
        return String::npos;
    return pcSubStr - GetString(m_kHandle);
}

//--------------------------------------------------------------------------------------------------
size_t String::FindOneOfReverse(const efd::Char* pcDelimiters, size_t stStart) const
{
    size_t stLength = Length();
    if (m_kHandle == NULL || stLength == 0 || pcDelimiters == NULL || pcDelimiters[0] == '\0')
        return String::npos;

    if (stStart >= Length() || stStart == String::npos)
        stStart = Length() - 1;

    stStart++;

    while (stStart > 0)
    {
        --stStart;
        if (strchr(pcDelimiters, GetString(m_kHandle)[stStart]))
            return stStart;
    }

    return String::npos;
}

//--------------------------------------------------------------------------------------------------
efd::Char* String::Strtok(efd::Char* pcStr, const efd::Char* pcDelimit, efd::Char** ppcContext)
{
    // support function similar to NiString
#if _MSC_VER >= 1400
    return strtok_s(pcStr, pcDelimit, ppcContext);
#else // #if _MSC_VER >= 1400
    return strtok(pcStr, pcDelimit);
#endif // #if _MSC_VER >= 1400
}

//--------------------------------------------------------------------------------------------------
// String manipulation
//--------------------------------------------------------------------------------------------------
void String::Concatenate(const efd::Char* pcStr)
{
    if (pcStr == NULL)
        return;

    size_t stLen = strlen(pcStr);
    if (stLen > 0)
    {
        StringBody* pBody = m_kHandle;
        efd::Bool bInternalIncrementRefCount = false;

        if (pcStr == GetString(m_kHandle))
        {
            IncRefCount(pBody);
            CopyOnWriteAndResize(stLen, true);
            bInternalIncrementRefCount = true;
        }
        else
        {
            CopyOnWriteAndResize(stLen, false);
        }

        Strcpy(GetString(m_kHandle) + Length(), stLen + 1, pcStr);

        // Update string length
        SetLength(m_kHandle, Length() + stLen);

        if (bInternalIncrementRefCount)
        {
            DecRefCount(pBody);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void String::Concatenate(efd::Char ch)
{
    CopyOnWriteAndResize(1);
    size_t stLength = Length();

    m_kHandle->m_data[stLength] = ch;
    m_kHandle->m_data[stLength + 1] = '\0';

    SetLength(m_kHandle, stLength + 1);
}

//--------------------------------------------------------------------------------------------------
String String::GetSubstring(size_t stBegin, size_t stEnd) const
{
    size_t stLength = Length();

    if (stBegin < stEnd && stBegin < stLength && m_kHandle != NULL)
    {
        if (stEnd > stLength || stEnd == String::npos)
            stEnd = stLength;

        String kString((size_t)(stEnd - stBegin + 2));

        Strncpy(
            kString.m_kHandle->m_data,
            kString.GetBufferSize(kString.m_kHandle) - 1,
            GetString(m_kHandle) + stBegin, stEnd - stBegin);

        kString.m_kHandle->m_data[stEnd - stBegin] = '\0';
        kString.CalcLength();

        return kString;
    }

    return String();
}

//--------------------------------------------------------------------------------------------------
String String::GetSubstring(size_t stBegin, const efd::Char* pcDelimiters) const
{
    if (stBegin < Length() && m_kHandle != NULL && pcDelimiters != NULL)
    {
        efd::Char* pcStr = strpbrk(GetString(m_kHandle) + stBegin, pcDelimiters);
        if (pcStr == NULL)
        {
            if (stBegin < Length())
                return GetSubstring(stBegin, String::npos);
            else
                return (const efd::Char*)NULL;
        }
        else
        {
            return GetSubstring(stBegin, pcStr - GetString(m_kHandle));
        }
    }

    return (const efd::Char*)NULL;
}

//--------------------------------------------------------------------------------------------------
String& String::SetToSubstring(const efd::Char* pcStr, size_t stCount)
{
    if (stCount == 0)
    {
        *this = "";
        return *this;
    }

    size_t stLen = strlen(pcStr);

    if (stCount > stLen)
        stCount = stLen;

    Swap(m_kHandle, pcStr, stCount);
    return *this;
}

//--------------------------------------------------------------------------------------------------
void String::Insert(const efd::Char* pcStr, size_t stBegin)
{
    if (pcStr == NULL)
        return;

    size_t stOriginalLength = Length();
    size_t stInsertLength = strlen(pcStr);
    // Test to make sure the insertion string isn't a substring of me
    {
#ifdef EE_ASSERTS_ARE_ENABLED
        const efd::Char* pcOriginalString = GetString(m_kHandle);
        EE_ASSERT(!(pcStr >= pcOriginalString &&
            pcStr <= pcOriginalString + stOriginalLength));
#endif
    }

    if (stBegin >= stOriginalLength)
    {
        Concatenate(pcStr);
        return;
    }

    size_t stNewSize = stInsertLength + stOriginalLength + 1;

    CopyOnWriteAndResize(stInsertLength);
    efd::Char* pcBuffer = GetString(m_kHandle);

    Memmove(&pcBuffer[stBegin + stInsertLength],
        stNewSize - stBegin - stInsertLength,
        &pcBuffer[stBegin], stOriginalLength - stBegin + 1);
    Memmove(&pcBuffer[stBegin], stNewSize - stBegin,
        pcStr, stInsertLength);

    SetLength(m_kHandle, stOriginalLength + stInsertLength);
}

//--------------------------------------------------------------------------------------------------
String String::Mid(size_t stBegin, size_t stCount) const
{
    // Extracts the middle part of a string
    size_t stEnd = stBegin + stCount;

    if (stEnd > Length() || stCount == String::npos)
        stEnd = Length();

    if (stBegin < stEnd && stBegin < Length() && stEnd <= Length())
    {
        String kString(stEnd - stBegin + 2);

        Strncpy(kString.m_kHandle->m_data,
            kString.GetBufferSize(kString.m_kHandle) - 1,
            GetString(m_kHandle) + stBegin, stEnd - stBegin);

        kString.m_kHandle->m_data[stEnd - stBegin] = '\0';
        SetLength(kString.m_kHandle, stEnd - stBegin);

        return kString;
    }

    return (const efd::Char*)NULL;
}

//--------------------------------------------------------------------------------------------------
size_t String::Replace(const efd::Char* pcWhatToReplace, const efd::Char* pcReplaceWith)
{
    if (pcWhatToReplace == NULL || pcReplaceWith == NULL)
        return 0;

    size_t stWhatToReplaceLength = strlen(pcWhatToReplace);
    size_t stReplaceWithLength = strlen(pcReplaceWith);
    size_t stOccuranceCount = 0;
    size_t stOccuranceCountNoChange = 0;
    size_t stOriginalLength = Length();

    for (size_t st = 0; st < stOriginalLength && st != String::npos;)
    {
        st = Find(pcWhatToReplace, st);
        if (st != String::npos)
        {
            stOccuranceCount++;
            st += stWhatToReplaceLength;
        }
    }

    if (stOccuranceCount == 0)
        return 0;

    size_t uiNumCharsToReplace =
        stOccuranceCount * stWhatToReplaceLength;
    size_t uiNumCharsNeededToReplace =
        stOccuranceCount * stReplaceWithLength;
    size_t uiSizeNeeded =
        stOriginalLength - uiNumCharsToReplace + uiNumCharsNeededToReplace;

    if (uiSizeNeeded > stOriginalLength)
        CopyOnWriteAndResize(uiSizeNeeded - stOriginalLength);
    else
        CopyOnWrite();

    stOccuranceCountNoChange = stOccuranceCount;
    for (size_t st = 0; st < uiSizeNeeded && stOccuranceCount > 0; stOccuranceCount--)
    {
        efd::Char* pcStart = strstr(m_kHandle->m_data + st, pcWhatToReplace);
        EE_ASSERT(pcStart != NULL);
        efd::Char* pcEnd = pcStart + stReplaceWithLength;
        memmove(pcStart + stReplaceWithLength, pcStart + stWhatToReplaceLength,
            strlen(pcStart + stWhatToReplaceLength) + 1);
        Memcpy(pcStart, pcReplaceWith, stReplaceWithLength);
        st = pcEnd - m_kHandle->m_data;
    }

    CalcLength();
    return stOccuranceCountNoChange;
}

//--------------------------------------------------------------------------------------------------
inline size_t String::OverlappingReplace(const efd::Char* pcFindStr, const efd::Char* pcReplaceStr)
{
    // If the replacement string contains the search string then we would get stuck in an
    // infinite loop if we continued, so we specifically check for this case:
    if (strstr(pcReplaceStr, pcFindStr))
    {
        return npos;
    }

    size_t result = 0;
    size_t foundAt = 0;
    size_t findStrLength = strlen(pcFindStr);
    while (npos != (foundAt = Find(pcFindStr, foundAt)))
    {
        ++result;
        RemoveRange(foundAt, findStrLength);
        Insert(pcReplaceStr, foundAt);
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
void String::RemoveRange(size_t stBegin, size_t stCount)
{
    size_t uiEnd = Length();
    if (stBegin >= uiEnd)
        return;

    if (stBegin + stCount > uiEnd)
        stCount = uiEnd - stBegin;

    CopyOnWrite();
    efd::Char* pcBuffer = GetString(m_kHandle);

    Memmove(&pcBuffer[stBegin], uiEnd - stBegin + 1,
        &pcBuffer[stBegin + stCount],
        uiEnd - stBegin - stCount + 1);

    CalcLength();
}

//--------------------------------------------------------------------------------------------------
void String::TrimLeft(efd::Char ch)
{
    if (ch == '\0' || m_kHandle == NULL)
        return;

    size_t stTrimCount = 0;
    size_t stLength = Length();
    efd::Char* pcStr = GetString(m_kHandle);

    while (stTrimCount < stLength && pcStr[stTrimCount] == ch)
        stTrimCount++;

    if (stTrimCount > 0)
    {
        CopyOnWrite();
        // Update string pointer
        pcStr = GetString(m_kHandle);
        memmove(pcStr, pcStr + stTrimCount, Length() - stTrimCount + 1);
        SetLength(m_kHandle, stLength - stTrimCount);
    }
}

//--------------------------------------------------------------------------------------------------
void String::TrimRight(efd::Char ch)
{
    if (ch == '\0' || m_kHandle == NULL)
        return;

    size_t stTrimCount = 0;
    size_t stLength = Length();
    efd::Char* pcStr = GetString(m_kHandle);

    while (stLength > stTrimCount && pcStr[stLength - stTrimCount - 1] == ch)
        stTrimCount++;

    if (stTrimCount > 0)
    {
        CopyOnWrite();
        GetString(m_kHandle)[stLength - stTrimCount] = '\0';
        SetLength(m_kHandle, stLength - stTrimCount);
    }
}

//--------------------------------------------------------------------------------------------------
void String::TrimLeft(const efd::Char* pcStr)
{
    if (pcStr == NULL || pcStr[0] == '\0' || m_kHandle == NULL)
        return;

    const efd::Char* pcOwnStr = GetString(m_kHandle);
    size_t stOwnLength = Length();
    size_t stLength = strlen(pcStr);
    size_t stPos = 0;

    while (stPos + stLength <= stOwnLength && !strncmp(pcOwnStr + stPos, pcStr, stLength))
        stPos += stLength;

    if (stPos > 0)
    {
        RemoveRange(0, stPos);
    }
}

//--------------------------------------------------------------------------------------------------
void String::TrimRight(const efd::Char* pcStr)
{
    if (pcStr == NULL || pcStr[0] == '\0' || m_kHandle == NULL)
        return;

    const efd::Char* pcOwnStr = GetString(m_kHandle);
    size_t stOwnLength = Length();
    size_t stLength = strlen(pcStr);
    size_t stPos = 0;

    while (stPos + stLength <= stOwnLength &&
        !strncmp(pcOwnStr + stOwnLength - stPos - stLength, pcStr, stLength))
    {
        stPos += stLength;
    }

    if (stPos > 0)
    {
        RemoveRange(stOwnLength - stPos - 1, stPos);
    }
}

//--------------------------------------------------------------------------------------------------
void String::TrimSetLeft(const efd::Char* pcDelimiters)
{
    if (pcDelimiters == NULL || m_kHandle == NULL || pcDelimiters[0] == '\0')
        return;

    size_t stTrimCount = 0;
    size_t stLength = Length();
    efd::Char* pcStr = GetString(m_kHandle);

    while (stTrimCount < stLength && strchr(pcDelimiters, pcStr[stTrimCount]))
        stTrimCount++;

    if (stTrimCount > 0)
    {
        CopyOnWrite();
        // Update string pointer
        pcStr = GetString(m_kHandle);
        memmove(pcStr, pcStr + stTrimCount, Length() - stTrimCount + 1);
        SetLength(m_kHandle, stLength - stTrimCount);
    }
}

//--------------------------------------------------------------------------------------------------
void String::TrimSetRight(const efd::Char* pcDelimiters)
{
    if (pcDelimiters == NULL || m_kHandle == NULL || pcDelimiters[0] == '\0')
        return;

    size_t stTrimCount = 0;
    size_t stLength = Length();
    efd::Char* pcStr = GetString(m_kHandle);

    while (stLength > stTrimCount && strchr(pcDelimiters, pcStr[stLength - stTrimCount - 1]))
        stTrimCount++;

    if (stTrimCount > 0)
    {
        CopyOnWrite();
        GetString(m_kHandle)[stLength - stTrimCount] = '\0';
        SetLength(m_kHandle, stLength - stTrimCount);
    }
}

//--------------------------------------------------------------------------------------------------
void String::Reverse()
{
    if (Length() < 2)
        return;

    CopyOnWrite();
    size_t stEnd = Length() - 1;

    efd::Char* pcStr = GetString(m_kHandle);

    for (size_t stBegin = 0; stBegin < stEnd; stBegin++, stEnd--)
    {
        efd::Char ch = pcStr[stBegin];
        pcStr[stBegin] = pcStr[stEnd];
        pcStr[stEnd] = ch;
    }
}

//--------------------------------------------------------------------------------------------------
void String::ToUpper()
{
    if (m_kHandle == NULL)
        return;

    CopyOnWrite();
    size_t stLength = Length();
    efd::Char* pcStr = GetString(m_kHandle);

    for (size_t st = 0; st < stLength ; st++)
    {
        efd::Char ch = pcStr[st];
        if (('a' <= ch) && (ch <= 'z'))
            pcStr[st] -= 'a' - 'A';
    }
}

//--------------------------------------------------------------------------------------------------
void String::ToLower()
{
    if (m_kHandle == NULL)
        return;

    CopyOnWrite();
    size_t stLength = Length();
    efd::Char* pcStr = GetString(m_kHandle);

    for (size_t st = 0; st < stLength ; st++)
    {
        efd::Char ch = pcStr[st];
        if (('A' <= ch) && (ch <= 'Z'))
            pcStr[st] -= 'A' - 'a';
    }
}

//--------------------------------------------------------------------------------------------------
size_t String::Format(const efd::Char* pcFormat, ...)
{
    // Since the format string may include itself, we must be sure to save a
    // copy of the buffer internally.
    StringBody* pBody = m_kHandle;
    IncRefCount(pBody);

    // Make the active copy unique so that we can write into it.
    CopyOnWrite();
    EE_ASSERT(pBody != m_kHandle || m_kHandle == NULL);

    va_list argList;
    va_start(argList, pcFormat);
    m_kHandle = vformat(pcFormat, argList);

    va_end(argList);
    DecRefCount(pBody);

    return Length();
}

//--------------------------------------------------------------------------------------------------
size_t String::Vformat(const efd::Char* pcFormat, va_list argList)
{
    // Since the format string may include itself, we must be sure to save a
    // copy of the buffer internally.
    StringBody* pBody = m_kHandle;
    IncRefCount(pBody);

    // Make the active copy unique so that we can write into it.
    CopyOnWrite();
    EE_ASSERT(pBody != m_kHandle || m_kHandle == NULL);

    m_kHandle = vformat(pcFormat, argList);

    DecRefCount(pBody);

    return Length();
}

//--------------------------------------------------------------------------------------------------
String::StringBody* String::vformat(const char* pcFormat, va_list argPtr)
{
    StringBody* pBody = m_kHandle;
    size_t stBufferSize = GetBufferSize(pBody);
    EE_ASSERT(GetRefCount(pBody) < 2);

    size_t numChars = String::npos;
    if (stBufferSize != 0)
    {
        //DT32423 Switch to using efd Vsnprintf wrapper
#if defined(_MSC_VER) && _MSC_VER >= 1400
        numChars = _vsnprintf_s(pBody->m_data, stBufferSize, stBufferSize - 1,  pcFormat, argPtr);
#elif defined(_PS3) || defined(linux)
        numChars = vsnprintf(pBody->m_data, stBufferSize - 1, pcFormat, argPtr);
#else //#if defined(_MSC_VER) && _MSC_VER >= 1400
        numChars = _vsnprintf(pBody->m_data, stBufferSize - 1, pcFormat, argPtr);
#endif //#if defined(_MSC_VER) && _MSC_VER >= 1400
    }

    while (numChars == String::npos || pBody == NULL || (size_t)numChars >= stBufferSize - 1)
    {
        // We need to throw away the old buffer
        Deallocate(pBody);

        // Allocate the larger buffer
        pBody = Allocate(stBufferSize * 2);
        stBufferSize = GetBufferSize(pBody, false);
        //DT32423 Switch to using efd Vsnprintf wrapper
#if defined(_MSC_VER) && _MSC_VER >= 1400
        numChars = _vsnprintf_s(pBody->m_data, stBufferSize, stBufferSize - 1, pcFormat, argPtr);
#elif defined(_PS3) || defined(linux)
        // _vsnprintf returns the number of characters actually written
        // but... vsnprintf returns the number of characters it wanted
        // to write, not the number actually written!!! (bug 4737)
        numChars = vsnprintf(pBody->m_data, stBufferSize - 1, pcFormat, argPtr);
        if (numChars >= stBufferSize - 1)
            numChars = String::npos;
#else //#if defined(_MSC_VER) && _MSC_VER >= 1400
        numChars = _vsnprintf(pBody->m_data, stBufferSize - 1, pcFormat, argPtr);
#endif //#if defined(_MSC_VER) && _MSC_VER >= 1400
    }

    pBody->m_data[numChars] = '\0';
    SetLength(pBody, numChars);
    return pBody;
}

//--------------------------------------------------------------------------------------------------
// Utf8 methods
//--------------------------------------------------------------------------------------------------
size_t String::GetLengthUtf8()
{
    const efd::Char* pcStr = GetString(m_kHandle);
    size_t stPos = 0;

    while (pcStr[stPos])
        stPos += GetCharLengthUtf8(pcStr + stPos);

    return stPos;
}

//--------------------------------------------------------------------------------------------------
size_t String::ConvertUtf8Pos(size_t stSymIdx) const
{
    size_t stLength = Length();
    const efd::Char* pcStr = GetString(m_kHandle);

    size_t stPos = 0;
    while (stPos < stLength)
    {
        if (stPos == stSymIdx)
            return stPos;

        stPos += efd::utf8_num_units(pcStr + stPos);
    }

    return String::npos;
}

//--------------------------------------------------------------------------------------------------
size_t String::GetCharLengthUtf8(size_t stBytePos) const
{
    if (m_kHandle == NULL || stBytePos >= Length())
        return 0;

    return efd::utf8_num_units(GetString(m_kHandle) + stBytePos);
}

//--------------------------------------------------------------------------------------------------
size_t String::GetCharLengthUtf8(const efd::Char* pcStr) const
{
    if (pcStr == NULL)
        return 0;

    return efd::utf8_num_units(pcStr);
}

//--------------------------------------------------------------------------------------------------
efd::utf8char_t String::GetAtUtf8(size_t stPos) const
{
    stPos = ConvertUtf8Pos(stPos);
    if (stPos == String::npos)
        return efd::utf8char_t('\0');

    return efd::utf8char_t::from_stream(GetString(m_kHandle) + stPos);
}

//--------------------------------------------------------------------------------------------------
void String::SetAtUtf8(size_t stPos, efd::utf8char_t uc)
{
    stPos = ConvertUtf8Pos(stPos);
    if (stPos == String::npos)
        return;

    const efd::Char* pcStr = GetString(m_kHandle);

    RemoveRange(stPos, GetCharLengthUtf8(pcStr + stPos));
    Insert(uc.c_str(), stPos);
}

//--------------------------------------------------------------------------------------------------
// Conversion to / from other types
//--------------------------------------------------------------------------------------------------
efd::Bool String::ToBool(efd::Bool& b) const
{
    if (ContainsNoCase("true"))
    {
        b = true;
        return true;
    }
    else if (ContainsNoCase("false"))
    {
        b = false;
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::ToFloat64(efd::Float64& f) const
{
    efd::Char* pcStr = GetString(m_kHandle);

    if (pcStr == NULL)
        return false;

    f = (efd::Float64)atof(pcStr);

    if (f == 0.0 && FindOneOf("-0") == String::npos)
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::ToFloat32(efd::Float32& f) const
{
    efd::Float64 f64;

    if (!ToFloat64(f64))
        return false;

    f = (efd::Float32)f64;
    return true;
}

//--------------------------------------------------------------------------------------------------
/*efd::Bool String::ToInt64(efd::SInt64 & i) const
{
    efd::Char* pcStr = GetString(m_kHandle);

    if (pcStr == NULL)
        return false;

    i = _atoi64(pcStr);

    if (i == 0 && FindOneOf(pcStr, "-0") == String::npos)
        return false;

    return true;
}*/

//--------------------------------------------------------------------------------------------------
efd::Bool String::ToInt32(efd::SInt32& i) const
{
    efd::Char* pcStr = GetString(m_kHandle);

    if (pcStr == NULL)
        return false;

    i = atoi(pcStr);

    if (i == 0 && FindOneOf("-0") == String::npos)
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::ToInt16(efd::SInt16& i) const
{
    efd::SInt32 i32;

    if (!ToInt32(i32))
        return false;

    if (i32 < -(EE_SINT16_MAX+1))
        i32 = -(EE_SINT16_MAX+1);
    if (i32 > EE_SINT16_MAX)
        i32 = EE_SINT16_MAX;

    i = (efd::SInt16)i32;

    return true;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::ToInt8(efd::SInt8& i) const
{
    efd::SInt32 i32;

    if (!ToInt32(i32))
        return false;

    if (i32 < -(EE_SINT8_MAX+1))
        i32 = -(EE_SINT8_MAX+1);
    if (i32 > EE_SINT8_MAX)
        i32 = EE_SINT8_MAX;

    i = (efd::SInt8)i32;

    return true;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::ToPoint2(Point2& kPoint)
{
    const efd::Char acSeps [] = " \t(,)";
    efd::Char* pcContext;
    StringBody* pBody = AllocateAndCopyHandle(m_kHandle);
    String kString(Strtok(GetString(pBody), acSeps, &pcContext));
    if (!kString.ToFloat32(kPoint.x))
    {
        Deallocate(pBody);
        return false;
    }

    String kString2(Strtok(NULL, acSeps, &pcContext));
    if (!kString2.ToFloat32(kPoint.y))
    {
        Deallocate(pBody);
        return false;
    }

    Deallocate(pBody);
    return true;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::ToPoint3(Point3& kPoint)
{
    const efd::Char acSeps [] = " \t(,)";
    efd::Char* pcContext;
    StringBody* pBody = AllocateAndCopyHandle(m_kHandle);
    String kString(Strtok(GetString(pBody), acSeps, &pcContext));
    if (!kString.ToFloat32(kPoint.x))
    {
        Deallocate(pBody);
        return false;
    }

    String kString2(Strtok(NULL, acSeps, &pcContext));
    if (!kString2.ToFloat32(kPoint.y))
    {
        Deallocate(pBody);
        return false;
    }

    String kString3(Strtok(NULL, acSeps, &pcContext));
    if (!kString3.ToFloat32(kPoint.z))
    {
        Deallocate(pBody);
        return false;
    }

    Deallocate(pBody);
    return true;
}

//--------------------------------------------------------------------------------------------------
/*efd::Bool ToPoint4(Point4& kPoint)
{
    return false;
}*/

//--------------------------------------------------------------------------------------------------
efd::Bool String::ToRGB(Color& kColor)
{
    const efd::Char acSeps [] = " \t(,)";
    efd::Char* pcContext;
    StringBody* pBody = AllocateAndCopyHandle(m_kHandle);
    String kColorString(Strtok(GetString(pBody), acSeps, &pcContext));
    if (!kColorString.Equals("#RGB"))
    {
        Deallocate(pBody);
        return false;
    }

    String kString(Strtok(NULL, acSeps, &pcContext));
    if (!kString.ToFloat32(kColor.m_r))
    {
        Deallocate(pBody);
        return false;
    }

    if (kColor.m_r < 0.0f || kColor.m_r > 1.0f)
    {
        Deallocate(pBody);
        return false;
    }

    String kString2(Strtok(NULL, acSeps, &pcContext));
    if (!kString2.ToFloat32(kColor.m_g))
    {
        Deallocate(pBody);
        return false;
    }

    if (kColor.m_g < 0.0f || kColor.m_g > 1.0f)
    {
        Deallocate(pBody);
        return false;
    }

    String kString3(Strtok(NULL, acSeps, &pcContext));
    if (!kString3.ToFloat32(kColor.m_b))
    {
        Deallocate(pBody);
        return false;
    }

    if (kColor.m_b < 0.0f || kColor.m_b > 1.0f)
    {
        Deallocate(pBody);
        return false;
    }

    Deallocate(pBody);
    return true;
}

//--------------------------------------------------------------------------------------------------
efd::Bool String::ToRGBA(ColorA& kColor)
{
    const efd::Char acSeps [] = " \t(,)";
    StringBody* pBody = AllocateAndCopyHandle(m_kHandle);
    efd::Char* pcContext;

    String kColorString(Strtok(GetString(pBody), acSeps, &pcContext));
    if (!kColorString.Equals("#RGBA"))
    {
        Deallocate(pBody);
        return false;
    }

    String kString(Strtok(NULL, acSeps, &pcContext));
    if (!kString.ToFloat32(kColor.m_r))
    {
        Deallocate(pBody);
        return false;
    }

    if (kColor.m_r < 0.0f || kColor.m_r > 1.0f)
    {
        Deallocate(pBody);
        return false;
    }
    String kString2(Strtok(NULL, acSeps, &pcContext));
    if (!kString2.ToFloat32(kColor.m_g))
    {
        Deallocate(pBody);
        return false;
    }

    if (kColor.m_g < 0.0f || kColor.m_g > 1.0f)
    {
        Deallocate(pBody);
        return false;
    }

    String kString3(Strtok(NULL, acSeps, &pcContext));
    if (!kString3.ToFloat32(kColor.m_b))
    {
        Deallocate(pBody);
        return false;
    }

    if (kColor.m_b < 0.0f || kColor.m_b > 1.0f)
    {
        Deallocate(pBody);
        return false;
    }

    String kString4(Strtok(NULL, acSeps, &pcContext));
    if (!kString4.ToFloat32(kColor.m_a))
    {
        Deallocate(pBody);
        return false;
    }

    if (kColor.m_a < 0.0f || kColor.m_a > 1.0f)
    {
        Deallocate(pBody);
        return false;
    }

    Deallocate(pBody);
    return true;
}

//--------------------------------------------------------------------------------------------------
String String::FromBool(efd::Bool b)
{
    if (b)
        return String("True");
    else
        return String("False");
}

//--------------------------------------------------------------------------------------------------
String String::FromFloat64(efd::Float64 f)
{
    efd::Char acString[50];
    Sprintf(acString, 50, "%10.17f", f);
    String kString(acString);
    kString.Replace(",", ".");
    return kString;
}

//--------------------------------------------------------------------------------------------------
String String::FromFloat32(efd::Float32 f)
{
    efd::Char acString[50];
    Sprintf(acString, 50, "%f", f);
    String kString(acString);
    kString.Replace(",", ".");
    return kString;
}

//--------------------------------------------------------------------------------------------------
/*String String::FromInt64(efd::SInt64 i)
{
    efd::Char acString[32];
    Sprintf(acString, 32, "%d", i);
    return String(acString);
}*/

//--------------------------------------------------------------------------------------------------
String String::FromInt32(efd::SInt32 i)
{
    efd::Char acString[32];
    Sprintf(acString, 32, "%d", i);
    return String(acString);
}

//--------------------------------------------------------------------------------------------------
String String::FromInt16(efd::SInt16 i)
{
    efd::Char acString[8];
    Sprintf(acString, 16, "%d", i);
    return String(acString);
}

//--------------------------------------------------------------------------------------------------
String String::FromInt8(efd::SInt8 i)
{
    efd::Char acString[8];
    Sprintf(acString, 8, "%d", i);
    return String(acString);
}

//--------------------------------------------------------------------------------------------------
String String::FromPoint2(Point2& kPt)
{
    efd::Char acString[96];
    Sprintf(acString, 96, "(%f, %f)", kPt.x, kPt.y);
    String kString(acString);
    return kString;
}

//--------------------------------------------------------------------------------------------------
String String::FromPoint3(Point3& kPt)
{
    efd::Char acString[128];
    Sprintf(acString, 128, "(%f, %f, %f)", kPt.x, kPt.y, kPt.z);
    String kString(acString);
    return kString;
}

//--------------------------------------------------------------------------------------------------
/*String String::FromPoint4(Point4& kPt)
{
    efd::Char acString[96];
    Sprintf(acString, 96, "(%f, %f, %f, %f)", kPt.x, kPt.y, kPt.z, kPt.w);
    String kString(acString);
    return kString;
}*/

//--------------------------------------------------------------------------------------------------
String String::FromRGB(Color& kColor)
{
    efd::Char acString[128];
    Sprintf(acString, 128, "#RGB(%f, %f, %f)", kColor.m_r, kColor.m_g, kColor.m_b);
    String kString(acString);
    return kString;
}

//--------------------------------------------------------------------------------------------------
String String::FromRGBA(ColorA& kColor)
{
    efd::Char acString[160];
    Sprintf(acString, 160, "#RGBA(%f, %f, %f, %f)", kColor.m_r, kColor.m_g, kColor.m_b, kColor.m_a);
    String kString(acString);
    return kString;
}

//--------------------------------------------------------------------------------------------------
// utf8string compatibility methods
//--------------------------------------------------------------------------------------------------
size_t String::split(const efd::Char* pcDelimiters, efd::vector<String>& kResults) const
{
    if (m_kHandle == NULL)
        return 0;

    size_t stCount = 0;
    kResults.clear();

    size_t stLength = Length();
    for (size_t st = 0; st < stLength; st++)
    {
        if (strchr(pcDelimiters, GetString(m_kHandle)[st]))
            continue;

        String kPartStr = GetSubstring(st, pcDelimiters);
        kResults.push_back(kPartStr);
        st = st + kPartStr.Length();
        stCount++;
    }

    return stCount;
}

//--------------------------------------------------------------------------------------------------
const String& String::WhiteSpace()
{
    static String ws(" \t\r\n");
    return ws;
}

//--------------------------------------------------------------------------------------------------
const String& String::NullString()
{
    static String ns;
    return ns;
}

//--------------------------------------------------------------------------------------------------
size_t String::sprintf(const efd::Char* pcFormat, ...)
{
    va_list argList;
    va_start(argList, pcFormat);
    size_t stWritten = Vformat(pcFormat, argList);
    va_end(argList);
    return stWritten;
}

//--------------------------------------------------------------------------------------------------
size_t String::vsprintf(const efd::Char* pcFormat, va_list argList)
{
    return Vformat(pcFormat, argList);
}

//--------------------------------------------------------------------------------------------------
size_t String::sprintf_append(const efd::Char* pcFormat, ...)
{
    String kTempStr;
    va_list argList;
    va_start(argList, pcFormat);
    size_t stWritten = kTempStr.Vformat(pcFormat, argList);
    Concatenate(kTempStr);
    va_end(argList);
    return stWritten;
}

//--------------------------------------------------------------------------------------------------
size_t String::vsprintf_append(const efd::Char* pcFormat, va_list argList)
{
    String kTempStr;
    size_t stWritten = kTempStr.Vformat(pcFormat, argList);
    Concatenate(kTempStr);
    return stWritten;
}

//--------------------------------------------------------------------------------------------------
