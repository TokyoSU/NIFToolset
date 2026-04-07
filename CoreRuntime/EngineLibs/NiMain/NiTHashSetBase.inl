// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2010 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include <NiUniversalTypes.h>
#include <NiDebug.h>

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::
    NiTHashSetBase(NiUInt32 uiHashSize)
{
    EE_ASSERT(uiHashSize > 0);

    m_uiHashSize = uiHashSize;
    m_kAllocator.m_uiCount = 0;

    NiUInt32 uiSize = sizeof(NiTHashSetItem<TVAL>*) * m_uiHashSize;
    m_ppkHashTable = (NiTHashSetItem<TVAL>**)NiMalloc(uiSize);
    EE_ASSERT(m_ppkHashTable);
    memset(m_ppkHashTable, 0, uiSize);
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::~NiTHashSetBase()
{
    RemoveAll();
    NiFree(m_ppkHashTable);
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline NiUInt32 NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::
    GetCount() const
{
    return m_kAllocator.m_uiCount;
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline bool NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::
    IsEmpty() const
{
    return m_kAllocator.m_uiCount == 0;
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline void NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::
    Add(TVAL val)
{
    // look up hash table location for val
    NiUInt32 uiIndex = THASH::KeyToHashIndex(val, m_uiHashSize);
    NiTHashSetItem<TVAL>* pkItem = m_ppkHashTable[uiIndex];

    // search list at hash table location for val
    while (pkItem != NULL)
    {
        if (TEQUALS::IsKeysEqual(val, pkItem->m_val))
        {
            // item already in hash table
            return;
        }
        pkItem = pkItem->m_pkNext;
    }

    // add object to beginning of list for this hash table index
    pkItem = (NiTHashSetItem<TVAL>*)NewItem();
    EE_ASSERT(pkItem);

    SetValue(pkItem, val);
    pkItem->m_pkNext = m_ppkHashTable[uiIndex];
    m_ppkHashTable[uiIndex] = pkItem;
    m_kAllocator.m_uiCount++;
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline bool NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::
    Remove(TVAL val)
{
    // look up hash table location for val
    NiUInt32 uiIndex = THASH::KeyToHashIndex(val, m_uiHashSize);
    NiTHashSetItem<TVAL>* pkItem = m_ppkHashTable[uiIndex];

    // search list at hash table location for val
    if (pkItem != NULL)
    {
        if (TEQUALS::IsKeysEqual(val, pkItem->m_val))
        {
            // item at front of list, remove it
            m_ppkHashTable[uiIndex] = pkItem->m_pkNext;
            ClearValue(pkItem);
            DeleteItem(pkItem);
            m_kAllocator.m_uiCount--;
            return true;
        }
        else
        {
            // search rest of list for item
            NiTHashSetItem<TVAL>* pkPrev = pkItem;
            NiTHashSetItem<TVAL>* pkCurr = pkPrev->m_pkNext;
            while (pkCurr != NULL && !TEQUALS::IsKeysEqual(val, pkCurr->m_val))
            {
                pkPrev = pkCurr;
                pkCurr = pkCurr->m_pkNext;
            }
            if (pkCurr != NULL)
            {
                // found the item, remove it
                pkPrev->m_pkNext = pkCurr->m_pkNext;
                ClearValue(pkCurr);
                DeleteItem(pkCurr);
                m_kAllocator.m_uiCount--;
                return true;
            }
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline void NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::RemoveAll()
{
    for (NiUInt32 ui = 0; ui < m_uiHashSize; ui++)
    {
        while (m_ppkHashTable[ui] != NULL)
        {
            NiTHashSetItem<TVAL>* pkSave = m_ppkHashTable[ui];
            m_ppkHashTable[ui] = m_ppkHashTable[ui]->m_pkNext;
            ClearValue(pkSave);
            DeleteItem(pkSave);
        }
    }

    m_kAllocator.m_uiCount = 0;
}

//--------------------------------------------------------------------------------------------------
template <class TVAL>
inline NiUInt32 NiTHashSetHashFunctor<TVAL>::
    KeyToHashIndex(TVAL val, unsigned int uiTableSize)
{
    // Modular arithmetic is used for building key. If a different scheme
    // is preferred, define your own templated class.
    return (NiUInt32) (((size_t) val) % uiTableSize);
}

//--------------------------------------------------------------------------------------------------
template <class TVAL>
inline bool NiTHashSetEqualsFunctor<TVAL>::IsKeysEqual(TVAL val1, TVAL val2)
{
    return val1 == val2;
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline void NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::
    SetValue(NiTHashSetItem<TVAL>* pkItem, TVAL val)
{
    pkItem->m_val = val;
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline void NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::
    ClearValue(NiTHashSetItem<TVAL>* /* pkItem */)
{
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline NiTHashSetIterator NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::
    GetFirstPos() const
{
    for (NiUInt32 ui = 0; ui < m_uiHashSize; ui++)
    {
        if (m_ppkHashTable[ui] != NULL)
            return m_ppkHashTable[ui];
    }
    return NULL;
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline TVAL& NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::
    GetNext(NiTHashSetIterator& pos) const
{
    NiTHashSetItem<TVAL>* pkItem = (NiTHashSetItem<TVAL>*) pos;

    TVAL& val = pkItem->m_val;

    if (pkItem->m_pkNext != NULL)
    {
        pos = pkItem->m_pkNext;
        return val;
    }

    NiUInt32 ui = THASH::KeyToHashIndex(pkItem->m_val, m_uiHashSize);
    for (++ui; ui < m_uiHashSize; ui++)
    {
        pkItem = m_ppkHashTable[ui];
        if (pkItem != NULL)
        {
            pos = pkItem;
            return val;
        }
    }

    pos = 0;

    return val;
}

//--------------------------------------------------------------------------------------------------
template <class TheAllocator, class TVAL, class THASH, class TEQUALS>
inline void NiTHashSetBase<TheAllocator, TVAL, THASH, TEQUALS>::
    Resize(NiUInt32 uiNewHashSize)
{
    EE_ASSERT(uiNewHashSize > 0);

    // Allocate a new hash array
    NiUInt32 uiNewSize = sizeof(NiTHashSetItem<TVAL>*) * uiNewHashSize;
    NiTHashSetItem<TVAL>** ppkNewHashTable =
        (NiTHashSetItem<TVAL>**)NiMalloc(uiNewSize);
    EE_ASSERT(ppkNewHashTable);
    memset(ppkNewHashTable, 0, uiNewSize);

    // Go through all entries in the existing hash array and transfer the
    // entire entry to the new hash array.
    NiUInt32 uiNewCount = 0;
    for (NiUInt32 ui = 0; ui < m_uiHashSize; ui++)
    {
        while (m_ppkHashTable[ui] != NULL)
        {
            // Remove the entry from the existing array
            NiTHashSetItem<TVAL>* pkItem = m_ppkHashTable[ui];
            m_ppkHashTable[ui] = pkItem->m_pkNext;
            m_kAllocator.m_uiCount--;

            // Clear values in the entry
            pkItem->m_pkNext = NULL;

            // Insert the entry in the new map
            NiUInt32 uiIndex =
                THASH::KeyToHashIndex(pkItem->m_val, uiNewHashSize);
            pkItem->m_pkNext = ppkNewHashTable[uiIndex];
            ppkNewHashTable[uiIndex] = pkItem;
            uiNewCount++;
        }
    }

    // At the end, the existing hash array should be empty, and we can just
    // delete it.
    EE_ASSERT(m_kAllocator.m_uiCount == 0);
    NiFree(m_ppkHashTable);

    // Set the updated values
    m_ppkHashTable = ppkNewHashTable;
    m_uiHashSize = uiNewHashSize;
    m_kAllocator.m_uiCount = uiNewCount;
}

//--------------------------------------------------------------------------------------------------
