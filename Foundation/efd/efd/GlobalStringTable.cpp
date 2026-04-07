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

#include <efd/GlobalStringTable.h>

using namespace efd;

GlobalStringTable* GlobalStringTable::ms_pGlobalStringTable = NULL;
const GlobalStringTable::GlobalStringHandle
    GlobalStringTable::NULL_STRING = NULL;
size_t GlobalStringTable::ms_numStrings = 0;
size_t GlobalStringTable::ms_numCollisions = 0;

//-------------------------------------------------------------------------------------------------
GlobalStringTable& GlobalStringTable::Get()
{
    EE_ASSERT_MESSAGE(ms_pGlobalStringTable != NULL, ("The GlobalStringTable is not available "
        "outside of an EE_INIT/EE_SHUTDOWN pair"));
    return *ms_pGlobalStringTable;
}

//-------------------------------------------------------------------------------------------------
void GlobalStringTable::_SDMInit()
{
    ms_pGlobalStringTable = EE_NEW GlobalStringTable();
}

//-------------------------------------------------------------------------------------------------
void GlobalStringTable::_SDMShutdown()
{
    EE_DELETE ms_pGlobalStringTable;
    ms_pGlobalStringTable = NULL;
}
//-------------------------------------------------------------------------------------------------
GlobalStringTable::GlobalStringTable() :
    m_criticalSection("GStrTab")
{
    // Assert that we don't have too many buckets since we're storing the
    // hash value in 16 bits.
    EE_ASSERT(GSTABLE_NUM_GLOBAL_STRING_HASH_BUCKETS < 65536);

    // Make sure we have enough room for storing the hash value and the
    // length. Later on, we'll pack them into a size_t object which must
    // be at least 4B.
    EE_ASSERT(sizeof(size_t) >= 4);

    for (efd::UInt32 ui = 0; ui < GSTABLE_NUM_GLOBAL_STRING_HASH_BUCKETS; ui++)
    {
        m_hashArray[ui].reserve(GSTABLE_NUM_ENTRIES_PER_BUCKET_GROWBY);
    }
}

//-------------------------------------------------------------------------------------------------
GlobalStringTable::GlobalStringHandle GlobalStringTable::AddString(
     const efd::Char* pString)
{
    if (pString == NULL)
    {
        return NULL_STRING;
    }

    size_t stStrLen = strlen(pString);

    // The limit for a fixed string is 65535 since we store the length in
    // 16 bits.
    EE_ASSERT(stStrLen < 65536);

    // Determine if the string already exists in the table
    // Because two threads could enter AddString with the same value
    // concurrently, the entire hash table will need to be locked.
    Get().m_criticalSection.Lock();
    GlobalStringHandle handle = Get().FindString(pString, stStrLen);

    if (handle != NULL_STRING)
    {
        IncRefCount(handle);
        Get().m_criticalSection.Unlock();
        return handle;
    }

    // Add in space for ref count, length, and null terminator.
    size_t stAllocLen = stStrLen + 2*sizeof(size_t) + sizeof(efd::Char);

    // since we need the size_t header to be properly aligned
    if (stAllocLen % sizeof(size_t) != 0)
    {
        stAllocLen += sizeof(size_t) - (stAllocLen % sizeof(size_t));
    }

    void * pvMem = EE_MALLOC(stAllocLen);

    efd::Char* pcMem = (efd::Char*) pvMem + 2*sizeof(size_t);
    handle = pcMem;
    size_t* kMem = (size_t*) pvMem;

    efd::UInt32 hash = HashFunction(pString, stStrLen);

    kMem[0] = 2;
    kMem[1] = (stStrLen << GSTABLE_LEN_SHIFT) |
        ((hash << GSTABLE_HASH_SHIFT) & GSTABLE_HASH_MASK);
    memcpy(pcMem, pString, stStrLen+1);

    Get().InsertString(handle, hash);
    Get().m_criticalSection.Unlock();
    return handle;
}

//-------------------------------------------------------------------------------------------------
efd::UInt32 GlobalStringTable::HashFunction(const efd::Char* pString,
    size_t len)
{
    efd::UInt32 hash = 0;
    // Unroll the loop 4x
    efd::UInt32 uiUnroll = ((efd::UInt32)len) & ~0x3;

    for (efd::UInt32 ui = 0; ui < uiUnroll; ui += 4)
    {
        efd::UInt32 hash0 = *pString;
        hash = (hash << 5) + hash + hash0;
        efd::UInt32 hash1 = *(pString + 1);
        hash = (hash << 5) + hash + hash1;
        efd::UInt32 hash2 = *(pString + 2);
        hash = (hash << 5) + hash + hash2;
        efd::UInt32 hash3 = *(pString + 3);
        hash = (hash << 5) + hash + hash3;
        pString += 4;
    }

    while (*pString)
        hash = (hash << 5) + hash + *pString++;

    return hash % GSTABLE_NUM_GLOBAL_STRING_HASH_BUCKETS;
}

