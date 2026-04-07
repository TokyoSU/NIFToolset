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

#include <efd/FixedString.h>
#include <efd/StaticDataManager.h>

using namespace efd;

//------------------------------------------------------------------------------------------------
static EE_FORCEINLINE void AssertValidAlloc()
{
    // If this assert is hit, then FixedString is being assigned to pre-main (or destroyed
    // post-main). These assignments should be moved into SDM init and shutdown functions.
    //
    // Note: By default, FixedString uses the SmallObjectAllocator and thread local storage.
    // Allocations prior to the TLS being initialized are handled gracefully by Gamebryo by
    // defaulting to the normal allocator.  However, Microsoft AppVerifier will still report this
    // as a failure.
    EE_MEMASSERT(efd::StaticDataManager::IsInitialized() ||
        !"FixedString cannot be initialized pre-main().");
}

//------------------------------------------------------------------------------------------------
static EE_FORCEINLINE void AssertValidDealloc()
{
    // See above comment in AssertValidAlloc() for details.
    EE_MEMASSERT(efd::StaticDataManager::IsInitialized() ||
        !"Static FixedString objects must be explicitly cleared prior to SDM shutdown.");
}

//------------------------------------------------------------------------------------------------
FixedString::FixedString()
{
    m_handle = GlobalStringTable::NULL_STRING;
}

//------------------------------------------------------------------------------------------------
FixedString::FixedString(const efd::Char* pString)
{
    if (pString == nullptr)
    {
        m_handle = GlobalStringTable::NULL_STRING;
    }
    else
    {
        AssertValidAlloc();
        m_handle = GlobalStringTable::AddString(pString);
    }
}

//------------------------------------------------------------------------------------------------
FixedString::FixedString(const FixedString& string)
{
    GlobalStringTable::IncRefCount((GlobalStringTable::GlobalStringHandle&)string.m_handle);
    m_handle = string.m_handle;
}

//------------------------------------------------------------------------------------------------
FixedString::~FixedString()
{
    if (m_handle)
        AssertValidDealloc();
    GlobalStringTable::DecRefCount(m_handle);
}

//------------------------------------------------------------------------------------------------
FixedString::operator const efd::Char*() const
{
    return GlobalStringTable::GetString(m_handle);
}

//------------------------------------------------------------------------------------------------
FixedString& FixedString::operator=(const FixedString& string)
{
    if (m_handle != string.m_handle)
    {
        GlobalStringTable::GlobalStringHandle handle = string.m_handle;
        GlobalStringTable::IncRefCount(handle);
        GlobalStringTable::DecRefCount(m_handle);
        m_handle = handle;
    }
    return *this;
}

//------------------------------------------------------------------------------------------------
FixedString& FixedString::operator=(const efd::Char* pString)
{
    if (m_handle != pString)
    {
        AssertValidAlloc();
        GlobalStringTable::GlobalStringHandle handle = m_handle;
        m_handle = GlobalStringTable::AddString(pString);
        GlobalStringTable::DecRefCount(handle);
    }
    return *this;
}

//------------------------------------------------------------------------------------------------
bool FixedString::Exists() const
{
    return m_handle != GlobalStringTable::NULL_STRING;
}

//------------------------------------------------------------------------------------------------
size_t FixedString::GetLength() const
{
    return GlobalStringTable::GetLength(m_handle);
}

//------------------------------------------------------------------------------------------------
efd::UInt32 FixedString::GetRefCount() const
{
    return (efd::UInt32)GlobalStringTable::GetRefCount(m_handle);
}

//------------------------------------------------------------------------------------------------
bool FixedString::Equals(const efd::Char* pCStr) const
{
    // Handle NULL checks and that this points to the exact same location in memory.
    if (m_handle == pCStr)
        return true;

    // The previous check would return true if BOTH were NULL. Now check if the const efd::Char*
    // is NULL
    if (pCStr == nullptr  || m_handle == nullptr)
        return false;

    return strcmp((const efd::Char*) m_handle, pCStr) == 0;
}

//------------------------------------------------------------------------------------------------
bool FixedString::EqualsNoCase(const efd::Char* pCStr) const
{
    // Handle NULL checks and that this points to the exact same location in memory.
    if (m_handle == pCStr)
        return true;

    // The previous check would return true if BOTH were NULL. Now check if the const efd::Char*
    // is NULL
    if (pCStr == nullptr || m_handle == nullptr)
        return false;

    return efd::Stricmp((const efd::Char*) m_handle, pCStr) == 0;
}

//------------------------------------------------------------------------------------------------
bool FixedString::Contains(const efd::Char* pCStr) const
{
    if (m_handle == pCStr && pCStr != nullptr)
        return true;

    // The previous check would return true if BOTH were NULL. Now check if the const efd::Char*
    // is NULL
    if (pCStr == nullptr  || m_handle == nullptr || pCStr[0] == '\0' ||
        ((const efd::Char*)m_handle)[0] == '\0')
    {
        return false;
    }

    return strstr((const efd::Char*) m_handle, pCStr) != nullptr;
}

//------------------------------------------------------------------------------------------------
bool FixedString::ContainsNoCase(const efd::Char* pCStr) const
{
    if (m_handle == pCStr && pCStr != nullptr)
        return true;

    // The previous check would return true if BOTH were NULL. Now check if the const efd::Char*
    // is NULL
    if (pCStr == nullptr  || m_handle == nullptr || pCStr[0] == '\0' ||
        ((const efd::Char*)m_handle)[0] == '\0')
    {
        return false;
    }

    const efd::Char* pcMyString = (const efd::Char*) m_handle;
    size_t stOtherLength = strlen(pCStr);
    for (efd::UInt32 uiMyIdx = 0; uiMyIdx < GetLength(); uiMyIdx++)
    {
        if (efd::Strnicmp(&pcMyString[uiMyIdx], pCStr, stOtherLength) == 0)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool efd::operator==(const FixedString& s1, const FixedString& s2)
{
    return s1.m_handle == s2.m_handle;
}

//------------------------------------------------------------------------------------------------
bool efd::operator!=(const FixedString& s1, const FixedString& s2)
{
    return s1.m_handle != s2.m_handle;
}

//------------------------------------------------------------------------------------------------
bool efd::operator==(const FixedString& s1, const efd::Char* s2)
{
    return s1.Equals(s2);
}

//------------------------------------------------------------------------------------------------
bool efd::operator!=(const FixedString& s1, const efd::Char* s2)
{
    return !(s1.Equals(s2));
}

//------------------------------------------------------------------------------------------------
bool efd::operator==(const efd::Char* s1, const FixedString& s2)
{
    return s2.Equals(s1);
}

//------------------------------------------------------------------------------------------------
bool efd::operator!=(const efd::Char* s1, const FixedString& s2)
{
    return !(s2.Equals(s1));
}

//------------------------------------------------------------------------------------------------
