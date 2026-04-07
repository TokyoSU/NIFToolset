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

//--------------------------------------------------------------------------------------------------
template <class TVAL, class THASH, class TEQUALS> inline
NiTHashSet<TVAL, THASH, TEQUALS>::~NiTHashSet()
{
    // RemoveAll is called from here because it depends on virtual functions
    // implemented in NiTAllocatorMap.  It will also be called in the
    // parent destructor, but the set will already be empty.
    NiTHashSet<TVAL, THASH, TEQUALS>::RemoveAll();
}

//--------------------------------------------------------------------------------------------------
template <class TVAL, class THASH, class TEQUALS> inline
NiTHashSetItem<TVAL>* NiTHashSet<TVAL, THASH, TEQUALS>::NewItem()
{
    return (NiTHashSetItem<TVAL>*)NiTHashSetBase<NiTDefaultAllocator<TVAL>,
        TVAL, THASH, TEQUALS >::m_kAllocator.Allocate();
}

//--------------------------------------------------------------------------------------------------
template <class TVAL, class THASH, class TEQUALS> inline
void NiTHashSet<TVAL, THASH, TEQUALS>::
    DeleteItem(NiTHashSetItem<TVAL>* pkItem)
{
    // set key and val to zero so that if they are smart pointers
    // their references will be decremented.
    pkItem->m_val = 0;
    NiTHashSetBase<NiTDefaultAllocator<TVAL>,
        TVAL, THASH, TEQUALS >::m_kAllocator.Deallocate(pkItem);
}

//--------------------------------------------------------------------------------------------------
template <class TVAL, class THASH, class TEQUALS>
const NiUInt32 NiTHashSet<TVAL, THASH, TEQUALS>::NUM_PRIMES = 18;

//--------------------------------------------------------------------------------------------------
template <class TVAL, class THASH, class TEQUALS>
const NiUInt32 NiTHashSet<TVAL, THASH, TEQUALS>::PRIMES[18] = {
    7, 13, 31, 61, 127, 257, 509, 1021, 2039, 4093, 8191, 16381, 32749,
    65521, 131071, 262139, 524287, 999983};
//--------------------------------------------------------------------------------------------------
template <class TVAL, class THASH, class TEQUALS>
inline NiUInt32 NiTHashSet<TVAL, THASH, TEQUALS>::
    NextPrime(const NiUInt32 uiTarget)
{
    NiUInt32 ui;
    for (ui = 0; ui < NUM_PRIMES - 1 && PRIMES[ui] < uiTarget; ui++) { }
    return PRIMES[ui];
}

//--------------------------------------------------------------------------------------------------
