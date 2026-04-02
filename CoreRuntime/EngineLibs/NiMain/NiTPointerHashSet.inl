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
NiTPointerHashSet<TVAL, THASH, TEQUALS>::~NiTPointerHashSet()
{
    // RemoveAll is called from here because it depends on virtual functions
    // implemented in NiTAllocatorHashSet.  It will also be called in the
    // parent destructor, but the set will already be empty.
    NiTPointerHashSet<TVAL, THASH, TEQUALS>::RemoveAll();
}

//--------------------------------------------------------------------------------------------------
template <class TVAL, class THASH, class TEQUALS> inline
NiTHashSetItem<TVAL>* NiTPointerHashSet<TVAL, THASH, TEQUALS>::NewItem()
{
    return (NiTHashSetItem<TVAL>*)NiTHashSetBase<NiTPointerAllocator<size_t>,
        TVAL, THASH, TEQUALS>::m_kAllocator.Allocate();
}

//--------------------------------------------------------------------------------------------------
template <class TVAL, class THASH, class TEQUALS> inline
void NiTPointerHashSet<TVAL,THASH,TEQUALS>::
    DeleteItem(NiTHashSetItem<TVAL>* pkItem)
{
    // set val to zero so that if they are smart pointers
    // their references will be decremented.
    pkItem->m_val = 0;
    NiTHashSetBase<NiTPointerAllocator<size_t>,
        TVAL, THASH, TEQUALS>::m_kAllocator.Deallocate(pkItem);
}

//--------------------------------------------------------------------------------------------------