//-------------------------------------------------------------------------------------------------
GlobalStringTable::GlobalStringHandle
    GlobalStringTable::FindString(const efd::Char* pString, size_t len)
{
    efd::UInt32 hash = HashFunction(pString, len);

    m_criticalSection.Lock();

    size_t uiBucketSize = m_hashArray[hash].size();
    for (size_t ui = 0; ui < uiBucketSize; ++ui)
    {
        GlobalStringTable::GlobalStringHandle kPossibleMatchString =
            m_hashArray[hash].at(ui);

        if (kPossibleMatchString == pString ||
            (GetString(kPossibleMatchString) &&
            GetLength(kPossibleMatchString) == len &&
            strcmp(GetString(kPossibleMatchString), pString) == 0))
        {
            m_criticalSection.Unlock();
            return kPossibleMatchString;
        }
    }

    m_criticalSection.Unlock();
    return NULL_STRING;
}

//-------------------------------------------------------------------------------------------------
void GlobalStringTable::InsertString(const GlobalStringHandle& handle,
    efd::UInt32 hash)
{
    EE_ASSERT(ValidateString(handle));

    // Adding and removing strings from the hash table should
    // trip the critical section.
    m_criticalSection.Lock();
    if (m_hashArray[hash].size() != 0)
    {
        ms_numCollisions++;
    }

    m_hashArray[hash].push_back(handle);

    ++ms_numStrings;

#ifdef TEST_HASHING_FUNCTION
    if (m_hashArray[hash].size() > 10)
    {
        EE_OUTPUT_DEBUG_STRING("Found > 10:\n");
        for (efd::UInt32 ui = 0; ui < m_hashArray[hash].size(); ui++)
        {
            efd::Char acString[1024];
            NiSprintf(acString, 1024, "[%d] = \"%s\"\n",
                ui, m_hashArray[hash].at(ui));
            EE_OUTPUT_DEBUG_STRING(acString);
        }
    }
#endif

    m_criticalSection.Unlock();
}

//-------------------------------------------------------------------------------------------------
void GlobalStringTable::RemoveString(const GlobalStringHandle& handle,
    efd::UInt32 hashValue)
{
    const efd::Char* pString = GetString(handle);

    // Adding and removing strings from the hash table should
    // trip the critical section.
    // N.B. The hash value is valid because we grabbed it from DecRefCount
    // before the decrement. However, handle may have been deleted at this
    // point. If so, we will fall through the loop. The only failure case
    // occurs if another thread grabbed the same memory and allocated a string
    // in the same hash bucket with that memory. In that case, the ref count
    // will only be 1 if the other context has decided to delete that string
    // as well. However, it is safe to delete from this context even
    // though we think we're deleting a different string. The second context
    // will pass a valid hash value into this function and fall through the
    // loop because the string has been removed. This error can safely cascade
    // as many times as it occurs, but the table will be valid at any point,
    // and invalid memory will never be dereferenced.
    m_criticalSection.Lock();

    vector<GlobalStringHandle>::iterator i;
    for (i = m_hashArray[hashValue].begin();
        i != m_hashArray[hashValue].end(); i++)
    {
        if (*i == pString)
        {
            size_t* mem = (size_t*)GetRealBufferStart(handle);
            if (GetRefCount(handle) == 1 &&
                AtomicDecrement(mem[0]) == 0)
            {
                EE_FREE(GetRealBufferStart(handle));
                m_hashArray[hashValue].erase(i);
                --ms_numStrings;
            }
            break;
        }
    }
    m_criticalSection.Unlock();
}

//-------------------------------------------------------------------------------------------------
size_t GlobalStringTable::GetBucketSize(efd::UInt32 uiWhichBucket)
{
    EE_ASSERT(uiWhichBucket < GSTABLE_NUM_GLOBAL_STRING_HASH_BUCKETS);
    Get().m_criticalSection.Lock();
    size_t stSize = Get().m_hashArray[uiWhichBucket].size();
    Get().m_criticalSection.Unlock();
    return stSize;
}

//-------------------------------------------------------------------------------------------------
size_t GlobalStringTable::GetMaxBucketSize()
{
    Get().m_criticalSection.Lock();
    size_t stSize = 0;
    for (efd::UInt32 ui = 0; ui < GSTABLE_NUM_GLOBAL_STRING_HASH_BUCKETS; ui++)
    {
        if (stSize < Get().m_hashArray[ui].size())
            stSize = Get().m_hashArray[ui].size();
    }
    Get().m_criticalSection.Unlock();
    return stSize;
}

//-------------------------------------------------------------------------------------------------
